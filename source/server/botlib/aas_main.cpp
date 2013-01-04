//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../../common/qcommon.h"
#include "local.h"

aas_t aasworlds[MAX_AAS_WORLDS];
aas_t* aasworld;

static libvar_t* saveroutingcache;

void AAS_Error(const char* fmt, ...)
{
	char str[1024];
	va_list arglist;

	va_start(arglist, fmt);
	Q_vsnprintf(str, sizeof(str), fmt, arglist);
	va_end(arglist);
	BotImport_Print(PRT_FATAL, "%s", str);
}

bool AAS_Loaded()
{
	return aasworld->loaded;
}

bool AAS_Initialized()
{
	return aasworld->initialized;
}

static void AAS_SetInitialized()
{
	aasworld->initialized = true;
	BotImport_Print(PRT_MESSAGE, "AAS initialized.\n");
}

float AAS_Time()
{
	return aasworld->time;
}

void AAS_SetCurrentWorld(int index)
{
	if (index >= MAX_AAS_WORLDS || index < 0)
	{
		AAS_Error("AAS_SetCurrentWorld: index out of range\n");
		return;
	}

	// set the current world pointer
	aasworld = &aasworlds[index];
}

// called when the library is first loaded
int AAS_Setup()
{
	// Ridah, just use the default world for entities
	AAS_SetCurrentWorld(0);

	aasworld->maxclients = (int)LibVarValue("maxclients", "128");
	aasworld->maxentities = (int)LibVarValue("maxentities", "1024");
	if (GGameType & GAME_Quake3)
	{
		// as soon as it's set to 1 the routing cache will be saved
		saveroutingcache = LibVar("saveroutingcache", "0");
	}
	//allocate memory for the entities
	if (aasworld->entities)
	{
		Mem_Free(aasworld->entities);
	}
	aasworld->entities = (aas_entity_t*)Mem_ClearedAlloc(aasworld->maxentities * sizeof(aas_entity_t));
	//invalidate all the entities
	AAS_InvalidateEntities();
	aasworld->numframes = 0;
	return BLERR_NOERROR;
}

void AAS_Shutdown()
{
	// Ridah, do each of the worlds
	for (int i = 0; i < MAX_AAS_WORLDS; i++)
	{
		AAS_SetCurrentWorld(i);

		AAS_ShutdownAlternativeRouting();

		AAS_DumpBSPData();
		//free routing caches
		AAS_FreeRoutingCaches();
		//free aas link heap
		AAS_FreeAASLinkHeap();
		//free aas linked entities
		AAS_FreeAASLinkedEntities();
		//free the aas data
		AAS_DumpAASData();

		if (i == 0)
		{
			//free the entities
			if (aasworld->entities)
			{
				Mem_Free(aasworld->entities);
			}
		}

		//clear the (*aasworld) structure
		Com_Memset(&(*aasworld), 0, sizeof(aas_t));
		//aas has not been initialized
		aasworld->initialized = false;
	}

	//NOTE: as soon as a new .bsp file is loaded the .bsp file memory is
	// freed an reallocated, so there's no need to free that memory here
	//print shutdown
	BotImport_Print(PRT_MESSAGE, "AAS shutdown.\n");
}

static void AAS_ContinueInit(float time)
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
		if (GGameType & GAME_Quake3 ? (int)LibVarValue("aasoptimize", "0") : !(int)LibVarValue("nooptimize", "1"))
		{
			AAS_Optimize();
		}
		//save the AAS file
		if (AAS_WriteAASFile(aasworld->filename))
		{
			BotImport_Print(PRT_MESSAGE, "%s written succesfully\n", aasworld->filename);
		}
		else
		{
			BotImport_Print(PRT_ERROR, "couldn't write %s\n", aasworld->filename);
		}
	}
	//initialize the routing
	AAS_InitRouting();
	//at this point AAS is initialized
	AAS_SetInitialized();
}

