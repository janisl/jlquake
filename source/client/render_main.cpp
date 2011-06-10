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

glconfig_t	glConfig;

trGlobals_t	tr;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	R_DecomposeSort
//
//==========================================================================

void R_DecomposeSort(unsigned sort, int* entityNum, shader_t** shader, 
	int* fogNum, int* dlightMap)
{
	*fogNum = (sort >> QSORT_FOGNUM_SHIFT) & 31;
	*shader = tr.sortedShaders[(sort >> QSORT_SHADERNUM_SHIFT) & (MAX_SHADERS - 1)];
	*entityNum = (sort >> QSORT_ENTITYNUM_SHIFT) & 1023;
	*dlightMap = sort & 3;
}

//==========================================================================
//
//	SetFarClip
//
//==========================================================================

static void SetFarClip()
{
	if (!(GGameType & GAME_Quake3))
	{
		tr.viewParms.zFar = 4096;
		return;
	}

	// if not rendering the world (icons, menus, etc)
	// set a 2k far clip plane
	if (tr.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		tr.viewParms.zFar = 2048;
		return;
	}

	//
	// set far clipping planes dynamically
	//
	float farthestCornerDistance = 0;
	for (int i = 0; i < 8; i++)
	{
		vec3_t v;
		if ( i & 1 )
		{
			v[0] = tr.viewParms.visBounds[0][0];
		}
		else
		{
			v[0] = tr.viewParms.visBounds[1][0];
		}

		if ( i & 2 )
		{
			v[1] = tr.viewParms.visBounds[0][1];
		}
		else
		{
			v[1] = tr.viewParms.visBounds[1][1];
		}

		if ( i & 4 )
		{
			v[2] = tr.viewParms.visBounds[0][2];
		}
		else
		{
			v[2] = tr.viewParms.visBounds[1][2];
		}

		vec3_t vecTo;
		VectorSubtract(v, tr.viewParms.orient.origin, vecTo);

		float distance = vecTo[0] * vecTo[0] + vecTo[1] * vecTo[1] + vecTo[2] * vecTo[2];

		if (distance > farthestCornerDistance)
		{
			farthestCornerDistance = distance;
		}
	}
	tr.viewParms.zFar = sqrt(farthestCornerDistance);
}

//==========================================================================
//
//	R_SetupProjection
//
//==========================================================================

void R_SetupProjection()
{
	// dynamically compute far clip plane distance
	SetFarClip();

	//
	// set up projection matrix
	//
	float zNear	= r_znear->value;
	float zFar	= tr.viewParms.zFar;

	float ymax = zNear * tan( tr.refdef.fov_y * M_PI / 360.0f );
	float ymin = -ymax;

	float xmax = zNear * tan(tr.refdef.fov_x * M_PI / 360.0f);
	float xmin = -xmax;

	float width = xmax - xmin;
	float height = ymax - ymin;
	float depth = zFar - zNear;

	tr.viewParms.projectionMatrix[0] = 2 * zNear / width;
	tr.viewParms.projectionMatrix[4] = 0;
	tr.viewParms.projectionMatrix[8] = (xmax + xmin) / width;	// normally 0
	tr.viewParms.projectionMatrix[12] = 0;

	tr.viewParms.projectionMatrix[1] = 0;
	tr.viewParms.projectionMatrix[5] = 2 * zNear / height;
	tr.viewParms.projectionMatrix[9] = (ymax + ymin) / height;	// normally 0
	tr.viewParms.projectionMatrix[13] = 0;

	tr.viewParms.projectionMatrix[2] = 0;
	tr.viewParms.projectionMatrix[6] = 0;
	tr.viewParms.projectionMatrix[10] = -(zFar + zNear) / depth;
	tr.viewParms.projectionMatrix[14] = -2 * zFar * zNear / depth;

	tr.viewParms.projectionMatrix[3] = 0;
	tr.viewParms.projectionMatrix[7] = 0;
	tr.viewParms.projectionMatrix[11] = -1;
	tr.viewParms.projectionMatrix[15] = 0;
}
