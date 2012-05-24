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
 * name:		be_aas_entity.c
 *
 * desc:		AAS entities
 *
 * $Archive: /MissionPack/code/botlib/be_aas_entity.c $
 *
 *****************************************************************************/

#include "../common/qcommon.h"
#include "l_memory.h"
#include "l_utils.h"
#include "botlib.h"
#include "be_interface.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_UpdateEntity(int entnum, bot_entitystate_t* state)
{
	int relink;
	aas_entity_t* ent;
	vec3_t absmins, absmaxs;

	if (!defaultaasworld->loaded)
	{
		BotImport_Print(PRT_MESSAGE, "AAS_UpdateEntity: not loaded\n");
		return Q3BLERR_NOAASFILE;
	}	//end if

	ent = &defaultaasworld->entities[entnum];

	if (!state)
	{
		//unlink the entity
		AAS_UnlinkFromAreas(ent->areas);
		//
		ent->areas = NULL;
		//
		ent->leaves = NULL;
		return BLERR_NOERROR;
	}

	ent->i.update_time = AAS_Time() - ent->i.ltime;
	ent->i.type = state->type;
	ent->i.flags = state->flags;
	ent->i.ltime = AAS_Time();
	VectorCopy(ent->i.origin, ent->i.lastvisorigin);
	VectorCopy(state->old_origin, ent->i.old_origin);
	ent->i.solid = state->solid;
	ent->i.groundent = state->groundent;
	ent->i.modelindex = state->modelindex;
	ent->i.modelindex2 = state->modelindex2;
	ent->i.frame = state->frame;
	ent->i.event = state->event;
	ent->i.eventParm = state->eventParm;
	ent->i.powerups = state->powerups;
	ent->i.weapon = state->weapon;
	ent->i.legsAnim = state->legsAnim;
	ent->i.torsoAnim = state->torsoAnim;
	//number of the entity
	ent->i.number = entnum;
	//updated so set valid flag
	ent->i.valid = true;
	//link everything the first frame
	if (defaultaasworld->numframes == 1)
	{
		relink = true;
	}
	else
	{
		relink = false;
	}
	//
	if (ent->i.solid == SOLID_BSP)
	{
		//if the angles of the model changed
		if (!VectorCompare(state->angles, ent->i.angles))
		{
			VectorCopy(state->angles, ent->i.angles);
			relink = true;
		}	//end if
			//get the mins and maxs of the model
			//FIXME: rotate mins and maxs
		AAS_BSPModelMinsMaxs(ent->i.modelindex, ent->i.angles, ent->i.mins, ent->i.maxs);
	}	//end if
	else if (ent->i.solid == SOLID_BBOX)
	{
		//if the bounding box size changed
		if (!VectorCompare(state->mins, ent->i.mins) ||
			!VectorCompare(state->maxs, ent->i.maxs))
		{
			VectorCopy(state->mins, ent->i.mins);
			VectorCopy(state->maxs, ent->i.maxs);
			relink = true;
		}	//end if
		VectorCopy(state->angles, ent->i.angles);
	}	//end if
		//if the origin changed
	if (!VectorCompare(state->origin, ent->i.origin))
	{
		VectorCopy(state->origin, ent->i.origin);
		relink = true;
	}	//end if
		//if the entity should be relinked
	if (relink)
	{
		//don't link the world model
		if (entnum != Q3ENTITYNUM_WORLD)
		{
			//absolute mins and maxs
			VectorAdd(ent->i.mins, ent->i.origin, absmins);
			VectorAdd(ent->i.maxs, ent->i.origin, absmaxs);
			//unlink the entity
			AAS_UnlinkFromAreas(ent->areas);
			//relink the entity to the AAS areas (use the larges bbox)
			ent->areas = AAS_LinkEntityClientBBox(absmins, absmaxs, entnum, PRESENCE_NORMAL);
			//link the entity to the world BSP tree
			ent->leaves = NULL;
		}	//end if
	}	//end if
	return BLERR_NOERROR;
}	//end of the function AAS_UpdateEntity
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_UnlinkInvalidEntities(void)
{
	int i;
	aas_entity_t* ent;

	for (i = 0; i < defaultaasworld->maxentities; i++)
	{
		ent = &defaultaasworld->entities[i];
		if (!ent->i.valid)
		{
			AAS_UnlinkFromAreas(ent->areas);
			ent->areas = NULL;
			ent->leaves = NULL;
		}	//end for
	}	//end for
}	//end of the function AAS_UnlinkInvalidEntities
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_BestReachableEntityArea(int entnum)
{
	aas_entity_t* ent;

	ent = &defaultaasworld->entities[entnum];
	return AAS_BestReachableLinkArea(ent->areas);
}	//end of the function AAS_BestReachableEntityArea
