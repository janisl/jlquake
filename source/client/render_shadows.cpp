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

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	RB_ProjectionShadowDeform
//
//==========================================================================

void RB_ProjectionShadowDeform()
{
	float* xyz = (float*)tess.xyz;

	vec3_t ground;
	ground[0] = backEnd.orient.axis[0][2];
	ground[1] = backEnd.orient.axis[1][2];
	ground[2] = backEnd.orient.axis[2][2];

	float groundDist = backEnd.orient.origin[2] - backEnd.currentEntity->e.shadowPlane;

	vec3_t lightDir;
	VectorCopy(backEnd.currentEntity->lightDir, lightDir);
	float d = DotProduct(lightDir, ground);
	// don't let the shadows get too long or go negative
	if (d < 0.5)
	{
		VectorMA(lightDir, (0.5 - d), ground, lightDir);
		d = DotProduct(lightDir, ground);
	}
	d = 1.0 / d;

	vec3_t light;
	light[0] = lightDir[0] * d;
	light[1] = lightDir[1] * d;
	light[2] = lightDir[2] * d;

	for (int i = 0; i < tess.numVertexes; i++, xyz += 4)
	{
		float h = DotProduct(xyz, ground) + groundDist;

		xyz[0] -= light[0] * h;
		xyz[1] -= light[1] * h;
		xyz[2] -= light[2] * h;
	}
}
