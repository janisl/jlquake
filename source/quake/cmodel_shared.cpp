//**************************************************************************
//**
//**	$Id: endian.cpp 101 2010-04-03 23:06:31Z dj_jl $
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

#include "../../libs/core/cm29_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static QClipMap29*	CMap;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	CM_LoadMap
//
//==========================================================================

clipHandle_t CM_LoadMap(const char* name, bool clientload, int* checksum)
{
	if (!name[0])
	{
		throw QException("CM_ForName: NULL name");
	}

	if (CMap)
	{
		if (!QStr::Cmp(CMap->Map.map_models[0].name, name))
		{
			//	Already got it.
			return 0;
		}

		delete CMap;
	}

	//
	// load the file
	//
	QArray<quint8> Buffer;
	if (FS_ReadFile(name, Buffer) <= 0)
	{
		throw QDropException(va("Couldn't load %s", name));
	}

	CMap = new QClipMap29;
	CMapShared = CMap;
	CMap->LoadModel(name, Buffer);

	return 0;
}
