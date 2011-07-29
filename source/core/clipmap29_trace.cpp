//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "core.h"
#include "clipmap29_local.h"

// MACROS ------------------------------------------------------------------

// 1/32 epsilon to keep floating point happy
#define DIST_EPSILON	(0.03125)

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
//  QClipMap29::HullCheckQ1
//
//==========================================================================

bool QClipMap29::HullCheckQ1(clipHandle_t Handle, vec3_t p1, vec3_t p2, q1trace_t * trace)
{
	chull_t *hull = ClipHandleToHull(Handle);
	return RecursiveHullCheck(hull, hull->firstclipnode, 0, 1, p1, p2, trace);
}

//==========================================================================
//
//  QClipMap29::RecursiveHullCheck
//
//==========================================================================

bool QClipMap29::RecursiveHullCheck(chull_t * hull, int num, float p1f,
	float p2f, vec3_t p1, vec3_t p2, q1trace_t * trace)
{
	// check for empty
	if (num < 0)
	{
		if (num != BSP29CONTENTS_SOLID)
		{
			trace->allsolid = false;
			if (num == BSP29CONTENTS_EMPTY)
			{
				trace->inopen = true;
			}
			else
			{
				trace->inwater = true;
			}
		}
		else
		{
			trace->startsolid = true;
		}
		return true;			// empty
	}

	if (num < hull->firstclipnode || num > hull->lastclipnode)
	{
		throw QException("SV_RecursiveHullCheck: bad node number");
	}

	//
	// find the point distances
	//
	cclipnode_t* node = clipnodes + num;
	cplane_t *plane = planes + node->planenum;

	float t1, t2;
	if (plane->type < 3)
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
	}
	else
	{
		t1 = DotProduct(plane->normal, p1) - plane->dist;
		t2 = DotProduct(plane->normal, p2) - plane->dist;
	}

	if (t1 >= 0 && t2 >= 0)
	{
		return RecursiveHullCheck(hull, node->children[0], p1f, p2f, p1, p2, trace);
	}
	if (t1 < 0 && t2 < 0)
	{
		return RecursiveHullCheck(hull, node->children[1], p1f, p2f, p1, p2, trace);
	}

	// put the crosspoint DIST_EPSILON pixels on the near side
	float frac;
	if (t1 < 0)
	{
		frac = (t1 + DIST_EPSILON) / (t1 - t2);
	}
	else
	{
		frac = (t1 - DIST_EPSILON) / (t1 - t2);
	}
	if (frac < 0)
	{
		frac = 0;
	}
	if (frac > 1)
	{
		frac = 1;
	}

	float midf = p1f + (p2f - p1f) * frac;
	vec3_t mid;
	for (int i = 0; i < 3; i++)
	{
		mid[i] = p1[i] + frac * (p2[i] - p1[i]);
	}

	int side = (t1 < 0);

	// move up to the node
	if (!RecursiveHullCheck(hull, node->children[side], p1f, midf, p1, mid, trace))
	{
		return false;
	}

#ifdef PARANOID
	if (HullPointContents(hull, node->children[side], mid) == BSP29CONTENTS_SOLID)
	{
		GLog.Write("mid PointInHullSolid\n");
		return false;
	}
#endif

	if (HullPointContents(hull, node->children[side ^ 1], mid) != BSP29CONTENTS_SOLID)
	{
		// go past the node
		return RecursiveHullCheck(hull, node->children[side ^ 1], midf, p2f,
								  mid, p2, trace);
	}

	if (trace->allsolid)
	{
		return false;			// never got out of the solid area
	}

	//==================
	// the other side of the node is solid, this is the impact point
	//==================
	if (!side)
	{
		VectorCopy(plane->normal, trace->plane.normal);
		trace->plane.dist = plane->dist;
	}
	else
	{
		VectorSubtract(vec3_origin, plane->normal, trace->plane.normal);
		trace->plane.dist = -plane->dist;
	}

	while (HullPointContents(hull, hull->firstclipnode, mid) == BSP29CONTENTS_SOLID)
	{
		// shouldn't really happen, but does occasionally
		frac -= 0.1;
		if (frac < 0)
		{
			trace->fraction = midf;
			VectorCopy(mid, trace->endpos);
			GLog.DWrite("backup past 0\n");
			return false;
		}
		midf = p1f + (p2f - p1f) * frac;
		for (int i = 0; i < 3; i++)
		{
			mid[i] = p1[i] + frac * (p2[i] - p1[i]);
		}
	}

	trace->fraction = midf;
	VectorCopy(mid, trace->endpos);

	return false;
}

//==========================================================================
//
//	QClipMap29::BoxTraceQ2
//
//==========================================================================

q2trace_t QClipMap29::BoxTraceQ2(vec3_t Start, vec3_t End,
	vec3_t Mins, vec3_t Maxs, clipHandle_t Model, int BrushMask)
{
	throw QDropException("Not implemented");
}

//==========================================================================
//
//	QClipMap29::TransformedBoxTraceQ2
//
//==========================================================================

q2trace_t QClipMap29::TransformedBoxTraceQ2(vec3_t Start, vec3_t End,
	vec3_t Mins, vec3_t Maxs, clipHandle_t Model, int BrushMask, vec3_t Origin, vec3_t Angles)
{
	throw QDropException("Not implemented");
}

//==========================================================================
//
//	QClipMap29::BoxTraceQ3
//
//==========================================================================

void QClipMap29::BoxTraceQ3(q3trace_t *Results, const vec3_t Start, const vec3_t End,
	vec3_t Mins, vec3_t Maxs, clipHandle_t Model, int BrushMask, int Capsule)
{
	throw QDropException("Not implemented");
}

//==========================================================================
//
//	QClipMap29::TransformedBoxTraceQ3
//
//==========================================================================

void QClipMap29::TransformedBoxTraceQ3(q3trace_t *Results, const vec3_t Start,
	const vec3_t End, vec3_t Mins, vec3_t Maxs, clipHandle_t Model, int BrushMask,
	const vec3_t Origin, const vec3_t Angles, int Capsule)
{
	throw QDropException("Not implemented");
}
