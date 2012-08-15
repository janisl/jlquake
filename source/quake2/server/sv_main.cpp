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

#include "server.h"

Cvar* sv_timedemo;

Cvar* timeout;					// seconds without any message
Cvar* zombietime;				// seconds to sink messages after disconnect

Cvar* rcon_password;			// password for remote server commands

Cvar* sv_airaccelerate;

Cvar* sv_showclamp;

Cvar* public_server;			// should heartbeats be sent

Cvar* sv_reconnect_limit;		// minimum seconds between connect messages

void Master_Shutdown(void);


/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/

/*
================
SVC_Status

Responds with all the info that qplug or qspy can see
================
*/
void SVC_Status(void)
{
	NET_OutOfBandPrint(NS_SERVER, net_from, "print\n%s", SVQ2_StatusString());
#if 0
	Com_BeginRedirect(RD_PACKET, sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);
	common->Printf(SVQ2_StatusString());
	Com_EndRedirect();
#endif
}

/*
================
SVC_Ack

================
*/
void SVC_Ack(void)
{
	common->Printf("Ping acknowledge from %s\n", SOCK_AdrToString(net_from));
}

/*
================
SVC_Info

Responds with short info for broadcast scans
The second parameter should be the current protocol version number.
================
*/
void SVC_Info(void)
{
	char string[64];
	int i, count;
	int version;

	if (sv_maxclients->value == 1)
	{
		return;		// ignore in single player

	}
	version = String::Atoi(Cmd_Argv(1));

	if (version != Q2PROTOCOL_VERSION)
	{
		String::Sprintf(string, sizeof(string), "%s: wrong version\n", sv_hostname->string, sizeof(string));
	}
	else
	{
		count = 0;
		for (i = 0; i < sv_maxclients->value; i++)
			if (svs.clients[i].state >= CS_CONNECTED)
			{
				count++;
			}

		String::Sprintf(string, sizeof(string), "%16s %8s %2i/%2i\n", sv_hostname->string, sv.name, count, (int)sv_maxclients->value);
	}

	NET_OutOfBandPrint(NS_SERVER, net_from, "info\n%s", string);
}

/*
================
SVC_Ping

Just responds with an acknowledgement
================
*/
void SVC_Ping(void)
{
	NET_OutOfBandPrint(NS_SERVER, net_from, "ack");
}


/*
=================
SVC_GetChallenge

Returns a challenge number that can be used
in a subsequent client_connect command.
We do this to prevent denial of service attacks that
flood the server with invalid connection IPs.  With a
challenge, they must give a valid IP address.
=================
*/
void SVC_GetChallenge(void)
{
	int i;
	int oldest;
	int oldestTime;

	oldest = 0;
	oldestTime = 0x7fffffff;

	// see if we already have a challenge for this ip
	for (i = 0; i < MAX_CHALLENGES; i++)
	{
		if (SOCK_CompareBaseAdr(net_from, svs.challenges[i].adr))
		{
			break;
		}
		if (svs.challenges[i].time < oldestTime)
		{
			oldestTime = svs.challenges[i].time;
			oldest = i;
		}
	}

	if (i == MAX_CHALLENGES)
	{
		// overwrite the oldest
		svs.challenges[oldest].challenge = rand() & 0x7fff;
		svs.challenges[oldest].adr = net_from;
		svs.challenges[oldest].time = curtime;
		i = oldest;
	}

	// send it back
	NET_OutOfBandPrint(NS_SERVER, net_from, "challenge %i", svs.challenges[i].challenge);
}

