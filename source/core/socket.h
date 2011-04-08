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

#define PORT_ANY	-1

struct netadr_t
{
	netadrtype_t	type;
	quint8			ip[4];
	quint16			port;
};

struct qsockaddr
{
	short			sa_family;
	unsigned char	sa_data[14];
};

void NetadrToSockadr(netadr_t* a, struct sockaddr_in* s);
void SockadrToNetadr(struct sockaddr_in* s, netadr_t* a);
bool SOCK_StringToSockaddr(const char* s, struct sockaddr_in* sadr);
const char* SOCK_ErrorString();
int SOCK_Open(const char* NetInterface, int Port);
void SOCK_Close(int Socket);
