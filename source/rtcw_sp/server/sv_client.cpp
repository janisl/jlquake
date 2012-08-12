/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// sv_client.c -- server code for dealing with clients

#include "server.h"

/*
============================================================

CLIENT COMMAND EXECUTION

============================================================
*/

/*
==================
SV_DoneDownload_f

Downloads are finished
==================
*/
void SV_DoneDownload_f(client_t* cl)
{
	common->DPrintf("clientDownload: %s Done\n", cl->name);
	// resend the game state to update any clients that entered during the download
	SVT3_SendClientGameState(cl);
}

/*
=================
SV_VerifyPaks_f

If we are pure, disconnect the client if they do no meet the following conditions:

1. the first two checksums match our view of cgame and ui
   Wolf specific: the checksum is the checksum of the pk3 we found the DLL in
2. there are no any additional checksums that we do not have

This routine would be a bit simpler with a goto but i abstained

=================
*/
static void SV_VerifyPaks_f(client_t* cl)
{
	int nChkSum1, nChkSum2, nClientPaks, nServerPaks, i, j, nCurArg;
	int nClientChkSum[1024];
	int nServerChkSum[1024];
	const char* pPaks, * pArg;
	qboolean bGood = true;

	// if we are pure, we "expect" the client to load certain things from
	// certain pk3 files, namely we want the client to have loaded the
	// ui and cgame that we think should be loaded based on the pure setting
	if (svt3_pure->integer != 0)
	{

		bGood = true;
		nChkSum1 = nChkSum2 = 0;
		// we run the game, so determine which cgame and ui the client "should" be running
		bGood = (FS_FileIsInPAK(GGameType & (GAME_WolfMP | GAME_ET) ? "cgame_mp_x86.dll" : "vm/cgame.qvm", &nChkSum1) == 1);
		if (bGood)
		{
			bGood = (FS_FileIsInPAK(GGameType & (GAME_WolfMP | GAME_ET) ? "ui_mp_x86.dll" : "vm/ui.qvm", &nChkSum2) == 1);
		}

		nClientPaks = Cmd_Argc();

		// start at arg 2 ( skip serverId cl_paks )
		nCurArg = 1;

		pArg = Cmd_Argv(nCurArg++);
		if (!pArg)
		{
			bGood = false;
		}
		else
		{
			// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=475
			// we may get incoming cp sequences from a previous checksumFeed, which we need to ignore
			// since serverId is a frame count, it always goes up
			if (String::Atoi(pArg) < sv.q3_checksumFeedServerId)
			{
				common->DPrintf("ignoring outdated cp command from client %s\n", cl->name);
				return;
			}
		}

		// we basically use this while loop to avoid using 'goto' :)
		while (bGood)
		{

			// must be at least 6: "cl_paks cgame ui @ firstref ... numChecksums"
			// numChecksums is encoded
			if (nClientPaks < 6)
			{
				bGood = false;
				break;
			}
			// verify first to be the cgame checksum
			pArg = Cmd_Argv(nCurArg++);
			if (!pArg || *pArg == '@' || String::Atoi(pArg) != nChkSum1)
			{
				bGood = false;
				break;
			}
			// verify the second to be the ui checksum
			pArg = Cmd_Argv(nCurArg++);
			if (!pArg || *pArg == '@' || String::Atoi(pArg) != nChkSum2)
			{
				bGood = false;
				break;
			}
			// should be sitting at the delimeter now
			pArg = Cmd_Argv(nCurArg++);
			if (*pArg != '@')
			{
				bGood = false;
				break;
			}
			// store checksums since tokenization is not re-entrant
			for (i = 0; nCurArg < nClientPaks; i++)
			{
				nClientChkSum[i] = String::Atoi(Cmd_Argv(nCurArg++));
			}

			// store number to compare against (minus one cause the last is the number of checksums)
			nClientPaks = i - 1;

			// make sure none of the client check sums are the same
			// so the client can't send 5 the same checksums
			for (i = 0; i < nClientPaks; i++)
			{
				for (j = 0; j < nClientPaks; j++)
				{
					if (i == j)
					{
						continue;
					}
					if (nClientChkSum[i] == nClientChkSum[j])
					{
						bGood = false;
						break;
					}
				}
				if (bGood == false)
				{
					break;
				}
			}
			if (bGood == false)
			{
				break;
			}

			// get the pure checksums of the pk3 files loaded by the server
			pPaks = FS_LoadedPakPureChecksums();
			Cmd_TokenizeString(pPaks);
			nServerPaks = Cmd_Argc();
			if (nServerPaks > 1024)
			{
				nServerPaks = 1024;
			}

			for (i = 0; i < nServerPaks; i++)
			{
				nServerChkSum[i] = String::Atoi(Cmd_Argv(i));
			}

			// check if the client has provided any pure checksums of pk3 files not loaded by the server
			for (i = 0; i < nClientPaks; i++)
			{
				for (j = 0; j < nServerPaks; j++)
				{
					if (nClientChkSum[i] == nServerChkSum[j])
					{
						break;
					}
				}
				if (j >= nServerPaks)
				{
					bGood = false;
					break;
				}
			}
			if (bGood == false)
			{
				break;
			}

			// check if the number of checksums was correct
			nChkSum1 = sv.q3_checksumFeed;
			for (i = 0; i < nClientPaks; i++)
			{
				nChkSum1 ^= nClientChkSum[i];
			}
			nChkSum1 ^= nClientPaks;
			if (nChkSum1 != nClientChkSum[nClientPaks])
			{
				bGood = false;
				break;
			}

			// break out
			break;
		}

		cl->q3_gotCP = true;

		if (bGood)
		{
			cl->q3_pureAuthentic = 1;
		}
		else
		{
			cl->q3_pureAuthentic = 0;
			cl->q3_nextSnapshotTime = -1;
			cl->state = CS_ACTIVE;
			SVT3_SendClientSnapshot(cl);
			SVT3_DropClient(cl, "Unpure client detected. Invalid .PK3 files referenced!");
		}
	}
}

