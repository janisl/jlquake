/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


/*****************************************************************************
 * name:		be_aas_entity.c
 *
 * desc:		AAS entities
 *
 *
 *****************************************************************************/

#include "../game/q_shared.h"
#include "l_memory.h"
#include "l_utils.h"
#include "../game/botlib.h"
#include "be_interface.h"
#include "../game/be_aas.h"
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
		return WOLFBLERR_NOAASFILE;
	}	//end if

	ent = &defaultaasworld->entities[entnum];

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
	//ent->i.event = state->event;
	ent->i.eventParm = state->eventParm;
	ent->i.powerups = state->powerups;
	ent->i.weapon = state->weapon;
	ent->i.legsAnim = state->legsAnim;
	ent->i.torsoAnim = state->torsoAnim;

//	ent->i.weapAnim = state->weapAnim;	//----(SA)
//----(SA)	didn't want to comment in as I wasn't sure of any implications of changing the aas_entityinfo_t and bot_entitystate_t structures.

	//number of the entity
	ent->i.number = entnum;
	//updated so set valid flag
	ent->i.valid = qtrue;
	//link everything the first frame

	if (defaultaasworld->numframes == 1)
	{
		relink = qtrue;
	}
	else
	{
		relink = qfalse;
	}

	//
	if (ent->i.solid == SOLID_BSP)
	{
		//if the angles of the model changed
		if (!VectorCompare(state->angles, ent->i.angles))
		{
			VectorCopy(state->angles, ent->i.angles);
			relink = qtrue;
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
			relink = qtrue;
		}	//end if
	}	//end if
		//if the origin changed
	if (!VectorCompare(state->origin, ent->i.origin))
	{
		VectorCopy(state->origin, ent->i.origin);
		relink = qtrue;
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
int AAS_BestReachableEntityArea(int entnum)
{
	aas_entity_t* ent;

	ent = &defaultaasworld->entities[entnum];
	return AAS_BestReachableLinkArea(ent->areas);
}	//end of the function AAS_BestReachableEntityArea

/*
=============
AAS_SetAASBlockingEntity
=============
*/
void AAS_SetAASBlockingEntity(vec3_t absmin, vec3_t absmax, qboolean blocking)
{
	int areas[128];
	int numareas, i, w;
	//
	// check for resetting AAS blocking
	if (VectorCompare(absmin, absmax) && blocking < 0)
	{
		for (w = 0; w < MAX_AAS_WORLDS; w++)
		{
			AAS_SetCurrentWorld(w);
			//
			if (!(*aasworld).loaded)
			{
				continue;
			}
			// now clear blocking status
			for (i = 1; i < (*aasworld).numareas; i++)
			{
				AAS_EnableRoutingArea(i, qtrue);
			}
		}
		//
		return;
	}
	//
	for (w = 0; w < MAX_AAS_WORLDS; w++)
	{
		AAS_SetCurrentWorld(w);
		//
		if (!(*aasworld).loaded)
		{
			continue;
		}
		// grab the list of areas
		numareas = AAS_BBoxAreas(absmin, absmax, areas, 128);
		// now set their blocking status
		for (i = 0; i < numareas; i++)
		{
			AAS_EnableRoutingArea(areas[i], !blocking);
		}
	}
}
