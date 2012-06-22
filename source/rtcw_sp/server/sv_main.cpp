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
Cvar* sv_gametype;
Cvar* sv_pure;
Cvar* sv_floodProtect;
Cvar* sv_allowAnonymous;

// Rafael gameskill
Cvar* sv_gameskill;
// done

Cvar* sv_reloading;		//----(SA)	added

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

/*
===============
SV_ExpandNewlines

Converts newlines to "\n" so a line prints nicer
===============
*/
char* SV_ExpandNewlines(char* in)
{
	static char string[1024];
	int l;

	l = 0;
	while (*in && l < (int)sizeof(string) - 3)
	{
		if (*in == '\n')
		{
			string[l++] = '\\';
			string[l++] = 'n';
		}
		else
		{
			string[l++] = *in;
		}
		in++;
	}
	string[l] = 0;

	return string;
}

/*
======================
SV_ReplacePendingServerCommands

  This is ugly
======================
*/
#if 0	// RF, not used anymore
int SV_ReplacePendingServerCommands(client_t* client, const char* cmd)
{
	int i, index, csnum1, csnum2;

	for (i = client->reliableSent + 1; i <= client->reliableSequence; i++)
	{
		index = i & (MAX_RELIABLE_COMMANDS_WS - 1);
		//
		//if ( !String::NCmp(cmd, client->reliableCommands[ index ], String::Length("cs")) ) {
		if (!String::NCmp(cmd, SV_GetReliableCommand(client, index), String::Length("cs")))
		{
			sscanf(cmd, "cs %i", &csnum1);
			//sscanf(client->reliableCommands[ index ], "cs %i", &csnum2);
			sscanf(SV_GetReliableCommand(client, index), "cs %i", &csnum2);
			if (csnum1 == csnum2)
			{
				//String::NCpyZ( client->reliableCommands[ index ], cmd, sizeof( client->reliableCommands[ index ] ) );

				/*
				if ( client->netchan.remoteAddress.type != NA_BOT ) {
				    Com_Printf( "WARNING: client %i removed double pending config string %i: %s\n", client-svs.clients, csnum1, cmd );
				}
				*/
				return qtrue;
			}
		}
	}
	return qfalse;
}
#endif

/*
======================
SV_AddServerCommand

The given command will be transmitted to the client, and is guaranteed to
not have future snapshot_t executed before it is executed
======================
*/
void SV_AddServerCommand(client_t* client, const char* cmd)
{
	int index, i;







	client->q3_reliableSequence++;
	// if we would be losing an old command that hasn't been acknowledged,
	// we must drop the connection
	// we check == instead of >= so a broadcast print added by SV_DropClient()
	// doesn't cause a recursive drop client
	if (client->q3_reliableSequence - client->q3_reliableAcknowledge == MAX_RELIABLE_COMMANDS_WS + 1)
	{
		Com_Printf("===== pending server commands =====\n");
		for (i = client->q3_reliableAcknowledge + 1; i <= client->q3_reliableSequence; i++)
		{
			//Com_Printf( "cmd %5d: %s\n", i, client->reliableCommands[ i & (MAX_RELIABLE_COMMANDS_WS-1) ] );
			Com_Printf("cmd %5d: %s\n", i, SV_GetReliableCommand(client, i & (MAX_RELIABLE_COMMANDS_WS - 1)));
		}
		Com_Printf("cmd %5d: %s\n", i, cmd);
		SV_DropClient(client, "Server command overflow");
		return;
	}
	index = client->q3_reliableSequence & (MAX_RELIABLE_COMMANDS_WS - 1);
	//String::NCpyZ( client->reliableCommands[ index ], cmd, sizeof( client->reliableCommands[ index ] ) );
	SV_AddReliableCommand(client, index, cmd);
}


/*
=================
SV_SendServerCommand

Sends a reliable command string to be interpreted by
the client game module: "cp", "print", "chat", etc
A NULL client will broadcast to all clients
=================
*/
void QDECL SV_SendServerCommand(client_t* cl, const char* fmt, ...)
{
	va_list argptr;
	byte message[MAX_MSGLEN_WOLF];
	client_t* client;
	int j;

	va_start(argptr,fmt);
	vsprintf((char*)message, fmt,argptr);
	va_end(argptr);

	if (cl != NULL)
	{
		SV_AddServerCommand(cl, (char*)message);
		return;
	}

	// hack to echo broadcast prints to console
	if (com_dedicated->integer && !String::NCmp((char*)message, "print", 5))
	{
		Com_Printf("broadcast: %s\n", SV_ExpandNewlines((char*)message));
	}

	// send the data to all relevent clients
	for (j = 0, client = svs.clients; j < sv_maxclients->integer; j++, client++)
	{
		if (client->state < CS_PRIMED)
		{
			continue;
		}
		// Ridah, don't need to send messages to AI
		if (client->ws_gentity && client->ws_gentity->r.svFlags & WSSVF_CASTAI)
		{
			continue;
		}
		// done.
		SV_AddServerCommand(client, (char*)message);
	}
}


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
#define HEARTBEAT_GAME  "Wolfenstein-1"
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
			Com_Printf("%s resolved to %i.%i.%i.%i:%i\n", sv_master[i]->string,
				adr[i].ip[0], adr[i].ip[1], adr[i].ip[2], adr[i].ip[3],
				BigShort(adr[i].port));
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
	char status[MAX_MSGLEN_WOLF];
	int i;
	client_t* cl;
	wsplayerState_t* ps;
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
			ps = SVWS_GameClientNum(i);
			String::Sprintf(player, sizeof(player), "%i %i \"%s\"\n",
				ps->persistant[WSPERS_SCORE], cl->ping, cl->name);
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
	if (Cvar_VariableValue("g_gametype") == Q3GT_SINGLE_PLAYER)
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
	Info_SetValueForKey(infostring, "gametype", va("%i", sv_gametype->integer), MAX_INFO_STRING_Q3);
	Info_SetValueForKey(infostring, "pure", va("%i", sv_pure->integer), MAX_INFO_STRING_Q3);

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
	Info_SetValueForKey(infostring, "sv_allowAnonymous", va("%i", sv_allowAnonymous->integer), MAX_INFO_STRING_Q3);

	// Rafael gameskill
	Info_SetValueForKey(infostring, "gameskill", va("%i", sv_gameskill->integer), MAX_INFO_STRING_Q3);
	// done

	NET_OutOfBandPrint(NS_SERVER, from, "infoResponse\n%s", infostring);
}

