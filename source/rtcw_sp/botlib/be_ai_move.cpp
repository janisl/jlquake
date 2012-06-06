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
 * name:		be_ai_move.c
 *
 * desc:		bot movement AI
 *
 *
 *****************************************************************************/

#include "../game/q_shared.h"
#include "../game/botlib.h"
#include "be_interface.h"
#include "be_aas_funcs.h"

#include "../game/be_ea.h"
#include "../game/be_ai_goal.h"
#include "../game/be_ai_move.h"

// Ridah, disabled this to prevent wierd navigational behaviour (mostly by Zombie, since it's so slow)
//#define AVOIDREACH

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
	//float dist, speed;
	vec3_t dir, viewdir, hordir, pos, p, v1, v2, vec, right;
	vec3_t origin = {0, 0, 0};
//	vec3_t up = {0, 0, 1};
	bot_moveresult_t result;
	float dist, speed;

	// RF, heavily modified, wolf has different ladder movement

	BotClearMoveResult(&result);
	//
	if ((ms->moveflags & MFL_AGAINSTLADDER)
		//NOTE: not a good idea for ladders starting in water
		|| !(ms->moveflags & MFL_ONGROUND))
	{
		//BotImport_Print(PRT_MESSAGE, "against ladder or not on ground\n");
		// RF, wolf has different ladder movement
		VectorSubtract(reach->end, reach->start, dir);
		VectorNormalize(dir);
		//set the ideal view angles, facing the ladder up or down
		viewdir[0] = dir[0];
		viewdir[1] = dir[1];
		viewdir[2] = dir[2];
		if (dir[2] < 0)			// going down, so face the other way (towards ladder)
		{
			VectorInverse(viewdir);
		}
		viewdir[2] = 0;	// straight forward goes up
		VectorNormalize(viewdir);
		VecToAngles(viewdir, result.ideal_viewangles);
		//elemantary action
		EA_Move(ms->client, origin, 0);
		if (dir[2] < 0)			// going down, so face the other way
		{
			EA_MoveBack(ms->client);
		}
		else
		{
			EA_MoveForward(ms->client);
		}
		// check for sideways adjustments to stay on the center of the ladder
		VectorMA(ms->origin, 18, viewdir, p);
		VectorCopy(reach->start, v1);
		v1[2] = ms->origin[2];
		VectorCopy(reach->end, v2);
		v2[2] = ms->origin[2];
		VectorSubtract(v2, v1, vec);
		VectorNormalize(vec);
		VectorMA(v1, -32, vec, v1);
		VectorMA(v2,  32, vec, v2);
		ProjectPointOntoVectorFromPoints(p, v1, v2, pos);
		VectorSubtract(pos, p, vec);
		if (VectorLength(vec) > 2)
		{
			AngleVectors(result.ideal_viewangles, NULL, right, NULL);
			if (DotProduct(vec, right) > 0)
			{
				EA_MoveRight(ms->client);
			}
			else
			{
				EA_MoveLeft(ms->client);
			}
		}
		//set movement view flag so the AI can see the view is focussed
		result.flags |= MOVERESULT_MOVEMENTVIEW;
	}	//end if
	else
	{
		//BotImport_Print(PRT_MESSAGE, "moving towards ladder base\n");
		// find a postion back away from the base of the ladder
		VectorSubtract(reach->end, reach->start, hordir);
		hordir[2] = 0;
		VectorNormalize(hordir);
		VectorMA(reach->start, -24, hordir, pos);
		// project our position onto the vector
		ProjectPointOntoVectorFromPoints(ms->origin, pos, reach->start, p);
		VectorSubtract(p, ms->origin, dir);
		//make sure the horizontal movement is large anough
		VectorCopy(dir, hordir);
		hordir[2] = 0;
		dist = VectorNormalize(hordir);
		if (dist < 8)		// within range, go for the end
		{	//BotImport_Print(PRT_MESSAGE, "found base, moving towards ladder top\n");
			VectorSubtract(reach->end, ms->origin, dir);
			//make sure the horizontal movement is large anough
			VectorCopy(dir, hordir);
			hordir[2] = 0;
			dist = VectorNormalize(hordir);
			//set the ideal view angles, facing the ladder up or down
			viewdir[0] = dir[0];
			viewdir[1] = dir[1];
			viewdir[2] = dir[2];
			if (dir[2] < 0)			// going down, so face the other way
			{
				VectorInverse(viewdir);
			}
			viewdir[2] = 0;
			VectorNormalize(viewdir);
			VecToAngles(viewdir, result.ideal_viewangles);
			result.flags |= MOVERESULT_MOVEMENTVIEW;
		}
		//
		dir[0] = hordir[0];
		dir[1] = hordir[1];
//		if (dist < 48) {
//			if (dir[2] > 0) dir[2] = 1;
//			else dir[2] = -1;
//		} else {
		dir[2] = 0;
//		}
		if (dist > 50)
		{
			dist = 50;
		}
		speed = 400 - (300 - 6 * dist);
		EA_Move(ms->client, dir, speed);
	}	//end else
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
			BotCheckBlocked(ms, dir, qfalse, &result);
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
			//result.failure = true;
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
		BotCheckBlocked(ms, dir, qfalse, &result);
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
	if (Q_fabs(bottomdir[2]) < Q_fabs(topdir[2]))
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
			BotCheckBlocked(ms, dir, qfalse, &result);
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
			result.type = RESULTTYPE_ELEVATORUP;
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
		BotCheckBlocked(ms, dir, qfalse, &result);
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
	if (reach.traveltype != TRAVEL_GRAPPLEHOOK)
	{
		if ((ms->moveflags & WOLFMFL_ACTIVEGRAPPLE) || ms->grapplevisible_time)
		{
			EA_Command(ms->client, CMD_HOOKOFF);
			ms->moveflags &= ~WOLFMFL_ACTIVEGRAPPLE;
			ms->grapplevisible_time = 0;
		}	//end if
	}	//end if
}	//end of the function BotResetGrapple
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_moveresult_t BotTravel_Grapple(bot_movestate_t* ms, aas_reachability_t* reach)
{
	bot_moveresult_t result;
	float dist, speed;
	vec3_t dir, viewdir, org;
	int state, areanum;

	BotClearMoveResult(&result);
	//
	if (ms->moveflags & WOLFMFL_GRAPPLERESET)
	{
		EA_Command(ms->client, CMD_HOOKOFF);
		ms->moveflags &= ~WOLFMFL_ACTIVEGRAPPLE;
		return result;
	}	//end if
		//
	if (ms->moveflags & WOLFMFL_ACTIVEGRAPPLE)
	{
		state = GrappleState(ms, reach);
		//
		VectorSubtract(reach->end, ms->origin, dir);
		dir[2] = 0;
		dist = VectorLength(dir);
		//if very close to the grapple end or
		//the grappled is hooked and the bot doesn't get any closer
		if (state && dist < 48)
		{
			if (ms->lastgrappledist - dist < 1)
			{
				EA_Command(ms->client, CMD_HOOKOFF);
				ms->moveflags &= ~WOLFMFL_ACTIVEGRAPPLE;
				ms->moveflags |= WOLFMFL_GRAPPLERESET;
				ms->reachability_time = 0;	//end the reachability
			}	//end if
		}	//end if
			//if no valid grapple at all, or the grapple hooked and the bot
			//isn't moving anymore
		else if (!state || (state == 2 && dist > ms->lastgrappledist - 2))
		{
			if (ms->grapplevisible_time < AAS_Time() - 0.4)
			{
				EA_Command(ms->client, CMD_HOOKOFF);
				ms->moveflags &= ~WOLFMFL_ACTIVEGRAPPLE;
				ms->moveflags |= WOLFMFL_GRAPPLERESET;
				ms->reachability_time = 0;	//end the reachability
				//result.failure = true;
				//result.type = WOLFRESULTTYPE_INVISIBLEGRAPPLE;
				return result;
			}	//end if
		}	//end if
		else
		{
			ms->grapplevisible_time = AAS_Time();
		}	//end else
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
			EA_Command(ms->client, CMD_HOOKON);
			ms->moveflags |= WOLFMFL_ACTIVEGRAPPLE;
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
	//
	if (dist < 5)
	{
//		BotImport_Print(PRT_MESSAGE, "between jump start and run start point\n");
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
		//
/*
    vec3_t hordir, dir1, dir2, start, end, runstart;
    float dist1, dist2, speed;
    bot_moveresult_t result;

    BotImport_Print(PRT_MESSAGE, "BotTravel_RocketJump: bah\n");
    BotClearMoveResult(&result);
    AAS_JumpReachRunStart(reach, runstart);
    //
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
        VectorMA(start, dist1+10, hordir, end);
        end[2] += 1;
        if (AAS_PointAreaNum(end) != ms->reachareanum) break;
    } //end for
    if (dist1 < 80) VectorMA(reach->start, dist1, hordir, runstart);
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
        if (dist1 < 24) EA_Jump(ms->client);
        else if (dist1 < 32) EA_DelayedJump(ms->client);
        EA_Attack(ms->client);
        EA_Move(ms->client, hordir, 800);
        //
        ms->jumpreach = ms->lastreachnum;
    } //end if
    else
    {
//		BotImport_Print(PRT_MESSAGE, "going towards run start point\n");
        hordir[0] = runstart[0] - ms->origin[0];
        hordir[1] = runstart[1] - ms->origin[1];
        hordir[2] = 0;
        VectorNormalize(hordir);
        //
        if (dist2 > 80) dist2 = 80;
        speed = 400 - (400 - 5 * dist2);
        EA_Move(ms->client, hordir, speed);
    } //end else*/
	//look in the movement direction
	VecToAngles(hordir, result.ideal_viewangles);
	//look straight down
	result.ideal_viewangles[PITCH] = 90;
	//set the view angles directly
	EA_View(ms->client, result.ideal_viewangles);
	//view is important for the movment
	result.flags |= MOVERESULT_MOVEMENTVIEWSET;
	//select the rocket launcher
	EA_SelectWeapon(ms->client, WEAPONINDEX_ROCKET_LAUNCHER);
	//weapon is used for movement
	result.weapon = WEAPONINDEX_ROCKET_LAUNCHER;
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
	bot_moveresult_t result;

	BotClearMoveResult(&result);
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
	VectorNormalize(hordir);
	//always use max speed when traveling through the air
	EA_Move(ms->client, hordir, 800);
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
	if (dist > 100 || (goal->flags & GFL_NOSLOWAPPROACH))
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
	ms->lasttime = AAS_Time();
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
	int reachnum = 0, lastreachnum, foundjumppad, ent;	// TTimo: init
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
					if (reach.traveltype != TRAVEL_ELEVATOR ||
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
					if (reach.traveltype != TRAVEL_FUNCBOB ||
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
					/* Ridah, disabled this, or standing on little fragments causes problems
					else
					{
					    result->blocked = true;
					    result->blockentity = ent;
					    result->flags |= MOVERESULT_ONTOPOFOBSTACLE;
					    return;
					} //end else
					*/
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
		//ms->areanum = BotReachabilityArea(ms->origin, (lastreach.traveltype != TRAVEL_ELEVATOR));
		ms->areanum = BotFuzzyPointReachabilityArea(ms->origin);
		//if the bot is in the goal area
		if (ms->areanum == goal->areanum)
		{
			*result = BotMoveInGoalArea(ms, goal);
			return;
		}	//end if
			//assume we can use the reachability from the last frame
		reachnum = ms->lastreachnum;
		//if there is a last reachability

// Ridah, best calc it each frame, especially for Zombie, which moves so slow that we can't afford to take a long route
//		reachnum = 0;
// done.
		if (reachnum)
		{
			AAS_ReachabilityFromNum(reachnum, &reach);
			//check if the reachability is still valid
			if (!(AAS_TravelFlagForType(reach.traveltype) & travelflags))
			{
				reachnum = 0;
			}	//end if
				//special grapple hook case
			else if (reach.traveltype == TRAVEL_GRAPPLEHOOK)
			{
				if (ms->reachability_time < AAS_Time() ||
					(ms->moveflags & WOLFMFL_GRAPPLERESET))
				{
					reachnum = 0;
				}	//end if
			}	//end if
				//special elevator case
			else if (reach.traveltype == TRAVEL_ELEVATOR || reach.traveltype == TRAVEL_FUNCBOB)
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
						AAS_PrintTravelType(reach.traveltype);
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
					ms->lastareanum != ms->areanum ||
					((ms->lasttime > (AAS_Time() - 0.5)) && (VectorDistance(ms->origin, ms->lastorigin) < 20 * (AAS_Time() - ms->lasttime))))
				{
					reachnum = 0;
					//BotImport_Print(PRT_MESSAGE, "area change or timeout\n");
				}	//end else if
			}	//end else
		}	//end if
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
				reinterpret_cast<bot_goal_t*>(goal), travelflags, travelflags, NULL, 0, NULL);
			//the area number the reachability starts in
			ms->reachareanum = ms->areanum;
			//reset some state variables
			ms->jumpreach = 0;						//for TRAVEL_JUMP
			ms->moveflags &= ~WOLFMFL_GRAPPLERESET;	//for TRAVEL_GRAPPLEHOOK
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
				memset(&reach, 0, sizeof(aas_reachability_t));		//make compiler happy
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
			switch (reach.traveltype)
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
			//case TRAVEL_BFGJUMP:
			case TRAVEL_JUMPPAD: *result = BotTravel_JumpPad(ms, &reach); break;
			case TRAVEL_FUNCBOB: *result = BotTravel_FuncBobbing(ms, &reach); break;
			default:
			{
				BotImport_Print(PRT_FATAL, "travel type %d not implemented yet\n", reach.traveltype);
				break;
			}		//end case
			}	//end switch
		}	//end if
		else
		{
			result->failure = true;
			memset(&reach, 0, sizeof(aas_reachability_t));
		}	//end else
