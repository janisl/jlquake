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
Cvar* sv_timeout;				// seconds without any message
Cvar* sv_zombietime;			// seconds to sink messages after disconnect
Cvar* sv_rconPassword;			// password for remote server commands
Cvar* sv_privatePassword;		// password for the privateClient slots
Cvar* sv_allowDownload;

Cvar* sv_privateClients;		// number of clients reserved for password
Cvar* sv_hostname;
Cvar* sv_master[MAX_MASTER_SERVERS];		// master server ip address
Cvar* sv_reconnectlimit;		// minimum seconds between connect messages
Cvar* sv_showloss;				// report when usercmds are lost
Cvar* sv_padPackets;			// add nop bytes to messages
Cvar* sv_killserver;			// menu system can set to 1 to shut server down
Cvar* sv_mapname;
Cvar* sv_mapChecksum;
Cvar* sv_serverid;
Cvar* sv_maxRate;
Cvar* sv_minPing;
Cvar* sv_maxPing;
Cvar* sv_floodProtect;
Cvar* sv_lanForceRate;		// dedicated 1 (LAN) server forces local client rates to 99999 (bug #491)
Cvar* sv_strictAuth;

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

/*
==============================================================================

MASTER SERVER FUNCTIONS

==============================================================================
*/

/*
================
SV_MasterHeartbeat

Send a message to the masters every few minutes to
let it know we are alive, and log information.
We will also have a heartbeat sent when a server
changes from empty to non-empty, and full to non-full,
but not on every player enter or exit.
================
*/
#define HEARTBEAT_MSEC  300 * 1000
#define HEARTBEAT_GAME  "QuakeArena-1"
void SV_MasterHeartbeat(void)
{
	static netadr_t adr[MAX_MASTER_SERVERS];
	int i;

	// "dedicated 1" is for lan play, "dedicated 2" is for inet public play
	if (!com_dedicated || com_dedicated->integer != 2)
	{
		return;		// only dedicated servers send heartbeats
	}

	// if not time yet, don't send anything
	if (svs.q3_time < svs.q3_nextHeartbeatTime)
	{
		return;
	}
	svs.q3_nextHeartbeatTime = svs.q3_time + HEARTBEAT_MSEC;


	// send to group masters
	for (i = 0; i < MAX_MASTER_SERVERS; i++)
	{
		if (!sv_master[i]->string[0])
		{
			continue;
		}

		// see if we haven't already resolved the name
		// resolving usually causes hitches on win95, so only
		// do it when needed
		if (sv_master[i]->modified)
		{
			sv_master[i]->modified = qfalse;

			Com_Printf("Resolving %s\n", sv_master[i]->string);
			if (!SOCK_StringToAdr(sv_master[i]->string, &adr[i], PORT_MASTER))
			{
				// if the address failed to resolve, clear it
				// so we don't take repeated dns hits
				Com_Printf("Couldn't resolve address: %s\n", sv_master[i]->string);
				Cvar_Set(sv_master[i]->name, "");
				sv_master[i]->modified = qfalse;
				continue;
			}
			Com_Printf("%s resolved to %s\n", sv_master[i]->string, SOCK_AdrToString(adr[i]));
		}


		Com_Printf("Sending heartbeat to %s\n", sv_master[i]->string);
		// this command should be changed if the server info / status format
		// ever incompatably changes
		NET_OutOfBandPrint(NS_SERVER, adr[i], "heartbeat %s\n", HEARTBEAT_GAME);
	}
}

/*
=================
SV_MasterShutdown

Informs all masters that this server is going down
=================
*/
void SV_MasterShutdown(void)
{
	// send a hearbeat right now
	svs.q3_nextHeartbeatTime = -9999;
	SV_MasterHeartbeat();

	// send it again to minimize chance of drops
	svs.q3_nextHeartbeatTime = -9999;
	SV_MasterHeartbeat();

	// when the master tries to poll the server, it won't respond, so
	// it will be removed from the list
}


/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/

