/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// unix_net.c

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../../wolfcore/socket_local.h"

static Cvar   *noudp;

netadr_t net_local_adr;

int ip_socket;

int NET_Socket( char *net_interface, int port );

//=============================================================================

char    *NET_BaseAdrToString( netadr_t a ) {
	static char s[64];

	String::Sprintf( s, sizeof( s ), "%i.%i.%i.%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3] );

	return s;
}

//=============================================================================

qboolean    Sys_GetPacket( netadr_t *net_from, QMsg *net_message ) {
	int ret;
	int net_socket;

	net_socket = ip_socket;

	if ( !net_socket ) {
		return qfalse;
	}

	ret = SOCK_Recv(net_socket, net_message->_data, net_message->maxsize, net_from);

	// bk000305: was missing
	net_message->readcount = 0;

	if ( ret == SOCKRECV_NO_DATA || ret == SOCKRECV_ERROR ) {
		return qfalse;
	}

	if ( ret == net_message->maxsize ) {
		Com_Printf( "Oversize packet from %s\n", NET_AdrToString( *net_from ) );
		return qfalse;
	}

	net_message->cursize = ret;
	return qtrue;
}

//=============================================================================

void    Sys_SendPacket( int length, const void *data, netadr_t to ) {
	int net_socket;

	if ( to.type == NA_BROADCAST ) {
		net_socket = ip_socket;
	} else if ( to.type == NA_IP )     {
		net_socket = ip_socket;
	} else {
		Com_Error( ERR_FATAL, "NET_SendPacket: bad address type" );
		return;
	}

	if ( !net_socket ) {
		return;
	}

	SOCK_Send(net_socket, data, length, &to);
}

//=============================================================================

/*
==================
Sys_IsLANAddress

LAN clients will have their rate var ignored
==================
*/
qboolean    Sys_IsLANAddress( netadr_t adr ) {
	int i;

	if ( adr.type == NA_LOOPBACK ) {
		return qtrue;
	}

	if ( adr.type != NA_IP ) {
		return qfalse;
	}

	// choose which comparison to use based on the class of the address being tested
	// any local adresses of a different class than the address being tested will fail based on the first byte

	// Class A
	if ( ( adr.ip[0] & 0x80 ) == 0x00 ) {
		for ( i = 0 ; i < numIP ; i++ ) {
			if ( adr.ip[0] == localIP[i][0] ) {
				return qtrue;
			}
		}
		// the RFC1918 class a block will pass the above test
		return qfalse;
	}

	// Class B
	if ( ( adr.ip[0] & 0xc0 ) == 0x80 ) {
		for ( i = 0 ; i < numIP ; i++ ) {
			if ( adr.ip[0] == localIP[i][0] && adr.ip[1] == localIP[i][1] ) {
				return qtrue;
			}
			// also check against the RFC1918 class b blocks
			if ( adr.ip[0] == 172 && localIP[i][0] == 172 && ( adr.ip[1] & 0xf0 ) == 16 && ( localIP[i][1] & 0xf0 ) == 16 ) {
				return qtrue;
			}
		}
		return qfalse;
	}

	// Class C
	for ( i = 0 ; i < numIP ; i++ ) {
		if ( adr.ip[0] == localIP[i][0] && adr.ip[1] == localIP[i][1] && adr.ip[2] == localIP[i][2] ) {
			return qtrue;
		}
		// also check against the RFC1918 class c blocks
		if ( adr.ip[0] == 192 && localIP[i][0] == 192 && adr.ip[1] == 168 && localIP[i][1] == 168 ) {
			return qtrue;
		}
	}
	return qfalse;
}

/*
==================
Sys_ShowIP
==================
*/
void Sys_ShowIP( void ) {
	int i;

	for ( i = 0; i < numIP; i++ ) {
		Com_Printf( "IP: %i.%i.%i.%i\n", localIP[i][0], localIP[i][1], localIP[i][2], localIP[i][3] );
	}
}

/*
=====================
NET_GetLocalAddress
=====================
*/
void NET_GetLocalAddress( void ) {
	SOCK_GetLocalAddress();
}

/*
====================
NET_OpenIP
====================
*/
// bk001204 - prototype needed
int NET_IPSocket( char *net_interface, int port );
void NET_OpenIP( void ) {
	Cvar  *ip;
	int port;
	int i;

	ip = Cvar_Get( "net_ip", "localhost", 0 );

	port = Cvar_Get( "net_port", va( "%i", PORT_SERVER ), 0 )->value;

	for ( i = 0 ; i < 10 ; i++ ) {
		ip_socket = NET_IPSocket( ip->string, port + i );
		if ( ip_socket ) {
			Cvar_SetValue( "net_port", port + i );
			NET_GetLocalAddress();
			return;
		}
	}
	Com_Error( ERR_FATAL, "Couldn't allocate IP port" );
}


/*
====================
NET_Init
====================
*/
void NET_Init( void ) {
	noudp = Cvar_Get( "net_noudp", "0", 0 );
	// open sockets
	if ( !noudp->value ) {
		NET_OpenIP();
	}
}


/*
====================
NET_IPSocket
====================
*/
int NET_IPSocket( char *net_interface, int port ) {
	return SOCK_Open(net_interface, port);
}

/*
====================
NET_Shutdown
====================
*/
void    NET_Shutdown( void ) {
	if ( ip_socket ) {
		SOCK_Close( ip_socket );
		ip_socket = 0;
	}
}

// sleeps msec or until net socket is ready
void NET_Sleep( int msec ) {
	if ( !com_dedicated->integer ) {
		return; // we're not a server, just run full speed

	}
	SOCK_Sleep(ip_socket, msec);
}

