/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

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

// max size of a network packet
#define MAX_PACKETLEN           1400

#define FRAGMENT_SIZE           ( MAX_PACKETLEN - 100 )

// two ints and a short
#define PACKET_HEADER           10

#define FRAGMENT_BIT    ( 1 << 31 )

Cvar      *showpackets;
Cvar      *showdrop;
Cvar      *qport;

static bool networkingEnabled = false;

static Cvar* net_noudp;

static int ip_socket;

static char *netsrcString[2] = {
	"client",
	"server"
};

/*
===============
Netchan_Init

===============
*/
void Netchan_Init( int port ) {
	port &= 0xffff;
	showpackets = Cvar_Get( "showpackets", "0", CVAR_TEMP );
	showdrop = Cvar_Get( "showdrop", "0", CVAR_TEMP );
	qport = Cvar_Get( "net_qport", va( "%i", port ), CVAR_INIT );
}

/*
==============
Netchan_Setup

called to open a channel to a remote system
==============
*/
void Netchan_Setup( netsrc_t sock, netchan_t *chan, netadr_t adr, int qport ) {
	Com_Memset( chan, 0, sizeof( *chan ) );

	chan->sock = sock;
	chan->remoteAddress = adr;
	chan->qport = qport;
	chan->incomingSequence = 0;
	chan->outgoingSequence = 1;
}

/*
=================
Netchan_TransmitNextFragment

Send one fragment of the current message
=================
*/
void Netchan_TransmitNextFragment( netchan_t *chan ) {
	QMsg send;
	byte send_buf[MAX_PACKETLEN];
	int fragmentLength;

	// write the packet header
	MSG_InitOOB( &send, send_buf, sizeof( send_buf ) );                // <-- only do the oob here

	send.WriteLong( chan->outgoingSequence | FRAGMENT_BIT );

	// send the qport if we are a client
	if ( chan->sock == NS_CLIENT ) {
		send.WriteShort( qport->integer );
	}

	// copy the reliable message to the packet first
	fragmentLength = FRAGMENT_SIZE;
	if ( chan->unsentFragmentStart  + fragmentLength > chan->unsentLength ) {
		fragmentLength = chan->unsentLength - chan->unsentFragmentStart;
	}

	send.WriteShort( chan->unsentFragmentStart );
	send.WriteShort( fragmentLength );
	send.WriteData( chan->unsentBuffer + chan->unsentFragmentStart, fragmentLength );

	// send the datagram
	NET_SendPacket( chan->sock, send.cursize, send._data, chan->remoteAddress );

	if ( showpackets->integer ) {
		Com_Printf( "%s send %4i : s=%i fragment=%i,%i\n"
					, netsrcString[ chan->sock ]
					, send.cursize
					, chan->outgoingSequence
					, chan->unsentFragmentStart, fragmentLength );
	}

	chan->unsentFragmentStart += fragmentLength;

	// this exit condition is a little tricky, because a packet
	// that is exactly the fragment length still needs to send
	// a second packet of zero length so that the other side
	// can tell there aren't more to follow
	if ( chan->unsentFragmentStart == chan->unsentLength && fragmentLength != FRAGMENT_SIZE ) {
		chan->outgoingSequence++;
		chan->unsentFragments = qfalse;
	}
}


/*
===============
Netchan_Transmit

Sends a message to a connection, fragmenting if necessary
A 0 length will still generate a packet.
================
*/
void Netchan_Transmit( netchan_t *chan, int length, const byte *data ) {
	QMsg send;
	byte send_buf[MAX_PACKETLEN];

	if ( length > MAX_MSGLEN ) {
		Com_Error( ERR_DROP, "Netchan_Transmit: length = %i", length );
	}

	chan->unsentFragmentStart = 0;

	// fragment large reliable messages
	if ( length >= FRAGMENT_SIZE ) {
		chan->unsentFragments = qtrue;
		chan->unsentLength = length;
		Com_Memcpy( chan->unsentBuffer, data, length );

		// only send the first fragment now
		Netchan_TransmitNextFragment( chan );

		return;
	}

	// write the packet header
	MSG_InitOOB( &send, send_buf, sizeof( send_buf ) );

	send.WriteLong( chan->outgoingSequence );
	chan->outgoingSequence++;

	// send the qport if we are a client
	if ( chan->sock == NS_CLIENT ) {
		send.WriteShort( qport->integer );
	}

	send.WriteData( data, length );

	// send the datagram
	NET_SendPacket( chan->sock, send.cursize, send._data, chan->remoteAddress );

	if ( showpackets->integer ) {
		Com_Printf( "%s send %4i : s=%i ack=%i\n"
					, netsrcString[ chan->sock ]
					, send.cursize
					, chan->outgoingSequence - 1
					, chan->incomingSequence );
	}
}

