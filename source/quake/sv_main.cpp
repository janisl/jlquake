/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// sv_main.c -- server main program

#include "quakedef.h"
#include "../common/file_formats/bsp29.h"

char localmodels[MAX_MODELS_Q1][5];				// inline model names for precache

//============================================================================

/*
===============
SV_Init
===============
*/
void SV_Init(void)
{
	int i;
	extern Cvar* sv_maxvelocity;
	extern Cvar* sv_gravity;
	extern Cvar* sv_nostep;
	extern Cvar* sv_friction;
	extern Cvar* sv_edgefriction;
	extern Cvar* sv_stopspeed;
	extern Cvar* sv_maxspeed;
	extern Cvar* sv_accelerate;
	extern Cvar* sv_idealpitchscale;
	extern Cvar* sv_aim;


	sv_maxvelocity = Cvar_Get("sv_maxvelocity", "2000", 0);
	sv_gravity = Cvar_Get("sv_gravity", "800", CVAR_SERVERINFO);
	sv_friction = Cvar_Get("sv_friction", "4", CVAR_SERVERINFO);
	sv_edgefriction = Cvar_Get("edgefriction", "2", 0);
	sv_stopspeed = Cvar_Get("sv_stopspeed","100", 0);
	sv_maxspeed = Cvar_Get("sv_maxspeed", "320", CVAR_SERVERINFO);
	sv_accelerate = Cvar_Get("sv_accelerate", "10", 0);
	sv_idealpitchscale = Cvar_Get("sv_idealpitchscale", "0.8", 0);
	sv_aim = Cvar_Get("sv_aim", "0.93", 0);
	sv_nostep = Cvar_Get("sv_nostep", "0", 0);

	for (i = 0; i < MAX_MODELS_Q1; i++)
		sprintf(localmodels[i], "*%i", i);
}

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

/*
==================
SV_StartParticle

Make sure the event gets sent to all clients
==================
*/
void SV_StartParticle(vec3_t org, vec3_t dir, int color, int count)
{
	int i, v;

	if (sv.qh_datagram.cursize > MAX_DATAGRAM_Q1 - 16)
	{
		return;
	}
	sv.qh_datagram.WriteByte(q1svc_particle);
	sv.qh_datagram.WriteCoord(org[0]);
	sv.qh_datagram.WriteCoord(org[1]);
	sv.qh_datagram.WriteCoord(org[2]);
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
		sv.qh_datagram.WriteChar(v);
	}
	sv.qh_datagram.WriteByte(count);
	sv.qh_datagram.WriteByte(color);
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

	if (volume < 0 || volume > 255)
	{
		Sys_Error("SV_StartSound: volume = %i", volume);
	}

	if (attenuation < 0 || attenuation > 4)
	{
		Sys_Error("SV_StartSound: attenuation = %f", attenuation);
	}

	if (channel < 0 || channel > 7)
	{
		Sys_Error("SV_StartSound: channel = %i", channel);
	}

	if (sv.qh_datagram.cursize > MAX_DATAGRAM_Q1 - 16)
	{
		return;
	}

// find precache number for sound
	for (sound_num = 1; sound_num < MAX_SOUNDS_Q1 &&
		 sv.qh_sound_precache[sound_num]; sound_num++)
		if (!String::Cmp(sample, sv.qh_sound_precache[sound_num]))
		{
			break;
		}

	if (sound_num == MAX_SOUNDS_Q1 || !sv.qh_sound_precache[sound_num])
	{
		Con_Printf("SV_StartSound: %s not precacheed\n", sample);
		return;
	}

	ent = QH_NUM_FOR_EDICT(entity);

	channel = (ent << 3) | channel;

	field_mask = 0;
	if (volume != DEFAULT_SOUND_PACKET_VOLUME)
	{
		field_mask |= SND_VOLUME;
	}
	if (attenuation != DEFAULT_SOUND_PACKET_ATTENUATION)
	{
		field_mask |= SND_ATTENUATION;
	}