/*
==============
SV_FlushRedirect

==============
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
	int i;
	char remaining[1024];
#define SV_OUTPUTBUF_LENGTH (MAX_MSGLEN_WOLF - 16)
	char sv_outputbuf[SV_OUTPUTBUF_LENGTH];

	if (!String::Length(sv_rconPassword->string) ||
		String::Cmp(Cmd_Argv(1), sv_rconPassword->string))
	{
		valid = qfalse;
		Com_DPrintf("Bad rcon from %s:\n%s\n", SOCK_AdrToString(from), Cmd_Argv(2));
	}
	else
	{
		valid = qtrue;
		Com_DPrintf("Rcon from %s:\n%s\n", SOCK_AdrToString(from), Cmd_Argv(2));
	}

	// start redirecting all print outputs to the packet
	svs.q3_redirectAddress = from;
	Com_BeginRedirect(sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);

	if (!String::Length(sv_rconPassword->string))
	{
		Com_Printf("No rconpassword set.\n");
	}
	else if (!valid)
	{
		Com_Printf("Bad rconpassword.\n");
	}
	else
	{
		remaining[0] = 0;

		for (i = 2; i < Cmd_Argc(); i++)
		{
			strcat(remaining, Cmd_Argv(i));
			strcat(remaining, " ");
		}

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

	s = msg->ReadStringLine();

	Cmd_TokenizeString(s);

	c = Cmd_Argv(0);
	Com_DPrintf("SV packet %s : %s\n", SOCK_AdrToString(from), c);

	if (!String::ICmp(c,"getstatus"))
	{
		SVC_Status(from);
	}
	else if (!String::ICmp(c,"getinfo"))
	{
		SVC_Info(from);
	}
	else if (!String::ICmp(c,"getchallenge"))
	{
		SV_GetChallenge(from);
	}
	else if (!String::ICmp(c,"connect"))
	{
		SV_DirectConnect(from);
	}
	else if (!String::ICmp(c,"ipAuthorize"))
	{
		SV_AuthorizeIpPacket(from);
	}
	else if (!String::ICmp(c, "rcon"))
	{
		SVC_RemoteCommand(from, msg);
	}
	else if (!String::ICmp(c,"disconnect"))
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
			Com_Printf("SV_ReadPackets: fixing up a translated port\n");
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

	// if we received a sequenced packet from an address we don't reckognize,
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
	wsplayerState_t* ps;

	for (i = 0; i < sv_maxclients->integer; i++)
	{
		cl = &svs.clients[i];
		if (cl->state != CS_ACTIVE)
		{
			cl->ping = 999;
			continue;
		}
		if (!cl->ws_gentity)
		{
			cl->ping = 999;
			continue;
		}
		if (cl->ws_gentity->r.svFlags & Q3SVF_BOT)
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
		ps = SVWS_GameClientNum(i);
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
			Com_DPrintf("Going from CS_ZOMBIE to CS_FREE for %s\n", cl->name);
			cl->state = CS_FREE;	// can now be reused
			continue;
		}
		// Ridah, AI's don't time out
		if (cl->ws_gentity && !(cl->ws_gentity->r.svFlags & WSSVF_CASTAI))
		{
			if (cl->state >= CS_CONNECTED && cl->q3_lastPacketTime < droppoint)
			{
				// wait several frames so a debugger session doesn't
				// cause a timeout
				if (++cl->q3_timeoutCount > 5)
				{
					SV_DropClient(cl, "timed out");
					cl->state = CS_FREE;	// don't bother with zombie state
				}
			}
			else
			{
				cl->q3_timeoutCount = 0;
			}
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
		sv_paused->integer = 0;
		return qfalse;
	}

	sv_paused->integer = 1;
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
		SV_BotFrame(svs.q3_time + sv.q3_timeResidual);
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
		SV_SetConfigstring(Q3CS_SERVERINFO, Cvar_InfoString(CVAR_SERVERINFO, MAX_INFO_STRING_Q3));
		cvar_modifiedFlags &= ~CVAR_SERVERINFO;
	}
	if (cvar_modifiedFlags & CVAR_SYSTEMINFO)
	{
		SV_SetConfigstring(Q3CS_SYSTEMINFO, Cvar_InfoString(CVAR_SYSTEMINFO, BIG_INFO_STRING));
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
		SV_BotFrame(svs.q3_time);
	}

	// run the game simulation in chunks
	while (sv.q3_timeResidual >= frameMsec)
	{
		sv.q3_timeResidual -= frameMsec;
		svs.q3_time += frameMsec;

		// let everything in the world think and move
		VM_Call(gvm, WSGAME_RUN_FRAME, svs.q3_time);
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