/*
=================
Netchan_Process

Returns qfalse if the message should not be processed due to being
out of order or a fragment.

Msg must be large enough to hold MAX_MSGLEN, because if this is the
final fragment of a multi-part message, the entire thing will be
copied out.
=================
*/
qboolean Netchan_Process( netchan_t *chan, QMsg *msg ) {
	int sequence;
	int qport;
	int fragmentStart, fragmentLength;
	qboolean fragmented;

	// get sequence numbers
	msg->BeginReadingOOB();
	sequence = msg->ReadLong();

	// check for fragment information
	if ( sequence & FRAGMENT_BIT ) {
		sequence &= ~FRAGMENT_BIT;
		fragmented = qtrue;
	} else {
		fragmented = qfalse;
	}

	// read the qport if we are a server
	if ( chan->sock == NS_SERVER ) {
		qport = msg->ReadShort();
	}

	// read the fragment information
	if ( fragmented ) {
		fragmentStart = msg->ReadShort();
		fragmentLength = msg->ReadShort();
	} else {
		fragmentStart = 0;      // stop warning message
		fragmentLength = 0;
	}

	if ( showpackets->integer ) {
		if ( fragmented ) {
			Com_Printf( "%s recv %4i : s=%i fragment=%i,%i\n"
						, netsrcString[ chan->sock ]
						, msg->cursize
						, sequence
						, fragmentStart, fragmentLength );
		} else {
			Com_Printf( "%s recv %4i : s=%i\n"
						, netsrcString[ chan->sock ]
						, msg->cursize
						, sequence );
		}
	}

	//
	// discard out of order or duplicated packets
	//
	if ( sequence <= chan->incomingSequence ) {
		if ( showdrop->integer || showpackets->integer ) {
			Com_Printf( "%s:Out of order packet %i at %i\n"
						, SOCK_AdrToString( chan->remoteAddress )
						,  sequence
						, chan->incomingSequence );
		}
		return qfalse;
	}

	//
	// dropped packets don't keep the message from being used
	//
	chan->dropped = sequence - ( chan->incomingSequence + 1 );
	if ( chan->dropped > 0 ) {
		if ( showdrop->integer || showpackets->integer ) {
			Com_Printf( "%s:Dropped %i packets at %i\n"
						, SOCK_AdrToString( chan->remoteAddress )
						, chan->dropped
						, sequence );
		}
	}


	//
	// if this is the final framgent of a reliable message,
	// bump incoming_reliable_sequence
	//
	if ( fragmented ) {
		// TTimo
		// make sure we add the fragments in correct order
		// either a packet was dropped, or we received this one too soon
		// we don't reconstruct the fragments. we will wait till this fragment gets to us again
		// (NOTE: we could probably try to rebuild by out of order chunks if needed)
		if ( sequence != chan->fragmentSequence ) {
			chan->fragmentSequence = sequence;
			chan->fragmentLength = 0;
		}

		// if we missed a fragment, dump the message
		if ( fragmentStart != chan->fragmentLength ) {
			if ( showdrop->integer || showpackets->integer ) {
				Com_Printf( "%s:Dropped a message fragment, sequence %d\n"
							, SOCK_AdrToString( chan->remoteAddress )
							, sequence );
			}
			// we can still keep the part that we have so far,
			// so we don't need to clear chan->fragmentLength
			return qfalse;
		}

		// copy the fragment to the fragment buffer
		if ( fragmentLength < 0 || msg->readcount + fragmentLength > msg->cursize ||
			 chan->fragmentLength + fragmentLength > sizeof( chan->fragmentBuffer ) ) {
			if ( showdrop->integer || showpackets->integer ) {
				Com_Printf( "%s:illegal fragment length\n"
							, SOCK_AdrToString( chan->remoteAddress ) );
			}
			return qfalse;
		}

		Com_Memcpy( chan->fragmentBuffer + chan->fragmentLength,
					msg->_data + msg->readcount, fragmentLength );

		chan->fragmentLength += fragmentLength;

		// if this wasn't the last fragment, don't process anything
		if ( fragmentLength == FRAGMENT_SIZE ) {
			return qfalse;
		}

		if ( chan->fragmentLength > msg->maxsize ) {
			Com_Printf( "%s:fragmentLength %i > msg->maxsize\n"
						, SOCK_AdrToString( chan->remoteAddress ),
						chan->fragmentLength );
			return qfalse;
		}

		// copy the full message over the partial fragment

		// make sure the sequence number is still there
		*(int *)msg->_data = LittleLong( sequence );

		Com_Memcpy( msg->_data + 4, chan->fragmentBuffer, chan->fragmentLength );
		msg->cursize = chan->fragmentLength + 4;
		chan->fragmentLength = 0;
		msg->readcount = 4; // past the sequence number
		msg->bit = 32;  // past the sequence number

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


/*
=============================================================================

LOOPBACK BUFFERS FOR LOCAL PLAYER

=============================================================================
*/

// there needs to be enough loopback messages to hold a complete
// gamestate of maximum size
#define MAX_LOOPBACK    16

typedef struct {
	byte data[MAX_PACKETLEN];
	int datalen;
} loopmsg_t;

typedef struct {
	loopmsg_t msgs[MAX_LOOPBACK];
	int get, send;
} loopback_t;

loopback_t loopbacks[2];


qboolean    NET_GetLoopPacket( netsrc_t sock, netadr_t *net_from, QMsg *net_message ) {
	int i;
	loopback_t  *loop;

	loop = &loopbacks[sock];

	if ( loop->send - loop->get > MAX_LOOPBACK ) {
		loop->get = loop->send - MAX_LOOPBACK;
	}

	if ( loop->get >= loop->send ) {
		return qfalse;
	}

	i = loop->get & ( MAX_LOOPBACK - 1 );
	loop->get++;

	Com_Memcpy( net_message->_data, loop->msgs[i].data, loop->msgs[i].datalen );
	net_message->cursize = loop->msgs[i].datalen;
	Com_Memset( net_from, 0, sizeof( *net_from ) );
	net_from->type = NA_LOOPBACK;
	return qtrue;

}


void NET_SendLoopPacket( netsrc_t sock, int length, const void *data, netadr_t to ) {
	int i;
	loopback_t  *loop;

	loop = &loopbacks[sock ^ 1];

	i = loop->send & ( MAX_LOOPBACK - 1 );
	loop->send++;

	Com_Memcpy( loop->msgs[i].data, data, length );
	loop->msgs[i].datalen = length;
}

//=============================================================================

//bani
extern Cvar *sv_maxclients;
extern Cvar *sv_packetloss;
extern Cvar *sv_packetdelay;
#ifndef DEDICATED
extern Cvar *cl_packetloss;
extern Cvar *cl_packetdelay;
#endif

typedef struct delaybuf delaybuf_t;
struct delaybuf {
	netsrc_t sock;
	int length;
	char data[MAX_PACKETLEN];
	netadr_t to;
	int time;
	delaybuf_t *next;
};

static delaybuf_t *sv_delaybuf_head = NULL;
static delaybuf_t *sv_delaybuf_tail = NULL;
#ifndef DEDICATED
static delaybuf_t *cl_delaybuf_head = NULL;
static delaybuf_t *cl_delaybuf_tail = NULL;
#endif

void NET_SendPacket( netsrc_t sock, int length, const void *data, netadr_t to ) {
//bani
	int packetloss, packetdelay;
	delaybuf_t **delaybuf_head, **delaybuf_tail;

	switch ( sock ) {
#ifndef DEDICATED
	case NS_CLIENT:
		packetloss = cl_packetloss->integer;
		packetdelay = cl_packetdelay->integer;
		delaybuf_head = &cl_delaybuf_head;
		delaybuf_tail = &cl_delaybuf_tail;
		break;
#endif
	case NS_SERVER:
		packetloss = sv_packetloss->integer;
		packetdelay = sv_packetdelay->integer;
		delaybuf_head = &sv_delaybuf_head;
		delaybuf_tail = &sv_delaybuf_tail;
		break;

	default:
		// shut up compiler for dedicated
		packetloss = 0;
		packetdelay = 0;
		delaybuf_head = NULL;
		delaybuf_tail = NULL;
		break;
	}

	if ( packetloss > 0 ) {
		if ( ( (float)rand() / RAND_MAX ) * 100 <= packetloss ) {
			if ( showpackets->integer ) {
				Com_Printf( "drop packet %4i\n", length );
			}
			return;
		}
	}

//bani
	if ( packetdelay ) {
		int curtime;
		delaybuf_t *buf, *nextbuf;

		curtime = Sys_Milliseconds();

		//send any scheduled packets, starting from oldest
		for ( buf = *delaybuf_head; buf; buf = nextbuf ) {

			if ( ( buf->time + packetdelay ) > curtime ) {
				break;
			}

			if ( showpackets->integer ) {
				Com_Printf( "delayed packet(%dms) %4i\n", buf->time - curtime, buf->length );
			}

			switch ( buf->to.type ) {
			case NA_BOT:
			case NA_BAD:
				break;
			case NA_LOOPBACK:
				NET_SendLoopPacket( buf->sock, buf->length, buf->data, buf->to );
				break;
			case NA_BROADCAST:
			case NA_IP:
				if (ip_socket)
				{
					SOCK_Send(ip_socket, buf->data, buf->length, &buf->to);
				}
				break;
			default:
				Com_Error( ERR_FATAL, "NET_SendPacket: bad address type" );
				break;
			}

			// remove from queue
			nextbuf = buf->next;
			*delaybuf_head = nextbuf;
			if ( !*delaybuf_head ) {
				*delaybuf_tail = NULL;
			}
			Z_Free( buf );
		}

		// create buffer and add it to the queue
		buf = (delaybuf_t *)Z_Malloc( sizeof( *buf ) );
		if ( !buf ) {
			Com_Error( ERR_FATAL, "Couldn't allocate packet delay buffer\n" );
		}

		buf->sock = sock;
		buf->length = length;
		memcpy( buf->data, data, length );
		buf->to = to;
		buf->time = curtime;
		buf->next = NULL;

		if ( *delaybuf_head ) {
			( *delaybuf_tail )->next = buf;
		} else {
			*delaybuf_head = buf;
		}
		*delaybuf_tail = buf;

		return;
	}

	// sequenced packets are shown in netchan, so just show oob
	if ( showpackets->integer && *(int *)data == -1 ) {
		Com_Printf( "send packet %4i\n", length );
	}

	if ( to.type == NA_LOOPBACK ) {
		NET_SendLoopPacket( sock, length, data, to );
		return;
	}
	if ( to.type == NA_BOT ) {
		return;
	}
	if ( to.type == NA_BAD ) {
		return;
	}

	if (to.type != NA_BROADCAST && to.type != NA_IP)
	{
		Com_Error( ERR_FATAL, "NET_SendPacket: bad address type" );
		return;
	}

	SOCK_Send(ip_socket, data, length, &to);
}

/*
===============
NET_OutOfBandPrint

Sends a text message in an out-of-band datagram
================
*/
void QDECL NET_OutOfBandPrint( netsrc_t sock, netadr_t adr, const char *format, ... ) {
	va_list argptr;
	char string[MAX_MSGLEN];

	// set the header
	string[0] = -1;
	string[1] = -1;
	string[2] = -1;
	string[3] = -1;

	va_start( argptr, format );
	Q_vsnprintf( string + 4, sizeof( string ) - 4, format, argptr );
	va_end( argptr );

	// send the datagram
	NET_SendPacket( sock, String::Length( string ), string, adr );
}



/*
===============
NET_OutOfBandPrint

Sends a data message in an out-of-band datagram (only used for "connect")
================
*/
void QDECL NET_OutOfBandData( netsrc_t sock, netadr_t adr, byte *format, int len ) {
	byte string[MAX_MSGLEN * 2];
	int i;
	QMsg mbuf;

	MSG_InitOOB( &mbuf, string, sizeof( string ) );

	// set the header
	string[0] = 0xff;
	string[1] = 0xff;
	string[2] = 0xff;
	string[3] = 0xff;

	for ( i = 0; i < len; i++ ) {
		string[i + 4] = format[i];
	}

	mbuf.cursize = len + 4;
	Huff_Compress( &mbuf, 12 );
	// send the datagram
	NET_SendPacket( sock, mbuf.cursize, mbuf._data, adr );
}

/*
==================
Sys_GetPacket

Never called by the game logic, just the system event queing
==================
*/
qboolean Sys_GetPacket( netadr_t *net_from, QMsg *net_message ) {
	int ret;
	int net_socket;

	net_socket = ip_socket;

	if ( !net_socket ) {
		return qfalse;
	}

	ret = SOCK_Recv(net_socket, net_message->_data, net_message->maxsize, net_from);
	if ( ret == SOCKRECV_NO_DATA || ret == SOCKRECV_ERROR) {
		return qfalse;
	}

	net_message->readcount = 0;

	if ( ret == net_message->maxsize ) {
		Com_Printf( "Oversize packet from %s\n", SOCK_AdrToString( *net_from ) );
		return qfalse;
	}

	net_message->cursize = ret;
	return qtrue;
}

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
NET_OpenIP
====================
*/
static void NET_OpenIP( void ) {
	Cvar  *ip;
	int port;
	int i;

	ip = Cvar_Get( "net_ip", "localhost", CVAR_LATCH2 );
	port = Cvar_Get( "net_port", va( "%i", PORT_SERVER ), CVAR_LATCH2 )->integer;

	// automatically scan for a valid port, so multiple
	// dedicated servers can be started without requiring
	// a different net_port for each one
	for ( i = 0 ; i < 10 ; i++ ) {
		ip_socket = SOCK_Open( ip->string, port + i );
		if ( ip_socket ) {
			Cvar_SetValue( "net_port", port + i );
			if ( net_socksEnabled->integer ) {
				SOCK_OpenSocks( port + i );
			}
			SOCK_GetLocalAddress();
			return;
		}
	}
	Com_Printf( "WARNING: Couldn't allocate IP port\n" );
}

/*
====================
NET_Config
====================
*/
static void NET_Config( qboolean enableNetworking ) {
	qboolean modified;
	qboolean stop;
	qboolean start;

	// get any latched changes to cvars
	modified = NET_GetCvars();

	if ( net_noudp->integer) {
		enableNetworking = qfalse;
	}

	// if enable state is the same and no cvars were modified, we have nothing to do
	if ( enableNetworking == networkingEnabled && !modified ) {
		return;
	}

	if ( enableNetworking == networkingEnabled ) {
		if ( enableNetworking ) {
			stop = qtrue;
			start = qtrue;
		} else {
			stop = qfalse;
			start = qfalse;
		}
	} else {
		if ( enableNetworking ) {
			stop = qfalse;
			start = qtrue;
		} else {
			stop = qtrue;
			start = qfalse;
		}
		networkingEnabled = enableNetworking;
	}

	if ( stop ) {
		if ( ip_socket && ip_socket != -1 ) {
			SOCK_Close( ip_socket );
			ip_socket = 0;
		}

		SOCK_CloseSocks();
	}

	if ( start ) {
		if ( !net_noudp->integer ) {
			NET_OpenIP();
		}
	}
}

/*
====================
NET_Init
====================
*/
void NET_Init( void ) {
	if (!SOCK_Init()) {
		return;
	}

	// this is really just to get the cvars registered
	NET_GetCvars();

	//FIXME testing!
	NET_Config( qtrue );
}

/*
====================
NET_Shutdown
====================
*/
void NET_Shutdown( void ) {
	NET_Config( qfalse );
	SOCK_Shutdown();
}

/*
====================
NET_Restart_f
====================
*/
void NET_Restart( void ) {
	NET_Config( networkingEnabled );
}

// sleeps msec or until net socket is ready
void NET_Sleep( int msec ) {
	if ( !com_dedicated->integer ) {
		return; // we're not a server, just run full speed

	}
	SOCK_Sleep(ip_socket, msec);
}