static ucmd_t ucmds[] = {
	{"userinfo", SVT3_UpdateUserinfo_f},
	{"disconnect", SVT3_Disconnect_f},
	{"cp", SV_VerifyPaks_f},
	{"vdr", SVT3_ResetPureClient_f},
	{"download", SVT3_BeginDownload_f},
	{"nextdl", SVT3_NextDownload_f},
	{"stopdl", SVT3_StopDownload_f},
	{"donedl", SV_DoneDownload_f},
	{NULL, NULL}
};

/*
==================
SV_ExecuteClientCommand

Also called by bot code
==================
*/
void SV_ExecuteClientCommand(client_t* cl, const char* s, bool clientOK, bool preMapRestart)
{
	ucmd_t* u;

	Cmd_TokenizeString(s);

	// see if it is a server level command
	for (u = ucmds; u->name; u++)
	{
		if (!String::Cmp(Cmd_Argv(0), u->name))
		{
			u->func(cl);
			break;
		}
	}

	if (clientOK)
	{
		// pass unknown strings to the game
		if (!u->name && sv.state == SS_GAME)
		{
			VM_Call(gvm, WSGAME_CLIENT_COMMAND, cl - svs.clients);
		}
	}
}

/*
===============
SV_ClientCommand
===============
*/
static qboolean SV_ClientCommand(client_t* cl, QMsg* msg)
{
	int seq;
	const char* s;
	qboolean clientOk = true;

	seq = msg->ReadLong();
	s = msg->ReadString();

	// see if we have already executed it
	if (cl->q3_lastClientCommand >= seq)
	{
		return true;
	}

	common->DPrintf("clientCommand: %s : %i : %s\n", cl->name, seq, s);

	// drop the connection if we have somehow lost commands
	if (seq > cl->q3_lastClientCommand + 1)
	{
		common->Printf("Client %s lost %i clientCommands\n", cl->name,
			seq - cl->q3_lastClientCommand + 1);
		SVT3_DropClient(cl, "Lost reliable commands");
		return false;
	}

	// malicious users may try using too many string commands
	// to lag other players.  If we decide that we want to stall
	// the command, we will stop processing the rest of the packet,
	// including the usercmd.  This causes flooders to lag themselves
	// but not other people
	// We don't do this when the client hasn't been active yet since its
	// normal to spam a lot of commands when downloading
	if (!com_cl_running->integer &&
		cl->state >= CS_ACTIVE &&		// (SA) this was commented out in Wolf.  Did we do that?
		sv_floodProtect->integer &&
		svs.q3_time < cl->q3_nextReliableTime)
	{
		// ignore any other text messages from this client but let them keep playing
		clientOk = false;
		common->DPrintf("client text ignored for %s\n", cl->name);
		//return false;	// stop processing
	}

	// don't allow another command for one second
	cl->q3_nextReliableTime = svs.q3_time + 1000;

	SV_ExecuteClientCommand(cl, s, clientOk, false);

	cl->q3_lastClientCommand = seq;
	String::Sprintf(cl->q3_lastClientCommandString, sizeof(cl->q3_lastClientCommandString), "%s", s);

	return true;		// continue procesing
}


