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
// net_loop.c

#include "quakedef.h"
#include "net_loop.h"

qboolean localconnectpending = false;
qsocket_t* loop_client = NULL;
qsocket_t* loop_server = NULL;

int Loop_Init(void)
{
	if (cls.state == CA_DEDICATED)
	{
		return -1;
	}
	return 0;
}


void Loop_Shutdown(void)
{
}


void Loop_Listen(qboolean state)
{
}


void Loop_SearchForHosts(qboolean xmit)
{
	if (sv.state == SS_DEAD)
	{
		return;
	}

	hostCacheCount = 1;
	if (String::Cmp(hostname->string, "UNNAMED") == 0)
	{
		String::Cpy(hostcache[0].name, "local");
	}
	else
	{
		String::Cpy(hostcache[0].name, hostname->string);
	}
	String::Cpy(hostcache[0].map, sv.name);
	hostcache[0].users = net_activeconnections;
	hostcache[0].maxusers = svs.qh_maxclients;
	hostcache[0].driver = net_driverlevel;
	String::Cpy(hostcache[0].cname, "local");
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
			Con_Printf("Loop_Connect: no qsocket available\n");
			return NULL;
		}
		String::Cpy(loop_client->address, "localhost");
	}
	loopbacks[0].send = 0;
	loopbacks[0].get = 0;
	loop_client->canSend = true;

	if (!loop_server)
	{
		if ((loop_server = NET_NewQSocket()) == NULL)
		{
			Con_Printf("Loop_Connect: no qsocket available\n");
			return NULL;
		}
		String::Cpy(loop_server->address, "LOCAL");
	}
	loopbacks[1].send = 0;
	loopbacks[1].get = 0;
	loop_server->canSend = true;

	loop_client->driverdata = (void*)loop_server;
	loop_server->driverdata = (void*)loop_client;

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
	loop_server->canSend = true;
	loopbacks[0].send = 0;
	loopbacks[0].get = 0;
	loop_client->canSend = true;
	return loop_server;
}

int Loop_GetMessage(qsocket_t* sock, netchan_t* chan)
{
	netadr_t net_from;
	int ret = NET_GetLoopPacket(chan->sock, &net_from, &net_message);

	if (sock->driverdata && ret == 1)
	{
		((qsocket_t*)sock->driverdata)->canSend = true;
	}

	return ret;
}

int Loop_SendMessage(qsocket_t* sock, netchan_t* chan, QMsg* data)
{
	if (!sock->driverdata)
	{
		return -1;
	}

	loopback_t* loopback = &loopbacks[chan->sock ^ 1];
	if (loopback->send - loopback->get >= MAX_LOOPBACK)
	{
		Sys_Error("Loop_SendMessage: overflow\n");
	}

	NET_SendLoopPacket(chan->sock, data->cursize, data->_data, 1);

	sock->canSend = false;
	return 1;
}


int Loop_SendUnreliableMessage(qsocket_t* sock, netchan_t* chan, QMsg* data)
{
	if (!sock->driverdata)
	{
		return -1;
	}

	loopback_t* loopback = &loopbacks[chan->sock ^ 1];
	if (loopback->send - loopback->get >= MAX_LOOPBACK)
	{
		return 0;
	}

	NET_SendLoopPacket(chan->sock, data->cursize, data->_data, 2);
	return 1;
}


qboolean Loop_CanSendMessage(qsocket_t* sock, netchan_t* chan)
{
	if (!sock->driverdata)
	{
		return false;
	}
	return sock->canSend;
}


qboolean Loop_CanSendUnreliableMessage(qsocket_t* sock)
{
	return true;
}


void Loop_Close(qsocket_t* sock, netchan_t* chan)
{
	if (sock->driverdata)
	{
		((qsocket_t*)sock->driverdata)->driverdata = NULL;
	}
	loopbacks[chan->sock].send = 0;
	loopbacks[chan->sock].get = 0;
	sock->canSend = true;
	if (sock == loop_client)
	{
		loop_client = NULL;
	}
	else
	{
		loop_server = NULL;
	}
}
