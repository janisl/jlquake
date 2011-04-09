//**************************************************************************
//**
//**	$Id$
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "core.h"
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>

#ifdef MACOS_X
#import <sys/sockio.h>
#import <net/if.h>
#import <net/if_types.h>
#import <arpa/inet.h>         // for inet_ntoa()
#import <net/if_dl.h>         // for 'struct sockaddr_dl'
#endif

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

int		numIP;
byte	localIP[MAX_IPS][4];
char				hostname_buf[256];

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	NetadrToSockadr
//
//==========================================================================

void NetadrToSockadr(netadr_t* a, sockaddr_in* s)
{
	Com_Memset(s, 0, sizeof(*s));

	if (a->type == NA_BROADCAST)
	{
		s->sin_family = AF_INET;

		s->sin_port = a->port;
		*(int*)&s->sin_addr = -1;
	}
	else if (a->type == NA_IP)
	{
		s->sin_family = AF_INET;

		*(int*)&s->sin_addr = *(int*)&a->ip;
		s->sin_port = a->port;
	}
	else
	{
		throw QException("Invalid address type");
	}
}

//==========================================================================
//
//	SockadrToNetadr
//
//==========================================================================

void SockadrToNetadr(sockaddr_in* s, netadr_t* a)
{
	*(int*)&a->ip = *(int*)&s->sin_addr;
	a->port = s->sin_port;
	a->type = NA_IP;
}

//==========================================================================
//
//	SOCK_StringToSockaddr
//
//==========================================================================

bool SOCK_StringToSockaddr(const char* s, sockaddr_in* sadr)
{
	Com_Memset(sadr, 0, sizeof(*sadr));
	sadr->sin_family = AF_INET;

	sadr->sin_port = 0;

	char copy[128];
	QStr::Cpy(copy, s);
	// strip off a trailing :port if present
	for (char* colon = copy; *colon; colon++)
	{
		if (*colon == ':')
		{
			*colon = 0;
			sadr->sin_port = htons((short)QStr::Atoi(colon + 1));
		}
	}
	
	if (copy[0] >= '0' && copy[0] <= '9')
	{
		*(int*)&sadr->sin_addr = inet_addr(copy);
	}
	else
	{
		hostent* h = gethostbyname(copy);
		if (!h)
		{
			return false;
		}
		*(int*)&sadr->sin_addr = *(int*)h->h_addr_list[0];
	}
	return true;
}

//==========================================================================
//
//	SOCK_ErrorString
//
//==========================================================================

const char* SOCK_ErrorString()
{
	int code = errno;
	return strerror(code);
}

//==========================================================================
//
//	SOCK_Init
//
//==========================================================================

bool SOCK_Init()
{
}

//==========================================================================
//
//	SOCK_Shutdown
//
//==========================================================================

void SOCK_Shutdown()
{
}

//==========================================================================
//
//	SOCK_GetLocalAddress
//
//==========================================================================

