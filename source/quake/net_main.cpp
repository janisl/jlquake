/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// net_main.c

#include "quakedef.h"

qsocket_t* net_activeSockets = NULL;
qsocket_t* net_freeSockets = NULL;
int net_numsockets = 0;

qboolean tcpipAvailable = false;

int net_hostport;
int DEFAULTnet_hostport = 26000;

static qboolean listening = false;

qboolean slistInProgress = false;
qboolean slistSilent = false;
qboolean slistLocal = true;
static double slistStartTime;
static int slistLastShown;

static void Slist_Send(void);
static void Slist_Poll(void);
PollProcedure slistSendProcedure = {NULL, 0.0, Slist_Send};
PollProcedure slistPollProcedure = {NULL, 0.0, Slist_Poll};


QMsg net_message;
byte net_message_buf[MAX_MSGLEN_Q1];
int net_activeconnections = 0;

int messagesSent = 0;
int messagesReceived = 0;
int unreliableMessagesSent = 0;
int unreliableMessagesReceived = 0;

Cvar* net_messagetimeout;
Cvar* hostname;

#ifdef IDGODS
Cvar* idgods = {"idgods", "0"};
#endif

double net_time;

#include "net_loop.h"
#include "net_dgrm.h"

bool datagram_initialized;

double SetNetTime(void)
{
	net_time = Sys_DoubleTime();
	return net_time;
}


/*
===================
NET_NewQSocket

Called by drivers when a new communications endpoint is required
The sequence and buffer fields will be filled in properly
===================
*/
qsocket_t* NET_NewQSocket(void)
{
	qsocket_t* sock;

	if (net_freeSockets == NULL)
	{
		return NULL;
	}

	if (net_activeconnections >= svs.qh_maxclients)
	{
		return NULL;
	}

	// get one from free list
	sock = net_freeSockets;
	net_freeSockets = sock->next;

	// add it to active list
	sock->next = net_activeSockets;
	net_activeSockets = sock;

	sock->disconnected = false;
	sock->connecttime = net_time;
	String::Cpy(sock->address,"UNSET ADDRESS");
	sock->socket = 0;
	sock->canSend = true;
	sock->sendNext = false;

	return sock;
}


void NET_FreeQSocket(qsocket_t* sock)
{
	qsocket_t* s;

	// remove it from active list
	if (sock == net_activeSockets)
	{
		net_activeSockets = net_activeSockets->next;
	}
	else
	{
		for (s = net_activeSockets; s; s = s->next)
			if (s->next == sock)
			{
				s->next = sock->next;
				break;
			}
		if (!s)
		{
			Sys_Error("NET_FreeQSocket: not active\n");
		}
	}

	// add it to free list
	sock->next = net_freeSockets;
	net_freeSockets = sock;
	sock->disconnected = true;
}


static void NET_Listen_f(void)
{
	if (Cmd_Argc() != 2)
	{
		Con_Printf("\"listen\" is \"%u\"\n", listening ? 1 : 0);
		return;
	}

	listening = String::Atoi(Cmd_Argv(1)) ? true : false;

	Loop_Listen(listening);
	if (datagram_initialized)
	{
		Datagram_Listen(listening);
	}
}


static void MaxPlayers_f(void)
{
	int n;

	if (Cmd_Argc() != 2)
	{
		Con_Printf("\"maxplayers\" is \"%u\"\n", svs.qh_maxclients);
		return;
	}

	if (sv.state != SS_DEAD)
	{
		Con_Printf("maxplayers can not be changed while a server is running.\n");
		return;
	}

	n = String::Atoi(Cmd_Argv(1));
	if (n < 1)
	{
		n = 1;
	}
	if (n > svs.qh_maxclientslimit)
	{
		n = svs.qh_maxclientslimit;
		Con_Printf("\"maxplayers\" set to \"%u\"\n", n);
	}

	if ((n == 1) && listening)
	{
		Cbuf_AddText("listen 0\n");
	}

	if ((n > 1) && (!listening))
	{
		Cbuf_AddText("listen 1\n");
	}

	svs.qh_maxclients = n;
	if (n == 1)
	{
		Cvar_Set("deathmatch", "0");
	}
	else
	{
		Cvar_Set("deathmatch", "1");
	}
}


