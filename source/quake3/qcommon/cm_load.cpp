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
	if (!name || !name[0])
	{
		throw QDropException("CM_LoadMap: NULL name");
	}

	GLog.DWrite("CM_LoadMap( %s, %i )\n", name, clientload);

	if (CMap && !QStr::Cmp(CMap->name, name) && clientload)
	{
		*checksum = CMap->checksum;
		return;
	}

	// free old stuff
	CM_ClearMap();

	//
	// load the file
	//
	QArray<quint8> Buffer;
	int length = FS_ReadFile(name, Buffer);

	if (!length)
	{
		throw QDropException(va("Couldn't load %s", name));
	}

	CMap = new QClipMap46;
	CMapShared = CMap;
	CMap->LoadMap(name, Buffer);
	*checksum = CMap->checksum;
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
