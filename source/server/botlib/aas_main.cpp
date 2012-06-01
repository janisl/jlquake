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

#include "../server.h"
#include "local.h"

aas_t aasworlds[MAX_AAS_WORLDS];
aas_t* aasworld;

libvar_t* saveroutingcache;

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

void AAS_SetInitialized()
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

int AAS_LoadFiles(const char* mapname)
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
