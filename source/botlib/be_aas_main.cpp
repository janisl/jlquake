/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
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

/*****************************************************************************
 * name:		be_aas_main.c
 *
 * desc:		AAS
 *
 * $Archive: /MissionPack/code/botlib/be_aas_main.c $
 *
 *****************************************************************************/

#include "../common/qcommon.h"
#include "botlib.h"
#include "be_interface.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"

libvar_t* saveroutingcache;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_SetInitialized(void)
{
	aasworld->initialized = true;
	BotImport_Print(PRT_MESSAGE, "AAS initialized.\n");
}	//end of the function AAS_SetInitialized
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_ContinueInit(float time)
{
	//if no AAS file loaded
	if (!aasworld->loaded)
	{
		return;
	}
	//if AAS is already initialized
	if (aasworld->initialized)
	{
		return;
	}
	//calculate reachability, if not finished return
	if (AAS_ContinueInitReachability(time))
	{
		return;
	}
	//initialize clustering for the new map
	AAS_InitClustering();
	//if reachability has been calculated and an AAS file should be written
	//or there is a forced data optimization
	if (aasworld->savefile || ((int)LibVarGetValue("forcewrite")))
	{
		//optimize the AAS data
		if ((int)LibVarValue("aasoptimize", "0"))
		{
			AAS_Optimize();
		}
		//save the AAS file
		if (AAS_WriteAASFile(aasworld->filename))
		{
			BotImport_Print(PRT_MESSAGE, "%s written succesfully\n", aasworld->filename);
		}	//end if
		else
		{
			BotImport_Print(PRT_ERROR, "couldn't write %s\n", aasworld->filename);
		}	//end else
	}	//end if
		//initialize the routing
	AAS_InitRouting();
	//at this point AAS is initialized
	AAS_SetInitialized();
}	//end of the function AAS_ContinueInit
//===========================================================================
// called at the start of every frame
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_StartFrame(float time)
{
	aasworld->time = time;
	//unlink all entities that were not updated last frame
	AAS_UnlinkInvalidEntities();
	//invalidate the entities
	AAS_InvalidateEntities();
	//initialize AAS
	AAS_ContinueInit(time);
	//
	aasworld->frameroutingupdates = 0;
	//
	if (bot_developer)
	{
		if (LibVarGetValue("showcacheupdates"))
		{
			AAS_RoutingInfo();
			LibVarSet("showcacheupdates", "0");
		}	//end if
	}	//end if
		//
	if (saveroutingcache->value)
	{
		AAS_WriteRouteCache();
		LibVarSet("saveroutingcache", "0");
	}	//end if
		//
	aasworld->numframes++;
	return BLERR_NOERROR;
}	//end of the function AAS_StartFrame
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_LoadFiles(const char* mapname)
{
	int errnum;
	char aasfile[MAX_PATH];
//	char bspfile[MAX_PATH];

	String::Cpy(aasworld->mapname, mapname);
	//NOTE: first reset the entity links into the AAS areas and BSP leaves
	// the AAS link heap and BSP link heap are reset after respectively the
	// AAS file and BSP file are loaded
	AAS_ResetEntityLinks();
	// load bsp info
	AAS_LoadBSPFile();

	//load the aas file
	String::Sprintf(aasfile, MAX_PATH, "maps/%s.aas", mapname);
	errnum = AAS_LoadAASFile(aasfile);
	if (errnum != BLERR_NOERROR)
	{
		return errnum;
	}

	BotImport_Print(PRT_MESSAGE, "loaded %s\n", aasfile);
	String::NCpy(aasworld->filename, aasfile, MAX_PATH);
	return BLERR_NOERROR;
}	//end of the function AAS_LoadFiles
//===========================================================================
// called everytime a map changes
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_LoadMap(const char* mapname)
{
	int errnum;

	//if no mapname is provided then the string indexes are updated
	if (!mapname)
	{
		return 0;
	}	//end if
		//
	aasworld->initialized = false;
	//NOTE: free the routing caches before loading a new map because
	// to free the caches the old number of areas, number of clusters
	// and number of areas in a clusters must be available
	AAS_FreeRoutingCaches();
	//load the map
	errnum = AAS_LoadFiles(mapname);
	if (errnum != BLERR_NOERROR)
	{
		aasworld->loaded = false;
		return errnum;
	}	//end if
		//
	AAS_InitSettings();
	//initialize the AAS link heap for the new map
	AAS_InitAASLinkHeap();
	//initialize the AAS linked entities for the new map
	AAS_InitAASLinkedEntities();
	//initialize reachability for the new map
	AAS_InitReachability();
	//initialize the alternative routing
	AAS_InitAlternativeRouting();
	//everything went ok
	return 0;
}	//end of the function AAS_LoadMap
//===========================================================================
// called when the library is first loaded
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_Setup(void)
{
	aasworld = &aasworlds[0];

	aasworld->maxclients = (int)LibVarValue("maxclients", "128");
	aasworld->maxentities = (int)LibVarValue("maxentities", "1024");
	// as soon as it's set to 1 the routing cache will be saved
	saveroutingcache = LibVar("saveroutingcache", "0");
	//allocate memory for the entities
	if (aasworld->entities)
	{
		Mem_Free(aasworld->entities);
	}
	aasworld->entities = (aas_entity_t*)Mem_ClearedAlloc(aasworld->maxentities * sizeof(aas_entity_t));
	//invalidate all the entities
	AAS_InvalidateEntities();
	//force some recalculations
	//LibVarSet("forceclustering", "1");			//force clustering calculation
	//LibVarSet("forcereachability", "1");		//force reachability calculation
	aasworld->numframes = 0;
	return BLERR_NOERROR;
}	//end of the function AAS_Setup
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_Shutdown(void)
{
	AAS_ShutdownAlternativeRouting();
	//
	AAS_DumpBSPData();
	//free routing caches
	AAS_FreeRoutingCaches();
	//free aas link heap
	AAS_FreeAASLinkHeap();
	//free aas linked entities
	AAS_FreeAASLinkedEntities();
	//free the aas data
	AAS_DumpAASData();
	//free the entities
	if (aasworld->entities)
	{
		Mem_Free(aasworld->entities);
	}
	//clear the (*aasworld) structure
	Com_Memset(&(*aasworld), 0, sizeof(aas_t));
	//aas has not been initialized
	aasworld->initialized = false;
	//NOTE: as soon as a new .bsp file is loaded the .bsp file memory is
	// freed an reallocated, so there's no need to free that memory here
	//print shutdown
	BotImport_Print(PRT_MESSAGE, "AAS shutdown.\n");
}	//end of the function AAS_Shutdown