/*
==================
SVC_DirectConnect

A connection request that did not come from the master
==================
*/
void SVC_DirectConnect(void)
{
	char userinfo[MAX_INFO_STRING_Q2];
	netadr_t adr;
	int i;
	client_t* cl, * newcl;
	client_t temp;
	q2edict_t* ent;
	int edictnum;
	int version;
	int qport;
	int challenge;

	adr = net_from;

	common->DPrintf("SVC_DirectConnect ()\n");

	version = String::Atoi(Cmd_Argv(1));
	if (version != Q2PROTOCOL_VERSION)
	{
		NET_OutOfBandPrint(NS_SERVER, adr, "print\nServer is version %4.2f.\n", VERSION);
		common->DPrintf("    rejected connect from version %i\n", version);
		return;
	}

	qport = String::Atoi(Cmd_Argv(2));

	challenge = String::Atoi(Cmd_Argv(3));

	String::NCpy(userinfo, Cmd_Argv(4), sizeof(userinfo) - 1);
	userinfo[sizeof(userinfo) - 1] = 0;

	// force the IP key/value pair so the game can filter based on ip
	Info_SetValueForKey(userinfo, "ip", SOCK_AdrToString(net_from), MAX_INFO_STRING_Q2, MAX_INFO_KEY_Q2,
		MAX_INFO_VALUE_Q2, true, false);

	// attractloop servers are ONLY for local clients
	if (sv.q2_attractloop)
	{
		if (!SOCK_IsLocalAddress(adr))
		{
			common->Printf("Remote connect in attract loop.  Ignored.\n");
			NET_OutOfBandPrint(NS_SERVER, adr, "print\nConnection refused.\n");
			return;
		}
	}

	// see if the challenge is valid
	if (!SOCK_IsLocalAddress(adr))
	{
		for (i = 0; i < MAX_CHALLENGES; i++)
		{
			if (SOCK_CompareBaseAdr(net_from, svs.challenges[i].adr))
			{
				if (challenge == svs.challenges[i].challenge)
				{
					break;		// good
				}
				NET_OutOfBandPrint(NS_SERVER, adr, "print\nBad challenge.\n");
				return;
			}
		}
		if (i == MAX_CHALLENGES)
		{
			NET_OutOfBandPrint(NS_SERVER, adr, "print\nNo challenge for address.\n");
			return;
		}
	}

	newcl = &temp;
	Com_Memset(newcl, 0, sizeof(client_t));

	// if there is already a slot for this ip, reuse it
	for (i = 0,cl = svs.clients; i < sv_maxclients->value; i++,cl++)
	{
		if (cl->state == CS_FREE)
		{
			continue;
		}
		if (SOCK_CompareBaseAdr(adr, cl->netchan.remoteAddress) &&
			(cl->netchan.qport == qport ||
			 adr.port == cl->netchan.remoteAddress.port))
		{
			if (!SOCK_IsLocalAddress(adr) && (svs.realtime - cl->q2_lastconnect) < ((int)sv_reconnect_limit->value * 1000))
			{
				common->DPrintf("%s:reconnect rejected : too soon\n", SOCK_AdrToString(adr));
				return;
			}
			common->Printf("%s:reconnect\n", SOCK_AdrToString(adr));
			newcl = cl;
			goto gotnewcl;
		}
	}

	// find a client slot
	newcl = NULL;
	for (i = 0,cl = svs.clients; i < sv_maxclients->value; i++,cl++)
	{
		if (cl->state == CS_FREE)
		{
			newcl = cl;
			break;
		}
	}
	if (!newcl)
	{
		NET_OutOfBandPrint(NS_SERVER, adr, "print\nServer is full.\n");
		common->DPrintf("Rejected a connection.\n");
		return;
	}

gotnewcl:
	// build a new connection
	// accept the new client
	// this is the only place a client_t is ever initialized
	*newcl = temp;
	edictnum = (newcl - svs.clients) + 1;
	ent = Q2_EDICT_NUM(edictnum);
	newcl->q2_edict = ent;
	newcl->challenge = challenge;	// save challenge for checksumming

	// get the game a chance to reject this connection or modify the userinfo
	if (!(ge->ClientConnect(ent, userinfo)))
	{
		if (*Info_ValueForKey(userinfo, "rejmsg"))
		{
			NET_OutOfBandPrint(NS_SERVER, adr, "print\n%s\nConnection refused.\n",
				Info_ValueForKey(userinfo, "rejmsg"));
		}
		else
		{
			NET_OutOfBandPrint(NS_SERVER, adr, "print\nConnection refused.\n");
		}
		common->DPrintf("Game rejected a connection.\n");
		return;
	}

	// parse some info from the info strings
	String::NCpy(newcl->userinfo, userinfo, MAX_INFO_STRING_Q2 - 1);
	SVQ2_UserinfoChanged(newcl);

	// send the connect packet to the client
	NET_OutOfBandPrint(NS_SERVER, adr, "client_connect");

	Netchan_Setup(NS_SERVER, &newcl->netchan, adr, qport);
	newcl->netchan.lastReceived = curtime;

	newcl->state = CS_CONNECTED;

	newcl->datagram.InitOOB(newcl->datagramBuffer, MAX_MSGLEN_Q2);
	newcl->datagram.allowoverflow = true;
	newcl->q2_lastmessage = svs.realtime;	// don't timeout
	newcl->q2_lastconnect = svs.realtime;
}

