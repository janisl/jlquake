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

static void SV_CloseDownload(client_t* cl);

/*
=================
SV_GetChallenge

A "getchallenge" OOB command has been received
Returns a challenge number that can be used
in a subsequent connectResponse command.
We do this to prevent denial of service attacks that
flood the server with invalid connection IPs.  With a
challenge, they must give a valid IP address.

If we are authorizing, a challenge request will cause a packet
to be sent to the authorize server.

When an authorizeip is returned, a challenge response will be
sent to that ip.
=================
*/
void SV_GetChallenge(netadr_t from)
{
	int i;
	int oldest;
	int oldestTime;
	challenge_t* challenge;

	// ignore if we are in single player
	if (Cvar_VariableValue("g_gametype") == Q3GT_SINGLE_PLAYER || Cvar_VariableValue("ui_singlePlayerActive"))
	{
		return;
	}

	oldest = 0;
	oldestTime = 0x7fffffff;

	// see if we already have a challenge for this ip
	challenge = &svs.challenges[0];
	for (i = 0; i < MAX_CHALLENGES; i++, challenge++)
	{
		if (!challenge->connected && SOCK_CompareAdr(from, challenge->adr))
		{
			break;
		}
		if (challenge->time < oldestTime)
		{
			oldestTime = challenge->time;
			oldest = i;
		}
	}

	if (i == MAX_CHALLENGES)
	{
		// this is the first time this client has asked for a challenge
		challenge = &svs.challenges[oldest];

		challenge->challenge = ((rand() << 16) ^ rand()) ^ svs.q3_time;
		challenge->adr = from;
		challenge->firstTime = svs.q3_time;
		challenge->time = svs.q3_time;
		challenge->connected = qfalse;
		i = oldest;
	}

	// if they are on a lan address, send the challengeResponse immediately
	if (SOCK_IsLANAddress(from))
	{
		challenge->pingTime = svs.q3_time;
		NET_OutOfBandPrint(NS_SERVER, from, "challengeResponse %i", challenge->challenge);
		return;
	}

	// look up the authorize server's IP
	if (!svs.q3_authorizeAddress.ip[0] && svs.q3_authorizeAddress.type != NA_BAD)
	{
		Com_Printf("Resolving %s\n", AUTHORIZE_SERVER_NAME);
		if (!SOCK_StringToAdr(AUTHORIZE_SERVER_NAME, &svs.q3_authorizeAddress, PORT_AUTHORIZE))
		{
			Com_Printf("Couldn't resolve address\n");
			return;
		}
		Com_Printf("%s resolved to %s\n", AUTHORIZE_SERVER_NAME, SOCK_AdrToString(svs.q3_authorizeAddress));
	}

	// if they have been challenging for a long time and we
	// haven't heard anything from the authorize server, go ahead and
	// let them in, assuming the id server is down
	if (svs.q3_time - challenge->firstTime > AUTHORIZE_TIMEOUT)
	{
		Com_DPrintf("authorize server timed out\n");

		challenge->pingTime = svs.q3_time;
		NET_OutOfBandPrint(NS_SERVER, challenge->adr,
			"challengeResponse %i", challenge->challenge);
		return;
	}

	// otherwise send their ip to the authorize server
	if (svs.q3_authorizeAddress.type != NA_BAD)
	{
		Cvar* fs;
		char game[1024];

		Com_DPrintf("sending getIpAuthorize for %s\n", SOCK_AdrToString(from));

		String::Cpy(game, BASEGAME);
		fs = Cvar_Get("fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO);
		if (fs && fs->string[0] != 0)
		{
			String::Cpy(game, fs->string);
		}

		// the 0 is for backwards compatibility with obsolete sv_allowanonymous flags
		// getIpAuthorize <challenge> <IP> <game> 0 <auth-flag>
		NET_OutOfBandPrint(NS_SERVER, svs.q3_authorizeAddress,
			"getIpAuthorize %i %s %s 0 %s",  svs.challenges[i].challenge,
			SOCK_BaseAdrToString(from), game, sv_strictAuth->string);
	}
}

/*
====================
SV_AuthorizeIpPacket

A packet has been returned from the authorize server.
If we have a challenge adr for that ip, send the
challengeResponse to it
====================
*/
void SV_AuthorizeIpPacket(netadr_t from)
{
	int challenge;
	int i;
	char* s;
	char* r;
	char ret[1024];

	if (!SOCK_CompareBaseAdr(from, svs.q3_authorizeAddress))
	{
		Com_Printf("SV_AuthorizeIpPacket: not from authorize server\n");
		return;
	}

	challenge = String::Atoi(Cmd_Argv(1));

	for (i = 0; i < MAX_CHALLENGES; i++)
	{
		if (svs.challenges[i].challenge == challenge)
		{
			break;
		}
	}
	if (i == MAX_CHALLENGES)
	{
		Com_Printf("SV_AuthorizeIpPacket: challenge not found\n");
		return;
	}

	// send a packet back to the original client
	svs.challenges[i].pingTime = svs.q3_time;
	s = Cmd_Argv(2);
	r = Cmd_Argv(3);			// reason

	if (!String::ICmp(s, "demo"))
	{
		// they are a demo client trying to connect to a real server
		NET_OutOfBandPrint(NS_SERVER, svs.challenges[i].adr, "print\nServer is not a demo server\n");
		// clear the challenge record so it won't timeout and let them through
		Com_Memset(&svs.challenges[i], 0, sizeof(svs.challenges[i]));
		return;
	}
	if (!String::ICmp(s, "accept"))
	{
		NET_OutOfBandPrint(NS_SERVER, svs.challenges[i].adr,
			"challengeResponse %i", svs.challenges[i].challenge);
		return;
	}
	if (!String::ICmp(s, "unknown"))
	{
		if (!r)
		{
			NET_OutOfBandPrint(NS_SERVER, svs.challenges[i].adr, "print\nAwaiting CD key authorization\n");
		}
		else
		{
			sprintf(ret, "print\n%s\n", r);
			NET_OutOfBandPrint(NS_SERVER, svs.challenges[i].adr, ret);
		}
		// clear the challenge record so it won't timeout and let them through
		Com_Memset(&svs.challenges[i], 0, sizeof(svs.challenges[i]));
		return;
	}

	// authorization failed
	if (!r)
	{
		NET_OutOfBandPrint(NS_SERVER, svs.challenges[i].adr, "print\nSomeone is using this CD Key\n");
	}
	else
	{
		sprintf(ret, "print\n%s\n", r);
		NET_OutOfBandPrint(NS_SERVER, svs.challenges[i].adr, ret);
	}

	// clear the challenge record so it won't timeout and let them through
	Com_Memset(&svs.challenges[i], 0, sizeof(svs.challenges[i]));
}

/*
==================
SV_DirectConnect

A "connect" OOB command has been received
==================
*/

#define PB_MESSAGE "PunkBuster Anti-Cheat software must be installed " \
				   "and Enabled in order to join this server. An updated game patch can be downloaded from " \
				   "www.idsoftware.com"

void SV_DirectConnect(netadr_t from)
{
	char userinfo[MAX_INFO_STRING_Q3];
	int i;
	client_t* cl, * newcl;
	MAC_STATIC client_t temp;
	q3sharedEntity_t* ent;
	int clientNum;
	int version;
	int qport;
	int challenge;
	const char* password;
	int startIndex;
	int count;

	Com_DPrintf("SVC_DirectConnect ()\n");

	String::NCpyZ(userinfo, Cmd_Argv(1), sizeof(userinfo));

	version = String::Atoi(Info_ValueForKey(userinfo, "protocol"));
	if (version != PROTOCOL_VERSION)
	{
		NET_OutOfBandPrint(NS_SERVER, from, "print\nServer uses protocol version %i.\n", PROTOCOL_VERSION);
		Com_DPrintf("    rejected connect from version %i\n", version);
		return;
	}

	challenge = String::Atoi(Info_ValueForKey(userinfo, "challenge"));
	qport = String::Atoi(Info_ValueForKey(userinfo, "qport"));

	// quick reject
	for (i = 0,cl = svs.clients; i < sv_maxclients->integer; i++,cl++)
	{
		if (cl->state == CS_FREE)
		{
			continue;
		}
		if (SOCK_CompareBaseAdr(from, cl->netchan.remoteAddress) &&
			(cl->netchan.qport == qport ||
			 from.port == cl->netchan.remoteAddress.port))
		{
			if ((svs.q3_time - cl->q3_lastConnectTime)
				< (sv_reconnectlimit->integer * 1000))
			{
				Com_DPrintf("%s:reconnect rejected : too soon\n", SOCK_AdrToString(from));
				return;
			}
			break;
		}
	}

	// see if the challenge is valid (LAN clients don't need to challenge)
	if (!SOCK_IsLocalAddress(from))
	{
		int ping;

		for (i = 0; i < MAX_CHALLENGES; i++)
		{
			if (SOCK_CompareAdr(from, svs.challenges[i].adr))
			{
				if (challenge == svs.challenges[i].challenge)
				{
					break;		// good
				}
			}
		}
		if (i == MAX_CHALLENGES)
		{
			NET_OutOfBandPrint(NS_SERVER, from, "print\nNo or bad challenge for address.\n");
			return;
		}
		// force the IP key/value pair so the game can filter based on ip
		Info_SetValueForKey(userinfo, "ip", SOCK_AdrToString(from), MAX_INFO_STRING_Q3);

		ping = svs.q3_time - svs.challenges[i].pingTime;
		Com_Printf("Client %i connecting with %i challenge ping\n", i, ping);
		svs.challenges[i].connected = qtrue;

		// never reject a LAN client based on ping
		if (!SOCK_IsLANAddress(from))
		{
			if (sv_minPing->value && ping < sv_minPing->value)
			{
				// don't let them keep trying until they get a big delay
				NET_OutOfBandPrint(NS_SERVER, from, "print\nServer is for high pings only\n");
				Com_DPrintf("Client %i rejected on a too low ping\n", i);
				// reset the address otherwise their ping will keep increasing
				// with each connect message and they'd eventually be able to connect
				svs.challenges[i].adr.port = 0;
				return;
			}
			if (sv_maxPing->value && ping > sv_maxPing->value)
			{
				NET_OutOfBandPrint(NS_SERVER, from, "print\nServer is for low pings only\n");
				Com_DPrintf("Client %i rejected on a too high ping\n", i);
				return;
			}
		}
	}
	else
	{
		// force the "ip" info key to "localhost"
		Info_SetValueForKey(userinfo, "ip", "localhost", MAX_INFO_STRING_Q3);
	}

	newcl = &temp;
	Com_Memset(newcl, 0, sizeof(client_t));

	// if there is already a slot for this ip, reuse it
	for (i = 0,cl = svs.clients; i < sv_maxclients->integer; i++,cl++)
	{
		if (cl->state == CS_FREE)
		{
			continue;
		}
		if (SOCK_CompareBaseAdr(from, cl->netchan.remoteAddress) &&
			(cl->netchan.qport == qport ||
			 from.port == cl->netchan.remoteAddress.port))
		{
			Com_Printf("%s:reconnect\n", SOCK_AdrToString(from));
			newcl = cl;

			// this doesn't work because it nukes the players userinfo

//			// disconnect the client from the game first so any flags the
//			// player might have are dropped
//			VM_Call( gvm, GAME_CLIENT_DISCONNECT, newcl - svs.clients );
			//
			goto gotnewcl;
		}
	}

	// find a client slot
	// if "sv_privateClients" is set > 0, then that number
	// of client slots will be reserved for connections that
	// have "password" set to the value of "sv_privatePassword"
	// Info requests will report the maxclients as if the private
	// slots didn't exist, to prevent people from trying to connect
	// to a full server.
	// This is to allow us to reserve a couple slots here on our
	// servers so we can play without having to kick people.

	// check for privateClient password
	password = Info_ValueForKey(userinfo, "password");
	if (!String::Cmp(password, sv_privatePassword->string))
	{
		startIndex = 0;
	}
	else
	{
		// skip past the reserved slots
		startIndex = sv_privateClients->integer;
	}

	newcl = NULL;
	for (i = startIndex; i < sv_maxclients->integer; i++)
	{
		cl = &svs.clients[i];
		if (cl->state == CS_FREE)
		{
			newcl = cl;
			break;
		}
	}

	if (!newcl)
	{
		if (SOCK_IsLocalAddress(from))
		{
			count = 0;
			for (i = startIndex; i < sv_maxclients->integer; i++)
			{
				cl = &svs.clients[i];
				if (cl->netchan.remoteAddress.type == NA_BOT)
				{
					count++;
				}
			}
			// if they're all bots
			if (count >= sv_maxclients->integer - startIndex)
			{
				SV_DropClient(&svs.clients[sv_maxclients->integer - 1], "only bots on server");
				newcl = &svs.clients[sv_maxclients->integer - 1];
			}
			else
			{
				Com_Error(ERR_FATAL, "server is full on local connect\n");
				return;
			}
		}
		else
		{
			NET_OutOfBandPrint(NS_SERVER, from, "print\nServer is full.\n");
			Com_DPrintf("Rejected a connection.\n");
			return;
		}
	}

	// we got a newcl, so reset the reliableSequence and reliableAcknowledge
	cl->q3_reliableAcknowledge = 0;
	cl->q3_reliableSequence = 0;

gotnewcl:
	// build a new connection
	// accept the new client
	// this is the only place a client_t is ever initialized
	*newcl = temp;
	clientNum = newcl - svs.clients;
	ent = SVQ3_GentityNum(clientNum);
	newcl->q3_gentity = ent;

	// save the challenge
	newcl->challenge = challenge;

	// save the address
	Netchan_Setup(NS_SERVER, &newcl->netchan, from, qport);
	// init the netchan queue
	newcl->q3_netchan_end_queue = &newcl->q3_netchan_start_queue;

	// save the userinfo
	String::NCpyZ(newcl->userinfo, userinfo, MAX_INFO_STRING_Q3);

	// get the game a chance to reject this connection or modify the userinfo
	qintptr denied = VM_Call(gvm, GAME_CLIENT_CONNECT, clientNum, qtrue, qfalse);	// firstTime = qtrue
	if (denied)
	{
		// we can't just use VM_ArgPtr, because that is only valid inside a VM_Call
		char* denied_str = (char*)VM_ExplicitArgPtr(gvm, denied);

		NET_OutOfBandPrint(NS_SERVER, from, "print\n%s\n", denied_str);
		Com_DPrintf("Game rejected a connection: %s.\n", denied_str);
		return;
	}

	SV_UserinfoChanged(newcl);

	// send the connect packet to the client
	NET_OutOfBandPrint(NS_SERVER, from, "connectResponse");

	Com_DPrintf("Going from CS_FREE to CS_CONNECTED for %s\n", newcl->name);

	newcl->state = CS_CONNECTED;
	newcl->q3_nextSnapshotTime = svs.q3_time;
	newcl->q3_lastPacketTime = svs.q3_time;
	newcl->q3_lastConnectTime = svs.q3_time;

	// when we receive the first packet from the client, we will
	// notice that it is from a different serverid and that the
	// gamestate message was not just sent, forcing a retransmit
	newcl->q3_gamestateMessageNum = -1;

	// if this was the first client on the server, or the last client
	// the server can hold, send a heartbeat to the master.
	count = 0;
	for (i = 0,cl = svs.clients; i < sv_maxclients->integer; i++,cl++)
	{
		if (svs.clients[i].state >= CS_CONNECTED)
		{
			count++;
		}
	}
	if (count == 1 || count == sv_maxclients->integer)
	{
		SV_Heartbeat_f();
	}
}


/*
=====================
SV_DropClient

Called when the player is totally leaving the server, either willingly
or unwillingly.  This is NOT called if the entire server is quiting
or crashing -- SV_FinalMessage() will handle that
=====================
*/
void SV_DropClient(client_t* drop, const char* reason)
{
	int i;
	challenge_t* challenge;

	if (drop->state == CS_ZOMBIE)
	{
		return;		// already dropped
	}

	if (!drop->q3_gentity || !(drop->q3_gentity->r.svFlags & Q3SVF_BOT))
	{
		// see if we already have a challenge for this ip
		challenge = &svs.challenges[0];

		for (i = 0; i < MAX_CHALLENGES; i++, challenge++)
		{
			if (SOCK_CompareAdr(drop->netchan.remoteAddress, challenge->adr))
			{
				challenge->connected = qfalse;
				break;
			}
		}
	}

	// Kill any download
	SV_CloseDownload(drop);

	// tell everyone why they got dropped
	SV_SendServerCommand(NULL, "print \"%s" S_COLOR_WHITE " %s\n\"", drop->name, reason);

	Com_DPrintf("Going to CS_ZOMBIE for %s\n", drop->name);
	drop->state = CS_ZOMBIE;		// become free in a few seconds

	if (drop->download)
	{
		FS_FCloseFile(drop->download);
		drop->download = 0;
	}

	// call the prog function for removing a client
	// this will remove the body, among other things
	VM_Call(gvm, GAME_CLIENT_DISCONNECT, drop - svs.clients);

	// add the disconnect command
	SV_SendServerCommand(drop, "disconnect \"%s\"", reason);

	if (drop->netchan.remoteAddress.type == NA_BOT)
	{
		SV_BotFreeClient(drop - svs.clients);
	}

	// nuke user info
	SV_SetUserinfo(drop - svs.clients, "");

	// if this was the last client on the server, send a heartbeat
	// to the master so it is known the server is empty
	// send a heartbeat now so the master will get up to date info
	// if there is already a slot for this ip, reuse it
	for (i = 0; i < sv_maxclients->integer; i++)
	{
		if (svs.clients[i].state >= CS_CONNECTED)
		{
			break;
		}
	}
	if (i == sv_maxclients->integer)
	{
		SV_Heartbeat_f();
	}
}

/*
================
SV_SendClientGameState

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each new map load.

It will be resent if the client acknowledges a later message but has
the wrong gamestate.
================
*/
void SV_SendClientGameState(client_t* client)
{
	int start;
	q3entityState_t* base, nullstate;
	QMsg msg;
	byte msgBuffer[MAX_MSGLEN_Q3];

	Com_DPrintf("SV_SendClientGameState() for %s\n", client->name);
	Com_DPrintf("Going from CS_CONNECTED to CS_PRIMED for %s\n", client->name);
	client->state = CS_PRIMED;
	client->q3_pureAuthentic = 0;
	client->q3_gotCP = qfalse;

	// when we receive the first packet from the client, we will
	// notice that it is from a different serverid and that the
	// gamestate message was not just sent, forcing a retransmit
	client->q3_gamestateMessageNum = client->netchan.outgoingSequence;

	msg.Init(msgBuffer, sizeof(msgBuffer));

	// NOTE, MRE: all server->client messages now acknowledge
	// let the client know which reliable clientCommands we have received
	msg.WriteLong(client->q3_lastClientCommand);

	// send any server commands waiting to be sent first.
	// we have to do this cause we send the client->reliableSequence
	// with a gamestate and it sets the clc.serverCommandSequence at
	// the client side
	SV_UpdateServerCommandsToClient(client, &msg);

	// send the gamestate
	msg.WriteByte(q3svc_gamestate);
	msg.WriteLong(client->q3_reliableSequence);

	// write the configstrings
	for (start = 0; start < MAX_CONFIGSTRINGS_Q3; start++)
	{
		if (sv.q3_configstrings[start][0])
		{
			msg.WriteByte(q3svc_configstring);
			msg.WriteShort(start);
			msg.WriteBigString(sv.q3_configstrings[start]);
		}
	}

	// write the baselines
	Com_Memset(&nullstate, 0, sizeof(nullstate));
	for (start = 0; start < MAX_GENTITIES_Q3; start++)
	{
		base = &sv.q3_svEntities[start].q3_baseline;
		if (!base->number)
		{
			continue;
		}
		msg.WriteByte(q3svc_baseline);
		MSGQ3_WriteDeltaEntity(&msg, &nullstate, base, qtrue);
	}

	msg.WriteByte(q3svc_EOF);

	msg.WriteLong(client - svs.clients);

	// write the checksum feed
	msg.WriteLong(sv.q3_checksumFeed);

	// deliver this to the client
	SV_SendMessageToClient(&msg, client);
}


/*
==================
SV_ClientEnterWorld
==================
*/
void SV_ClientEnterWorld(client_t* client, q3usercmd_t* cmd)
{
	int clientNum;
	q3sharedEntity_t* ent;

	Com_DPrintf("Going from CS_PRIMED to CS_ACTIVE for %s\n", client->name);
	client->state = CS_ACTIVE;

	// set up the entity for the client
	clientNum = client - svs.clients;
	ent = SVQ3_GentityNum(clientNum);
	ent->s.number = clientNum;
	client->q3_gentity = ent;

	client->q3_deltaMessage = -1;
	client->q3_nextSnapshotTime = svs.q3_time;	// generate a snapshot immediately
	client->q3_lastUsercmd = *cmd;

	// call the game begin function
	VM_Call(gvm, GAME_CLIENT_BEGIN, client - svs.clients);
}

/*
============================================================

CLIENT COMMAND EXECUTION

============================================================
*/

/*
==================
SV_CloseDownload

clear/free any download vars
==================
*/
static void SV_CloseDownload(client_t* cl)
{
	int i;

	// EOF
	if (cl->download)
	{
		FS_FCloseFile(cl->download);
	}
	cl->download = 0;
	*cl->downloadName = 0;

	// Free the temporary buffer space
	for (i = 0; i < MAX_DOWNLOAD_WINDOW; i++)
	{
		if (cl->downloadBlocks[i])
		{
			Z_Free(cl->downloadBlocks[i]);
			cl->downloadBlocks[i] = NULL;
		}
	}

}

/*
==================
SV_StopDownload_f

Abort a download if in progress
==================
*/
void SV_StopDownload_f(client_t* cl)
{
	if (*cl->downloadName)
	{
		Com_DPrintf("clientDownload: %d : file \"%s\" aborted\n", cl - svs.clients, cl->downloadName);
	}

	SV_CloseDownload(cl);
}

/*
==================
SV_DoneDownload_f

Downloads are finished
==================
*/
void SV_DoneDownload_f(client_t* cl)
{
	Com_DPrintf("clientDownload: %s Done\n", cl->name);
	// resend the game state to update any clients that entered during the download
	SV_SendClientGameState(cl);
}

/*
==================
SV_NextDownload_f

The argument will be the last acknowledged block from the client, it should be
the same as cl->downloadClientBlock
==================
*/
void SV_NextDownload_f(client_t* cl)
{
	int block = String::Atoi(Cmd_Argv(1));

	if (block == cl->downloadClientBlock)
	{
		Com_DPrintf("clientDownload: %d : client acknowledge of block %d\n", cl - svs.clients, block);

		// Find out if we are done.  A zero-length block indicates EOF
		if (cl->downloadBlockSize[cl->downloadClientBlock % MAX_DOWNLOAD_WINDOW] == 0)
		{
			Com_Printf("clientDownload: %d : file \"%s\" completed\n", cl - svs.clients, cl->downloadName);
			SV_CloseDownload(cl);
			return;
		}

		cl->downloadSendTime = svs.q3_time;
		cl->downloadClientBlock++;
		return;
	}
	// We aren't getting an acknowledge for the correct block, drop the client
	// FIXME: this is bad... the client will never parse the disconnect message
	//			because the cgame isn't loaded yet
	SV_DropClient(cl, "broken download");
}

/*
==================
SV_BeginDownload_f
==================
*/
void SV_BeginDownload_f(client_t* cl)
{

	// Kill any existing download
	SV_CloseDownload(cl);

	// cl->downloadName is non-zero now, SV_WriteDownloadToClient will see this and open
	// the file itself
	String::NCpyZ(cl->downloadName, Cmd_Argv(1), sizeof(cl->downloadName));
}

/*
==================
SV_WriteDownloadToClient

Check to see if the client wants a file, open it if needed and start pumping the client
Fill up msg with data
==================
*/
void SV_WriteDownloadToClient(client_t* cl, QMsg* msg)
{
	int curindex;
	int rate;
	int blockspersnap;
	int idPack, missionPack;
	char errorMessage[1024];

	if (!*cl->downloadName)
	{
		return;	// Nothing being downloaded

	}
	if (!cl->download)
	{
		// We open the file here

		Com_Printf("clientDownload: %d : begining \"%s\"\n", cl - svs.clients, cl->downloadName);

		missionPack = FS_idPak(cl->downloadName, "missionpack");
		idPack = missionPack || FS_idPak(cl->downloadName, "baseq3");

		if (!sv_allowDownload->integer || idPack ||
			(cl->downloadSize = FS_SV_FOpenFileRead(cl->downloadName, &cl->download)) <= 0)
		{
			// cannot auto-download file
			if (idPack)
			{
				Com_Printf("clientDownload: %d : \"%s\" cannot download id pk3 files\n", cl - svs.clients, cl->downloadName);
				if (missionPack)
				{
					String::Sprintf(errorMessage, sizeof(errorMessage), "Cannot autodownload Team Arena file \"%s\"\n"
																		"The Team Arena mission pack can be found in your local game store.", cl->downloadName);
				}
				else
				{
					String::Sprintf(errorMessage, sizeof(errorMessage), "Cannot autodownload id pk3 file \"%s\"", cl->downloadName);
				}
			}
			else if (!sv_allowDownload->integer)
			{
				Com_Printf("clientDownload: %d : \"%s\" download disabled", cl - svs.clients, cl->downloadName);
				if (sv_pure->integer)
				{
					String::Sprintf(errorMessage, sizeof(errorMessage), "Could not download \"%s\" because autodownloading is disabled on the server.\n\n"
																		"You will need to get this file elsewhere before you "
																		"can connect to this pure server.\n", cl->downloadName);
				}
				else
				{
					String::Sprintf(errorMessage, sizeof(errorMessage), "Could not download \"%s\" because autodownloading is disabled on the server.\n\n"
																		"The server you are connecting to is not a pure server, "
																		"set autodownload to No in your settings and you might be "
																		"able to join the game anyway.\n", cl->downloadName);
				}
			}
			else
			{
				// NOTE TTimo this is NOT supposed to happen unless bug in our filesystem scheme?
				//   if the pk3 is referenced, it must have been found somewhere in the filesystem
				Com_Printf("clientDownload: %d : \"%s\" file not found on server\n", cl - svs.clients, cl->downloadName);
				String::Sprintf(errorMessage, sizeof(errorMessage), "File \"%s\" not found on server for autodownloading.\n", cl->downloadName);
			}
			msg->WriteByte(q3svc_download);
			msg->WriteShort(0);	// client is expecting block zero
			msg->WriteLong(-1);	// illegal file size
			msg->WriteString(errorMessage);

			*cl->downloadName = 0;
			return;
		}

		// Init
		cl->downloadCurrentBlock = cl->downloadClientBlock = cl->downloadXmitBlock = 0;
		cl->downloadCount = 0;
		cl->downloadEOF = qfalse;
	}

	// Perform any reads that we need to
	while (cl->downloadCurrentBlock - cl->downloadClientBlock < MAX_DOWNLOAD_WINDOW &&
		   cl->downloadSize != cl->downloadCount)
	{

		curindex = (cl->downloadCurrentBlock % MAX_DOWNLOAD_WINDOW);

		if (!cl->downloadBlocks[curindex])
		{
			cl->downloadBlocks[curindex] = (byte*)Z_Malloc(MAX_DOWNLOAD_BLKSIZE);
		}

		cl->downloadBlockSize[curindex] = FS_Read(cl->downloadBlocks[curindex], MAX_DOWNLOAD_BLKSIZE, cl->download);

		if (cl->downloadBlockSize[curindex] < 0)
		{
			// EOF right now
			cl->downloadCount = cl->downloadSize;
			break;
		}

		cl->downloadCount += cl->downloadBlockSize[curindex];

		// Load in next block
		cl->downloadCurrentBlock++;
	}

	// Check to see if we have eof condition and add the EOF block
	if (cl->downloadCount == cl->downloadSize &&
		!cl->downloadEOF &&
		cl->downloadCurrentBlock - cl->downloadClientBlock < MAX_DOWNLOAD_WINDOW)
	{

		cl->downloadBlockSize[cl->downloadCurrentBlock % MAX_DOWNLOAD_WINDOW] = 0;
		cl->downloadCurrentBlock++;

		cl->downloadEOF = qtrue;	// We have added the EOF block
	}

	// Loop up to window size times based on how many blocks we can fit in the
	// client snapMsec and rate

	// based on the rate, how many bytes can we fit in the snapMsec time of the client
	// normal rate / snapshotMsec calculation
	rate = cl->rate;
	if (sv_maxRate->integer)
	{
		if (sv_maxRate->integer < 1000)
		{
			Cvar_Set("sv_MaxRate", "1000");
		}
		if (sv_maxRate->integer < rate)
		{
			rate = sv_maxRate->integer;
		}
	}

	if (!rate)
	{
		blockspersnap = 1;
	}
	else
	{
		blockspersnap = ((rate * cl->q3_snapshotMsec) / 1000 + MAX_DOWNLOAD_BLKSIZE) /
						MAX_DOWNLOAD_BLKSIZE;
	}

	if (blockspersnap < 0)
	{
		blockspersnap = 1;
	}

	while (blockspersnap--)
	{

		// Write out the next section of the file, if we have already reached our window,
		// automatically start retransmitting

		if (cl->downloadClientBlock == cl->downloadCurrentBlock)
		{
			return;	// Nothing to transmit

		}
		if (cl->downloadXmitBlock == cl->downloadCurrentBlock)
		{
			// We have transmitted the complete window, should we start resending?

			//FIXME:  This uses a hardcoded one second timeout for lost blocks
			//the timeout should be based on client rate somehow
			if (svs.q3_time - cl->downloadSendTime > 1000)
			{
				cl->downloadXmitBlock = cl->downloadClientBlock;
			}
			else
			{
				return;
			}
		}

		// Send current block
		curindex = (cl->downloadXmitBlock % MAX_DOWNLOAD_WINDOW);

		msg->WriteByte(q3svc_download);
		msg->WriteShort(cl->downloadXmitBlock);

		// block zero is special, contains file size
		if (cl->downloadXmitBlock == 0)
		{
			msg->WriteLong(cl->downloadSize);
		}

		msg->WriteShort(cl->downloadBlockSize[curindex]);

		// Write the block
		if (cl->downloadBlockSize[curindex])
		{
			msg->WriteData(cl->downloadBlocks[curindex], cl->downloadBlockSize[curindex]);
		}

		Com_DPrintf("clientDownload: %d : writing block %d\n", cl - svs.clients, cl->downloadXmitBlock);

		// Move on to the next block
		// It will get sent with next snap shot.  The rate will keep us in line.
		cl->downloadXmitBlock++;

		cl->downloadSendTime = svs.q3_time;
	}
}

/*
=================
SV_Disconnect_f

The client is going to disconnect, so remove the connection immediately  FIXME: move to game?
=================
*/
static void SV_Disconnect_f(client_t* cl)
{
	SV_DropClient(cl, "disconnected");
}

/*
=================
SV_VerifyPaks_f

If we are pure, disconnect the client if they do no meet the following conditions:

1. the first two checksums match our view of cgame and ui
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
	qboolean bGood = qtrue;

	// if we are pure, we "expect" the client to load certain things from
	// certain pk3 files, namely we want the client to have loaded the
	// ui and cgame that we think should be loaded based on the pure setting
	//
	if (sv_pure->integer != 0)
	{

		bGood = qtrue;
		nChkSum1 = nChkSum2 = 0;
		// we run the game, so determine which cgame and ui the client "should" be running
		bGood = (FS_FileIsInPAK("vm/cgame.qvm", &nChkSum1) == 1);
		if (bGood)
		{
			bGood = (FS_FileIsInPAK("vm/ui.qvm", &nChkSum2) == 1);
		}

		nClientPaks = Cmd_Argc();

		// start at arg 2 ( skip serverId cl_paks )
		nCurArg = 1;

		pArg = Cmd_Argv(nCurArg++);
		if (!pArg)
		{
			bGood = qfalse;
		}
		else
		{
			// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=475
			// we may get incoming cp sequences from a previous checksumFeed, which we need to ignore
			// since serverId is a frame count, it always goes up
			if (String::Atoi(pArg) < sv.q3_checksumFeedServerId)
			{
				Com_DPrintf("ignoring outdated cp command from client %s\n", cl->name);
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
				bGood = qfalse;
				break;
			}
			// verify first to be the cgame checksum
			pArg = Cmd_Argv(nCurArg++);
			if (!pArg || *pArg == '@' || String::Atoi(pArg) != nChkSum1)
			{
				bGood = qfalse;
				break;
			}
			// verify the second to be the ui checksum
			pArg = Cmd_Argv(nCurArg++);
			if (!pArg || *pArg == '@' || String::Atoi(pArg) != nChkSum2)
			{
				bGood = qfalse;
				break;
			}
			// should be sitting at the delimeter now
			pArg = Cmd_Argv(nCurArg++);
			if (*pArg != '@')
			{
				bGood = qfalse;
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
						bGood = qfalse;
						break;
					}
				}
				if (bGood == qfalse)
				{
					break;
				}
			}
			if (bGood == qfalse)
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
					bGood = qfalse;
					break;
				}
			}
			if (bGood == qfalse)
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
				bGood = qfalse;
				break;
			}

			// break out
			break;
		}

		cl->q3_gotCP = qtrue;

		if (bGood)
		{
			cl->q3_pureAuthentic = 1;
		}
		else
		{
			cl->q3_pureAuthentic = 0;
			cl->q3_nextSnapshotTime = -1;
			cl->state = CS_ACTIVE;
			SV_SendClientSnapshot(cl);
			SV_DropClient(cl, "Unpure client detected. Invalid .PK3 files referenced!");
		}
	}
}

/*
=================
SV_ResetPureClient_f
=================
*/
static void SV_ResetPureClient_f(client_t* cl)
{
	cl->q3_pureAuthentic = 0;
	cl->q3_gotCP = qfalse;
}

/*
=================
SV_UserinfoChanged

Pull specific info from a newly changed userinfo string
into a more C friendly form.
=================
*/
void SV_UserinfoChanged(client_t* cl)
{
	const char* val;
	int i;

	// name for C code
	String::NCpyZ(cl->name, Info_ValueForKey(cl->userinfo, "name"), sizeof(cl->name));

	// rate command

	// if the client is on the same subnet as the server and we aren't running an
	// internet public server, assume they don't need a rate choke
	if (SOCK_IsLANAddress(cl->netchan.remoteAddress) && com_dedicated->integer != 2 && sv_lanForceRate->integer == 1)
	{
		cl->rate = 99999;	// lans should not rate limit
	}
	else
	{
		val = Info_ValueForKey(cl->userinfo, "rate");
		if (String::Length(val))
		{
			i = String::Atoi(val);
			cl->rate = i;
			if (cl->rate < 1000)
			{
				cl->rate = 1000;
			}
			else if (cl->rate > 90000)
			{
				cl->rate = 90000;
			}
		}
		else
		{
			cl->rate = 3000;
		}
	}
	val = Info_ValueForKey(cl->userinfo, "handicap");
	if (String::Length(val))
	{
		i = String::Atoi(val);
		if (i <= 0 || i > 100 || String::Length(val) > 4)
		{
			Info_SetValueForKey(cl->userinfo, "handicap", "100", MAX_INFO_STRING_Q3);
		}
	}

	// snaps command
	val = Info_ValueForKey(cl->userinfo, "snaps");
	if (String::Length(val))
	{
		i = String::Atoi(val);
		if (i < 1)
		{
			i = 1;
		}
		else if (i > 30)
		{
			i = 30;
		}
		cl->q3_snapshotMsec = 1000 / i;
	}
	else
	{
		cl->q3_snapshotMsec = 50;
	}

	// TTimo
	// maintain the IP information
	// this is set in SV_DirectConnect (directly on the server, not transmitted), may be lost when client updates it's userinfo
	// the banning code relies on this being consistently present
	val = Info_ValueForKey(cl->userinfo, "ip");
	if (!val[0])
	{
		//Com_DPrintf("Maintain IP in userinfo for '%s'\n", cl->name);
		if (!SOCK_IsLocalAddress(cl->netchan.remoteAddress))
		{
			Info_SetValueForKey(cl->userinfo, "ip", SOCK_AdrToString(cl->netchan.remoteAddress), MAX_INFO_STRING_Q3);
		}
		else
		{
			// force the "ip" info key to "localhost" for local clients
			Info_SetValueForKey(cl->userinfo, "ip", "localhost", MAX_INFO_STRING_Q3);
		}
	}
}


/*
==================
SV_UpdateUserinfo_f
==================
*/
static void SV_UpdateUserinfo_f(client_t* cl)
{
	String::NCpyZ(cl->userinfo, Cmd_Argv(1), MAX_INFO_STRING_Q3);

	SV_UserinfoChanged(cl);
	// call prog code to allow overrides
	VM_Call(gvm, GAME_CLIENT_USERINFO_CHANGED, cl - svs.clients);
}

typedef struct
{
	const char* name;
	void (* func)(client_t* cl);
} ucmd_t;

static ucmd_t ucmds[] = {
	{"userinfo", SV_UpdateUserinfo_f},
	{"disconnect", SV_Disconnect_f},
	{"cp", SV_VerifyPaks_f},
	{"vdr", SV_ResetPureClient_f},
	{"download", SV_BeginDownload_f},
	{"nextdl", SV_NextDownload_f},
	{"stopdl", SV_StopDownload_f},
	{"donedl", SV_DoneDownload_f},

	{NULL, NULL}
};

/*
==================
SV_ExecuteClientCommand

Also called by bot code
==================
*/
void SV_ExecuteClientCommand(client_t* cl, const char* s, qboolean clientOK)
{
	ucmd_t* u;
	qboolean bProcessed = qfalse;

	Cmd_TokenizeString(s);

	// see if it is a server level command
	for (u = ucmds; u->name; u++)
	{
		if (!String::Cmp(Cmd_Argv(0), u->name))
		{
			u->func(cl);
			bProcessed = qtrue;
			break;
		}
	}

	if (clientOK)
	{
		// pass unknown strings to the game
		if (!u->name && sv.state == SS_GAME)
		{
			VM_Call(gvm, GAME_CLIENT_COMMAND, cl - svs.clients);
		}
	}
	else if (!bProcessed)
	{
		Com_DPrintf("client text ignored for %s: %s\n", cl->name, Cmd_Argv(0));
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
	qboolean clientOk = qtrue;

	seq = msg->ReadLong();
	s = msg->ReadString();

	// see if we have already executed it
	if (cl->q3_lastClientCommand >= seq)
	{
		return qtrue;
	}

	Com_DPrintf("clientCommand: %s : %i : %s\n", cl->name, seq, s);

	// drop the connection if we have somehow lost commands
	if (seq > cl->q3_lastClientCommand + 1)
	{
		Com_Printf("Client %s lost %i clientCommands\n", cl->name,
			seq - cl->q3_lastClientCommand + 1);
		SV_DropClient(cl, "Lost reliable commands");
		return qfalse;
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
		clientOk = qfalse;
	}

	// don't allow another command for one second
	cl->q3_nextReliableTime = svs.q3_time + 1000;

	SV_ExecuteClientCommand(cl, s, clientOk);

	cl->q3_lastClientCommand = seq;
	String::Sprintf(cl->q3_lastClientCommandString, sizeof(cl->q3_lastClientCommandString), "%s", s);

	return qtrue;		// continue procesing
}


//==================================================================================


/*
==================
SV_ClientThink

Also called by bot code
==================
*/
void SV_ClientThink(client_t* cl, q3usercmd_t* cmd)
{
	cl->q3_lastUsercmd = *cmd;

	if (cl->state != CS_ACTIVE)
	{
		return;		// may have been kicked during the last usercmd
	}

	VM_Call(gvm, GAME_CLIENT_THINK, cl - svs.clients);
}

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
		Com_Printf("cmdCount < 1\n");
		return;
	}

	if (cmdCount > MAX_PACKET_USERCMDS)
	{
		Com_Printf("cmdCount > MAX_PACKET_USERCMDS\n");
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
	// catch the no-cp-yet situation before SV_ClientEnterWorld
	// if CS_ACTIVE, then it's time to trigger a new gamestate emission
	// if not, then we are getting remaining parasite usermove commands, which we should ignore
	if (sv_pure->integer != 0 && cl->q3_pureAuthentic == 0 && !cl->q3_gotCP)
	{
		if (cl->state == CS_ACTIVE)
		{
			// we didn't get a cp yet, don't assume anything and just send the gamestate all over again
			Com_DPrintf("%s: didn't get cp command, resending gamestate\n", cl->name, cl->state);
			SV_SendClientGameState(cl);
		}
		return;
	}

	// if this is the first usercmd we have received
	// this gamestate, put the client into the world
	if (cl->state == CS_PRIMED)
	{
		SV_ClientEnterWorld(cl, &cmds[0]);
		// the moves can be processed normaly
	}

	// a bad cp command was sent, drop the client
	if (sv_pure->integer != 0 && cl->q3_pureAuthentic == 0)
	{
		SV_DropClient(cl, "Cannot validate pure client!");
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
		SV_ClientThink(cl, &cmds[i]);
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
		SV_DropClient(cl, "DEBUG: illegible client message");
#endif
		return;
	}

	cl->q3_reliableAcknowledge = msg->ReadLong();

	// NOTE: when the client message is fux0red the acknowledgement numbers
	// can be out of range, this could cause the server to send thousands of server
	// commands which the server thinks are not yet acknowledged in SV_UpdateServerCommandsToClient
	if (cl->q3_reliableAcknowledge < cl->q3_reliableSequence - MAX_RELIABLE_COMMANDS_Q3)
	{
		// usually only hackers create messages like this
		// it is more annoying for them to let them hanging
#ifndef NDEBUG
		SV_DropClient(cl, "DEBUG: illegible client message");
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
			Com_DPrintf("%s : ignoring pre map_restart / outdated client message\n", cl->name);
			return;
		}
		// if we can tell that the client has dropped the last
		// gamestate we sent them, resend it
		if (cl->q3_messageAcknowledge > cl->q3_gamestateMessageNum)
		{
			Com_DPrintf("%s : dropped gamestate, resending\n", cl->name);
			SV_SendClientGameState(cl);
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
		SV_UserMove(cl, msg, qtrue);
	}
	else if (c == q3clc_moveNoDelta)
	{
		SV_UserMove(cl, msg, qfalse);
	}
	else if (c != q3clc_EOF)
	{
		Com_Printf("WARNING: bad command byte for client %i\n", cl - svs.clients);
	}
//	if ( msg->readcount != msg->cursize ) {
//		Com_Printf( "WARNING: Junk at end of packet for client %i\n", cl - svs.clients );
//	}
}
