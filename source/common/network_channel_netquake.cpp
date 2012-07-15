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

#include "qcommon.h"

int net_hostport;

qsocket_t* net_activeSockets = NULL;
qsocket_t* net_freeSockets = NULL;
int net_numsockets = 0;

double net_time;

int net_activeconnections = 0;
int hostCacheCount = 0;
hostcache_t hostcache[HOSTCACHESIZE];

bool localconnectpending = false;
qsocket_t* loop_client = NULL;
qsocket_t* loop_server = NULL;

bool tcpipAvailable = false;

static const char* net_interface;
int net_controlsocket;
int net_acceptsocket = -1;		// socket for fielding new connections

//	Called by drivers when a new communications endpoint is required
// The sequence and buffer fields will be filled in properly
qsocket_t* NET_NewQSocket()
{
	qsocket_t* sock;

	if (net_freeSockets == NULL)
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

qsocket_t* Loop_Connect(const char* host, netchan_t* chan)
{
	if (String::Cmp(host,"local") != 0)
	{
		return NULL;
	}

	localconnectpending = true;

	if (!loop_client)
	{
		if ((loop_client = NET_NewQSocket()) == NULL)
		{
			common->Printf("Loop_Connect: no qsocket available\n");
			return NULL;
		}
		String::Cpy(loop_client->address, "localhost");
	}
	loopbacks[0].send = 0;
	loopbacks[0].get = 0;
	loopbacks[0].canSend = true;

	if (!loop_server)
	{
		if ((loop_server = NET_NewQSocket()) == NULL)
		{
			common->Printf("Loop_Connect: no qsocket available\n");
			return NULL;
		}
		String::Cpy(loop_server->address, "LOCAL");
	}
	loopbacks[1].send = 0;
	loopbacks[1].get = 0;
	loopbacks[1].canSend = true;

	chan->remoteAddress.type = NA_LOOPBACK;
	return loop_client;
}

qsocket_t* Loop_CheckNewConnections(netadr_t* outaddr)
{
	if (!localconnectpending)
	{
		return NULL;
	}

	localconnectpending = false;
	loopbacks[1].send = 0;
	loopbacks[1].get = 0;
	loopbacks[1].canSend = true;
	loopbacks[0].send = 0;
	loopbacks[0].get = 0;
	loopbacks[0].canSend = true;
	outaddr->type = NA_LOOPBACK;
	return loop_server;
}

int Loop_GetMessage(netchan_t* chan, QMsg* message)
{
	netadr_t net_from;
	int ret = NET_GetLoopPacket(chan->sock, &net_from, message);

	if (ret == 1)
	{
		loopbacks[chan->sock ^ 1].canSend = true;
	}

	return ret;
}

int Loop_SendMessage(netchan_t* chan, QMsg* data)
{
	loopback_t* loopback = &loopbacks[chan->sock ^ 1];
	if (loopback->send - loopback->get >= MAX_LOOPBACK)
	{
		common->Error("Loop_SendMessage: overflow\n");
	}

	NET_SendLoopPacket(chan->sock, data->cursize, data->_data, 1);

	loopbacks[chan->sock].canSend = false;
	return 1;
}

int Loop_SendUnreliableMessage(netchan_t* chan, QMsg* data)
{
	loopback_t* loopback = &loopbacks[chan->sock ^ 1];
	if (loopback->send - loopback->get >= MAX_LOOPBACK)
	{
		return 0;
	}

	NET_SendLoopPacket(chan->sock, data->cursize, data->_data, 2);
	return 1;
}

bool Loop_CanSendMessage(netchan_t* chan)
{
	return loopbacks[chan->sock].canSend;
}

void Loop_Close(qsocket_t* sock, netchan_t* chan)
{
	loopbacks[chan->sock].send = 0;
	loopbacks[chan->sock].get = 0;
	loopbacks[chan->sock].canSend = true;
	if (sock == loop_client)
	{
		loop_client = NULL;
	}
	else
	{
		loop_server = NULL;
	}
}

int UDPNQ_OpenSocket(int port)
{
	int newsocket = SOCK_Open(net_interface, port);
	if (newsocket == 0)
	{
		return -1;
	}
	return newsocket;
}

int UDP_Init()
{
	if (COM_CheckParm("-noudp"))
	{
		return -1;
	}

	if (!SOCK_Init())
	{
		return -1;
	}

	// determine my name & address
	SOCK_GetLocalAddress();

	int i = COM_CheckParm("-ip");
	if (i)
	{
		if (i < COM_Argc() - 1)
		{
			net_interface = COM_Argv(i + 1);
		}
		else
		{
			common->FatalError("NET_Init: you must specify an IP address after -ip");
		}
	}

	if ((net_controlsocket = UDPNQ_OpenSocket(PORT_ANY)) == -1)
	{
		common->Printf("UDP_Init: Unable to open control socket\n");
		SOCK_Shutdown();
		return -1;
	}

	tcpipAvailable = true;

	return net_controlsocket;
}

int UDP_CloseSocket(int socket)
{
	SOCK_Close(socket);
	return 0;
}

void UDP_Listen(bool state)
{
	// enable listening
	if (state)
	{
		if (net_acceptsocket != -1)
		{
			return;
		}
		if ((net_acceptsocket = UDPNQ_OpenSocket(net_hostport)) == -1)
		{
			common->FatalError("UDP_Listen: Unable to open accept socket\n");
		}
		return;
	}

	// disable listening
	if (net_acceptsocket == -1)
	{
		return;
	}
	UDP_CloseSocket(net_acceptsocket);
	net_acceptsocket = -1;
}

void UDP_Shutdown()
{
	UDP_Listen(false);
	UDP_CloseSocket(net_controlsocket);
	SOCK_Shutdown();
}

int UDP_Read(int socket, byte* buf, int len, netadr_t* addr)
{
	int ret = SOCK_Recv(socket, buf, len, addr);
	if (ret == SOCKRECV_NO_DATA)
	{
		return 0;
	}
	if (ret == SOCKRECV_ERROR)
	{
		return -1;
	}
	return ret;
}

int UDP_Write(int socket, byte* buf, int len, netadr_t* addr)
{
	int ret = SOCK_Send(socket, buf, len, addr);
	if (ret == SOCKSEND_WOULDBLOCK)
	{
		return 0;
	}
	return ret;
}

int UDP_Broadcast(int socket, byte* buf, int len)
{
	netadr_t to;
	Com_Memset(&to, 0, sizeof(to));
	to.type = NA_BROADCAST;
	to.port = BigShort(net_hostport);
	int ret = SOCK_Send(socket, buf, len, &to);
	if (ret == SOCKSEND_WOULDBLOCK)
	{
		return 0;
	}
	return ret;
}

int UDP_GetAddrFromName(const char* name, netadr_t* addr)
{
	if (!SOCK_StringToAdr(name, addr, 0))
	{
		return -1;
	}

	addr->port = BigShort(net_hostport);

	return 0;
}

int UDP_AddrCompare(netadr_t* addr1, netadr_t* addr2)
{
	if (SOCK_CompareAdr(*addr1, *addr2))
	{
		return 0;
	}

	if (SOCK_CompareBaseAdr(*addr1, *addr2))
	{
		return 1;
	}

	return -1;
}

int UDP_GetNameFromAddr(netadr_t* addr, char* name)
{
	const char* host = SOCK_GetHostByAddr(addr);
	if (host)
	{
		String::NCpy(name, host, NET_NAMELEN_Q1 - 1);
		return 0;
	}

	String::Cpy(name, SOCK_AdrToString(*addr));
	return 0;
}
