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
#include "../../server/quake2/local.h"

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
