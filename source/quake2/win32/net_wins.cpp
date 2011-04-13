/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// net_wins.c

#include "winsock.h"
#include "../qcommon/qcommon.h"

extern int			ip_sockets[2];

// sleeps msec or until net socket is ready
void NET_Sleep(int msec)
{
	extern QCvar *dedicated;

	if (!dedicated || !dedicated->value)
	{
		return; // we're not a server, just run full speed
	}

	SOCK_Sleep(ip_sockets[NS_SERVER], msec);
}
