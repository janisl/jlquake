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

float		speedscale;		// for top sky and bottom sky

char		skyname[MAX_QPATH];
float		skyrotate;
vec3_t		skyaxis;
image_t*	sky_images[6];

float		sky_mins[2][6], sky_maxs[2][6];
float		sky_min, sky_max;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// 3dstudio environment map names
static const char* suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	R_InitSky
//
//	A sky texture is 256*128, with the right side being a masked overlay
//
//==========================================================================

void R_InitSky(mbrush29_texture_t* mt)
{
	byte* src = (byte*)mt + mt->offsets[0];

	// make an average value for the back to avoid
	// a fringe on the top level

	int r = 0;
	int g = 0;
	int b = 0;
	byte trans[128 * 128 * 4];
	for (int i = 0; i < 128; i++)
	{
		for (int j = 0; j < 128; j++)
		{
			int p = src[i * 256 + j + 128];
			byte* rgba = r_palette[p];
			trans[(i * 128 + j) * 4 + 0] = rgba[0];
			trans[(i * 128 + j) * 4 + 1] = rgba[1];
			trans[(i * 128 + j) * 4 + 2] = rgba[2];
			trans[(i * 128 + j) * 4 + 3] = 255;
			r += rgba[0];
			g += rgba[1];
			b += rgba[2];
		}
	}

	byte transpix[4];
	transpix[0] = r / (128 * 128);
	transpix[1] = g / (128 * 128);
	transpix[2] = b / (128 * 128);
	transpix[3] = 0;

	if (!tr.solidskytexture)
	{
		tr.solidskytexture = R_CreateImage("*solidsky", trans, 128, 128, false, false, GL_REPEAT, false);
	}
	else
	{
		R_ReUploadImage(tr.solidskytexture, trans);
	}

	for (int i = 0; i < 128; i++)
	{
		for (int j = 0; j < 128; j++)
		{
			int p = src[i * 256 + j];
			byte* rgba;
			if (p == 0)
			{
				rgba = transpix;
			}
			else
			{
				rgba = r_palette[p];
			}
			trans[(i * 128 + j) * 4 + 0] = rgba[0];
			trans[(i * 128 + j) * 4 + 1] = rgba[1];
			trans[(i * 128 + j) * 4 + 2] = rgba[2];
			trans[(i * 128 + j) * 4 + 3] = rgba[3];
		}
	}

	if (!tr.alphaskytexture)
	{
		tr.alphaskytexture = R_CreateImage("*alphasky", trans, 128, 128, false, false, GL_REPEAT, false);
	}
	else
	{
		R_ReUploadImage(tr.alphaskytexture, trans);
	}
}

//==========================================================================
//
//	EmitSkyPolys
//
//==========================================================================

void EmitSkyPolys(mbrush29_surface_t* fa)
{
	for (mbrush29_glpoly_t* p = fa->polys; p; p = p->next)
	{
		qglBegin(GL_POLYGON);
		float* v = p->verts[0];
		for (int i = 0; i < p->numverts; i++, v += BRUSH29_VERTEXSIZE)
		{
			vec3_t dir;
			VectorSubtract(v, tr.refdef.vieworg, dir);
			dir[2] *= 3;	// flatten the sphere

			float length = dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2];
			length = sqrt(length);
			length = 6 * 63 / length;

			dir[0] *= length;
			dir[1] *= length;

			float s = (speedscale + dir[0]) * (1.0 / 128);
			float t = (speedscale + dir[1]) * (1.0 / 128);

			qglTexCoord2f(s, t);
			qglVertex3fv(v);
		}
		qglEnd();
	}
}

//==========================================================================
//
//	EmitBothSkyLayers
//
//	Does a sky warp on the pre-fragmented mbrush29_glpoly_t chain
//	This will be called for brushmodels, the world will have them chained together.
//
//==========================================================================

void EmitBothSkyLayers(mbrush29_surface_t* fa)
{
	GL_Bind(tr.solidskytexture);
	speedscale = tr.refdef.floatTime * 8;
	speedscale -= (int)speedscale & ~127;

	EmitSkyPolys(fa);

	GL_State(GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	GL_Bind(tr.alphaskytexture);
	speedscale = tr.refdef.floatTime * 16;
	speedscale -= (int)speedscale & ~127;

	EmitSkyPolys(fa);

	GL_State(GLS_DEFAULT);
}

//==========================================================================
//
//	R_DrawSkyChain
//
//==========================================================================

void R_DrawSkyChain(mbrush29_surface_t* s)
{
	// used when gl_texsort is on
	GL_Bind(tr.solidskytexture);
	speedscale = tr.refdef.floatTime * 8;
	speedscale -= (int)speedscale & ~127;

	for (mbrush29_surface_t* fa = s; fa; fa = fa->texturechain)
	{
		EmitSkyPolys(fa);
	}

	GL_State(GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	GL_Bind(tr.alphaskytexture);
	speedscale = tr.refdef.floatTime * 16;
	speedscale -= (int)speedscale & ~127;

	for (mbrush29_surface_t* fa = s; fa; fa = fa->texturechain)
	{
		EmitSkyPolys(fa);
	}

	GL_State(GLS_DEFAULT);
}

//==========================================================================
//
//	R_SetSky
//
//==========================================================================

void R_SetSky(const char *name, float rotate, vec3_t axis)
{
	QStr::NCpy(skyname, name, sizeof(skyname) - 1);
	skyrotate = rotate;
	VectorCopy(axis, skyaxis);

	for (int i = 0; i < 6; i++)
	{
		// chop down rotating skies for less memory
		if (r_skymip->value || skyrotate)
		{
			r_picmip->value++;
		}

		char pathname[MAX_QPATH];
		QStr::Sprintf(pathname, sizeof(pathname), "env/%s%s.tga", skyname, suf[i]);

		sky_images[i] = R_FindImageFile(pathname, false, false, GL_CLAMP);
		if (!sky_images[i])
		{
			sky_images[i] = tr.defaultImage;
		}

		if (r_skymip->value || skyrotate)
		{
			// take less memory
			r_picmip->value--;
			sky_min = 1.0 / 256;
			sky_max = 255.0 / 256;
		}
		else	
		{
			sky_min = 1.0 / 512;
			sky_max = 511.0 / 512;
		}
	}
}
