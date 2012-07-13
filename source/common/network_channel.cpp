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

loopback_t loopbacks[2];

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
