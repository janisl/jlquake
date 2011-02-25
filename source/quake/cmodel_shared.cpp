//**************************************************************************
//**
//**	$Id: endian.cpp 101 2010-04-03 23:06:31Z dj_jl $
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

#include "../../libs/core/cm29_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static QClipMap29*	CMap;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	CM_Init
//
//==========================================================================

void CM_Init()
{
}

//==========================================================================
//
//	CM_LoadMap
//
//==========================================================================

clipHandle_t CM_LoadMap(const char* name, bool clientload, int* checksum)
{
	if (!name[0])
	{
		throw QException("CM_ForName: NULL name");
	}

	if (CMap)
	{
		if (!QStr::Cmp(CMap->Map.map_models[0].name, name))
		{
			//	Already got it.
			return 0;
		}

		delete CMap;
	}

	CMap = new QClipMap29;
	CMapShared = CMap;
	CMap->LoadModel(name);

	return 0;
}

//==========================================================================
//
//	CM_PrecacheModel
//
//==========================================================================

clipHandle_t CM_PrecacheModel(const char* name)
{
	if (!name[0])
	{
		Sys_Error("CM_ForName: NULL name");
	}

	//
	// search the currently loaded models
	//
	for (int i = 0; i < CMap->numknown; i++)
	{
		if (!QStr::Cmp(CMap->known[i]->model.name, name))
		{
			return (MAX_MAP_MODELS + i) * MAX_MAP_HULLS;
		}
	}

	if (CMap->numknown == MAX_CMOD_KNOWN)
	{
		Sys_Error("mod_numknown == MAX_CMOD_KNOWN");
	}
	CMap->known[CMap->numknown] = new QClipModelNonMap29;
	QClipModelNonMap29* LoadCMap = CMap->known[CMap->numknown];
	CMap->numknown++;

	LoadCMap->Load(name);

	return (MAX_MAP_MODELS + CMap->numknown - 1) * MAX_MAP_HULLS;
}

//==========================================================================
//
//	CM_RecursiveHullCheck
//
//==========================================================================

// 1/32 epsilon to keep floating point happy
#define	DIST_EPSILON	(0.03125)

static bool CM_RecursiveHullCheck(chull_t* hull, int num, float p1f, float p2f,
	vec3_t p1, vec3_t p2, q1trace_t* trace)
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
		return true;		// empty
	}

	if (num < hull->firstclipnode || num > hull->lastclipnode)
	{
		Sys_Error("SV_RecursiveHullCheck: bad node number");
	}

	//
	// find the point distances
	//
	bsp29_dclipnode_t* node = hull->clipnodes + num;
	cplane_t* plane = hull->planes + node->planenum;

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
		return CM_RecursiveHullCheck(hull, node->children[0], p1f, p2f, p1, p2, trace);
	}
	if (t1 < 0 && t2 < 0)
	{
		return CM_RecursiveHullCheck(hull, node->children[1], p1f, p2f, p1, p2, trace);
	}

	// put the crosspoint DIST_EPSILON pixels on the near side
	float frac;
	if (t1 < 0)
	{
		frac = (t1 + DIST_EPSILON) / (t1 - t2);
	}
	else
	{
		frac = (t1 - DIST_EPSILON)/ (t1 - t2);
	}
	if (frac < 0)
	{
		frac = 0;
	}
	if (frac > 1)
	{
		frac = 1;
	}

	float midf = p1f + (p2f - p1f)*frac;
	vec3_t mid;
	for (int i = 0; i < 3; i++)
	{
		mid[i] = p1[i] + frac * (p2[i] - p1[i]);
	}

	int side = (t1 < 0);

	// move up to the node
	if (!CM_RecursiveHullCheck(hull, node->children[side], p1f, midf, p1, mid, trace))
	{
		return false;
	}

#ifdef PARANOID
	if (CM_HullPointContents(hull, node->children[side], mid) == BSP29CONTENTS_SOLID)
	{
		GLog.Write("mid PointInHullSolid\n");
		return false;
	}
#endif

	if (CMap->HullPointContents(hull, node->children[side ^ 1], mid) != BSP29CONTENTS_SOLID)
	{
		// go past the node
		return CM_RecursiveHullCheck (hull, node->children[side^1], midf, p2f, mid, p2, trace);
	}

	if (trace->allsolid)
	{
		return false;		// never got out of the solid area
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

	while (CMap->HullPointContents(hull, hull->firstclipnode, mid) == BSP29CONTENTS_SOLID)
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
			mid[i] = p1[i] + frac*(p2[i] - p1[i]);
		}
	}

	trace->fraction = midf;
	VectorCopy(mid, trace->endpos);

	return false;
}

//==========================================================================
//
//	CM_HullCheck
//
//==========================================================================

bool CM_HullCheck(clipHandle_t Handle, vec3_t p1, vec3_t p2, q1trace_t* trace)
{
	chull_t* hull = CMap->ClipHandleToHull(Handle);
	return CM_RecursiveHullCheck(hull, hull->firstclipnode, 0, 1, p1, p2, trace);
}

//==========================================================================
//
//	CM_TraceLine
//
//==========================================================================

int CM_TraceLine(vec3_t start, vec3_t end, q1trace_t* trace)
{
	return CM_RecursiveHullCheck(&CMap->Map.map_models[0].hulls[0], 0, 0, 1, start, end, trace);
}

//==========================================================================
//
//	CM_CalcPHS
//
//	Calculates the PHS (Potentially Hearable Set)
//
//==========================================================================

void CM_CalcPHS()
{
	CMap->CalcPHS();
}
