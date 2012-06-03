// sv_main.c -- server main program

#include "qwsvdef.h"

unsigned clients_multicast;

#define CHAN_AUTO   0
#define CHAN_WEAPON 1
#define CHAN_VOICE  2
#define CHAN_ITEM   3
#define CHAN_BODY   4
#define PHS_OVERRIDE_R 8

/*
=============================================================================

Con_Printf redirection

=============================================================================
*/

char outputbuf[8000];

redirect_t sv_redirected;

extern Cvar* sv_phs;
extern Cvar* sv_namedistance;

/*
==================
SV_FlushRedirect
==================
*/
void SV_FlushRedirect(void)
{
	char send[8000 + 6];

	if (sv_redirected == RD_PACKET)
	{
		send[0] = 0xff;
		send[1] = 0xff;
		send[2] = 0xff;
		send[3] = 0xff;
		send[4] = A2C_PRINT;
		Com_Memcpy(send + 5, outputbuf, String::Length(outputbuf) + 1);

		NET_SendPacket(String::Length(send) + 1, send, net_from);
	}
	else if (sv_redirected == RD_CLIENT)
	{
		host_client->netchan.message.WriteByte(h2svc_print);
		host_client->netchan.message.WriteByte(PRINT_HIGH);
		host_client->netchan.message.WriteString2(outputbuf);
	}

	// clear it
	outputbuf[0] = 0;
}


/*
==================
SV_BeginRedirect

  Send Con_Printf data to the remote client
  instead of the console
==================
*/
void SV_BeginRedirect(redirect_t rd)
{
	sv_redirected = rd;
	outputbuf[0] = 0;
}

void SV_EndRedirect(void)
{
	SV_FlushRedirect();
	sv_redirected = RD_NONE;
}


/*
================
Con_Printf

Handles cursor positioning, line wrapping, etc
================
*/
void Con_Printf(const char* fmt, ...)
{
	va_list argptr;
	char msg[MAXPRINTMSG];

	va_start(argptr,fmt);
	Q_vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end(argptr);

	// add to redirected message
	if (sv_redirected)
	{
		if (String::Length(msg) + String::Length(outputbuf) > (int)sizeof(outputbuf) - 1)
		{
			SV_FlushRedirect();
		}
		String::Cat(outputbuf, sizeof(outputbuf), msg);
		return;
	}

	Sys_Print(msg);	// also echo to debugging console
	if (sv_logfile)
	{
		FS_Printf(sv_logfile, "%s", msg);
	}
}

/*
================
Con_DPrintf

A Con_Printf that only shows up if the "developer" cvar is set
================
*/
void Con_DPrintf(const char* fmt, ...)
{
	va_list argptr;
	char msg[MAXPRINTMSG];

	if (!com_developer || !com_developer->value)
	{
		return;
	}

	va_start(argptr,fmt);
	Q_vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end(argptr);

	Con_Printf("%s", msg);
}

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/


/*
=================
SV_ClientPrintf

Sends text across to be displayed if the level passes
=================
*/
void SV_ClientPrintf(client_t* cl, int level, const char* fmt, ...)
{
	va_list argptr;
	char string[1024];

	if (level < cl->messagelevel)
	{
		return;
	}

	va_start(argptr,fmt);
	Q_vsnprintf(string, 1024, fmt, argptr);
	va_end(argptr);

	cl->netchan.message.WriteByte(h2svc_print);
	cl->netchan.message.WriteByte(level);
	cl->netchan.message.WriteString2(string);
}


/*
=================
SV_BroadcastPrintf

Sends text to all active clients
=================
*/
void SV_BroadcastPrintf(int level, const char* fmt, ...)
{
	va_list argptr;
	char string[1024];
	client_t* cl;
	int i;

	va_start(argptr,fmt);
	Q_vsnprintf(string, 1024, fmt, argptr);
	va_end(argptr);

	Con_Printf("%s", string);	// print to the console

	for (i = 0, cl = svs.clients; i < HWMAX_CLIENTS; i++, cl++)
	{
		if (level < cl->messagelevel)
		{
			continue;
		}
		if (!cl->state)
		{
			continue;
		}
		cl->netchan.message.WriteByte(h2svc_print);
		cl->netchan.message.WriteByte(level);
		cl->netchan.message.WriteString2(string);
	}
}