static void NET_Port_f(void)
{
	int n;

	if (Cmd_Argc() != 2)
	{
		Con_Printf("\"port\" is \"%u\"\n", net_hostport);
		return;
	}

	n = String::Atoi(Cmd_Argv(1));
	if (n < 1 || n > 65534)
	{
		Con_Printf("Bad value, must be between 1 and 65534\n");
		return;
	}

	DEFAULTnet_hostport = n;
	net_hostport = n;

	if (listening)
	{
		// force a change to the new port
		Cbuf_AddText("listen 0\n");
		Cbuf_AddText("listen 1\n");
	}
}


static void PrintSlistHeader(void)
{
	Con_Printf("Server          Map             Users\n");
	Con_Printf("--------------- --------------- -----\n");
	slistLastShown = 0;
}


static void PrintSlist(void)
{
	int n;

	for (n = slistLastShown; n < hostCacheCount; n++)
	{
		if (hostcache[n].maxusers)
		{
			Con_Printf("%-15.15s %-15.15s %2u/%2u\n", hostcache[n].name, hostcache[n].map, hostcache[n].users, hostcache[n].maxusers);
		}
		else
		{
			Con_Printf("%-15.15s %-15.15s\n", hostcache[n].name, hostcache[n].map);
		}
	}
	slistLastShown = n;
}


static void PrintSlistTrailer(void)
{
	if (hostCacheCount)
	{
		Con_Printf("== end list ==\n\n");
	}
	else
	{
		Con_Printf("No Quake servers found.\n\n");
	}
}


void NET_Slist_f(void)
{
	if (slistInProgress)
	{
		return;
	}

	if (!slistSilent)
	{
		Con_Printf("Looking for Quake servers...\n");
		PrintSlistHeader();
	}

	slistInProgress = true;
	slistStartTime = Sys_DoubleTime();

	SchedulePollProcedure(&slistSendProcedure, 0.0);
	SchedulePollProcedure(&slistPollProcedure, 0.1);

	hostCacheCount = 0;
}


static void Slist_Send(void)
{
	if (slistLocal)
	{
		Loop_SearchForHosts(true);
	}
	if (datagram_initialized)
	{
		Datagram_SearchForHosts(true);
	}

	if ((Sys_DoubleTime() - slistStartTime) < 0.5)
	{
		SchedulePollProcedure(&slistSendProcedure, 0.75);
	}
}


static void Slist_Poll(void)
{
	if (slistLocal)
	{
		Loop_SearchForHosts(false);
	}
	if (datagram_initialized)
	{
		Datagram_SearchForHosts(false);
	}

	if (!slistSilent)
	{
		PrintSlist();
	}

	if ((Sys_DoubleTime() - slistStartTime) < 1.5)
	{
		SchedulePollProcedure(&slistPollProcedure, 0.1);
		return;
	}

	if (!slistSilent)
	{
		PrintSlistTrailer();
	}
	slistInProgress = false;
	slistSilent = false;
	slistLocal = true;
}


/*
===================
NET_Connect
===================
*/

int hostCacheCount = 0;
hostcache_t hostcache[HOSTCACHESIZE];

