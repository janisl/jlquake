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
#include <netinet/in.h>
#include <sys/ioctl.h>

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

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
