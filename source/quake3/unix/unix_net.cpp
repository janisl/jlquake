/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
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
// unix_net.c

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h> // bk001204

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <errno.h>

static QCvar	*noudp;

netadr_t	net_local_adr;

extern int			ip_socket;

int NET_Socket (char *net_interface, int port);

//=============================================================================

char	*NET_BaseAdrToString (netadr_t a)
{
	static	char	s[64];
	
	QStr::Sprintf(s, sizeof(s), "%i.%i.%i.%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3]);

	return s;
}

/*
=============
Sys_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
qboolean	Sys_StringToAdr (const char *s, netadr_t *a)
{
	struct sockaddr_in sadr;
	
	if (!SOCK_StringToSockaddr(s, &sadr))
		return qfalse;
	
	SockadrToNetadr (&sadr, a);

	return qtrue;
}


//=============================================================================

/*
==================
Sys_IsLANAddress

LAN clients will have their rate var ignored
==================
*/
qboolean	Sys_IsLANAddress (netadr_t adr) {
	int		i;

	if( adr.type == NA_LOOPBACK ) {
		return qtrue;
	}

	if( adr.type != NA_IP ) {
		return qfalse;
	}

	// choose which comparison to use based on the class of the address being tested
	// any local adresses of a different class than the address being tested will fail based on the first byte

	// Class A
	if( (adr.ip[0] & 0x80) == 0x00 ) {
		for ( i = 0 ; i < numIP ; i++ ) {
			if( adr.ip[0] == localIP[i][0] ) {
				return qtrue;
			}
		}
		// the RFC1918 class a block will pass the above test
		return qfalse;
	}

	// Class B
	if( (adr.ip[0] & 0xc0) == 0x80 ) {
		for ( i = 0 ; i < numIP ; i++ ) {
			if( adr.ip[0] == localIP[i][0] && adr.ip[1] == localIP[i][1] ) {
				return qtrue;
			}
			// also check against the RFC1918 class b blocks
			if( adr.ip[0] == 172 && localIP[i][0] == 172 && (adr.ip[1] & 0xf0) == 16 && (localIP[i][1] & 0xf0) == 16 ) {
				return qtrue;
			}
		}
		return qfalse;
	}

	// Class C
	for ( i = 0 ; i < numIP ; i++ ) {
		if( adr.ip[0] == localIP[i][0] && adr.ip[1] == localIP[i][1] && adr.ip[2] == localIP[i][2] ) {
			return qtrue;
		}
		// also check against the RFC1918 class c blocks
		if( adr.ip[0] == 192 && localIP[i][0] == 192 && adr.ip[1] == 168 && localIP[i][1] == 168 ) {
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
void Sys_ShowIP(void) {
	int i;

	for (i = 0; i < numIP; i++) {
		Com_Printf( "IP: %i.%i.%i.%i\n", localIP[i][0], localIP[i][1], localIP[i][2], localIP[i][3] );
	}
}

/*
====================
NET_OpenIP
====================
*/
// bk001204 - prototype needed
void NET_OpenIP (void)
{
	QCvar	*ip;
	int		port;
	int		i;

	ip = Cvar_Get ("net_ip", "localhost", 0);

	port = Cvar_Get("net_port", va("%i", PORT_SERVER), 0)->value;

	for ( i = 0 ; i < 10 ; i++ ) {
		ip_socket = SOCK_Open(ip->string, port + i);
		if ( ip_socket ) {
			Cvar_SetValue( "net_port", port + i );
			if (net_socksEnabled->integer)
			{
				SOCK_OpenSocks(port + i);
			}
			SOCK_GetLocalAddress();
			return;
		}
	}
	Com_Error (ERR_FATAL, "Couldn't allocate IP port");
}


/*
====================
NET_Init
====================
*/
void NET_Init (void)
{
	noudp = Cvar_Get ("net_noudp", "0", 0);

	SOCK_GetSocksCvars();

	// open sockets
	if (! noudp->value) {
		NET_OpenIP ();
	}
}


/*
====================
NET_Shutdown
====================
*/
void	NET_Shutdown (void)
{
	if (ip_socket) {
		close(ip_socket);
		ip_socket = 0;
	}

	SOCK_CloseSocks();
}

// sleeps msec or until net socket is ready
void NET_Sleep(int msec)
{
    struct timeval timeout;
	fd_set	fdset;
	extern qboolean stdin_active;

	if (!ip_socket || !com_dedicated->integer)
		return; // we're not a server, just run full speed

	FD_ZERO(&fdset);
	if (stdin_active)
		FD_SET(0, &fdset); // stdin is processed too
	FD_SET(ip_socket, &fdset); // network socket
	timeout.tv_sec = msec/1000;
	timeout.tv_usec = (msec%1000)*1000;
	select(ip_socket+1, &fdset, NULL, NULL, &timeout);
}

