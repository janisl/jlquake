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

#include "quakedef.h"

#ifdef _WIN32
#include "../../client/windows_shared.h"
#else
#include <unistd.h>
#endif

#define PACKET_HEADER   8

/*

packet header
-------------
31	sequence
1	does this message contain a reliable payload
31	acknowledge sequence
1	acknowledge receipt of even/odd message
16  qport

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

Reliable messages are allways placed first in a packet, then the unreliable
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


*/

Cvar* showdrop;

/*
===============
Netchan_Init

===============
*/
void Netchan_Init(void)
{
	int port;

	// pick a port value that should be nice and random
#ifdef _WIN32
	port = ((int)(timeGetTime() * 1000) * time(NULL)) & 0xffff;
#else
	port = ((int)(getpid() + getuid() * 1000) * time(NULL)) & 0xffff;
#endif

	showpackets = Cvar_Get("showpackets", "0", 0);
	showdrop = Cvar_Get("showdrop", "0", 0);
	qport = Cvar_Get("qport", "0", 0);
	Cvar_SetValue("qport", port);
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

	chan->remoteAddress = adr;
	chan->lastReceived = realtime * 1000;

	chan->message.InitOOB(chan->messageBuffer, MAX_MSGLEN_QW);
	chan->message.allowoverflow = true;

	chan->qport = qport;
}

/*
===============
Netchan_CanReliable

Returns true if the bandwidth choke isn't
================
*/
qboolean Netchan_CanReliable(netchan_t* chan)
{
	if (chan->reliableOrUnsentLength)
	{
		return false;			// waiting for ack
	}
	return true;
}

#ifdef SERVERONLY
qboolean ServerPaused(void);
#endif

/*
=================
Netchan_Process

called when the current net_message is from remote_address
modifies net_message so that it points to the packet payload
=================
*/
qboolean Netchan_Process(netchan_t* chan)
{
	unsigned sequence, sequence_ack;
	unsigned reliable_ack, reliable_message;
#ifdef SERVERONLY
	int qport;
#endif

	if (
#ifndef SERVERONLY
		!clc.demoplaying &&
#endif
		!SOCK_CompareAdr(net_from, chan->remoteAddress))
	{
		return false;
	}

// get sequence numbers
	net_message.BeginReadingOOB();
	sequence = net_message.ReadLong();
	sequence_ack = net_message.ReadLong();

	// read the qport if we are a server
#ifdef SERVERONLY
	qport = net_message.ReadShort();
#endif

	reliable_message = sequence >> 31;
	reliable_ack = sequence_ack >> 31;

	sequence &= ~(1 << 31);
	sequence_ack &= ~(1 << 31);

	if (showpackets->value)
	{
		Con_Printf("<-- s=%i(%i) a=%i(%i) %i\n",
			sequence,
			reliable_message,
			sequence_ack,
			reliable_ack,
			net_message.cursize);
	}

// get a rate estimation
#if 0
	if (chan->outgoing_sequence - sequence_ack < MAX_LATENT)
	{
		int i;
		double time, rate;

		i = sequence_ack & (MAX_LATENT - 1);
		time = realtime - chan->outgoing_time[i];
		time -= 0.1;	// subtract 100 ms
		if (time <= 0)
		{	// gotta be a digital link for <100 ms ping
			if (chan->rate > 1.0 / 5000)
			{
				chan->rate = 1.0 / 5000;
			}
		}
		else
		{
			if (chan->outgoing_size[i] < 512)
			{	// only deal with small messages
				rate = chan->outgoing_size[i] / time;
				if (rate > 5000)
				{
					rate = 5000;
				}
				rate = 1.0 / rate;
				if (chan->rate > rate)
				{
					chan->rate = rate;
				}
			}
		}
	}
#endif

//
// discard stale or duplicated packets
//
	if (sequence <= (unsigned)chan->incomingSequence)
	{
		if (showdrop->value)
		{
			Con_Printf("%s:Out of order packet %i at %i\n",
				SOCK_AdrToString(chan->remoteAddress),
				sequence,
				chan->incomingSequence);
		}
		return false;
	}

//
// dropped packets don't keep the message from being used
//
	chan->dropped = sequence - (chan->incomingSequence + 1);
	if (chan->dropped > 0)
	{
		chan->dropCount += 1;

		if (showdrop->value)
		{
			Con_Printf("%s:Dropped %i packets at %i\n",
				SOCK_AdrToString(chan->remoteAddress),
				sequence - (chan->incomingSequence + 1),
				sequence);
		}
	}

//
// if the current outgoing reliable message has been acknowledged
// clear the buffer to make way for the next
//
	if (reliable_ack == (unsigned)chan->outgoingReliableSequence)
	{
		chan->reliableOrUnsentLength = 0;	// it has been received

	}
//
// if this message contains a reliable message, bump incoming_reliable_sequence
//
	chan->incomingSequence = sequence;
	chan->incomingAcknowledged = sequence_ack;
	chan->incomingReliableAcknowledged = reliable_ack;
	if (reliable_message)
	{
		chan->incomingReliableSequence ^= 1;
	}

//
// the message can now be read from the current message pointer
// update statistics counters
//
	chan->frameRate = chan->frameRate * OLD_AVG
					  + (realtime - chan->lastReceived / 1000.0) * (1.0 - OLD_AVG);

	chan->lastReceived = realtime * 1000;

	return true;
}


//=============================================================================

netadr_t net_from;
QMsg net_message;

#define MAX_UDP_PACKET  (MAX_MSGLEN_QW * 2)		// one more than msg + header
byte net_message_buffer[MAX_UDP_PACKET];

//=============================================================================

static int UDP_OpenSocket(int port)
{
	int newsocket;
	int i;

	const char* net_interface = NULL;
	//ZOID -- check for interface binding option
	if ((i = COM_CheckParm("-ip")) != 0 && i < COM_Argc())
	{
		net_interface = COM_Argv(i + 1);
	}
	newsocket = SOCK_Open(net_interface, port);
	if (newsocket == 0)
	{
		Sys_Error("UDP_OpenSocket: socket failed");
	}
	return newsocket;
}

/*
====================
NET_Init
====================
*/
void NET_Init(int port)
{
	if (!SOCK_Init())
	{
		Sys_Error("Sockets initialization failed.");
	}

	//
	// open the single socket to be used for all communications
	//
	ip_sockets[0] = UDP_OpenSocket(port);
	ip_sockets[1] = ip_sockets[0];

	//
	// init the message buffer
	//
	net_message.InitOOB(net_message_buffer, sizeof(net_message_buffer));

	//
	// determine my addresses
	//
	SOCK_GetLocalAddress();
}

/*
====================
NET_Shutdown
====================
*/
void    NET_Shutdown(void)
{
	SOCK_Close(ip_sockets[0]);
	SOCK_Shutdown();
}



