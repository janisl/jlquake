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

#include "local.h"
#include "../../client_main.h"
#include "../../../common/common_defs.h"
#include "../../../common/strings.h"
#include "../../../common/command_buffer.h"
#include "../../../common/event_queue.h"
#include "../../../common/endian.h"

#define MAX_SERVERSTATUSREQUESTS    16
#define MAX_PINGREQUESTS            32

#define NUM_SERVER_PORTS    4		// broadcast scan this many ports after
									// Q3PORT_SERVER so a single machine can
									// run multiple servers

struct serverStatus_t
{
	char string[BIG_INFO_STRING];
	netadr_t address;
	int time, startTime;
	qboolean pending;
	qboolean print;
	qboolean retrieved;
};

struct ping_t
{
	netadr_t adr;
	int start;
	int time;
	char info[MAX_INFO_STRING_Q3];
};

static Cvar* clt3_serverStatusResendTime;

static serverStatus_t clt3_serverStatusList[MAX_SERVERSTATUSREQUESTS];
static int serverStatusCount;

static ping_t clt3_pinglist[MAX_PINGREQUESTS];

static serverStatus_t* CLT3_GetServerStatus(netadr_t from)
{
	for (int i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
	{
		if (SOCK_CompareAdr(from, clt3_serverStatusList[i].address))
		{
			return &clt3_serverStatusList[i];
		}
	}
	for (int i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
	{
		if (clt3_serverStatusList[i].retrieved)
		{
			return &clt3_serverStatusList[i];
		}
	}
	int oldest = -1;
	int oldestTime = 0;
	for (int i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
	{
		if (oldest == -1 || clt3_serverStatusList[i].startTime < oldestTime)
		{
			oldest = i;
			oldestTime = clt3_serverStatusList[i].startTime;
		}
	}
	if (oldest != -1)
	{
		return &clt3_serverStatusList[oldest];
	}
	serverStatusCount++;
	return &clt3_serverStatusList[serverStatusCount & (MAX_SERVERSTATUSREQUESTS - 1)];
}

int CLT3_ServerStatus(char* serverAddress, char* serverStatusString, int maxLen)
{
	// if no server address then reset all server status requests
	if (!serverAddress)
	{
		for (int i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
		{
			clt3_serverStatusList[i].address.port = 0;
			clt3_serverStatusList[i].retrieved = true;
		}
		return false;
	}
	// get the address
	netadr_t to;
	if (!SOCK_StringToAdr(serverAddress, &to, Q3PORT_SERVER))
	{
		return false;
	}
	serverStatus_t* serverStatus = CLT3_GetServerStatus(to);
	// if no server status string then reset the server status request for this address
	if (!serverStatusString)
	{
		serverStatus->retrieved = true;
		return false;
	}

	// if this server status request has the same address
	if (SOCK_CompareAdr(to, serverStatus->address))
	{
		// if we recieved an response for this server status request
		if (!serverStatus->pending)
		{
			String::NCpyZ(serverStatusString, serverStatus->string, maxLen);
			serverStatus->retrieved = true;
			serverStatus->startTime = 0;
			return true;
		}
		// resend the request regularly
		else if (serverStatus->startTime < Com_Milliseconds() - clt3_serverStatusResendTime->integer)
		{
			serverStatus->print = false;
			serverStatus->pending = true;
			serverStatus->retrieved = false;
			serverStatus->time = 0;
			serverStatus->startTime = Com_Milliseconds();
			NET_OutOfBandPrint(NS_CLIENT, to, "getstatus");
			return false;
		}
	}
	// if retrieved
	else if (serverStatus->retrieved)
	{
		serverStatus->address = to;
		serverStatus->print = false;
		serverStatus->pending = true;
		serverStatus->retrieved = false;
		serverStatus->startTime = Com_Milliseconds();
		serverStatus->time = 0;
		NET_OutOfBandPrint(NS_CLIENT, to, "getstatus");
		return false;
	}
	return false;
}

static void CLT3_ServerStatus_f()
{
	const char* server;
	if (Cmd_Argc() != 2)
	{
		if (cls.state != CA_ACTIVE || clc.demoplaying)
		{
			common->Printf("Not connected to a server.\n");
			common->Printf("Usage: serverstatus [server]\n");
			return;
		}
		server = cls.servername;
	}
	else
	{
		server = Cmd_Argv(1);
	}

	netadr_t to;
	Com_Memset(&to, 0, sizeof(netadr_t));
	if (!SOCK_StringToAdr(server, &to, Q3PORT_SERVER))
	{
		return;
	}

	NET_OutOfBandPrint(NS_CLIENT, to, "getstatus");

	serverStatus_t* serverStatus = CLT3_GetServerStatus(to);
	serverStatus->address = to;
	serverStatus->print = true;
	serverStatus->pending = true;
}

void CLT3_ServerStatusResponse(netadr_t from, QMsg* msg)
{
	serverStatus_t* serverStatus = NULL;
	for (int i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
	{
		if (SOCK_CompareAdr(from, clt3_serverStatusList[i].address))
		{
			serverStatus = &clt3_serverStatusList[i];
			break;
		}
	}
	// if we didn't request this server status
	if (!serverStatus)
	{
		return;
	}

	const char* s = msg->ReadStringLine();

	int len = 0;
	String::Sprintf(&serverStatus->string[len], sizeof(serverStatus->string) - len, "%s", s);

	if (serverStatus->print)
	{
		common->Printf("Server settings:\n");
		// print cvars
		while (*s)
		{
			for (int i = 0; i < 2 && *s; i++)
			{
				if (*s == '\\')
				{
					s++;
				}
				int l = 0;
				char info[MAX_INFO_STRING_Q3];
				while (*s)
				{
					info[l++] = *s;
					if (l >= MAX_INFO_STRING_Q3 - 1)
					{
						break;
					}
					s++;
					if (*s == '\\')
					{
						break;
					}
				}
				info[l] = '\0';
				if (i)
				{
					common->Printf("%s\n", info);
				}
				else
				{
					common->Printf("%-24s", info);
				}
			}
		}
	}

	len = String::Length(serverStatus->string);
	String::Sprintf(&serverStatus->string[len], sizeof(serverStatus->string) - len, "\\");

	if (serverStatus->print)
	{
		common->Printf("\nPlayers:\n");
		common->Printf("num: score: ping: name:\n");
	}
	s = msg->ReadStringLine();
	for (int i = 0; *s; s = msg->ReadStringLine(), i++)
	{

		len = String::Length(serverStatus->string);
		String::Sprintf(&serverStatus->string[len], sizeof(serverStatus->string) - len, "\\%s", s);

		if (serverStatus->print)
		{
			int score = 0;
			int ping = 0;
			sscanf(s, "%d %d", &score, &ping);
			s = strchr(s, ' ');
			if (s)
			{
				s = strchr(s + 1, ' ');
			}
			if (s)
			{
				s++;
			}
			else
			{
				s = "unknown";
			}
			common->Printf("%-2d   %-3d    %-3d   %s\n", i, score, ping, s);
		}
	}
	len = String::Length(serverStatus->string);
	String::Sprintf(&serverStatus->string[len], sizeof(serverStatus->string) - len, "\\");

	serverStatus->time = Com_Milliseconds();
	serverStatus->address = from;
	serverStatus->pending = false;
	if (serverStatus->print)
	{
		serverStatus->retrieved = true;
	}
}

int CLT3_GetPingQueueCount()
{
	int count = 0;
	ping_t* pingptr = clt3_pinglist;

	for (int i = 0; i < MAX_PINGREQUESTS; i++, pingptr++)
	{
		if (pingptr->adr.port)
		{
			count++;
		}
	}

	return count;
}

void CLT3_ClearPing(int n)
{
	if (n < 0 || n >= MAX_PINGREQUESTS)
	{
		return;
	}

	clt3_pinglist[n].adr.port = 0;
}

static void CLT3_SetServerInfo(q3serverInfo_t* server, const char* info, int ping)
{
	if (server)
	{
		if (info)
		{
			server->clients = String::Atoi(Info_ValueForKey(info, "clients"));
			String::NCpyZ(server->hostName,Info_ValueForKey(info, "hostname"), MAX_NAME_LENGTH_ET);
			String::NCpyZ(server->mapName, Info_ValueForKey(info, "mapname"), MAX_NAME_LENGTH_ET);
			server->maxClients = String::Atoi(Info_ValueForKey(info, "sv_maxclients"));
			String::NCpyZ(server->game,Info_ValueForKey(info, "game"), MAX_NAME_LENGTH_ET);
			server->gameType = String::Atoi(Info_ValueForKey(info, "gametype"));
			server->netType = String::Atoi(Info_ValueForKey(info, "nettype"));
			server->minPing = String::Atoi(Info_ValueForKey(info, "minping"));
			server->maxPing = String::Atoi(Info_ValueForKey(info, "maxping"));
			server->punkbuster = String::Atoi(Info_ValueForKey(info, "punkbuster"));
			if (GGameType & (GAME_WolfMP | GAME_ET))
			{
				server->allowAnonymous = String::Atoi(Info_ValueForKey(info, "sv_allowAnonymous"));
				server->friendlyFire = String::Atoi(Info_ValueForKey(info, "friendlyFire"));			// NERVE - SMF
				server->maxlives = String::Atoi(Info_ValueForKey(info, "maxlives"));					// NERVE - SMF
				String::NCpyZ(server->gameName, Info_ValueForKey(info, "gamename"), MAX_NAME_LENGTH_ET);		// Arnout
				server->antilag = String::Atoi(Info_ValueForKey(info, "g_antilag"));
			}
			if (GGameType & GAME_WolfMP)
			{
				server->tourney = String::Atoi(Info_ValueForKey(info, "tourney"));							// NERVE - SMF
			}
			if (GGameType & GAME_ET)
			{
				server->load = String::Atoi(Info_ValueForKey(info, "serverload"));
				server->needpass = String::Atoi(Info_ValueForKey(info, "needpass"));					// NERVE - SMF
				server->weaprestrict = String::Atoi(Info_ValueForKey(info, "weaprestrict"));
				server->balancedteams = String::Atoi(Info_ValueForKey(info, "balancedteams"));
			}
		}
		server->ping = ping;
	}
}

static void CLT3_SetServerInfoByAddress(netadr_t from, const char* info, int ping)
{
	for (int i = 0; i < MAX_OTHER_SERVERS_Q3; i++)
	{
		if (SOCK_CompareAdr(from, cls.q3_localServers[i].adr))
		{
			CLT3_SetServerInfo(&cls.q3_localServers[i], info, ping);
		}
	}

	for (int i = 0; i < MAX_OTHER_SERVERS_Q3; i++)
	{
		if (SOCK_CompareAdr(from, cls.q3_mplayerServers[i].adr))
		{
			CLT3_SetServerInfo(&cls.q3_mplayerServers[i], info, ping);
		}
	}

	for (int i = 0; i < BIGGEST_MAX_GLOBAL_SERVERS; i++)
	{
		if (SOCK_CompareAdr(from, cls.q3_globalServers[i].adr))
		{
			CLT3_SetServerInfo(&cls.q3_globalServers[i], info, ping);
		}
	}

	for (int i = 0; i < MAX_OTHER_SERVERS_Q3; i++)
	{
		if (SOCK_CompareAdr(from, cls.q3_favoriteServers[i].adr))
		{
			CLT3_SetServerInfo(&cls.q3_favoriteServers[i], info, ping);
		}
	}

}

void CLT3_GetPing(int n, char* buf, int buflen, int* pingtime)
{
	if (n < 0 || n >= MAX_PINGREQUESTS || !clt3_pinglist[n].adr.port)
	{
		// empty slot
		buf[0]    = '\0';
		*pingtime = 0;
		return;
	}

	const char* str = SOCK_AdrToString(clt3_pinglist[n].adr);
	String::NCpyZ(buf, str, buflen);

	int time = clt3_pinglist[n].time;
	if (!time)
	{
		// check for timeout
		time = cls.realtime - clt3_pinglist[n].start;
		int maxPing = Cvar_VariableIntegerValue("cl_maxPing");
		if (maxPing < 100)
		{
			maxPing = 100;
		}
		if (time < maxPing)
		{
			// not timed out yet
			time = 0;
		}
	}

	CLT3_SetServerInfoByAddress(clt3_pinglist[n].adr, clt3_pinglist[n].info, clt3_pinglist[n].time);

	*pingtime = time;
}

void CLT3_GetPingInfo(int n, char* buf, int buflen)
{
	if (n < 0 || n >= MAX_PINGREQUESTS || !clt3_pinglist[n].adr.port)
	{
		// empty slot
		if (buflen)
		{
			buf[0] = '\0';
		}
		return;
	}

	String::NCpyZ(buf, clt3_pinglist[n].info, buflen);
}

static void CLT3_InitServerInfo(q3serverInfo_t* server, netadr_t* address)
{
	server->adr = *address;
	server->clients = 0;
	server->hostName[0] = '\0';
	server->mapName[0] = '\0';
	server->maxClients = 0;
	server->maxPing = 0;
	server->minPing = 0;
	server->ping = -1;
	server->game[0] = '\0';
	server->gameType = 0;
	server->netType = 0;
	server->allowAnonymous = 0;
}

bool CLT3_UpdateVisiblePings(int source)
{
	if (source < 0 || source > (GGameType & (GAME_WolfMP | GAME_ET) ? WMAS_FAVORITES : Q3AS_FAVORITES))
	{
		return false;
	}

	cls.q3_pingUpdateSource = source;

	bool status = false;

	int slots = CLT3_GetPingQueueCount();
	if (slots < MAX_PINGREQUESTS)
	{
		q3serverInfo_t* servers;
		int max;
		int* count;
		CLT3_GetServersForSource(source, servers, max, count);

		for (int i = 0; i < *count; i++)
		{
			if (servers[i].visible)
			{
				if (servers[i].ping == -1)
				{
					int j;

					if (slots >= MAX_PINGREQUESTS)
					{
						break;
					}
					for (j = 0; j < MAX_PINGREQUESTS; j++)
					{
						if (!clt3_pinglist[j].adr.port)
						{
							continue;
						}
						if (SOCK_CompareAdr(clt3_pinglist[j].adr, servers[i].adr))
						{
							// already on the list
							break;
						}
					}
					if (j >= MAX_PINGREQUESTS)
					{
						status = true;
						for (j = 0; j < MAX_PINGREQUESTS; j++)
						{
							if (!clt3_pinglist[j].adr.port)
							{
								break;
							}
						}
						Com_Memcpy(&clt3_pinglist[j].adr, &servers[i].adr, sizeof(netadr_t));
						clt3_pinglist[j].start = cls.realtime;
						clt3_pinglist[j].time = 0;
						NET_OutOfBandPrint(NS_CLIENT, clt3_pinglist[j].adr, "getinfo xxx");
						slots++;
					}
				}
				// if the server has a ping higher than cl_maxPing or
				// the ping packet got lost
				else if (servers[i].ping == 0)
				{
					// if we are updating global servers
					if (source == (GGameType & (GAME_WolfMP | GAME_ET) ? WMAS_GLOBAL : Q3AS_GLOBAL))
					{
						//
						if (cls.q3_numGlobalServerAddresses > 0)
						{
							// overwrite this server with one from the additional global servers
							cls.q3_numGlobalServerAddresses--;
							CLT3_InitServerInfo(&servers[i], &cls.q3_globalServerAddresses[cls.q3_numGlobalServerAddresses]);
							// NOTE: the server[i].visible flag stays untouched
						}
					}
				}
			}
		}
	}

	if (slots)
	{
		status = true;
	}
	for (int i = 0; i < MAX_PINGREQUESTS; i++)
	{
		if (!clt3_pinglist[i].adr.port)
		{
			continue;
		}
		char buff[MAX_STRING_CHARS];
		int pingTime;
		CLT3_GetPing(i, buff, MAX_STRING_CHARS, &pingTime);
		if (pingTime != 0)
		{
			CLT3_ClearPing(i);
			status = true;
		}
	}

	return status;
}

static ping_t* CLT3_GetFreePing()
{
	ping_t* pingptr = clt3_pinglist;
	for (int i = 0; i < MAX_PINGREQUESTS; i++, pingptr++)
	{
		// find free ping slot
		if (pingptr->adr.port)
		{
			if (!pingptr->time)
			{
				if (cls.realtime - pingptr->start < 500)
				{
					// still waiting for response
					continue;
				}
			}
			else if (pingptr->time < 500)
			{
				// results have not been queried
				continue;
			}
		}

		// clear it
		pingptr->adr.port = 0;
		return pingptr;
	}

	// use oldest entry
	pingptr = clt3_pinglist;
	ping_t* best = clt3_pinglist;
	int oldest = MIN_QINT32;
	for (int i = 0; i < MAX_PINGREQUESTS; i++, pingptr++)
	{
		// scan for oldest
		int time = cls.realtime - pingptr->start;
		if (time > oldest)
		{
			oldest = time;
			best   = pingptr;
		}
	}

	return best;
}

static void CLT3_Ping_f()
{

	if (Cmd_Argc() != 2)
	{
		common->Printf("usage: ping [server]\n");
		return;
	}

	netadr_t to;
	Com_Memset(&to, 0, sizeof(netadr_t));

	char* server = Cmd_Argv(1);

	if (!SOCK_StringToAdr(server, &to, Q3PORT_SERVER))
	{
		return;
	}

	ping_t* pingptr = CLT3_GetFreePing();

	Com_Memcpy(&pingptr->adr, &to, sizeof(netadr_t));
	pingptr->start = cls.realtime;
	pingptr->time  = 0;

	CLT3_SetServerInfoByAddress(pingptr->adr, NULL, 0);

	NET_OutOfBandPrint(NS_CLIENT, to, "getinfo xxx");
}

static void CLT3_LocalServers_f()
{
	common->Printf("Scanning for servers on the local network...\n");

	// reset the list, waiting for response
	cls.q3_numlocalservers = 0;
	cls.q3_pingUpdateSource = GGameType & (GAME_WolfMP | GAME_ET) ? WMAS_LOCAL : Q3AS_LOCAL;

	for (int i = 0; i < MAX_OTHER_SERVERS_Q3; i++)
	{
		bool b = cls.q3_localServers[i].visible;
		Com_Memset(&cls.q3_localServers[i], 0, sizeof(cls.q3_localServers[i]));
		cls.q3_localServers[i].visible = b;
	}
	netadr_t to;
	Com_Memset(&to, 0, sizeof(to));

	// The 'xxx' in the message is a challenge that will be echoed back
	// by the server.  We don't care about that here, but master servers
	// can use that to prevent spoofed server responses from invalid ip
	const char* message = "\377\377\377\377getinfo xxx";

	// send each message twice in case one is dropped
	for (int i = 0; i < 2; i++)
	{
		// send a broadcast packet on each server port
		// we support multiple server ports so a single machine
		// can nicely run multiple servers
		for (int j = 0; j < NUM_SERVER_PORTS; j++)
		{
			to.port = BigShort((short)(Q3PORT_SERVER + j));

			to.type = NA_BROADCAST;
			NET_SendPacket(NS_CLIENT, String::Length(message), message, to);
		}
	}
}

static void CLT3_GlobalServers_f()
{
	if (Cmd_Argc() < 3)
	{
		common->Printf("usage: globalservers <master# 0-1> <protocol> [keywords]\n");
		return;
	}

	cls.q3_masterNum = String::Atoi(Cmd_Argv(1));

	common->Printf("Requesting servers from the master...\n");

	// reset the list, waiting for response
	// -1 is used to distinguish a "no response"

	netadr_t to;
	if (!(GGameType & GAME_ET) && cls.q3_masterNum == 1)
	{
		SOCK_StringToAdr(GGameType & GAME_Quake3 ? Q3MASTER_SERVER_NAME : "master.quake3world.com", &to, Q3PORT_MASTER);
		cls.q3_nummplayerservers = -1;
		cls.q3_pingUpdateSource = GGameType & GAME_WolfMP ? WMAS_MPLAYER : Q3AS_MPLAYER;
	}
	else
	{
		SOCK_StringToAdr(GGameType & GAME_WolfSP ? WSMASTER_SERVER_NAME :
			GGameType & GAME_WolfMP ? WMMASTER_SERVER_NAME :
			GGameType & GAME_ET ? ETMASTER_SERVER_NAME : Q3MASTER_SERVER_NAME, &to, Q3PORT_MASTER);
		cls.q3_numglobalservers = -1;
		cls.q3_pingUpdateSource = GGameType & (GAME_WolfMP | GAME_ET) ? WMAS_GLOBAL : Q3AS_GLOBAL;
	}
	to.type = NA_IP;

	char command[1024];
	sprintf(command, "getservers %s", Cmd_Argv(2));

	// tack on keywords
	char* buffptr = command + String::Length(command);
	int count   = Cmd_Argc();
	for (int i = 3; i < count; i++)
	{
		buffptr += sprintf(buffptr, " %s", Cmd_Argv(i));
	}

	NET_OutOfBandPrint(NS_SERVER, to, "%s", command);
}

void CLT3_ServerInfoPacket(netadr_t from, QMsg* msg)
{
	const char* infoString = msg->ReadString();

	int protocol = GGameType & GAME_WolfSP ? WSPROTOCOL_VERSION :
		GGameType & GAME_WolfMP ? WMPROTOCOL_VERSION :
		GGameType & GAME_ET ? ETPROTOCOL_VERSION : Q3PROTOCOL_VERSION;
	int debug_protocol = Cvar_VariableIntegerValue("debug_protocol");
	if (debug_protocol)
	{
		protocol = debug_protocol;
	}

	// if this isn't the correct protocol version, ignore it
	int prot = String::Atoi(Info_ValueForKey(infoString, "protocol"));
	if (prot != protocol)
	{
		common->DPrintf("Different protocol info packet: %s\n", infoString);
		return;
	}

	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		// if this isn't the correct game, ignore it
		const char* gameName = Info_ValueForKey(infoString, "gamename");
		if (!gameName[0] || String::ICmp(gameName, GGameType & GAME_ET ? ETGAMENAME_STRING : WMGAMENAME_STRING))
		{
			common->DPrintf("Different game info packet: %s\n", infoString);
			return;
		}
	}

	// iterate servers waiting for ping response
	for (int i = 0; i < MAX_PINGREQUESTS; i++)
	{
		if (clt3_pinglist[i].adr.port && !clt3_pinglist[i].time && SOCK_CompareAdr(from, clt3_pinglist[i].adr))
		{
			// calc ping time
			clt3_pinglist[i].time = cls.realtime - clt3_pinglist[i].start + 1;
			common->DPrintf("ping time %dms from %s\n", clt3_pinglist[i].time, SOCK_AdrToString(from));

			// save of info
			String::NCpyZ(clt3_pinglist[i].info, infoString, sizeof(clt3_pinglist[i].info));

			// tack on the net type
			// NOTE: make sure these types are in sync with the netnames strings in the UI
			int type;
			switch (from.type)
			{
			case NA_BROADCAST:
			case NA_IP:
				type = 1;
				break;

			default:
				type = 0;
				break;
			}
			Info_SetValueForKey(clt3_pinglist[i].info, "nettype", va("%d", type), MAX_INFO_STRING_Q3);
			CLT3_SetServerInfoByAddress(from, infoString, clt3_pinglist[i].time);

			return;
		}
	}

	// if not just sent a local broadcast or pinging local servers
	if (cls.q3_pingUpdateSource != (GGameType & (GAME_WolfMP | GAME_ET) ? WMAS_LOCAL : Q3AS_LOCAL))
	{
		return;
	}

	int i;
	for (i = 0; i < MAX_OTHER_SERVERS_Q3; i++)
	{
		// empty slot
		if (cls.q3_localServers[i].adr.port == 0)
		{
			break;
		}

		// avoid duplicate
		if (SOCK_CompareAdr(from, cls.q3_localServers[i].adr))
		{
			return;
		}
	}

	if (i == MAX_OTHER_SERVERS_Q3)
	{
		common->DPrintf("MAX_OTHER_SERVERS_Q3 hit, dropping infoResponse\n");
		return;
	}

	// add this to the list
	cls.q3_numlocalservers = i + 1;
	cls.q3_localServers[i].adr = from;
	cls.q3_localServers[i].clients = 0;
	cls.q3_localServers[i].hostName[0] = '\0';
	cls.q3_localServers[i].mapName[0] = '\0';
	cls.q3_localServers[i].maxClients = 0;
	cls.q3_localServers[i].maxPing = 0;
	cls.q3_localServers[i].minPing = 0;
	cls.q3_localServers[i].ping = -1;
	cls.q3_localServers[i].game[0] = '\0';
	cls.q3_localServers[i].gameType = 0;
	cls.q3_localServers[i].netType = from.type;
	cls.q3_localServers[i].punkbuster = 0;
	cls.q3_localServers[i].allowAnonymous = 0;
	cls.q3_localServers[i].friendlyFire = 0;			// NERVE - SMF
	cls.q3_localServers[i].maxlives = 0;				// NERVE - SMF
	cls.q3_localServers[i].gameName[0] = '\0';			// Arnout
	cls.q3_localServers[i].tourney = 0;					// NERVE - SMF
	cls.q3_localServers[i].needpass = 0;
	cls.q3_localServers[i].antilag = 0;
	cls.q3_localServers[i].load = -1;
	cls.q3_localServers[i].weaprestrict = 0;
	cls.q3_localServers[i].balancedteams = 0;

	char info[MAX_INFO_STRING_Q3];
	String::NCpyZ(info, msg->ReadString(), MAX_INFO_STRING_Q3);
	if (String::Length(info))
	{
		if (info[String::Length(info) - 1] != '\n')
		{
			String::Cat(info, sizeof(info), "\n");
		}
		common->Printf("%s: %s", SOCK_AdrToString(from), info);
	}
}

#define MAX_SERVERSPERPACKET    256

void CLT3_ServersResponsePacket(netadr_t from, QMsg* msg)
{
	common->Printf("CLT3_ServersResponsePacket\n");

	if (cls.q3_numglobalservers == -1)
	{
		// state to detect lack of servers or lack of response
		cls.q3_numglobalservers = 0;
		cls.q3_numGlobalServerAddresses = 0;
	}

	if (cls.q3_nummplayerservers == -1)
	{
		cls.q3_nummplayerservers = 0;
	}

	// parse through server response string
	netadr_t addresses[MAX_SERVERSPERPACKET];
	int numservers = 0;
	byte* buffptr = msg->_data;
	byte* buffend = buffptr + msg->cursize;
	while (buffptr + 1 < buffend)
	{
		// advance to initial token
		do
		{
			if (*buffptr++ == '\\')
			{
				break;
			}
		}
		while (buffptr < buffend);

		if (buffptr >= buffend - 6)
		{
			break;
		}

		addresses[numservers].type = NA_IP;
		// parse out ip
		addresses[numservers].ip[0] = *buffptr++;
		addresses[numservers].ip[1] = *buffptr++;
		addresses[numservers].ip[2] = *buffptr++;
		addresses[numservers].ip[3] = *buffptr++;

		// parse out port
		addresses[numservers].port = (*buffptr++) << 8;
		addresses[numservers].port += *buffptr++;
		addresses[numservers].port = BigShort(addresses[numservers].port);

		// syntax check
		if (*buffptr != '\\')
		{
			break;
		}

		common->DPrintf("server: %d ip: %s\n",numservers, SOCK_AdrToString(addresses[numservers]));

		numservers++;
		if (numservers >= MAX_SERVERSPERPACKET)
		{
			break;
		}

		// parse out EOT
		if (buffptr[1] == 'E' && buffptr[2] == 'O' && buffptr[3] == 'T')
		{
			break;
		}
	}

	int count;
	int max;
	if (cls.q3_masterNum == 0)
	{
		count = cls.q3_numglobalservers;
		max = GGameType & GAME_WolfSP ? MAX_GLOBAL_SERVERS_WS :
			GGameType & GAME_WolfMP ? MAX_GLOBAL_SERVERS_WM :
			GGameType & GAME_ET ? MAX_GLOBAL_SERVERS_ET : MAX_GLOBAL_SERVERS_Q3;
	}
	else
	{
		count = cls.q3_nummplayerservers;
		max = MAX_OTHER_SERVERS_Q3;
	}

	int i;
	for (i = 0; i < numservers && count < max; i++)
	{
		// build net address
		q3serverInfo_t* server = (cls.q3_masterNum == 0) ? &cls.q3_globalServers[count] : &cls.q3_mplayerServers[count];

		CLT3_InitServerInfo(server, &addresses[i]);
		// advance to next slot
		count++;
	}

	// if getting the global list and there are too many servers
	if (cls.q3_masterNum == 0 && count >= max)
	{
		for (; i < numservers && cls.q3_numGlobalServerAddresses < max; i++)
		{
			// just store the addresses in an additional list
			cls.q3_globalServerAddresses[cls.q3_numGlobalServerAddresses++] = addresses[i];
		}
	}

	int total;
	if (cls.q3_masterNum == 0)
	{
		cls.q3_numglobalservers = count;
		total = count + cls.q3_numGlobalServerAddresses;
	}
	else
	{
		cls.q3_nummplayerservers = count;
		total = count;
	}

	common->Printf("%d servers parsed (total %d)\n", numservers, total);
}

void CLT3_InitServerLists()
{
	clt3_serverStatusResendTime = Cvar_Get("cl_serverStatusResendTime", "750", 0);
	Cmd_AddCommand("serverstatus", CLT3_ServerStatus_f);
	Cmd_AddCommand("ping", CLT3_Ping_f);
	Cmd_AddCommand("localservers", CLT3_LocalServers_f);
	Cmd_AddCommand("globalservers", CLT3_GlobalServers_f);
}