/*
=================
SV_BroadcastCommand

Sends text to all active clients
=================
*/
void SV_BroadcastCommand(const char* fmt, ...)
{
	va_list argptr;
	char string[1024];

	if (!sv.state)
	{
		return;
	}
	va_start(argptr,fmt);
	Q_vsnprintf(string, 1024, fmt, argptr);
	va_end(argptr);

	sv.reliable_datagram.WriteByte(h2svc_stufftext);
	sv.reliable_datagram.WriteString2(string);
}


/*
=================
SV_Multicast

Sends the contents of sv.multicast to a subset of the clients,
then clears sv.multicast.

MULTICAST_ALL	same as broadcast
MULTICAST_PVS	send to clients potentially visible from org
MULTICAST_PHS	send to clients potentially hearable from org
=================
*/
void SV_Multicast(vec3_t origin, int to)
{
	client_t* client;
	byte* mask;
	int leafnum;
	int j;
	qboolean reliable;
	vec3_t adjust_origin;

	clients_multicast = 0;

	leafnum = CM_PointLeafnum(origin);

	reliable = false;

	switch (to)
	{
	case MULTICAST_ALL_R:
		reliable = true;	// intentional fallthrough
	case MULTICAST_ALL:
		mask = NULL;
		break;

	case MULTICAST_PHS_R:
		reliable = true;	// intentional fallthrough
	case MULTICAST_PHS:
		mask = CM_ClusterPHS(CM_LeafCluster(leafnum));
		break;

	case MULTICAST_PVS_R:
		reliable = true;	// intentional fallthrough
	case MULTICAST_PVS:
		mask = CM_ClusterPVS(CM_LeafCluster(leafnum));
		break;

	default:
		mask = NULL;
		SV_Error("SV_Multicast: bad to:%i", to);
	}

	// send the data to all relevent clients
	for (j = 0, client = svs.clients; j < HWMAX_CLIENTS; j++, client++)
	{
		if (client->state != CS_ACTIVE)
		{
			continue;
		}

		VectorCopy(client->qh_edict->GetOrigin(), adjust_origin);
		adjust_origin[2] += 16;
		if (mask)
		{
			leafnum = CM_PointLeafnum(adjust_origin);
			// if (leaf != sv.worldmodel->leafs)
			{
				leafnum = CM_LeafCluster(leafnum);
				if (leafnum < 0 || !(mask[leafnum >> 3] & (1 << (leafnum & 7))))
				{
					continue;
				}
			}
		}

		clients_multicast |= 1l << j;

		if (reliable)
		{
			client->netchan.message.WriteData(sv.multicast._data, sv.multicast.cursize);
		}
		else
		{
			client->datagram.WriteData(sv.multicast._data, sv.multicast.cursize);
		}
	}

	sv.multicast.Clear();
}

/*
=================
SV_MulticastSpecific

Sends the contents of sv.multicast to a subset of the clients,
then clears sv.multicast.
=================
*/
void SV_MulticastSpecific(unsigned clients, qboolean reliable)
{
	client_t* client;
	int j;

	clients_multicast = 0;

	// send the data to all relevent clients
	for (j = 0, client = svs.clients; j < HWMAX_CLIENTS; j++, client++)
	{
		if (client->state != CS_ACTIVE)
		{
			continue;
		}

		if ((1l << j) & clients)
		{
			clients_multicast |= 1l << j;

			if (reliable)
			{
				client->netchan.message.WriteData(sv.multicast._data, sv.multicast.cursize);
			}
			else
			{
				client->datagram.WriteData(sv.multicast._data, sv.multicast.cursize);
			}
		}
	}

	sv.multicast.Clear();
}

