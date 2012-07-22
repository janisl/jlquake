
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
}

int packet_latency[256];

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