qsocket_t* NET_Connect(const char* host, netchan_t* chan)
{
	qsocket_t* ret;
	int n;
	int numdrivers = 2;

	SetNetTime();

	if (host && *host == 0)
	{
		host = NULL;
	}

	if (host)
	{
		if (String::ICmp(host, "local") == 0)
		{
			numdrivers = 1;
			goto JustDoIt;
		}

		if (hostCacheCount)
		{
			for (n = 0; n < hostCacheCount; n++)
				if (String::ICmp(host, hostcache[n].name) == 0)
				{
					host = hostcache[n].cname;
					break;
				}
			if (n < hostCacheCount)
			{
				goto JustDoIt;
			}
		}
	}

	slistSilent = host ? true : false;
	NET_Slist_f();

	while (slistInProgress)
		NET_Poll();

	if (host == NULL)
	{
		if (hostCacheCount != 1)
		{
			return NULL;
		}
		host = hostcache[0].cname;
		Con_Printf("Connecting to...\n%s @ %s\n\n", hostcache[0].name, host);
	}

	if (hostCacheCount)
	{
		for (n = 0; n < hostCacheCount; n++)
			if (String::ICmp(host, hostcache[n].name) == 0)
			{
				host = hostcache[n].cname;
				break;
			}
	}

JustDoIt:
	ret = Loop_Connect(host, chan);
	if (ret)
	{
		return ret;
	}

	if (datagram_initialized && numdrivers > 1)
	{
		ret = Datagram_Connect(host, chan);
		if (ret)
		{
			return ret;
		}
	}

	if (host)
	{
		Con_Printf("\n");
		PrintSlistHeader();
		PrintSlist();
		PrintSlistTrailer();
	}

	return NULL;
}


/*
===================
NET_CheckNewConnections
===================
*/

qsocket_t* NET_CheckNewConnections(netadr_t* outaddr)
{
	Com_Memset(outaddr, 0, sizeof(*outaddr));
	qsocket_t* ret;

	SetNetTime();

	ret = Loop_CheckNewConnections(outaddr);
	if (ret)
	{
		return ret;
	}

	if (!datagram_initialized)
	{
		return NULL;
	}
	if (listening == false)
	{
		return NULL;
	}
	ret = Datagram_CheckNewConnections(outaddr);
	if (ret)
	{
		return ret;
	}

	return NULL;
}

/*
===================
NET_Close
===================
*/
void NET_Close(qsocket_t* sock, netchan_t* chan)
{
	if (!sock)
	{
		return;
	}

	if (sock->disconnected)
	{
		return;
	}

	SetNetTime();

	// call the driver_Close function
	if (chan->remoteAddress.type == NA_LOOPBACK)
	{
		Loop_Close(sock, chan);
	}
	else
	{
		Datagram_Close(sock, chan);
	}

	NET_FreeQSocket(sock);
}


/*
=================
NET_GetMessage

If there is a complete message, return it in net_message

returns 0 if no data is waiting
returns 1 if a message was received
returns -1 if connection is invalid
=================
*/

extern void PrintStats(qsocket_t* s);

int NET_GetMessage(qsocket_t* sock, netchan_t* chan)
{
	int ret;

	if (!sock)
	{
		return -1;
	}

	if (sock->disconnected)
	{
		Con_Printf("NET_GetMessage: disconnected socket\n");
		return -1;
	}

	SetNetTime();

	if (chan->remoteAddress.type == NA_LOOPBACK)
	{
		ret = Loop_GetMessage(sock, chan);
	}
	else
	{
		ret = Datagram_GetMessage(sock, chan);
	}

	// see if this connection has timed out
	if (ret == 0 && chan->remoteAddress.type == NA_IP)
	{
		if (net_time - chan->lastReceived * 1000.0 > net_messagetimeout->value)
		{
			NET_Close(sock, chan);
			return -1;
		}
	}


	if (ret > 0)
	{
		if (chan->remoteAddress.type == NA_IP)
		{
			chan->lastReceived = net_time * 1000;
			if (ret == 1)
			{
				messagesReceived++;
			}
			else if (ret == 2)
			{
				unreliableMessagesReceived++;
			}
		}
	}

	return ret;
}


