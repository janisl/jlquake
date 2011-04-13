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
// net_udp.c

#include "quakedef.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>

#ifdef __sun__
#include <sys/filio.h>
#endif

#ifdef NeXT
#include <libc.h>
#endif

extern int net_acceptsocket;		// socket for fielding new connections
extern int net_controlsocket;

extern const char* net_interface;

#include "net_udp.h"

//=============================================================================

int UDP_Init (void)
{
	netadr_t addr;
	char *colon;
	
	if (COM_CheckParm ("-noudp"))
		return -1;

	// determine my name & address
	SOCK_GetLocalAddress();

	int i = COM_CheckParm ("-ip");
	if (i)
	{
		if (i < COM_Argc()-1)
		{
			QStr::Cpy(my_tcpip_address, COM_Argv(i+1));
			net_interface = COM_Argv(i+1);
		}
		else
		{
			Sys_Error ("NET_Init: you must specify an IP address after -ip");
		}
	}
	else
	{
		QStr::Cpy(my_tcpip_address, "INADDR_ANY");
	}

	if ((net_controlsocket = UDP_OpenSocket (PORT_ANY)) == -1)
		Sys_Error("UDP_Init: Unable to open control socket\n");

	UDP_GetSocketAddr (net_controlsocket, &addr);
	QStr::Cpy(my_tcpip_address,  UDP_AddrToString(&addr));
	colon = QStr::RChr(my_tcpip_address, ':');
	if (colon)
		*colon = 0;

	Con_Printf("UDP Initialized\n");
	tcpipAvailable = true;

	return net_controlsocket;
}

//=============================================================================

int UDP_CheckNewConnections (void)
{
	unsigned long	available;

	if (net_acceptsocket == -1)
		return -1;

	if (ioctl (net_acceptsocket, FIONREAD, &available) == -1)
		Sys_Error ("UDP: ioctlsocket (FIONREAD) failed\n");
	if (available)
		return net_acceptsocket;
	return -1;
}

//=============================================================================

int UDP_GetNameFromAddr(netadr_t* addr, char* name)
{
	const char* host = SOCK_GetHostByAddr(addr);
	if (host)
	{
		QStr::NCpy(name, host, NET_NAMELEN - 1);
		return 0;
	}

	QStr::Cpy(name, UDP_AddrToString(addr));
	return 0;
}