/*
==================
SV_StopSound
==================
*/
void SV_StopSound(qhedict_t* entity, int channel)
{
	int ent,i;
	vec3_t origin;

	ent = NUM_FOR_EDICT(entity);
	channel = (ent << 3) | channel;

	// use the entity origin unless it is a bmodel
	if (entity->GetSolid() == SOLID_BSP)
	{
		for (i = 0; i < 3; i++)	//FIXME: This may not work- should be using (absmin + absmax)*0.5?
			origin[i] = entity->GetOrigin()[i] + 0.5 * (entity->GetMins()[i] + entity->GetMaxs()[i]);
	}
	else
	{
		VectorCopy(entity->GetOrigin(), origin);
	}

	sv.multicast.WriteByte(h2svc_stopsound);
	sv.multicast.WriteShort(channel);
	SV_Multicast(origin, MULTICAST_ALL_R);
}

/*
==================
SV_UpdateSoundPos
==================
*/
void SV_UpdateSoundPos(qhedict_t* entity, int channel)
{
	int ent;
	int i;
	vec3_t origin;

	ent = NUM_FOR_EDICT(entity);
	channel = (ent << 3) | channel;

	// use the entity origin unless it is a bmodel
	if (entity->GetSolid() == SOLID_BSP)
	{
		for (i = 0; i < 3; i++)	//FIXME: This may not work- should be using (absmin + absmax)*0.5?
			origin[i] = entity->GetOrigin()[i] + 0.5 * (entity->GetMins()[i] + entity->GetMaxs()[i]);
	}
	else
	{
		VectorCopy(entity->GetOrigin(), origin);
	}

	sv.multicast.WriteByte(hwsvc_sound_update_pos);
	sv.multicast.WriteShort(channel);
	for (i = 0; i < 3; i++)
		sv.multicast.WriteCoord(entity->GetOrigin()[i] + 0.5 * (entity->GetMins()[i] + entity->GetMaxs()[i]));
	SV_Multicast(origin, MULTICAST_PHS);
}

/*
==================
SV_StartSound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
allready running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.  (max 4 attenuation)

==================
*/
void SV_StartSound(qhedict_t* entity, int channel, const char* sample, int volume,
	float attenuation)
{
	int sound_num;
	int field_mask;
	int i;
	int ent;
	vec3_t origin;
	qboolean use_phs;
	qboolean reliable = false;

	if (volume < 0 || volume > 255)
	{
		SV_Error("SV_StartSound: volume = %i", volume);
	}

	if (attenuation < 0 || attenuation > 4)
	{
		SV_Error("SV_StartSound: attenuation = %f", attenuation);
	}

	if (channel < 0 || channel > 15)
	{
		SV_Error("SV_StartSound: channel = %i", channel);
	}

// find precache number for sound
	for (sound_num = 1; sound_num < MAX_SOUNDS_HW &&
		 sv.sound_precache[sound_num]; sound_num++)
		if (!String::Cmp(sample, sv.sound_precache[sound_num]))
		{
			break;
		}

	if (sound_num == MAX_SOUNDS_HW || !sv.sound_precache[sound_num])
	{
		Con_Printf("SV_StartSound: %s not precacheed\n", sample);
		return;
	}

	ent = NUM_FOR_EDICT(entity);

	if ((channel & PHS_OVERRIDE_R) || !sv_phs->value)	// no PHS flag
	{
		if (channel & PHS_OVERRIDE_R)	//PHS_OVERRIDE_R = 8
		{
			reliable = true;// sounds that break the phs are reliable
		}
		use_phs = false;
		channel &= 7;	//clear out the PHS_OVERRIDE_R flag
	}
	else
	{
		use_phs = true;
	}

//	if (channel == CHAN_BODY || channel == CHAN_VOICE)
//		reliable = true;

	channel = (ent << 3) | channel;

	field_mask = 0;
	if (volume != DEFAULT_SOUND_PACKET_VOLUME)
	{
		channel |= SND_VOLUME;
	}
	if (attenuation != DEFAULT_SOUND_PACKET_ATTENUATION)
	{
		channel |= SND_ATTENUATION;
	}

	// use the entity origin unless it is a bmodel
	if (entity->GetSolid() == SOLID_BSP)
	{
		for (i = 0; i < 3; i++)	//FIXME: This may not work- should be using (absmin + absmax)*0.5?
			origin[i] = entity->GetOrigin()[i] + 0.5 * (entity->GetMins()[i] + entity->GetMaxs()[i]);
	}
	else
	{
		VectorCopy(entity->GetOrigin(), origin);
	}

	sv.multicast.WriteByte(h2svc_sound);
	sv.multicast.WriteShort(channel);
	if (channel & SND_VOLUME)
	{
		sv.multicast.WriteByte(volume);
	}
	if (channel & SND_ATTENUATION)
	{
		sv.multicast.WriteByte(attenuation * 32);
	}
	sv.multicast.WriteByte(sound_num);
	for (i = 0; i < 3; i++)
		sv.multicast.WriteCoord(origin[i]);

	if (use_phs)
	{
		SV_Multicast(origin, reliable ? MULTICAST_PHS_R : MULTICAST_PHS);
	}
	else
	{
		SV_Multicast(origin, reliable ? MULTICAST_ALL_R : MULTICAST_ALL);
	}
}