//==================================================================================

/*
==================
SV_UserMove

The message usually contains all the movement commands
that were in the last three packets, so that the information
in dropped packets can be recovered.

On very fast clients, there may be multiple usercmd packed into
each of the backup packets.
==================
*/
static void SV_UserMove(client_t* cl, QMsg* msg, qboolean delta)
{
	int i, key;
	int cmdCount;
	wsusercmd_t nullcmd;
	wsusercmd_t cmds[MAX_PACKET_USERCMDS];
	wsusercmd_t* cmd, * oldcmd;

	if (delta)
	{
		cl->q3_deltaMessage = cl->q3_messageAcknowledge;
	}
	else
	{
		cl->q3_deltaMessage = -1;
	}

	cmdCount = msg->ReadByte();

	if (cmdCount < 1)
	{
		common->Printf("cmdCount < 1\n");
		return;
	}

	if (cmdCount > MAX_PACKET_USERCMDS)
	{
		common->Printf("cmdCount > MAX_PACKET_USERCMDS\n");
		return;
	}

	// use the checksum feed in the key
	key = sv.q3_checksumFeed;
	// also use the message acknowledge
	key ^= cl->q3_messageAcknowledge;
	// also use the last acknowledged server command in the key
	//key ^= Com_HashKey(cl->reliableCommands[ cl->reliableAcknowledge & (MAX_RELIABLE_COMMANDS_WOLF-1) ], 32);
	key ^= Com_HashKey(SVT3_GetReliableCommand(cl, cl->q3_reliableAcknowledge & (MAX_RELIABLE_COMMANDS_WOLF - 1)), 32);

	memset(&nullcmd, 0, sizeof(nullcmd));
	oldcmd = &nullcmd;
	for (i = 0; i < cmdCount; i++)
	{
		cmd = &cmds[i];
		MSGWS_ReadDeltaUsercmdKey(msg, key, oldcmd, cmd);
		oldcmd = cmd;
	}

	// save time for ping calculation
	cl->q3_frames[cl->q3_messageAcknowledge & PACKET_MASK_Q3].messageAcked = svs.q3_time;

	// if this is the first usercmd we have received
	// this gamestate, put the client into the world
	if (cl->state == CS_PRIMED)
	{
		SVWS_ClientEnterWorld(cl, &cmds[0]);
		// the moves can be processed normaly
	}
	//
	if (svt3_pure->integer != 0 && cl->q3_pureAuthentic == 0)
	{
		SVT3_DropClient(cl, "Cannot validate pure client!");
		return;
	}

	if (cl->state != CS_ACTIVE)
	{
		cl->q3_deltaMessage = -1;
		return;
	}

	// usually, the first couple commands will be duplicates
	// of ones we have previously received, but the servertimes
	// in the commands will cause them to be immediately discarded
	for (i =  0; i < cmdCount; i++)
	{
		// if this is a cmd from before a map_restart ignore it
		if (cmds[i].serverTime > cmds[cmdCount - 1].serverTime)
		{
			continue;
		}
		// extremely lagged or cmd from before a map_restart
		//if ( cmds[i].serverTime > svs.q3_time + 3000 ) {
		//	continue;
		//}
		if (svt3_gametype->integer != Q3GT_SINGLE_PLAYER)		// RF, we need to allow this in single player, where loadgame's can cause the player to freeze after reloading if we do this check
		{	// don't execute if this is an old cmd which is already executed
			// these old cmds are included when cl_packetdup > 0
			if (cmds[i].serverTime <= cl->ws_lastUsercmd.serverTime)		// Q3_MISSIONPACK
			{	//			if ( cmds[i].serverTime > cmds[cmdCount-1].serverTime ) {
				continue;	// from just before a map_restart
			}
		}
		SVWS_ClientThink(cl, &cmds[i]);
	}
}


