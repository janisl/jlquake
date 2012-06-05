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
 * name:		be_aas_debug.c
 *
 * desc:		AAS debug code
 *
 * $Archive: /MissionPack/code/botlib/be_aas_debug.c $
 *
 *****************************************************************************/

#include "../common/qcommon.h"
#include "botlib.h"
#include "be_interface.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_ShowReachability(aas_reachability_t* reach)
{
	vec3_t dir, cmdmove, velocity;
	float speed, zvel;
	aas_clientmove_q3_t move;

	AAS_ShowAreaPolygons(reach->areanum, 5, true);
	AAS_DrawArrow(reach->start, reach->end, LINECOLOR_BLUE, LINECOLOR_YELLOW);
	//
	if ((reach->traveltype & TRAVELTYPE_MASK) == TRAVEL_JUMP ||
		(reach->traveltype & TRAVELTYPE_MASK) == TRAVEL_WALKOFFLEDGE)
	{
		AAS_HorizontalVelocityForJump(aassettings.phys_jumpvel, reach->start, reach->end, &speed);
		//
		VectorSubtract(reach->end, reach->start, dir);
		dir[2] = 0;
		VectorNormalize(dir);
		//set the velocity
		VectorScale(dir, speed, velocity);
		//set the command movement
		VectorClear(cmdmove);
		cmdmove[2] = aassettings.phys_jumpvel;
		//
		AAS_PredictClientMovementQ3(&move, -1, reach->start, PRESENCE_NORMAL, true,
			velocity, cmdmove, 3, 30, 0.1f,
			SE_HITGROUND | SE_ENTERWATER | SE_ENTERSLIME |
			SE_ENTERLAVA | SE_HITGROUNDDAMAGE, 0, true);
		//
		if ((reach->traveltype & TRAVELTYPE_MASK) == TRAVEL_JUMP)
		{
			AAS_JumpReachRunStart(reach, dir);
			AAS_DrawCross(dir, 4, LINECOLOR_BLUE);
		}	//end if
	}	//end if
	else if ((reach->traveltype & TRAVELTYPE_MASK) == TRAVEL_ROCKETJUMP)
	{
		zvel = AAS_RocketJumpZVelocity(reach->start);
		AAS_HorizontalVelocityForJump(zvel, reach->start, reach->end, &speed);
		//
		VectorSubtract(reach->end, reach->start, dir);
		dir[2] = 0;
		VectorNormalize(dir);
		//get command movement
		VectorScale(dir, speed, cmdmove);
		VectorSet(velocity, 0, 0, zvel);
		//
		AAS_PredictClientMovementQ3(&move, -1, reach->start, PRESENCE_NORMAL, true,
			velocity, cmdmove, 30, 30, 0.1f,
			SE_ENTERWATER | SE_ENTERSLIME |
			SE_ENTERLAVA | SE_HITGROUNDDAMAGE |
			SE_TOUCHJUMPPAD | SE_HITGROUNDAREA, reach->areanum, true);
	}	//end else if
	else if ((reach->traveltype & TRAVELTYPE_MASK) == TRAVEL_JUMPPAD)
	{
		VectorSet(cmdmove, 0, 0, 0);
		//
		VectorSubtract(reach->end, reach->start, dir);
		dir[2] = 0;
		VectorNormalize(dir);
		//set the velocity
		//NOTE: the edgenum is the horizontal velocity
		VectorScale(dir, reach->edgenum, velocity);
		//NOTE: the facenum is the Z velocity
		velocity[2] = reach->facenum;
		//
		AAS_PredictClientMovementQ3(&move, -1, reach->start, PRESENCE_NORMAL, true,
			velocity, cmdmove, 30, 30, 0.1f,
			SE_ENTERWATER | SE_ENTERSLIME |
			SE_ENTERLAVA | SE_HITGROUNDDAMAGE |
			SE_TOUCHJUMPPAD | SE_HITGROUNDAREA, reach->areanum, true);
	}	//end else if
}	//end of the function AAS_ShowReachability
