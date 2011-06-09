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
#include "render_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

backEndData_t*	backEndData[SMP_FRAMES];
backEndState_t	backEnd;

int				max_polys;
int				max_polyverts;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	R_InitBackEndData
//
//==========================================================================

void R_InitBackEndData()
{
	max_polys = r_maxpolys->integer;
	if (max_polys < MAX_POLYS)
	{
		max_polys = MAX_POLYS;
	}

	max_polyverts = r_maxpolyverts->integer;
	if (max_polyverts < MAX_POLYVERTS)
	{
		max_polyverts = MAX_POLYVERTS;
	}

	byte* ptr = (byte*)Mem_Alloc(sizeof(*backEndData[0]) + sizeof(srfPoly_t) * max_polys + sizeof(polyVert_t) * max_polyverts);
	backEndData[0] = (backEndData_t*)ptr;
	backEndData[0]->polys = (srfPoly_t*)((char*)ptr + sizeof(*backEndData[0]));
	backEndData[0]->polyVerts = (polyVert_t*)((char*)ptr + sizeof(*backEndData[0]) + sizeof(srfPoly_t) * max_polys);
	if (r_smp->integer)
	{
		ptr = (byte*)Mem_Alloc(sizeof(*backEndData[1]) + sizeof(srfPoly_t) * max_polys + sizeof(polyVert_t) * max_polyverts);
		backEndData[1] = (backEndData_t*)ptr;
		backEndData[1]->polys = (srfPoly_t*)((char*)ptr + sizeof(*backEndData[1]));
		backEndData[1]->polyVerts = (polyVert_t*)((char*)ptr + sizeof(*backEndData[1]) + sizeof(srfPoly_t) * max_polys);
	}
	else
	{
		backEndData[1] = NULL;
	}
	R_ToggleSmpFrame();
}

//==========================================================================
//
//	R_FreeBackEndData
//
//==========================================================================

void R_FreeBackEndData()
{
	Mem_Free(backEndData[0]);
	backEndData[0] = NULL;

	if (backEndData[1])
	{
		Mem_Free(backEndData[1]);
		backEndData[1] = NULL;
	}
}
