/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// net_wins.c

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../../core/socket_local.h"

static qboolean networkingEnabled = qfalse;

static Cvar   *net_noudp;

static int ip_socket;

//=============================================================================

/*
==================
Sys_GetPacket

Never called by the game logic, just the system event queing
==================
*/
int recvfromCount;

qboolean Sys_GetPacket( netadr_t *net_from, QMsg *net_message ) {
	int ret;
	int net_socket;

	net_socket = ip_socket;

	if ( !net_socket ) {
		return qfalse;
	}

	recvfromCount++;        // performance check
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

//=============================================================================

/*
==================
Sys_SendPacket
==================
*/
void Sys_SendPacket( int length, const void *data, netadr_t to ) {
	int net_socket;

	if ( to.type == NA_BROADCAST ) {
		net_socket = ip_socket;
	} else if ( to.type == NA_IP )    {
		net_socket = ip_socket;
	} else {
		Com_Error( ERR_FATAL, "Sys_SendPacket: bad address type" );
		return;
	}

	if ( !net_socket ) {
		return;
	}

	SOCK_Send(net_socket, data, length, &to);
}


//=============================================================================


/*
====================
NET_IPSocket
====================
*/
int NET_IPSocket( char *net_interface, int port ) {
	return SOCK_Open(net_interface, port);
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
void NET_OpenIP( void ) {
	Cvar  *ip;
	int port;
	int i;

	ip = Cvar_Get( "net_ip", "localhost", CVAR_LATCH2 );
	port = Cvar_Get( "net_port", va( "%i", PORT_SERVER ), CVAR_LATCH2 )->integer;

	// automatically scan for a valid port, so multiple
	// dedicated servers can be started without requiring
	// a different net_port for each one
	for ( i = 0 ; i < 10 ; i++ ) {
		ip_socket = NET_IPSocket( ip->string, port + i );
		if ( ip_socket ) {
			Cvar_SetValue( "net_port", port + i );
			if ( net_socksEnabled->integer ) {
				SOCK_OpenSocks( port + i );
			}
			NET_GetLocalAddress();
			return;
		}
	}
	Com_Printf( "WARNING: Couldn't allocate IP port\n" );
}

//===================================================================


/*
====================
NET_GetCvars
====================
*/
static qboolean NET_GetCvars( void ) {
	qboolean modified;

	modified = qfalse;

	if ( net_noudp && net_noudp->modified ) {
		modified = qtrue;
	}
	net_noudp = Cvar_Get( "net_noudp", "0", CVAR_LATCH2 | CVAR_ARCHIVE );

	if (SOCK_GetSocksCvars()) {
		modified = qtrue;
	}

	return modified;
}


/*
====================
NET_Config
====================
*/
void NET_Config( qboolean enableNetworking ) {
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
	return; //----(SA)	disabled networking for SP

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
NET_Sleep

sleeps msec or until net socket is ready
====================
*/
void NET_Sleep( int msec ) {
}


/*
====================
NET_Restart_f
====================
*/
void NET_Restart( void ) {
	NET_Config( networkingEnabled );
}
