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

Cvar* sv_hostname;

loopback_t loopbacks[2];

unsigned char huffbuff[65536];

int ip_sockets[2];

int NET_GetLoopPacket(netsrc_t sock, netadr_t* net_from, QMsg* net_message)
{
	loopback_t* loop = &loopbacks[sock];

	if (loop->send - loop->get > MAX_LOOPBACK)
	{
		loop->get = loop->send - MAX_LOOPBACK;
	}

	if (loop->get >= loop->send)
	{
		return false;
	}

	int i = loop->get & (MAX_LOOPBACK - 1);
	loop->get++;

	Com_Memcpy(net_message->_data, loop->msgs[i].data, loop->msgs[i].datalen);
	net_message->cursize = loop->msgs[i].datalen;
	Com_Memset(net_from, 0, sizeof(*net_from));
	net_from->type = NA_LOOPBACK;
	return loop->msgs[i].type;
}

void NET_SendLoopPacket(netsrc_t sock, int length, const void* data, int type)
{
	loopback_t* loop = &loopbacks[sock ^ 1];

	int i = loop->send & (MAX_LOOPBACK - 1);
	loop->send++;

	Com_Memcpy(loop->msgs[i].data, data, length);
	loop->msgs[i].datalen = length;
	loop->msgs[i].type = type;
}

bool NET_GetUdpPacket(netsrc_t sock, netadr_t* net_from, QMsg* net_message)
{
	int net_socket = ip_sockets[sock];

	if (!net_socket)
	{
		return false;
	}

	int ret = SOCK_Recv(net_socket, net_message->_data, net_message->maxsize, net_from);
	if (ret == SOCKRECV_NO_DATA)
	{
		return false;
	}
	if (ret == SOCKRECV_ERROR)
	{
		if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld) ||
			(GGameType & GAME_Quake2 && !com_dedicated->value))	// let dedicated servers continue after errors
		{
			common->Error("NET_GetPacket failed");
		}
		return false;
	}

	if (ret == net_message->maxsize)
	{
		common->Printf("Oversize packet from %s\n", SOCK_AdrToString(*net_from));
		return false;
	}

	if (GGameType & GAME_HexenWorld)
	{
		Com_Memcpy(huffbuff, net_message->_data, ret);
		HuffDecode(huffbuff, net_message->_data, ret, &ret);
	}

	net_message->cursize = ret;
	net_message->readcount = 0;
	return true;
}

bool NET_GetPacket(netsrc_t sock, netadr_t* net_from, QMsg* net_message)
{
	if (NET_GetLoopPacket(sock, net_from, net_message))
	{
		return true;
	}

	return NET_GetUdpPacket(sock, net_from, net_message);
}