/*
================
SVC_Status

Responds with all the info that qplug or qspy can see about the server
and all connected players.  Used for getting detailed information after
the simple info query.
================
*/
void SVC_Status(netadr_t from)
{
	char player[1024];
	char status[MAX_MSGLEN_Q3];
	int i;
	client_t* cl;
	q3playerState_t* ps;
	int statusLength;
	int playerLength;
	char infostring[MAX_INFO_STRING_Q3];

	// ignore if we are in single player
	if (Cvar_VariableValue("g_gametype") == Q3GT_SINGLE_PLAYER)
	{
		return;
	}

	String::Cpy(infostring, Cvar_InfoString(CVAR_SERVERINFO, MAX_INFO_STRING_Q3));

	// echo back the parameter to status. so master servers can use it as a challenge
	// to prevent timed spoofed reply packets that add ghost servers
	Info_SetValueForKey(infostring, "challenge", Cmd_Argv(1), MAX_INFO_STRING_Q3);

	status[0] = 0;
	statusLength = 0;

	for (i = 0; i < sv_maxclients->integer; i++)
	{
		cl = &svs.clients[i];
		if (cl->state >= CS_CONNECTED)
		{
			ps = SVQ3_GameClientNum(i);
			String::Sprintf(player, sizeof(player), "%i %i \"%s\"\n",
				ps->persistant[Q3PERS_SCORE], cl->ping, cl->name);
			playerLength = String::Length(player);
			if (statusLength + playerLength >= (int)sizeof(status))
			{
				break;		// can't hold any more
			}
			String::Cpy(status + statusLength, player);
			statusLength += playerLength;
		}
	}

	NET_OutOfBandPrint(NS_SERVER, from, "statusResponse\n%s\n%s", infostring, status);
}

/*
================
SVC_Info

Responds with a short info message that should be enough to determine
if a user is interested in a server to do a full status
================
*/
void SVC_Info(netadr_t from)
{
	int i, count;
	const char* gamedir;
	char infostring[MAX_INFO_STRING_Q3];

	// ignore if we are in single player
	if (Cvar_VariableValue("g_gametype") == Q3GT_SINGLE_PLAYER || Cvar_VariableValue("ui_singlePlayerActive"))
	{
		return;
	}

	// don't count privateclients
	count = 0;
	for (i = sv_privateClients->integer; i < sv_maxclients->integer; i++)
	{
		if (svs.clients[i].state >= CS_CONNECTED)
		{
			count++;
		}
	}

	infostring[0] = 0;

	// echo back the parameter to status. so servers can use it as a challenge
	// to prevent timed spoofed reply packets that add ghost servers
	Info_SetValueForKey(infostring, "challenge", Cmd_Argv(1), MAX_INFO_STRING_Q3);

	Info_SetValueForKey(infostring, "protocol", va("%i", PROTOCOL_VERSION), MAX_INFO_STRING_Q3);
	Info_SetValueForKey(infostring, "hostname", sv_hostname->string, MAX_INFO_STRING_Q3);
	Info_SetValueForKey(infostring, "mapname", sv_mapname->string, MAX_INFO_STRING_Q3);
	Info_SetValueForKey(infostring, "clients", va("%i", count), MAX_INFO_STRING_Q3);
	Info_SetValueForKey(infostring, "sv_maxclients",
		va("%i", sv_maxclients->integer - sv_privateClients->integer), MAX_INFO_STRING_Q3);
	Info_SetValueForKey(infostring, "gametype", va("%i", svt3_gametype->integer), MAX_INFO_STRING_Q3);
	Info_SetValueForKey(infostring, "pure", va("%i", svt3_pure->integer), MAX_INFO_STRING_Q3);

	if (sv_minPing->integer)
	{
		Info_SetValueForKey(infostring, "minPing", va("%i", sv_minPing->integer), MAX_INFO_STRING_Q3);
	}
	if (sv_maxPing->integer)
	{
		Info_SetValueForKey(infostring, "maxPing", va("%i", sv_maxPing->integer), MAX_INFO_STRING_Q3);
	}
	gamedir = Cvar_VariableString("fs_game");
	if (*gamedir)
	{
		Info_SetValueForKey(infostring, "game", gamedir, MAX_INFO_STRING_Q3);
	}

	NET_OutOfBandPrint(NS_SERVER, from, "infoResponse\n%s", infostring);
}

/*
================
SVC_FlushRedirect

================
*/
void SV_FlushRedirect(char* outputbuf)
{
	NET_OutOfBandPrint(NS_SERVER, svs.q3_redirectAddress, "print\n%s", outputbuf);
}

