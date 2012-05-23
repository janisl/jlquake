/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


/*****************************************************************************
 * name:		be_aas_debug.c
 *
 * desc:		AAS debug code
 *
 *
 *****************************************************************************/

#include "../game/q_shared.h"
#include "l_memory.h"
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
void AAS_ShowFacePolygon(int facenum, int color, int flip)
{
	int i, edgenum, numpoints;
	vec3_t points[128];
	aas_edge_t* edge;
	aas_face_t* face;

	//check if face number is in range
	if (facenum >= (*aasworld).numfaces)
	{
		BotImport_Print(PRT_ERROR, "facenum %d out of range\n", facenum);
	}	//end if
	face = &(*aasworld).faces[facenum];
	//walk through the edges of the face
	numpoints = 0;
	if (flip)
	{
		for (i = face->numedges - 1; i >= 0; i--)
		{
			//edge number
			edgenum = (*aasworld).edgeindex[face->firstedge + i];
			edge = &(*aasworld).edges[abs(edgenum)];
			VectorCopy((*aasworld).vertexes[edge->v[edgenum < 0]], points[numpoints]);
			numpoints++;
		}	//end for
	}	//end if
	else
	{
		for (i = 0; i < face->numedges; i++)
		{
			//edge number
			edgenum = (*aasworld).edgeindex[face->firstedge + i];
			edge = &(*aasworld).edges[abs(edgenum)];
			VectorCopy((*aasworld).vertexes[edge->v[edgenum < 0]], points[numpoints]);
			numpoints++;
		}	//end for
	}	//end else
	AAS_ShowPolygon(color, numpoints, points);
}	//end of the function AAS_ShowFacePolygon
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_ShowAreaPolygons(int areanum, int color, int groundfacesonly)
{
	int i, facenum;
	aas_area_t* area;
	aas_face_t* face;

	//
	if (areanum < 0 || areanum >= (*aasworld).numareas)
	{
		BotImport_Print(PRT_ERROR, "area %d out of range [0, %d]\n",
			areanum, (*aasworld).numareas);
		return;
	}	//end if
		//pointer to the convex area
	area = &(*aasworld).areas[areanum];
	//walk through the faces of the area
	for (i = 0; i < area->numfaces; i++)
	{
		facenum = abs((*aasworld).faceindex[area->firstface + i]);
		//check if face number is in range
		if (facenum >= (*aasworld).numfaces)
		{
			BotImport_Print(PRT_ERROR, "facenum %d out of range\n", facenum);
		}	//end if
		face = &(*aasworld).faces[facenum];
		//ground faces only
		if (groundfacesonly)
		{
			if (!(face->faceflags & (FACE_GROUND | FACE_LADDER)))
			{
				continue;
			}
		}	//end if
		AAS_ShowFacePolygon(facenum, color, face->frontarea != areanum);
	}	//end for
}	//end of the function AAS_ShowAreaPolygons
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_DrawCross(vec3_t origin, float size, int color)
{
	int i;
	vec3_t start, end;

	for (i = 0; i < 3; i++)
	{
		VectorCopy(origin, start);
		start[i] += size;
		VectorCopy(origin, end);
		end[i] -= size;
		AAS_DebugLine(start, end, color);
	}	//end for
}	//end of the function AAS_DrawCross
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_PrintTravelType(int traveltype)
{
#ifdef DEBUG
	char* str;
	//
	switch (traveltype)
	{
	case TRAVEL_INVALID: str = "TRAVEL_INVALID"; break;
	case TRAVEL_WALK: str = "TRAVEL_WALK"; break;
	case TRAVEL_CROUCH: str = "TRAVEL_CROUCH"; break;
	case TRAVEL_BARRIERJUMP: str = "TRAVEL_BARRIERJUMP"; break;
	case TRAVEL_JUMP: str = "TRAVEL_JUMP"; break;
	case TRAVEL_LADDER: str = "TRAVEL_LADDER"; break;
	case TRAVEL_WALKOFFLEDGE: str = "TRAVEL_WALKOFFLEDGE"; break;
	case TRAVEL_SWIM: str = "TRAVEL_SWIM"; break;
	case TRAVEL_WATERJUMP: str = "TRAVEL_WATERJUMP"; break;
	case TRAVEL_TELEPORT: str = "TRAVEL_TELEPORT"; break;
	case TRAVEL_ELEVATOR: str = "TRAVEL_ELEVATOR"; break;
	case TRAVEL_ROCKETJUMP: str = "TRAVEL_ROCKETJUMP"; break;
	case TRAVEL_BFGJUMP: str = "TRAVEL_BFGJUMP"; break;
	case TRAVEL_GRAPPLEHOOK: str = "TRAVEL_GRAPPLEHOOK"; break;
	case TRAVEL_JUMPPAD: str = "TRAVEL_JUMPPAD"; break;
	case TRAVEL_FUNCBOB: str = "TRAVEL_FUNCBOB"; break;
	default: str = "UNKNOWN TRAVEL TYPE"; break;
	}	//end switch
	BotImport_Print(PRT_MESSAGE, "%s", str);
#endif	//DEBUG
}	//end of the function AAS_PrintTravelType
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_DrawArrow(vec3_t start, vec3_t end, int linecolor, int arrowcolor)
{
	vec3_t dir, cross, p1, p2, up = {0, 0, 1};
	float dot;

	VectorSubtract(end, start, dir);
	VectorNormalize(dir);
	dot = DotProduct(dir, up);
	if (dot > 0.99 || dot < -0.99)
	{
		VectorSet(cross, 1, 0, 0);
	}
	else
	{
		CrossProduct(dir, up, cross);
	}

	VectorMA(end, -6, dir, p1);
	VectorCopy(p1, p2);
	VectorMA(p1, 6, cross, p1);
	VectorMA(p2, -6, cross, p2);

	AAS_DebugLine(start, end, linecolor);
	AAS_DebugLine(p1, end, arrowcolor);
	AAS_DebugLine(p2, end, arrowcolor);
}	//end of the function AAS_DrawArrow
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
	aas_clientmove_t move;

	AAS_ShowAreaPolygons(reach->areanum, 5, qtrue);
	AAS_DrawArrow(reach->start, reach->end, LINECOLOR_BLUE, LINECOLOR_YELLOW);
	//
	if (reach->traveltype == TRAVEL_JUMP || reach->traveltype == TRAVEL_WALKOFFLEDGE)
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
		AAS_PredictClientMovement(&move, -1, reach->start, PRESENCE_NORMAL, qtrue,
			velocity, cmdmove, 3, 30, 0.1,
			SE_HITGROUND | SE_ENTERWATER | SE_ENTERSLIME |
			SE_ENTERLAVA | SE_HITGROUNDDAMAGE, 0, qtrue);
		//
		if (reach->traveltype == TRAVEL_JUMP)
		{
			AAS_JumpReachRunStart(reach, dir);
			AAS_DrawCross(dir, 4, LINECOLOR_BLUE);
		}	//end if
	}	//end if
	else if (reach->traveltype == TRAVEL_ROCKETJUMP)
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
		AAS_PredictClientMovement(&move, -1, reach->start, PRESENCE_NORMAL, qtrue,
			velocity, cmdmove, 30, 30, 0.1,
			SE_ENTERWATER | SE_ENTERSLIME |
			SE_ENTERLAVA | SE_HITGROUNDDAMAGE |
			SE_TOUCHJUMPPAD | SE_HITGROUNDAREA, reach->areanum, qtrue);
	}	//end else if
	else if (reach->traveltype == TRAVEL_JUMPPAD)
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
		AAS_PredictClientMovement(&move, -1, reach->start, PRESENCE_NORMAL, qtrue,
			velocity, cmdmove, 30, 30, 0.1,
			SE_ENTERWATER | SE_ENTERSLIME |
			SE_ENTERLAVA | SE_HITGROUNDDAMAGE |
			SE_TOUCHJUMPPAD | SE_HITGROUNDAREA, reach->areanum, qtrue);
	}	//end else if
}	//end of the function AAS_ShowReachability
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_ShowReachableAreas(int areanum)
{
	aas_areasettings_t* settings;
	static aas_reachability_t reach;
	static int index, lastareanum;
	static float lasttime;

	if (areanum != lastareanum)
	{
		index = 0;
		lastareanum = areanum;
	}	//end if
	settings = &(*aasworld).areasettings[areanum];
	//
	if (!settings->numreachableareas)
	{
		return;
	}
	//
	if (index >= settings->numreachableareas)
	{
		index = 0;
	}
	//
	if (AAS_Time() - lasttime > 1.5)
	{
		memcpy(&reach, &(*aasworld).reachability[settings->firstreachablearea + index], sizeof(aas_reachability_t));
		index++;
		lasttime = AAS_Time();
		AAS_PrintTravelType(reach.traveltype);
		BotImport_Print(PRT_MESSAGE, "(traveltime: %i)\n", reach.traveltime);
	}	//end if
	AAS_ShowReachability(&reach);
}	//end of the function ShowReachableAreas
