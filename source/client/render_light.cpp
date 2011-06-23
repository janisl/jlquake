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

vec3_t			lightspot;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static vec3_t	pointcolor;

// CODE --------------------------------------------------------------------

/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

//==========================================================================
//
//	RecursiveLightPointQ1
//
//==========================================================================

static int RecursiveLightPointQ1(mbrush29_node_t* node, vec3_t start, vec3_t end)
{
	if (node->contents < 0)
	{
		return -1;		// didn't hit anything
	}

	// calculate mid point

	// FIXME: optimize for axial
	cplane_t* plane = node->plane;
	float front = DotProduct(start, plane->normal) - plane->dist;
	float back = DotProduct(end, plane->normal) - plane->dist;
	int side = front < 0;
	
	if ((back < 0) == side)
	{
		return RecursiveLightPointQ1(node->children[side], start, end);
	}

	float frac = front / (front - back);
	vec3_t mid;
	mid[0] = start[0] + (end[0] - start[0]) * frac;
	mid[1] = start[1] + (end[1] - start[1]) * frac;
	mid[2] = start[2] + (end[2] - start[2]) * frac;
	
	// go down front side	
	int r = RecursiveLightPointQ1(node->children[side], start, mid);
	if (r >= 0)
	{
		return r;		// hit something
	}
		
	if ((back < 0) == side)
	{
		return -1;		// didn't hit anuthing
	}

	// check for impact on this node
	VectorCopy(mid, lightspot);

	mbrush29_surface_t* surf = tr.worldModel->brush29_surfaces + node->firstsurface;
	for (int i = 0; i < node->numsurfaces; i++, surf++)
	{
		if (surf->flags & BRUSH29_SURF_DRAWTILED)
		{
			continue;	// no lightmaps
		}

		mbrush29_texinfo_t* tex = surf->texinfo;

		int s = DotProduct(mid, tex->vecs[0]) + tex->vecs[0][3];
		int t = DotProduct(mid, tex->vecs[1]) + tex->vecs[1][3];;

		if (s < surf->texturemins[0] || t < surf->texturemins[1])
		{
			continue;
		}
		
		int ds = s - surf->texturemins[0];
		int dt = t - surf->texturemins[1];
		
		if (ds > surf->extents[0] || dt > surf->extents[1])
		{
			continue;
		}

		if (!surf->samples)
		{
			return 0;
		}

		ds >>= 4;
		dt >>= 4;

		byte* lightmap = surf->samples;
		r = 0;
		if (lightmap)
		{

			lightmap += dt * ((surf->extents[0]>>4)+1) + ds;

			for (int maps = 0; maps < BSP29_MAXLIGHTMAPS && surf->styles[maps] != 255; maps++)
			{
				r += *lightmap * tr.refdef.lightstyles[surf->styles[maps]].rgb[0];
				lightmap += ((surf->extents[0] >> 4) + 1) * ((surf->extents[1] >> 4) + 1);
			}
		}

		return r;
	}

	// go down back side
	return RecursiveLightPointQ1(node->children[!side], mid, end);
}

//==========================================================================
//
//	R_LightPointQ1
//
//==========================================================================

int R_LightPointQ1(vec3_t p)
{
	if (!tr.worldModel->brush29_lightdata)
	{
		return 255;
	}

	vec3_t end;
	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 2048;
	
	int r = RecursiveLightPointQ1(tr.worldModel->brush29_nodes, p, end);
	
	if (r == -1)
	{
		r = 0;
	}

	return r;
}

//==========================================================================
//
//	RecursiveLightPointQ2
//
//==========================================================================

