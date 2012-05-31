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

// Ridah, always use the default world for entities
aas_t* defaultaasworld = aasworlds;

void AAS_EntityInfo(int entnum, aas_entityinfo_t* info)
{
	if (!defaultaasworld->initialized)
	{
		BotImport_Print(PRT_FATAL, "AAS_EntityInfo: (*defaultaasworld) not initialized\n");
		Com_Memset(info, 0, sizeof(aas_entityinfo_t));
		return;
	}

	if (entnum < 0 || entnum >= defaultaasworld->maxentities)
	{
		// if it's not a bot game entity, then report it
		if (!(GGameType & GAME_ET) || !(entnum >= defaultaasworld->maxentities && entnum < defaultaasworld->maxentities + NUM_BOTGAMEENTITIES))
		{
			BotImport_Print(PRT_FATAL, "AAS_EntityInfo: entnum %d out of range\n", entnum);
		}
		Com_Memset(info, 0, sizeof(aas_entityinfo_t));
		return;
	}

	Com_Memcpy(info, &defaultaasworld->entities[entnum].i, sizeof(aas_entityinfo_t));
}

void AAS_EntityOrigin(int entnum, vec3_t origin)
{
	if (entnum < 0 || entnum >= defaultaasworld->maxentities)
	{
		BotImport_Print(PRT_FATAL, "AAS_EntityOrigin: entnum %d out of range\n", entnum);
		VectorClear(origin);
		return;
	}

	VectorCopy(defaultaasworld->entities[entnum].i.origin, origin);
}

int AAS_EntityModelindex(int entnum)
{
	if (entnum < 0 || entnum >= defaultaasworld->maxentities)
	{
		BotImport_Print(PRT_FATAL, "AAS_EntityModelindex: entnum %d out of range\n", entnum);
		return 0;
	}
	return defaultaasworld->entities[entnum].i.modelindex;
}

int AAS_EntityType(int entnum)
{
	if (!defaultaasworld->initialized)
	{
		return 0;
	}

	if (entnum < 0 || entnum >= defaultaasworld->maxentities)
	{
		BotImport_Print(PRT_FATAL, "AAS_EntityType: entnum %d out of range\n", entnum);
		return 0;
	}
	return defaultaasworld->entities[entnum].i.type;
}

int AAS_EntityModelNum(int entnum)
{
	if (!defaultaasworld->initialized)
	{
		return 0;
	}

	if (entnum < 0 || entnum >= defaultaasworld->maxentities)
	{
		BotImport_Print(PRT_FATAL, "AAS_EntityModelNum: entnum %d out of range\n", entnum);
		return 0;
	}
	return defaultaasworld->entities[entnum].i.modelindex;
}

bool AAS_OriginOfMoverWithModelNum(int modelnum, vec3_t origin)
{
	for (int i = 0; i < defaultaasworld->maxentities; i++)
	{
		aas_entity_t* ent = &defaultaasworld->entities[i];
		if (ent->i.type == Q3ET_MOVER)
		{
			if (ent->i.modelindex == modelnum)
			{
				VectorCopy(ent->i.origin, origin);
				return true;
			}
		}
	}
	return false;
}

void AAS_ResetEntityLinks()
{
	for (int i = 0; i < defaultaasworld->maxentities; i++)
	{
		defaultaasworld->entities[i].areas = NULL;
		defaultaasworld->entities[i].leaves = NULL;
	}
}

void AAS_InvalidateEntities()
{
	for (int i = 0; i < defaultaasworld->maxentities; i++)
	{
		defaultaasworld->entities[i].i.valid = false;
		defaultaasworld->entities[i].i.number = i;
	}
}

int AAS_NextEntity(int entnum)
{
	if (!defaultaasworld->loaded)
	{
		return 0;
	}

	if (entnum < 0)
	{
		entnum = -1;
	}
	while (++entnum < defaultaasworld->maxentities)
	{
		if (defaultaasworld->entities[entnum].i.valid)
		{
			return entnum;
		}
	}
	return 0;
}

// Ridah, used to find out if there is an entity touching the given area, if so, try and avoid it
bool AAS_IsEntityInArea(int entnumIgnore, int entnumIgnore2, int areanum)
{
	for (aas_link_t* link = aasworld->arealinkedentities[areanum]; link; link = link->next_ent)
	{
		//ignore the pass entity
		if (link->entnum == entnumIgnore)
		{
			continue;
		}
		if (link->entnum == entnumIgnore2)
		{
			continue;
		}

		aas_entity_t* ent = &defaultaasworld->entities[link->entnum];
		if (!ent->i.valid)
		{
			continue;
		}
		if (!ent->i.solid)
		{
			continue;
		}
		return true;
	}
	return false;
}
