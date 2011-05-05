//**************************************************************************
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
#include "socket_local.h"
#include <windows.h>
#include <iphlpapi.h>

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static bool		winsockInitialized = false;
static WSADATA	winsockdata;

static bool		usingSocks = false;
static SOCKET	socks_socket;
static sockaddr	socksRelayAddr;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	NetadrToSockadr
//
//==========================================================================

static void NetadrToSockadr(netadr_t* a, sockaddr_in* s)
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

static void SockadrToNetadr(sockaddr_in* s, netadr_t* a)
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

static bool SOCK_StringToSockaddr(const char* s, sockaddr_in* sadr)
{
	Com_Memset(sadr, 0, sizeof(*sadr));

	sadr->sin_family = AF_INET;
	sadr->sin_port = 0;

	if (s[0] >= '0' && s[0] <= '9')
	{
		*(int*)&sadr->sin_addr = inet_addr(s);
	}
	else
	{
		hostent* h = gethostbyname(s);
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
//	SOCK_GetAddressByName
//
//==========================================================================

bool SOCK_GetAddressByName(const char* s, netadr_t* a)
{
	sockaddr_in sadr;
	if (!SOCK_StringToSockaddr(s, &sadr))
	{
		return false;
	}

	SockadrToNetadr(&sadr, a);

	return true;
}

//==========================================================================
//
//	SOCK_ErrorString
//
//==========================================================================

static const char* SOCK_ErrorString()
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
//	SOCK_Init
//
//==========================================================================

bool SOCK_Init()
{
	int r = WSAStartup(MAKEWORD(1, 1), &winsockdata);
	if (r)
	{
		GLog.Write("WARNING: Winsock initialization failed, returned %d\n", r);
		return false;
	}

	winsockInitialized = true;
	GLog.Write("Winsock Initialized\n");
	return true;
}

//==========================================================================
//
//	SOCK_Shutdown
//
//==========================================================================

void SOCK_Shutdown()
{
	if (!winsockInitialized)
	{
		return;
	}
	WSACleanup();
	winsockInitialized = false;
}

//==========================================================================
//
//	SOCK_GetLocalAddress
//
//==========================================================================

void SOCK_GetLocalAddress()
{
	numIP = 0;

	byte Buffer[2048];
	MIB_IPADDRTABLE* AddrTable = (MIB_IPADDRTABLE*)Buffer;
	ULONG AddrTableSize = sizeof(Buffer);
	if (GetIpAddrTable(AddrTable, &AddrTableSize, FALSE) != NO_ERROR)
	{
		return;
	}

	for (int i = 0; i < AddrTable->dwNumEntries && numIP < MAX_IPS; i++)
	{
		int ip = ntohl(AddrTable->table[i].dwAddr);
		localIP[numIP][0] = (ip >> 24) & 0xff;
		localIP[numIP][1] = (ip >> 16) & 0xff;
		localIP[numIP][2] = (ip >> 8) & 0xff;
		localIP[numIP][3] = ip & 0xff;
		GLog.Write("IP: %i.%i.%i.%i\n", (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
		numIP++;
	}
}

//==========================================================================
//
//	SOCK_OpenSocks
//
//==========================================================================

void SOCK_OpenSocks(int port)
{
	usingSocks = false;

	GLog.Write("Opening connection to SOCKS server.\n");

	socks_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socks_socket == INVALID_SOCKET)
	{
		GLog.Write("WARNING: SOCK_OpenSocks: socket: %s\n", SOCK_ErrorString());
		return;
	}

	hostent* h = gethostbyname(net_socksServer->string);
	if (h == NULL)
	{
		GLog.Write("WARNING: SOCK_OpenSocks: gethostbyname: %s\n", SOCK_ErrorString());
		return;
	}
	if (h->h_addrtype != AF_INET)
	{
		GLog.Write("WARNING: SOCK_OpenSocks: gethostbyname: address type was not AF_INET\n");
		return;
	}
	sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = *(int*)h->h_addr_list[0];
	address.sin_port = htons((short)net_socksPort->integer);

	if (connect(socks_socket, (sockaddr*)&address, sizeof(address)) == SOCKET_ERROR)
	{
		GLog.Write("SOCK_OpenSocks: connect: %s\n", SOCK_ErrorString());
		return;
	}

	// send socks authentication handshake
	bool rfc1929 = *net_socksUsername->string || *net_socksPassword->string;

	unsigned char buf[64];
	int len;
	buf[0] = 5;		// SOCKS version
	// method count
	if (rfc1929)
	{
		buf[1] = 2;
		len = 4;
	}
	else
	{
		buf[1] = 1;
		len = 3;
	}
	buf[2] = 0;		// method #1 - method id #00: no authentication
	if (rfc1929)
	{
		buf[2] = 2;		// method #2 - method id #02: username/password
	}
	if (send(socks_socket, (char*)buf, len, 0) == SOCKET_ERROR)
	{
		GLog.Write("SOCK_OpenSocks: send: %s\n", SOCK_ErrorString());
		return;
	}

	// get the response
	len = recv(socks_socket, (char*)buf, 64, 0);
	if (len == SOCKET_ERROR)
	{
		GLog.Write("SOCK_OpenSocks: recv: %s\n", SOCK_ErrorString());
		return;
	}
	if (len != 2 || buf[0] != 5)
	{
		GLog.Write("SOCK_OpenSocks: bad response\n");
		return;
	}
	switch (buf[1])
	{
	case 0:	// no authentication
		break;
	case 2: // username/password authentication
		break;
	default:
		GLog.Write("SOCK_OpenSocks: request denied\n");
		return;
	}

	// do username/password authentication if needed
	if (buf[1] == 2)
	{
		// build the request
		int ulen = QStr::Length(net_socksUsername->string);
		int plen = QStr::Length(net_socksPassword->string);

		buf[0] = 1;		// username/password authentication version
		buf[1] = ulen;
		if (ulen)
		{
			Com_Memcpy(&buf[2], net_socksUsername->string, ulen);
		}
		buf[2 + ulen] = plen;
		if (plen)
		{
			Com_Memcpy(&buf[3 + ulen], net_socksPassword->string, plen);
		}

		// send it
		if (send(socks_socket, (char*)buf, 3 + ulen + plen, 0) == SOCKET_ERROR)
		{
			GLog.Write("SOCK_OpenSocks: send: %s\n", SOCK_ErrorString());
			return;
		}

		// get the response
		len = recv(socks_socket, (char*)buf, 64, 0);
		if (len == SOCKET_ERROR)
		{
			GLog.Write("SOCK_OpenSocks: recv: %s\n", SOCK_ErrorString());
			return;
		}
		if (len != 2 || buf[0] != 1)
		{
			GLog.Write("SOCK_OpenSocks: bad response\n");
			return;
		}
		if (buf[1] != 0)
		{
			GLog.Write("SOCK_OpenSocks: authentication failed\n");
			return;
		}
	}

	// send the UDP associate request
	buf[0] = 5;		// SOCKS version
	buf[1] = 3;		// command: UDP associate
	buf[2] = 0;		// reserved
	buf[3] = 1;		// address type: IPV4
	*(int*)&buf[4] = INADDR_ANY;
	*(short*)&buf[8] = htons((short)port);		// port
	if (send(socks_socket, (char*)buf, 10, 0) == SOCKET_ERROR)
	{
		GLog.Write("SOCK_OpenSocks: send: %s\n", SOCK_ErrorString());
		return;
	}

	// get the response
	len = recv( socks_socket, (char*)buf, 64, 0 );
	if (len == SOCKET_ERROR)
	{
		GLog.Write("SOCK_OpenSocks: recv: %s\n", SOCK_ErrorString());
		return;
	}
	if (len < 2 || buf[0] != 5)
	{
		GLog.Write("SOCK_OpenSocks: bad response\n");
		return;
	}
	// check completion code
	if (buf[1] != 0)
	{
		GLog.Write("SOCK_OpenSocks: request denied: %i\n", buf[1]);
		return;
	}
	if (buf[3] != 1)
	{
		GLog.Write("SOCK_OpenSocks: relay address is not IPV4: %i\n", buf[3]);
		return;
	}
	((sockaddr_in*)&socksRelayAddr)->sin_family = AF_INET;
	((sockaddr_in*)&socksRelayAddr)->sin_addr.s_addr = *(int*)&buf[4];
	((sockaddr_in*)&socksRelayAddr)->sin_port = *(short*)&buf[8];
	Com_Memset(((sockaddr_in*)&socksRelayAddr)->sin_zero, 0, sizeof(((sockaddr_in*)&socksRelayAddr)->sin_zero));

	usingSocks = true;
}

//==========================================================================
//
//	SOCK_CloseSocks
//
//==========================================================================

void SOCK_CloseSocks()
{
	if (socks_socket && socks_socket != INVALID_SOCKET)
	{
		closesocket(socks_socket);
		socks_socket = 0;
	}
	usingSocks = false;
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
		closesocket(newsocket);
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

//==========================================================================
//
//	SOCK_Close
//
//==========================================================================

void SOCK_Close(int Socket)
{
	closesocket(Socket);
}

//==========================================================================
//
//	SOCK_Recv
//
//==========================================================================

int SOCK_Recv(int socket, void* buf, int len, netadr_t* From)
{
	sockaddr_in addr;
	int addrlen = sizeof(addr);
	int ret;
	QArray<quint8> SocksBuf;
	if (usingSocks)
	{
		SocksBuf.SetNum(len + 10);
		ret = recvfrom(socket, (char*)SocksBuf.Ptr(), len + 10, 0, (sockaddr*)&addr, &addrlen);
	}
	else
	{
		ret = recvfrom(socket, (char*)buf, len, 0, (sockaddr*)&addr, &addrlen);
	}

	if (ret == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK || err == WSAECONNRESET)
		{
			return SOCKRECV_NO_DATA;
		}

		GLog.Write("NET_GetPacket: %s\n", SOCK_ErrorString());
		return SOCKRECV_ERROR;
	}

	Com_Memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

	if (usingSocks)
	{
		if (memcmp(&addr, &socksRelayAddr, sizeof(addr)) == 0)
		{
			if (ret < 10 || SocksBuf[0] != 0 || SocksBuf[1] != 0 || SocksBuf[2] != 0 || SocksBuf[3] != 1)
			{
				return SOCKRECV_ERROR;
			}
			From->type = NA_IP;
			From->ip[0] = SocksBuf[4];
			From->ip[1] = SocksBuf[5];
			From->ip[2] = SocksBuf[6];
			From->ip[3] = SocksBuf[7];
			From->port = *(short*)&SocksBuf[8];
			ret -= 10;
			if (ret > 0)
			{
				Com_Memcpy(buf, &SocksBuf[10], ret);
			}
		}
		else
		{
			SockadrToNetadr(&addr, From);
			if (ret > len)
			{
				ret = len;
			}
			if (ret > 0)
			{
				Com_Memcpy(buf, SocksBuf.Ptr(), ret);
			}
		}
	}
	else
	{
		SockadrToNetadr(&addr, From);
	}
	return ret;
}

//==========================================================================
//
//	SOCK_Send
//
//==========================================================================

int SOCK_Send(int Socket, const void* Data, int Length, netadr_t* To)
{
	sockaddr_in addr;

	NetadrToSockadr(To, &addr);

	int ret;
	if (usingSocks && To->type == NA_IP)
	{
		QArray<char> socksBuf;
		socksBuf.SetNum(Length + 10);
		socksBuf[0] = 0;	// reserved
		socksBuf[1] = 0;
		socksBuf[2] = 0;	// fragment (not fragmented)
		socksBuf[3] = 1;	// address type: IPV4
		*(int*)&socksBuf[4] = addr.sin_addr.s_addr;
		*(short*)&socksBuf[8] = addr.sin_port;
		Com_Memcpy(&socksBuf[10], Data, Length);
		ret = sendto(Socket, socksBuf.Ptr(), Length + 10, 0, &socksRelayAddr, sizeof(socksRelayAddr));
	}
	else
	{
		ret = sendto(Socket, (char*)Data, Length, 0, (sockaddr*)&addr, sizeof(addr));
	}

	if (ret == SOCKET_ERROR)
	{
		int err = WSAGetLastError();

		// wouldblock is silent
		if (err == WSAEWOULDBLOCK)
		{
			return SOCKSEND_WOULDBLOCK;
		}

		// some PPP links do not allow broadcasts and return an error
		if ((err == WSAEADDRNOTAVAIL) && (To->type == NA_BROADCAST))
		{
			return SOCKSEND_WOULDBLOCK;
		}

		GLog.Write("NET_SendPacket: %s\n", SOCK_ErrorString());
		return SOCKSEND_ERROR;
	}

	return ret;
}

//==========================================================================
//
//	SOCK_Sleep
//
//==========================================================================

bool SOCK_Sleep(int socket, int msec)
{
	fd_set fdset;
	FD_ZERO(&fdset);
	int i = 0;
	if (socket)
	{
		FD_SET(socket, &fdset); // network socket
		i = socket;
	}

    timeval timeout;
	timeout.tv_sec = msec / 1000;
	timeout.tv_usec = (msec % 1000) * 1000;

	return select(i + 1, &fdset, NULL, NULL, &timeout) != -1;
}

//==========================================================================
//
//	SOCK_GetAddr
//
//==========================================================================

bool SOCK_GetAddr(int Socket, netadr_t* Address)
{
	sockaddr_in sadr;
	int addrlen = sizeof(sadr);

	Com_Memset(&sadr, 0, sizeof(sadr));
	if (getsockname(Socket, (struct sockaddr *)&sadr, &addrlen) == -1)
	{
		GLog.Write("WARNING: SOCK_GetAddr: getsockname: ", SOCK_ErrorString());
		return false;
	}

	SockadrToNetadr(&sadr, Address);
	return true;
}

//==========================================================================
//
//	SOCK_GetHostByAddr
//
//==========================================================================

const char* SOCK_GetHostByAddr(netadr_t* addr)
{
	sockaddr_in sadr;
	NetadrToSockadr(addr, &sadr);

	hostent* hostentry = gethostbyaddr((char*)&sadr.sin_addr, sizeof(in_addr), AF_INET);
	if (hostentry)
	{
		return hostentry->h_name;
	}

	return NULL;
}
