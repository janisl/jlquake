//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../server.h"
#include "local.h"

Cvar* allow_download;
Cvar* allow_download_players;
Cvar* allow_download_models;
Cvar* allow_download_sounds;
Cvar* allow_download_maps;

Cvar* svq2_enforcetime;

Cvar* svq2_noreload;				// don't reload level state when reentering

Cvar* svq2_airaccelerate;

Cvar* svq2_reconnect_limit;		// minimum seconds between connect messages

Cvar* q2rcon_password;			// password for remote server commands

Cvar* q2public_server;			// should heartbeats be sent

/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/

//	Builds the string that is sent as heartbeats and status replies
static const char* SVQ2_StatusString()
{
	static char status[MAX_MSGLEN_Q2 - 16];

	String::Cpy(status, Cvar_InfoString(CVAR_SERVERINFO, MAX_INFO_STRING_Q2, MAX_INFO_KEY_Q2,
			MAX_INFO_VALUE_Q2, true, false));
	String::Cat(status, sizeof(status), "\n");
	int statusLength = String::Length(status);

	for (int i = 0; i < sv_maxclients->value; i++)
	{
		client_t* cl = &svs.clients[i];
		if (cl->state == CS_CONNECTED || cl->state == CS_ACTIVE)
		{
			char player[1024];
			String::Sprintf(player, sizeof(player), "%i %i \"%s\"\n",
				cl->q2_edict->client->ps.stats[Q2STAT_FRAGS], cl->ping, cl->name);
			int playerLength = String::Length(player);
			if (statusLength + playerLength >= (int)sizeof(status))
			{
				// can't hold any more
				break;
			}
			String::Cpy(status + statusLength, player);
			statusLength += playerLength;
		}
	}

	return status;
}

//	Responds with all the info that qplug or qspy can see
static void SVQ2C_Status(const netadr_t& from)
{
	NET_OutOfBandPrint(NS_SERVER, from, "print\n%s", SVQ2_StatusString());
}

static void SVQ2C_Ack(const netadr_t& from)
{
	common->Printf("Ping acknowledge from %s\n", SOCK_AdrToString(from));
}

//	Responds with short info for broadcast scans
// The second parameter should be the current protocol version number.
static void SVQ2C_Info(const netadr_t& from)
{
	if (sv_maxclients->value == 1)
	{
		return;		// ignore in single player

	}
	int version = String::Atoi(Cmd_Argv(1));

	char string[64];
	if (version != Q2PROTOCOL_VERSION)
	{
		String::Sprintf(string, sizeof(string), "%s: wrong version\n", sv_hostname->string, sizeof(string));
	}
	else
	{
		int count = 0;
		for (int i = 0; i < sv_maxclients->value; i++)
		{
			if (svs.clients[i].state >= CS_CONNECTED)
			{
				count++;
			}
		}

		String::Sprintf(string, sizeof(string), "%16s %8s %2i/%2i\n", sv_hostname->string, sv.name, count, (int)sv_maxclients->value);
	}

	NET_OutOfBandPrint(NS_SERVER, from, "info\n%s", string);
}

//	Just responds with an acknowledgement
static void SVQ2C_Ping(const netadr_t& from)
{
	NET_OutOfBandPrint(NS_SERVER, from, "ack");
}

//	Returns a challenge number that can be used
// in a subsequent client_connect command.
// We do this to prevent denial of service attacks that
// flood the server with invalid connection IPs.  With a
// challenge, they must give a valid IP address.
static void SVQ2C_GetChallenge(const netadr_t& from)
{
	int oldest = 0;
	int oldestTime = 0x7fffffff;

	// see if we already have a challenge for this ip
	int i;
	for (i = 0; i < MAX_CHALLENGES; i++)
	{
		if (SOCK_CompareBaseAdr(from, svs.challenges[i].adr))
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
		svs.challenges[oldest].adr = from;
		svs.challenges[oldest].time = Sys_Milliseconds();
		i = oldest;
	}

	// send it back
	NET_OutOfBandPrint(NS_SERVER, from, "challenge %i", svs.challenges[i].challenge);
}