/*
==================
SV_StartParticle

Make sure the event gets sent to all clients
==================
*/
void SV_StartParticle(vec3_t org, vec3_t dir, int color, int count)
{
	int i, v;

	sv.multicast.WriteByte(h2svc_particle);
	sv.multicast.WriteCoord(org[0]);
	sv.multicast.WriteCoord(org[1]);
	sv.multicast.WriteCoord(org[2]);
	for (i = 0; i < 3; i++)
	{
		v = dir[i] * 16;
		if (v > 127)
		{
			v = 127;
		}
		else if (v < -128)
		{
			v = -128;
		}
		sv.multicast.WriteChar(v);
	}
	sv.multicast.WriteByte(count);
	sv.multicast.WriteByte(color);

	SV_Multicast(org, MULTICAST_PVS);
}

/*
==================
SV_StartParticle2

Make sure the event gets sent to all clients
==================
*/
void SV_StartParticle2(vec3_t org, vec3_t dmin, vec3_t dmax, int color, int effect, int count)
{
	sv.multicast.WriteByte(hwsvc_particle2);
	sv.multicast.WriteCoord(org[0]);
	sv.multicast.WriteCoord(org[1]);
	sv.multicast.WriteCoord(org[2]);
	sv.multicast.WriteFloat(dmin[0]);
	sv.multicast.WriteFloat(dmin[1]);
	sv.multicast.WriteFloat(dmin[2]);
	sv.multicast.WriteFloat(dmax[0]);
	sv.multicast.WriteFloat(dmax[1]);
	sv.multicast.WriteFloat(dmax[2]);

	sv.multicast.WriteShort(color);
	sv.multicast.WriteByte(count);
	sv.multicast.WriteByte(effect);

	SV_Multicast(org, MULTICAST_PVS);
}

/*
==================
SV_StartParticle3

Make sure the event gets sent to all clients
==================
*/
void SV_StartParticle3(vec3_t org, vec3_t box, int color, int effect, int count)
{
	sv.multicast.WriteByte(hwsvc_particle3);
	sv.multicast.WriteCoord(org[0]);
	sv.multicast.WriteCoord(org[1]);
	sv.multicast.WriteCoord(org[2]);
	sv.multicast.WriteByte(box[0]);
	sv.multicast.WriteByte(box[1]);
	sv.multicast.WriteByte(box[2]);

	sv.multicast.WriteShort(color);
	sv.multicast.WriteByte(count);
	sv.multicast.WriteByte(effect);

	SV_Multicast(org, MULTICAST_PVS);
}

/*
==================
SV_StartParticle4

Make sure the event gets sent to all clients
==================
*/
void SV_StartParticle4(vec3_t org, float radius, int color, int effect, int count)
{
	sv.multicast.WriteByte(hwsvc_particle4);
	sv.multicast.WriteCoord(org[0]);
	sv.multicast.WriteCoord(org[1]);
	sv.multicast.WriteCoord(org[2]);
	sv.multicast.WriteByte(radius);

	sv.multicast.WriteShort(color);
	sv.multicast.WriteByte(count);
	sv.multicast.WriteByte(effect);

	SV_Multicast(org, MULTICAST_PVS);
}