#ifdef DEBUG
		if (bot_developer)
		{
			if (result->failure)
			{
				BotImport_Print(PRT_MESSAGE, "client %d: movement failure in ", ms->client);
				AAS_PrintTravelType(reach.traveltype);
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
		foundjumppad = qfalse;
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
					reinterpret_cast<bot_goal_t*>(goal), travelflags, TFL_JUMPPAD, NULL, 0, NULL);
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
						if (reach.traveltype == TRAVEL_JUMPPAD)
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
			//AAS_PrintTravelType(reach.traveltype);
			//BotImport_Print(PRT_MESSAGE, "\n");
#endif	//DEBUG
			//
			switch (reach.traveltype)
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
			case TRAVEL_ROCKETJUMP: *result = BotFinishTravel_WeaponJump(ms, &reach); break;
			//case TRAVEL_BFGJUMP:
			case TRAVEL_JUMPPAD: *result = BotFinishTravel_JumpPad(ms, &reach); break;
			case TRAVEL_FUNCBOB: *result = BotFinishTravel_FuncBobbing(ms, &reach); break;
			default:
			{
				BotImport_Print(PRT_FATAL, "(last) travel type %d not implemented yet\n", reach.traveltype);
				break;
			}		//end case
			}	//end switch
#ifdef DEBUG
			if (bot_developer)
			{
				if (result->failure)
				{
					BotImport_Print(PRT_MESSAGE, "client %d: movement failure in finish ", ms->client);
					AAS_PrintTravelType(reach.traveltype);
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
	ms->lasttime = AAS_Time();

	// RF, try to look in the direction we will be moving ahead of time
	if (reachnum > 0 && !(result->flags & (MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW)))
	{
		vec3_t dir;
		int ftraveltime, freachnum, straveltime, ltraveltime;

		AAS_ReachabilityFromNum(reachnum, &reach);
		if (reach.areanum != goal->areanum)
		{
			if (AAS_AreaRouteToGoalArea(reach.areanum, reach.end, goal->areanum, travelflags, &straveltime, &freachnum))
			{
				ltraveltime = 999999;
				while (AAS_AreaRouteToGoalArea(reach.areanum, reach.end, goal->areanum, travelflags, &ftraveltime, &freachnum))
				{
					// make sure we are not in a loop
					if (ftraveltime > ltraveltime)
					{
						break;
					}
					ltraveltime = ftraveltime;
					//
					AAS_ReachabilityFromNum(freachnum, &reach);
					if (reach.areanum == goal->areanum)
					{
						VectorSubtract(goal->origin, ms->origin, dir);
						VectorNormalize(dir);
						VecToAngles(dir, result->ideal_viewangles);
						result->flags |= WOLFMOVERESULT_FUTUREVIEW;
						break;
					}
					if (straveltime - ftraveltime > 120)
					{
						VectorSubtract(reach.end, ms->origin, dir);
						VectorNormalize(dir);
						VecToAngles(dir, result->ideal_viewangles);
						result->flags |= WOLFMOVERESULT_FUTUREVIEW;
						break;
					}
				}
			}
		}
		else
		{
			VectorSubtract(goal->origin, ms->origin, dir);
			VectorNormalize(dir);
			VecToAngles(dir, result->ideal_viewangles);
			result->flags |= WOLFMOVERESULT_FUTUREVIEW;
		}
	}

	//return the movement result
	return;
}	//end of the function BotMoveToGoal