// directed messages go only to the entity the are targeted on
	sv.qh_datagram.WriteByte(q1svc_sound);
	sv.qh_datagram.WriteByte(field_mask);
	if (field_mask & SND_VOLUME)
	{
		sv.qh_datagram.WriteByte(volume);
	}
	if (field_mask & SND_ATTENUATION)
	{
		sv.qh_datagram.WriteByte(attenuation * 64);
	}
	sv.qh_datagram.WriteShort(channel);
	sv.qh_datagram.WriteByte(sound_num);
	for (i = 0; i < 3; i++)
		sv.qh_datagram.WriteCoord(entity->GetOrigin()[i] + 0.5 * (entity->GetMins()[i] + entity->GetMaxs()[i]));
}

/*
==============================================================================

CLIENT SPAWNING

==============================================================================
*/

/*
================
SV_SendServerinfo

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each server load.
================
*/
void SV_SendServerinfo(client_t* client)
{
	const char** s;
	char message[2048];

	client->qh_message.WriteByte(q1svc_print);
	sprintf(message, "%c\nVERSION %4.2f SERVER (%i CRC)", 2, VERSION, pr_crc);
	client->qh_message.WriteString2(message);

	client->qh_message.WriteByte(q1svc_serverinfo);
	client->qh_message.WriteLong(PROTOCOL_VERSION);
	client->qh_message.WriteByte(svs.qh_maxclients);

	if (!coop->value && deathmatch->value)
	{
		client->qh_message.WriteByte(GAME_DEATHMATCH);
	}
	else
	{
		client->qh_message.WriteByte(GAME_COOP);
	}

	String::Cpy(message, PR_GetString(sv.qh_edicts->GetMessage()));

	client->qh_message.WriteString2(message);

	for (s = sv.qh_model_precache + 1; *s; s++)
		client->qh_message.WriteString2(*s);
	client->qh_message.WriteByte(0);

	for (s = sv.qh_sound_precache + 1; *s; s++)
		client->qh_message.WriteString2(*s);
	client->qh_message.WriteByte(0);

// send music
	client->qh_message.WriteByte(q1svc_cdtrack);
	client->qh_message.WriteByte(sv.qh_edicts->GetSounds());
	client->qh_message.WriteByte(sv.qh_edicts->GetSounds());

// set view
	client->qh_message.WriteByte(q1svc_setview);
	client->qh_message.WriteShort(QH_NUM_FOR_EDICT(client->qh_edict));

	client->qh_message.WriteByte(q1svc_signonnum);
	client->qh_message.WriteByte(1);

	client->qh_sendsignon = true;
	client->state = CS_CONNECTED;		// need prespawn, spawn, etc
}

/*
================
SV_ConnectClient

Initializes a client_t for a new net connection.  This will only be called
once for a player each game, not once for each level change.
================
*/
void SV_ConnectClient(int clientnum)
{
	qhedict_t* ent;
	client_t* client;
	int edictnum;
	qsocket_t* netconnection;
	int i;
	float spawn_parms[NUM_SPAWN_PARMS];

	client = svs.clients + clientnum;

	Con_DPrintf("Client %s connected\n", client->qh_netconnection->address);

	edictnum = clientnum + 1;

	ent = QH_EDICT_NUM(edictnum);

// set up the client_t
	netconnection = client->qh_netconnection;
	netchan_t netchan = client->netchan;

	if (sv.loadgame)
	{
		Com_Memcpy(spawn_parms, client->qh_spawn_parms, sizeof(spawn_parms));
	}
	Com_Memset(client, 0, sizeof(*client));
	client->qh_netconnection = netconnection;
	client->netchan = netchan;

	String::Cpy(client->name, "unconnected");
	client->state = CS_CONNECTED;
	client->qh_edict = ent;
	client->qh_message.InitOOB(client->qh_messageBuffer, MAX_MSGLEN_Q1);
	client->qh_message.allowoverflow = true;		// we can catch it

#ifdef IDGODS
	client->qh_privileged = IsID(&client->netconnection->addr);
#else
	client->qh_privileged = false;
#endif

	if (sv.loadgame)
	{
		Com_Memcpy(client->qh_spawn_parms, spawn_parms, sizeof(spawn_parms));
	}
	else
	{
		// call the progs to get default spawn parms for the new client
		PR_ExecuteProgram(*pr_globalVars.SetNewParms);
		for (i = 0; i < NUM_SPAWN_PARMS; i++)
			client->qh_spawn_parms[i] = pr_globalVars.parm1[i];
	}

	SV_SendServerinfo(client);
}


