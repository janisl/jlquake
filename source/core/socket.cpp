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
