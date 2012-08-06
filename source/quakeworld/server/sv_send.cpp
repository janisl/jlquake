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
common->Printf

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
===============================================================================

FRAME UPDATES

===============================================================================
*/

void SV_FindModelNumbers(void)
{
	int i;

	svqw_nailmodel = -1;
	svqw_supernailmodel = -1;
	svqw_playermodel = -1;

	for (i = 0; i < MAX_MODELS_Q1; i++)
	{
		if (!sv.qh_model_precache[i])
		{
			break;
		}
		if (!String::Cmp(sv.qh_model_precache[i],"progs/spike.mdl"))
		{
			svqw_nailmodel = i;
		}
		if (!String::Cmp(sv.qh_model_precache[i],"progs/s_spike.mdl"))
		{
			svqw_supernailmodel = i;
		}
		if (!String::Cmp(sv.qh_model_precache[i],"progs/player.mdl"))
		{
			svqw_playermodel = i;
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

	stats[Q1STAT_HEALTH] = ent->GetHealth();
	stats[Q1STAT_WEAPON] = SVQH_ModelIndex(PR_GetString(ent->GetWeaponModel()));
	stats[Q1STAT_AMMO] = ent->GetCurrentAmmo();
	stats[Q1STAT_ARMOR] = ent->GetArmorValue();
	stats[Q1STAT_SHELLS] = ent->GetAmmoShells();
	stats[Q1STAT_NAILS] = ent->GetAmmoNails();
	stats[Q1STAT_ROCKETS] = ent->GetAmmoRockets();
	stats[Q1STAT_CELLS] = ent->GetAmmoCells();
	if (!client->qh_spectator)
	{
		stats[Q1STAT_ACTIVEWEAPON] = ent->GetWeapon();
	}
	// stuff the sigil bits into the high bits of items for sbar
	stats[QWSTAT_ITEMS] = (int)ent->GetItems() | ((int)*pr_globalVars.serverflags << 28);

	for (i = 0; i < MAX_CL_STATS; i++)
		if (stats[i] != client->qh_stats[i])
		{
			client->qh_stats[i] = stats[i];
			if (stats[i] >= 0 && stats[i] <= 255)
			{
				SVQH_ClientReliableWrite_Begin(client, q1svc_updatestat, 3);
				SVQH_ClientReliableWrite_Byte(client, i);
				SVQH_ClientReliableWrite_Byte(client, stats[i]);
			}
			else
			{
				SVQH_ClientReliableWrite_Begin(client, qwsvc_updatestatlong, 6);
				SVQH_ClientReliableWrite_Byte(client, i);
				SVQH_ClientReliableWrite_Long(client, stats[i]);
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
	byte buf[MAX_DATAGRAM_QW];
	QMsg msg;

	msg.InitOOB(buf, sizeof(buf));
	msg.allowoverflow = true;

	// add the client specific data to the datagram
	SVQH_WriteClientdataToMessage(client, &msg);

	// send over all the objects that are in the PVS
	// this will include clients, a packetentities, and
	// possibly a nails update
	SVQW_WriteEntitiesToClient(client, &msg);

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
				SVQH_ClientReliableWrite_Begin(client, q1svc_updatefrags, 4);
				SVQH_ClientReliableWrite_Byte(client, i);
				SVQH_ClientReliableWrite_Short(client, host_client->qh_edict->GetFrags());
			}

			host_client->qh_old_frags = host_client->qh_edict->GetFrags();
		}

		// maxspeed/entgravity changes
		ent = host_client->qh_edict;

		val = GetEdictFieldValue(ent, "gravity");
		if (val && host_client->qh_entgravity != val->_float)
		{
			host_client->qh_entgravity = val->_float;
			SVQH_ClientReliableWrite_Begin(host_client, qwsvc_entgravity, 5);
			SVQH_ClientReliableWrite_Float(host_client, host_client->qh_entgravity);
		}
		val = GetEdictFieldValue(ent, "maxspeed");
		if (val && host_client->qh_maxspeed != val->_float)
		{
			host_client->qh_maxspeed = val->_float;
			SVQH_ClientReliableWrite_Begin(host_client, qwsvc_maxspeed, 5);
			SVQH_ClientReliableWrite_Float(host_client, host_client->qh_maxspeed);
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
		SVQH_ClientReliableCheckBlock(client, sv.qh_reliable_datagram.cursize);
		SVQH_ClientReliableWrite_SZ(client, sv.qh_reliable_datagram._data, sv.qh_reliable_datagram.cursize);

		if (client->state != CS_ACTIVE)
		{
			continue;	// datagrams only go to spawned
		}
		client->datagram.WriteData(sv.qh_datagram._data, sv.qh_datagram.cursize);
	}

	sv.qh_reliable_datagram.Clear();
	sv.qh_datagram.Clear();
}

#ifdef _WIN32
#pragma optimize( "", off )
#endif



/*
=======================
SV_SendClientMessages
=======================
*/
void SV_SendClientMessages(void)
{
	int i, j;
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

		if (c->qw_drop)
		{
			SVQHW_DropClient(c);
			c->qw_drop = false;
			continue;
		}

		// check to see if we have a backbuf to stick in the reliable
		if (c->qw_num_backbuf)
		{
			// will it fit?
			if (c->netchan.message.cursize + c->qw_backbuf_size[0] <
				c->netchan.message.maxsize)
			{

				common->DPrintf("%s: backbuf %d bytes\n",
					c->name, c->qw_backbuf_size[0]);

				// it'll fit
				c->netchan.message.WriteData(c->qw_backbuf_data[0],
					c->qw_backbuf_size[0]);

				//move along, move along
				for (j = 1; j < c->qw_num_backbuf; j++)
				{
					Com_Memcpy(c->qw_backbuf_data[j - 1], c->qw_backbuf_data[j],
						c->qw_backbuf_size[j]);
					c->qw_backbuf_size[j - 1] = c->qw_backbuf_size[j];
				}

				c->qw_num_backbuf--;
				if (c->qw_num_backbuf)
				{
					c->qw_backbuf.InitOOB(c->qw_backbuf_data[c->qw_num_backbuf - 1], sizeof(c->qw_backbuf_data[c->qw_num_backbuf - 1]));
					c->qw_backbuf.cursize = c->qw_backbuf_size[c->qw_num_backbuf - 1];
				}
			}
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
}

#ifdef _WIN32
#pragma optimize( "", on )
#endif



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