/*
===============
SVC_RemoteCommand

An rcon packet arrived from the network.
Shift down the remaining args
Redirect all printfs
===============
*/
void SVC_RemoteCommand(netadr_t from, QMsg* msg)
{
	qboolean valid;
	unsigned int time;
	char remaining[1024];
	// TTimo - scaled down to accumulate, but not overflow anything network wise, print wise etc.
	// (OOB messages are the bottleneck here)
#define SV_OUTPUTBUF_LENGTH (1024 - 16)
	char sv_outputbuf[SV_OUTPUTBUF_LENGTH];
	static unsigned int lasttime = 0;
	char* cmd_aux;

	// TTimo - https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=534
	time = Com_Milliseconds();
	if (time < (lasttime + 500))
	{
		return;
	}
	lasttime = time;

	if (!String::Length(sv_rconPassword->string) ||
		String::Cmp(Cmd_Argv(1), sv_rconPassword->string))
	{
		valid = qfalse;
		Com_Printf("Bad rcon from %s:\n%s\n", SOCK_AdrToString(from), Cmd_Argv(2));
	}
	else
	{
		valid = qtrue;
		Com_Printf("Rcon from %s:\n%s\n", SOCK_AdrToString(from), Cmd_Argv(2));
	}

	// start redirecting all print outputs to the packet
	svs.q3_redirectAddress = from;
	Com_BeginRedirect(sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);

	if (!String::Length(sv_rconPassword->string))
	{
		Com_Printf("No rconpassword set on the server.\n");
	}
	else if (!valid)
	{
		Com_Printf("Bad rconpassword.\n");
	}
	else
	{
		remaining[0] = 0;

		// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=543
		// get the command directly, "rcon <pass> <command>" to avoid quoting issues
		// extract the command by walking
		// since the cmd formatting can fuckup (amount of spaces), using a dumb step by step parsing
		cmd_aux = Cmd_Cmd();
		cmd_aux += 4;
		while (cmd_aux[0] == ' ')
			cmd_aux++;
		while (cmd_aux[0] && cmd_aux[0] != ' ')	// password
			cmd_aux++;
		while (cmd_aux[0] == ' ')
			cmd_aux++;

		String::Cat(remaining, sizeof(remaining), cmd_aux);

		Cmd_ExecuteString(remaining);

	}

	Com_EndRedirect();
}

/*
=================
SV_ConnectionlessPacket

A connectionless packet has four leading 0xff
characters to distinguish it from a game channel.
Clients that are in the game can still send
connectionless packets.
=================
*/
void SV_ConnectionlessPacket(netadr_t from, QMsg* msg)
{
	const char* s;
	char* c;

	msg->BeginReadingOOB();
	msg->ReadLong();		// skip the -1 marker

	if (!String::NCmp("connect", (char*)&msg->_data[4], 7))
	{
		Huff_Decompress(msg, 12);
	}

	s = msg->ReadStringLine();
	Cmd_TokenizeString(s);

	c = Cmd_Argv(0);
	Com_DPrintf("SV packet %s : %s\n", SOCK_AdrToString(from), c);

	if (!String::ICmp(c, "getstatus"))
	{
		SVC_Status(from);
	}
	else if (!String::ICmp(c, "getinfo"))
	{
		SVC_Info(from);
	}
	else if (!String::ICmp(c, "getchallenge"))
	{
		SV_GetChallenge(from);
	}
	else if (!String::ICmp(c, "connect"))
	{
		SV_DirectConnect(from);
	}
	else if (!String::ICmp(c, "ipAuthorize"))
	{
		SV_AuthorizeIpPacket(from);
	}
	else if (!String::ICmp(c, "rcon"))
	{
		SVC_RemoteCommand(from, msg);
	}
	else if (!String::ICmp(c, "disconnect"))
	{
		// if a client starts up a local server, we may see some spurious
		// server disconnect messages when their new server sees our final
		// sequenced messages to the old client
	}
	else
	{
		Com_DPrintf("bad connectionless packet from %s:\n%s\n",
			SOCK_AdrToString(from), s);
	}
}

//============================================================================

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
		SV_ConnectionlessPacket(from, msg);
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
			Com_Printf("SV_PacketEvent: fixing up a translated port\n");
			cl->netchan.remoteAddress.port = from.port;
		}

		// make sure it is a valid, in sequence packet
		if (SV_Netchan_Process(cl, msg))
		{
			// zombie clients still need to do the Netchan_Process
			// to make sure they don't need to retransmit the final
			// reliable message, but they don't do any other processing
			if (cl->state != CS_ZOMBIE)
			{
				cl->q3_lastPacketTime = svs.q3_time;	// don't timeout
				SV_ExecuteClientMessage(cl, msg);
			}
		}
		return;
	}

	// if we received a sequenced packet from an address we don't recognize,
	// send an out of band disconnect packet to it
	NET_OutOfBandPrint(NS_SERVER, from, "disconnect");
}


