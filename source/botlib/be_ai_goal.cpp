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
 * name:		be_ai_goal.c
 *
 * desc:		goal AI
 *
 * $Archive: /MissionPack/code/botlib/be_ai_goal.c $
 *
 *****************************************************************************/

#include "../common/qcommon.h"
#include "botlib.h"
#include "be_interface.h"
#include "be_aas_funcs.h"
#include "../server/botlib/ai_weight.h"
#include "be_ai_goal.h"
#include "be_ai_move.h"

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotInitLevelItems(void)
{
	int i, spawnflags, value;
	char classname[MAX_EPAIRKEY];
	vec3_t origin, end;
	int ent, goalareanum;
	itemconfig_t* ic;
	levelitem_t* li;
	bsp_trace_t trace;

	//initialize the map locations and camp spots
	BotInitInfoEntities();

	//initialize the level item heap
	InitLevelItemHeap();
	levelitems = NULL;
	numlevelitems = 0;
	//
	ic = itemconfig;
	if (!ic)
	{
		return;
	}

	//if there's no AAS file loaded
	if (!AAS_Loaded())
	{
		return;
	}

	//update the modelindexes of the item info
	for (i = 0; i < ic->numiteminfo; i++)
	{
		if (!ic->iteminfo[i].modelindex)
		{
			Log_Write("item %s has modelindex 0", ic->iteminfo[i].classname);
		}	//end if
	}	//end for

	for (ent = AAS_NextBSPEntity(0); ent; ent = AAS_NextBSPEntity(ent))
	{
		if (!AAS_ValueForBSPEpairKey(ent, "classname", classname, MAX_EPAIRKEY))
		{
			continue;
		}
		//
		spawnflags = 0;
		AAS_IntForBSPEpairKey(ent, "spawnflags", &spawnflags);
		//
		for (i = 0; i < ic->numiteminfo; i++)
		{
			if (!String::Cmp(classname, ic->iteminfo[i].classname))
			{
				break;
			}
		}	//end for
		if (i >= ic->numiteminfo)
		{
			Log_Write("entity %s unknown item\r\n", classname);
			continue;
		}	//end if
			//get the origin of the item
		if (!AAS_VectorForBSPEpairKey(ent, "origin", origin))
		{
			BotImport_Print(PRT_ERROR, "item %s without origin\n", classname);
			continue;
		}	//end else
			//
		goalareanum = 0;
		//if it is a floating item
		if (spawnflags & 1)
		{
			//if the item is not floating in water
			if (!(AAS_PointContents(origin) & BSP46CONTENTS_WATER))
			{
				VectorCopy(origin, end);
				end[2] -= 32;
				trace = AAS_Trace(origin, ic->iteminfo[i].mins, ic->iteminfo[i].maxs, end, -1, BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP);
				//if the item not near the ground
				if (trace.fraction >= 1)
				{
					//if the item is not reachable from a jumppad
					goalareanum = AAS_BestReachableFromJumpPadArea(origin, ic->iteminfo[i].mins, ic->iteminfo[i].maxs);
					Log_Write("item %s reachable from jumppad area %d\r\n", ic->iteminfo[i].classname, goalareanum);
					//BotImport_Print(PRT_MESSAGE, "item %s reachable from jumppad area %d\r\n", ic->iteminfo[i].classname, goalareanum);
					if (!goalareanum)
					{
						continue;
					}
				}	//end if
			}	//end if
		}	//end if

		li = AllocLevelItem();
		if (!li)
		{
			return;
		}
		//
		li->number = ++numlevelitems;
		li->timeout = 0;
		li->entitynum = 0;
		//
		li->flags = 0;
		AAS_IntForBSPEpairKey(ent, "notfree", &value);
		if (value)
		{
			li->flags |= IFL_NOTFREE;
		}
		AAS_IntForBSPEpairKey(ent, "notteam", &value);
		if (value)
		{
			li->flags |= IFL_NOTTEAM;
		}
		AAS_IntForBSPEpairKey(ent, "notsingle", &value);
		if (value)
		{
			li->flags |= IFL_NOTSINGLE;
		}
		AAS_IntForBSPEpairKey(ent, "notbot", &value);
		if (value)
		{
			li->flags |= IFL_NOTBOT;
		}
		if (!String::Cmp(classname, "item_botroam"))
		{
			li->flags |= IFL_ROAM;
			AAS_FloatForBSPEpairKey(ent, "weight", &li->weight);
		}	//end if
			//if not a stationary item
		if (!(spawnflags & 1))
		{
			if (!AAS_DropToFloor(origin, ic->iteminfo[i].mins, ic->iteminfo[i].maxs))
			{
				BotImport_Print(PRT_MESSAGE, "%s in solid at (%1.1f %1.1f %1.1f)\n",
					classname, origin[0], origin[1], origin[2]);
			}	//end if
		}	//end if
			//item info of the level item
		li->iteminfo = i;
		//origin of the item
		VectorCopy(origin, li->origin);
		//
		if (goalareanum)
		{
			li->goalareanum = goalareanum;
			VectorCopy(origin, li->goalorigin);
		}	//end if
		else
		{
			//get the item goal area and goal origin
			li->goalareanum = AAS_BestReachableArea(origin,
				ic->iteminfo[i].mins, ic->iteminfo[i].maxs,
				li->goalorigin);
			if (!li->goalareanum)
			{
				BotImport_Print(PRT_MESSAGE, "%s not reachable for bots at (%1.1f %1.1f %1.1f)\n",
					classname, origin[0], origin[1], origin[2]);
			}	//end if
		}	//end else
			//
		AddLevelItemToList(li);
	}	//end for
	BotImport_Print(PRT_MESSAGE, "found %d level items\n", numlevelitems);
}	//end of the function BotInitLevelItems
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================

