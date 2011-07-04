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

#define ON_EPSILON		0.1f			// point on plane side epsilon

#define SIDE_FRONT		0
#define SIDE_BACK		1
#define SIDE_ON			2

#define MAX_CLIP_VERTS	64

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

int sky_texorder[6] = {0, 2, 1, 3, 4, 5};

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// 3dstudio environment map names
static const char* suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};

static vec3_t sky_clip[6] =
{
	{1,1,0},
	{1,-1,0},
	{0,-1,1},
	{0,1,1},
	{1,0,1},
	{-1,0,1} 
};

// 1 = s, 2 = t, 3 = 2048
static int st_to_vec[6][3] =
{
	{3,-1,2},
	{-3,1,2},

	{1,3,2},
	{-1,-3,2},

	{-2,-1,3},		// 0 degrees yaw, look straight up
	{2,-1,-3}		// look straight down
};

// s = [0]/[2], t = [1]/[2]
static int vec_to_st[6][3] =
{
	{-2,3,1},
	{2,3,-1},

	{1,3,2},
	{-1,3,-2},

	{-2,-1,3},
	{-2,1,-3}

//	{-1,2,3},
//	{1,2,-3}
};

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

/*
===================================================================================

POLYGON TO BOX SIDE PROJECTION

===================================================================================
*/

//==========================================================================
//
//	AddSkyPolygon
//
//==========================================================================

static void AddSkyPolygon(int nump, vec3_t vecs)
{
	// decide which face it maps to
	vec3_t v;
	VectorCopy(vec3_origin, v);
	float* vp = vecs;
	for (int i = 0; i < nump; i++, vp += 3)
	{
		VectorAdd(vp, v, v);
	}
	vec3_t av;
	av[0] = fabs(v[0]);
	av[1] = fabs(v[1]);
	av[2] = fabs(v[2]);
	int axis;
	if (av[0] > av[1] && av[0] > av[2])
	{
		if (v[0] < 0)
		{
			axis = 1;
		}
		else
		{
			axis = 0;
		}
	}
	else if (av[1] > av[2] && av[1] > av[0])
	{
		if (v[1] < 0)
		{
			axis = 3;
		}
		else
		{
			axis = 2;
		}
	}
	else
	{
		if (v[2] < 0)
		{
			axis = 5;
		}
		else
		{
			axis = 4;
		}
	}

	// project new texture coords
	for (int i = 0; i < nump; i++, vecs += 3)
	{
		int j = vec_to_st[axis][2];
		float dv;
		if (j > 0)
		{
			dv = vecs[j - 1];
		}
		else
		{
			dv = -vecs[-j - 1];
		}
		if (dv < 0.001)
		{
			continue;	// don't divide by zero
		}
		j = vec_to_st[axis][0];
		float s;
		if (j < 0)
		{
			s = -vecs[-j - 1] / dv;
		}
		else
		{
			s = vecs[j - 1] / dv;
		}
		j = vec_to_st[axis][1];
		float t;
		if (j < 0)
		{
			t = -vecs[-j - 1] / dv;
		}
		else
		{
			t = vecs[j - 1] / dv;
		}

		if (s < sky_mins[0][axis])
		{
			sky_mins[0][axis] = s;
		}
		if (t < sky_mins[1][axis])
		{
			sky_mins[1][axis] = t;
		}
		if (s > sky_maxs[0][axis])
		{
			sky_maxs[0][axis] = s;
		}
		if (t > sky_maxs[1][axis])
		{
			sky_maxs[1][axis] = t;
		}
	}
}

//==========================================================================
//
//	ClipSkyPolygon
//
//==========================================================================

static void ClipSkyPolygon(int nump, vec3_t vecs, int stage)
{
	if (nump > MAX_CLIP_VERTS - 2)
	{
		throw QDropException("ClipSkyPolygon: MAX_CLIP_VERTS");
	}
	if (stage == 6)
	{
		// fully clipped, so draw it
		AddSkyPolygon(nump, vecs);
		return;
	}

	bool front = false;
	bool back = false;
	float* norm = sky_clip[stage];
	int sides[MAX_CLIP_VERTS];
	float dists[MAX_CLIP_VERTS];
	float* v = vecs;
	for (int i = 0; i < nump; i++, v += 3)
	{
		float d = DotProduct(v, norm);
		if (d > ON_EPSILON)
		{
			front = true;
			sides[i] = SIDE_FRONT;
		}
		else if (d < -ON_EPSILON)
		{
			back = true;
			sides[i] = SIDE_BACK;
		}
		else
		{
			sides[i] = SIDE_ON;
		}
		dists[i] = d;
	}

	if (!front || !back)
	{
		// not clipped
		ClipSkyPolygon(nump, vecs, stage + 1);
		return;
	}

	// clip it
	sides[nump] = sides[0];
	dists[nump] = dists[0];
	VectorCopy(vecs, (vecs + (nump * 3)));
	int newc[2];
	newc[0] = newc[1] = 0;
	vec3_t newv[2][MAX_CLIP_VERTS];

	v = vecs;
	for (int i = 0; i < nump; i++, v += 3)
	{
		switch (sides[i])
		{
		case SIDE_FRONT:
			VectorCopy(v, newv[0][newc[0]]);
			newc[0]++;
			break;

		case SIDE_BACK:
			VectorCopy(v, newv[1][newc[1]]);
			newc[1]++;
			break;

		case SIDE_ON:
			VectorCopy(v, newv[0][newc[0]]);
			newc[0]++;
			VectorCopy(v, newv[1][newc[1]]);
			newc[1]++;
			break;
		}

		if (sides[i] == SIDE_ON || sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i])
		{
			continue;
		}

		float d = dists[i] / (dists[i] - dists[i + 1]);
		for (int j = 0; j < 3; j++)
		{
			float e = v[j] + d * (v[j + 3] - v[j]);
			newv[0][newc[0]][j] = e;
			newv[1][newc[1]][j] = e;
		}
		newc[0]++;
		newc[1]++;
	}

	// continue
	ClipSkyPolygon(newc[0], newv[0][0], stage + 1);
	ClipSkyPolygon(newc[1], newv[1][0], stage + 1);
}