int Rcon_Validate(void)
{
	if (!String::Length(rcon_password->string))
	{
		return 0;
	}

	if (String::Cmp(Cmd_Argv(1), rcon_password->string))
	{
		return 0;
	}

	return 1;
}

/*
===============
SVC_RemoteCommand

A client issued an rcon command.
Shift down the remaining args
Redirect all printfs
===============
*/
void SVC_RemoteCommand(void)
{
	int i;
	char remaining[1024];

	i = Rcon_Validate();

	if (i == 0)
	{
		common->Printf("Bad rcon from %s:\n%s\n", SOCK_AdrToString(net_from), net_message._data + 4);
	}
	else
	{
		common->Printf("Rcon from %s:\n%s\n", SOCK_AdrToString(net_from), net_message._data + 4);
	}

	Com_BeginRedirect(sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);

	if (!Rcon_Validate())
	{
		common->Printf("Bad rcon_password.\n");
	}
	else
	{
		remaining[0] = 0;

		for (i = 2; i < Cmd_Argc(); i++)
		{
			String::Cat(remaining, sizeof(remaining), Cmd_Argv(i));
			String::Cat(remaining, sizeof(remaining), " ");
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
void SV_ConnectionlessPacket(void)
{
	char* s;
	char* c;

	net_message.BeginReadingOOB();
	net_message.ReadLong();		// skip the -1 marker

	s = const_cast<char*>(net_message.ReadStringLine2());

	Cmd_TokenizeString(s, false);

	c = Cmd_Argv(0);
	common->DPrintf("Packet %s : %s\n", SOCK_AdrToString(net_from), c);

	if (!String::Cmp(c, "ping"))
	{
		SVC_Ping();
	}
	else if (!String::Cmp(c, "ack"))
	{
		SVC_Ack();
	}
	else if (!String::Cmp(c,"status"))
	{
		SVC_Status();
	}
	else if (!String::Cmp(c,"info"))
	{
		SVC_Info();
	}
	else if (!String::Cmp(c,"getchallenge"))
	{
		SVC_GetChallenge();
	}
	else if (!String::Cmp(c,"connect"))
	{
		SVC_DirectConnect();
	}
	else if (!String::Cmp(c, "rcon"))
	{
		SVC_RemoteCommand();
	}
	else
	{
		common->Printf("bad connectionless packet from %s:\n%s\n",
			SOCK_AdrToString(net_from), s);
	}
}


//============================================================================

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

	for (i = 0; i < sv_maxclients->value; i++)
	{
		cl = &svs.clients[i];
		if (cl->state != CS_ACTIVE)
		{
			continue;
		}

#if 0
		if (cl->lastframe > 0)
		{
			cl->frame_latency[sv.framenum & (LATENCY_COUNTS - 1)] = sv.framenum - cl->lastframe + 1;
		}
		else
		{
			cl->frame_latency[sv.framenum & (LATENCY_COUNTS - 1)] = 0;
		}
#endif

		total = 0;
		count = 0;
		for (j = 0; j < LATENCY_COUNTS; j++)
		{
			if (cl->q2_frame_latency[j] > 0)
			{
				count++;
				total += cl->q2_frame_latency[j];
			}
		}
		if (!count)
		{
			cl->ping = 0;
		}
		else
#if 0
		{ cl->ping = total * 100 / count - 100; }
#else
		{ cl->ping = total / count; }
#endif

		// let the game dll know about the ping
		cl->q2_edict->client->ping = cl->ping;
	}
}


/*
===================
SV_GiveMsec

Every few frames, gives all clients an allotment of milliseconds
for their command moves.  If they exceed it, assume cheating.
===================
*/
void SV_GiveMsec(void)
{
	int i;
	client_t* cl;

	if (sv.q2_framenum & 15)
	{
		return;
	}

	for (i = 0; i < sv_maxclients->value; i++)
	{
		cl = &svs.clients[i];
		if (cl->state == CS_FREE)
		{
			continue;
		}

		cl->q2_commandMsec = 1800;		// 1600 + some slop
	}
}


/*
=================
SV_ReadPackets
=================
*/
void SV_ReadPackets(void)
{
	int i;
	client_t* cl;
	int qport;

	while (NET_GetPacket(NS_SERVER, &net_from, &net_message))
	{
		// check for connectionless packet (0xffffffff) first
		if (*(int*)net_message._data == -1)
		{
			SV_ConnectionlessPacket();
			continue;
		}

		// read the qport out of the message so we can fix up
		// stupid address translating routers
		net_message.BeginReadingOOB();
		net_message.ReadLong();		// sequence number
		net_message.ReadLong();		// sequence number
		qport = net_message.ReadShort() & 0xffff;

		// check for packets from connected clients
		for (i = 0, cl = svs.clients; i < sv_maxclients->value; i++,cl++)
		{
			if (cl->state == CS_FREE)
			{
				continue;
			}
			if (!SOCK_CompareBaseAdr(net_from, cl->netchan.remoteAddress))
			{
				continue;
			}
			if (cl->netchan.qport != qport)
			{
				continue;
			}
			if (cl->netchan.remoteAddress.port != net_from.port)
			{
				common->Printf("SV_ReadPackets: fixing up a translated port\n");
				cl->netchan.remoteAddress.port = net_from.port;
			}

			if (Netchan_Process(&cl->netchan, &net_message))
			{
				// this is a valid, sequenced packet, so process it
				cl->netchan.lastReceived = curtime;
				if (cl->state != CS_ZOMBIE)
				{
					cl->q2_lastmessage = svs.realtime;	// don't timeout
					SV_ExecuteClientMessage(cl);
				}
			}
			break;
		}

		if (i != sv_maxclients->value)
		{
			continue;
		}
	}
}

/*
==================
SV_CheckTimeouts

If a packet has not been received from a client for timeout->value
seconds, drop the conneciton.  Server frames are used instead of
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

	droppoint = svs.realtime - 1000 * timeout->value;
	zombiepoint = svs.realtime - 1000 * zombietime->value;

	for (i = 0,cl = svs.clients; i < sv_maxclients->value; i++,cl++)
	{
		// message times may be wrong across a changelevel
		if (cl->q2_lastmessage > svs.realtime)
		{
			cl->q2_lastmessage = svs.realtime;
		}

		if (cl->state == CS_ZOMBIE &&
			cl->q2_lastmessage < zombiepoint)
		{
			cl->state = CS_FREE;	// can now be reused
			continue;
		}
		if ((cl->state == CS_CONNECTED || cl->state == CS_ACTIVE) &&
			cl->q2_lastmessage < droppoint)
		{
			SVQ2_BroadcastPrintf(PRINT_HIGH, "%s timed out\n", cl->name);
			SVQ2_DropClient(cl);
			cl->state = CS_FREE;	// don't bother with zombie state
		}
	}
}

/*
================
SV_PrepWorldFrame

This has to be done before the world logic, because
player processing happens outside RunWorldFrame
================
*/
void SV_PrepWorldFrame(void)
{
	q2edict_t* ent;
	int i;

	for (i = 0; i < ge->num_edicts; i++, ent++)
	{
		ent = Q2_EDICT_NUM(i);
		// events only last for a single message
		ent->s.event = 0;
	}

}


/*
=================
SV_RunGameFrame
=================
*/
void SV_RunGameFrame(void)
{
	if (host_speeds->value)
	{
		time_before_game = Sys_Milliseconds_();
	}

	// we always need to bump framenum, even if we
	// don't run the world, otherwise the delta
	// compression can get confused when a client
	// has the "current" frame
	sv.q2_framenum++;
	sv.q2_time = sv.q2_framenum * 100;

	// don't run if paused
	if (!sv_paused->value || sv_maxclients->value > 1)
	{
		ge->RunFrame();

		// never get more than one tic behind
		if (sv.q2_time < (unsigned)svs.realtime)
		{
			if (sv_showclamp->value)
			{
				common->Printf("sv highclamp\n");
			}
			svs.realtime = sv.q2_time;
		}
	}

	if (host_speeds->value)
	{
		time_after_game = Sys_Milliseconds_();
	}

}

/*
==================
SV_Frame

==================
*/
void SV_Frame(int msec)
{
	time_before_game = time_after_game = 0;

	// if server is not active, do nothing
	if (!svs.initialized)
	{
		return;
	}

	svs.realtime += msec;

	// keep the random time dependent
	rand();

	// check timeouts
	SV_CheckTimeouts();

	// get packets from clients
	SV_ReadPackets();

	// move autonomous things around if enough time has passed
	if (!sv_timedemo->value && (unsigned)svs.realtime < sv.q2_time)
	{
		// never let the time get too far off
		if (sv.q2_time - svs.realtime > 100)
		{
			if (sv_showclamp->value)
			{
				common->Printf("sv lowclamp\n");
			}
			svs.realtime = sv.q2_time - 100;
		}
		NET_Sleep(sv.q2_time - svs.realtime);
		return;
	}

	// update ping based on the last known frame from all clients
	SV_CalcPings();

	// give the clients some timeslices
	SV_GiveMsec();

	// let everything in the world think and move
	SV_RunGameFrame();

	// send messages back to the clients that had packets read this frame
	SV_SendClientMessages();

	// save the entire world state if recording a serverdemo
	SVQ2_RecordDemoMessage();

	// send a heartbeat to the master if needed
	Master_Heartbeat();

	// clear teleport flags, etc for next frame
	SV_PrepWorldFrame();

}

//============================================================================

/*
================
Master_Heartbeat

Send a message to the master every few minutes to
let it know we are alive, and log information
================
*/
#define HEARTBEAT_SECONDS   300
void Master_Heartbeat(void)
{
	const char* string;
	int i;


	if (!com_dedicated->value)
	{
		return;		// only dedicated servers send heartbeats

	}
	if (!public_server->value)
	{
		return;		// a private dedicated game

	}
	// check for time wraparound
	if (svs.q2_last_heartbeat > svs.realtime)
	{
		svs.q2_last_heartbeat = svs.realtime;
	}

	if (svs.realtime - svs.q2_last_heartbeat < HEARTBEAT_SECONDS * 1000)
	{
		return;		// not time to send yet

	}
	svs.q2_last_heartbeat = svs.realtime;

	// send the same string that we would give for a status OOB command
	string = SVQ2_StatusString();

	// send to group master
	for (i = 0; i < MAX_MASTERS; i++)
		if (master_adr[i].port)
		{
			common->Printf("Sending heartbeat to %s\n", SOCK_AdrToString(master_adr[i]));
			NET_OutOfBandPrint(NS_SERVER, master_adr[i], "heartbeat\n%s", string);
		}
}

/*
=================
Master_Shutdown

Informs all masters that this server is going down
=================
*/
void Master_Shutdown(void)
{
	int i;

	if (!com_dedicated->value)
	{
		return;		// only dedicated servers send heartbeats

	}
	if (!public_server->value)
	{
		return;		// a private dedicated game

	}
	// send to group master
	for (i = 0; i < MAX_MASTERS; i++)
		if (master_adr[i].port)
		{
			if (i > 0)
			{
				common->Printf("Sending heartbeat to %s\n", SOCK_AdrToString(master_adr[i]));
			}
			NET_OutOfBandPrint(NS_SERVER, master_adr[i], "shutdown");
		}
}

//============================================================================

/*
===============
SV_Init

Only called at quake2.exe startup, not for each game
===============
*/
void SV_Init(void)
{
	SV_InitOperatorCommands();

	rcon_password = Cvar_Get("rcon_password", "", 0);
	Cvar_Get("skill", "1", 0);
	Cvar_Get("deathmatch", "0", CVAR_LATCH);
	Cvar_Get("coop", "0", CVAR_LATCH);
	Cvar_Get("dmflags", va("%i", DF_INSTANT_ITEMS), CVAR_SERVERINFO);
	Cvar_Get("fraglimit", "0", CVAR_SERVERINFO);
	Cvar_Get("timelimit", "0", CVAR_SERVERINFO);
	Cvar_Get("cheats", "0", CVAR_SERVERINFO | CVAR_LATCH);
	Cvar_Get("protocol", va("%i", Q2PROTOCOL_VERSION), CVAR_SERVERINFO | CVAR_INIT);;
	sv_maxclients = Cvar_Get("maxclients", "1", CVAR_SERVERINFO | CVAR_LATCH);
	sv_hostname = Cvar_Get("hostname", "noname", CVAR_SERVERINFO | CVAR_ARCHIVE);
	timeout = Cvar_Get("timeout", "125", 0);
	zombietime = Cvar_Get("zombietime", "2", 0);
	sv_showclamp = Cvar_Get("showclamp", "0", 0);
	sv_paused = Cvar_Get("paused", "0", 0);
	sv_timedemo = Cvar_Get("timedemo", "0", 0);
	svq2_enforcetime = Cvar_Get("sv_enforcetime", "0", 0);
	allow_download = Cvar_Get("allow_download", "0", CVAR_ARCHIVE);
	allow_download_players  = Cvar_Get("allow_download_players", "0", CVAR_ARCHIVE);
	allow_download_models = Cvar_Get("allow_download_models", "1", CVAR_ARCHIVE);
	allow_download_sounds = Cvar_Get("allow_download_sounds", "1", CVAR_ARCHIVE);
	allow_download_maps   = Cvar_Get("allow_download_maps", "1", CVAR_ARCHIVE);

	svq2_noreload = Cvar_Get("sv_noreload", "0", 0);

	sv_airaccelerate = Cvar_Get("sv_airaccelerate", "0", CVAR_LATCH);

	public_server = Cvar_Get("public", "0", 0);

	sv_reconnect_limit = Cvar_Get("sv_reconnect_limit", "3", CVAR_ARCHIVE);

	net_message.InitOOB(net_message_buffer, sizeof(net_message_buffer));
}

/*
==================
SV_FinalMessage

Used by SV_Shutdown to send a final message to all
connected clients before the server goes down.  The messages are sent immediately,
not just stuck on the outgoing message list, because the server is going
to totally exit after returning from this function.
==================
*/
void SV_FinalMessage(const char* message, qboolean reconnect)
{
	int i;
	client_t* cl;

	net_message.Clear();
	net_message.WriteByte(q2svc_print);
	net_message.WriteByte(PRINT_HIGH);
	net_message.WriteString2(message);

	if (reconnect)
	{
		net_message.WriteByte(q2svc_reconnect);
	}
	else
	{
		net_message.WriteByte(q2svc_disconnect);
	}

	// send it twice
	// stagger the packets to crutch operating system limited buffers

	for (i = 0, cl = svs.clients; i < sv_maxclients->value; i++, cl++)
		if (cl->state >= CS_CONNECTED)
		{
			Netchan_Transmit(&cl->netchan, net_message.cursize,
				net_message._data);
			cl->netchan.lastSent = curtime;
		}

	for (i = 0, cl = svs.clients; i < sv_maxclients->value; i++, cl++)
		if (cl->state >= CS_CONNECTED)
		{
			Netchan_Transmit(&cl->netchan, net_message.cursize,
				net_message._data);
			cl->netchan.lastSent = curtime;
		}
}



/*
================
SV_Shutdown

Called when each game quits,
before Sys_Quit or Sys_Error
================
*/
void SV_Shutdown(const char* finalmsg, qboolean reconnect)
{
	if (svs.clients)
	{
		SV_FinalMessage(finalmsg, reconnect);
	}

	Master_Shutdown();
	SVQ2_ShutdownGameProgs();

	// free current level
	if (sv.q2_demofile)
	{
		FS_FCloseFile(sv.q2_demofile);
	}
	Com_Memset(&sv, 0, sizeof(sv));
	Com_SetServerState(sv.state);

	// free server static data
	if (svs.clients)
	{
		Z_Free(svs.clients);
	}
	if (svs.q2_client_entities)
	{
		Z_Free(svs.q2_client_entities);
	}
	if (svs.q2_demofile)
	{
		FS_FCloseFile(svs.q2_demofile);
	}
	Com_Memset(&svs, 0, sizeof(svs));
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

bool CL_GetTag(int clientNum, const char* tagname, orientation_t* _or)
{
	return false;
}
void CL_MapLoading(void)
{
}
void CL_ShutdownAll(void)
{
}
