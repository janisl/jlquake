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
// sv_user.c -- server code for moving users

#include "quakedef.h"

/*
===================
SV_ReadClientMove
===================
*/
void SV_ReadClientMove(q1usercmd_t* move)
{
	int i;
	vec3_t angle;
	int bits;

// read ping time
	host_client->qh_ping_times[host_client->qh_num_pings % NUM_PING_TIMES]
		= sv.qh_time - net_message.ReadFloat();
	host_client->qh_num_pings++;

// read current angles
	for (i = 0; i < 3; i++)
		angle[i] = net_message.ReadAngle();

	host_client->qh_edict->SetVAngle(angle);

// read movement
	move->forwardmove = net_message.ReadShort();
	move->sidemove = net_message.ReadShort();
	move->upmove = net_message.ReadShort();

// read buttons
	bits = net_message.ReadByte();
	host_client->qh_edict->SetButton0(bits & 1);
	host_client->qh_edict->SetButton2((bits & 2) >> 1);

	i = net_message.ReadByte();
	if (i)
	{
		host_client->qh_edict->SetImpulse(i);
	}
}

void SV_ExecuteClientCommand(client_t* cl, const char* s, bool clientOK, bool preMapRestart)
{
	Cmd_TokenizeString(s);

	for (ucmd_t* u = q1_ucmds; u->name; u++)
	{
		if (!String::Cmp(Cmd_Argv(0), u->name))
		{
			u->func(cl);
			return;
		}
	}

	common->DPrintf("%s tried to %s\n", cl->name, s);
}

/*
===================
SV_ReadClientMessage

Returns false if the client should be killed
===================
*/
qboolean SV_ReadClientMessage(void)
{
	int ret;
	int cmd;
	const char* s;

	do
	{
nextmsg:
		ret = NET_GetMessage(host_client->qh_netconnection, &host_client->netchan, &net_message);
		if (ret == -1)
		{
			common->Printf("SV_ReadClientMessage: NET_GetMessage failed\n");
			return false;
		}
		if (!ret)
		{
			return true;
		}

		net_message.BeginReadingOOB();

		while (1)
		{
			if (host_client->state < CS_CONNECTED)
			{
				return false;	// a command caused an error

			}
			if (net_message.badread)
			{
				common->Printf("SV_ReadClientMessage: badread\n");
				return false;
			}

			cmd = net_message.ReadChar();

			switch (cmd)
			{
			case -1:
				goto nextmsg;		// end of message

			default:
				common->Printf("SV_ReadClientMessage: unknown command char\n");
				return false;

			case q1clc_nop:
//				common->Printf ("q1clc_nop\n");
				break;

			case q1clc_stringcmd:
				s = net_message.ReadString2();
				SV_ExecuteClientCommand(host_client, s, true, false);
				break;

			case q1clc_disconnect:
//				common->Printf ("SV_ReadClientMessage: client disconnected\n");
				return false;

			case q1clc_move:
				SV_ReadClientMove(&host_client->q1_lastUsercmd);
				break;
			}
		}
	}
	while (ret == 1);

	return true;
}


/*
==================
SV_RunClients
==================
*/
void SV_RunClients(void)
{
	int i;

	for (i = 0, host_client = svs.clients; i < svs.qh_maxclients; i++, host_client++)
	{
		if (host_client->state < CS_CONNECTED)
		{
			continue;
		}

		if (!SV_ReadClientMessage())
		{
			SVQH_DropClient(host_client, false);	// client misbehaved...
			continue;
		}

		if (host_client->state != CS_ACTIVE)
		{
			// clear client movement until a new packet is received
			Com_Memset(&host_client->q1_lastUsercmd, 0, sizeof(host_client->q1_lastUsercmd));
			continue;
		}

// always pause in single player if in console or menus
#ifdef DEDICATED
		if (!sv.qh_paused)
#else
		if (!sv.qh_paused && (svs.qh_maxclients > 1 || in_keyCatchers == 0))
#endif
		{
			SVQH_ClientThink(host_client, host_frametime);
		}
	}
}
