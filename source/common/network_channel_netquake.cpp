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

int Loop_Init()
{
	if (com_dedicated->integer)
	{
		return -1;
	}
	return 0;
}

void Loop_Shutdown()
{
}

void Loop_Listen(bool state)
{
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

int Loop_GetMessage(qsocket_t* sock, netchan_t* chan, QMsg* message)
{
	netadr_t net_from;
	int ret = NET_GetLoopPacket(chan->sock, &net_from, message);

	if (ret == 1)
	{
		loopbacks[chan->sock ^ 1].canSend = true;
	}

	return ret;
}

int Loop_SendMessage(qsocket_t* sock, netchan_t* chan, QMsg* data)
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


int Loop_SendUnreliableMessage(qsocket_t* sock, netchan_t* chan, QMsg* data)
{
	loopback_t* loopback = &loopbacks[chan->sock ^ 1];
	if (loopback->send - loopback->get >= MAX_LOOPBACK)
	{
		return 0;
	}

	NET_SendLoopPacket(chan->sock, data->cursize, data->_data, 2);
	return 1;
}

bool Loop_CanSendMessage(qsocket_t* sock, netchan_t* chan)
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
