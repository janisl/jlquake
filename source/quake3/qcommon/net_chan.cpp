/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "../game/q_shared.h"
#include "qcommon.h"

/*

packet header
-------------
4	outgoing sequence.  high bit will be set if this is a fragmented message
[2	qport (only for client to server)]
[2	fragment start byte]
[2	fragment length. if < FRAGMENT_SIZE, this is the last fragment]

if the sequence number is -1, the packet should be handled as an out-of-band
message instead of as part of a netcon.

All fragments will have the same sequence numbers.

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
void Netchan_Init(int port)
{
	port &= 0xffff;
	showpackets = Cvar_Get("showpackets", "0", CVAR_TEMP);
	showdrop = Cvar_Get("showdrop", "0", CVAR_TEMP);
	qport = Cvar_Get("net_qport", va("%i", port), CVAR_INIT);
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
	chan->incomingSequence = 0;
	chan->outgoingSequence = 1;
}

/*
=================
Netchan_Process

Returns qfalse if the message should not be processed due to being
out of order or a fragment.

Msg must be large enough to hold MAX_MSGLEN_Q3, because if this is the
final fragment of a multi-part message, the entire thing will be
copied out.
=================
*/
qboolean Netchan_Process(netchan_t* chan, QMsg* msg)
{
	int sequence;
	int qport;
	int fragmentStart, fragmentLength;
	qboolean fragmented;

	// XOR unscramble all data in the packet after the header
//	Netchan_UnScramblePacket( msg );

	// get sequence numbers
	msg->BeginReadingOOB();
	sequence = msg->ReadLong();

	// check for fragment information
	if (sequence & FRAGMENT_BIT)
	{
		sequence &= ~FRAGMENT_BIT;
		fragmented = qtrue;
	}
	else
	{
		fragmented = qfalse;
	}

	// read the qport if we are a server
	if (chan->sock == NS_SERVER)
	{
		qport = msg->ReadShort();
	}

	// read the fragment information
	if (fragmented)
	{
		fragmentStart = msg->ReadShort();
		fragmentLength = msg->ReadShort();
	}
	else
	{
		fragmentStart = 0;		// stop warning message
		fragmentLength = 0;
	}

	if (showpackets->integer)
	{
		if (fragmented)
		{
			Com_Printf("%s recv %4i : s=%i fragment=%i,%i\n",
				netsrcString[chan->sock],
				msg->cursize,
				sequence,
				fragmentStart, fragmentLength);
		}
		else
		{
			Com_Printf("%s recv %4i : s=%i\n",
				netsrcString[chan->sock],
				msg->cursize,
				sequence);
		}
	}

	//
	// discard out of order or duplicated packets
	//
	if (sequence <= chan->incomingSequence)
	{
		if (showdrop->integer || showpackets->integer)
		{
			Com_Printf("%s:Out of order packet %i at %i\n",
				SOCK_AdrToString(chan->remoteAddress),
				sequence,
				chan->incomingSequence);
		}
		return qfalse;
	}

	//
	// dropped packets don't keep the message from being used
	//
	chan->dropped = sequence - (chan->incomingSequence + 1);
	if (chan->dropped > 0)
	{
		if (showdrop->integer || showpackets->integer)
		{
			Com_Printf("%s:Dropped %i packets at %i\n",
				SOCK_AdrToString(chan->remoteAddress),
				chan->dropped,
				sequence);
		}
	}


	//
	// if this is the final framgent of a reliable message,
	// bump incoming_reliable_sequence
	//
	if (fragmented)
	{
		// TTimo
		// make sure we add the fragments in correct order
		// either a packet was dropped, or we received this one too soon
		// we don't reconstruct the fragments. we will wait till this fragment gets to us again
		// (NOTE: we could probably try to rebuild by out of order chunks if needed)
		if (sequence != chan->fragmentSequence)
		{
			chan->fragmentSequence = sequence;
			chan->fragmentLength = 0;
		}

		// if we missed a fragment, dump the message
		if (fragmentStart != chan->fragmentLength)
		{
			if (showdrop->integer || showpackets->integer)
			{
				Com_Printf("%s:Dropped a message fragment\n",
					SOCK_AdrToString(chan->remoteAddress),
					sequence);
			}
			// we can still keep the part that we have so far,
			// so we don't need to clear chan->fragmentLength
			return qfalse;
		}

		// copy the fragment to the fragment buffer
		if (fragmentLength < 0 || msg->readcount + fragmentLength > msg->cursize ||
			chan->fragmentLength + fragmentLength > MAX_MSGLEN_Q3)
		{
			if (showdrop->integer || showpackets->integer)
			{
				Com_Printf("%s:illegal fragment length\n",
					SOCK_AdrToString(chan->remoteAddress));
			}
			return qfalse;
		}

		Com_Memcpy(chan->fragmentBuffer + chan->fragmentLength,
			msg->_data + msg->readcount, fragmentLength);

		chan->fragmentLength += fragmentLength;

		// if this wasn't the last fragment, don't process anything
		if (fragmentLength == FRAGMENT_SIZE)
		{
			return qfalse;
		}

		if (chan->fragmentLength > msg->maxsize)
		{
			Com_Printf("%s:fragmentLength %i > msg->maxsize\n",
				SOCK_AdrToString(chan->remoteAddress),
				chan->fragmentLength);
			return qfalse;
		}

		// copy the full message over the partial fragment

		// make sure the sequence number is still there
		*(int*)msg->_data = LittleLong(sequence);

		Com_Memcpy(msg->_data + 4, chan->fragmentBuffer, chan->fragmentLength);
		msg->cursize = chan->fragmentLength + 4;
		chan->fragmentLength = 0;
		msg->readcount = 4;	// past the sequence number
		msg->bit = 32;	// past the sequence number

		// TTimo
		// clients were not acking fragmented messages
		chan->incomingSequence = sequence;

		return qtrue;
	}

	//
	// the message can now be read from the current message pointer
	//
	chan->incomingSequence = sequence;

	return qtrue;
}

