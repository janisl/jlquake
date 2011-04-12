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

enum netadrtype_t
{
	NA_BOT,
	NA_BAD,					// an address lookup failed
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
};

#define PORT_ANY			-1

#define SOCKRECV_ERROR		-1
#define SOCKRECV_NO_DATA	-2

#define SOCKSEND_ERROR		-1
#define SOCKSEND_WOULDBLOCK	-2

struct netadr_t
{
	netadrtype_t	type;
	quint8			ip[4];
	quint16			port;
};

void NetadrToSockadr(netadr_t* a, struct sockaddr_in* s);
void SockadrToNetadr(struct sockaddr_in* s, netadr_t* a);
const char* SOCK_ErrorString();
bool SOCK_GetAddressByName(const char* s, netadr_t* a);
bool SOCK_StringToAdr(const char* s, netadr_t* a, int DefaultPort = 0);

bool SOCK_Init();
void SOCK_Shutdown();
void SOCK_GetLocalAddress();

bool SOCK_GetSocksCvars();
void SOCK_OpenSocks(int Port);
void SOCK_CloseSocks();

int SOCK_Open(const char* NetInterface, int Port);
void SOCK_Close(int Socket);
int SOCK_Recv(int Socket, void* Buffer, int Length, netadr_t* From);
int SOCL_Send(int Socket, const void* Data, int Length, netadr_t* To);

extern QCvar*	net_socksEnabled;
extern QCvar*	net_socksServer;
extern QCvar*	net_socksPort;
extern QCvar*	net_socksUsername;
extern QCvar*	net_socksPassword;

#define	MAX_IPS		16
extern int		numIP;
extern byte		localIP[MAX_IPS][4];
