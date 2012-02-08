/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
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

Con_Printf redirection

=============================================================================
*/

char	outputbuf[8000];

redirect_t	sv_redirected;

extern Cvar* sv_phs;

/*
==================
SV_FlushRedirect
==================
*/
void SV_FlushRedirect (void)
{
	char	send[8000+6];

	if (sv_redirected == RD_PACKET)
	{
		send[0] = 0xff;
		send[1] = 0xff;
		send[2] = 0xff;
		send[3] = 0xff;
		send[4] = A2C_PRINT;
		Com_Memcpy(send+5, outputbuf, String::Length(outputbuf)+1);

		NET_SendPacket (String::Length(send)+1, send, net_from);
	}
	else if (sv_redirected == RD_CLIENT)
	{
		ClientReliableWrite_Begin (host_client, q1svc_print, String::Length(outputbuf)+3);
		ClientReliableWrite_Byte (host_client, PRINT_HIGH);
		ClientReliableWrite_String (host_client, outputbuf);
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
void SV_BeginRedirect (redirect_t rd)
{
	sv_redirected = rd;
	outputbuf[0] = 0;
}

void SV_EndRedirect (void)
{
	SV_FlushRedirect ();
	sv_redirected = RD_NONE;
}


/*
================
Con_Printf

Handles cursor positioning, line wrapping, etc
================
*/
void Con_Printf (const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	
	va_start (argptr,fmt);
	Q_vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end (argptr);

	// add to redirected message
	if (sv_redirected)
	{
		if (String::Length(msg) + String::Length(outputbuf) > (int)sizeof(outputbuf) - 1)
			SV_FlushRedirect ();
		String::Cat(outputbuf, sizeof(outputbuf), msg);
		return;
	}

	Sys_Print(msg);	// also echo to debugging console
	if (sv_logfile)
		FS_Printf(sv_logfile, "%s", msg);
}

/*
================
Con_DPrintf

A Con_Printf that only shows up if the "developer" cvar is set
================
*/
void Con_DPrintf (const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	if (!com_developer || !com_developer->value)
		return;

	va_start (argptr,fmt);
	Q_vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end (argptr);
	
	Con_Printf ("%s", msg);
}

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

static void SV_PrintToClient(client_t *cl, int level, char *string)
{
	ClientReliableWrite_Begin (cl, q1svc_print, String::Length(string)+3);
	ClientReliableWrite_Byte (cl, level);
	ClientReliableWrite_String (cl, string);
}


/*
=================
SV_ClientPrintf

Sends text across to be displayed if the level passes
=================
*/
void SV_ClientPrintf (client_t *cl, int level, const char *fmt, ...)
{
	va_list		argptr;
	char		string[MAXPRINTMSG];
	
	if (level < cl->messagelevel)
		return;
	
	va_start (argptr,fmt);
	Q_vsnprintf(string, MAXPRINTMSG, fmt, argptr);
	va_end (argptr);

	SV_PrintToClient(cl, level, string);
}

/*
=================
SV_BroadcastPrintf

Sends text to all active clients
=================
*/
void SV_BroadcastPrintf (int level, const char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];
	client_t	*cl;
	int			i;

	va_start (argptr,fmt);
	Q_vsnprintf(string, 1024, fmt, argptr);
	va_end (argptr);
	
	Sys_Print(string);	// print to the console

	for (i=0, cl = svs.clients ; i<MAX_CLIENTS_QW ; i++, cl++)
	{
		if (level < cl->messagelevel)
			continue;
		if (!cl->state)
			continue;

		SV_PrintToClient(cl, level, string);
	}
}

/*
=================
SV_BroadcastCommand

Sends text to all active clients
=================
*/
void SV_BroadcastCommand (const char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];
	
	if (!sv.state)
		return;
	va_start (argptr,fmt);
	Q_vsnprintf(string, 1024, fmt, argptr);
	va_end (argptr);

	sv.reliable_datagram.WriteByte(q1svc_stufftext);
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
void SV_Multicast (vec3_t origin, int to)
{
	client_t	*client;
	byte		*mask;
	int			leafnum;
	int			j;
	qboolean	reliable;

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
		SV_Error ("SV_Multicast: bad to:%i", to);
	}

	// send the data to all relevent clients
	for (j = 0, client = svs.clients; j < MAX_CLIENTS_QW; j++, client++)
	{
		if (client->state != cs_spawned)
			continue;

		if (to == MULTICAST_PHS_R || to == MULTICAST_PHS) {
			vec3_t delta;
			VectorSubtract(origin, client->edict->GetOrigin(), delta);
			if (VectorLength(delta) <= 1024)
				goto inrange;
		}

		if (mask)
		{
			leafnum = CM_PointLeafnum(client->edict->GetOrigin());
			leafnum = CM_LeafCluster(leafnum);
			if (leafnum < 0 || !(mask[leafnum >> 3] & (1 << (leafnum & 7))))
			{
//				Con_Printf ("supressed multicast\n");
				continue;
			}
		}

inrange:
		if (reliable) {
			ClientReliableCheckBlock(client, sv.multicast.cursize);
			ClientReliableWrite_SZ(client, sv.multicast._data, sv.multicast.cursize);
		} else
			client->datagram.WriteData(sv.multicast._data, sv.multicast.cursize);
	}

	sv.multicast.Clear();
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
void SV_StartSound (qhedict_t *entity, int channel, const char *sample, int volume,
    float attenuation)
{       
    int         sound_num;
    int			field_mask;
    int			i;
	int			ent;
	vec3_t		origin;
	qboolean	use_phs;
	qboolean	reliable = false;

	if (volume < 0 || volume > 255)
		SV_Error ("SV_StartSound: volume = %i", volume);

	if (attenuation < 0 || attenuation > 4)
		SV_Error ("SV_StartSound: attenuation = %f", attenuation);

	if (channel < 0 || channel > 15)
		SV_Error ("SV_StartSound: channel = %i", channel);

// find precache number for sound
    for (sound_num=1 ; sound_num<MAX_SOUNDS_Q1
        && sv.sound_precache[sound_num] ; sound_num++)
        if (!String::Cmp(sample, sv.sound_precache[sound_num]))
            break;
    
    if ( sound_num == MAX_SOUNDS_Q1 || !sv.sound_precache[sound_num] )
    {
        Con_Printf ("SV_StartSound: %s not precacheed\n", sample);
        return;
    }
    
	ent = NUM_FOR_EDICT(entity);

	if ((channel & 8) || !sv_phs->value)	// no PHS flag
	{
		if (channel & 8)
			reliable = true; // sounds that break the phs are reliable
		use_phs = false;
		channel &= 7;
	}
	else
		use_phs = true;

//	if (channel == CHAN_BODY || channel == CHAN_VOICE)
//		reliable = true;

	channel = (ent<<3) | channel;

	field_mask = 0;
	if (volume != DEFAULT_SOUND_PACKET_VOLUME)
		channel |= SND_VOLUME;
	if (attenuation != DEFAULT_SOUND_PACKET_ATTENUATION)
		channel |= SND_ATTENUATION;

	// use the entity origin unless it is a bmodel
	if (entity->GetSolid() == SOLID_BSP)
	{
		for (i=0 ; i<3 ; i++)
			origin[i] = entity->GetOrigin()[i]+0.5*(entity->GetMins()[i]+entity->GetMaxs()[i]);
	}
	else
	{
		VectorCopy (entity->GetOrigin(), origin);
	}

	sv.multicast.WriteByte(q1svc_sound);
	sv.multicast.WriteShort(channel);
	if (channel & SND_VOLUME)
		sv.multicast.WriteByte(volume);
	if (channel & SND_ATTENUATION)
		sv.multicast.WriteByte(attenuation*64);
	sv.multicast.WriteByte(sound_num);
	for (i=0 ; i<3 ; i++)
		sv.multicast.WriteCoord(origin[i]);

	if (use_phs)
		SV_Multicast (origin, reliable ? MULTICAST_PHS_R : MULTICAST_PHS);
	else
		SV_Multicast (origin, reliable ? MULTICAST_ALL_R : MULTICAST_ALL);
}           


/*
===============================================================================

FRAME UPDATES

===============================================================================
*/

int		sv_nailmodel, sv_supernailmodel, sv_playermodel;

void SV_FindModelNumbers (void)
{
	int		i;

	sv_nailmodel = -1;
	sv_supernailmodel = -1;
	sv_playermodel = -1;

	for (i=0 ; i<MAX_MODELS_Q1 ; i++)
	{
		if (!sv.model_precache[i])
			break;
		if (!String::Cmp(sv.model_precache[i],"progs/spike.mdl"))
			sv_nailmodel = i;
		if (!String::Cmp(sv.model_precache[i],"progs/s_spike.mdl"))
			sv_supernailmodel = i;
		if (!String::Cmp(sv.model_precache[i],"progs/player.mdl"))
			sv_playermodel = i;
	}
}


/*
==================
SV_WriteClientdataToMessage

==================
*/
void SV_WriteClientdataToMessage (client_t *client, QMsg *msg)
{
	int		i;
	qhedict_t	*other;
	qhedict_t	*ent;

	ent = client->edict;

	// send the chokecount for cl_netgraph
	if (client->chokecount)
	{
		msg->WriteByte(qwsvc_chokecount);
		msg->WriteByte(client->chokecount);
		client->chokecount = 0;
	}

	// send a damage message if the player got hit this frame
	if (ent->GetDmgTake() || ent->GetDmgSave())
	{
		other = PROG_TO_EDICT(ent->GetDmgInflictor());
		msg->WriteByte(q1svc_damage);
		msg->WriteByte(ent->GetDmgSave());
		msg->WriteByte(ent->GetDmgTake());
		for (i=0 ; i<3 ; i++)
			msg->WriteCoord(other->GetOrigin()[i] + 0.5*(other->GetMins()[i] + other->GetMaxs()[i]));
	
		ent->SetDmgTake(0);
		ent->SetDmgSave(0);
	}

	// a fixangle might get lost in a dropped packet.  Oh well.
	if ( ent->GetFixAngle())
	{
		msg->WriteByte(q1svc_setangle);
		for (i=0 ; i < 3 ; i++)
			msg->WriteAngle(ent->GetAngles()[i] );
		ent->SetFixAngle(0);
	}
}

/*
=======================
SV_UpdateClientStats

Performs a delta update of the stats array.  This should only be performed
when a reliable message can be delivered this frame.
=======================
*/
void SV_UpdateClientStats (client_t *client)
{
	qhedict_t	*ent;
	int		stats[MAX_CL_STATS];
	int		i;
	
	ent = client->edict;
	Com_Memset(stats, 0, sizeof(stats));
	
	// if we are a spectator and we are tracking a player, we get his stats
	// so our status bar reflects his
	if (client->spectator && client->spec_track > 0)
		ent = svs.clients[client->spec_track - 1].edict;

	stats[STAT_HEALTH] = ent->GetHealth();
	stats[STAT_WEAPON] = SV_ModelIndex(PR_GetString(ent->GetWeaponModel()));
	stats[STAT_AMMO] = ent->GetCurrentAmmo();
	stats[STAT_ARMOR] = ent->GetArmorValue();
	stats[STAT_SHELLS] = ent->GetAmmoShells();
	stats[STAT_NAILS] = ent->GetAmmoNails();
	stats[STAT_ROCKETS] = ent->GetAmmoRockets();
	stats[STAT_CELLS] = ent->GetAmmoCells();
	if (!client->spectator)
		stats[STAT_ACTIVEWEAPON] = ent->GetWeapon();
	// stuff the sigil bits into the high bits of items for sbar
	stats[STAT_ITEMS] = (int)ent->GetItems() | ((int)pr_global_struct->serverflags << 28);

	for (i=0 ; i<MAX_CL_STATS ; i++)
		if (stats[i] != client->stats[i])
		{
			client->stats[i] = stats[i];
			if (stats[i] >=0 && stats[i] <= 255)
			{
				ClientReliableWrite_Begin(client, q1svc_updatestat, 3);
				ClientReliableWrite_Byte(client, i);
				ClientReliableWrite_Byte(client, stats[i]);
			}
			else
			{
				ClientReliableWrite_Begin(client, qwsvc_updatestatlong, 6);
				ClientReliableWrite_Byte(client, i);
				ClientReliableWrite_Long(client, stats[i]);
			}
		}
}

/*
=======================
SV_SendClientDatagram
=======================
*/
qboolean SV_SendClientDatagram (client_t *client)
{
	byte		buf[MAX_DATAGRAM_QW];
	QMsg		msg;

	msg.InitOOB(buf, sizeof(buf));
	msg.allowoverflow = true;

	// add the client specific data to the datagram
	SV_WriteClientdataToMessage (client, &msg);

	// send over all the objects that are in the PVS
	// this will include clients, a packetentities, and
	// possibly a nails update
	SV_WriteEntitiesToClient (client, &msg);

	// copy the accumulated multicast datagram
	// for this client out to the message
	if (client->datagram.overflowed)
		Con_Printf ("WARNING: datagram overflowed for %s\n", client->name);
	else
		msg.WriteData(client->datagram._data, client->datagram.cursize);
	client->datagram.Clear();

	// send deltas over reliable stream
	if (Netchan_CanReliable (&client->netchan))
		SV_UpdateClientStats (client);

	if (msg.overflowed)
	{
		Con_Printf ("WARNING: msg overflowed for %s\n", client->name);
		msg.Clear();
	}

	// send the datagram
	Netchan_Transmit (&client->netchan, msg.cursize, buf);

	return true;
}

/*
=======================
SV_UpdateToReliableMessages
=======================
*/
void SV_UpdateToReliableMessages (void)
{
	int			i, j;
	client_t *client;
	eval_t *val;
	qhedict_t *ent;

// check for changes to be sent over the reliable streams to all clients
	for (i=0, host_client = svs.clients ; i<MAX_CLIENTS_QW ; i++, host_client++)
	{
		if (host_client->state != cs_spawned)
			continue;
		if (host_client->sendinfo)
		{
			host_client->sendinfo = false;
			SV_FullClientUpdate (host_client, &sv.reliable_datagram);
		}
		if (host_client->old_frags != host_client->edict->GetFrags())
		{
			for (j=0, client = svs.clients ; j<MAX_CLIENTS_QW ; j++, client++)
			{
				if (client->state < cs_connected)
					continue;
				ClientReliableWrite_Begin(client, q1svc_updatefrags, 4);
				ClientReliableWrite_Byte(client, i);
				ClientReliableWrite_Short(client, host_client->edict->GetFrags());
			}

			host_client->old_frags = host_client->edict->GetFrags();
		}

		// maxspeed/entgravity changes
		ent = host_client->edict;

		val = GetEdictFieldValue(ent, "gravity");
		if (val && host_client->entgravity != val->_float) {
			host_client->entgravity = val->_float;
			ClientReliableWrite_Begin(host_client, qwsvc_entgravity, 5);
			ClientReliableWrite_Float(host_client, host_client->entgravity);
		}
		val = GetEdictFieldValue(ent, "maxspeed");
		if (val && host_client->maxspeed != val->_float) {
			host_client->maxspeed = val->_float;
			ClientReliableWrite_Begin(host_client, qwsvc_maxspeed, 5);
			ClientReliableWrite_Float(host_client, host_client->maxspeed);
		}

	}

	if (sv.datagram.overflowed)
		sv.datagram.Clear();

	// append the broadcast messages to each client messages
	for (j=0, client = svs.clients ; j<MAX_CLIENTS_QW ; j++, client++)
	{
		if (client->state < cs_connected)
			continue;	// reliables go to all connected or spawned

		ClientReliableCheckBlock(client, sv.reliable_datagram.cursize);
		ClientReliableWrite_SZ(client, sv.reliable_datagram._data, sv.reliable_datagram.cursize);

		if (client->state != cs_spawned)
			continue;	// datagrams only go to spawned
		client->datagram.WriteData(sv.datagram._data, sv.datagram.cursize);
	}

	sv.reliable_datagram.Clear();
	sv.datagram.Clear();
}

#ifdef _WIN32
#pragma optimize( "", off )
#endif



/*
=======================
SV_SendClientMessages
=======================
*/
void SV_SendClientMessages (void)
{
	int			i, j;
	client_t	*c;

// update frags, names, etc
	SV_UpdateToReliableMessages ();

// build individual updates
	for (i=0, c = svs.clients ; i<MAX_CLIENTS_QW ; i++, c++)
	{
		if (!c->state)
			continue;

		if (c->drop) {
			SV_DropClient(c);
			c->drop = false;
			continue;
		}

		// check to see if we have a backbuf to stick in the reliable
		if (c->num_backbuf) {
			// will it fit?
			if (c->netchan.message.cursize + c->backbuf_size[0] <
				c->netchan.message.maxsize) {

				Con_DPrintf("%s: backbuf %d bytes\n",
					c->name, c->backbuf_size[0]);

				// it'll fit
				c->netchan.message.WriteData(c->backbuf_data[0],
					c->backbuf_size[0]);
				
				//move along, move along
				for (j = 1; j < c->num_backbuf; j++) {
					Com_Memcpy(c->backbuf_data[j - 1], c->backbuf_data[j],
						c->backbuf_size[j]);
					c->backbuf_size[j - 1] = c->backbuf_size[j];
				}

				c->num_backbuf--;
				if (c->num_backbuf) {
					c->backbuf.InitOOB(c->backbuf_data[c->num_backbuf - 1], sizeof(c->backbuf_data[c->num_backbuf - 1]));
					c->backbuf.cursize = c->backbuf_size[c->num_backbuf - 1];
				}
			}
		}

		// if the reliable message overflowed,
		// drop the client
		if (c->netchan.message.overflowed)
		{
			c->netchan.message.Clear();
			c->datagram.Clear();
			SV_BroadcastPrintf (PRINT_HIGH, "%s overflowed\n", c->name);
			Con_Printf ("WARNING: reliable overflow for %s\n",c->name);
			SV_DropClient (c);
			c->send_message = true;
			c->netchan.clearTime = 0;	// don't choke this message
		}

		// only send messages if the client has sent one
		// and the bandwidth is not choked
		if (!c->send_message)
			continue;
		c->send_message = false;	// try putting this after choke?
		if (!sv.paused && !Netchan_CanPacket (&c->netchan))
		{
			c->chokecount++;
			continue;		// bandwidth choke
		}

		if (c->state == cs_spawned)
			SV_SendClientDatagram (c);
		else
			Netchan_Transmit (&c->netchan, 0, NULL);	// just update reliable
			
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
void SV_SendMessagesToAll (void)
{
	int			i;
	client_t	*c;

	for (i=0, c = svs.clients ; i<MAX_CLIENTS_QW ; i++, c++)
		if (c->state)		// FIXME: should this only send to active?
			c->send_message = true;
	
	SV_SendClientMessages ();
}

