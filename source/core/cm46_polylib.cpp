//**************************************************************************
//**
//**	$Id$
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

#include "core.h"
#include "cm46_local.h"

// MACROS ------------------------------------------------------------------

#define	MAX_POINTS_ON_WINDING	64

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
//	AllocWinding
//
//==========================================================================

static winding_t* AllocWinding(int points)
{
	int s = sizeof(vec_t) * 3 * points + sizeof(int);
	winding_t* w = (winding_t*)Mem_Alloc(s);
	Com_Memset(w, 0, s); 
	return w;
}

//==========================================================================
//
//	CM46_FreeWinding
//
//==========================================================================

void CM46_FreeWinding(winding_t* w)
{
	if (*(unsigned *)w == 0xdeaddead)
	{
		throw QException("FreeWinding: freed a freed winding");
	}
	*(unsigned *)w = 0xdeaddead;

	Mem_Free(w);
}

//==========================================================================
//
//	CM46_CopyWinding
//
//==========================================================================

winding_t* CM46_CopyWinding(winding_t* w)
{
	winding_t* c = AllocWinding(w->numpoints);
	int size = (int)(qintptr)((winding_t*)0)->p[w->numpoints];
	Com_Memcpy(c, w, size);
	return c;
}

//==========================================================================
//
//	CM46_BaseWindingForPlane
//
//==========================================================================

winding_t* CM46_BaseWindingForPlane(vec3_t normal, vec_t dist)
{
	// find the major axis

	vec_t max = -MAX_MAP_BOUNDS;
	int x = -1;
	for (int i = 0; i < 3; i++)
	{
		vec_t v = fabs(normal[i]);
		if (v > max)
		{
			x = i;
			max = v;
		}
	}
	if (x==-1)
	{
		throw QDropException("BaseWindingForPlane: no axis found");
	}

	vec3_t vup;
	VectorCopy(vec3_origin, vup);
	switch (x)
	{
	case 0:
	case 1:
		vup[2] = 1;
		break;		
	case 2:
		vup[0] = 1;
		break;		
	}

	vec_t v = DotProduct(vup, normal);
	VectorMA(vup, -v, normal, vup);
	VectorNormalize2(vup, vup);

	vec3_t org;
	VectorScale(normal, dist, org);

	vec3_t vright;
	CrossProduct(vup, normal, vright);

	VectorScale(vup, MAX_MAP_BOUNDS, vup);
	VectorScale(vright, MAX_MAP_BOUNDS, vright);

	// project a really big	axis aligned box onto the plane
	winding_t* w = AllocWinding(4);

	VectorSubtract(org, vright, w->p[0]);
	VectorAdd(w->p[0], vup, w->p[0]);

	VectorAdd(org, vright, w->p[1]);
	VectorAdd(w->p[1], vup, w->p[1]);

	VectorAdd(org, vright, w->p[2]);
	VectorSubtract(w->p[2], vup, w->p[2]);

	VectorSubtract(org, vright, w->p[3]);
	VectorSubtract(w->p[3], vup, w->p[3]);

	w->numpoints = 4;

	return w;	
}

//==========================================================================
//
//	CM46_ChopWindingInPlace
//
//==========================================================================

void CM46_ChopWindingInPlace(winding_t** inout, vec3_t normal, vec_t dist, vec_t epsilon)
{
	winding_t* in = *inout;
	int counts[3];
	counts[0] = counts[1] = counts[2] = 0;

	// determine sides for each point
	vec_t dists[MAX_POINTS_ON_WINDING + 4];
	int sides[MAX_POINTS_ON_WINDING + 4];
	for (int i = 0; i < in->numpoints; i++)
	{
		vec_t dot = DotProduct(in->p[i], normal);
		dot -= dist;
		dists[i] = dot;
		if (dot > epsilon)
		{
			sides[i] = SIDE_FRONT;
		}
		else if (dot < -epsilon)
		{
			sides[i] = SIDE_BACK;
		}
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[in->numpoints] = sides[0];
	dists[in->numpoints] = dists[0];

	if (!counts[0])
	{
		CM46_FreeWinding(in);
		*inout = NULL;
		return;
	}
	if (!counts[1])
	{
		return;		// inout stays the same
	}

	int maxpts = in->numpoints + 4;	// cant use counts[0]+2 because
									// of fp grouping errors

	winding_t* f = AllocWinding(maxpts);

	for (int i = 0; i < in->numpoints; i++)
	{
		vec_t* p1 = in->p[i];

		if (sides[i] == SIDE_ON)
		{
			VectorCopy(p1, f->p[f->numpoints]);
			f->numpoints++;
			continue;
		}
	
		if (sides[i] == SIDE_FRONT)
		{
			VectorCopy(p1, f->p[f->numpoints]);
			f->numpoints++;
		}

		if (sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
		{
			continue;
		}

		// generate a split point
		vec_t* p2 = in->p[(i + 1) % in->numpoints];
		
		vec_t dot = dists[i] / (dists[i] - dists[i + 1]);
		vec3_t mid;
		for (int j = 0; j < 3; j++)
		{
			// avoid round off error when possible
			if (normal[j] == 1)
			{
				mid[j] = dist;
			}
			else if (normal[j] == -1)
			{
				mid[j] = -dist;
			}
			else
			{
				mid[j] = p1[j] + dot * (p2[j] - p1[j]);
			}
		}

		VectorCopy(mid, f->p[f->numpoints]);
		f->numpoints++;
	}
	
	if (f->numpoints > maxpts)
	{
		throw QDropException("ClipWinding: points exceeded estimate");
	}
	if (f->numpoints > MAX_POINTS_ON_WINDING)
	{
		throw QDropException("ClipWinding: MAX_POINTS_ON_WINDING");
	}

	CM46_FreeWinding(in);
	*inout = f;
}

//==========================================================================
//
//	CM46_WindingBounds
//
//==========================================================================

void CM46_WindingBounds(winding_t* w, vec3_t mins, vec3_t maxs)
{
	mins[0] = mins[1] = mins[2] = MAX_MAP_BOUNDS;
	maxs[0] = maxs[1] = maxs[2] = -MAX_MAP_BOUNDS;

	for (int i = 0; i < w->numpoints; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			vec_t v = w->p[i][j];
			if (v < mins[j])
			{
				mins[j] = v;
			}
			if (v > maxs[j])
			{
				maxs[j] = v;
			}
		}
	}
}
