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

#include "server.h"

Cvar* sv_fps;					// time rate for running non-clients

Cvar* sv_showloss;				// report when usercmds are lost
Cvar* sv_killserver;			// menu system can set to 1 to shut server down
Cvar* sv_mapChecksum;
Cvar* sv_serverid;

#define HEARTBEAT_MSEC      300 * 1000
#define Q3HEARTBEAT_GAME    "QuakeArena-1"
#define WSMHEARTBEAT_GAME   "Wolfenstein-1"
#define WMHEARTBEAT_DEAD    "WolfFlatline-1"
#define ETHEARTBEAT_GAME    "EnemyTerritory-1"
#define ETHEARTBEAT_DEAD    "ETFlatline-1"			// NERVE - SMF

/*
=================
SV_ReadPackets
=================
*/
void SV_PacketEvent(netadr_t from, QMsg* msg)
{
	int i;
	client_t* cl;
	int qport;

	// check for connectionless packet (0xffffffff) first
	if (msg->cursize >= 4 && *(int*)msg->_data == -1)
	{
		SVT3_ConnectionlessPacket(from, msg);
		return;
	}

	// read the qport out of the message so we can fix up
	// stupid address translating routers
	msg->BeginReadingOOB();
	msg->ReadLong();				// sequence number
	qport = msg->ReadShort() & 0xffff;

	// find which client the message is from
	for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++,cl++)
	{
		if (cl->state == CS_FREE)
		{
			continue;
		}
		if (!SOCK_CompareBaseAdr(from, cl->netchan.remoteAddress))
		{
			continue;
		}
		// it is possible to have multiple clients from a single IP
		// address, so they are differentiated by the qport variable
		if (cl->netchan.qport != qport)
		{
			continue;
		}

		// the IP port can't be used to differentiate them, because
		// some address translating routers periodically change UDP
		// port assignments
		if (cl->netchan.remoteAddress.port != from.port)
		{
			common->Printf("SV_PacketEvent: fixing up a translated port\n");
			cl->netchan.remoteAddress.port = from.port;
		}

		// make sure it is a valid, in sequence packet
		if (SVT3_Netchan_Process(cl, msg))
		{
			// zombie clients still need to do the Netchan_Process
			// to make sure they don't need to retransmit the final
			// reliable message, but they don't do any other processing
			if (cl->state != CS_ZOMBIE)
			{
				cl->q3_lastPacketTime = svs.q3_time;	// don't timeout
				SVT3_ExecuteClientMessage(cl, msg);
			}
		}
		return;
	}

	// if we received a sequenced packet from an address we don't recognize,
	// send an out of band disconnect packet to it
	NET_OutOfBandPrint(NS_SERVER, from, "disconnect");
}


/*
==================
SV_Frame

Player movement occurs as a result of packet events, which
happen before SV_Frame is called
==================
*/
void SV_Frame(int msec)
{
	int frameMsec;
	int startTime;

	// the menu kills the server with this cvar
	if (sv_killserver->integer)
	{
		SVT3_Shutdown("Server was killed.\n");
		Cvar_Set("sv_killserver", "0");
		return;
	}

	if (!com_sv_running->integer)
	{
		return;
	}

	// allow pause if only the local client is connected
	if (SVT3_CheckPaused())
	{
		return;
	}

	// if it isn't time for the next frame, do nothing
	if (sv_fps->integer < 1)
	{
		Cvar_Set("sv_fps", "10");
	}
	frameMsec = 1000 / sv_fps->integer;

	sv.q3_timeResidual += msec;

	if (!com_dedicated->integer)
	{
		SVT3_BotFrame(svs.q3_time + sv.q3_timeResidual);
	}

	if (com_dedicated->integer && sv.q3_timeResidual < frameMsec)
	{
		// NET_Sleep will give the OS time slices until either get a packet
		// or time enough for a server frame has gone by
		NET_Sleep(frameMsec - sv.q3_timeResidual);
		return;
	}

	// if time is about to hit the 32nd bit, kick all clients
	// and clear sv.time, rather
	// than checking for negative time wraparound everywhere.
	// 2giga-milliseconds = 23 days, so it won't be too often
	if (svs.q3_time > 0x70000000)
	{
		SVT3_Shutdown("Restarting server due to time wrapping");
		Cbuf_AddText("vstr nextmap\n");
		return;
	}
	// this can happen considerably earlier when lots of clients play and the map doesn't change
	if (svs.q3_nextSnapshotEntities >= 0x7FFFFFFE - svs.q3_numSnapshotEntities)
	{
		SVT3_Shutdown("Restarting server due to numSnapshotEntities wrapping");
		Cbuf_AddText("vstr nextmap\n");
		return;
	}

	if (sv.q3_restartTime && svs.q3_time >= sv.q3_restartTime)
	{
		sv.q3_restartTime = 0;
		Cbuf_AddText("map_restart 0\n");
		return;
	}

	// update infostrings if anything has been changed
	if (cvar_modifiedFlags & CVAR_SERVERINFO)
	{
		SVT3_SetConfigstring(Q3CS_SERVERINFO, Cvar_InfoString(CVAR_SERVERINFO, MAX_INFO_STRING_Q3));
		cvar_modifiedFlags &= ~CVAR_SERVERINFO;
	}
	if (cvar_modifiedFlags & CVAR_SYSTEMINFO)
	{
		SVT3_SetConfigstring(Q3CS_SYSTEMINFO, Cvar_InfoString(CVAR_SYSTEMINFO, BIG_INFO_STRING));
		cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
	}

	if (com_speeds->integer)
	{
		startTime = Sys_Milliseconds();
	}
	else
	{
		startTime = 0;	// quite a compiler warning
	}

	// update ping based on the all received frames
	SVT3_CalcPings();

	if (com_dedicated->integer)
	{
		SVT3_BotFrame(svs.q3_time);
	}

	// run the game simulation in chunks
	while (sv.q3_timeResidual >= frameMsec)
	{
		sv.q3_timeResidual -= frameMsec;
		svs.q3_time += frameMsec;

		// let everything in the world think and move
		SVT3_GameRunFrame(svs.q3_time);
	}

	if (com_speeds->integer)
	{
		time_game = Sys_Milliseconds() - startTime;
	}

	// check timeouts
	SVT3_CheckTimeouts();

	// send messages back to the clients
	SVT3_SendClientMessages();

	// send a heartbeat to the master if needed
	SVT3_MasterHeartbeat(Q3HEARTBEAT_GAME);
}

//============================================================================


bool CL_IsServerActive()
{
	return sv.state == SS_GAME;
}

void CL_Disconnect()
{
}
void SCRQH_BeginLoadingPlaque()
{
}
void CL_EstablishConnection(const char* name)
{
}
void Host_Reconnect_f()
{
}
void Cmd_ForwardToServer()
{
}
