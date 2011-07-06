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

mbrush29_glpoly_t*	lightmap_polys[MAX_LIGHTMAPS];
bool		lightmap_modified[MAX_LIGHTMAPS];
glRect_t	lightmap_rectchange[MAX_LIGHTMAPS];

// the lightmap texture data needs to be kept in
// main memory so texsubimage can update properly
byte		lightmaps[4*MAX_LIGHTMAPS*BLOCK_WIDTH*BLOCK_HEIGHT];

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static int					allocated[MAX_LIGHTMAPS][BLOCK_WIDTH];

static unsigned				blocklights_q1[18 * 18];

static mbrush29_vertex_t*	r_pcurrentvertbase;

// CODE --------------------------------------------------------------------

/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/

//==========================================================================
//
//	AllocBlock
//
// returns a texture number and the position inside it
//
//==========================================================================

static int AllocBlock(int w, int h, int* x, int* y)
{
	for (int texnum = 0; texnum < MAX_LIGHTMAPS; texnum++)
	{
		int best = BLOCK_HEIGHT;

		for (int i = 0; i < BLOCK_WIDTH - w; i++)
		{
			int best2 = 0;

			int j;
			for (j = 0; j < w; j++)
			{
				if (allocated[texnum][i + j] >= best)
				{
					break;
				}
				if (allocated[texnum][i + j] > best2)
				{
					best2 = allocated[texnum][i + j];
				}
			}
			if (j == w)
			{
				// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > BLOCK_HEIGHT)
		{
			continue;
		}

		for (int i = 0; i < w; i++)
		{
			allocated[texnum][*x + i] = best + h;
		}

		return texnum;
	}

	throw QException("AllocBlock: full");
}

//==========================================================================
//
//	R_AddDynamicLightsQ1
//
//==========================================================================

static void R_AddDynamicLightsQ1(mbrush29_surface_t* surf)
{
	int smax = (surf->extents[0] >> 4) + 1;
	int tmax = (surf->extents[1] >> 4) + 1;
	mbrush29_texinfo_t* tex = surf->texinfo;

	for (int lnum = 0; lnum < tr.refdef.num_dlights; lnum++)
	{
		if (!(surf->dlightbits & (1 << lnum)))
		{
			continue;		// not lit by this light
		}

		float rad = tr.refdef.dlights[lnum].radius;
		float dist = DotProduct(tr.refdef.dlights[lnum].origin, surf->plane->normal) - surf->plane->dist;
		rad -= Q_fabs(dist);
		float minlight = 0;//tr.refdef.dlights[lnum].minlight;
		if (rad < minlight)
		{
			continue;
		}
		minlight = rad - minlight;

		vec3_t impact;
		for (int i = 0; i < 3; i++)
		{
			impact[i] = tr.refdef.dlights[lnum].origin[i] - surf->plane->normal[i] * dist;
		}

		vec3_t local;
		local[0] = DotProduct(impact, tex->vecs[0]) + tex->vecs[0][3];
		local[1] = DotProduct(impact, tex->vecs[1]) + tex->vecs[1][3];

		local[0] -= surf->texturemins[0];
		local[1] -= surf->texturemins[1];
		
		for (int t = 0; t < tmax; t++)
		{
			int td = local[1] - t * 16;
			if (td < 0)
			{
				td = -td;
			}
			for (int s = 0; s < smax; s++)
			{
				int sd = local[0] - s * 16;
				if (sd < 0)
				{
					sd = -sd;
				}
				if (sd > td)
				{
					dist = sd + (td >> 1);
				}
				else
				{
					dist = td + (sd >> 1);
				}
				if (dist < minlight)
				{
					blocklights_q1[t * smax + s] += (rad - dist) * 256;
				}
			}
		}
	}
}

//==========================================================================
//
//	R_BuildLightMapQ1
//
//	Combine and scale multiple lightmaps into the 8.8 format in blocklights_q1
//
//==========================================================================

void R_BuildLightMapQ1(mbrush29_surface_t* surf, byte* dest, int stride)
{
	int			smax, tmax;
	int			t;
	int			i, j, size;
	byte		*lightmap;
	unsigned	scale;
	int			maps;
	unsigned	*bl;

	surf->cached_dlight = (surf->dlightframe == tr.frameCount);

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	size = smax*tmax;
	lightmap = surf->samples;

// set to full bright if no light data
	if (!tr.worldModel->brush29_lightdata)
	{
		for (i=0 ; i<size ; i++)
			blocklights_q1[i] = 255*256;
		goto store;
	}

// clear to no light
	for (i=0 ; i<size ; i++)
		blocklights_q1[i] = 0;

// add all the lightmaps
	if (lightmap)
		for (maps = 0 ; maps < BSP29_MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			scale = tr.refdef.lightstyles[surf->styles[maps]].rgb[0] * 256;
			surf->cached_light[maps] = scale;	// 8.8 fraction
			for (i=0 ; i<size ; i++)
				blocklights_q1[i] += lightmap[i] * scale;
			lightmap += size;	// skip to next lightmap
		}

// add all the dynamic lights
	if (surf->dlightframe == tr.frameCount)
		R_AddDynamicLightsQ1 (surf);

// bound, invert, and shift
store:
	stride -= (smax<<2);
	bl = blocklights_q1;
	for (i=0 ; i<tmax ; i++, dest += stride)
	{
		for (j=0 ; j<smax ; j++)
		{
			t = *bl++;
			t >>= 7;
			if (t > 255)
				t = 255;
			dest[0] = t;
			dest[1] = t;
			dest[2] = t;
			dest += 4;
		}
	}
}

//==========================================================================
//
//	GL_CreateSurfaceLightmapQ1
//
//==========================================================================

static void GL_CreateSurfaceLightmapQ1(mbrush29_surface_t* surf)
{
	if (surf->flags & (BRUSH29_SURF_DRAWSKY | BRUSH29_SURF_DRAWTURB))
	{
		return;
	}

	int smax = (surf->extents[0] >> 4) + 1;
	int tmax = (surf->extents[1] >> 4) + 1;

	surf->lightmaptexturenum = AllocBlock(smax, tmax, &surf->light_s, &surf->light_t);
	byte* base = lightmaps + surf->lightmaptexturenum * 4 * BLOCK_WIDTH * BLOCK_HEIGHT;
	base += (surf->light_t * BLOCK_WIDTH + surf->light_s) * 4;
	R_BuildLightMapQ1(surf, base, BLOCK_WIDTH * 4);
}

//==========================================================================
//
//	BuildSurfaceDisplayList
//
//==========================================================================

static void BuildSurfaceDisplayList(mbrush29_surface_t* fa)
{
	// reconstruct the polygon
	mbrush29_edge_t* pedges = tr.currentModel->brush29_edges;
	int lnumverts = fa->numedges;

	//
	// draw texture
	//
	mbrush29_glpoly_t* poly = (mbrush29_glpoly_t*)Mem_Alloc(sizeof(mbrush29_glpoly_t) + (lnumverts - 4) * BRUSH29_VERTEXSIZE * sizeof(float));
	poly->next = fa->polys;
	poly->flags = fa->flags;
	poly->chain = NULL;
	fa->polys = poly;
	poly->numverts = lnumverts;

	for (int i = 0; i < lnumverts; i++)
	{
		int lindex = tr.currentModel->brush29_surfedges[fa->firstedge + i];

		float* vec;
		if (lindex > 0)
		{
			mbrush29_edge_t* r_pedge = &pedges[lindex];
			vec = r_pcurrentvertbase[r_pedge->v[0]].position;
		}
		else
		{
			mbrush29_edge_t* r_pedge = &pedges[-lindex];
			vec = r_pcurrentvertbase[r_pedge->v[1]].position;
		}
		float s = DotProduct(vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->texture->width;

		float t = DotProduct(vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->texture->height;

		VectorCopy(vec, poly->verts[i]);
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;

		//
		// lightmap texture coordinates
		//
		s = DotProduct(vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s * 16;
		s += 8;
		s /= BLOCK_WIDTH * 16; //fa->texinfo->texture->width;

		t = DotProduct(vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t * 16;
		t += 8;
		t /= BLOCK_HEIGHT * 16; //fa->texinfo->texture->height;

		poly->verts[i][5] = s;
		poly->verts[i][6] = t;
	}

	//
	// remove co-linear points - Ed
	//
	if (!r_keeptjunctions->value && !(fa->flags & BRUSH29_SURF_UNDERWATER))
	{
		for (int i = 0; i < lnumverts; ++i)
		{
			vec3_t v1, v2;
			float *prev, *thisv, *next;

			prev = poly->verts[(i + lnumverts - 1) % lnumverts];
			thisv = poly->verts[i];
			next = poly->verts[(i + 1) % lnumverts];

			VectorSubtract(thisv, prev, v1);
			VectorNormalize(v1);
			VectorSubtract(next, prev, v2);
			VectorNormalize(v2);

			// skip co-linear points
			#define COLINEAR_EPSILON 0.001
			if ((Q_fabs(v1[0] - v2[0]) <= COLINEAR_EPSILON) &&
				(Q_fabs(v1[1] - v2[1]) <= COLINEAR_EPSILON) && 
				(Q_fabs(v1[2] - v2[2]) <= COLINEAR_EPSILON))
			{
				for (int j = i + 1; j < lnumverts; ++j)
				{
					for (int k = 0; k < BRUSH29_VERTEXSIZE; ++k)
					{
						poly->verts[j - 1][k] = poly->verts[j][k];
					}
				}
				--lnumverts;
				// retry next vertex next time, which is now current vertex
				--i;
			}
		}
	}
	poly->numverts = lnumverts;
}

//==========================================================================
//
//	GL_BuildLightmaps
//
//	Builds the lightmap texture with all the surfaces from all brush models
//
//==========================================================================

void GL_BuildLightmaps()
{
	Com_Memset(allocated, 0, sizeof(allocated));

	tr.frameCount = 1;		// no dlightcache

	for (int i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		backEndData[tr.smpFrame]->lightstyles[i].rgb[0] = 1;
		backEndData[tr.smpFrame]->lightstyles[i].rgb[1] = 1;
		backEndData[tr.smpFrame]->lightstyles[i].rgb[2] = 1;
		backEndData[tr.smpFrame]->lightstyles[i].white = 3;
	}
	tr.refdef.lightstyles = backEndData[tr.smpFrame]->lightstyles;

	if (!tr.lightmaps[0])
	{
		for (int i = 0; i < MAX_LIGHTMAPS; i++)
		{
			tr.lightmaps[i] = R_CreateImage(va("*lightmap%d", i), lightmaps+i*BLOCK_WIDTH*BLOCK_HEIGHT*4, BLOCK_WIDTH, BLOCK_HEIGHT, false, false, GL_CLAMP, false);
		}
	}

	for (int j = 1; j < MAX_MOD_KNOWN; j++)
	{
		model_t* m = tr.models[j];
		if (!m)
		{
			break;
		}
		if (m->type != MOD_BRUSH29)
		{
			continue;
		}
		if (m->name[0] == '*')
		{
			continue;
		}
		r_pcurrentvertbase = m->brush29_vertexes;
		tr.currentModel = m;
		for (int i = 0; i < m->brush29_numsurfaces; i++)
		{
			GL_CreateSurfaceLightmapQ1(m->brush29_surfaces + i);
			if (m->brush29_surfaces[i].flags & BRUSH29_SURF_DRAWTURB)
			{
				continue;
			}
			if (m->brush29_surfaces[i].flags & BRUSH29_SURF_DRAWSKY)
			{
				continue;
			}
			BuildSurfaceDisplayList(m->brush29_surfaces + i);
		}
	}

	//
	// upload all lightmaps that were filled
	//
	for (int i = 0; i < MAX_LIGHTMAPS; i++)
	{
		if (!allocated[i][0])
		{
			break;		// no more used
		}
		lightmap_modified[i] = false;
		lightmap_rectchange[i].l = BLOCK_WIDTH;
		lightmap_rectchange[i].t = BLOCK_HEIGHT;
		lightmap_rectchange[i].w = 0;
		lightmap_rectchange[i].h = 0;
		R_ReUploadImage(tr.lightmaps[i], lightmaps + i * BLOCK_WIDTH * BLOCK_HEIGHT * 4);
	}
}