//==========================================================================
//
//	R_ClearSkyBox
//
//==========================================================================

void R_ClearSkyBox()
{
	for (int i = 0; i < 6; i++)
	{
		sky_mins[0][i] = sky_mins[1][i] = 9999;
		sky_maxs[0][i] = sky_maxs[1][i] = -9999;
	}
}

//==========================================================================
//
//	R_AddSkySurface
//
//==========================================================================

void R_AddSkySurface(mbrush38_surface_t* fa)
{
	// calculate vertex values for sky box
	for (mbrush38_glpoly_t* p = fa->polys; p; p = p->next)
	{
		vec3_t verts[MAX_CLIP_VERTS];
		for (int i = 0; i < p->numverts; i++)
		{
			VectorSubtract(p->verts[i], tr.viewParms.orient.origin, verts[i]);
		}
		ClipSkyPolygon(p->numverts, verts[0], 0);
	}
}

//==========================================================================
//
//	RB_ClipSkyPolygons
//
//==========================================================================

void RB_ClipSkyPolygons(shaderCommands_t* input)
{
	R_ClearSkyBox();

	for (int i = 0; i < input->numIndexes; i += 3)
	{
		vec3_t p[5];	// need one extra point for clipping
		for (int j = 0; j < 3; j++) 
		{
			VectorSubtract(input->xyz[input->indexes[i + j]], backEnd.viewParms.orient.origin, p[j]);
		}
		ClipSkyPolygon(3, p[0], 0);
	}
}

//==========================================================================
//
//	MakeSkyVec
//
//	Parms: s, t range from -1 to 1
//
//==========================================================================

void MakeSkyVec(float s, float t, int axis, float outSt[2], vec3_t outXYZ)
{
	float boxSize = (GGameType & GAME_Quake3) ? backEnd.viewParms.zFar / 1.75 : 2300;		// div sqrt(3)
	vec3_t b;
	b[0] = s * boxSize;
	b[1] = t * boxSize;
	b[2] = boxSize;

	for (int j = 0; j < 3; j++)
	{
		int k = st_to_vec[axis][j];
		if (k < 0)
		{
			outXYZ[j] = -b[-k - 1];
		}
		else
		{
			outXYZ[j] = b[k - 1];
		}
	}

	// avoid bilerp seam
	s = (s + 1) * 0.5;
	t = (t + 1) * 0.5;

	if (s < sky_min)
	{
		s = sky_min;
	}
	else if (s > sky_max)
	{
		s = sky_max;
	}
	if (t < sky_min)
	{
		t = sky_min;
	}
	else if (t > sky_max)
	{
		t = sky_max;
	}

	t = 1.0 - t;

	if (outSt)
	{
		outSt[0] = s;
		outSt[1] = t;
	}
}

//==========================================================================
//
//	EmitSkyVertex
//
//==========================================================================

static void EmitSkyVertex(float s, float t, int axis)
{
	vec3_t v;
	float St[2];	
	MakeSkyVec(s, t, axis, St, v);
	qglTexCoord2f(St[0], St[1]);
	qglVertex3fv(v);
}

//==========================================================================
//
//	R_DrawSkyBoxQ2
//
//==========================================================================

void R_DrawSkyBoxQ2()
{
	if (skyrotate)
	{
		// check for no sky at all
		int i;
		for (i = 0; i < 6; i++)
		{
			if (sky_mins[0][i] < sky_maxs[0][i] &&
				sky_mins[1][i] < sky_maxs[1][i])
			{
				break;
			}
		}
		if (i == 6)
		{
			return;		// nothing visible
		}
	}

	qglPushMatrix();
	qglTranslatef(tr.refdef.vieworg[0], tr.refdef.vieworg[1], tr.refdef.vieworg[2]);
	qglRotatef(tr.refdef.floatTime * skyrotate, skyaxis[0], skyaxis[1], skyaxis[2]);

	for (int i = 0; i < 6; i++)
	{
		if (skyrotate)
		{
			// hack, forces full sky to draw when rotating
			sky_mins[0][i] = -1;
			sky_mins[1][i] = -1;
			sky_maxs[0][i] = 1;
			sky_maxs[1][i] = 1;
		}

		if (sky_mins[0][i] >= sky_maxs[0][i] ||
			sky_mins[1][i] >= sky_maxs[1][i])
		{
			continue;
		}

		GL_Bind(sky_images[sky_texorder[i]]);

		qglBegin(GL_QUADS);
		EmitSkyVertex(sky_mins[0][i], sky_mins[1][i], i);
		EmitSkyVertex(sky_mins[0][i], sky_maxs[1][i], i);
		EmitSkyVertex(sky_maxs[0][i], sky_maxs[1][i], i);
		EmitSkyVertex(sky_maxs[0][i], sky_mins[1][i], i);
		qglEnd();
	}
	qglPopMatrix();
}
