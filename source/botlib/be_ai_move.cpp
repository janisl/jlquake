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
#include "../server/server.h"
#include "../server/botlib/local.h"
#include "be_ai_move.h"

#define AVOIDREACH

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
			Q_fabs(AngleDiff(result.ideal_viewangles[0], ms->viewangles[0])) < 2 &&
			Q_fabs(AngleDiff(result.ideal_viewangles[1], ms->viewangles[1])) < 2)
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
			*result = BotMoveInGoalArea(ms, reinterpret_cast<bot_goal_t*>(goal));
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
