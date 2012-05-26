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

//===========================================================================
//
// Name:			be_aas_routetable.c
// Function:		Area Awareness System, Route-table defines
// Programmer:		Ridah
// Tab Size:		3
//===========================================================================

#include "../game/q_shared.h"
#include "l_memory.h"
#include "l_utils.h"
#include "../game/botlib.h"
#include "be_interface.h"
#include "../game/be_aas.h"
#include "be_aas_def.h"

// hmm, is there a cleaner way of finding out memory usage?
extern int totalmemorysize;
static int memorycount;

//===========================================================================
// Memory debugging/optimization

void AAS_RT_FreeMemory(void* ptr)
{
	int before;

	before = totalmemorysize;

	// FreeMemory( ptr );
	// Ryan - 01102k
	free(ptr);

	memorycount -= before - totalmemorysize;
}
//===========================================================================
//	free permanent memory used by route-table system
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_RT_ShutdownRouteTable(void)
{
	if (!aasworld->routetable)
	{
		return;
	}

	// free the dynamic lists
	AAS_RT_FreeMemory(aasworld->routetable->areaChildIndexes);
	AAS_RT_FreeMemory(aasworld->routetable->children);
	AAS_RT_FreeMemory(aasworld->routetable->parents);
	AAS_RT_FreeMemory(aasworld->routetable->parentChildren);
	AAS_RT_FreeMemory(aasworld->routetable->visibleParents);
//	AAS_RT_FreeMemory( aasworld->routetable->localRoutes );
//	AAS_RT_FreeMemory( aasworld->routetable->parentRoutes );
	AAS_RT_FreeMemory(aasworld->routetable->parentLinks);
//	AAS_RT_FreeMemory( aasworld->routetable->routeIndexes );
//	AAS_RT_FreeMemory( aasworld->routetable->parentTravelTimes );

	// kill the table
	AAS_RT_FreeMemory(aasworld->routetable);
	aasworld->routetable = NULL;
}

//===========================================================================
//	draws the route-table from src to dest
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
#include "../game/be_ai_goal.h"
int BotGetReachabilityToGoal(vec3_t origin, int areanum, int entnum,
	int lastgoalareanum, int lastareanum,
	int* avoidreach, float* avoidreachtimes, int* avoidreachtries,
	bot_goal_q3_t* goal, int travelflags, int movetravelflags);

void AAS_RT_ShowRoute(vec3_t srcpos, int srcnum, int destnum)
{
#ifdef DEBUG
#define MAX_RT_AVOID_REACH 1
	AAS_ClearShownPolygons();
	AAS_ClearShownDebugLines();
	AAS_ShowAreaPolygons(srcnum, 1, qtrue);
	AAS_ShowAreaPolygons(destnum, 4, qtrue);
	{
		static int lastgoalareanum, lastareanum;
		static int avoidreach[MAX_RT_AVOID_REACH];
		static float avoidreachtimes[MAX_RT_AVOID_REACH];
		static int avoidreachtries[MAX_RT_AVOID_REACH];
		int reachnum;
		bot_goal_q3_t goal;
		aas_reachability_t reach;

		goal.areanum = destnum;
		VectorCopy(botlibglobals.goalorigin, goal.origin);
		reachnum = BotGetReachabilityToGoal(srcpos, srcnum, -1,
			lastgoalareanum, lastareanum,
			avoidreach, avoidreachtimes, avoidreachtries,
			&goal, WMTFL_DEFAULT | TFL_FUNCBOB, WMTFL_DEFAULT | TFL_FUNCBOB);
		AAS_ReachabilityFromNum(reachnum, &reach);
		AAS_ShowReachability(&reach);
	}
#endif
}

/*
=================
AAS_RT_GetHidePos

  "src" is hiding ent, "dest" is the enemy
=================
*/
int AAS_NearestHideArea(int srcnum, vec3_t origin, int areanum, int enemynum, vec3_t enemyorigin, int enemyareanum, int travelflags);
qboolean AAS_RT_GetHidePos(vec3_t srcpos, int srcnum, int srcarea, vec3_t destpos, int destnum, int destarea, vec3_t returnPos)
{
	static int tfl = WMTFL_DEFAULT & ~(TFL_JUMPPAD | TFL_ROCKETJUMP | TFL_BFGJUMP | TFL_GRAPPLEHOOK | TFL_DOUBLEJUMP | TFL_RAMPJUMP | TFL_STRAFEJUMP | TFL_LAVA);		//----(SA)	modified since slime is no longer deadly

	// use MrE's breadth first method
	int hideareanum;

	hideareanum = AAS_NearestHideArea(srcnum, srcpos, srcarea, destnum, destpos, destarea, tfl);
	if (!hideareanum)
	{
		return qfalse;
	}
	// we found a valid hiding area
	VectorCopy((*aasworld).areawaypoints[hideareanum], returnPos);

	return qtrue;
}
