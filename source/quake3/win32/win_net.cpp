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
// net_wins.c

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "win_local.h"

extern int	ip_socket;

/*
=============
Sys_StringToAdr

idnewt
192.246.40.70
=============
*/
qboolean Sys_StringToAdr( const char *s, netadr_t *a ) {
	struct sockaddr sadr;
	
	if ( !SOCK_StringToSockaddr( s, (struct sockaddr_in*)&sadr ) ) {
		return qfalse;
	}
	
	SockadrToNetadr( (struct sockaddr_in*)&sadr, a );
	return qtrue;
}

/*
====================
NET_Sleep

sleeps msec or until net socket is ready
====================
*/
void NET_Sleep( int msec ) {
}
