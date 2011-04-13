//**************************************************************************
//**
//**	$Id: socket_unix.cpp 476 2011-04-09 09:59:54Z dj_jl $
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

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

QCvar*		net_socksEnabled;
QCvar*		net_socksServer;
QCvar*		net_socksPort;
QCvar*		net_socksUsername;
QCvar*		net_socksPassword;

int			numIP;
byte		localIP[MAX_IPS][4];

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	SOCK_GetSocksCvars
//
//==========================================================================

bool SOCK_GetSocksCvars()
{
	bool modified = false;

	if (net_socksEnabled && net_socksEnabled->modified)
	{
		modified = true;
	}
	net_socksEnabled = Cvar_Get("net_socksEnabled", "0", CVAR_LATCH | CVAR_ARCHIVE);

	if (net_socksServer && net_socksServer->modified)
	{
		modified = true;
	}
	net_socksServer = Cvar_Get("net_socksServer", "", CVAR_LATCH | CVAR_ARCHIVE);

	if (net_socksPort && net_socksPort->modified)
	{
		modified = true;
	}
	net_socksPort = Cvar_Get("net_socksPort", "1080", CVAR_LATCH | CVAR_ARCHIVE);

	if (net_socksUsername && net_socksUsername->modified)
	{
		modified = true;
	}
	net_socksUsername = Cvar_Get("net_socksUsername", "", CVAR_LATCH | CVAR_ARCHIVE);

	if (net_socksPassword && net_socksPassword->modified)
	{
		modified = true;
	}
	net_socksPassword = Cvar_Get("net_socksPassword", "", CVAR_LATCH | CVAR_ARCHIVE);

	return modified;
}

//==========================================================================
//
//	SOCK_StringToAdr
//
//	Traps "localhost" for loopback, passes everything else to system
//
//==========================================================================

bool SOCK_StringToAdr(const char* s, netadr_t* a, int DefaultPort)
{
	if ((GGameType & GAME_Quake2) || (GGameType & GAME_Quake3))
	{
		if (!QStr::Cmp(s, "localhost"))
		{
			Com_Memset(a, 0, sizeof(*a));
			a->type = NA_LOOPBACK;
			return true;
		}
	}

	// look for a port number
	char base[MAX_STRING_CHARS];
	QStr::NCpyZ(base, s, sizeof(base));
	char* port = strstr(base, ":");
	if (port)
	{
		*port = 0;
		port++;
	}

	if (!SOCK_GetAddressByName(base, a))
	{
		a->type = NA_BAD;
		return false;
	}

	// inet_addr returns this if out of range
	if (a->ip[0] == 255 && a->ip[1] == 255 && a->ip[2] == 255 && a->ip[3] == 255)
	{
		a->type = NA_BAD;
		return false;
	}

	if (port)
	{
		a->port = BigShort((short)QStr::Atoi(port));
	}
	else
	{
		a->port = BigShort(DefaultPort);
	}

	return true;
}

//==========================================================================
//
//	SOCK_IsLocalAddress
//
//==========================================================================

bool SOCK_IsLocalAddress(const netadr_t& adr)
{
	return adr.type == NA_LOOPBACK;
}

//==========================================================================
//
//	SOCK_IsLocalIP
//
//==========================================================================

bool SOCK_IsLocalIP(const netadr_t& adr)
{
	if (adr.type != NA_IP)
	{
		return false;
	}

	//	Check for 127.0.0.1
	if (adr.ip[0] == 127 && adr.ip[1] == 0 && adr.ip[2] == 0 && adr.ip[3] == 1)
	{
		return true;
	}

	for (int i = 0; i < numIP; i++)
	{
		if (adr.ip[0] == localIP[i][0] && adr.ip[1] == localIP[i][1] && adr.ip[2] == localIP[i][2] && adr.ip[3] == localIP[i][3])
		{
			return true;
		}
	}
	return false;
}
