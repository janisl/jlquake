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
#include <windows.h>

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
		s->sin_addr.s_addr = INADDR_BROADCAST;
	}
	else if (a->type == NA_IP)
	{
		s->sin_family = AF_INET;
		s->sin_addr.s_addr = *(int*)&a->ip;
		s->sin_port = a->port;
	}
}

//==========================================================================
//
//	SockadrToNetadr
//
//==========================================================================

void SockadrToNetadr(sockaddr_in* s, netadr_t* a)
{
	if (s->sin_family != AF_INET)
	{
		throw QException("Not an IP address");
	}
	a->type = NA_IP;
	*(int*)&a->ip = s->sin_addr.s_addr;
	a->port = s->sin_port;
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
	int code = WSAGetLastError();
	switch (code)
	{
	case WSAEINTR: return "WSAEINTR";
	case WSAEBADF: return "WSAEBADF";
	case WSAEACCES: return "WSAEACCES";
	case WSAEDISCON: return "WSAEDISCON";
	case WSAEFAULT: return "WSAEFAULT";
	case WSAEINVAL: return "WSAEINVAL";
	case WSAEMFILE: return "WSAEMFILE";
	case WSAEWOULDBLOCK: return "WSAEWOULDBLOCK";
	case WSAEINPROGRESS: return "WSAEINPROGRESS";
	case WSAEALREADY: return "WSAEALREADY";
	case WSAENOTSOCK: return "WSAENOTSOCK";
	case WSAEDESTADDRREQ: return "WSAEDESTADDRREQ";
	case WSAEMSGSIZE: return "WSAEMSGSIZE";
	case WSAEPROTOTYPE: return "WSAEPROTOTYPE";
	case WSAENOPROTOOPT: return "WSAENOPROTOOPT";
	case WSAEPROTONOSUPPORT: return "WSAEPROTONOSUPPORT";
	case WSAESOCKTNOSUPPORT: return "WSAESOCKTNOSUPPORT";
	case WSAEOPNOTSUPP: return "WSAEOPNOTSUPP";
	case WSAEPFNOSUPPORT: return "WSAEPFNOSUPPORT";
	case WSAEAFNOSUPPORT: return "WSAEAFNOSUPPORT";
	case WSAEADDRINUSE: return "WSAEADDRINUSE";
	case WSAEADDRNOTAVAIL: return "WSAEADDRNOTAVAIL";
	case WSAENETDOWN: return "WSAENETDOWN";
	case WSAENETUNREACH: return "WSAENETUNREACH";
	case WSAENETRESET: return "WSAENETRESET";
	case WSAECONNABORTED: return "WSWSAECONNABORTEDAEINTR";
	case WSAECONNRESET: return "WSAECONNRESET";
	case WSAENOBUFS: return "WSAENOBUFS";
	case WSAEISCONN: return "WSAEISCONN";
	case WSAENOTCONN: return "WSAENOTCONN";
	case WSAESHUTDOWN: return "WSAESHUTDOWN";
	case WSAETOOMANYREFS: return "WSAETOOMANYREFS";
	case WSAETIMEDOUT: return "WSAETIMEDOUT";
	case WSAECONNREFUSED: return "WSAECONNREFUSED";
	case WSAELOOP: return "WSAELOOP";
	case WSAENAMETOOLONG: return "WSAENAMETOOLONG";
	case WSAEHOSTDOWN: return "WSAEHOSTDOWN";
	case WSASYSNOTREADY: return "WSASYSNOTREADY";
	case WSAVERNOTSUPPORTED: return "WSAVERNOTSUPPORTED";
	case WSANOTINITIALISED: return "WSANOTINITIALISED";
	case WSAHOST_NOT_FOUND: return "WSAHOST_NOT_FOUND";
	case WSATRY_AGAIN: return "WSATRY_AGAIN";
	case WSANO_RECOVERY: return "WSANO_RECOVERY";
	case WSANO_DATA: return "WSANO_DATA";
	default: return "NO ERROR";
	}
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

	SOCKET newsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (newsocket == INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		if (err != WSAEAFNOSUPPORT)
		{
			GLog.Write("WARNING: SOCK_Open: socket: %s\n", SOCK_ErrorString());
		}
		return 0;
	}

	// make it non-blocking
	u_long _true = 1;
	if (ioctlsocket(newsocket, FIONBIO, &_true) == SOCKET_ERROR)
	{
		GLog.Write("WARNING: UDP_OpenSocket: ioctl FIONBIO: %s\n", SOCK_ErrorString());
		closesocket(newsocket);
		return 0;
	}

	// make it broadcast capable
	int i = 1;
	if (setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (char*)&i, sizeof(i)) == SOCKET_ERROR)
	{
		GLog.Write("WARNING: UDP_OpenSocket: setsockopt SO_BROADCAST: %s\n", SOCK_ErrorString());
		return 0;
	}

	sockaddr_in	address;
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

	if (bind(newsocket, (sockaddr*)&address, sizeof(address)) == SOCKET_ERROR)
	{
		GLog.Write("WARNING: UDP_OpenSocket: bind: %s\n", SOCK_ErrorString());
		closesocket(newsocket);
		return 0;
	}

	return newsocket;
}