/*
===================
SV_CheckForNewClients

===================
*/
void SV_CheckForNewClients(void)
{
	qsocket_t* ret;
	int i;

//
// check for new connections
//
	while (1)
	{
		netadr_t addr;
		ret = NET_CheckNewConnections(&addr);
		if (!ret)
		{
			break;
		}

		//
		// init a new client structure
		//
		for (i = 0; i < svs.qh_maxclients; i++)
			if (svs.clients[i].state == CS_FREE)
			{
				break;
			}
		if (i == svs.qh_maxclients)
		{
			Sys_Error("Host_CheckForNewClients: no free clients");
		}

		Com_Memset(&svs.clients[i].netchan, 0, sizeof(svs.clients[i].netchan));
		svs.clients[i].netchan.sock = NS_SERVER;
		svs.clients[i].netchan.remoteAddress = addr;
		svs.clients[i].netchan.lastReceived = net_time * 1000;
		svs.clients[i].qh_netconnection = ret;
		SV_ConnectClient(i);

		net_activeconnections++;
	}
}



/*
===============================================================================

FRAME UPDATES

===============================================================================
*/

/*
==================
SV_ClearDatagram

==================
*/
void SV_ClearDatagram(void)
{
	sv.qh_datagram.Clear();
}

/*
=============================================================================

The PVS must include a small area around the client to allow head bobbing
or other small motion on the client side.  Otherwise, a bob might cause an
entity that should be visible to not show up, especially when the bob
crosses a waterline.

=============================================================================
*/

static byte fatpvs[BSP29_MAX_MAP_LEAFS / 8];

/*
=============
SV_FatPVS

Calculates a PVS that is the inclusive or of all leafs within 8 pixels of the
given point.
=============
*/
byte* SV_FatPVS(vec3_t org)
{
	vec3_t mins, maxs;
	for (int i = 0; i < 3; i++)
	{
		mins[i] = org[i] - 8;
		maxs[i] = org[i] + 8;
	}

	int leafs[64];
	int count = CM_BoxLeafnums(mins, maxs, leafs, 64);
	if (count < 1)
	{
		throw Exception("SV_FatPVS: count < 1");
	}

	// convert leafs to clusters
	for (int i = 0; i < count; i++)
	{
		leafs[i] = CM_LeafCluster(leafs[i]);
	}

	int fatbytes = (CM_NumClusters() + 31) >> 3;
	Com_Memcpy(fatpvs, CM_ClusterPVS(leafs[0]), fatbytes);
	// or in all the other leaf bits
	for (int i = 1; i < count; i++)
	{
		byte* pvs = CM_ClusterPVS(leafs[i]);
		for (int j = 0; j < fatbytes; j++)
		{
			fatpvs[j] |= pvs[j];
		}
	}
	return fatpvs;
}

//=============================================================================