void SV_StartRainEffect(vec3_t org, vec3_t e_size, int x_dir, int y_dir, int color, int count)
{
	sv.multicast.WriteByte(hwsvc_raineffect);
	sv.multicast.WriteCoord(org[0]);
	sv.multicast.WriteCoord(org[1]);
	sv.multicast.WriteCoord(org[2]);
	sv.multicast.WriteCoord(e_size[0]);
	sv.multicast.WriteCoord(e_size[1]);
	sv.multicast.WriteCoord(e_size[2]);
	sv.multicast.WriteAngle(x_dir);
	sv.multicast.WriteAngle(y_dir);
	sv.multicast.WriteShort(color);
	sv.multicast.WriteShort(count);

	SV_Multicast(org, MULTICAST_PVS);
}


/*
===============================================================================

FRAME UPDATES

===============================================================================
*/

//int		sv_nailmodel, sv_supernailmodel, sv_playermodel[MAX_PLAYER_CLASS];
int sv_magicmissmodel, sv_playermodel[MAX_PLAYER_CLASS];
int sv_ravenmodel, sv_raven2model;

void SV_FindModelNumbers(void)
{
	int i;

//	sv_nailmodel = -1;
//	sv_supernailmodel = -1;
	sv_playermodel[0] = -1;
	sv_playermodel[1] = -1;
	sv_playermodel[2] = -1;
	sv_playermodel[3] = -1;
	sv_playermodel[4] = -1;
	sv_magicmissmodel = -1;
	sv_ravenmodel = -1;
	sv_raven2model = -1;

	for (i = 0; i < MAX_MODELS_H2; i++)
	{
		if (!sv.model_precache[i])
		{
			break;
		}
//		if (!String::Cmp(sv.model_precache[i],"progs/spike.mdl"))
//			sv_nailmodel = i;
//		if (!String::Cmp(sv.model_precache[i],"progs/s_spike.mdl"))
//			sv_supernailmodel = i;
		if (!String::Cmp(sv.model_precache[i],"models/paladin.mdl"))
		{
			sv_playermodel[0] = i;
		}
		if (!String::Cmp(sv.model_precache[i],"models/crusader.mdl"))
		{
			sv_playermodel[1] = i;
		}
		if (!String::Cmp(sv.model_precache[i],"models/necro.mdl"))
		{
			sv_playermodel[2] = i;
		}
		if (!String::Cmp(sv.model_precache[i],"models/assassin.mdl"))
		{
			sv_playermodel[3] = i;
		}
		if (!String::Cmp(sv.model_precache[i],"models/succubus.mdl"))
		{
			sv_playermodel[4] = i;
		}
		if (!String::Cmp(sv.model_precache[i],"models/ball.mdl"))
		{
			sv_magicmissmodel = i;
		}
		if (!String::Cmp(sv.model_precache[i],"models/ravproj.mdl"))
		{
			sv_ravenmodel = i;
		}
		if (!String::Cmp(sv.model_precache[i],"models/vindsht1.mdl"))
		{
			sv_raven2model = i;
		}
	}
}


/*
==================
SV_WriteClientdataToMessage

==================
*/
void SV_WriteClientdataToMessage(client_t* client, QMsg* msg)
{
	int i;
	qhedict_t* other;
	qhedict_t* ent;

	ent = client->qh_edict;

	// send the chokecount for cl_netgraph
	if (client->qh_chokecount)
	{
		msg->WriteByte(hwsvc_chokecount);
		msg->WriteByte(client->qh_chokecount);
		client->qh_chokecount = 0;
	}

	// send a damage message if the player got hit this frame
	if (ent->GetDmgTake() || ent->GetDmgSave())
	{
		other = PROG_TO_EDICT(ent->GetDmgInflictor());
		msg->WriteByte(h2svc_damage);
		msg->WriteByte(ent->GetDmgSave());
		msg->WriteByte(ent->GetDmgTake());
		for (i = 0; i < 3; i++)
			msg->WriteCoord(other->GetOrigin()[i] + 0.5 * (other->GetMins()[i] + other->GetMaxs()[i]));

		ent->SetDmgTake(0);
		ent->SetDmgSave(0);
	}

	// a fixangle might get lost in a dropped packet.  Oh well.
	if (ent->GetFixAngle())
	{
		msg->WriteByte(h2svc_setangle);
		for (i = 0; i < 3; i++)
			msg->WriteAngle(ent->GetAngles()[i]);
		ent->SetFixAngle(0);
	}

	// if the player has a target, send its info...
	if (ent->GetTargDist())
	{
		msg->WriteByte(hwsvc_targetupdate);
		msg->WriteByte(ent->GetTargAng());
		msg->WriteByte(ent->GetTargPitch());
		msg->WriteByte((ent->GetTargDist() < 255.0) ? (int)(ent->GetTargDist()) : 255);
	}
}

