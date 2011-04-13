/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// net_main.c

#include "quakedef.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#if defined(sun)
#include <unistd.h>
#endif

#ifdef sun
#include <sys/filio.h>
#endif

#ifdef NeXT
#include <libc.h>
#endif

void NET_GetLocalAddress (void)
{
	SOCK_GetLocalAddress();

	struct sockaddr_in sadr;
	
	Com_Memset(&sadr, 0, sizeof(sadr));
	sadr.sin_family = AF_INET;

	sadr.sin_port = 0;

	*(int*)&sadr.sin_addr = *(int*)localIP[0];
	
	SockadrToNetadr (&sadr, &net_local_adr);

	netadr_t address;
	if (!SOCK_GetAddr(net_socket, &address))
	{
		Sys_Error("NET_Init: getsockname failed");
	}
	net_local_adr.port = address.port;
}