/*
=============
SV_WriteEntitiesToClient

=============
*/
void SV_WriteEntitiesToClient(qhedict_t* clent, QMsg* msg)
{
	int e, i;
	int bits;
	byte* pvs;
	vec3_t org;
	float miss;
	qhedict_t* ent;

// find the client's PVS
	VectorAdd(clent->GetOrigin(), clent->GetViewOfs(), org);
	pvs = SV_FatPVS(org);

// send over all entities (excpet the client) that touch the pvs
	ent = NEXT_EDICT(sv.qh_edicts);
	for (e = 1; e < sv.qh_num_edicts; e++, ent = NEXT_EDICT(ent))
	{
// ignore if not touching a PV leaf
		if (ent != clent)	// clent is ALLWAYS sent
		{
// ignore ents without visible models
			if (!ent->v.modelindex || !*PR_GetString(ent->GetModel()))
			{
				continue;
			}

			for (i = 0; i < ent->num_leafs; i++)
			{
				int l = CM_LeafCluster(ent->LeafNums[i]);
				if (pvs[l >> 3] & (1 << (l & 7)))
				{
					break;
				}
			}

			if (i == ent->num_leafs)
			{
				continue;		// not visible
			}
		}

		if (msg->maxsize - msg->cursize < 16)
		{
			Con_Printf("packet overflow\n");
			return;
		}

// send an update
		bits = 0;

		for (i = 0; i < 3; i++)
		{
			miss = ent->GetOrigin()[i] - ent->q1_baseline.origin[i];
			if (miss < -0.1 || miss > 0.1)
			{
				bits |= Q1U_ORIGIN1 << i;
			}
		}

		if (ent->GetAngles()[0] != ent->q1_baseline.angles[0])
		{
			bits |= Q1U_ANGLE1;
		}

		if (ent->GetAngles()[1] != ent->q1_baseline.angles[1])
		{
			bits |= Q1U_ANGLE2;
		}

		if (ent->GetAngles()[2] != ent->q1_baseline.angles[2])
		{
			bits |= Q1U_ANGLE3;
		}

		if (ent->GetMoveType() == QHMOVETYPE_STEP)
		{
			bits |= Q1U_NOLERP;	// don't mess up the step animation

		}
		if (ent->q1_baseline.colormap != ent->GetColorMap())
		{
			bits |= Q1U_COLORMAP;
		}

		if (ent->q1_baseline.skinnum != ent->GetSkin())
		{
			bits |= Q1U_SKIN;
		}

		if (ent->q1_baseline.frame != ent->GetFrame())
		{
			bits |= Q1U_FRAME;
		}

		if (ent->q1_baseline.effects != ent->GetEffects())
		{
			bits |= Q1U_EFFECTS;
		}

		if (ent->q1_baseline.modelindex != ent->v.modelindex)
		{
			bits |= Q1U_MODEL;
		}

		if (e >= 256)
		{
			bits |= Q1U_LONGENTITY;
		}

		if (bits >= 256)
		{
			bits |= Q1U_MOREBITS;
		}

		//
		// write the message
		//
		msg->WriteByte(bits | Q1U_SIGNAL);

		if (bits & Q1U_MOREBITS)
		{
			msg->WriteByte(bits >> 8);
		}
		if (bits & Q1U_LONGENTITY)
		{
			msg->WriteShort(e);
		}
		else
		{
			msg->WriteByte(e);
		}

		if (bits & Q1U_MODEL)
		{
			msg->WriteByte(ent->v.modelindex);
		}
		if (bits & Q1U_FRAME)
		{
			msg->WriteByte(ent->GetFrame());
		}
		if (bits & Q1U_COLORMAP)
		{
			msg->WriteByte(ent->GetColorMap());
		}
		if (bits & Q1U_SKIN)
		{
			msg->WriteByte(ent->GetSkin());
		}
		if (bits & Q1U_EFFECTS)
		{
			msg->WriteByte(ent->GetEffects());
		}
		if (bits & Q1U_ORIGIN1)
		{
			msg->WriteCoord(ent->GetOrigin()[0]);
		}
		if (bits & Q1U_ANGLE1)
		{
			msg->WriteAngle(ent->GetAngles()[0]);
		}
		if (bits & Q1U_ORIGIN2)
		{
			msg->WriteCoord(ent->GetOrigin()[1]);
		}
		if (bits & Q1U_ANGLE2)
		{
			msg->WriteAngle(ent->GetAngles()[1]);
		}
		if (bits & Q1U_ORIGIN3)
		{
			msg->WriteCoord(ent->GetOrigin()[2]);
		}
		if (bits & Q1U_ANGLE3)
		{
			msg->WriteAngle(ent->GetAngles()[2]);
		}
	}
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

	ent = NEXT_EDICT(sv.qh_edicts);
	for (e = 1; e < sv.qh_num_edicts; e++, ent = NEXT_EDICT(ent))
	{
		ent->SetEffects((int)ent->GetEffects() & ~Q1EF_MUZZLEFLASH);
	}

}

