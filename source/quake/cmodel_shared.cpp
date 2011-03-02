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
//	CM_Init
//
//==========================================================================

void CM_Init()
{
}

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

	CMap = new QClipMap29;
	CMapShared = CMap;
	CMap->LoadModel(name);

	return 0;
}

//==========================================================================
//
//	CM_PrecacheModel
//
//==========================================================================

clipHandle_t CM_PrecacheModel(const char* name)
{
	if (!name[0])
	{
		Sys_Error("CM_ForName: NULL name");
	}

	//
	// search the currently loaded models
	//
	for (int i = 0; i < CMap->numknown; i++)
	{
		if (!QStr::Cmp(CMap->known[i]->model.name, name))
		{
			return (MAX_MAP_MODELS + i) * MAX_MAP_HULLS;
		}
	}

	if (CMap->numknown == MAX_CMOD_KNOWN)
	{
		Sys_Error("mod_numknown == MAX_CMOD_KNOWN");
	}
	CMap->known[CMap->numknown] = new QClipModelNonMap29;
	QClipModelNonMap29* LoadCMap = CMap->known[CMap->numknown];
	CMap->numknown++;

	LoadCMap->Load(name);

	return (MAX_MAP_MODELS + CMap->numknown - 1) * MAX_MAP_HULLS;
}

//==========================================================================
//
//	CM_CalcPHS
//
//	Calculates the PHS (Potentially Hearable Set)
//
//==========================================================================

void CM_CalcPHS()
{
	CMap->CalcPHS();
}
