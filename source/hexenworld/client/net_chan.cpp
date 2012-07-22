
#include "quakedef.h"
#ifdef _WIN32
#include <windows.h>
#endif

#define PACKET_HEADER   8

/*

packet header
-------------
31	sequence
1	does this message contain a reliable payload
31	acknowledge sequence
1	acknowledge receipt of even/odd message

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

*/

Cvar* showdrop;

/*
===============
Netchan_Init

===============
*/
void Netchan_Init(void)
{
	showpackets = Cvar_Get("showpackets", "0", 0);
	showdrop = Cvar_Get("showdrop", "0", 0);
}

/*
==============
Netchan_Setup

called to open a channel to a remote system
==============
*/
void Netchan_Setup(netsrc_t sock, netchan_t* chan, netadr_t adr)
{
	Com_Memset(chan, 0, sizeof(*chan));

	chan->sock = sock;
	chan->remoteAddress = adr;
	chan->lastReceived = realtime * 1000;

	chan->message.InitOOB(chan->messageBuffer, MAX_MSGLEN_HW);
	chan->message.allowoverflow = true;

	chan->rate = 1.0 / 2500;
}


/*
===============
Netchan_CanPacket

Returns true if the bandwidth choke isn't active
================
*/
#define MAX_BACKUP  200
qboolean Netchan_CanPacket(netchan_t* chan)
{
	if (chan->clearTime < realtime + MAX_BACKUP * chan->rate)
	{
		return true;
	}
	return false;
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
	return Netchan_CanPacket(chan);
}


/*
===============
Netchan_Transmit_

tries to send an unreliable message to a connection, and handles the
transmition / retransmition of the reliable messages.

A 0 length will still generate a packet and deal with the reliable messages.
================
*/
void Netchan_Transmit_(netchan_t* chan, int length, byte* data)
{
	Netchan_Transmit(chan, length, data);

	if (chan->clearTime < realtime)
	{
		chan->clearTime = realtime + (length + chan->reliableOrUnsentLength) * chan->rate;
	}
	else
	{
		chan->clearTime += (length + chan->reliableOrUnsentLength) * chan->rate;
	}
}

int packet_latency[256];

/*
=================
Netchan_Process

called when the current net_message is from remote_address
modifies net_message so that it points to the packet payload
=================
*/
qboolean Netchan_Process(netchan_t* chan)
{
	int sequence, sequence_ack;
	int reliable_ack, reliable_message;

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

	reliable_message = (unsigned)sequence >> 31;
	reliable_ack = (unsigned)sequence_ack >> 31;

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
	if (sequence <= chan->incomingSequence)
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
	if (reliable_ack == chan->outgoingReliableSequence)
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

#define MAX_UDP_PACKET  (MAX_MSGLEN_HW + 9)		// one more than msg + header
static byte net_message_buffer[MAX_UDP_PACKET];

//=============================================================================

static int UDP_OpenSocket(int port)
{
	int newsocket = SOCK_Open(NULL, port);
	if (newsocket == 0)
	{
		Sys_Error("UDP_OpenSocket: failed");
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
	HuffInit();

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

#ifdef _DEBUG
	PrintFreqs();
#endif
}
