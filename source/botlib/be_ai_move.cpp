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
 * name:		be_ai_move.c
 *
 * desc:		bot movement AI
 *
 * $Archive: /MissionPack/code/botlib/be_ai_move.c $
 *
 *****************************************************************************/

#include "../common/qcommon.h"
#include "botlib.h"
#include "be_interface.h"
#include "be_aas.h"
#include "be_aas_funcs.h"

#include "be_ea.h"
#include "be_ai_goal.h"
#include "be_ai_move.h"

#define AVOIDREACH

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotFuzzyPointReachabilityArea(vec3_t origin)
{
	int firstareanum, j, x, y, z;
	int areas[10], numareas, areanum, bestareanum;
	float dist, bestdist;
	vec3_t points[10], v, end;

	firstareanum = 0;
	areanum = AAS_PointAreaNum(origin);
	if (areanum)
	{
		firstareanum = areanum;
		if (AAS_AreaReachability(areanum))
		{
			return areanum;
		}
	}	//end if
	VectorCopy(origin, end);
	end[2] += 4;
	numareas = AAS_TraceAreas(origin, end, areas, points, 10);
	for (j = 0; j < numareas; j++)
	{
		if (AAS_AreaReachability(areas[j]))
		{
			return areas[j];
		}
	}	//end for
	bestdist = 999999;
	bestareanum = 0;
	for (z = 1; z >= -1; z -= 1)
	{
		for (x = 1; x >= -1; x -= 1)
		{
			for (y = 1; y >= -1; y -= 1)
			{
				VectorCopy(origin, end);
				end[0] += x * 8;
				end[1] += y * 8;
				end[2] += z * 12;
				numareas = AAS_TraceAreas(origin, end, areas, points, 10);
				for (j = 0; j < numareas; j++)
				{
					if (AAS_AreaReachability(areas[j]))
					{
						VectorSubtract(points[j], origin, v);
						dist = VectorLength(v);
						if (dist < bestdist)
						{
							bestareanum = areas[j];
							bestdist = dist;
						}	//end if
					}	//end if
					if (!firstareanum)
					{
						firstareanum = areas[j];
					}
				}	//end for
			}	//end for
		}	//end for
		if (bestareanum)
		{
			return bestareanum;
		}
	}	//end for
	return firstareanum;
}	//end of the function BotFuzzyPointReachabilityArea
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotReachabilityArea(vec3_t origin, int client)
{
	int modelnum, modeltype, reachnum, areanum;
	aas_reachability_t reach;
	vec3_t org, end, mins, maxs, up = {0, 0, 1};
	bsp_trace_t bsptrace;
	aas_trace_t trace;

	//check if the bot is standing on something
	AAS_PresenceTypeBoundingBox(PRESENCE_CROUCH, mins, maxs);
	VectorMA(origin, -3, up, end);
	bsptrace = AAS_Trace(origin, mins, maxs, end, client, BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP);
	if (!bsptrace.startsolid && bsptrace.fraction < 1 && bsptrace.ent != Q3ENTITYNUM_NONE)
	{
		//if standing on the world the bot should be in a valid area
		if (bsptrace.ent == Q3ENTITYNUM_WORLD)
		{
			return BotFuzzyPointReachabilityArea(origin);
		}	//end if

		modelnum = AAS_EntityModelindex(bsptrace.ent);
		modeltype = modeltypes[modelnum];

		//if standing on a func_plat or func_bobbing then the bot is assumed to be
		//in the area the reachability points to
		if (modeltype == MODELTYPE_FUNC_PLAT || modeltype == MODELTYPE_FUNC_BOB)
		{
			reachnum = AAS_NextModelReachability(0, modelnum);
			if (reachnum)
			{
				AAS_ReachabilityFromNum(reachnum, &reach);
				return reach.areanum;
			}	//end if
		}	//end else if

		//if the bot is swimming the bot should be in a valid area
		if (AAS_Swimming(origin))
		{
			return BotFuzzyPointReachabilityArea(origin);
		}	//end if
			//
		areanum = BotFuzzyPointReachabilityArea(origin);
		//if the bot is in an area with reachabilities
		if (areanum && AAS_AreaReachability(areanum))
		{
			return areanum;
		}
		//trace down till the ground is hit because the bot is standing on some other entity
		VectorCopy(origin, org);
		VectorCopy(org, end);
		end[2] -= 800;
		trace = AAS_TraceClientBBox(org, end, PRESENCE_CROUCH, -1);
		if (!trace.startsolid)
		{
			VectorCopy(trace.endpos, org);
		}	//end if
			//
		return BotFuzzyPointReachabilityArea(org);
	}	//end if
		//
	return BotFuzzyPointReachabilityArea(origin);
}	//end of the function BotReachabilityArea
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotOnMover(vec3_t origin, int entnum, aas_reachability_t* reach)
{
	int i, modelnum;
	vec3_t mins, maxs, modelorigin, org, end;
	vec3_t angles = {0, 0, 0};
	vec3_t boxmins = {-16, -16, -8}, boxmaxs = {16, 16, 8};
	bsp_trace_t trace;

	modelnum = reach->facenum & 0x0000FFFF;
	//get some bsp model info
	AAS_BSPModelMinsMaxs(modelnum, angles, mins, maxs);
	//
	if (!AAS_OriginOfMoverWithModelNum(modelnum, modelorigin))
	{
		BotImport_Print(PRT_MESSAGE, "no entity with model %d\n", modelnum);
		return false;
	}	//end if
		//
	for (i = 0; i < 2; i++)
	{
		if (origin[i] > modelorigin[i] + maxs[i] + 16)
		{
			return false;
		}
		if (origin[i] < modelorigin[i] + mins[i] - 16)
		{
			return false;
		}
	}	//end for
		//
	VectorCopy(origin, org);
	org[2] += 24;
	VectorCopy(origin, end);
	end[2] -= 48;
	//
	trace = AAS_Trace(org, boxmins, boxmaxs, end, entnum, BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP);
	if (!trace.startsolid && !trace.allsolid)
	{
		//NOTE: the reachability face number is the model number of the elevator
		if (trace.ent != Q3ENTITYNUM_NONE && AAS_EntityModelNum(trace.ent) == modelnum)
		{
			return true;
		}	//end if
	}	//end if
	return false;
}	//end of the function BotOnMover
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotOnTopOfEntity(bot_movestate_t* ms)
{
	vec3_t mins, maxs, end, up = {0, 0, 1};
	bsp_trace_t trace;

	AAS_PresenceTypeBoundingBox(ms->presencetype, mins, maxs);
	VectorMA(ms->origin, -3, up, end);
	trace = AAS_Trace(ms->origin, mins, maxs, end, ms->entitynum, BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP);
	if (!trace.startsolid && (trace.ent != Q3ENTITYNUM_WORLD && trace.ent != Q3ENTITYNUM_NONE))
	{
		return trace.ent;
	}	//end if
	return -1;
}	//end of the function BotOnTopOfEntity
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotVisible(int ent, vec3_t eye, vec3_t target)
{
	bsp_trace_t trace;

	trace = AAS_Trace(eye, NULL, NULL, target, ent, BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP);
	if (trace.fraction >= 1)
	{
		return true;
	}
	return false;
}	//end of the function BotVisible
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotPredictVisiblePosition(vec3_t origin, int areanum, bot_goal_q3_t* goal, int travelflags, vec3_t target)
{
	aas_reachability_t reach;
	int reachnum, lastgoalareanum, lastareanum, i;
	int avoidreach[MAX_AVOIDREACH];
	float avoidreachtimes[MAX_AVOIDREACH];
	int avoidreachtries[MAX_AVOIDREACH];
	vec3_t end;

	//if the bot has no goal or no last reachability
	if (!goal)
	{
		return false;
	}
	//if the areanum is not valid
	if (!areanum)
	{
		return false;
	}
	//if the goal areanum is not valid
	if (!goal->areanum)
	{
		return false;
	}

	Com_Memset(avoidreach, 0, MAX_AVOIDREACH * sizeof(int));
	lastgoalareanum = goal->areanum;
	lastareanum = areanum;
	VectorCopy(origin, end);
	//only do 20 hops
	for (i = 0; i < 20 && (areanum != goal->areanum); i++)
	{
		//
		reachnum = BotGetReachabilityToGoal(end, areanum,
			lastgoalareanum, lastareanum,
			avoidreach, avoidreachtimes, avoidreachtries,
			reinterpret_cast<bot_goal_t*>(goal), travelflags, travelflags, NULL, 0, NULL);
		if (!reachnum)
		{
			return false;
		}
		AAS_ReachabilityFromNum(reachnum, &reach);
		//
		if (BotVisible(goal->entitynum, goal->origin, reach.start))
		{
			VectorCopy(reach.start, target);
			return true;
		}	//end if
			//
		if (BotVisible(goal->entitynum, goal->origin, reach.end))
		{
			VectorCopy(reach.end, target);
			return true;
		}	//end if
			//
		if (reach.areanum == goal->areanum)
		{
			VectorCopy(reach.end, target);
			return true;
		}	//end if
			//
		lastareanum = areanum;
		areanum = reach.areanum;
		VectorCopy(reach.end, end);
		//
	}	//end while
		//
	return false;
}	//end of the function BotPredictVisiblePosition
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
float BotGapDistance(vec3_t origin, vec3_t hordir, int entnum)
{
	float dist, startz;
	vec3_t start, end;
	aas_trace_t trace;

	//do gap checking
	startz = origin[2];
	//this enables walking down stairs more fluidly
	{
		VectorCopy(origin, start);
		VectorCopy(origin, end);
		end[2] -= 60;
		trace = AAS_TraceClientBBox(start, end, PRESENCE_CROUCH, entnum);
		if (trace.fraction >= 1)
		{
			return 1;
		}
		startz = trace.endpos[2] + 1;
	}
	//
	for (dist = 8; dist <= 100; dist += 8)
	{
		VectorMA(origin, dist, hordir, start);
		start[2] = startz + 24;
		VectorCopy(start, end);
		end[2] -= 48 + sv_maxbarrier->value;
		trace = AAS_TraceClientBBox(start, end, PRESENCE_CROUCH, entnum);
		//if solid is found the bot can't walk any further and fall into a gap
		if (!trace.startsolid)
		{
			//if it is a gap
			if (trace.endpos[2] < startz - sv_maxstep->value - 8)
			{
				VectorCopy(trace.endpos, end);
				end[2] -= 20;
				if (AAS_PointContents(end) & BSP46CONTENTS_WATER)
				{
					break;
				}
				//if a gap is found slow down
				//BotImport_Print(PRT_MESSAGE, "gap at %f\n", dist);
				return dist;
			}	//end if
			startz = trace.endpos[2];
		}	//end if
	}	//end for
	return 0;
}	//end of the function BotGapDistance
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotCheckBarrierJump(bot_movestate_t* ms, vec3_t dir, float speed)
{
	vec3_t start, hordir, end;
	aas_trace_t trace;

	VectorCopy(ms->origin, end);
	end[2] += sv_maxbarrier->value;
	//trace right up
	trace = AAS_TraceClientBBox(ms->origin, end, PRESENCE_NORMAL, ms->entitynum);
	//this shouldn't happen... but we check anyway
	if (trace.startsolid)
	{
		return false;
	}
	//if very low ceiling it isn't possible to jump up to a barrier
	if (trace.endpos[2] - ms->origin[2] < sv_maxstep->value)
	{
		return false;
	}
	//
	hordir[0] = dir[0];
	hordir[1] = dir[1];
	hordir[2] = 0;
	VectorNormalize(hordir);
	VectorMA(ms->origin, ms->thinktime * speed * 0.5, hordir, end);
	VectorCopy(trace.endpos, start);
	end[2] = trace.endpos[2];
	//trace from previous trace end pos horizontally in the move direction
	trace = AAS_TraceClientBBox(start, end, PRESENCE_NORMAL, ms->entitynum);
	//again this shouldn't happen
	if (trace.startsolid)
	{
		return false;
	}
	//
	VectorCopy(trace.endpos, start);
	VectorCopy(trace.endpos, end);
	end[2] = ms->origin[2];
	//trace down from the previous trace end pos
	trace = AAS_TraceClientBBox(start, end, PRESENCE_NORMAL, ms->entitynum);
	//if solid
	if (trace.startsolid)
	{
		return false;
	}
	//if no obstacle at all
	if (trace.fraction >= 1.0)
	{
		return false;
	}
	//if less than the maximum step height
	if (trace.endpos[2] - ms->origin[2] < sv_maxstep->value)
	{
		return false;
	}
	//
	EA_Jump(ms->client);
	EA_Move(ms->client, hordir, speed);
	ms->moveflags |= MFL_BARRIERJUMP;
	//there is a barrier
	return true;
}	//end of the function BotCheckBarrierJump
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotSwimInDirection(bot_movestate_t* ms, vec3_t dir, float speed, int type)
{
	vec3_t normdir;

	VectorCopy(dir, normdir);
	VectorNormalize(normdir);
	EA_Move(ms->client, normdir, speed);
	return true;
}	//end of the function BotSwimInDirection
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotWalkInDirection(bot_movestate_t* ms, vec3_t dir, float speed, int type)
{
	vec3_t hordir, cmdmove, velocity, tmpdir, origin;
	int presencetype, maxframes, cmdframes, stopevent;
	aas_clientmove_t move;
	float dist;

	if (AAS_OnGround(ms->origin, ms->presencetype, ms->entitynum))
	{
		ms->moveflags |= MFL_ONGROUND;
	}
	//if the bot is on the ground
	if (ms->moveflags & MFL_ONGROUND)
	{
		//if there is a barrier the bot can jump on
		if (BotCheckBarrierJump(ms, dir, speed))
		{
			return true;
		}
		//remove barrier jump flag
		ms->moveflags &= ~MFL_BARRIERJUMP;
		//get the presence type for the movement
		if ((type & MOVE_CROUCH) && !(type & MOVE_JUMP))
		{
			presencetype = PRESENCE_CROUCH;
		}
		else
		{
			presencetype = PRESENCE_NORMAL;
		}
		//horizontal direction
		hordir[0] = dir[0];
		hordir[1] = dir[1];
		hordir[2] = 0;
		VectorNormalize(hordir);
		//if the bot is not supposed to jump
		if (!(type & MOVE_JUMP))
		{
			//if there is a gap, try to jump over it
			if (BotGapDistance(ms->origin, hordir, ms->entitynum) > 0)
			{
				type |= MOVE_JUMP;
			}
		}	//end if
			//get command movement
		VectorScale(hordir, speed, cmdmove);
		VectorCopy(ms->velocity, velocity);
		//
		if (type & MOVE_JUMP)
		{
			//BotImport_Print(PRT_MESSAGE, "trying jump\n");
			cmdmove[2] = 400;
			maxframes = PREDICTIONTIME_JUMP / 0.1;
			cmdframes = 1;
			stopevent = SE_HITGROUND | SE_HITGROUNDDAMAGE |
						SE_ENTERWATER | SE_ENTERSLIME | SE_ENTERLAVA;
		}	//end if
		else
		{
			maxframes = 2;
			cmdframes = 2;
			stopevent = SE_HITGROUNDDAMAGE |
						SE_ENTERWATER | SE_ENTERSLIME | SE_ENTERLAVA;
		}	//end else
			//AAS_ClearShownDebugLines();
			//
		VectorCopy(ms->origin, origin);
		origin[2] += 0.5;
		AAS_PredictClientMovement(&move, ms->entitynum, origin, presencetype, true,
			velocity, cmdmove, cmdframes, maxframes, 0.1f,
			stopevent, 0, false);						//qtrue);
		//if prediction time wasn't enough to fully predict the movement
		if (move.frames >= maxframes && (type & MOVE_JUMP))
		{
			//BotImport_Print(PRT_MESSAGE, "client %d: max prediction frames\n", ms->client);
			return false;
		}	//end if
			//don't enter slime or lava and don't fall from too high
		if (move.stopevent & (SE_ENTERSLIME | SE_ENTERLAVA | SE_HITGROUNDDAMAGE))
		{
			//BotImport_Print(PRT_MESSAGE, "client %d: would be hurt ", ms->client);
			//if (move.stopevent & SE_ENTERSLIME) BotImport_Print(PRT_MESSAGE, "slime\n");
			//if (move.stopevent & SE_ENTERLAVA) BotImport_Print(PRT_MESSAGE, "lava\n");
			//if (move.stopevent & SE_HITGROUNDDAMAGE) BotImport_Print(PRT_MESSAGE, "hitground\n");
			return false;
		}	//end if
			//if ground was hit
		if (move.stopevent & SE_HITGROUND)
		{
			//check for nearby gap
			VectorNormalize2(move.velocity, tmpdir);
			dist = BotGapDistance(move.endpos, tmpdir, ms->entitynum);
			if (dist > 0)
			{
				return false;
			}
			//
			dist = BotGapDistance(move.endpos, hordir, ms->entitynum);
			if (dist > 0)
			{
				return false;
			}
		}	//end if
			//get horizontal movement
		tmpdir[0] = move.endpos[0] - ms->origin[0];
		tmpdir[1] = move.endpos[1] - ms->origin[1];
		tmpdir[2] = 0;
		//
		//the bot is blocked by something
		if (VectorLength(tmpdir) < speed * ms->thinktime * 0.5)
		{
			return false;
		}
		//perform the movement
		if (type & MOVE_JUMP)
		{
			EA_Jump(ms->client);
		}
		if (type & MOVE_CROUCH)
		{
			EA_Crouch(ms->client);
		}
		EA_Move(ms->client, hordir, speed);
		//movement was succesfull
		return true;
	}	//end if
	else
	{
		if (ms->moveflags & MFL_BARRIERJUMP)
		{
			//if near the top or going down
			if (ms->velocity[2] < 50)
			{
				EA_Move(ms->client, dir, speed);
			}	//end if
		}	//end if
			//FIXME: do air control to avoid hazards
		return true;
	}	//end else
}	//end of the function BotWalkInDirection
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotMoveInDirection(int movestate, vec3_t dir, float speed, int type)
{
	bot_movestate_t* ms;

	ms = BotMoveStateFromHandle(movestate);
	if (!ms)
	{
		return false;
	}
	//if swimming
	if (AAS_Swimming(ms->origin))
	{
		return BotSwimInDirection(ms, dir, speed, type);
	}	//end if
	else
	{
		return BotWalkInDirection(ms, dir, speed, type);
	}	//end else
}	//end of the function BotMoveInDirection
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotCheckBlocked(bot_movestate_t* ms, vec3_t dir, int checkbottom, bot_moveresult_t* result)
{
	vec3_t mins, maxs, end, up = {0, 0, 1};
	bsp_trace_t trace;

	//test for entities obstructing the bot's path
	AAS_PresenceTypeBoundingBox(ms->presencetype, mins, maxs);
	//
	if (fabs(DotProduct(dir, up)) < 0.7)
	{
		mins[2] += sv_maxstep->value;	//if the bot can step on
		maxs[2] -= 10;	//a little lower to avoid low ceiling
	}	//end if
	VectorMA(ms->origin, 3, dir, end);
	trace = AAS_Trace(ms->origin, mins, maxs, end, ms->entitynum, BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP | BSP46CONTENTS_BODY);
	//if not started in solid and not hitting the world entity
	if (!trace.startsolid && (trace.ent != Q3ENTITYNUM_WORLD && trace.ent != Q3ENTITYNUM_NONE))
	{
		result->blocked = true;
		result->blockentity = trace.ent;
#ifdef DEBUG
		//BotImport_Print(PRT_MESSAGE, "%d: BotCheckBlocked: I'm blocked\n", ms->client);
#endif	//DEBUG
	}	//end if
		//if not in an area with reachability
	else if (checkbottom && !AAS_AreaReachability(ms->areanum))
	{
		//check if the bot is standing on something
		AAS_PresenceTypeBoundingBox(ms->presencetype, mins, maxs);
		VectorMA(ms->origin, -3, up, end);
		trace = AAS_Trace(ms->origin, mins, maxs, end, ms->entitynum, BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP);
		if (!trace.startsolid && (trace.ent != Q3ENTITYNUM_WORLD && trace.ent != Q3ENTITYNUM_NONE))
		{
			result->blocked = true;
			result->blockentity = trace.ent;
			result->flags |= MOVERESULT_ONTOPOFOBSTACLE;
#ifdef DEBUG
			//BotImport_Print(PRT_MESSAGE, "%d: BotCheckBlocked: I'm blocked\n", ms->client);
#endif	//DEBUG
		}	//end if
	}	//end else
}	//end of the function BotCheckBlocked
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotTravel_Walk(bot_movestate_t* ms, aas_reachability_t* reach)
{
	float dist, speed;
	vec3_t hordir;
	bot_moveresult_t result;

	BotClearMoveResult(&result);
	//first walk straight to the reachability start
	hordir[0] = reach->start[0] - ms->origin[0];
	hordir[1] = reach->start[1] - ms->origin[1];
	hordir[2] = 0;
	dist = VectorNormalize(hordir);
	//
	BotCheckBlocked(ms, hordir, true, &result);
	//
	if (dist < 10)
	{
		//walk straight to the reachability end
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
		dist = VectorNormalize(hordir);
	}	//end if
		//if going towards a crouch area
	if (!(AAS_AreaPresenceType(reach->areanum) & PRESENCE_NORMAL))
	{
		//if pretty close to the reachable area
		if (dist < 20)
		{
			EA_Crouch(ms->client);
		}
	}	//end if
		//
	dist = BotGapDistance(ms->origin, hordir, ms->entitynum);
	//
	if (ms->moveflags & Q3MFL_WALK)
	{
		if (dist > 0)
		{
			speed = 200 - (180 - 1 * dist);
		}
		else
		{
			speed = 200;
		}
		EA_Walk(ms->client);
	}	//end if
	else
	{
		if (dist > 0)
		{
			speed = 400 - (360 - 2 * dist);
		}
		else
		{
			speed = 400;
		}
	}	//end else
		//elemantary action move in direction
	EA_Move(ms->client, hordir, speed);
	VectorCopy(hordir, result.movedir);
	//
	return result;
}	//end of the function BotTravel_Walk
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotFinishTravel_Walk(bot_movestate_t* ms, aas_reachability_t* reach)
{
	vec3_t hordir;
	float dist, speed;
	bot_moveresult_t result;

	BotClearMoveResult(&result);
	//if not on the ground and changed areas... don't walk back!!
	//(doesn't seem to help)
	/*
	ms->areanum = BotFuzzyPointReachabilityArea(ms->origin);
	if (ms->areanum == reach->areanum)
	{
	#ifdef DEBUG
	    BotImport_Print(PRT_MESSAGE, "BotFinishTravel_Walk: already in reach area\n");
	#endif //DEBUG
	    return result;
	} //end if*/
	//go straight to the reachability end
	hordir[0] = reach->end[0] - ms->origin[0];
	hordir[1] = reach->end[1] - ms->origin[1];
	hordir[2] = 0;
	dist = VectorNormalize(hordir);
	//
	if (dist > 100)
	{
		dist = 100;
	}
	speed = 400 - (400 - 3 * dist);
	//
	EA_Move(ms->client, hordir, speed);
	VectorCopy(hordir, result.movedir);
	//
	return result;
}	//end of the function BotFinishTravel_Walk
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotTravel_Crouch(bot_movestate_t* ms, aas_reachability_t* reach)
{
	float speed;
	vec3_t hordir;
	bot_moveresult_t result;

	BotClearMoveResult(&result);
	//
	speed = 400;
	//walk straight to reachability end
	hordir[0] = reach->end[0] - ms->origin[0];
	hordir[1] = reach->end[1] - ms->origin[1];
	hordir[2] = 0;
	VectorNormalize(hordir);
	//
	BotCheckBlocked(ms, hordir, true, &result);
	//elemantary actions
	EA_Crouch(ms->client);
	EA_Move(ms->client, hordir, speed);
	//
	VectorCopy(hordir, result.movedir);
	//
	return result;
}	//end of the function BotTravel_Crouch
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotTravel_BarrierJump(bot_movestate_t* ms, aas_reachability_t* reach)
{
	float dist, speed;
	vec3_t hordir;
	bot_moveresult_t result;

	BotClearMoveResult(&result);
	//walk straight to reachability start
	hordir[0] = reach->start[0] - ms->origin[0];
	hordir[1] = reach->start[1] - ms->origin[1];
	hordir[2] = 0;
	dist = VectorNormalize(hordir);
	//
	BotCheckBlocked(ms, hordir, true, &result);
	//if pretty close to the barrier
	if (dist < 9)
	{
		EA_Jump(ms->client);
	}	//end if
	else
	{
		if (dist > 60)
		{
			dist = 60;
		}
		speed = 360 - (360 - 6 * dist);
		EA_Move(ms->client, hordir, speed);
	}	//end else
	VectorCopy(hordir, result.movedir);
	//
	return result;
}	//end of the function BotTravel_BarrierJump
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotFinishTravel_BarrierJump(bot_movestate_t* ms, aas_reachability_t* reach)
{
	float dist;
	vec3_t hordir;
	bot_moveresult_t result;

	BotClearMoveResult(&result);
	//if near the top or going down
	if (ms->velocity[2] < 250)
	{
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
		dist = VectorNormalize(hordir);
		//
		BotCheckBlocked(ms, hordir, true, &result);
		//
		EA_Move(ms->client, hordir, 400);
		VectorCopy(hordir, result.movedir);
	}	//end if
		//
	return result;
}	//end of the function BotFinishTravel_BarrierJump
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotTravel_Swim(bot_movestate_t* ms, aas_reachability_t* reach)
{
	vec3_t dir;
	bot_moveresult_t result;

	BotClearMoveResult(&result);
	//swim straight to reachability end
	VectorSubtract(reach->start, ms->origin, dir);
	VectorNormalize(dir);
	//
	BotCheckBlocked(ms, dir, true, &result);
	//elemantary actions
	EA_Move(ms->client, dir, 400);
	//
	VectorCopy(dir, result.movedir);
	VecToAngles(dir, result.ideal_viewangles);
	result.flags |= MOVERESULT_SWIMVIEW;
	//
	return result;
}	//end of the function BotTravel_Swim
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotTravel_WaterJump(bot_movestate_t* ms, aas_reachability_t* reach)
{
	vec3_t dir, hordir;
	float dist;
	bot_moveresult_t result;

	BotClearMoveResult(&result);
	//swim straight to reachability end
	VectorSubtract(reach->end, ms->origin, dir);
	VectorCopy(dir, hordir);
	hordir[2] = 0;
	dir[2] += 15 + crandom() * 40;
	//BotImport_Print(PRT_MESSAGE, "BotTravel_WaterJump: dir[2] = %f\n", dir[2]);
	VectorNormalize(dir);
	dist = VectorNormalize(hordir);
	//elemantary actions
	//EA_Move(ms->client, dir, 400);
	EA_MoveForward(ms->client);
	//move up if close to the actual out of water jump spot
	if (dist < 40)
	{
		EA_MoveUp(ms->client);
	}
	//set the ideal view angles
	VecToAngles(dir, result.ideal_viewangles);
	result.flags |= MOVERESULT_MOVEMENTVIEW;
	//
	VectorCopy(dir, result.movedir);
	//
	return result;
}	//end of the function BotTravel_WaterJump
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotFinishTravel_WaterJump(bot_movestate_t* ms, aas_reachability_t* reach)
{
	vec3_t dir, pnt;
	float dist;
	bot_moveresult_t result;

	//BotImport_Print(PRT_MESSAGE, "BotFinishTravel_WaterJump\n");
	BotClearMoveResult(&result);
	//if waterjumping there's nothing to do
	if (ms->moveflags & MFL_WATERJUMP)
	{
		return result;
	}
	//if not touching any water anymore don't do anything
	//otherwise the bot sometimes keeps jumping?
	VectorCopy(ms->origin, pnt);
	pnt[2] -= 32;	//extra for q2dm4 near red armor/mega health
	if (!(AAS_PointContents(pnt) & (BSP46CONTENTS_LAVA | BSP46CONTENTS_SLIME | BSP46CONTENTS_WATER)))
	{
		return result;
	}
	//swim straight to reachability end
	VectorSubtract(reach->end, ms->origin, dir);
	dir[0] += crandom() * 10;
	dir[1] += crandom() * 10;
	dir[2] += 70 + crandom() * 10;
	dist = VectorNormalize(dir);
	//elemantary actions
	EA_Move(ms->client, dir, 400);
	//set the ideal view angles
	VecToAngles(dir, result.ideal_viewangles);
	result.flags |= MOVERESULT_MOVEMENTVIEW;
	//
	VectorCopy(dir, result.movedir);
	//
	return result;
}	//end of the function BotFinishTravel_WaterJump
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotTravel_WalkOffLedge(bot_movestate_t* ms, aas_reachability_t* reach)
{
	vec3_t hordir, dir;
	float dist, speed, reachhordist;
	bot_moveresult_t result;

	BotClearMoveResult(&result);
	//check if the bot is blocked by anything
	VectorSubtract(reach->start, ms->origin, dir);
	VectorNormalize(dir);
	BotCheckBlocked(ms, dir, true, &result);
	//if the reachability start and end are practially above each other
	VectorSubtract(reach->end, reach->start, dir);
	dir[2] = 0;
	reachhordist = VectorLength(dir);
	//walk straight to the reachability start
	hordir[0] = reach->start[0] - ms->origin[0];
	hordir[1] = reach->start[1] - ms->origin[1];
	hordir[2] = 0;
	dist = VectorNormalize(hordir);
	//if pretty close to the start focus on the reachability end
	if (dist < 48)
	{
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
		VectorNormalize(hordir);
		//
		if (reachhordist < 20)
		{
			speed = 100;
		}	//end if
		else if (!AAS_HorizontalVelocityForJump(0, reach->start, reach->end, &speed))
		{
			speed = 400;
		}	//end if
	}	//end if
	else
	{
		if (reachhordist < 20)
		{
			if (dist > 64)
			{
				dist = 64;
			}
			speed = 400 - (256 - 4 * dist);
		}	//end if
		else
		{
			speed = 400;
		}	//end else
	}	//end else
		//
	BotCheckBlocked(ms, hordir, true, &result);
	//elemantary action
	EA_Move(ms->client, hordir, speed);
	VectorCopy(hordir, result.movedir);
	//
	return result;
}	//end of the function BotTravel_WalkOffLedge
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotFinishTravel_WalkOffLedge(bot_movestate_t* ms, aas_reachability_t* reach)
{
	vec3_t dir, hordir, end, v;
	float dist, speed;
	bot_moveresult_t result;

	BotClearMoveResult(&result);
	//
	VectorSubtract(reach->end, ms->origin, dir);
	BotCheckBlocked(ms, dir, true, &result);
	//
	VectorSubtract(reach->end, ms->origin, v);
	v[2] = 0;
	dist = VectorNormalize(v);
	if (dist > 16)
	{
		VectorMA(reach->end, 16, v, end);
	}
	else
	{
		VectorCopy(reach->end, end);
	}
	//
	if (!BotAirControl(ms->origin, ms->velocity, end, hordir, &speed))
	{
		//go straight to the reachability end
		VectorCopy(dir, hordir);
		hordir[2] = 0;
		//
		dist = VectorNormalize(hordir);
		speed = 400;
	}	//end if
		//
	EA_Move(ms->client, hordir, speed);
	VectorCopy(hordir, result.movedir);
	//
	return result;
}	//end of the function BotFinishTravel_WalkOffLedge
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotTravel_Jump(bot_movestate_t* ms, aas_reachability_t* reach)
{
	vec3_t hordir, dir1, dir2, start, end, runstart;
//	vec3_t runstart, dir1, dir2, hordir;
	float dist1, dist2, speed;
	bot_moveresult_t result;

	BotClearMoveResult(&result);
	//
	AAS_JumpReachRunStart(reach, runstart);
	//*
	hordir[0] = runstart[0] - reach->start[0];
	hordir[1] = runstart[1] - reach->start[1];
	hordir[2] = 0;
	VectorNormalize(hordir);
	//
	VectorCopy(reach->start, start);
	start[2] += 1;
	VectorMA(reach->start, 80, hordir, runstart);
	//check for a gap
	for (dist1 = 0; dist1 < 80; dist1 += 10)
	{
		VectorMA(start, dist1 + 10, hordir, end);
		end[2] += 1;
		if (AAS_PointAreaNum(end) != ms->reachareanum)
		{
			break;
		}
	}	//end for
	if (dist1 < 80)
	{
		VectorMA(reach->start, dist1, hordir, runstart);
	}
	//
	VectorSubtract(ms->origin, reach->start, dir1);
	dir1[2] = 0;
	dist1 = VectorNormalize(dir1);
	VectorSubtract(ms->origin, runstart, dir2);
	dir2[2] = 0;
	dist2 = VectorNormalize(dir2);
	//if just before the reachability start
	if (DotProduct(dir1, dir2) < -0.8 || dist2 < 5)
	{
//		BotImport_Print(PRT_MESSAGE, "between jump start and run start point\n");
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
		VectorNormalize(hordir);
		//elemantary action jump
		if (dist1 < 24)
		{
			EA_Jump(ms->client);
		}
		else if (dist1 < 32)
		{
			EA_DelayedJump(ms->client);
		}
		EA_Move(ms->client, hordir, 600);
		//
		ms->jumpreach = ms->lastreachnum;
	}	//end if
	else
	{
//		BotImport_Print(PRT_MESSAGE, "going towards run start point\n");
		hordir[0] = runstart[0] - ms->origin[0];
		hordir[1] = runstart[1] - ms->origin[1];
		hordir[2] = 0;
		VectorNormalize(hordir);
		//
		if (dist2 > 80)
		{
			dist2 = 80;
		}
		speed = 400 - (400 - 5 * dist2);
		EA_Move(ms->client, hordir, speed);
	}	//end else
	VectorCopy(hordir, result.movedir);
	//
	return result;
}	//end of the function BotTravel_Jump*/
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotFinishTravel_Jump(bot_movestate_t* ms, aas_reachability_t* reach)
{
	vec3_t hordir, hordir2;
	float speed, dist;
	bot_moveresult_t result;

	BotClearMoveResult(&result);
	//if not jumped yet
	if (!ms->jumpreach)
	{
		return result;
	}
	//go straight to the reachability end
	hordir[0] = reach->end[0] - ms->origin[0];
	hordir[1] = reach->end[1] - ms->origin[1];
	hordir[2] = 0;
	dist = VectorNormalize(hordir);
	//
	hordir2[0] = reach->end[0] - reach->start[0];
	hordir2[1] = reach->end[1] - reach->start[1];
	hordir2[2] = 0;
	VectorNormalize(hordir2);
	//
	if (DotProduct(hordir, hordir2) < -0.5 && dist < 24)
	{
		return result;
	}
	//always use max speed when traveling through the air
	speed = 800;
	//
	EA_Move(ms->client, hordir, speed);
	VectorCopy(hordir, result.movedir);
	//
	return result;
}	//end of the function BotFinishTravel_Jump
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotTravel_Ladder(bot_movestate_t* ms, aas_reachability_t* reach)
{
	vec3_t dir, viewdir;
	vec3_t origin = {0, 0, 0};
	bot_moveresult_t result;

	BotClearMoveResult(&result);
	//
	VectorSubtract(reach->end, ms->origin, dir);
	VectorNormalize(dir);
	//set the ideal view angles, facing the ladder up or down
	viewdir[0] = dir[0];
	viewdir[1] = dir[1];
	viewdir[2] = 3 * dir[2];
	VecToAngles(viewdir, result.ideal_viewangles);
	//elemantary action
	EA_Move(ms->client, origin, 0);
	EA_MoveForward(ms->client);
	//set movement view flag so the AI can see the view is focussed
	result.flags |= MOVERESULT_MOVEMENTVIEW;
	//save the movement direction
	VectorCopy(dir, result.movedir);
	//
	return result;
}	//end of the function BotTravel_Ladder
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotTravel_Teleport(bot_movestate_t* ms, aas_reachability_t* reach)
{
	vec3_t hordir;
	float dist;
	bot_moveresult_t result;

	BotClearMoveResult(&result);
	//if the bot is being teleported
	if (ms->moveflags & MFL_TELEPORTED)
	{
		return result;
	}

	//walk straight to center of the teleporter
	VectorSubtract(reach->start, ms->origin, hordir);
	if (!(ms->moveflags & MFL_SWIMMING))
	{
		hordir[2] = 0;
	}
	dist = VectorNormalize(hordir);
	//
	BotCheckBlocked(ms, hordir, true, &result);

	if (dist < 30)
	{
		EA_Move(ms->client, hordir, 200);
	}
	else
	{
		EA_Move(ms->client, hordir, 400);
	}

	if (ms->moveflags & MFL_SWIMMING)
	{
		result.flags |= MOVERESULT_SWIMVIEW;
	}

	VectorCopy(hordir, result.movedir);
	return result;
}	//end of the function BotTravel_Teleport
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotTravel_Elevator(bot_movestate_t* ms, aas_reachability_t* reach)
{
	vec3_t dir, dir1, dir2, hordir, bottomcenter;
	float dist, dist1, dist2, speed;
	bot_moveresult_t result;

	BotClearMoveResult(&result);
	//if standing on the plat
	if (BotOnMover(ms->origin, ms->entitynum, reach))
	{
		//if vertically not too far from the end point
		if (abs(ms->origin[2] - reach->end[2]) < sv_maxbarrier->value)
		{
			//move to the end point
			VectorSubtract(reach->end, ms->origin, hordir);
			hordir[2] = 0;
			VectorNormalize(hordir);
			if (!BotCheckBarrierJump(ms, hordir, 100))
			{
				EA_Move(ms->client, hordir, 400);
			}	//end if
			VectorCopy(hordir, result.movedir);
		}	//end else
			//if not really close to the center of the elevator
		else
		{
			MoverBottomCenter(reach, bottomcenter);
			VectorSubtract(bottomcenter, ms->origin, hordir);
			hordir[2] = 0;
			dist = VectorNormalize(hordir);
			//
			if (dist > 10)
			{
				//move to the center of the plat
				if (dist > 100)
				{
					dist = 100;
				}
				speed = 400 - (400 - 4 * dist);
				//
				EA_Move(ms->client, hordir, speed);
				VectorCopy(hordir, result.movedir);
			}	//end if
		}	//end else
	}	//end if
	else
	{
		//if very near the reachability end
		VectorSubtract(reach->end, ms->origin, dir);
		dist = VectorLength(dir);
		if (dist < 64)
		{
			if (dist > 60)
			{
				dist = 60;
			}
			speed = 360 - (360 - 6 * dist);
			//
			if ((ms->moveflags & MFL_SWIMMING) || !BotCheckBarrierJump(ms, dir, 50))
			{
				if (speed > 5)
				{
					EA_Move(ms->client, dir, speed);
				}
			}	//end if
			VectorCopy(dir, result.movedir);
			//
			if (ms->moveflags & MFL_SWIMMING)
			{
				result.flags |= MOVERESULT_SWIMVIEW;
			}
			//stop using this reachability
			ms->reachability_time = 0;
			return result;
		}	//end if
			//get direction and distance to reachability start
		VectorSubtract(reach->start, ms->origin, dir1);
		if (!(ms->moveflags & MFL_SWIMMING))
		{
			dir1[2] = 0;
		}
		dist1 = VectorNormalize(dir1);
		//if the elevator isn't down
		if (!MoverDown(reach))
		{
			dist = dist1;
			VectorCopy(dir1, dir);
			//
			BotCheckBlocked(ms, dir, false, &result);
			//
			if (dist > 60)
			{
				dist = 60;
			}
			speed = 360 - (360 - 6 * dist);
			//
			if (!(ms->moveflags & MFL_SWIMMING) && !BotCheckBarrierJump(ms, dir, 50))
			{
				if (speed > 5)
				{
					EA_Move(ms->client, dir, speed);
				}
			}	//end if
			VectorCopy(dir, result.movedir);
			//
			if (ms->moveflags & MFL_SWIMMING)
			{
				result.flags |= MOVERESULT_SWIMVIEW;
			}
			//this isn't a failure... just wait till the elevator comes down
			result.type = RESULTTYPE_ELEVATORUP;
			result.flags |= MOVERESULT_WAITING;
			return result;
		}	//end if
			//get direction and distance to elevator bottom center
		MoverBottomCenter(reach, bottomcenter);
		VectorSubtract(bottomcenter, ms->origin, dir2);
		if (!(ms->moveflags & MFL_SWIMMING))
		{
			dir2[2] = 0;
		}
		dist2 = VectorNormalize(dir2);
		//if very close to the reachability start or
		//closer to the elevator center or
		//between reachability start and elevator center
		if (dist1 < 20 || dist2 < dist1 || DotProduct(dir1, dir2) < 0)
		{
			dist = dist2;
			VectorCopy(dir2, dir);
		}	//end if
		else//closer to the reachability start
		{
			dist = dist1;
			VectorCopy(dir1, dir);
		}	//end else
			//
		BotCheckBlocked(ms, dir, false, &result);
		//
		if (dist > 60)
		{
			dist = 60;
		}
		speed = 400 - (400 - 6 * dist);
		//
		if (!(ms->moveflags & MFL_SWIMMING) && !BotCheckBarrierJump(ms, dir, 50))
		{
			EA_Move(ms->client, dir, speed);
		}	//end if
		VectorCopy(dir, result.movedir);
		//
		if (ms->moveflags & MFL_SWIMMING)
		{
			result.flags |= MOVERESULT_SWIMVIEW;
		}
	}	//end else
	return result;
}	//end of the function BotTravel_Elevator
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotFinishTravel_Elevator(bot_movestate_t* ms, aas_reachability_t* reach)
{
	vec3_t bottomcenter, bottomdir, topdir;
	bot_moveresult_t result;

	BotClearMoveResult(&result);
	//
	MoverBottomCenter(reach, bottomcenter);
	VectorSubtract(bottomcenter, ms->origin, bottomdir);
	//
	VectorSubtract(reach->end, ms->origin, topdir);
	//
	if (fabs(bottomdir[2]) < fabs(topdir[2]))
	{
		VectorNormalize(bottomdir);
		EA_Move(ms->client, bottomdir, 300);
	}	//end if
	else
	{
		VectorNormalize(topdir);
		EA_Move(ms->client, topdir, 300);
	}	//end else
	return result;
}	//end of the function BotFinishTravel_Elevator
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotTravel_FuncBobbing(bot_movestate_t* ms, aas_reachability_t* reach)
{
	vec3_t dir, dir1, dir2, hordir, bottomcenter, bob_start, bob_end, bob_origin;
	float dist, dist1, dist2, speed;
	bot_moveresult_t result;

	BotClearMoveResult(&result);
	//
	BotFuncBobStartEnd(reach, bob_start, bob_end, bob_origin);
	//if standing ontop of the func_bobbing
	if (BotOnMover(ms->origin, ms->entitynum, reach))
	{
#ifdef DEBUG_FUNCBOB
		BotImport_Print(PRT_MESSAGE, "bot on func_bobbing\n");
#endif
		//if near end point of reachability
		VectorSubtract(bob_origin, bob_end, dir);
		if (VectorLength(dir) < 24)
		{
#ifdef DEBUG_FUNCBOB
			BotImport_Print(PRT_MESSAGE, "bot moving to reachability end\n");
#endif
			//move to the end point
			VectorSubtract(reach->end, ms->origin, hordir);
			hordir[2] = 0;
			VectorNormalize(hordir);
			if (!BotCheckBarrierJump(ms, hordir, 100))
			{
				EA_Move(ms->client, hordir, 400);
			}	//end if
			VectorCopy(hordir, result.movedir);
		}	//end else
			//if not really close to the center of the elevator
		else
		{
			MoverBottomCenter(reach, bottomcenter);
			VectorSubtract(bottomcenter, ms->origin, hordir);
			hordir[2] = 0;
			dist = VectorNormalize(hordir);
			//
			if (dist > 10)
			{
#ifdef DEBUG_FUNCBOB
				BotImport_Print(PRT_MESSAGE, "bot moving to func_bobbing center\n");
#endif
				//move to the center of the plat
				if (dist > 100)
				{
					dist = 100;
				}
				speed = 400 - (400 - 4 * dist);
				//
				EA_Move(ms->client, hordir, speed);
				VectorCopy(hordir, result.movedir);
			}	//end if
		}	//end else
	}	//end if
	else
	{
#ifdef DEBUG_FUNCBOB
		BotImport_Print(PRT_MESSAGE, "bot not ontop of func_bobbing\n");
#endif
		//if very near the reachability end
		VectorSubtract(reach->end, ms->origin, dir);
		dist = VectorLength(dir);
		if (dist < 64)
		{
#ifdef DEBUG_FUNCBOB
			BotImport_Print(PRT_MESSAGE, "bot moving to end\n");
#endif
			if (dist > 60)
			{
				dist = 60;
			}
			speed = 360 - (360 - 6 * dist);
			//if swimming or no barrier jump
			if ((ms->moveflags & MFL_SWIMMING) || !BotCheckBarrierJump(ms, dir, 50))
			{
				if (speed > 5)
				{
					EA_Move(ms->client, dir, speed);
				}
			}	//end if
			VectorCopy(dir, result.movedir);
			//
			if (ms->moveflags & MFL_SWIMMING)
			{
				result.flags |= MOVERESULT_SWIMVIEW;
			}
			//stop using this reachability
			ms->reachability_time = 0;
			return result;
		}	//end if
			//get direction and distance to reachability start
		VectorSubtract(reach->start, ms->origin, dir1);
		if (!(ms->moveflags & MFL_SWIMMING))
		{
			dir1[2] = 0;
		}
		dist1 = VectorNormalize(dir1);
		//if func_bobbing is Not it's start position
		VectorSubtract(bob_origin, bob_start, dir);
		if (VectorLength(dir) > 16)
		{
#ifdef DEBUG_FUNCBOB
			BotImport_Print(PRT_MESSAGE, "func_bobbing not at start\n");
#endif
			dist = dist1;
			VectorCopy(dir1, dir);
			//
			BotCheckBlocked(ms, dir, false, &result);
			//
			if (dist > 60)
			{
				dist = 60;
			}
			speed = 360 - (360 - 6 * dist);
			//
			if (!(ms->moveflags & MFL_SWIMMING) && !BotCheckBarrierJump(ms, dir, 50))
			{
				if (speed > 5)
				{
					EA_Move(ms->client, dir, speed);
				}
			}	//end if
			VectorCopy(dir, result.movedir);
			//
			if (ms->moveflags & MFL_SWIMMING)
			{
				result.flags |= MOVERESULT_SWIMVIEW;
			}
			//this isn't a failure... just wait till the func_bobbing arrives
			result.type = Q3RESULTTYPE_WAITFORFUNCBOBBING;
			result.flags |= MOVERESULT_WAITING;
			return result;
		}	//end if
			//get direction and distance to func_bob bottom center
		MoverBottomCenter(reach, bottomcenter);
		VectorSubtract(bottomcenter, ms->origin, dir2);
		if (!(ms->moveflags & MFL_SWIMMING))
		{
			dir2[2] = 0;
		}
		dist2 = VectorNormalize(dir2);
		//if very close to the reachability start or
		//closer to the elevator center or
		//between reachability start and func_bobbing center
		if (dist1 < 20 || dist2 < dist1 || DotProduct(dir1, dir2) < 0)
		{
#ifdef DEBUG_FUNCBOB
			BotImport_Print(PRT_MESSAGE, "bot moving to func_bobbing center\n");
#endif
			dist = dist2;
			VectorCopy(dir2, dir);
		}	//end if
		else//closer to the reachability start
		{
#ifdef DEBUG_FUNCBOB
			BotImport_Print(PRT_MESSAGE, "bot moving to reachability start\n");
#endif
			dist = dist1;
			VectorCopy(dir1, dir);
		}	//end else
			//
		BotCheckBlocked(ms, dir, false, &result);
		//
		if (dist > 60)
		{
			dist = 60;
		}
		speed = 400 - (400 - 6 * dist);
		//
		if (!(ms->moveflags & MFL_SWIMMING) && !BotCheckBarrierJump(ms, dir, 50))
		{
			EA_Move(ms->client, dir, speed);
		}	//end if
		VectorCopy(dir, result.movedir);
		//
		if (ms->moveflags & MFL_SWIMMING)
		{
			result.flags |= MOVERESULT_SWIMVIEW;
		}
	}	//end else
	return result;
}	//end of the function BotTravel_FuncBobbing
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotFinishTravel_FuncBobbing(bot_movestate_t* ms, aas_reachability_t* reach)
{
	vec3_t bob_origin, bob_start, bob_end, dir, hordir, bottomcenter;
	bot_moveresult_t result;
	float dist, speed;

	BotClearMoveResult(&result);
	//
	BotFuncBobStartEnd(reach, bob_start, bob_end, bob_origin);
	//
	VectorSubtract(bob_origin, bob_end, dir);
	dist = VectorLength(dir);
	//if the func_bobbing is near the end
	if (dist < 16)
	{
		VectorSubtract(reach->end, ms->origin, hordir);
		if (!(ms->moveflags & MFL_SWIMMING))
		{
			hordir[2] = 0;
		}
		dist = VectorNormalize(hordir);
		//
		if (dist > 60)
		{
			dist = 60;
		}
		speed = 360 - (360 - 6 * dist);
		//
		if (speed > 5)
		{
			EA_Move(ms->client, dir, speed);
		}
		VectorCopy(dir, result.movedir);
		//
		if (ms->moveflags & MFL_SWIMMING)
		{
			result.flags |= MOVERESULT_SWIMVIEW;
		}
	}	//end if
	else
	{
		MoverBottomCenter(reach, bottomcenter);
		VectorSubtract(bottomcenter, ms->origin, hordir);
		if (!(ms->moveflags & MFL_SWIMMING))
		{
			hordir[2] = 0;
		}
		dist = VectorNormalize(hordir);
		//
		if (dist > 5)
		{
			//move to the center of the plat
			if (dist > 100)
			{
				dist = 100;
			}
			speed = 400 - (400 - 4 * dist);
			//
			EA_Move(ms->client, hordir, speed);
			VectorCopy(hordir, result.movedir);
		}	//end if
	}	//end else
	return result;
}	//end of the function BotFinishTravel_FuncBobbing
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotResetGrapple(bot_movestate_t* ms)
{
	aas_reachability_t reach;

	AAS_ReachabilityFromNum(ms->lastreachnum, &reach);
	//if not using the grapple hook reachability anymore
	if ((reach.traveltype & TRAVELTYPE_MASK) != TRAVEL_GRAPPLEHOOK)
	{
		if ((ms->moveflags & Q3MFL_ACTIVEGRAPPLE) || ms->grapplevisible_time)
		{
			if (offhandgrapple->value)
			{
				EA_Command(ms->client, cmd_grappleoff->string);
			}
			ms->moveflags &= ~Q3MFL_ACTIVEGRAPPLE;
			ms->grapplevisible_time = 0;
		}	//end if
	}	//end if
}	//end of the function BotResetGrapple
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotTravel_Grapple(bot_movestate_t* ms, aas_reachability_t* reach)
{
	bot_moveresult_t result;
	float dist, speed;
	vec3_t dir, viewdir, org;
	int state, areanum;
	bsp_trace_t trace;

	BotClearMoveResult(&result);
	//
	if (ms->moveflags & Q3MFL_GRAPPLERESET)
	{
		if (offhandgrapple->value)
		{
			EA_Command(ms->client, cmd_grappleoff->string);
		}
		ms->moveflags &= ~Q3MFL_ACTIVEGRAPPLE;
		return result;
	}	//end if
		//
	if (!(int)offhandgrapple->value)
	{
		result.weapon = weapindex_grapple->value;
		result.flags |= MOVERESULT_MOVEMENTWEAPON;
	}	//end if
		//
	if (ms->moveflags & Q3MFL_ACTIVEGRAPPLE)
	{
		state = GrappleState(ms, reach);
		//
		VectorSubtract(reach->end, ms->origin, dir);
		dir[2] = 0;
		dist = VectorLength(dir);
		//if very close to the grapple end or the grappled is hooked and
		//the bot doesn't get any closer
		if (state && dist < 48)
		{
			if (ms->lastgrappledist - dist < 1)
			{
				if (offhandgrapple->value)
				{
					EA_Command(ms->client, cmd_grappleoff->string);
				}
				ms->moveflags &= ~Q3MFL_ACTIVEGRAPPLE;
				ms->moveflags |= Q3MFL_GRAPPLERESET;
				ms->reachability_time = 0;	//end the reachability
				return result;
			}	//end if
		}	//end if
			//if no valid grapple at all, or the grapple hooked and the bot
			//isn't moving anymore
		else if (!state || (state == 2 && dist > ms->lastgrappledist - 2))
		{
			if (ms->grapplevisible_time < AAS_Time() - 0.4)
			{
				if (offhandgrapple->value)
				{
					EA_Command(ms->client, cmd_grappleoff->string);
				}
				ms->moveflags &= ~Q3MFL_ACTIVEGRAPPLE;
				ms->moveflags |= Q3MFL_GRAPPLERESET;
				ms->reachability_time = 0;	//end the reachability
				return result;
			}	//end if
		}	//end if
		else
		{
			ms->grapplevisible_time = AAS_Time();
		}	//end else
			//
		if (!(int)offhandgrapple->value)
		{
			EA_Attack(ms->client);
		}	//end if
			//remember the current grapple distance
		ms->lastgrappledist = dist;
	}	//end if
	else
	{
		ms->grapplevisible_time = AAS_Time();
		//
		VectorSubtract(reach->start, ms->origin, dir);
		if (!(ms->moveflags & MFL_SWIMMING))
		{
			dir[2] = 0;
		}
		VectorAdd(ms->origin, ms->viewoffset, org);
		VectorSubtract(reach->end, org, viewdir);
		//
		dist = VectorNormalize(dir);
		VecToAngles(viewdir, result.ideal_viewangles);
		result.flags |= MOVERESULT_MOVEMENTVIEW;
		//
		if (dist < 5 &&
			fabs(AngleDiff(result.ideal_viewangles[0], ms->viewangles[0])) < 2 &&
			fabs(AngleDiff(result.ideal_viewangles[1], ms->viewangles[1])) < 2)
		{
			//check if the grapple missile path is clear
			VectorAdd(ms->origin, ms->viewoffset, org);
			trace = AAS_Trace(org, NULL, NULL, reach->end, ms->entitynum, BSP46CONTENTS_SOLID);
			VectorSubtract(reach->end, trace.endpos, dir);
			if (VectorLength(dir) > 16)
			{
				result.failure = true;
				return result;
			}	//end if
				//activate the grapple
			if (offhandgrapple->value)
			{
				EA_Command(ms->client, cmd_grappleon->string);
			}	//end if
			else
			{
				EA_Attack(ms->client);
			}	//end else
			ms->moveflags |= Q3MFL_ACTIVEGRAPPLE;
			ms->lastgrappledist = 999999;
		}	//end if
		else
		{
			if (dist < 70)
			{
				speed = 300 - (300 - 4 * dist);
			}
			else
			{
				speed = 400;
			}
			//
			BotCheckBlocked(ms, dir, true, &result);
			//elemantary action move in direction
			EA_Move(ms->client, dir, speed);
			VectorCopy(dir, result.movedir);
		}	//end else
			//if in another area before actually grappling
		areanum = AAS_PointAreaNum(ms->origin);
		if (areanum && areanum != ms->reachareanum)
		{
			ms->reachability_time = 0;
		}
	}	//end else
	return result;
}	//end of the function BotTravel_Grapple
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:			-
//===========================================================================
bot_moveresult_t BotTravel_RocketJump(bot_movestate_t* ms, aas_reachability_t* reach)
{
	vec3_t hordir;
	float dist, speed;
	bot_moveresult_t result;

	//BotImport_Print(PRT_MESSAGE, "BotTravel_RocketJump: bah\n");
	BotClearMoveResult(&result);
	//
	hordir[0] = reach->start[0] - ms->origin[0];
	hordir[1] = reach->start[1] - ms->origin[1];
	hordir[2] = 0;
	//
	dist = VectorNormalize(hordir);
	//look in the movement direction
	VecToAngles(hordir, result.ideal_viewangles);
	//look straight down
	result.ideal_viewangles[PITCH] = 90;
	//
	if (dist < 5 &&
		fabs(AngleDiff(result.ideal_viewangles[0], ms->viewangles[0])) < 5 &&
		fabs(AngleDiff(result.ideal_viewangles[1], ms->viewangles[1])) < 5)
	{
		//BotImport_Print(PRT_MESSAGE, "between jump start and run start point\n");
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
		VectorNormalize(hordir);
		//elemantary action jump
		EA_Jump(ms->client);
		EA_Attack(ms->client);
		EA_Move(ms->client, hordir, 800);
		//
		ms->jumpreach = ms->lastreachnum;
	}	//end if
	else
	{
		if (dist > 80)
		{
			dist = 80;
		}
		speed = 400 - (400 - 5 * dist);
		EA_Move(ms->client, hordir, speed);
	}	//end else
		//look in the movement direction
	VecToAngles(hordir, result.ideal_viewangles);
	//look straight down
	result.ideal_viewangles[PITCH] = 90;
	//set the view angles directly
	EA_View(ms->client, result.ideal_viewangles);
	//view is important for the movment
	result.flags |= MOVERESULT_MOVEMENTVIEWSET;
	//select the rocket launcher
	EA_SelectWeapon(ms->client, (int)weapindex_rocketlauncher->value);
	//weapon is used for movement
	result.weapon = (int)weapindex_rocketlauncher->value;
	result.flags |= MOVERESULT_MOVEMENTWEAPON;
	//
	VectorCopy(hordir, result.movedir);
	//
	return result;
}	//end of the function BotTravel_RocketJump
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotTravel_BFGJump(bot_movestate_t* ms, aas_reachability_t* reach)
{
	vec3_t hordir;
	float dist, speed;
	bot_moveresult_t result;

	//BotImport_Print(PRT_MESSAGE, "BotTravel_BFGJump: bah\n");
	BotClearMoveResult(&result);
	//
	hordir[0] = reach->start[0] - ms->origin[0];
	hordir[1] = reach->start[1] - ms->origin[1];
	hordir[2] = 0;
	//
	dist = VectorNormalize(hordir);
	//
	if (dist < 5 &&
		fabs(AngleDiff(result.ideal_viewangles[0], ms->viewangles[0])) < 5 &&
		fabs(AngleDiff(result.ideal_viewangles[1], ms->viewangles[1])) < 5)
	{
		//BotImport_Print(PRT_MESSAGE, "between jump start and run start point\n");
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
		VectorNormalize(hordir);
		//elemantary action jump
		EA_Jump(ms->client);
		EA_Attack(ms->client);
		EA_Move(ms->client, hordir, 800);
		//
		ms->jumpreach = ms->lastreachnum;
	}	//end if
	else
	{
		if (dist > 80)
		{
			dist = 80;
		}
		speed = 400 - (400 - 5 * dist);
		EA_Move(ms->client, hordir, speed);
	}	//end else
		//look in the movement direction
	VecToAngles(hordir, result.ideal_viewangles);
	//look straight down
	result.ideal_viewangles[PITCH] = 90;
	//set the view angles directly
	EA_View(ms->client, result.ideal_viewangles);
	//view is important for the movment
	result.flags |= MOVERESULT_MOVEMENTVIEWSET;
	//select the rocket launcher
	EA_SelectWeapon(ms->client, (int)weapindex_bfg10k->value);
	//weapon is used for movement
	result.weapon = (int)weapindex_bfg10k->value;
	result.flags |= MOVERESULT_MOVEMENTWEAPON;
	//
	VectorCopy(hordir, result.movedir);
	//
	return result;
}	//end of the function BotTravel_BFGJump
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotFinishTravel_WeaponJump(bot_movestate_t* ms, aas_reachability_t* reach)
{
	vec3_t hordir;
	float speed;
	bot_moveresult_t result;

	BotClearMoveResult(&result);
	//if not jumped yet
	if (!ms->jumpreach)
	{
		return result;
	}
	/*
	//go straight to the reachability end
	hordir[0] = reach->end[0] - ms->origin[0];
	hordir[1] = reach->end[1] - ms->origin[1];
	hordir[2] = 0;
	VectorNormalize(hordir);
	//always use max speed when traveling through the air
	EA_Move(ms->client, hordir, 800);
	VectorCopy(hordir, result.movedir);
	*/
	//
	if (!BotAirControl(ms->origin, ms->velocity, reach->end, hordir, &speed))
	{
		//go straight to the reachability end
		VectorSubtract(reach->end, ms->origin, hordir);
		hordir[2] = 0;
		VectorNormalize(hordir);
		speed = 400;
	}	//end if
		//
	EA_Move(ms->client, hordir, speed);
	VectorCopy(hordir, result.movedir);
	//
	return result;
}	//end of the function BotFinishTravel_WeaponJump
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotTravel_JumpPad(bot_movestate_t* ms, aas_reachability_t* reach)
{
	float dist, speed;
	vec3_t hordir;
	bot_moveresult_t result;

	BotClearMoveResult(&result);
	//first walk straight to the reachability start
	hordir[0] = reach->start[0] - ms->origin[0];
	hordir[1] = reach->start[1] - ms->origin[1];
	hordir[2] = 0;
	dist = VectorNormalize(hordir);
	//
	BotCheckBlocked(ms, hordir, true, &result);
	speed = 400;
	//elemantary action move in direction
	EA_Move(ms->client, hordir, speed);
	VectorCopy(hordir, result.movedir);
	//
	return result;
}	//end of the function BotTravel_JumpPad
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotFinishTravel_JumpPad(bot_movestate_t* ms, aas_reachability_t* reach)
{
	float speed;
	vec3_t hordir;
	bot_moveresult_t result;

	BotClearMoveResult(&result);
	if (!BotAirControl(ms->origin, ms->velocity, reach->end, hordir, &speed))
	{
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
		VectorNormalize(hordir);
		speed = 400;
	}	//end if
	BotCheckBlocked(ms, hordir, true, &result);
	//elemantary action move in direction
	EA_Move(ms->client, hordir, speed);
	VectorCopy(hordir, result.movedir);
	//
	return result;
}	//end of the function BotFinishTravel_JumpPad
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotMoveInGoalArea(bot_movestate_t* ms, bot_goal_q3_t* goal)
{
	bot_moveresult_t result;
	vec3_t dir;
	float dist, speed;

#ifdef DEBUG
	//BotImport_Print(PRT_MESSAGE, "%s: moving straight to goal\n", ClientName(ms->entitynum-1));
	//AAS_ClearShownDebugLines();
	//AAS_DebugLine(ms->origin, goal->origin, LINECOLOR_RED);
#endif	//DEBUG
	BotClearMoveResult(&result);
	//walk straight to the goal origin
	dir[0] = goal->origin[0] - ms->origin[0];
	dir[1] = goal->origin[1] - ms->origin[1];
	if (ms->moveflags & MFL_SWIMMING)
	{
		dir[2] = goal->origin[2] - ms->origin[2];
		result.traveltype = TRAVEL_SWIM;
	}	//end if
	else
	{
		dir[2] = 0;
		result.traveltype = TRAVEL_WALK;
	}	//endif
		//
	dist = VectorNormalize(dir);
	if (dist > 100)
	{
		dist = 100;
	}
	speed = 400 - (400 - 4 * dist);
	if (speed < 10)
	{
		speed = 0;
	}
	//
	BotCheckBlocked(ms, dir, true, &result);
	//elemantary action move in direction
	EA_Move(ms->client, dir, speed);
	VectorCopy(dir, result.movedir);
	//
	if (ms->moveflags & MFL_SWIMMING)
	{
		VecToAngles(dir, result.ideal_viewangles);
		result.flags |= MOVERESULT_SWIMVIEW;
	}	//end if
		//if (!debugline) debugline = BotImport_DebugLineCreate();
		//BotImport_DebugLineShow(debugline, ms->origin, goal->origin, LINECOLOR_BLUE);
		//
	ms->lastreachnum = 0;
	ms->lastareanum = 0;
	ms->lastgoalareanum = goal->areanum;
	VectorCopy(ms->origin, ms->lastorigin);
	//
	return result;
}	//end of the function BotMoveInGoalArea
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotMoveToGoal(bot_moveresult_t* result, int movestate, bot_goal_q3_t* goal, int travelflags)
{
	int reachnum, lastreachnum, foundjumppad, ent, resultflags;
	aas_reachability_t reach, lastreach;
	bot_movestate_t* ms;
	//vec3_t mins, maxs, up = {0, 0, 1};
	//bsp_trace_t trace;
	//static int debugline;


	BotClearMoveResult(result);
	//
	ms = BotMoveStateFromHandle(movestate);
	if (!ms)
	{
		return;
	}
	//reset the grapple before testing if the bot has a valid goal
	//because the bot could loose all it's goals when stuck to a wall
	BotResetGrapple(ms);
	//
	if (!goal)
	{
#ifdef DEBUG
		BotImport_Print(PRT_MESSAGE, "client %d: movetogoal -> no goal\n", ms->client);
#endif	//DEBUG
		result->failure = true;
		return;
	}	//end if
		//BotImport_Print(PRT_MESSAGE, "numavoidreach = %d\n", ms->numavoidreach);
		//remove some of the move flags
	ms->moveflags &= ~(MFL_SWIMMING | MFL_AGAINSTLADDER);
	//set some of the move flags
	//NOTE: the MFL_ONGROUND flag is also set in the higher AI
	if (AAS_OnGround(ms->origin, ms->presencetype, ms->entitynum))
	{
		ms->moveflags |= MFL_ONGROUND;
	}
	//
	if (ms->moveflags & MFL_ONGROUND)
	{
		int modeltype, modelnum;

		ent = BotOnTopOfEntity(ms);

		if (ent != -1)
		{
			modelnum = AAS_EntityModelindex(ent);
			if (modelnum >= 0 && modelnum < MAX_MODELS_Q3)
			{
				modeltype = modeltypes[modelnum];

				if (modeltype == MODELTYPE_FUNC_PLAT)
				{
					AAS_ReachabilityFromNum(ms->lastreachnum, &reach);
					//if the bot is Not using the elevator
					if ((reach.traveltype & TRAVELTYPE_MASK) != TRAVEL_ELEVATOR ||
						//NOTE: the face number is the plat model number
						(reach.facenum & 0x0000FFFF) != modelnum)
					{
						reachnum = AAS_NextModelReachability(0, modelnum);
						if (reachnum)
						{
							//BotImport_Print(PRT_MESSAGE, "client %d: accidentally ended up on func_plat\n", ms->client);
							AAS_ReachabilityFromNum(reachnum, &reach);
							ms->lastreachnum = reachnum;
							ms->reachability_time = AAS_Time() + BotReachabilityTime(&reach);
						}	//end if
						else
						{
							if (bot_developer)
							{
								BotImport_Print(PRT_MESSAGE, "client %d: on func_plat without reachability\n", ms->client);
							}	//end if
							result->blocked = true;
							result->blockentity = ent;
							result->flags |= MOVERESULT_ONTOPOFOBSTACLE;
							return;
						}	//end else
					}	//end if
					result->flags |= MOVERESULT_ONTOPOF_ELEVATOR;
				}	//end if
				else if (modeltype == MODELTYPE_FUNC_BOB)
				{
					AAS_ReachabilityFromNum(ms->lastreachnum, &reach);
					//if the bot is Not using the func bobbing
					if ((reach.traveltype & TRAVELTYPE_MASK) != TRAVEL_FUNCBOB ||
						//NOTE: the face number is the func_bobbing model number
						(reach.facenum & 0x0000FFFF) != modelnum)
					{
						reachnum = AAS_NextModelReachability(0, modelnum);
						if (reachnum)
						{
							//BotImport_Print(PRT_MESSAGE, "client %d: accidentally ended up on func_bobbing\n", ms->client);
							AAS_ReachabilityFromNum(reachnum, &reach);
							ms->lastreachnum = reachnum;
							ms->reachability_time = AAS_Time() + BotReachabilityTime(&reach);
						}	//end if
						else
						{
							if (bot_developer)
							{
								BotImport_Print(PRT_MESSAGE, "client %d: on func_bobbing without reachability\n", ms->client);
							}	//end if
							result->blocked = true;
							result->blockentity = ent;
							result->flags |= MOVERESULT_ONTOPOFOBSTACLE;
							return;
						}	//end else
					}	//end if
					result->flags |= MOVERESULT_ONTOPOF_FUNCBOB;
				}	//end if
				else if (modeltype == MODELTYPE_FUNC_STATIC || modeltype == MODELTYPE_FUNC_DOOR)
				{
					// check if ontop of a door bridge ?
					ms->areanum = BotFuzzyPointReachabilityArea(ms->origin);
					// if not in a reachability area
					if (!AAS_AreaReachability(ms->areanum))
					{
						result->blocked = true;
						result->blockentity = ent;
						result->flags |= MOVERESULT_ONTOPOFOBSTACLE;
						return;
					}	//end if
				}	//end else if
				else
				{
					result->blocked = true;
					result->blockentity = ent;
					result->flags |= MOVERESULT_ONTOPOFOBSTACLE;
					return;
				}	//end else
			}	//end if
		}	//end if
	}	//end if
		//if swimming
	if (AAS_Swimming(ms->origin))
	{
		ms->moveflags |= MFL_SWIMMING;
	}
	//if against a ladder
	if (AAS_AgainstLadder(ms->origin, ms->areanum))
	{
		ms->moveflags |= MFL_AGAINSTLADDER;
	}
	//if the bot is on the ground, swimming or against a ladder
	if (ms->moveflags & (MFL_ONGROUND | MFL_SWIMMING | MFL_AGAINSTLADDER))
	{
		//BotImport_Print(PRT_MESSAGE, "%s: onground, swimming or against ladder\n", ClientName(ms->entitynum-1));
		//
		AAS_ReachabilityFromNum(ms->lastreachnum, &lastreach);
		//reachability area the bot is in
		//ms->areanum = BotReachabilityArea(ms->origin, ((lastreach.traveltype & TRAVELTYPE_MASK) != TRAVEL_ELEVATOR));
		ms->areanum = BotFuzzyPointReachabilityArea(ms->origin);
		//
		if (!ms->areanum)
		{
			result->failure = true;
			result->blocked = true;
			result->blockentity = 0;
			result->type = Q3RESULTTYPE_INSOLIDAREA;
			return;
		}	//end if
			//if the bot is in the goal area
		if (ms->areanum == goal->areanum)
		{
			*result = BotMoveInGoalArea(ms, goal);
			return;
		}	//end if
			//assume we can use the reachability from the last frame
		reachnum = ms->lastreachnum;
		//if there is a last reachability
		if (reachnum)
		{
			AAS_ReachabilityFromNum(reachnum, &reach);
			//check if the reachability is still valid
			if (!(AAS_TravelFlagForType(reach.traveltype) & travelflags))
			{
				reachnum = 0;
			}	//end if
				//special grapple hook case
			else if ((reach.traveltype & TRAVELTYPE_MASK) == TRAVEL_GRAPPLEHOOK)
			{
				if (ms->reachability_time < AAS_Time() ||
					(ms->moveflags & Q3MFL_GRAPPLERESET))
				{
					reachnum = 0;
				}	//end if
			}	//end if
				//special elevator case
			else if ((reach.traveltype & TRAVELTYPE_MASK) == TRAVEL_ELEVATOR ||
					 (reach.traveltype & TRAVELTYPE_MASK) == TRAVEL_FUNCBOB)
			{
				if ((result->flags & MOVERESULT_ONTOPOF_FUNCBOB) ||
					(result->flags & MOVERESULT_ONTOPOF_FUNCBOB))
				{
					ms->reachability_time = AAS_Time() + 5;
				}	//end if
					//if the bot was going for an elevator and reached the reachability area
				if (ms->areanum == reach.areanum ||
					ms->reachability_time < AAS_Time())
				{
					reachnum = 0;
				}	//end if
			}	//end if
			else
			{
#ifdef DEBUG
				if (bot_developer)
				{
					if (ms->reachability_time < AAS_Time())
					{
						BotImport_Print(PRT_MESSAGE, "client %d: reachability timeout in ", ms->client);
						AAS_PrintTravelType(reach.traveltype & TRAVELTYPE_MASK);
						BotImport_Print(PRT_MESSAGE, "\n");
					}	//end if
						/*
						if (ms->lastareanum != ms->areanum)
						{
						    BotImport_Print(PRT_MESSAGE, "changed from area %d to %d\n", ms->lastareanum, ms->areanum);
						} //end if*/
				}	//end if
#endif	//DEBUG
				//if the goal area changed or the reachability timed out
				//or the area changed
				if (ms->lastgoalareanum != goal->areanum ||
					ms->reachability_time < AAS_Time() ||
					ms->lastareanum != ms->areanum)
				{
					reachnum = 0;
					//BotImport_Print(PRT_MESSAGE, "area change or timeout\n");
				}	//end else if
			}	//end else
		}	//end if
		resultflags = 0;
		//if the bot needs a new reachability
		if (!reachnum)
		{
			//if the area has no reachability links
			if (!AAS_AreaReachability(ms->areanum))
			{
#ifdef DEBUG
				if (bot_developer)
				{
					BotImport_Print(PRT_MESSAGE, "area %d no reachability\n", ms->areanum);
				}	//end if
#endif	//DEBUG
			}	//end if
				//get a new reachability leading towards the goal
			reachnum = BotGetReachabilityToGoal(ms->origin, ms->areanum,
				ms->lastgoalareanum, ms->lastareanum,
				ms->avoidreach, ms->avoidreachtimes, ms->avoidreachtries,
				reinterpret_cast<bot_goal_t*>(goal), travelflags, travelflags,
				ms->avoidspots, ms->numavoidspots, &resultflags);
			//the area number the reachability starts in
			ms->reachareanum = ms->areanum;
			//reset some state variables
			ms->jumpreach = 0;						//for TRAVEL_JUMP
			ms->moveflags &= ~Q3MFL_GRAPPLERESET;	//for TRAVEL_GRAPPLEHOOK
			//if there is a reachability to the goal
			if (reachnum)
			{
				AAS_ReachabilityFromNum(reachnum, &reach);
				//set a timeout for this reachability
				ms->reachability_time = AAS_Time() + BotReachabilityTime(&reach);
				//
#ifdef AVOIDREACH
				//add the reachability to the reachabilities to avoid for a while
				BotAddToAvoidReach(ms, reachnum, AVOIDREACH_TIME_Q3);
#endif	//AVOIDREACH
			}	//end if
#ifdef DEBUG

			else if (bot_developer)
			{
				BotImport_Print(PRT_MESSAGE, "goal not reachable\n");
				Com_Memset(&reach, 0, sizeof(aas_reachability_t));	//make compiler happy
			}	//end else
			if (bot_developer)
			{
				//if still going for the same goal
				if (ms->lastgoalareanum == goal->areanum)
				{
					if (ms->lastareanum == reach.areanum)
					{
						BotImport_Print(PRT_MESSAGE, "same goal, going back to previous area\n");
					}	//end if
				}	//end if
			}	//end if
#endif	//DEBUG
		}	//end else
			//
		ms->lastreachnum = reachnum;
		ms->lastgoalareanum = goal->areanum;
		ms->lastareanum = ms->areanum;
		//if the bot has a reachability
		if (reachnum)
		{
			//get the reachability from the number
			AAS_ReachabilityFromNum(reachnum, &reach);
			result->traveltype = reach.traveltype;
			//
			switch (reach.traveltype & TRAVELTYPE_MASK)
			{
			case TRAVEL_WALK: *result = BotTravel_Walk(ms, &reach); break;
			case TRAVEL_CROUCH: *result = BotTravel_Crouch(ms, &reach); break;
			case TRAVEL_BARRIERJUMP: *result = BotTravel_BarrierJump(ms, &reach); break;
			case TRAVEL_LADDER: *result = BotTravel_Ladder(ms, &reach); break;
			case TRAVEL_WALKOFFLEDGE: *result = BotTravel_WalkOffLedge(ms, &reach); break;
			case TRAVEL_JUMP: *result = BotTravel_Jump(ms, &reach); break;
			case TRAVEL_SWIM: *result = BotTravel_Swim(ms, &reach); break;
			case TRAVEL_WATERJUMP: *result = BotTravel_WaterJump(ms, &reach); break;
			case TRAVEL_TELEPORT: *result = BotTravel_Teleport(ms, &reach); break;
			case TRAVEL_ELEVATOR: *result = BotTravel_Elevator(ms, &reach); break;
			case TRAVEL_GRAPPLEHOOK: *result = BotTravel_Grapple(ms, &reach); break;
			case TRAVEL_ROCKETJUMP: *result = BotTravel_RocketJump(ms, &reach); break;
			case TRAVEL_BFGJUMP: *result = BotTravel_BFGJump(ms, &reach); break;
			case TRAVEL_JUMPPAD: *result = BotTravel_JumpPad(ms, &reach); break;
			case TRAVEL_FUNCBOB: *result = BotTravel_FuncBobbing(ms, &reach); break;
			default:
			{
				BotImport_Print(PRT_FATAL, "travel type %d not implemented yet\n", (reach.traveltype & TRAVELTYPE_MASK));
				break;
			}		//end case
			}	//end switch
			result->traveltype = reach.traveltype;
			result->flags |= resultflags;
		}	//end if
		else
		{
			result->failure = true;
			result->flags |= resultflags;
			Com_Memset(&reach, 0, sizeof(aas_reachability_t));
		}	//end else
#ifdef DEBUG
		if (bot_developer)
		{
			if (result->failure)
			{
				BotImport_Print(PRT_MESSAGE, "client %d: movement failure in ", ms->client);
				AAS_PrintTravelType(reach.traveltype & TRAVELTYPE_MASK);
				BotImport_Print(PRT_MESSAGE, "\n");
			}	//end if
		}	//end if
#endif	//DEBUG
	}	//end if
	else
	{
		int i, numareas, areas[16];
		vec3_t end;

		//special handling of jump pads when the bot uses a jump pad without knowing it
		foundjumppad = false;
		VectorMA(ms->origin, -2 * ms->thinktime, ms->velocity, end);
		numareas = AAS_TraceAreas(ms->origin, end, areas, NULL, 16);
		for (i = numareas - 1; i >= 0; i--)
		{
			if (AAS_AreaJumpPad(areas[i]))
			{
				//BotImport_Print(PRT_MESSAGE, "client %d used a jumppad without knowing, area %d\n", ms->client, areas[i]);
				foundjumppad = true;
				lastreachnum = BotGetReachabilityToGoal(end, areas[i],
					ms->lastgoalareanum, ms->lastareanum,
					ms->avoidreach, ms->avoidreachtimes, ms->avoidreachtries,
					reinterpret_cast<bot_goal_t*>(goal), travelflags, TFL_JUMPPAD, ms->avoidspots, ms->numavoidspots, NULL);
				if (lastreachnum)
				{
					ms->lastreachnum = lastreachnum;
					ms->lastareanum = areas[i];
					//BotImport_Print(PRT_MESSAGE, "found jumppad reachability\n");
					break;
				}	//end if
				else
				{
					for (lastreachnum = AAS_NextAreaReachability(areas[i], 0); lastreachnum;
						 lastreachnum = AAS_NextAreaReachability(areas[i], lastreachnum))
					{
						//get the reachability from the number
						AAS_ReachabilityFromNum(lastreachnum, &reach);
						if ((reach.traveltype & TRAVELTYPE_MASK) == TRAVEL_JUMPPAD)
						{
							ms->lastreachnum = lastreachnum;
							ms->lastareanum = areas[i];
							//BotImport_Print(PRT_MESSAGE, "found jumppad reachability hard!!\n");
							break;
						}	//end if
					}	//end for
					if (lastreachnum)
					{
						break;
					}
				}	//end else
			}	//end if
		}	//end for
		if (bot_developer)
		{
			//if a jumppad is found with the trace but no reachability is found
			if (foundjumppad && !ms->lastreachnum)
			{
				BotImport_Print(PRT_MESSAGE, "client %d didn't find jumppad reachability\n", ms->client);
			}	//end if
		}	//end if
			//
		if (ms->lastreachnum)
		{
			//BotImport_Print(PRT_MESSAGE, "%s: NOT onground, swimming or against ladder\n", ClientName(ms->entitynum-1));
			AAS_ReachabilityFromNum(ms->lastreachnum, &reach);
			result->traveltype = reach.traveltype;
#ifdef DEBUG
			//BotImport_Print(PRT_MESSAGE, "client %d finish: ", ms->client);
			//AAS_PrintTravelType(reach.traveltype & TRAVELTYPE_MASK);
			//BotImport_Print(PRT_MESSAGE, "\n");
#endif	//DEBUG
			//
			switch (reach.traveltype & TRAVELTYPE_MASK)
			{
			case TRAVEL_WALK: *result = BotTravel_Walk(ms, &reach); break;		//BotFinishTravel_Walk(ms, &reach); break;
			case TRAVEL_CROUCH:	/*do nothing*/ break;
			case TRAVEL_BARRIERJUMP: *result = BotFinishTravel_BarrierJump(ms, &reach); break;
			case TRAVEL_LADDER: *result = BotTravel_Ladder(ms, &reach); break;
			case TRAVEL_WALKOFFLEDGE: *result = BotFinishTravel_WalkOffLedge(ms, &reach); break;
			case TRAVEL_JUMP: *result = BotFinishTravel_Jump(ms, &reach); break;
			case TRAVEL_SWIM: *result = BotTravel_Swim(ms, &reach); break;
			case TRAVEL_WATERJUMP: *result = BotFinishTravel_WaterJump(ms, &reach); break;
			case TRAVEL_TELEPORT: /*do nothing*/ break;
			case TRAVEL_ELEVATOR: *result = BotFinishTravel_Elevator(ms, &reach); break;
			case TRAVEL_GRAPPLEHOOK: *result = BotTravel_Grapple(ms, &reach); break;
			case TRAVEL_ROCKETJUMP:
			case TRAVEL_BFGJUMP: *result = BotFinishTravel_WeaponJump(ms, &reach); break;
			case TRAVEL_JUMPPAD: *result = BotFinishTravel_JumpPad(ms, &reach); break;
			case TRAVEL_FUNCBOB: *result = BotFinishTravel_FuncBobbing(ms, &reach); break;
			default:
			{
				BotImport_Print(PRT_FATAL, "(last) travel type %d not implemented yet\n", (reach.traveltype & TRAVELTYPE_MASK));
				break;
			}		//end case
			}	//end switch
			result->traveltype = reach.traveltype;
#ifdef DEBUG
			if (bot_developer)
			{
				if (result->failure)
				{
					BotImport_Print(PRT_MESSAGE, "client %d: movement failure in finish ", ms->client);
					AAS_PrintTravelType(reach.traveltype & TRAVELTYPE_MASK);
					BotImport_Print(PRT_MESSAGE, "\n");
				}	//end if
			}	//end if
#endif	//DEBUG
		}	//end if
	}	//end else
		//FIXME: is it right to do this here?
	if (result->blocked)
	{
		ms->reachability_time -= 10 * ms->thinktime;
	}
	//copy the last origin
	VectorCopy(ms->origin, ms->lastorigin);
	//return the movement result
	return;
}	//end of the function BotMoveToGoal