//=============================================================================

static qboolean networkingEnabled = qfalse;

static Cvar* net_noudp;

/*
====================
NET_OpenIP
====================
*/
static void NET_OpenIP()
{
	Cvar* ip = Cvar_Get("net_ip", "localhost", CVAR_LATCH2);
	int port = Cvar_Get("net_port", va("%i", PORT_SERVER), CVAR_LATCH2)->integer;

	// automatically scan for a valid port, so multiple
	// dedicated servers can be started without requiring
	// a different net_port for each one
	for (int i = 0; i < 10; i++)
	{
		ip_sockets[0] = SOCK_Open(ip->string, port + i);
		if (ip_sockets[0])
		{
			ip_sockets[1] = ip_sockets[0];
			Cvar_SetValue("net_port", port + i);
			if (net_socksEnabled->integer)
			{
				SOCK_OpenSocks(port + i);
			}
			SOCK_GetLocalAddress();
			return;
		}
	}
	Com_Printf("WARNING: Couldn't allocate IP port\n");
}

//===================================================================


/*
====================
NET_GetCvars
====================
*/
static bool NET_GetCvars()
{
	bool modified = false;

	if (net_noudp && net_noudp->modified)
	{
		modified = true;
	}
	net_noudp = Cvar_Get("net_noudp", "0", CVAR_LATCH2 | CVAR_ARCHIVE);

	if (SOCK_GetSocksCvars())
	{
		modified = true;
	}

	return modified;
}

/*
====================
NET_Config
====================
*/
void NET_Config(bool enableNetworking)
{
	bool modified;
	bool stop;
	bool start;

	// get any latched changes to cvars
	modified = NET_GetCvars();

	if (net_noudp->integer)
	{
		enableNetworking = false;
	}

	// if enable state is the same and no cvars were modified, we have nothing to do
	if (enableNetworking == networkingEnabled && !modified)
	{
		return;
	}

	if (enableNetworking == networkingEnabled)
	{
		if (enableNetworking)
		{
			stop = true;
			start = true;
		}
		else
		{
			stop = false;
			start = false;
		}
	}
	else
	{
		if (enableNetworking)
		{
			stop = false;
			start = true;
		}
		else
		{
			stop = true;
			start = false;
		}
		networkingEnabled = enableNetworking;
	}

	if (stop)
	{
		if (ip_sockets[0])
		{
			SOCK_Close(ip_sockets[0]);
			ip_sockets[0] = 0;
		}

		SOCK_CloseSocks();
	}

	if (start)
	{
		NET_OpenIP();
	}
}

/*
====================
NET_Init
====================
*/
void NET_Init()
{
	if (!SOCK_Init())
	{
		return;
	}

	// this is really just to get the cvars registered
	NET_GetCvars();

	//FIXME testing!
	NET_Config(true);
}

/*
====================
NET_Shutdown
====================
*/
void NET_Shutdown()
{
	NET_Config(false);
}

/*
====================
NET_Restart_f
====================
*/
void NET_Restart()
{
	NET_Config(networkingEnabled);
}

// sleeps msec or until net socket is ready
//JL: Note on Windows this  method was empty.
void NET_Sleep(int msec)
{
	if (!com_dedicated->integer)
	{
		return;	// we're not a server, just run full speed
	}

	SOCK_Sleep(ip_sockets[0], msec);
}