/*
=======================
SV_UpdateClientStats

Performs a delta update of the stats array.  This should only be performed
when a reliable message can be delivered this frame.
=======================
*/
void SV_UpdateClientStats(client_t* client)
{
	qhedict_t* ent;
	int stats[MAX_CL_STATS];
	int i;

	ent = client->qh_edict;
	Com_Memset(stats, 0, sizeof(stats));

	// if we are a spectator and we are tracking a player, we get his stats
	// so our status bar reflects his
	if (client->qh_spectator && client->qh_spec_track > 0)
	{
		ent = svs.clients[client->qh_spec_track - 1].qh_edict;
	}

	stats[STAT_HEALTH] = 0;	//ent->v.health;
	stats[STAT_WEAPON] = SV_ModelIndex(PR_GetString(ent->GetWeaponModel()));
	stats[STAT_AMMO] = 0;	//ent->v.currentammo;
	stats[STAT_ARMOR] = 0;	//ent->v.armorvalue;
	stats[STAT_SHELLS] = 0;	//ent->v.ammo_shells;
	stats[STAT_NAILS] = 0;	//ent->v.ammo_nails;
	stats[STAT_ROCKETS] = 0;//ent->v.ammo_rockets;
	stats[STAT_CELLS] = 0;	//ent->v.ammo_cells;
	if (!client->qh_spectator)
	{
		stats[STAT_ACTIVEWEAPON] = 0;	//ent->v.weapon;
	}
	// stuff the sigil bits into the high bits of items for sbar
	stats[STAT_ITEMS] = 0;	//(int)ent->v.items | ((int)pr_global_struct->serverflags << 28);

	for (i = 0; i < MAX_CL_STATS; i++)
		if (stats[i] != client->qh_stats[i])
		{
			client->qh_stats[i] = stats[i];
			if (stats[i] >= 0 && stats[i] <= 255)
			{
				client->netchan.message.WriteByte(h2svc_updatestat);
				client->netchan.message.WriteByte(i);
				client->netchan.message.WriteByte(stats[i]);
			}
			else
			{
				client->netchan.message.WriteByte(hwsvc_updatestatlong);
				client->netchan.message.WriteByte(i);
				client->netchan.message.WriteLong(stats[i]);
			}
		}
}

/*
=======================
SV_SendClientDatagram
=======================
*/
qboolean SV_SendClientDatagram(client_t* client)
{
	byte buf[MAX_DATAGRAM_HW];
	QMsg msg;

	msg.InitOOB(buf, sizeof(buf));
	msg.allowoverflow = true;

	// add the client specific data to the datagram
	SV_WriteClientdataToMessage(client, &msg);

	// send over all the objects that are in the PVS
	// this will include clients, a packetentities, and
	// possibly a nails update
	SV_WriteEntitiesToClient(client, &msg);

	// copy the accumulated multicast datagram
	// for this client out to the message
	if (client->datagram.overflowed)
	{
		Con_Printf("WARNING: datagram overflowed for %s\n", client->name);
	}
	else
	{
		msg.WriteData(client->datagram._data, client->datagram.cursize);
	}
	client->datagram.Clear();

	// send deltas over reliable stream
	if (Netchan_CanReliable(&client->netchan))
	{
		SV_UpdateClientStats(client);
	}

	if (msg.overflowed)
	{
		Con_Printf("WARNING: msg overflowed for %s\n", client->name);
		msg.Clear();
	}

	// send the datagram
	Netchan_Transmit(&client->netchan, msg.cursize, buf);

	return true;
}

static qboolean ValidToShowName(qhedict_t* edict)
{
	if (edict->GetDeadFlag())
	{
		return false;
	}
	if ((int)edict->GetEffects() & H2EF_NODRAW)
	{
		return false;
	}
	return true;
}

