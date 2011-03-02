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

#include "../../../libs/core/core.h"
#include "../../../libs/core/bsp46file.h"
#include "../../../libs/core/cm46_local.h"
#include "cm_public.h"

QClipMap46*	CMap;

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
}
