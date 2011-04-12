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

// Returns true if we can't bind the address locally--in other words, 
// the IP is NOT one of our interfaces.
qboolean NET_IsClientLegal(netadr_t *adr)
{
	struct sockaddr_in sadr;
	int newsocket;

#if 0
	if (adr->ip[0] == 127)
		return false; // no local connections period

	NetadrToSockadr (adr, &sadr);

	if ((newsocket = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		Sys_Error ("NET_IsClientLegal: socket:", SOCK_ErrorString());

	sadr.sin_port = 0;

	if( bind (newsocket, (void *)&sadr, sizeof(sadr)) == -1) 
	{
		// It is not a local address
		close(newsocket);
		return true;
	}
	close(newsocket);
	return false;
#else
	return true;
#endif
}


void NET_GetLocalAddress (void)
{
	struct sockaddr_in	address;
	socklen_t	namelen;

	SOCK_GetLocalAddress();

	struct sockaddr_in sadr;
	
	Com_Memset(&sadr, 0, sizeof(sadr));
	sadr.sin_family = AF_INET;

	sadr.sin_port = 0;

	*(int*)&sadr.sin_addr = *(int*)localIP[0];
	
	SockadrToNetadr (&sadr, &net_local_adr);

	namelen = sizeof(address);
	if (getsockname (net_socket, (struct sockaddr *)&address, &namelen) == -1)
		Sys_Error ("NET_Init: getsockname:", SOCK_ErrorString());
	net_local_adr.port = address.sin_port;
}