static int RecursiveLightPointQ2 (mbrush38_node_t *node, vec3_t start, vec3_t end)
{
	if (node->contents != -1)
	{
		return -1;		// didn't hit anything
	}
	
	// calculate mid point

	// FIXME: optimize for axial
	cplane_t* plane = node->plane;
	float front = DotProduct(start, plane->normal) - plane->dist;
	float back = DotProduct(end, plane->normal) - plane->dist;
	int side = front < 0;
	
	if ((back < 0) == side)
	{
		return RecursiveLightPointQ2(node->children[side], start, end);
	}

	float frac = front / (front - back);
	vec3_t mid;
	mid[0] = start[0] + (end[0] - start[0]) * frac;
	mid[1] = start[1] + (end[1] - start[1]) * frac;
	mid[2] = start[2] + (end[2] - start[2]) * frac;
	
	// go down front side	
	int r = RecursiveLightPointQ2(node->children[side], start, mid);
	if (r >= 0)
	{
		return r;		// hit something
	}
		
	if ((back < 0) == side)
	{
		return -1;		// didn't hit anuthing
	}

	// check for impact on this node
	VectorCopy(mid, lightspot);

	mbrush38_surface_t* surf = tr.worldModel->brush38_surfaces + node->firstsurface;
	for (int i = 0; i < node->numsurfaces; i++, surf++)
	{
		if (surf->flags & (BRUSH38_SURF_DRAWTURB | BRUSH38_SURF_DRAWSKY))
		{
			continue;	// no lightmaps
		}

		mbrush38_texinfo_t* tex = surf->texinfo;

		int s = (int)(DotProduct(mid, tex->vecs[0]) + tex->vecs[0][3]);
		int t = (int)(DotProduct(mid, tex->vecs[1]) + tex->vecs[1][3]);

		if (s < surf->texturemins[0] || t < surf->texturemins[1])
		{
			continue;
		}

		int ds = s - surf->texturemins[0];
		int dt = t - surf->texturemins[1];
		
		if (ds > surf->extents[0] || dt > surf->extents[1])
		{
			continue;
		}

		if (!surf->samples)
		{
			return 0;
		}

		ds >>= 4;
		dt >>= 4;

		byte* lightmap = surf->samples;
		VectorCopy(vec3_origin, pointcolor);
		if (lightmap)
		{
			vec3_t scale;

			lightmap += 3 * (dt * ((surf->extents[0] >> 4) + 1) + ds);

			for (int maps = 0; maps < BSP38_MAXLIGHTMAPS && surf->styles[maps] != 255; maps++)
			{
				for (int j = 0; j < 3; j++)
				{
					scale[j] = r_modulate->value * tr.refdef.lightstyles[surf->styles[maps]].rgb[j];
				}

				pointcolor[0] += lightmap[0] * scale[0] * (1.0 / 255);
				pointcolor[1] += lightmap[1] * scale[1] * (1.0 / 255);
				pointcolor[2] += lightmap[2] * scale[2] * (1.0 / 255);
				lightmap += 3 * ((surf->extents[0] >> 4) + 1) * ((surf->extents[1] >> 4) + 1);
			}
		}
		return 1;
	}

	// go down back side
	return RecursiveLightPointQ2(node->children[!side], mid, end);
}

//==========================================================================
//
//	R_LightPointQ2
//
//==========================================================================

void R_LightPointQ2(vec3_t p, vec3_t color)
{
	if (!tr.worldModel->brush38_lightdata)
	{
		color[0] = color[1] = color[2] = 1.0;
		return;
	}

	vec3_t end;
	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 2048;
	
	int r = RecursiveLightPointQ2(tr.worldModel->brush38_nodes, p, end);
	
	if (r == -1)
	{
		VectorCopy(vec3_origin, color);
	}
	else
	{
		VectorCopy(pointcolor, color);
	}

	//
	// add dynamic lights
	//
	dlight_t* dl = tr.refdef.dlights;
	for (int lnum = 0; lnum < tr.refdef.num_dlights; lnum++, dl++)
	{
		vec3_t dist;
		VectorSubtract(tr.currentEntity->e.origin, dl->origin, dist);
		float add = dl->radius - VectorLength(dist);
		add *= (1.0 / 256);
		if (add > 0)
		{
			VectorMA(color, add, dl->color, color);
		}
	}

	VectorScale(color, r_modulate->value, color);
}