/*
===========================================================================

USER CMD EXECUTION

===========================================================================
*/

/*
===================
SV_ExecuteClientMessage

Parse a client packet
===================
*/
void SV_ExecuteClientMessage(client_t* cl, QMsg* msg)
{
	int c;
	int serverId;

	msg->Bitstream();

	serverId = msg->ReadLong();
	cl->q3_messageAcknowledge = msg->ReadLong();

	if (cl->q3_messageAcknowledge < 0)
	{
		// usually only hackers create messages like this
		// it is more annoying for them to let them hanging
		//SVT3_DropClient( cl, "illegible client message" );
		return;
	}

	cl->q3_reliableAcknowledge = msg->ReadLong();

	// NOTE: when the client message is fux0red the acknowledgement numbers
	// can be out of range, this could cause the server to send thousands of server
	// commands which the server thinks are not yet acknowledged in SVT3_UpdateServerCommandsToClient
	if (cl->q3_reliableAcknowledge < cl->q3_reliableSequence - MAX_RELIABLE_COMMANDS_WOLF)
	{
		// usually only hackers create messages like this
		// it is more annoying for them to let them hanging
		//SVT3_DropClient( cl, "illegible client message" );
		cl->q3_reliableAcknowledge = cl->q3_reliableSequence;
		return;
	}
	// if this is a usercmd from a previous gamestate,
	// ignore it or retransmit the current gamestate
	//
	// if the client was downloading, let it stay at whatever serverId and
	// gamestate it was at.  This allows it to keep downloading even when
	// the gamestate changes.  After the download is finished, we'll
	// notice and send it a new game state
	if (serverId != sv.q3_serverId &&
		!*cl->downloadName)
	{
		if (serverId == sv.q3_restartedServerId)
		{
			// they just haven't caught the map_restart yet
			return;
		}
		// if we can tell that the client has dropped the last
		// gamestate we sent them, resend it
		if (cl->q3_messageAcknowledge > cl->q3_gamestateMessageNum)
		{
			common->DPrintf("%s : dropped gamestate, resending\n", cl->name);
			SVT3_SendClientGameState(cl);
		}
		return;
	}

	// RF, kill any reliableCommands that have been acknowledged
	SVWS_FreeAcknowledgedReliableCommands(cl);

	// read optional clientCommand strings
	do
	{
		c = msg->ReadByte();
		if (c == q3clc_EOF)
		{
			break;
		}
		if (c != q3clc_clientCommand)
		{
			break;
		}
		if (!SV_ClientCommand(cl, msg))
		{
			return;	// we couldn't execute it because of the flood protection
		}
		if (cl->state == CS_ZOMBIE)
		{
			return;	// disconnect command
		}
	}
	while (1);

	// read the wsusercmd_t
	if (c == q3clc_move)
	{
		SV_UserMove(cl, msg, true);
	}
	else if (c == q3clc_moveNoDelta)
	{
		SV_UserMove(cl, msg, false);
	}
	else if (c != q3clc_EOF)
	{
		common->Printf("WARNING: bad command byte for client %i\n", cl - svs.clients);
	}
//	if ( msg->readcount != msg->cursize ) {
//		common->Printf( "WARNING: Junk at end of packet for client %i\n", cl - svs.clients );
//	}
}
