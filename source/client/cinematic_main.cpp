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

#include "client.h"
#include "cinematic_local.h"

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
//	CIN_MakeFullName
//
//==========================================================================

void CIN_MakeFullName(const char* Name, char* FullName)
{
	const char* Dot = strstr(Name, ".");
	if (Dot && !QStr::ICmp(Dot, ".pcx"))
	{
		// static pcx image
		QStr::Sprintf(FullName, MAX_QPATH, "pics/%s", Name);
	}
	if (strstr(Name, "/") == NULL && strstr(Name, "\\") == NULL)
	{
		QStr::Sprintf(FullName, MAX_QPATH, "video/%s", Name);
	}
	else
	{
		QStr::Sprintf(FullName, MAX_QPATH, "%s", Name);
	}
}

//==========================================================================
//
//	CIN_Open
//
//==========================================================================

QCinematic* CIN_Open(const char* Name)
{
	const char* dot = strstr(Name, ".");
	if (dot && !QStr::ICmp(dot, ".pcx"))
	{
		// static pcx image
		QCinematicPcx* Cin = new QCinematicPcx();
		if (!Cin->Open(Name))
		{
			delete Cin;
			return NULL;
		}
		return Cin;
	}

	if (dot && !QStr::ICmp(dot, ".cin"))
	{
		QCinematicCin* Cin = new QCinematicCin();
		if (!Cin->Open(Name))
		{
			delete Cin;
			return NULL;
		}
		return Cin;
	}

	QCinematicRoq* Cin = new QCinematicRoq();
	if (!Cin->Open(Name))
	{
		delete Cin;
		return NULL;
	}
	return Cin;
}
