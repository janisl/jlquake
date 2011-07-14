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

static byte dottexture[16][16] =
{
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},//1
	{0,0,0,1,0,1,0,0,0,0,0,0,1,1,0,0},
	{0,0,0,0,1,0,0,0,0,0,0,1,1,1,1,0},
	{0,1,0,1,1,1,0,1,0,0,0,1,1,1,1,0},//4
	{0,0,1,1,1,1,1,0,0,0,0,0,1,1,0,0},
	{0,1,0,1,1,1,0,1,0,0,0,0,0,0,0,0},
	{0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0},//8
	{0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0},
	{0,0,0,0,0,0,0,0,0,1,1,1,0,1,0,0},
	{0,0,0,0,0,0,0,0,1,1,0,1,1,0,1,0},
	{0,0,0,1,0,0,0,0,1,1,1,1,1,0,1,0},//12
	{0,1,1,1,0,0,0,0,1,1,0,1,1,0,1,0},
	{0,0,1,1,1,0,0,0,0,1,1,1,0,1,0,0},
	{0,0,1,0,0,0,0,0,0,0,1,1,1,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},//16
};

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	R_InitParticleTexture
//
//==========================================================================

void R_InitParticleTexture()
{
	//
	// particle texture
	//
	byte data[16][16][4];
	for (int x = 0; x < 16; x++)
	{
		for (int y = 0; y < 16; y++)
		{
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = dottexture[y][x] * 255;
		}
	}
	tr.particleImage = R_CreateImage("*particle", (byte*)data, 16, 16, true, false, GL_CLAMP, false);
}

//==========================================================================
//
//	R_DrawParticle
//
//==========================================================================

void R_DrawParticle(const particle_t* p, const vec3_t up, const vec3_t right,
	float s1, float t1, float s2, float t2)
{
	// hack a scale up to keep particles from disapearing
	float scale = (p->origin[0] - tr.viewParms.orient.origin[0]) * tr.viewParms.orient.axis[0][0] + 
		(p->origin[1] - tr.viewParms.orient.origin[1]) * tr.viewParms.orient.axis[0][1] +
		(p->origin[2] - tr.viewParms.orient.origin[2]) * tr.viewParms.orient.axis[0][2];

	if (scale < 20)
	{
		scale = p->size;
	}
	else
	{
		scale = p->size + scale * 0.004;
	}

	qglColor4ubv(p->rgba);

	qglTexCoord2f(s1, t1);
	qglVertex3fv(p->origin);

	qglTexCoord2f(s2, t1);
	qglVertex3f(p->origin[0] + up[0] * scale, p->origin[1] + up[1] * scale, p->origin[2] + up[2] * scale);

	qglTexCoord2f(s1, t2);
	qglVertex3f(p->origin[0] + right[0] * scale, p->origin[1] + right[1] * scale, p->origin[2] + right[2] * scale);
}

//==========================================================================
//
//	R_DrawRegularParticle
//
//==========================================================================

void R_DrawRegularParticle(const particle_t* p, const vec3_t up, const vec3_t right)
{
	R_DrawParticle(p, up, right, 1 - 0.0625 / 2, 0.0625 / 2, 1 - 1.0625 / 2, 1.0625 / 2);
}