void BotUpdateEntityItems(void)
{
	int ent, i, modelindex;
	vec3_t dir;
	levelitem_t* li, * nextli;
	aas_entityinfo_t entinfo;
	itemconfig_t* ic;

	//timeout current entity items if necessary
	for (li = levelitems; li; li = nextli)
	{
		nextli = li->next;
		//if it is a item that will time out
		if (li->timeout)
		{
			//timeout the item
			if (li->timeout < AAS_Time())
			{
				RemoveLevelItemFromList(li);
				FreeLevelItem(li);
			}	//end if
		}	//end if
	}	//end for
		//find new entity items
	ic = itemconfig;
	if (!itemconfig)
	{
		return;
	}
	//
	for (ent = AAS_NextEntity(0); ent; ent = AAS_NextEntity(ent))
	{
		if (AAS_EntityType(ent) != Q3ET_ITEM)
		{
			continue;
		}
		//get the model index of the entity
		modelindex = AAS_EntityModelindex(ent);
		//
		if (!modelindex)
		{
			continue;
		}
		//get info about the entity
		AAS_EntityInfo(ent, &entinfo);
		//FIXME: don't do this
		//skip all floating items for now
		//if (entinfo.groundent != Q3ENTITYNUM_WORLD) continue;
		//if the entity is still moving
		if (entinfo.origin[0] != entinfo.lastvisorigin[0] ||
			entinfo.origin[1] != entinfo.lastvisorigin[1] ||
			entinfo.origin[2] != entinfo.lastvisorigin[2])
		{
			continue;
		}
		//check if the entity is already stored as a level item
		for (li = levelitems; li; li = li->next)
		{
			//if the level item is linked to an entity
			if (li->entitynum && li->entitynum == ent)
			{
				//the entity is re-used if the models are different
				if (ic->iteminfo[li->iteminfo].modelindex != modelindex)
				{
					//remove this level item
					RemoveLevelItemFromList(li);
					FreeLevelItem(li);
					li = NULL;
					break;
				}	//end if
				else
				{
					if (entinfo.origin[0] != li->origin[0] ||
						entinfo.origin[1] != li->origin[1] ||
						entinfo.origin[2] != li->origin[2])
					{
						VectorCopy(entinfo.origin, li->origin);
						//also update the goal area number
						li->goalareanum = AAS_BestReachableArea(li->origin,
							ic->iteminfo[li->iteminfo].mins, ic->iteminfo[li->iteminfo].maxs,
							li->goalorigin);
					}	//end if
					break;
				}	//end else
			}	//end if
		}	//end for
		if (li)
		{
			continue;
		}
		//try to link the entity to a level item
		for (li = levelitems; li; li = li->next)
		{
			//if this level item is already linked
			if (li->entitynum)
			{
				continue;
			}
			//
			if (g_gametype == Q3GT_SINGLE_PLAYER)
			{
				if (li->flags & IFL_NOTSINGLE)
				{
					continue;
				}
			}
			else if (g_gametype >= Q3GT_TEAM)
			{
				if (li->flags & IFL_NOTTEAM)
				{
					continue;
				}
			}
			else
			{
				if (li->flags & IFL_NOTFREE)
				{
					continue;
				}
			}
			//if the model of the level item and the entity are the same
			if (ic->iteminfo[li->iteminfo].modelindex == modelindex)
			{
				//check if the entity is very close
				VectorSubtract(li->origin, entinfo.origin, dir);
				if (VectorLength(dir) < 30)
				{
					//found an entity for this level item
					li->entitynum = ent;
					//if the origin is different
					if (entinfo.origin[0] != li->origin[0] ||
						entinfo.origin[1] != li->origin[1] ||
						entinfo.origin[2] != li->origin[2])
					{
						//update the level item origin
						VectorCopy(entinfo.origin, li->origin);
						//also update the goal area number
						li->goalareanum = AAS_BestReachableArea(li->origin,
							ic->iteminfo[li->iteminfo].mins, ic->iteminfo[li->iteminfo].maxs,
							li->goalorigin);
					}	//end if
#ifdef DEBUG
					Log_Write("linked item %s to an entity", ic->iteminfo[li->iteminfo].classname);
#endif	//DEBUG
					break;
				}	//end if
			}	//end else
		}	//end for
		if (li)
		{
			continue;
		}
		//check if the model is from a known item
		for (i = 0; i < ic->numiteminfo; i++)
		{
			if (ic->iteminfo[i].modelindex == modelindex)
			{
				break;
			}	//end if
		}	//end for
			//if the model is not from a known item
		if (i >= ic->numiteminfo)
		{
			continue;
		}
		//allocate a new level item
		li = AllocLevelItem();
		//
		if (!li)
		{
			continue;
		}
		//entity number of the level item
		li->entitynum = ent;
		//number for the level item
		li->number = numlevelitems + ent;
		//set the item info index for the level item
		li->iteminfo = i;
		//origin of the item
		VectorCopy(entinfo.origin, li->origin);
		//get the item goal area and goal origin
		li->goalareanum = AAS_BestReachableArea(li->origin,
			ic->iteminfo[i].mins, ic->iteminfo[i].maxs,
			li->goalorigin);
		//never go for items dropped into jumppads
		if (AAS_AreaJumpPad(li->goalareanum))
		{
			FreeLevelItem(li);
			continue;
		}	//end if
			//time this item out after 30 seconds
			//dropped items disappear after 30 seconds
		li->timeout = AAS_Time() + 30;
		//add the level item to the list
		AddLevelItemToList(li);
		//BotImport_Print(PRT_MESSAGE, "found new level item %s\n", ic->iteminfo[i].classname);
	}	//end for
}	//end of the function BotUpdateEntityItems
//===========================================================================
// pops a new long term goal on the goal stack in the goalstate
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotChooseLTGItem(int goalstate, vec3_t origin, int* inventory, int travelflags)
{
	int areanum, t, weightnum;
	float weight, bestweight, avoidtime;
	iteminfo_t* iteminfo;
	itemconfig_t* ic;
	levelitem_t* li, * bestitem;
	bot_goal_q3_t goal;
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle(goalstate);
	if (!gs)
	{
		return false;
	}
	if (!gs->itemweightconfig)
	{
		return false;
	}
	//get the area the bot is in
	areanum = BotReachabilityArea(origin, gs->client);
	//if the bot is in solid or if the area the bot is in has no reachability links
	if (!areanum || !AAS_AreaReachability(areanum))
	{
		//use the last valid area the bot was in
		areanum = gs->lastreachabilityarea;
	}	//end if
		//remember the last area with reachabilities the bot was in
	gs->lastreachabilityarea = areanum;
	//if still in solid
	if (!areanum)
	{
		return false;
	}
	//the item configuration
	ic = itemconfig;
	if (!itemconfig)
	{
		return false;
	}
	//best weight and item so far
	bestweight = 0;
	bestitem = NULL;
	Com_Memset(&goal, 0, sizeof(bot_goal_q3_t));
	//go through the items in the level
	for (li = levelitems; li; li = li->next)
	{
		if (g_gametype == Q3GT_SINGLE_PLAYER)
		{
			if (li->flags & IFL_NOTSINGLE)
			{
				continue;
			}
		}
		else if (g_gametype >= Q3GT_TEAM)
		{
			if (li->flags & IFL_NOTTEAM)
			{
				continue;
			}
		}
		else
		{
			if (li->flags & IFL_NOTFREE)
			{
				continue;
			}
		}
		if (li->flags & IFL_NOTBOT)
		{
			continue;
		}
		//if the item is not in a possible goal area
		if (!li->goalareanum)
		{
			continue;
		}
		//FIXME: is this a good thing? added this for items that never spawned into the game (f.i. CTF flags in obelisk)
		if (!li->entitynum && !(li->flags & IFL_ROAM))
		{
			continue;
		}
		//get the fuzzy weight function for this item
		iteminfo = &ic->iteminfo[li->iteminfo];
		weightnum = gs->itemweightindex[iteminfo->number];
		if (weightnum < 0)
		{
			continue;
		}

		weight = FuzzyWeightUndecided(inventory, gs->itemweightconfig, weightnum);
		//HACK: to make dropped items more attractive
		if (li->timeout)
		{
			weight += droppedweight->value;
		}
		//use weight scale for item_botroam
		if (li->flags & IFL_ROAM)
		{
			weight *= li->weight;
		}
		//
		if (weight > 0)
		{
			//get the travel time towards the goal area
			t = AAS_AreaTravelTimeToGoalArea(areanum, origin, li->goalareanum, travelflags);
			//if the goal is reachable
			if (t > 0)
			{
				//if this item won't respawn before we get there
				avoidtime = BotAvoidGoalTime(goalstate, li->number);
				if (avoidtime - t * 0.009 > 0)
				{
					continue;
				}
				//
				weight /= (float)t * TRAVELTIME_SCALE;
				//
				if (weight > bestweight)
				{
					bestweight = weight;
					bestitem = li;
				}	//end if
			}	//end if
		}	//end if
	}	//end for
		//if no goal item found
	if (!bestitem)
	{
		/*
		//if not in lava or slime
		if (!AAS_AreaLava(areanum) && !AAS_AreaSlime(areanum))
		{
		    if (AAS_RandomGoalArea(areanum, travelflags, &goal.areanum, goal.origin))
		    {
		        VectorSet(goal.mins, -15, -15, -15);
		        VectorSet(goal.maxs, 15, 15, 15);
		        goal.entitynum = 0;
		        goal.number = 0;
		        goal.flags = GFL_ROAM;
		        goal.iteminfo = 0;
		        //push the goal on the stack
		        BotPushGoalQ3(goalstate, &goal);
		        //
		#ifdef DEBUG
		        BotImport_Print(PRT_MESSAGE, "chosen roam goal area %d\n", goal.areanum);
		#endif //DEBUG
		        return qtrue;
		    } //end if
		} //end if
		*/
		return false;
	}	//end if
		//create a bot goal for this item
	iteminfo = &ic->iteminfo[bestitem->iteminfo];
	VectorCopy(bestitem->goalorigin, goal.origin);
	VectorCopy(iteminfo->mins, goal.mins);
	VectorCopy(iteminfo->maxs, goal.maxs);
	goal.areanum = bestitem->goalareanum;
	goal.entitynum = bestitem->entitynum;
	goal.number = bestitem->number;
	goal.flags = GFL_ITEM;
	if (bestitem->timeout)
	{
		goal.flags |= GFL_DROPPED;
	}
	if (bestitem->flags & IFL_ROAM)
	{
		goal.flags |= GFL_ROAM;
	}
	goal.iteminfo = bestitem->iteminfo;
	//if it's a dropped item
	if (bestitem->timeout)
	{
		avoidtime = AVOID_DROPPED_TIME;
	}	//end if
	else
	{
		avoidtime = iteminfo->respawntime;
		if (!avoidtime)
		{
			avoidtime = AVOID_DEFAULT_TIME;
		}
		if (avoidtime < AVOID_MINIMUM_TIME)
		{
			avoidtime = AVOID_MINIMUM_TIME;
		}
	}	//end else
		//add the chosen goal to the goals to avoid for a while
	BotAddToAvoidGoals(gs, bestitem->number, avoidtime);
	//push the goal on the stack
	BotPushGoalQ3(goalstate, &goal);
	//
	return true;
}	//end of the function BotChooseLTGItem
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotChooseNBGItem(int goalstate, vec3_t origin, int* inventory, int travelflags,
	bot_goal_q3_t* ltg, float maxtime)
{
	int areanum, t, weightnum, ltg_time;
	float weight, bestweight, avoidtime;
	iteminfo_t* iteminfo;
	itemconfig_t* ic;
	levelitem_t* li, * bestitem;
	bot_goal_q3_t goal;
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle(goalstate);
	if (!gs)
	{
		return false;
	}
	if (!gs->itemweightconfig)
	{
		return false;
	}
	//get the area the bot is in
	areanum = BotReachabilityArea(origin, gs->client);
	//if the bot is in solid or if the area the bot is in has no reachability links
	if (!areanum || !AAS_AreaReachability(areanum))
	{
		//use the last valid area the bot was in
		areanum = gs->lastreachabilityarea;
	}	//end if
		//remember the last area with reachabilities the bot was in
	gs->lastreachabilityarea = areanum;
	//if still in solid
	if (!areanum)
	{
		return false;
	}
	//
	if (ltg)
	{
		ltg_time = AAS_AreaTravelTimeToGoalArea(areanum, origin, ltg->areanum, travelflags);
	}
	else
	{
		ltg_time = 99999;
	}
	//the item configuration
	ic = itemconfig;
	if (!itemconfig)
	{
		return false;
	}
	//best weight and item so far
	bestweight = 0;
	bestitem = NULL;
	Com_Memset(&goal, 0, sizeof(bot_goal_q3_t));
	//go through the items in the level
	for (li = levelitems; li; li = li->next)
	{
		if (g_gametype == Q3GT_SINGLE_PLAYER)
		{
			if (li->flags & IFL_NOTSINGLE)
			{
				continue;
			}
		}
		else if (g_gametype >= Q3GT_TEAM)
		{
			if (li->flags & IFL_NOTTEAM)
			{
				continue;
			}
		}
		else
		{
			if (li->flags & IFL_NOTFREE)
			{
				continue;
			}
		}
		if (li->flags & IFL_NOTBOT)
		{
			continue;
		}
		//if the item is in a possible goal area
		if (!li->goalareanum)
		{
			continue;
		}
		//FIXME: is this a good thing? added this for items that never spawned into the game (f.i. CTF flags in obelisk)
		if (!li->entitynum && !(li->flags & IFL_ROAM))
		{
			continue;
		}
		//get the fuzzy weight function for this item
		iteminfo = &ic->iteminfo[li->iteminfo];
		weightnum = gs->itemweightindex[iteminfo->number];
		if (weightnum < 0)
		{
			continue;
		}
		//
		weight = FuzzyWeightUndecided(inventory, gs->itemweightconfig, weightnum);
		//HACK: to make dropped items more attractive
		if (li->timeout)
		{
			weight += droppedweight->value;
		}
		//use weight scale for item_botroam
		if (li->flags & IFL_ROAM)
		{
			weight *= li->weight;
		}
		//
		if (weight > 0)
		{
			//get the travel time towards the goal area
			t = AAS_AreaTravelTimeToGoalArea(areanum, origin, li->goalareanum, travelflags);
			//if the goal is reachable
			if (t > 0 && t < maxtime)
			{
				//if this item won't respawn before we get there
				avoidtime = BotAvoidGoalTime(goalstate, li->number);
				if (avoidtime - t * 0.009 > 0)
				{
					continue;
				}
				//
				weight /= (float)t * TRAVELTIME_SCALE;
				//
				if (weight > bestweight)
				{
					t = 0;
					if (ltg && !li->timeout)
					{
						//get the travel time from the goal to the long term goal
						t = AAS_AreaTravelTimeToGoalArea(li->goalareanum, li->goalorigin, ltg->areanum, travelflags);
					}	//end if
						//if the travel back is possible and doesn't take too long
					if (t <= ltg_time)
					{
						bestweight = weight;
						bestitem = li;
					}	//end if
				}	//end if
			}	//end if
		}	//end if
	}	//end for
		//if no goal item found
	if (!bestitem)
	{
		return false;
	}
	//create a bot goal for this item
	iteminfo = &ic->iteminfo[bestitem->iteminfo];
	VectorCopy(bestitem->goalorigin, goal.origin);
	VectorCopy(iteminfo->mins, goal.mins);
	VectorCopy(iteminfo->maxs, goal.maxs);
	goal.areanum = bestitem->goalareanum;
	goal.entitynum = bestitem->entitynum;
	goal.number = bestitem->number;
	goal.flags = GFL_ITEM;
	if (bestitem->timeout)
	{
		goal.flags |= GFL_DROPPED;
	}
	if (bestitem->flags & IFL_ROAM)
	{
		goal.flags |= GFL_ROAM;
	}
	goal.iteminfo = bestitem->iteminfo;
	//if it's a dropped item
	if (bestitem->timeout)
	{
		avoidtime = AVOID_DROPPED_TIME;
	}	//end if
	else
	{
		avoidtime = iteminfo->respawntime;
		if (!avoidtime)
		{
			avoidtime = AVOID_DEFAULT_TIME;
		}
		if (avoidtime < AVOID_MINIMUM_TIME)
		{
			avoidtime = AVOID_MINIMUM_TIME;
		}
	}	//end else
		//add the chosen goal to the goals to avoid for a while
	BotAddToAvoidGoals(gs, bestitem->number, avoidtime);
	//push the goal on the stack
	BotPushGoalQ3(goalstate, &goal);
	//
	return true;
}	//end of the function BotChooseNBGItem
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotItemGoalInVisButNotVisible(int viewer, vec3_t eye, vec3_t viewangles, bot_goal_q3_t* goal)
{
	aas_entityinfo_t entinfo;
	bsp_trace_t trace;
	vec3_t middle;

	if (!(goal->flags & GFL_ITEM))
	{
		return false;
	}
	//
	VectorAdd(goal->mins, goal->mins, middle);
	VectorScale(middle, 0.5, middle);
	VectorAdd(goal->origin, middle, middle);
	//
	trace = AAS_Trace(eye, NULL, NULL, middle, viewer, BSP46CONTENTS_SOLID);
	//if the goal middle point is visible
	if (trace.fraction >= 1)
	{
		//the goal entity number doesn't have to be valid
		//just assume it's valid
		if (goal->entitynum <= 0)
		{
			return false;
		}
		//
		//if the entity data isn't valid
		AAS_EntityInfo(goal->entitynum, &entinfo);
		//NOTE: for some wacko reason entities are sometimes
		// not updated
		//if (!entinfo.valid) return qtrue;
		if (entinfo.ltime < AAS_Time() - 0.5)
		{
			return true;
		}
	}	//end if
	return false;
}	//end of the function BotItemGoalInVisButNotVisible
