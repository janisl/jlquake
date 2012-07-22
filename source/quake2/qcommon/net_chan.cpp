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

#include "qcommon.h"

/*

packet header
-------------
31	sequence
1	does this message contain a reliable payload
31	acknowledge sequence
1	acknowledge receipt of even/odd message
16	qport

The remote connection never knows if it missed a reliable message, the
local side detects that it has been dropped by seeing a sequence acknowledge
higher thatn the last reliable sequence, but without the correct evon/odd
bit for the reliable set.

If the sender notices that a reliable message has been dropped, it will be
retransmitted.  It will not be retransmitted again until a message after
the retransmit has been acknowledged and the reliable still failed to get there.

if the sequence number is -1, the packet should be handled without a netcon

The reliable message can be added to at any time by doing
MSG_Write* (&netchan->message, <data>).

If the message buffer is overflowed, either by a single message, or by
multiple frames worth piling up while the last reliable transmit goes
unacknowledged, the netchan signals a fatal error.

Reliable messages are always placed first in a packet, then the unreliable
message is included if there is sufficient room.

To the receiver, there is no distinction between the reliable and unreliable
parts of the message, they are just processed out as a single larger message.

Illogical packet sequence numbers cause the packet to be dropped, but do
not kill the connection.  This, combined with the tight window of valid
reliable acknowledgement numbers provides protection against malicious
address spoofing.


The qport field is a workaround for bad address translating routers that
sometimes remap the client's source port on a packet during gameplay.

If the base part of the net address matches and the qport matches, then the
channel matches even if the IP port differs.  The IP port should be updated
to the new value before sending out any replies.


If there is no information that needs to be transfered on a given frame,
such as during the connection stage while waiting for the client to load,
then a packet only needs to be delivered if there is something in the
unacknowledged reliable
*/

netadr_t net_from;
QMsg net_message;
byte net_message_buffer[MAX_MSGLEN_Q2];

/*
===============
Netchan_Init

===============
*/
void Netchan_Init(void)
{
	int port;

	// pick a port value that should be nice and random
	port = Sys_Milliseconds_() & 0xffff;

	showpackets = Cvar_Get("showpackets", "0", 0);
	showdrop = Cvar_Get("showdrop", "0", 0);
	qport = Cvar_Get("qport", va("%i", port), CVAR_INIT);
}

/*
==============
Netchan_Setup

called to open a channel to a remote system
==============
*/
void Netchan_Setup(netsrc_t sock, netchan_t* chan, netadr_t adr, int qport)
{
	Com_Memset(chan, 0, sizeof(*chan));

	chan->sock = sock;
	chan->remoteAddress = adr;
	chan->qport = qport;
	chan->lastReceived = curtime;
	chan->incomingSequence = 0;
	chan->outgoingSequence = 1;

	chan->message.InitOOB(chan->messageBuffer, MAX_MSGLEN_Q2 - 16);	// leave space for header
	chan->message.allowoverflow = true;
}

static Cvar* noudp;

/*
====================
NET_OpenIP
====================
*/
static void NET_OpenIP()
{
	Cvar* ip = Cvar_Get("ip", "localhost", CVAR_INIT);

	int dedicated = Cvar_VariableValue("dedicated");

	if (!ip_sockets[NS_SERVER])
	{
		int port = Cvar_Get("ip_hostport", "0", CVAR_INIT)->integer;
		if (!port)
		{
			port = Cvar_Get("hostport", "0", CVAR_INIT)->integer;
			if (!port)
			{
				port = Cvar_Get("port", va("%i", PORT_SERVER), CVAR_INIT)->integer;
			}
		}
		ip_sockets[NS_SERVER] = SOCK_Open(ip->string, port);
		if (!ip_sockets[NS_SERVER] && dedicated)
		{
			Com_Error(ERR_FATAL, "Couldn't allocate dedicated server IP port");
		}
	}

	// dedicated servers don't need client ports
	if (dedicated)
	{
		return;
	}

	if (!ip_sockets[NS_CLIENT])
	{
		int port = Cvar_Get("ip_clientport", "0", CVAR_INIT)->integer;
		if (!port)
		{
			port = Cvar_Get("clientport", va("%i", PORT_CLIENT), CVAR_INIT)->integer;
			if (!port)
			{
				port = PORT_ANY;
			}
		}
		ip_sockets[NS_CLIENT] = SOCK_Open(ip->string, port);
		if (!ip_sockets[NS_CLIENT])
		{
			ip_sockets[NS_CLIENT] = SOCK_Open(ip->string, PORT_ANY);
		}
	}
}

/*
====================
NET_Config

A single player game will only use the loopback code
====================
*/
void    NET_Config(qboolean multiplayer)
{
	int i;
	static qboolean old_config;

	if (old_config == multiplayer)
	{
		return;
	}

	old_config = multiplayer;

	if (!multiplayer)
	{	// shut down any existing sockets
		for (i = 0; i < 2; i++)
		{
			if (ip_sockets[i])
			{
				SOCK_Close(ip_sockets[i]);
				ip_sockets[i] = 0;
			}
		}
	}
	else
	{	// open sockets
		if (!noudp->value)
		{
			NET_OpenIP();
		}
	}
}

//===================================================================

/*
====================
NET_Init
====================
*/
void NET_Init()
{
	if (!SOCK_Init())
	{
		Com_Error(ERR_FATAL,"Sockets initialization failed.");
	}

	noudp = Cvar_Get("noudp", "0", CVAR_INIT);
}

/*
====================
NET_Shutdown
====================
*/
void NET_Shutdown()
{
	NET_Config(false);	// close sockets

	SOCK_Shutdown();
}

//=============================================================================

// sleeps msec or until net socket is ready
void NET_Sleep(int msec)
{
	if (!com_dedicated || !com_dedicated->value)
	{
		return;	// we're not a server, just run full speed
	}

	SOCK_Sleep(ip_sockets[NS_SERVER], msec);
}