/*
==================
NET_SendMessage

Try to send a complete length+message unit over the reliable stream.
returns 0 if the message cannot be delivered reliably, but the connection
        is still considered valid
returns 1 if the message was sent properly
returns -1 if the connection died
==================
*/
int NET_SendMessage(qsocket_t* sock, netchan_t* chan, QMsg* data)
{
	int r;

	if (!sock)
	{
		return -1;
	}

	if (sock->disconnected)
	{
		Con_Printf("NET_SendMessage: disconnected socket\n");
		return -1;
	}

	SetNetTime();
	if (chan->remoteAddress.type == NA_LOOPBACK)
	{
		r = Loop_SendMessage(sock, chan, data);
	}
	else
	{
		r = Datagram_SendMessage(sock, chan, data);
	}
	if (r == 1 && chan->remoteAddress.type == NA_IP)
	{
		messagesSent++;
	}

	return r;
}


int NET_SendUnreliableMessage(qsocket_t* sock, netchan_t* chan, QMsg* data)
{
	int r;

	if (!sock)
	{
		return -1;
	}

	if (sock->disconnected)
	{
		Con_Printf("NET_SendMessage: disconnected socket\n");
		return -1;
	}

	SetNetTime();
	if (chan->remoteAddress.type == NA_LOOPBACK)
	{
		r = Loop_SendUnreliableMessage(sock, chan, data);
	}
	else
	{
		r = Datagram_SendUnreliableMessage(sock, chan, data);
	}
	if (r == 1 && chan->remoteAddress.type == NA_IP)
	{
		unreliableMessagesSent++;
	}

	return r;
}


/*
==================
NET_CanSendMessage

Returns true or false if the given qsocket can currently accept a
message to be transmitted.
==================
*/
qboolean NET_CanSendMessage(qsocket_t* sock, netchan_t* chan)
{
	int r;

	if (!sock)
	{
		return false;
	}

	if (sock->disconnected)
	{
		return false;
	}

	SetNetTime();

	if (chan->remoteAddress.type == NA_LOOPBACK)
	{
		r = Loop_CanSendMessage(sock, chan);
	}
	else
	{
		r = Datagram_CanSendMessage(sock, chan);
	}

	return r;
}


int NET_SendToAll(QMsg* data, int blocktime)
{
	double start;
	int i;
	int count = 0;
	qboolean state1 [MAX_CLIENTS_Q1];
	qboolean state2 [MAX_CLIENTS_Q1];

	for (i = 0, host_client = svs.clients; i < svs.qh_maxclients; i++, host_client++)
	{
		if (!host_client->qh_netconnection)
		{
			continue;
		}
		if (host_client->state >= CS_CONNECTED)
		{
			if (host_client->netchan.remoteAddress.type == NA_LOOPBACK)
			{
				NET_SendMessage(host_client->qh_netconnection, &host_client->netchan, data);
				state1[i] = true;
				state2[i] = true;
				continue;
			}
			count++;
			state1[i] = false;
			state2[i] = false;
		}
		else
		{
			state1[i] = true;
			state2[i] = true;
		}
	}

	start = Sys_DoubleTime();
	while (count)
	{
		count = 0;
		for (i = 0, host_client = svs.clients; i < svs.qh_maxclients; i++, host_client++)
		{
			if (!state1[i])
			{
				if (NET_CanSendMessage(host_client->qh_netconnection, &host_client->netchan))
				{
					state1[i] = true;
					NET_SendMessage(host_client->qh_netconnection, &host_client->netchan, data);
				}
				else
				{
					NET_GetMessage(host_client->qh_netconnection, &host_client->netchan);
				}
				count++;
				continue;
			}

			if (!state2[i])
			{
				if (NET_CanSendMessage(host_client->qh_netconnection, &host_client->netchan))
				{
					state2[i] = true;
				}
				else
				{
					NET_GetMessage(host_client->qh_netconnection, &host_client->netchan);
				}
				count++;
				continue;
			}
		}
		if ((Sys_DoubleTime() - start) > blocktime)
		{
			break;
		}
	}
	return count;
}


//=============================================================================

/*
====================
NET_Init
====================
*/