static void UpdatePIV(void)
{
	int i, j;
	client_t* client;
	q1trace_t trace;
	vec3_t adjust_org1, adjust_org2, distvec;
	float save_hull, dist;

	for (i = 0, host_client = svs.clients; i < HWMAX_CLIENTS; i++, host_client++)
	{
		host_client->hw_PIV = 0;
	}

	for (i = 0, host_client = svs.clients; i < HWMAX_CLIENTS; i++, host_client++)
	{
		if (host_client->state != CS_ACTIVE || host_client->qh_spectator)
		{
			continue;
		}

		VectorCopy(host_client->qh_edict->GetOrigin(), adjust_org1);
		adjust_org1[2] += 24;

		save_hull = host_client->qh_edict->GetHull();
		host_client->qh_edict->SetHull(0);

		for (j = i + 1, client = host_client + 1; j < HWMAX_CLIENTS; j++, client++)
		{
			if (client->state != CS_ACTIVE || client->qh_spectator)
			{
				continue;
			}

			VectorSubtract(client->qh_edict->GetOrigin(), host_client->qh_edict->GetOrigin(), distvec);
			dist = VectorNormalize(distvec);
			if (dist > sv_namedistance->value)
			{
//				Con_Printf("dist %f\n", dist);
				continue;
			}

			VectorCopy(client->qh_edict->GetOrigin(), adjust_org2);
			adjust_org2[2] += 24;

			trace = SV_Move(adjust_org1, vec3_origin, vec3_origin, adjust_org2, false, host_client->qh_edict);
			if (EDICT_NUM(trace.entityNum) == client->qh_edict)
			{	//can see each other, check for invisible, dead
				if (ValidToShowName(client->qh_edict))
				{
					host_client->hw_PIV |= 1 << j;
				}
				if (ValidToShowName(host_client->qh_edict))
				{
					client->hw_PIV |= 1 << i;
				}
			}
		}
		host_client->qh_edict->SetHull(save_hull);
	}
}

/*
=======================
SV_UpdateToReliableMessages
=======================
*/
void SV_UpdateToReliableMessages(void)
{
	int i, j;
	client_t* client;
	eval_t* val;
	qhedict_t* ent;
	qboolean CheckPIV = false;

//	Con_Printf("SV_UpdateToReliableMessages\n");
	if (sv.time - sv.next_PIV_time >= 1)
	{
		sv.next_PIV_time = sv.time + 1;
		CheckPIV = true;
		UpdatePIV();
	}

// check for changes to be sent over the reliable streams to all clients
	for (i = 0, host_client = svs.clients; i < HWMAX_CLIENTS; i++, host_client++)
	{
		if (host_client->state != CS_ACTIVE)
		{
			continue;
		}
		if (host_client->qh_sendinfo)
		{
			host_client->qh_sendinfo = false;
			SV_FullClientUpdate(host_client, &sv.reliable_datagram);
		}
		if (host_client->qh_old_frags != host_client->qh_edict->GetFrags())
		{
			for (j = 0, client = svs.clients; j < HWMAX_CLIENTS; j++, client++)
			{
				if (client->state < CS_CONNECTED)
				{
					continue;
				}
//Con_Printf("SV_UpdateToReliableMessages:  Updated frags for client %d to %d\n", i, j);
				client->netchan.message.WriteByte(hwsvc_updatedminfo);
				client->netchan.message.WriteByte(i);
				client->netchan.message.WriteShort(host_client->qh_edict->GetFrags());
				client->netchan.message.WriteByte((host_client->h2_playerclass << 5) | ((int)host_client->qh_edict->GetLevel() & 31));

				if (dmMode->value == DM_SIEGE)
				{
					client->netchan.message.WriteByte(hwsvc_updatesiegelosses);
					client->netchan.message.WriteByte(pr_global_struct->defLosses);
					client->netchan.message.WriteByte(pr_global_struct->attLosses);
				}
			}

			host_client->qh_old_frags = host_client->qh_edict->GetFrags();
		}

		SV_WriteInventory(host_client, host_client->qh_edict, &host_client->netchan.message);

		if (CheckPIV && host_client->hw_PIV != host_client->hw_LastPIV)
		{
			host_client->netchan.message.WriteByte(hwsvc_update_piv);
			host_client->netchan.message.WriteLong(host_client->hw_PIV);
			host_client->hw_LastPIV = host_client->hw_PIV;
		}

		// maxspeed/entgravity changes
		ent = host_client->qh_edict;

		val = GetEdictFieldValue(ent, "gravity");
		if (val && host_client->qh_entgravity != val->_float)
		{
			host_client->qh_entgravity = val->_float;
			host_client->netchan.message.WriteByte(hwsvc_entgravity);
			host_client->netchan.message.WriteFloat(host_client->qh_entgravity);
		}
		val = GetEdictFieldValue(ent, "maxspeed");
		if (val && host_client->qh_maxspeed != val->_float)
		{
			host_client->qh_maxspeed = val->_float;
			host_client->netchan.message.WriteByte(hwsvc_maxspeed);
			host_client->netchan.message.WriteFloat(host_client->qh_maxspeed);
		}

	}

	if (sv.datagram.overflowed)
	{
		sv.datagram.Clear();
	}

	// append the broadcast messages to each client messages
	for (j = 0, client = svs.clients; j < HWMAX_CLIENTS; j++, client++)
	{
		if (client->state < CS_CONNECTED)
		{
			continue;	// reliables go to all connected or spawned
		}
		client->netchan.message.WriteData(
			sv.reliable_datagram._data,
			sv.reliable_datagram.cursize);

		if (client->state != CS_ACTIVE)
		{
			continue;	// datagrams only go to spawned
		}
		client->datagram.WriteData(
			sv.datagram._data,
			sv.datagram.cursize);
	}

	sv.reliable_datagram.Clear();
	sv.datagram.Clear();
}