// called at the start of every frame
int AAS_StartFrame(float time)
{
	// Ridah, do each of the aasworlds
	int numWorlds = (GGameType & (GAME_WolfSP | GAME_WolfMP)) ? MAX_AAS_WORLDS : 1;
	for (int i = 0; i < numWorlds; i++)
	{
		AAS_SetCurrentWorld(i);

		aasworld->time = time;
		if (GGameType & GAME_Quake3)
		{
			//unlink all entities that were not updated last frame
			AAS_UnlinkInvalidEntities();
		}
		//invalidate the entities
		AAS_InvalidateEntities();
		//initialize AAS
		AAS_ContinueInit(time);

		aasworld->frameroutingupdates = 0;
	}

	if (GGameType & GAME_Quake3)
	{
		if (bot_developer)
		{
			if (LibVarGetValue("showcacheupdates"))
			{
				AAS_RoutingInfo();
				LibVarSet("showcacheupdates", "0");
			}
		}

		if (saveroutingcache->value)
		{
			AAS_WriteRouteCache();
			LibVarSet("saveroutingcache", "0");
		}
	}

	aasworld->numframes++;
	return BLERR_NOERROR;
}

static int AAS_LoadFiles(const char* mapname)
{
	int errnum;
	char aasfile[MAX_QPATH];

	String::Cpy(aasworld->mapname, mapname);
	//NOTE: first reset the entity links into the AAS areas and BSP leaves
	// the AAS link heap and BSP link heap are reset after respectively the
	// AAS file and BSP file are loaded
	AAS_ResetEntityLinks();

	// load bsp info
	AAS_LoadBSPFile();

	//load the aas file
	String::Sprintf(aasfile, MAX_QPATH, "maps/%s.aas", mapname);
	errnum = AAS_LoadAASFile(aasfile);
	if (errnum != BLERR_NOERROR)
	{
		return errnum;
	}

	BotImport_Print(PRT_MESSAGE, "loaded %s\n", aasfile);
	String::NCpy(aasworld->filename, aasfile, MAX_QPATH);
	return BLERR_NOERROR;
}

// called everytime a map changes
int AAS_LoadMap(const char* mapname)
{
	//if no mapname is provided then the string indexes are updated
	if (!mapname)
	{
		return 0;
	}

	bool loaded = false;
	int missingErrNum = 0;
	int numWorlds = (GGameType & (GAME_WolfSP | GAME_WolfMP)) ? MAX_AAS_WORLDS : 1;
	for (int i = 0; i < numWorlds; i++)
	{
		AAS_SetCurrentWorld(i);

		char this_mapname[MAX_QPATH];
		if (GGameType & (GAME_WolfSP | GAME_WolfMP))
		{
			String::Sprintf(this_mapname, MAX_QPATH, "%s_b%i", mapname, i);
		}
		else
		{
			String::NCpyZ(this_mapname, mapname, MAX_QPATH);
		}

		aasworld->initialized = false;
		//NOTE: free the routing caches before loading a new map because
		// to free the caches the old number of areas, number of clusters
		// and number of areas in a clusters must be available
		AAS_FreeRoutingCaches();
		//load the map
		int errnum = AAS_LoadFiles(this_mapname);
		if (errnum != BLERR_NOERROR)
		{
			aasworld->loaded = false;
			// RF, we are allowed to skip one of the files, but not both
			missingErrNum = errnum;
			continue;
		}

		loaded = true;

		AAS_InitSettings();
		//initialize the AAS link heap for the new map
		AAS_InitAASLinkHeap();
		//initialize the AAS linked entities for the new map
		AAS_InitAASLinkedEntities();
		//initialize reachability for the new map
		AAS_InitReachability();
		//initialize the alternative routing
		AAS_InitAlternativeRouting();
	}

	if (!loaded)
	{
		return missingErrNum;
	}

	//everything went ok
	return 0;
}
