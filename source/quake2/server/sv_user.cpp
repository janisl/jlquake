/*
Copyright (C) 1997-2001 Id Software, Inc.

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

#include "server.h"

/*
============================================================

USER STRINGCMD EXECUTION

sv_client and sv_player will be valid.
============================================================
*/

/*
==================
SV_ExecuteClientCommand
==================
*/
void SV_ExecuteClientCommand(client_t* cl, const char* s, bool clientOK, bool preMapRestart)
{
	ucmd_t* u;

	Cmd_TokenizeString(s, true);

	for (u = q2_ucmds; u->name; u++)
		if (!String::Cmp(Cmd_Argv(0), u->name))
		{
			u->func(cl);
			break;
		}

	if (!u->name && sv.state == SS_GAME)
	{
		ge->ClientCommand(cl->q2_edict);
	}
}

/*
===========================================================================

USER CMD EXECUTION

===========================================================================
*/

bool SVQ2_ParseMove(client_t* cl, QMsg& net_message, bool& move_issued)
{
	if (move_issued)
	{
		// someone is trying to cheat...
		return false;
	}
	move_issued = true;

	int checksumIndex = net_message.readcount;
	int checksum = net_message.ReadByte();
	int lastframe = net_message.ReadLong();

	if (lastframe != cl->q2_lastframe)
	{
		cl->q2_lastframe = lastframe;
		if (cl->q2_lastframe > 0)
		{
			cl->q2_frame_latency[cl->q2_lastframe & (LATENCY_COUNTS - 1)] =
				svs.realtime - cl->q2_frames[cl->q2_lastframe & UPDATE_MASK_Q2].senttime;
		}
	}

	q2usercmd_t nullcmd;
	Com_Memset(&nullcmd, 0, sizeof(nullcmd));
	q2usercmd_t oldest, oldcmd, newcmd;
	MSGQ2_ReadDeltaUsercmd(&net_message, &nullcmd, &oldest);
	MSGQ2_ReadDeltaUsercmd(&net_message, &oldest, &oldcmd);
	MSGQ2_ReadDeltaUsercmd(&net_message, &oldcmd, &newcmd);

	if (cl->state != CS_ACTIVE)
	{
		cl->q2_lastframe = -1;
		return true;
	}

	// if the checksum fails, ignore the rest of the packet
	int calculatedChecksum = COM_BlockSequenceCRCByte(
		net_message._data + checksumIndex + 1,
		net_message.readcount - checksumIndex - 1,
		cl->netchan.incomingSequence);

	if (calculatedChecksum != checksum)
	{
		common->DPrintf("Failed command checksum for %s (%d != %d)/%d\n",
			cl->name, calculatedChecksum, checksum,
			cl->netchan.incomingSequence);
		return false;
	}

	if (!sv_paused->value)
	{
		int net_drop = cl->netchan.dropped;
		if (net_drop < 20)
		{
			while (net_drop > 2)
			{
				SVQ2_ClientThink(cl, &cl->q2_lastUsercmd);

				net_drop--;
			}
			if (net_drop > 1)
			{
				SVQ2_ClientThink(cl, &oldest);
			}

			if (net_drop > 0)
			{
				SVQ2_ClientThink(cl, &oldcmd);
			}

		}
		SVQ2_ClientThink(cl, &newcmd);
	}

	cl->q2_lastUsercmd = newcmd;
	return true;
}

bool SVQ2_ParseStringCommand(client_t* cl, QMsg& net_message, int& stringCmdCount)
{
	enum { MAX_STRINGCMDS = 8 };

	const char* s = net_message.ReadString2();

	// malicious users may try using too many string commands
	if (++stringCmdCount < MAX_STRINGCMDS)
	{
		SV_ExecuteClientCommand(cl, s, true, false);
	}

	if (cl->state == CS_ZOMBIE)
	{
		return false;	// disconnect command
	}
	return true;
}

/*
===================
SV_ExecuteClientMessage

The current net_message is parsed for the given client
===================
*/
void SV_ExecuteClientMessage(client_t* cl)
{
	// only allow one move command
	bool move_issued = false;
	int stringCmdCount = 0;

	while (1)
	{
		if (net_message.readcount > net_message.cursize)
		{
			common->Printf("SV_ReadClientMessage: badread\n");
			SVQ2_DropClient(cl);
			return;
		}

		int c = net_message.ReadByte();
		if (c == -1)
		{
			break;
		}

		switch (c)
		{
		default:
			common->Printf("SV_ReadClientMessage: unknown command char\n");
			SVQ2_DropClient(cl);
			return;
		case q2clc_nop:
			break;
		case q2clc_userinfo:
			SVQ2_ParseUserInfo(cl, net_message);
			break;
		case q2clc_move:
			if (!SVQ2_ParseMove(cl, net_message, move_issued))
			{
				return;
			}
			break;
		case q2clc_stringcmd:
			if (!SVQ2_ParseStringCommand(cl, net_message, stringCmdCount))
			{
				return;
			}
			break;
		}
	}
}