/*
==================
SV_WriteClientdataToMessage

==================
*/
void SV_WriteClientdataToMessage(qhedict_t* ent, QMsg* msg)
{
	int bits;
	int i;
	qhedict_t* other;
	int items;
	eval_t* val;

//
// send a damage message
//
	if (ent->GetDmgTake() || ent->GetDmgSave())
	{
		other = PROG_TO_EDICT(ent->GetDmgInflictor());
		msg->WriteByte(q1svc_damage);
		msg->WriteByte(ent->GetDmgSave());
		msg->WriteByte(ent->GetDmgTake());
		for (i = 0; i < 3; i++)
			msg->WriteCoord(other->GetOrigin()[i] + 0.5 * (other->GetMins()[i] + other->GetMaxs()[i]));

		ent->SetDmgTake(0);
		ent->SetDmgSave(0);
	}

//
// send the current viewpos offset from the view entity
//
	SV_SetIdealPitch();			// how much to look up / down ideally

// a fixangle might get lost in a dropped packet.  Oh well.
	if (ent->GetFixAngle())
	{
		msg->WriteByte(q1svc_setangle);
		for (i = 0; i < 3; i++)
			msg->WriteAngle(ent->GetAngles()[i]);
		ent->SetFixAngle(0);
	}

	bits = 0;

	if (ent->GetViewOfs()[2] != DEFAULT_VIEWHEIGHT)
	{
		bits |= SU_VIEWHEIGHT;
	}

	if (ent->GetIdealPitch())
	{
		bits |= SU_IDEALPITCH;
	}

// stuff the sigil bits into the high bits of items for sbar, or else
// mix in items2
	val = GetEdictFieldValue(ent, "items2");

	if (val)
	{
		items = (int)ent->GetItems() | ((int)val->_float << 23);
	}
	else
	{
		items = (int)ent->GetItems() | ((int)*pr_globalVars.serverflags << 28);
	}

	bits |= SU_ITEMS;

	if ((int)ent->GetFlags() & QHFL_ONGROUND)
	{
		bits |= SU_ONGROUND;
	}

	if (ent->GetWaterLevel() >= 2)
	{
		bits |= SU_INWATER;
	}

	for (i = 0; i < 3; i++)
	{
		if (ent->GetPunchAngle()[i])
		{
			bits |= (SU_PUNCH1 << i);
		}
		if (ent->GetVelocity()[i])
		{
			bits |= (SU_VELOCITY1 << i);
		}
	}

	if (ent->GetWeaponFrame())
	{
		bits |= SU_WEAPONFRAME;
	}

	if (ent->GetArmorValue())
	{
		bits |= SU_ARMOR;
	}

//	if (ent->v.weapon)
	bits |= SU_WEAPON;

// send the data

	msg->WriteByte(q1svc_clientdata);
	msg->WriteShort(bits);

	if (bits & SU_VIEWHEIGHT)
	{
		msg->WriteChar(ent->GetViewOfs()[2]);
	}

	if (bits & SU_IDEALPITCH)
	{
		msg->WriteChar(ent->GetIdealPitch());
	}

	for (i = 0; i < 3; i++)
	{
		if (bits & (SU_PUNCH1 << i))
		{
			msg->WriteChar(ent->GetPunchAngle()[i]);
		}
		if (bits & (SU_VELOCITY1 << i))
		{
			msg->WriteChar(ent->GetVelocity()[i] / 16);
		}
	}

// [always sent]	if (bits & SU_ITEMS)
	msg->WriteLong(items);

	if (bits & SU_WEAPONFRAME)
	{
		msg->WriteByte(ent->GetWeaponFrame());
	}
	if (bits & SU_ARMOR)
	{
		msg->WriteByte(ent->GetArmorValue());
	}
	if (bits & SU_WEAPON)
	{
		msg->WriteByte(SV_ModelIndex(PR_GetString(ent->GetWeaponModel())));
	}

	msg->WriteShort(ent->GetHealth());
	msg->WriteByte(ent->GetCurrentAmmo());
	msg->WriteByte(ent->GetAmmoShells());
	msg->WriteByte(ent->GetAmmoNails());
	msg->WriteByte(ent->GetAmmoRockets());
	msg->WriteByte(ent->GetAmmoCells());

	if (standard_quake)
	{
		msg->WriteByte(ent->GetWeapon());
	}
	else
	{
		for (i = 0; i < 32; i++)
		{
			if (((int)ent->GetWeapon()) & (1 << i))
			{
				msg->WriteByte(i);
				break;
			}
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
	byte buf[MAX_DATAGRAM_Q1];
	QMsg msg;

	msg.InitOOB(buf, sizeof(buf));

	msg.WriteByte(q1svc_time);
	msg.WriteFloat(sv.qh_time);

// add the client specific data to the datagram
	SV_WriteClientdataToMessage(client->qh_edict, &msg);

	SV_WriteEntitiesToClient(client->qh_edict, &msg);

// copy the server datagram if there is space
	if (msg.cursize + sv.qh_datagram.cursize < msg.maxsize)
	{
		msg.WriteData(sv.qh_datagram._data, sv.qh_datagram.cursize);
	}

// send the datagram
	if (NET_SendUnreliableMessage(client->qh_netconnection, &client->netchan, &msg) == -1)
	{
		SV_DropClient(true);// if the message couldn't send, kick off
		return false;
	}

	return true;
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

// check for changes to be sent over the reliable streams
	for (i = 0, host_client = svs.clients; i < svs.qh_maxclients; i++, host_client++)
	{
		if (host_client->qh_old_frags != host_client->qh_edict->GetFrags())
		{
			for (j = 0, client = svs.clients; j < svs.qh_maxclients; j++, client++)
			{
				if (client->state < CS_CONNECTED)
				{
					continue;
				}
				client->qh_message.WriteByte(q1svc_updatefrags);
				client->qh_message.WriteByte(i);
				client->qh_message.WriteShort(host_client->qh_edict->GetFrags());
			}

			host_client->qh_old_frags = host_client->qh_edict->GetFrags();
		}
	}

	for (j = 0, client = svs.clients; j < svs.qh_maxclients; j++, client++)
	{
		if (client->state < CS_CONNECTED)
		{
			continue;
		}
		client->qh_message.WriteData(sv.qh_reliable_datagram._data, sv.qh_reliable_datagram.cursize);
	}

	sv.qh_reliable_datagram.Clear();
}


/*
=======================
SV_SendNop

Send a nop message without trashing or sending the accumulated client
message buffer
=======================
*/
void SV_SendNop(client_t* client)
{
	QMsg msg;
	byte buf[4];

	msg.InitOOB(buf, sizeof(buf));

	msg.WriteChar(q1svc_nop);

	if (NET_SendUnreliableMessage(client->qh_netconnection, &client->netchan, &msg) == -1)
	{
		SV_DropClient(true);	// if the message couldn't send, kick off
	}
	client->qh_last_message = realtime;
}

/*
=======================
SV_SendClientMessages
=======================
*/
void SV_SendClientMessages(void)
{
	int i;

// update frags, names, etc
	SV_UpdateToReliableMessages();

// build individual updates
	for (i = 0, host_client = svs.clients; i < svs.qh_maxclients; i++, host_client++)
	{
		if (host_client->state < CS_CONNECTED)
		{
			continue;
		}

		if (host_client->state == CS_ACTIVE)
		{
			if (!SV_SendClientDatagram(host_client))
			{
				continue;
			}
		}
		else
		{
			// the player isn't totally in the game yet
			// send small keepalive messages if too much time has passed
			// send a full message when the next signon stage has been requested
			// some other message data (name changes, etc) may accumulate
			// between signon stages
			if (!host_client->qh_sendsignon)
			{
				if (realtime - host_client->qh_last_message > 5)
				{
					SV_SendNop(host_client);
				}
				continue;	// don't send out non-signon messages
			}
		}

		// check for an overflowed message.  Should only happen
		// on a very fucked up connection that backs up a lot, then
		// changes level
		if (host_client->qh_message.overflowed)
		{
			SV_DropClient(true);
			host_client->qh_message.overflowed = false;
			continue;
		}

		if (host_client->qh_message.cursize || host_client->qh_dropasap)
		{
			if (!NET_CanSendMessage(host_client->qh_netconnection, &host_client->netchan))
			{
//				I_Printf ("can't write\n");
				continue;
			}

			if (host_client->qh_dropasap)
			{
				SV_DropClient(false);	// went to another level
			}
			else
			{
				if (NET_SendMessage(host_client->qh_netconnection,
						&host_client->netchan, &host_client->qh_message) == -1)
				{
					SV_DropClient(true);	// if the message couldn't send, kick off
				}
				host_client->qh_message.Clear();
				host_client->qh_last_message = realtime;
				host_client->qh_sendsignon = false;
			}
		}
	}


// clear muzzle flashes
	SV_CleanupEnts();
}


/*
==============================================================================

SERVER SPAWNING

==============================================================================
*/

/*
================
SV_ModelIndex

================
*/
int SV_ModelIndex(const char* name)
{
	int i;

	if (!name || !name[0])
	{
		return 0;
	}

	for (i = 0; i < MAX_MODELS_Q1 && sv.qh_model_precache[i]; i++)
		if (!String::Cmp(sv.qh_model_precache[i], name))
		{
			return i;
		}
	if (i == MAX_MODELS_Q1 || !sv.qh_model_precache[i])
	{
		Sys_Error("SV_ModelIndex: model %s not precached", name);
	}
	return i;
}

/*
================
SV_CreateBaseline

================
*/
void SV_CreateBaseline(void)
{
	int i;
	qhedict_t* svent;
	int entnum;

	for (entnum = 0; entnum < sv.qh_num_edicts; entnum++)
	{
		// get the current server version
		svent = QH_EDICT_NUM(entnum);
		if (svent->free)
		{
			continue;
		}
		if (entnum > svs.qh_maxclients && !svent->v.modelindex)
		{
			continue;
		}

		//
		// create entity baseline
		//
		VectorCopy(svent->GetOrigin(), svent->q1_baseline.origin);
		VectorCopy(svent->GetAngles(), svent->q1_baseline.angles);
		svent->q1_baseline.frame = svent->GetFrame();
		svent->q1_baseline.skinnum = svent->GetSkin();
		if (entnum > 0 && entnum <= svs.qh_maxclients)
		{
			svent->q1_baseline.colormap = entnum;
			svent->q1_baseline.modelindex = SV_ModelIndex("progs/player.mdl");
		}
		else
		{
			svent->q1_baseline.colormap = 0;
			svent->q1_baseline.modelindex =
				SV_ModelIndex(PR_GetString(svent->GetModel()));
		}

		//
		// add to the message
		//
		sv.qh_signon.WriteByte(q1svc_spawnbaseline);
		sv.qh_signon.WriteShort(entnum);

		sv.qh_signon.WriteByte(svent->q1_baseline.modelindex);
		sv.qh_signon.WriteByte(svent->q1_baseline.frame);
		sv.qh_signon.WriteByte(svent->q1_baseline.colormap);
		sv.qh_signon.WriteByte(svent->q1_baseline.skinnum);
		for (i = 0; i < 3; i++)
		{
			sv.qh_signon.WriteCoord(svent->q1_baseline.origin[i]);
			sv.qh_signon.WriteAngle(svent->q1_baseline.angles[i]);
		}
	}
}


/*
================
SV_SendReconnect

Tell all the clients that the server is changing levels
================
*/
void SV_SendReconnect(void)
{
	byte data[128];
	QMsg msg;

	msg.InitOOB(data, sizeof(data));

	msg.WriteChar(q1svc_stufftext);
	msg.WriteString2("reconnect\n");
	NET_SendToAll(&msg, 5);

	if (cls.state != CA_DEDICATED)
	{
		Cmd_ExecuteString("reconnect\n", src_command);
	}
}


/*
================
SV_SaveSpawnparms

Grabs the current state of each client for saving across the
transition to another level
================
*/
void SV_SaveSpawnparms(void)
{
	int i, j;

	svs.qh_serverflags = *pr_globalVars.serverflags;

	for (i = 0, host_client = svs.clients; i < svs.qh_maxclients; i++, host_client++)
	{
		if (host_client->state < CS_CONNECTED)
		{
			continue;
		}

		// call the progs to get default spawn parms for the new client
		*pr_globalVars.self = EDICT_TO_PROG(host_client->qh_edict);
		PR_ExecuteProgram(*pr_globalVars.SetChangeParms);
		for (j = 0; j < NUM_SPAWN_PARMS; j++)
			host_client->qh_spawn_parms[j] = pr_globalVars.parm1[j];
	}
}


/*
================
SV_SpawnServer

This is called at the start of each level
================
*/
extern float scr_centertime_off;

void SV_SpawnServer(char* server)
{
	qhedict_t* ent;
	int i;

	// let's not have any servers with no name
	if (!hostname || hostname->string[0] == 0)
	{
		Cvar_Set("hostname", "UNNAMED");
	}
	scr_centertime_off = 0;

	Con_DPrintf("SpawnServer: %s\n",server);
	svs.qh_changelevel_issued = false;		// now safe to issue another

//
// tell all connected clients that we are going to a new level
//
	if (sv.state != SS_DEAD)
	{
		SV_SendReconnect();
	}

//
// make cvars consistant
//
	if (coop->value)
	{
		Cvar_SetValue("deathmatch", 0);
	}
	current_skill = (int)(skill->value + 0.5);
	if (current_skill < 0)
	{
		current_skill = 0;
	}
	if (current_skill > 3)
	{
		current_skill = 3;
	}

	Cvar_SetValue("skill", (float)current_skill);

//
// set up the new server
//
	Host_ClearMemory();

	Com_Memset(&sv, 0, sizeof(sv));

	String::Cpy(sv.name, server);

// load progs to get entity field count
	PR_LoadProgs();

// allocate server memory
	sv.qh_edicts = (qhedict_t*)Hunk_AllocName(MAX_EDICTS_QH * pr_edict_size, "edicts");

	sv.qh_datagram.InitOOB(sv.qh_datagramBuffer, MAX_DATAGRAM_Q1);

	sv.qh_reliable_datagram.InitOOB(sv.qh_reliable_datagramBuffer, MAX_DATAGRAM_Q1);

	sv.qh_signon.InitOOB(sv.qh_signonBuffer, MAX_MSGLEN_Q1);

// leave slots at start for clients only
	sv.qh_num_edicts = svs.qh_maxclients + 1;
	for (i = 0; i < svs.qh_maxclients; i++)
	{
		ent = QH_EDICT_NUM(i + 1);
		svs.clients[i].qh_edict = ent;
	}

	sv.state = SS_LOADING;
	sv.qh_paused = false;

	sv.qh_time = 1.0;

	String::Cpy(sv.name, server);
	sprintf(sv.qh_modelname,"maps/%s.bsp", server);
	CM_LoadMap(sv.qh_modelname, false, NULL);
	sv.models[1] = 0;

//
// clear world interaction links
//
	SV_ClearWorld();

	sv.qh_sound_precache[0] = PR_GetString(0);

	sv.qh_model_precache[0] = PR_GetString(0);
	sv.qh_model_precache[1] = sv.qh_modelname;
	for (i = 1; i < CM_NumInlineModels(); i++)
	{
		sv.qh_model_precache[1 + i] = localmodels[i];
		sv.models[i + 1] = CM_InlineModel(i);
	}

//
// load the rest of the entities
//
	ent = QH_EDICT_NUM(0);
	Com_Memset(&ent->v, 0, progs->entityfields * 4);
	ent->free = false;
	ent->SetModel(PR_SetString(sv.qh_modelname));
	ent->v.modelindex = 1;		// world model
	ent->SetSolid(QHSOLID_BSP);
	ent->SetMoveType(QHMOVETYPE_PUSH);

	if (coop->value)
	{
		*pr_globalVars.coop = coop->value;
	}
	else
	{
		*pr_globalVars.deathmatch = deathmatch->value;
	}

	*pr_globalVars.mapname = PR_SetString(sv.name);

// serverflags are for cross level information (sigils)
	*pr_globalVars.serverflags = svs.qh_serverflags;

	ED_LoadFromFile(CM_EntityString());

// all setup is completed, any further precache statements are errors
	sv.state = SS_GAME;

// run two frames to allow everything to settle
	host_frametime = 0.1;
	SV_Physics();
	SV_Physics();

// create a baseline for more efficient communications
	SV_CreateBaseline();

// send serverinfo to all connected clients
	for (i = 0,host_client = svs.clients; i < svs.qh_maxclients; i++, host_client++)
		if (host_client->state >= CS_CONNECTED)
		{
			SV_SendServerinfo(host_client);
		}

	Con_DPrintf("Server spawned.\n");
}
