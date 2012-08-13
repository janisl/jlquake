/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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
SV_ExecuteClientCommand

Also called by bot code
==================
*/
void SV_ExecuteClientCommand(client_t* cl, const char* s, bool clientOK, bool preMapRestart)
{
	ucmd_t* u;
	qboolean bProcessed = false;

	Cmd_TokenizeString(s);

	// see if it is a server level command
	for (u = q3_ucmds; u->name; u++)
	{
		if (!String::Cmp(Cmd_Argv(0), u->name))
		{
			u->func(cl);
			bProcessed = true;
			break;
		}
	}

	if (clientOK)
	{
		// pass unknown strings to the game
		if (!u->name && sv.state == SS_GAME)
		{
			VM_Call(gvm, Q3GAME_CLIENT_COMMAND, cl - svs.clients);
		}
	}
	else if (!bProcessed)
	{
		common->DPrintf("client text ignored for %s: %s\n", cl->name, Cmd_Argv(0));
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
		cl->state >= CS_ACTIVE &&
		sv_floodProtect->integer &&
		svs.q3_time < cl->q3_nextReliableTime)
	{
		// ignore any other text messages from this client but let them keep playing
		// TTimo - moved the ignored verbose to the actual processing in SV_ExecuteClientCommand, only printing if the core doesn't intercept
		clientOk = false;
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
	q3usercmd_t nullcmd;
	q3usercmd_t cmds[MAX_PACKET_USERCMDS];
	q3usercmd_t* cmd, * oldcmd;

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
	key ^= Com_HashKey(cl->q3_reliableCommands[cl->q3_reliableAcknowledge & (MAX_RELIABLE_COMMANDS_Q3 - 1)], 32);

	Com_Memset(&nullcmd, 0, sizeof(nullcmd));
	oldcmd = &nullcmd;
	for (i = 0; i < cmdCount; i++)
	{
		cmd = &cmds[i];
		MSGQ3_ReadDeltaUsercmdKey(msg, key, oldcmd, cmd);
		oldcmd = cmd;
	}

	// save time for ping calculation
	cl->q3_frames[cl->q3_messageAcknowledge & PACKET_MASK_Q3].messageAcked = svs.q3_time;

	// TTimo
	// catch the no-cp-yet situation before SVQ3_ClientEnterWorld
	// if CS_ACTIVE, then it's time to trigger a new gamestate emission
	// if not, then we are getting remaining parasite usermove commands, which we should ignore
	if (svt3_pure->integer != 0 && cl->q3_pureAuthentic == 0 && !cl->q3_gotCP)
	{
		if (cl->state == CS_ACTIVE)
		{
			// we didn't get a cp yet, don't assume anything and just send the gamestate all over again
			common->DPrintf("%s: didn't get cp command, resending gamestate\n", cl->name, cl->state);
			SVT3_SendClientGameState(cl);
		}
		return;
	}

	// if this is the first usercmd we have received
	// this gamestate, put the client into the world
	if (cl->state == CS_PRIMED)
	{
		SVQ3_ClientEnterWorld(cl, &cmds[0]);
		// the moves can be processed normaly
	}

	// a bad cp command was sent, drop the client
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
		// don't execute if this is an old cmd which is already executed
		// these old cmds are included when cl_packetdup > 0
		if (cmds[i].serverTime <= cl->q3_lastUsercmd.serverTime)
		{
			continue;
		}
		SVQ3_ClientThink(cl, &cmds[i]);
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
#ifndef NDEBUG
		SVT3_DropClient(cl, "DEBUG: illegible client message");
#endif
		return;
	}

	cl->q3_reliableAcknowledge = msg->ReadLong();

	// NOTE: when the client message is fux0red the acknowledgement numbers
	// can be out of range, this could cause the server to send thousands of server
	// commands which the server thinks are not yet acknowledged in SVT3_UpdateServerCommandsToClient
	if (cl->q3_reliableAcknowledge < cl->q3_reliableSequence - MAX_RELIABLE_COMMANDS_Q3)
	{
		// usually only hackers create messages like this
		// it is more annoying for them to let them hanging
#ifndef NDEBUG
		SVT3_DropClient(cl, "DEBUG: illegible client message");
#endif
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
	//
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=536
	// don't drop as long as previous command was a nextdl, after a dl is done, downloadName is set back to ""
	// but we still need to read the next message to move to next download or send gamestate
	// I don't like this hack though, it must have been working fine at some point, suspecting the fix is somewhere else
	if (serverId != sv.q3_serverId && !*cl->downloadName && !strstr(cl->q3_lastClientCommandString, "nextdl"))
	{
		if (serverId >= sv.q3_restartedServerId && serverId < sv.q3_serverId)		// TTimo - use a comparison here to catch multiple map_restart
		{	// they just haven't caught the map_restart yet
			common->DPrintf("%s : ignoring pre map_restart / outdated client message\n", cl->name);
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

	// read the q3usercmd_t
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
