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

static unsigned long myAddr;
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
			myAddr = inet_addr(COM_Argv(i+1));
			if (myAddr == INADDR_NONE)
				Sys_Error ("%s is not a valid IP address", COM_Argv(i+1));
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
		myAddr = INADDR_ANY;
		QStr::Cpy(my_tcpip_address, "INADDR_ANY");
	}

	myAddr = *(int *)localIP[0];

	if ((net_controlsocket = UDP_OpenSocket (PORT_ANY)) == -1)
		Sys_Error("UDP_Init: Unable to open control socket\n");

	UDP_GetSocketAddr (net_controlsocket, &addr);
	qsockaddr sadr;
	NetadrToSockadr(&addr, (struct sockaddr_in*)&sadr);
	QStr::Cpy(my_tcpip_address,  UDP_AddrToString (&sadr));
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

int UDP_GetSocketAddr(int socket, netadr_t* addr)
{
	sockaddr_in sadr;
	socklen_t addrlen = sizeof(sadr);
	unsigned int a;

	Com_Memset(&sadr, 0, sizeof(struct qsockaddr));
	getsockname(socket, (struct sockaddr *)&sadr, &addrlen);
	a = sadr.sin_addr.s_addr;
	if (a == 0 || a == inet_addr("127.0.0.1"))
		sadr.sin_addr.s_addr = myAddr;

	SockadrToNetadr(&sadr, addr);
	return 0;
}

//=============================================================================

int UDP_GetNameFromAddr(netadr_t* addr, char* name)
{
	sockaddr_in sadr;
	NetadrToSockadr(addr, &sadr);
	struct hostent *hostentry;

	hostentry = gethostbyaddr ((char *)&sadr.sin_addr, sizeof(struct in_addr), AF_INET);
	if (hostentry)
	{
		QStr::NCpy(name, (char *)hostentry->h_name, NET_NAMELEN - 1);
		return 0;
	}

	QStr::Cpy(name, UDP_AddrToString((qsockaddr*)&sadr));
	return 0;
}

//=============================================================================

int UDP_GetAddrFromName(const char *name, struct qsockaddr *addr)
{
	netadr_t a;
	sockaddr_in sadr;
	if (!SOCK_StringToSockaddr(name, &sadr))
		return -1;

	SockadrToNetadr(&sadr, &a);
	a.port = htons(net_hostport);
	NetadrToSockadr(&a, (struct sockaddr_in*)addr);

	return 0;
}
