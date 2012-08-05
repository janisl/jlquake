// sv_user.c -- server code for moving users

/*
 * $Header: /H2 Mission Pack/SV_USER.C 6     3/13/98 1:51p Mgummelt $
 */

#include "quakedef.h"
#include "../client/public.h"

Cvar* sv_idealrollscale;

/*
===================
SV_ReadClientMove
===================
*/
void SV_ReadClientMove(h2usercmd_t* move)
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

	if (bits & 4)	// crouched?
	{
		host_client->qh_edict->SetFlags2(((int)host_client->qh_edict->GetFlags2()) | FL2_CROUCHED);
	}
	else
	{
		host_client->qh_edict->SetFlags2(((int)host_client->qh_edict->GetFlags2()) & (~FL2_CROUCHED));
	}

	i = net_message.ReadByte();
	if (i)
	{
		host_client->qh_edict->SetImpulse(i);
	}

// read light level
	host_client->qh_edict->SetLightLevel(net_message.ReadByte());
}

void SV_ExecuteClientCommand(client_t* cl, const char* s, bool clientOK, bool preMapRestart)
{
	Cmd_TokenizeString(s);

	for (ucmd_t* u = h2_ucmds; u->name; u++)
	{
		if (!String::Cmp(Cmd_Argv(0), u->name))
		{
			u->func(cl);
			return;
		}
	}

	common->DPrintf("%s tried to %s\n", host_client->name, s);
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
	char* s;

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

			case h2clc_nop:
//				common->Printf ("h2clc_nop\n");
				break;

			case h2clc_stringcmd:
				s = const_cast<char*>(net_message.ReadString2());
				SV_ExecuteClientCommand(host_client, s, true, false);
				break;

			case h2clc_disconnect:
//				common->Printf ("SV_ReadClientMessage: client disconnected\n");
				return false;

			case h2clc_move:
				SV_ReadClientMove(&host_client->h2_lastUsercmd);
				break;

			case h2clc_inv_select:
				host_client->qh_edict->SetInventory(net_message.ReadByte());
				break;

			case h2clc_frame:
				host_client->h2_last_frame = net_message.ReadByte();
				host_client->h2_last_sequence = net_message.ReadByte();
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
			Com_Memset(&host_client->h2_lastUsercmd, 0, sizeof(host_client->h2_lastUsercmd));
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
