/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// cmodel.c -- model loading

#include "cm_local.h"

#define	LL(x) x=LittleLong(x)


QClipMap46*	CMap;

void	CM_FloodAreaConnections (void);

//==================================================================

/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
void CM_LoadMap( const char *name, qboolean clientload, int *checksum )
{
	if ( !name || !name[0] ) {
		throw QDropException("CM_LoadMap: NULL name" );
	}

	cm_noAreas = Cvar_Get ("cm_noAreas", "0", CVAR_CHEAT);
	cm_noCurves = Cvar_Get ("cm_noCurves", "0", CVAR_CHEAT);
	cm_playerCurveClip = Cvar_Get ("cm_playerCurveClip", "1", CVAR_ARCHIVE|CVAR_CHEAT );
	GLog.DWrite("CM_LoadMap( %s, %i )\n", name, clientload);

	if (CMap && !QStr::Cmp(CMap->name, name) && clientload)
	{
		*checksum = CMap->checksum;
		return;
	}

	// free old stuff
	CM_ClearMap();

	CMap = new QClipMap46;
	CMapShared = CMap;

	if (!name[0])
	{
		CMap->numLeafs = 1;
		CMap->numClusters = 1;
		CMap->numAreas = 1;
		CMap->cmodels = new cmodel_t[1];
		Com_Memset(CMap->cmodels, 0, sizeof(cmodel_t));
		*checksum = 0;
		return;
	}

	CMap->LoadMap(name);
	*checksum = CMap->checksum;

	CMap->FloodAreaConnections();

	// allow this to be cached if it is loaded by the server
	if (!clientload)
	{
		QStr::NCpyZ(CMap->name, name, sizeof(CMap->name));
	}
}

/*
==================
CM_ClearMap
==================
*/
void CM_ClearMap()
{
	if (CMap)
	{
		delete CMap;
		CMap = NULL;
		CMapShared = NULL;
	}
	CM_ClearLevelPatches();
}

/*
==================
CM_BoxTrace
==================
*/
void CM_BoxTrace( q3trace_t *Results, const vec3_t Start, const vec3_t End,
						  vec3_t Mins, vec3_t Maxs,
						  clipHandle_t Model, int BrushMask, int Capsule)
{
	CMap->BoxTraceQ3(Results, Start, End, Mins, Maxs, Model, BrushMask, Capsule);
}

/*
==================
CM_TransformedBoxTrace

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
void CM_TransformedBoxTrace(q3trace_t *Results, const vec3_t Start, const vec3_t End,
						  vec3_t Mins, vec3_t Maxs,
						  clipHandle_t Model, int BrushMask,
						  const vec3_t Origin, const vec3_t Angles, int Capsule)
{
	CMap->TransformedBoxTraceQ3(Results, Start, End, Mins, Maxs, Model, BrushMask, Origin, Angles, Capsule);
}