/*
===================
SV_CalcPings

Updates the cl->ping variables
===================
*/
void SV_CalcPings(void)
{
	int i, j;
	client_t* cl;
	int total, count;
	int delta;
	q3playerState_t* ps;

	for (i = 0; i < sv_maxclients->integer; i++)
	{
		cl = &svs.clients[i];
		if (cl->state != CS_ACTIVE)
		{
			cl->ping = 999;
			continue;
		}
		if (!cl->q3_gentity)
		{
			cl->ping = 999;
			continue;
		}
		if (cl->q3_gentity->r.svFlags & Q3SVF_BOT)
		{
			cl->ping = 0;
			continue;
		}

		total = 0;
		count = 0;
		for (j = 0; j < PACKET_BACKUP_Q3; j++)
		{
			if (cl->q3_frames[j].messageAcked <= 0)
			{
				continue;
			}
			delta = cl->q3_frames[j].messageAcked - cl->q3_frames[j].messageSent;
			count++;
			total += delta;
		}
		if (!count)
		{
			cl->ping = 999;
		}
		else
		{
			cl->ping = total / count;
			if (cl->ping > 999)
			{
				cl->ping = 999;
			}
		}

		// let the game dll know about the ping
		ps = SVQ3_GameClientNum(i);
		ps->ping = cl->ping;
	}
}

/*
==================
SV_CheckTimeouts

If a packet has not been received from a client for timeout->integer
seconds, drop the conneciton.  Server time is used instead of
realtime to avoid dropping the local client while debugging.

When a client is normally dropped, the client_t goes into a zombie state
for a few seconds to make sure any final reliable message gets resent
if necessary
==================
*/
void SV_CheckTimeouts(void)
{
	int i;
	client_t* cl;
	int droppoint;
	int zombiepoint;

	droppoint = svs.q3_time - 1000 * sv_timeout->integer;
	zombiepoint = svs.q3_time - 1000 * sv_zombietime->integer;

	for (i = 0,cl = svs.clients; i < sv_maxclients->integer; i++,cl++)
	{
		// message times may be wrong across a changelevel
		if (cl->q3_lastPacketTime > svs.q3_time)
		{
			cl->q3_lastPacketTime = svs.q3_time;
		}

		if (cl->state == CS_ZOMBIE &&
			cl->q3_lastPacketTime < zombiepoint)
		{
			// using the client id cause the cl->name is empty at this point
			Com_DPrintf("Going from CS_ZOMBIE to CS_FREE for client %d\n", i);
			cl->state = CS_FREE;	// can now be reused
			continue;
		}
		if (cl->state >= CS_CONNECTED && cl->q3_lastPacketTime < droppoint)
		{
			// wait several frames so a debugger session doesn't
			// cause a timeout
			if (++cl->q3_timeoutCount > 5)
			{
				SVT3_DropClient(cl, "timed out");
				cl->state = CS_FREE;	// don't bother with zombie state
			}
		}
		else
		{
			cl->q3_timeoutCount = 0;
		}
	}
}


/*
==================
SV_CheckPaused
==================
*/
qboolean SV_CheckPaused(void)
{
	int count;
	client_t* cl;
	int i;

	if (!cl_paused->integer)
	{
		return qfalse;
	}

	// only pause if there is just a single client connected
	count = 0;
	for (i = 0,cl = svs.clients; i < sv_maxclients->integer; i++,cl++)
	{
		if (cl->state >= CS_CONNECTED && cl->netchan.remoteAddress.type != NA_BOT)
		{
			count++;
		}
	}

	if (count > 1)
	{
		// don't pause
		if (sv_paused->integer)
		{
			Cvar_Set("sv_paused", "0");
		}
		return qfalse;
	}

	if (!sv_paused->integer)
	{
		Cvar_Set("sv_paused", "1");
	}
	return qtrue;
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
		SV_Shutdown("Server was killed.\n");
		Cvar_Set("sv_killserver", "0");
		return;
	}

	if (!com_sv_running->integer)
	{
		return;
	}

	// allow pause if only the local client is connected
	if (SV_CheckPaused())
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
		SV_Shutdown("Restarting server due to time wrapping");
		Cbuf_AddText("vstr nextmap\n");
		return;
	}
	// this can happen considerably earlier when lots of clients play and the map doesn't change
	if (svs.q3_nextSnapshotEntities >= 0x7FFFFFFE - svs.q3_numSnapshotEntities)
	{
		SV_Shutdown("Restarting server due to numSnapshotEntities wrapping");
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
	SV_CalcPings();

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
		VM_Call(gvm, Q3GAME_RUN_FRAME, svs.q3_time);
	}

	if (com_speeds->integer)
	{
		time_game = Sys_Milliseconds() - startTime;
	}

	// check timeouts
	SV_CheckTimeouts();

	// send messages back to the clients
	SV_SendClientMessages();

	// send a heartbeat to the master if needed
	SV_MasterHeartbeat();
}

//============================================================================


bool CL_IsServerActive()
{
	return sv.state == SS_GAME;
}
