/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cmodel.c -- model loading

#include "qcommon.h"
#include "../../../libs/core/cm38_local.h"


static QClipMap38*	CMap;

/*
===============================================================================

					MAP LOADING

===============================================================================
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

/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
clipHandle_t CM_LoadMap(const char* name, bool clientload, unsigned* checksum)
{
	map_noareas = Cvar_Get("map_noareas", "0", 0);

	if (CMap && !QStr::Cmp(CMap->name, name) && (clientload || !Cvar_VariableValue("flushmap")))
	{
		// still have the right version
		if (!clientload)
		{
			CMap->ClearPortalOpen();
		}
	}
	else
	{
		// free old stuff
		CM_ClearMap();

		CMap = new QClipMap38();
		CMapShared = CMap;

		CMap->LoadMap(name);
	}

	*checksum = CMap->checksum;
	return 0;
}
