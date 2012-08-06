// sv_main.c -- server main program

#include "qwsvdef.h"

#define CHAN_AUTO   0
#define CHAN_WEAPON 1
#define CHAN_VOICE  2
#define CHAN_ITEM   3
#define CHAN_BODY   4

/*
=============================================================================

common->Printf redirection

=============================================================================
*/

char outputbuf[8000];

redirect_t sv_redirected;

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

		NET_SendPacket(NS_SERVER, String::Length(send) + 1, send, net_from);
	}
	else if (sv_redirected == RD_CLIENT)
	{
		SVQH_PrintToClient(host_client, PRINT_HIGH, outputbuf);
	}

	// clear it
	outputbuf[0] = 0;
}


/*
==================
SV_BeginRedirect

  Send common->Printf data to the remote client
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

A common->Printf that only shows up if the "developer" cvar is set
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

	common->Printf("%s", msg);
}

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

/*
===============================================================================

FRAME UPDATES

===============================================================================
*/

void SV_FindModelNumbers(void)
{
	int i;

	svhw_playermodel[0] = -1;
	svhw_playermodel[1] = -1;
	svhw_playermodel[2] = -1;
	svhw_playermodel[3] = -1;
	svhw_playermodel[4] = -1;
	svhw_magicmissmodel = -1;
	svhw_ravenmodel = -1;
	svhw_raven2model = -1;

	for (i = 0; i < MAX_MODELS_H2; i++)
	{
		if (!sv.qh_model_precache[i])
		{
			break;
		}
		if (!String::Cmp(sv.qh_model_precache[i],"models/paladin.mdl"))
		{
			svhw_playermodel[0] = i;
		}
		if (!String::Cmp(sv.qh_model_precache[i],"models/crusader.mdl"))
		{
			svhw_playermodel[1] = i;
		}
		if (!String::Cmp(sv.qh_model_precache[i],"models/necro.mdl"))
		{
			svhw_playermodel[2] = i;
		}
		if (!String::Cmp(sv.qh_model_precache[i],"models/assassin.mdl"))
		{
			svhw_playermodel[3] = i;
		}
		if (!String::Cmp(sv.qh_model_precache[i],"models/succubus.mdl"))
		{
			svhw_playermodel[4] = i;
		}
		if (!String::Cmp(sv.qh_model_precache[i],"models/ball.mdl"))
		{
			svhw_magicmissmodel = i;
		}
		if (!String::Cmp(sv.qh_model_precache[i],"models/ravproj.mdl"))
		{
			svhw_ravenmodel = i;
		}
		if (!String::Cmp(sv.qh_model_precache[i],"models/vindsht1.mdl"))
		{
			svhw_raven2model = i;
		}
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

	stats[Q1STAT_HEALTH] = 0;	//ent->v.health;
	stats[Q1STAT_WEAPON] = SVQH_ModelIndex(PR_GetString(ent->GetWeaponModel()));
	stats[Q1STAT_AMMO] = 0;	//ent->v.currentammo;
	stats[Q1STAT_ARMOR] = 0;	//ent->v.armorvalue;
	stats[Q1STAT_SHELLS] = 0;	//ent->v.ammo_shells;
	stats[Q1STAT_NAILS] = 0;	//ent->v.ammo_nails;
	stats[Q1STAT_ROCKETS] = 0;//ent->v.ammo_rockets;
	stats[Q1STAT_CELLS] = 0;	//ent->v.ammo_cells;
	if (!client->qh_spectator)
	{
		stats[Q1STAT_ACTIVEWEAPON] = 0;	//ent->v.weapon;
	}
	// stuff the sigil bits into the high bits of items for sbar
	stats[QWSTAT_ITEMS] = 0;	//(int)ent->v.items | ((int)pr_global_struct->serverflags << 28);

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
	SVQH_WriteClientdataToMessage(client, &msg);

	// send over all the objects that are in the PVS
	// this will include clients, a packetentities, and
	// possibly a nails update
	SVHW_WriteEntitiesToClient(client, &msg);

	// copy the accumulated multicast datagram
	// for this client out to the message
	if (client->datagram.overflowed)
	{
		common->Printf("WARNING: datagram overflowed for %s\n", client->name);
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
		common->Printf("WARNING: msg overflowed for %s\n", client->name);
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

	for (i = 0, host_client = svs.clients; i < MAX_CLIENTS_QHW; i++, host_client++)
	{
		host_client->hw_PIV = 0;
	}

	for (i = 0, host_client = svs.clients; i < MAX_CLIENTS_QHW; i++, host_client++)
	{
		if (host_client->state != CS_ACTIVE || host_client->qh_spectator)
		{
			continue;
		}

		VectorCopy(host_client->qh_edict->GetOrigin(), adjust_org1);
		adjust_org1[2] += 24;

		save_hull = host_client->qh_edict->GetHull();
		host_client->qh_edict->SetHull(0);

		for (j = i + 1, client = host_client + 1; j < MAX_CLIENTS_QHW; j++, client++)
		{
			if (client->state != CS_ACTIVE || client->qh_spectator)
			{
				continue;
			}

			VectorSubtract(client->qh_edict->GetOrigin(), host_client->qh_edict->GetOrigin(), distvec);
			dist = VectorNormalize(distvec);
			if (dist > sv_namedistance->value)
			{
//				common->Printf("dist %f\n", dist);
				continue;
			}

			VectorCopy(client->qh_edict->GetOrigin(), adjust_org2);
			adjust_org2[2] += 24;

			trace = SVQH_Move(adjust_org1, vec3_origin, vec3_origin, adjust_org2, false, host_client->qh_edict);
			if (QH_EDICT_NUM(trace.entityNum) == client->qh_edict)
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

//	common->Printf("SV_UpdateToReliableMessages\n");
	if (sv.qh_time - sv.hw_next_PIV_time >= 1)
	{
		sv.hw_next_PIV_time = sv.qh_time + 1;
		CheckPIV = true;
		UpdatePIV();
	}

// check for changes to be sent over the reliable streams to all clients
	for (i = 0, host_client = svs.clients; i < MAX_CLIENTS_QHW; i++, host_client++)
	{
		if (host_client->state != CS_ACTIVE)
		{
			continue;
		}
		if (host_client->qh_sendinfo)
		{
			host_client->qh_sendinfo = false;
			SVQHW_FullClientUpdate(host_client, &sv.qh_reliable_datagram);
		}
		if (host_client->qh_old_frags != host_client->qh_edict->GetFrags())
		{
			for (j = 0, client = svs.clients; j < MAX_CLIENTS_QHW; j++, client++)
			{
				if (client->state < CS_CONNECTED)
				{
					continue;
				}
//common->Printf("SV_UpdateToReliableMessages:  Updated frags for client %d to %d\n", i, j);
				client->netchan.message.WriteByte(hwsvc_updatedminfo);
				client->netchan.message.WriteByte(i);
				client->netchan.message.WriteShort(host_client->qh_edict->GetFrags());
				client->netchan.message.WriteByte((host_client->h2_playerclass << 5) | ((int)host_client->qh_edict->GetLevel() & 31));

				if (hw_dmMode->value == HWDM_SIEGE)
				{
					client->netchan.message.WriteByte(hwsvc_updatesiegelosses);
					client->netchan.message.WriteByte(*pr_globalVars.defLosses);
					client->netchan.message.WriteByte(*pr_globalVars.attLosses);
				}
			}

			host_client->qh_old_frags = host_client->qh_edict->GetFrags();
		}

		SVHW_WriteInventory(host_client, host_client->qh_edict, &host_client->netchan.message);

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

	if (sv.qh_datagram.overflowed)
	{
		sv.qh_datagram.Clear();
	}

	// append the broadcast messages to each client messages
	for (j = 0, client = svs.clients; j < MAX_CLIENTS_QHW; j++, client++)
	{
		if (client->state < CS_CONNECTED)
		{
			continue;	// reliables go to all connected or spawned
		}
		client->netchan.message.WriteData(
			sv.qh_reliable_datagram._data,
			sv.qh_reliable_datagram.cursize);

		if (client->state != CS_ACTIVE)
		{
			continue;	// datagrams only go to spawned
		}
		client->datagram.WriteData(
			sv.qh_datagram._data,
			sv.qh_datagram.cursize);
	}

	sv.qh_reliable_datagram.Clear();
	sv.qh_datagram.Clear();
}


/*
=============
SVQH_CleanupEnts

=============
*/
void SVQH_CleanupEnts(void)
{
	int e;
	qhedict_t* ent;

	ent = NEXT_EDICT(sv.qh_edicts);
	for (e = 1; e < sv.qh_num_edicts; e++, ent = NEXT_EDICT(ent))
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
	for (i = 0, c = svs.clients; i < MAX_CLIENTS_QHW; i++, c++)
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
			SVQH_BroadcastPrintf(PRINT_HIGH, "%s overflowed\n", c->name);
			common->Printf("WARNING: reliable overflow for %s\n",c->name);
			SVQHW_DropClient(c);
			c->qh_send_message = true;
		}

		// only send messages if the client has sent one
		// and the bandwidth is not choked
		if (!c->qh_send_message)
		{
			continue;
		}
		c->qh_send_message = false;	// try putting this after choke?

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
	SVQH_CleanupEnts();
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

	for (i = 0, c = svs.clients; i < MAX_CLIENTS_QHW; i++, c++)
		if (c->state)		// FIXME: should this only send to active?
		{
			c->qh_send_message = true;
		}

	SV_SendClientMessages();
}