//	A connection request that did not come from the master
static void SVQ2C_DirectConnect(const netadr_t& from)
{
	netadr_t adr = from;

	common->DPrintf("SVC_DirectConnect ()\n");

	int version = String::Atoi(Cmd_Argv(1));
	if (version != Q2PROTOCOL_VERSION)
	{
		NET_OutOfBandPrint(NS_SERVER, adr, "print\nServer is JLQuake2 version " JLQUAKE_VERSION_STRING ".\n");
		common->DPrintf("    rejected connect from version %i\n", version);
		return;
	}

	int qport = String::Atoi(Cmd_Argv(2));

	int challenge = String::Atoi(Cmd_Argv(3));

	char userinfo[MAX_INFO_STRING_Q2];
	String::NCpy(userinfo, Cmd_Argv(4), sizeof(userinfo) - 1);
	userinfo[sizeof(userinfo) - 1] = 0;

	// force the IP key/value pair so the game can filter based on ip
	Info_SetValueForKey(userinfo, "ip", SOCK_AdrToString(from), MAX_INFO_STRING_Q2, MAX_INFO_KEY_Q2,
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
		int i;
		for (i = 0; i < MAX_CHALLENGES; i++)
		{
			if (SOCK_CompareBaseAdr(from, svs.challenges[i].adr))
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

	client_t temp;
	client_t* newcl = &temp;
	Com_Memset(newcl, 0, sizeof(client_t));

	// if there is already a slot for this ip, reuse it
	client_t* cl = svs.clients;
	for (int i = 0; i < sv_maxclients->value; i++,cl++)
	{
		if (cl->state == CS_FREE)
		{
			continue;
		}
		if (SOCK_CompareBaseAdr(adr, cl->netchan.remoteAddress) &&
			(cl->netchan.qport == qport ||
			 adr.port == cl->netchan.remoteAddress.port))
		{
			if (!SOCK_IsLocalAddress(adr) && (svs.realtime - cl->q2_lastconnect) < (svq2_reconnect_limit->integer * 1000))
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
	cl = svs.clients;
	for (int i = 0; i < sv_maxclients->value; i++,cl++)
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
	int edictnum = (newcl - svs.clients) + 1;
	q2edict_t* ent = Q2_EDICT_NUM(edictnum);
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
	newcl->netchan.lastReceived = Sys_Milliseconds();

	newcl->state = CS_CONNECTED;

	newcl->datagram.InitOOB(newcl->datagramBuffer, MAX_MSGLEN_Q2);
	newcl->datagram.allowoverflow = true;
	newcl->q2_lastmessage = svs.realtime;	// don't timeout
	newcl->q2_lastconnect = svs.realtime;
}

static bool SVQ2_Rcon_Validate()
{
	if (!String::Length(q2rcon_password->string))
	{
		return false;
	}

	if (String::Cmp(Cmd_Argv(1), q2rcon_password->string))
	{
		return false;
	}

	return true;
}

static void SVQ2_FlushRedirect(char* outputbuf)
{
	NET_OutOfBandPrint(NS_SERVER, svs.redirectAddress, "print\n%s", outputbuf);
}

//	A client issued an rcon command.
// Shift down the remaining args
// Redirect all printfs
static void SVQ2C_RemoteCommand(const netadr_t& from, QMsg& message)
{
	svs.redirectAddress = from;
	int i = SVQ2_Rcon_Validate();

	if (i == 0)
	{
		common->Printf("Bad rcon from %s:\n%s\n", SOCK_AdrToString(from), message._data + 4);
	}
	else
	{
		common->Printf("Rcon from %s:\n%s\n", SOCK_AdrToString(from), message._data + 4);
	}

	enum { SV_OUTPUTBUF_LENGTH = (MAX_MSGLEN_Q2 - 16) };
	char sv_outputbuf[SV_OUTPUTBUF_LENGTH];
	Com_BeginRedirect(sv_outputbuf, SV_OUTPUTBUF_LENGTH, SVQ2_FlushRedirect);

	if (!SVQ2_Rcon_Validate())
	{
		common->Printf("Bad rcon_password.\n");
	}
	else
	{
		char remaining[1024];
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

//	A connectionless packet has four leading 0xff
// characters to distinguish it from a game channel.
// Clients that are in the game can still send
// connectionless packets.
void SVQ2_ConnectionlessPacket(const netadr_t& from, QMsg& message)
{
	message.BeginReadingOOB();
	message.ReadLong();		// skip the -1 marker

	const char* s = message.ReadStringLine2();

	Cmd_TokenizeString(s, false);

	const char* c = Cmd_Argv(0);
	common->DPrintf("Packet %s : %s\n", SOCK_AdrToString(from), c);

	if (!String::Cmp(c, "ping"))
	{
		SVQ2C_Ping(from);
	}
	else if (!String::Cmp(c, "ack"))
	{
		SVQ2C_Ack(from);
	}
	else if (!String::Cmp(c,"status"))
	{
		SVQ2C_Status(from);
	}
	else if (!String::Cmp(c,"info"))
	{
		SVQ2C_Info(from);
	}
	else if (!String::Cmp(c,"getchallenge"))
	{
		SVQ2C_GetChallenge(from);
	}
	else if (!String::Cmp(c,"connect"))
	{
		SVQ2C_DirectConnect(from);
	}
	else if (!String::Cmp(c, "rcon"))
	{
		SVQ2C_RemoteCommand(from, message);
	}
	else
	{
		common->Printf("bad connectionless packet from %s:\n%s\n",
			SOCK_AdrToString(from), s);
	}
}

//	Send a message to the master every few minutes to
// let it know we are alive, and log information
#define HEARTBEAT_SECONDS   300
void SVQ2_MasterHeartbeat()
{
	if (!com_dedicated->value)
	{
		return;		// only dedicated servers send heartbeats

	}
	if (!q2public_server->value)
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
	const char* string = SVQ2_StatusString();

	// send to group master
	for (int i = 0; i < MAX_MASTERS; i++)
	{
		if (master_adr[i].port)
		{
			common->Printf("Sending heartbeat to %s\n", SOCK_AdrToString(master_adr[i]));
			NET_OutOfBandPrint(NS_SERVER, master_adr[i], "heartbeat\n%s", string);
		}
	}
}

//	Informs all masters that this server is going down
void SVQ2_MasterShutdown()
{
	if (!com_dedicated->value)
	{
		return;		// only dedicated servers send heartbeats

	}
	if (!q2public_server->value)
	{
		return;		// a private dedicated game

	}
	// send to group master
	for (int i = 0; i < MAX_MASTERS; i++)
	{
		if (master_adr[i].port)
		{
			if (i > 0)
			{
				common->Printf("Sending heartbeat to %s\n", SOCK_AdrToString(master_adr[i]));
			}
			NET_OutOfBandPrint(NS_SERVER, master_adr[i], "shutdown");
		}
	}
}

//	Used by SV_Shutdown to send a final message to all
// connected clients before the server goes down.  The messages are sent immediately,
// not just stuck on the outgoing message list, because the server is going
// to totally exit after returning from this function.
static void SVQ2_FinalMessage(const char* message, bool reconnect)
{
	QMsg net_message;
	byte net_message_buf[MAX_MSGLEN_Q2];
	net_message.InitOOB(net_message_buf, MAX_MSGLEN_Q2);
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

	client_t* cl = svs.clients;
	for (int i = 0; i < sv_maxclients->value; i++, cl++)
	{
		if (cl->state >= CS_CONNECTED)
		{
			Netchan_Transmit(&cl->netchan, net_message.cursize, net_message._data);
			cl->netchan.lastSent = Sys_Milliseconds();
		}
	}

	cl = svs.clients;
	for (int i = 0; i < sv_maxclients->value; i++, cl++)
	{
		if (cl->state >= CS_CONNECTED)
		{
			Netchan_Transmit(&cl->netchan, net_message.cursize, net_message._data);
			cl->netchan.lastSent = Sys_Milliseconds();
		}
	}
}

//	Called when each game quits,
// before Sys_Quit or Sys_Error
void SVQ2_Shutdown(const char* finalmsg, bool reconnect)
{
	if (svs.clients)
	{
		SVQ2_FinalMessage(finalmsg, reconnect);
	}

	SVQ2_MasterShutdown();
	SVQ2_ShutdownGameProgs();

	// free current level
	if (sv.q2_demofile)
	{
		FS_FCloseFile(sv.q2_demofile);
	}
	Com_Memset(&sv, 0, sizeof(sv));
	ComQ2_SetServerState(sv.state);

	// free server static data
	if (svs.clients)
	{
		Mem_Free(svs.clients);
	}
	if (svs.q2_client_entities)
	{
		Mem_Free(svs.q2_client_entities);
	}
	if (svs.q2_demofile)
	{
		FS_FCloseFile(svs.q2_demofile);
	}
	Com_Memset(&svs, 0, sizeof(svs));
}
