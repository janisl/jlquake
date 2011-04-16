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
#include "system_unix.h"
#include <unistd.h>

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
//	Sys_CommonConsoleInput
//
//==========================================================================

char text[256];
char* Sys_CommonConsoleInput()
{
	if (!com_dedicated || !com_dedicated->value)
	{
		return NULL;
	}

	if (!stdin_active)
	{
		return NULL;
	}

	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(0, &fdset); // stdin
	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	if (select(1, &fdset, NULL, NULL, &timeout) == -1 || !FD_ISSET(0, &fdset))
	{
		return NULL;
	}

	int len = read(0, text, sizeof(text));
	if (len == 0)
	{
		// eof!
		stdin_active = false;
		return NULL;
	}

	if (len < 1)
	{
		return NULL;
	}
	text[len - 1] = 0;    // rip off the /n and terminate

	return text;
}