void SOCK_GetLocalAddress()
{
#ifdef MACOS_X
	// Don't do a forward mapping from the hostname of the machine to the IP.
	// The reason is that we might have obtained an IP address from DHCP and
	// there might not be any name registered for the machine.  On Mac OS X,
	// the machine name defaults to 'localhost' and NetInfo has 127.0.0.1
	// listed for this name.  Instead, we want to get a list of all the IP
	// network interfaces on the machine.
	// This code adapted from OmniNetworking.

#define IFR_NEXT(ifr)	\
    ((struct ifreq *) ((char *) (ifr) + sizeof(*(ifr)) + \
      MAX(0, (int) (ifr)->ifr_addr.sa_len - (int) sizeof((ifr)->ifr_addr))))

	ifreq requestBuffer[MAX_IPS], *linkInterface, *inetInterface;
	ifconf ifc;
	ifreq ifr;
	sockaddr_dl *sdl;
	int interfaceSocket;
	int family;

	//GLog.Write("NET_GetLocalAddress: Querying for network interfaces\n");

	// Set this early so we can just return if there is an error
	numIP = 0;

	ifc.ifc_len = sizeof(requestBuffer);
	ifc.ifc_buf = (caddr_t)requestBuffer;

	// Since we get at this info via an ioctl, we need a temporary little socket.  This will only get AF_INET interfaces, but we probably don't care about anything else.  If we do end up caring later, we should add a ONAddressFamily and at a -interfaces method to it.
	family = AF_INET;
	if ((interfaceSocket = socket(family, SOCK_DGRAM, 0)) < 0)
	{
		GLog.Write("NET_GetLocalAddress: Unable to create temporary socket, errno = %d\n", errno);
		return;
	}

	if (ioctl(interfaceSocket, SIOCGIFCONF, &ifc) != 0)
	{
		GLog.Write("NET_GetLocalAddress: Unable to get list of network interfaces, errno = %d\n", errno);
		return;
	}


	linkInterface = (struct ifreq *) ifc.ifc_buf;
	while ((char *) linkInterface < &ifc.ifc_buf[ifc.ifc_len])
	{
		unsigned int nameLength;

		// The ioctl returns both the entries having the address (AF_INET) and the link layer entries (AF_LINK).
		// The AF_LINK entry has the link layer address which contains the interface type.
		// This is the only way I can see to get this information.  We cannot assume that we will get bot an
		// AF_LINK and AF_INET entry since the interface may not be configured.  For example, if you have a
		// 10Mb port on the motherboard and a 100Mb card, you may not configure the motherboard port.

		// For each AF_LINK entry...
		if (linkInterface->ifr_addr.sa_family == AF_LINK)
		{
			// if there is a matching AF_INET entry
			inetInterface = (struct ifreq *) ifc.ifc_buf;
			while ((char*)inetInterface < &ifc.ifc_buf[ifc.ifc_len])
			{
				if (inetInterface->ifr_addr.sa_family == AF_INET &&
					!QStr::NCmp(inetInterface->ifr_name, linkInterface->ifr_name, sizeof(linkInterface->ifr_name)))
				{
					for (nameLength = 0; nameLength < IFNAMSIZ; nameLength++)
					{
						if (!linkInterface->ifr_name[nameLength])
						{
							break;
						}
					}

					sdl = (struct sockaddr_dl *)&linkInterface->ifr_addr;
					// Skip loopback interfaces
					if (sdl->sdl_type != IFT_LOOP)
					{
						// Get the local interface address
						QStr::NCpy(ifr.ifr_name, inetInterface->ifr_name, sizeof(ifr.ifr_name));
						if (ioctl(interfaceSocket, OSIOCGIFADDR, (caddr_t)&ifr) < 0)
						{
							GLog.Write("NET_GetLocalAddress: Unable to get local address for interface '%s', errno = %d\n", inetInterface->ifr_name, errno);
						}
						else
						{
							struct sockaddr_in *sin;
							int ip;

							sin = (struct sockaddr_in *)&ifr.ifr_addr;

							ip = ntohl(sin->sin_addr.s_addr);
							localIP[ numIP ][0] = (ip >> 24) & 0xff;
							localIP[ numIP ][1] = (ip >> 16) & 0xff;
							localIP[ numIP ][2] = (ip >>  8) & 0xff;
							localIP[ numIP ][3] = (ip >>  0) & 0xff;
							GLog.Write( "IP: %i.%i.%i.%i (%s)\n", localIP[ numIP ][0], localIP[ numIP ][1], localIP[ numIP ][2], localIP[ numIP ][3], inetInterface->ifr_name);
							numIP++;
						}
					}

					// We will assume that there is only one AF_INET entry per AF_LINK entry.
					// What happens when we have an interface that has multiple IP addresses, or
					// can that even happen?
					// break;
				}
				inetInterface = IFR_NEXT(inetInterface);
			}
		}
		linkInterface = IFR_NEXT(linkInterface);
	}

	close(interfaceSocket);
#else
	if (gethostname(hostname_buf, 256) == -1)
	{
		return;
	}

	hostent* hostInfo = gethostbyname(hostname_buf);
	if (!hostInfo)
	{
		return;
	}

	GLog.Write("Hostname: %s\n", hostInfo->h_name);
	int n = 0;
	char* p;
	while ((p = hostInfo->h_aliases[n++]) != NULL)
	{
		GLog.Write("Alias: %s\n", p);
	}

	if (hostInfo->h_addrtype != AF_INET)
	{
		return;
	}

	numIP = 0;
	while ((p = hostInfo->h_addr_list[numIP]) != NULL && numIP < MAX_IPS)
	{
		int ip = ntohl(*(int*)p);
		localIP[numIP][0] = p[0];
		localIP[numIP][1] = p[1];
		localIP[numIP][2] = p[2];
		localIP[numIP][3] = p[3];
		GLog.Write("IP: %i.%i.%i.%i\n", (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
		numIP++;
	}
#endif
}

//==========================================================================
//
//	SOCK_Open
//
//==========================================================================

int SOCK_Open(const char* net_interface, int port)
{
	if (net_interface)
	{
		GLog.Write("Opening IP socket: %s:%i\n", net_interface, port);
	}
	else
	{
		GLog.Write("Opening IP socket: localhost:%i\n", port);
	}

	int newsocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (newsocket == -1)
	{
		GLog.Write("ERROR: UDP_OpenSocket: socket: %s\n", SOCK_ErrorString());
		return 0;
	}

	// make it non-blocking
	int _true = 1;
	if (ioctl(newsocket, FIONBIO, &_true) == -1)
	{
		GLog.Write("ERROR: UDP_OpenSocket: ioctl FIONBIO:%s\n", SOCK_ErrorString());
		close(newsocket);
		return 0;
	}

	// make it broadcast capable
	int i = 1;
	if (setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (char*)&i, sizeof(i)) == -1)
	{
		GLog.Write("ERROR: UDP_OpenSocket: setsockopt SO_BROADCAST:%s\n", SOCK_ErrorString());
		close(newsocket);
		return 0;
	}

	sockaddr_in address;
	if (!net_interface || !net_interface[0] || !QStr::ICmp(net_interface, "localhost"))
	{
		address.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		SOCK_StringToSockaddr(net_interface, &address);
	}

	if (port == PORT_ANY)
	{
		address.sin_port = 0;
	}
	else
	{
		address.sin_port = htons((short)port);
	}

	address.sin_family = AF_INET;

	if (bind(newsocket, (sockaddr*)&address, sizeof(address)) == -1)
	{
		GLog.Write("ERROR: UDP_OpenSocket: bind: %s\n", SOCK_ErrorString());
		close(newsocket);
		return 0;
	}

	return newsocket;
}

//==========================================================================
//
//	SOCK_Close
//
//==========================================================================

void SOCK_Close(int Socket)
{
	close(Socket);
}

//==========================================================================
//
//	SOCK_Recv
//
//==========================================================================

int SOCK_Recv(int socket, void* buf, int len, netadr_t* From)
{
	sockaddr_in	addr;
	socklen_t addrlen = sizeof(addr);
	int ret = recvfrom(socket, buf, len, 0, (struct sockaddr*)&addr, &addrlen);

	if (ret == -1)
	{
		int err = errno;
		if (err == EWOULDBLOCK || err == ECONNREFUSED)
		{
			return SOCKRECV_NO_DATA;
		}

		GLog.Write("NET_GetPacket: %s", SOCK_ErrorString());
		//GLog.Write("NET_GetPacket: %s from %s\n", SOCK_ErrorString(), NET_AdrToString(*net_from));
		return SOCKRECV_ERROR;
	}

	SockadrToNetadr(&addr, From);
	return ret;
}

//==========================================================================
//
//	SOCL_Send
//
//==========================================================================

int SOCL_Send(int Socket, const void* Data, int Length, netadr_t* To)
{
	sockaddr_in addr;
	NetadrToSockadr(To, &addr);

	int ret = sendto(Socket, Data, Length, 0, (struct sockaddr*)&addr, sizeof(addr));

	if (ret == -1)
	{
		GLog.Write("NET_SendPacket ERROR: %i\n", SOCK_ErrorString());
		//GLog.Write("NET_SendPacket ERROR: %s to %s\n", SOCK_ErrorString(), NET_AdrToString(to));
		if (errno == EWOULDBLOCK)
		{
			return SOCKSEND_WOULDBLOCK;
		}
		return SOCKSEND_ERROR;
	}

	return ret;
}