void NET_Init(void)
{
	int i;
	int controlSocket;
	qsocket_t* s;

	i = COM_CheckParm("-port");
	if (!i)
	{
		i = COM_CheckParm("-udpport");
	}

	if (i)
	{
		if (i < COM_Argc() - 1)
		{
			DEFAULTnet_hostport = String::Atoi(COM_Argv(i + 1));
		}
		else
		{
			Sys_Error("NET_Init: you must specify a number after -port");
		}
	}
	net_hostport = DEFAULTnet_hostport;

	if (COM_CheckParm("-listen") || cls.state == CA_DEDICATED)
	{
		listening = true;
	}
	net_numsockets = svs.qh_maxclientslimit;
	if (cls.state != CA_DEDICATED)
	{
		net_numsockets++;
	}

	SetNetTime();

	for (i = 0; i < net_numsockets; i++)
	{
		s = (qsocket_t*)Hunk_AllocName(sizeof(qsocket_t), "qsocket");
		s->next = net_freeSockets;
		net_freeSockets = s;
		s->disconnected = true;
	}

	// allocate space for network message buffer
	net_message.InitOOB(net_message_buf, MAX_MSGLEN_Q1);

	net_messagetimeout = Cvar_Get("net_messagetimeout", "300", 0);
	hostname = Cvar_Get("hostname", "UNNAMED", CVAR_ARCHIVE);
#ifdef IDGODS
	idgods = Cvar_Get("idgods", "0", 0);
#endif

	Cmd_AddCommand("slist", NET_Slist_f);
	Cmd_AddCommand("listen", NET_Listen_f);
	Cmd_AddCommand("maxplayers", MaxPlayers_f);
	Cmd_AddCommand("port", NET_Port_f);

	// initialize all the drivers
	controlSocket = Loop_Init();
	if (controlSocket != -1)
	{
		if (listening)
		{
			Loop_Listen(true);
		}
	}
	controlSocket = Datagram_Init();
	if (controlSocket != -1)
	{
		datagram_initialized = true;
		if (listening)
		{
			Datagram_Listen(true);
		}
	}
}

/*
====================
NET_Shutdown
====================
*/

void        NET_Shutdown(void)
{
	SetNetTime();

	client_t* client = svs.clients;
	for (int i = 0; i < svs.qh_maxclients; i++, client++)
	{
		if (!client->qh_netconnection)
		{
			continue;
		}
		if (client->state < CS_CONNECTED)
		{
			continue;
		}
		NET_Close(client->qh_netconnection, &client->netchan);
	}
	if (cls.qh_netcon)
	{
		NET_Close(cls.qh_netcon, &clc.netchan);
	}

//
// shutdown the drivers
//
	Loop_Shutdown();
	if (datagram_initialized)
	{
		Datagram_Shutdown();
		datagram_initialized = false;
	}
}


static PollProcedure* pollProcedureList = NULL;

void NET_Poll(void)
{
	PollProcedure* pp;

	SetNetTime();

	for (pp = pollProcedureList; pp; pp = pp->next)
	{
		if (pp->nextTime > net_time)
		{
			break;
		}
		pollProcedureList = pp->next;
		pp->procedure();
	}
}


void SchedulePollProcedure(PollProcedure* proc, double timeOffset)
{
	PollProcedure* pp, * prev;

	proc->nextTime = Sys_DoubleTime() + timeOffset;
	for (pp = pollProcedureList, prev = NULL; pp; pp = pp->next)
	{
		if (pp->nextTime >= proc->nextTime)
		{
			break;
		}
		prev = pp;
	}

	if (prev == NULL)
	{
		proc->next = pollProcedureList;
		pollProcedureList = proc;
		return;
	}

	proc->next = pp;
	prev->next = proc;
}


#ifdef IDGODS
#define IDNET   0xc0f62800

qboolean IsID(netadr_t* addr)
{
	if (idgods->value == 0.0)
	{
		return false;
	}

	if (addr->type != NA_IP)
	{
		return false;
	}

	if ((BigLong(*(int*)addr->ip) & 0xffffff00) == IDNET)
	{
		return true;
	}
	return false;
}
#endif
