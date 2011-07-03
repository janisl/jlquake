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

/*
  THIS ENTIRE FILE IS BACK END

backEnd.currentEntity will be valid.

Tess_Begin has already been called for the surface's shader.

The modelview matrix will be set.

It is safe to actually issue drawing commands here if you don't want to
use the shader system.
*/

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

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	RB_CheckOverflow
//
//==========================================================================

void RB_CheckOverflow(int verts, int indexes)
{
	if (tess.numVertexes + verts < SHADER_MAX_VERTEXES &&
		tess.numIndexes + indexes < SHADER_MAX_INDEXES)
	{
		return;
	}

	RB_EndSurface();

	if (verts >= SHADER_MAX_VERTEXES)
	{
		throw QDropException(va("RB_CheckOverflow: verts > MAX (%d > %d)", verts, SHADER_MAX_VERTEXES));
	}
	if (indexes >= SHADER_MAX_INDEXES)
	{
		throw QDropException(va("RB_CheckOverflow: indices > MAX (%d > %d)", indexes, SHADER_MAX_INDEXES));
	}

	RB_BeginSurface(tess.shader, tess.fogNum);
}