/*
=============
SV_CleanupEnts

=============
*/
void SV_CleanupEnts(void)
{
	int e;
	qhedict_t* ent;

	ent = NEXT_EDICT(sv.edicts);
	for (e = 1; e < sv.num_edicts; e++, ent = NEXT_EDICT(ent))
	{
		ent->SetEffects((int)ent->GetEffects() & ~H2EF_MUZZLEFLASH);
		ent->SetWpnSound(0);
	}

}

/*
=======================
SV_SendClientMessages
=======================
*/
void SV_SendClientMessages(void)
{
	int i;
	client_t* c;

// update frags, names, etc
	SV_UpdateToReliableMessages();

// build individual updates
	for (i = 0, c = svs.clients; i < HWMAX_CLIENTS; i++, c++)
	{
		if (!c->state)
		{
			continue;
		}
		// if the reliable message overflowed,
		// drop the client
		if (c->netchan.message.overflowed)
		{
			c->netchan.message.Clear();
			c->datagram.Clear();
			SV_BroadcastPrintf(PRINT_HIGH, "%s overflowed\n", c->name);
			Con_Printf("WARNING: reliable overflow for %s\n",c->name);
			SV_DropClient(c);
			c->qh_send_message = true;
			c->netchan.clearTime = 0;	// don't choke this message
		}

		// only send messages if the client has sent one
		// and the bandwidth is not choked
		if (!c->qh_send_message)
		{
			continue;
		}
		c->qh_send_message = false;	// try putting this after choke?
		if (!Netchan_CanPacket(&c->netchan))
		{
			c->qh_chokecount++;
			continue;		// bandwidth choke
		}

		if (c->state == CS_ACTIVE)
		{
			SV_SendClientDatagram(c);
		}
		else
		{
			Netchan_Transmit(&c->netchan, 0, NULL);		// just update reliable

		}
	}

	// clear muzzle flashes & wpn_sound
	SV_CleanupEnts();
}


/*
=======================
SV_SendMessagesToAll

FIXME: does this sequence right?
=======================
*/
void SV_SendMessagesToAll(void)
{
	int i;
	client_t* c;

	for (i = 0, c = svs.clients; i < HWMAX_CLIENTS; i++, c++)
		if (c->state)		// FIXME: should this only send to active?
		{
			c->qh_send_message = true;
		}

	SV_SendClientMessages();
}
