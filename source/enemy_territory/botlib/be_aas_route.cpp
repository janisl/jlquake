/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


/*****************************************************************************
 * name:		be_aas_route.c
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#include "../game/q_shared.h"
#include "../game/botlib.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_aas_def.h"

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_CreateVisibility(bool waypointsOnly);
void AAS_InitRouting(void)
{
	AAS_InitTravelFlagFromType();
	AAS_InitAreaContentsTravelFlags();
	//initialize the routing update fields
	AAS_InitRoutingUpdate();
	//create reversed reachability links used by the routing update algorithm
	AAS_CreateReversedReachability();
	//initialize the cluster cache
	AAS_InitClusterAreaCache();
	//initialize portal cache
	AAS_InitPortalCache();
	//initialize the area travel times
	AAS_CalculateAreaTravelTimes();
	//calculate the maximum travel times through portals
	AAS_InitPortalMaxTravelTimes();
	//
#ifdef ROUTING_DEBUG
	numareacacheupdates = 0;
	numportalcacheupdates = 0;
#endif	//ROUTING_DEBUG
		//
	routingcachesize = 0;
	max_routingcachesize = 1024 * (int)LibVarValue("max_routingcache", DEFAULT_MAX_ROUTINGCACHESIZE);
	max_frameroutingupdates = (int)LibVarGetValue("bot_frameroutingupdates");
	//
	// enable this for quick testing of maps without enemies
	if (LibVarGetValue("bot_norcd"))
	{
		// RF, create the waypoints for each area
		AAS_CreateVisibility(qtrue);
	}
	else
	{
		// Ridah, load or create the routing cache
		if (!AAS_ReadRouteCache())
		{
			aasworld->initialized = qtrue;	// Hack, so routing can compute traveltimes
			AAS_CreateVisibility(qfalse);
			aasworld->initialized = qfalse;

			AAS_WriteRouteCache();	// save it so we don't have to create it again
		}
		// done.
	}
}	//end of the function AAS_InitRouting
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_RandomGoalArea(int areanum, int travelflags, int* goalareanum, vec3_t goalorigin)
{
	int i, n, t;
	vec3_t start, end;
	aas_trace_t trace;

	//if the area has no reachabilities
	if (!AAS_AreaReachability(areanum))
	{
		return qfalse;
	}
	//
	n = aasworld->numareas * random();
	for (i = 0; i < aasworld->numareas; i++)
	{
		if (n <= 0)
		{
			n = 1;
		}
		if (n >= aasworld->numareas)
		{
			n = 1;
		}
		if (AAS_AreaReachability(n))
		{
			t = AAS_AreaTravelTimeToGoalArea(areanum, aasworld->areas[areanum].center, n, travelflags);
			//if the goal is reachable
			if (t > 0)
			{
				if (AAS_AreaSwim(n))
				{
					*goalareanum = n;
					VectorCopy(aasworld->areas[n].center, goalorigin);
					//BotImport_Print(PRT_MESSAGE, "found random goal area %d\n", *goalareanum);
					return qtrue;
				}	//end if
				VectorCopy(aasworld->areas[n].center, start);
				if (!AAS_PointAreaNum(start))
				{
					Log_Write("area %d center %f %f %f in solid?", n,
						start[0], start[1], start[2]);
				}
				VectorCopy(start, end);
				end[2] -= 300;
				trace = AAS_TraceClientBBox(start, end, PRESENCE_CROUCH, -1);
				if (!trace.startsolid && AAS_PointAreaNum(trace.endpos) == n)
				{
					if (AAS_AreaGroundFaceArea(n) > 300)
					{
						*goalareanum = n;
						VectorCopy(trace.endpos, goalorigin);
						//BotImport_Print(PRT_MESSAGE, "found random goal area %d\n", *goalareanum);
						return qtrue;
					}	//end if
				}	//end if
			}	//end if
		}	//end if
		n++;
	}	//end for
	return qfalse;
}	//end of the function AAS_RandomGoalArea
//===========================================================================
// just center to center visibility checking...
// FIXME: implement a correct full vis
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_CreateVisibility(bool waypointsOnly)
{
	int i, j, size, totalsize;
	vec3_t endpos, mins, maxs;
	bsp_trace_t trace;
	byte* buf;
	byte* validareas = NULL;
	int numvalid = 0;
	byte* areaTable = NULL;
	int numAreas, numAreaBits;

	numAreas = aasworld->numareas;
	numAreaBits = ((numAreas + 8) >> 3);

	if (!waypointsOnly)
	{
		validareas = (byte*)Mem_ClearedAlloc(numAreas * sizeof(byte));
	}

	aasworld->areawaypoints = (vec3_t*)Mem_ClearedAlloc(numAreas * sizeof(vec3_t));
	totalsize = numAreas * sizeof(byte*);

	for (i = 1; i < numAreas; i++)
	{
		if (!AAS_AreaReachability(i))
		{
			continue;
		}

		// find the waypoint
		VectorCopy(aasworld->areas[i].center, endpos);
		endpos[2] -= 256;
		AAS_PresenceTypeBoundingBox(PRESENCE_NORMAL, mins, maxs);
		trace = AAS_Trace(aasworld->areas[i].center, mins, maxs, endpos, -1, BSP46CONTENTS_SOLID |
			(GGameType & GAME_WolfMP ? 0 : BSP46CONTENTS_PLAYERCLIP | BSP46CONTENTS_MONSTERCLIP));
		if (GGameType & GAME_ET && trace.startsolid && trace.ent < Q3ENTITYNUM_WORLD)
		{
			trace = AAS_Trace(aasworld->areas[i].center, mins, maxs, endpos, trace.ent, BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP | BSP46CONTENTS_MONSTERCLIP);
		}
		if (!trace.startsolid && trace.fraction < 1 && AAS_PointAreaNum(trace.endpos) == i)
		{
			VectorCopy(trace.endpos, aasworld->areawaypoints[i]);
			if (!waypointsOnly)
			{
				validareas[i] = 1;
			}
			numvalid++;
		}
		else
		{
			VectorClear(aasworld->areawaypoints[i]);
		}
	}

	if (waypointsOnly)
	{
		return;
	}

	aasworld->areavisibility = (byte**)Mem_ClearedAlloc(numAreas * sizeof(byte*));
	aasworld->decompressedvis = (byte*)Mem_ClearedAlloc(numAreas * sizeof(byte));

	areaTable = (byte*)Mem_ClearedAlloc(numAreas * numAreaBits * sizeof(byte));
	buf = (byte*)Mem_ClearedAlloc(numAreas * 2 * sizeof(byte));			// in case it ends up bigger than the decompressedvis, which is rare but possible

	for (i = 1; i < numAreas; i++)
	{
		if (!validareas[i])
		{
			continue;
		}

		if (GGameType & GAME_WolfMP && !AAS_AreaReachability(i))
		{
			continue;
		}

		for (j = 1; j < numAreas; j++)
		{
			aasworld->decompressedvis[j] = 0;
			if (i == j)
			{
				aasworld->decompressedvis[j] = 1;
				areaTable[(i * numAreaBits) + (j >> 3)] |= (1 << (j & 7));
				continue;
			}

			if (!validareas[j] || (GGameType & GAME_WolfMP && !AAS_AreaReachability(j)))
			{
				if (GGameType & GAME_WolfMP)
				{
					aasworld->decompressedvis[j] = 0;
				}
				continue;
			}

			// if we have already checked this combination, copy the result
			if (!(GGameType & GAME_WolfMP) && areaTable && (i > j))
			{
				// use the reverse result stored in the table
				if (areaTable[(j * numAreaBits) + (i >> 3)] & (1 << (i & 7)))
				{
					aasworld->decompressedvis[j] = 1;
				}
				// done, move to the next area
				continue;
			}

			// RF, check PVS first, since it's much faster
			if (!(GGameType & GAME_WolfMP) && !AAS_inPVS(aasworld->areawaypoints[i], aasworld->areawaypoints[j]))
			{
				continue;
			}

			trace = AAS_Trace(aasworld->areawaypoints[i], NULL, NULL, aasworld->areawaypoints[j], -1, BSP46CONTENTS_SOLID);
			if (GGameType & GAME_ET && trace.startsolid && trace.ent < Q3ENTITYNUM_WORLD)
			{
				trace = AAS_Trace(aasworld->areas[i].center, mins, maxs, endpos, trace.ent, BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP | BSP46CONTENTS_MONSTERCLIP);
			}

			if (trace.fraction >= 1)
			{
				if (GGameType & GAME_ET)
				{
					areaTable[(i * numAreaBits) + (j >> 3)] |= (1 << (j & 7));
				}
				aasworld->decompressedvis[j] = 1;
			}
			else if (GGameType & GAME_WolfMP)
			{
				aasworld->decompressedvis[j] = 0;
			}
		}

		size = AAS_CompressVis(aasworld->decompressedvis, numAreas, buf);
		aasworld->areavisibility[i] = (byte*)Mem_Alloc(size);
		memcpy(aasworld->areavisibility[i], buf, size);
		totalsize += size;
	}

	Mem_Free(areaTable);
	BotImport_Print(PRT_MESSAGE, "AAS_CreateVisibility: compressed vis size = %i\n", totalsize);
}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_NearestHideArea(int srcnum, vec3_t origin, int areanum, int enemynum, vec3_t enemyorigin, int enemyareanum, int travelflags, float maxdist, vec3_t distpos)
{
	int i, j, nextareanum, badtravelflags, numreach, bestarea;
	unsigned short int t, besttraveltime, enemytraveltime;
	aas_routingupdate_t* updateliststart, * updatelistend, * curupdate, * nextupdate;
	aas_reachability_t* reach;
	float dist1, dist2;
	float enemytraveldist;
	vec3_t enemyVec;
	qboolean startVisible;
	vec3_t v1, v2, p;
	#define MAX_HIDEAREA_LOOPS  3000
	static float lastTime;
	static int loopCount;
	//
	if (!aasworld->areavisibility)
	{
		return 0;
	}
	//
	if (srcnum < 0)			// hack to force run this call
	{
		srcnum = -srcnum - 1;
		lastTime = 0;
	}
	// don't run this more than once per frame
	if (lastTime == AAS_Time() && loopCount >= MAX_HIDEAREA_LOOPS)
	{
		return 0;
	}
	if (lastTime != AAS_Time())
	{
		loopCount = 0;
	}
	lastTime = AAS_Time();
	//
	if (!aasworld->hidetraveltimes)
	{
		aasworld->hidetraveltimes = (unsigned short int*)Mem_ClearedAlloc(aasworld->numareas * sizeof(unsigned short int));
	}
	else
	{
		memset(aasworld->hidetraveltimes, 0, aasworld->numareas * sizeof(unsigned short int));
	}	//end else
		//
	if (!aasworld->visCache)
	{
		aasworld->visCache = (byte*)Mem_ClearedAlloc(aasworld->numareas * sizeof(byte));
	}
	else
	{
		memset(aasworld->visCache, 0, aasworld->numareas * sizeof(byte));
	}	//end else
	besttraveltime = 0;
	bestarea = 0;
	if (enemyareanum)
	{
		enemytraveltime = AAS_AreaTravelTimeToGoalArea(areanum, origin, enemyareanum, travelflags);
	}
	VectorSubtract(enemyorigin, origin, enemyVec);
	enemytraveldist = VectorNormalize(enemyVec);
	startVisible = botimport.BotVisibleFromPos(enemyorigin, enemynum, origin, srcnum, qfalse);
	//
	badtravelflags = ~travelflags;
	//
	curupdate = &aasworld->areaupdate[areanum];
	curupdate->areanum = areanum;
	VectorCopy(origin, curupdate->start);
	// GORDON: TEMP: FIXME: just avoiding a crash for the moment
	if (areanum == 0)
	{
		return 0;
	}
	curupdate->areatraveltimes = aasworld->areatraveltimes[areanum][0];
	curupdate->tmptraveltime = 0;
	//put the area to start with in the current read list
	curupdate->next = NULL;
	curupdate->prev = NULL;
	updateliststart = curupdate;
	updatelistend = curupdate;
	//while there are updates in the current list, flip the lists
	while (updateliststart)
	{
		curupdate = updateliststart;
		//
		if (curupdate->next)
		{
			curupdate->next->prev = NULL;
		}
		else
		{
			updatelistend = NULL;
		}
		updateliststart = curupdate->next;
		//
		curupdate->inlist = qfalse;
		//check all reversed reachability links
		numreach = aasworld->areasettings[curupdate->areanum].numreachableareas;
		reach = &aasworld->reachability[aasworld->areasettings[curupdate->areanum].firstreachablearea];
		//
		for (i = 0; i < numreach; i++, reach++)
		{
			//if an undesired travel type is used
			if (aasworld->travelflagfortype[reach->traveltype] & badtravelflags)
			{
				continue;
			}
			//
			if (AAS_AreaContentsTravelFlags(reach->areanum) & badtravelflags)
			{
				continue;
			}
			// dont pass through ladder areas
			if (aasworld->areasettings[reach->areanum].areaflags & AREA_LADDER)
			{
				continue;
			}
			//
			if (aasworld->areasettings[reach->areanum].areaflags & AREA_DISABLED)
			{
				continue;
			}
			//number of the area the reachability leads to
			nextareanum = reach->areanum;
			// if this moves us into the enemies area, skip it
			if (nextareanum == enemyareanum)
			{
				continue;
			}
			// if this moves us outside maxdist
			if (distpos && (VectorDistance(reach->end, distpos) > maxdist))
			{
				continue;
			}
			//time already travelled plus the traveltime through
			//the current area plus the travel time from the reachability
			t = curupdate->tmptraveltime +
				AAS_AreaTravelTime(curupdate->areanum, curupdate->start, reach->start) +
				reach->traveltime;
			// inc the loopCount, we are starting to use a bit of cpu time
			loopCount++;
			// if this isn't the fastest route to this area, ignore
			if (aasworld->hidetraveltimes[nextareanum] && aasworld->hidetraveltimes[nextareanum] < t)
			{
				continue;
			}
			aasworld->hidetraveltimes[nextareanum] = t;
			// if the bestarea is this area, then it must be a longer route, so ignore it
			if (bestarea == nextareanum)
			{
				bestarea = 0;
				besttraveltime = 0;
			}
			// do this test now, so we can reject the route if it starts out too long
			if (besttraveltime && t >= besttraveltime)
			{
				continue;
			}
			//
			//avoid going near the enemy
			ProjectPointOntoVectorFromPoints(enemyorigin, curupdate->start, reach->end, p);
			for (j = 0; j < 3; j++)
			{
				if ((p[j] > curupdate->start[j] + 0.1 && p[j] > reach->end[j] + 0.1) ||
					(p[j] < curupdate->start[j] - 0.1 && p[j] < reach->end[j] - 0.1))
				{
					break;
				}
			}
			if (j < 3)
			{
				VectorSubtract(enemyorigin, reach->end, v2);
			}	//end if
			else
			{
				VectorSubtract(enemyorigin, p, v2);
			}	//end else
			dist2 = VectorLength(v2);
			//never go through the enemy
			if (enemytraveldist > 32 && dist2 < enemytraveldist && dist2 < 256)
			{
				continue;
			}
			//
			VectorSubtract(reach->end, origin, v2);
			if (enemytraveldist > 32 && DotProduct(v2, enemyVec) > enemytraveldist / 2)
			{
				continue;
			}
			//
			VectorSubtract(enemyorigin, curupdate->start, v1);
			dist1 = VectorLength(v1);
			//
			if (enemytraveldist > 32 && dist2 < dist1)
			{
				t += (dist1 - dist2) * 10;
				// test it again after modifying it
				if (besttraveltime && t >= besttraveltime)
				{
					continue;
				}
			}
			// make sure the hide area doesn't have anyone else in it
			if (AAS_IsEntityInArea(srcnum, -1, nextareanum))
			{
				t += 1000;	// avoid this path/area
				//continue;
			}
			//
			// if we weren't visible when starting, make sure we don't move into their view
			if (enemyareanum && !startVisible && AAS_AreaVisible(enemyareanum, nextareanum))
			{
				continue;
				//t += 1000;
			}
			//
			if (!besttraveltime || besttraveltime > t)
			{
				//
				// if this area doesn't have a vis list, ignore it
				if (aasworld->areavisibility[nextareanum])
				{
					//if the nextarea is not visible from the enemy area
					if (!AAS_AreaVisible(enemyareanum, nextareanum))		// now last of all, check that this area is a safe hiding spot
					{
						if ((aasworld->visCache[nextareanum] == 2) ||
							(!aasworld->visCache[nextareanum] && !botimport.BotVisibleFromPos(enemyorigin, enemynum, aasworld->areawaypoints[nextareanum], srcnum, qfalse)))
						{
							aasworld->visCache[nextareanum] = 2;
							besttraveltime = t;
							bestarea = nextareanum;
						}
						else
						{
							aasworld->visCache[nextareanum] = 1;
						}
					}	//end if
				}
				//
				// getting down to here is bad for cpu usage
				if (loopCount++ > MAX_HIDEAREA_LOOPS)
				{
					//BotImport_Print(PRT_MESSAGE, "AAS_NearestHideArea: exceeded max loops, aborting\n" );
					continue;
				}
				//
				// otherwise, add this to the list so we check is reachables
				// disabled, this should only store the raw traveltime, not the adjusted time
				//aasworld->hidetraveltimes[nextareanum] = t;
				nextupdate = &aasworld->areaupdate[nextareanum];
				nextupdate->areanum = nextareanum;
				nextupdate->tmptraveltime = t;
				//remember where we entered this area
				VectorCopy(reach->end, nextupdate->start);
				//if this update is not in the list yet
				if (!nextupdate->inlist)
				{
					//add the new update to the end of the list
					nextupdate->next = NULL;
					nextupdate->prev = updatelistend;
					if (updatelistend)
					{
						updatelistend->next = nextupdate;
					}
					else
					{
						updateliststart = nextupdate;
					}
					updatelistend = nextupdate;
					nextupdate->inlist = qtrue;
				}	//end if
			}	//end if
		}	//end for
	}	//end while
		//BotImport_Print(PRT_MESSAGE, "AAS_NearestHideArea: hidearea: %i, %i loops\n", bestarea, count );
	return bestarea;
}	//end of the function AAS_NearestHideArea
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_FindAttackSpotWithinRange(int srcnum, int rangenum, int enemynum, float rangedist, int travelflags, float* outpos)
{
	int i, nextareanum, badtravelflags, numreach, bestarea;
	unsigned short int t, besttraveltime, enemytraveltime;
	aas_routingupdate_t* updateliststart, * updatelistend, * curupdate, * nextupdate;
	aas_reachability_t* reach;
	vec3_t srcorg, rangeorg, enemyorg;
	int srcarea, rangearea, enemyarea;
	unsigned short int srctraveltime;
	int count = 0;
	#define MAX_ATTACKAREA_LOOPS    200
	static float lastTime;
	//
	// RF, currently doesn't work with multiple AAS worlds, so only enable for the default world
	//if (aasworld != aasworlds) return 0;
	//
	// don't run this more than once per frame
	if (lastTime == AAS_Time())
	{
		return 0;
	}
	lastTime = AAS_Time();
	//
	if (!aasworld->hidetraveltimes)
	{
		aasworld->hidetraveltimes = (unsigned short int*)Mem_ClearedAlloc(aasworld->numareas * sizeof(unsigned short int));
	}
	else
	{
		memset(aasworld->hidetraveltimes, 0, aasworld->numareas * sizeof(unsigned short int));
	}	//end else
		//
	if (!aasworld->visCache)
	{
		aasworld->visCache = (byte*)Mem_ClearedAlloc(aasworld->numareas * sizeof(byte));
	}
	else
	{
		memset(aasworld->visCache, 0, aasworld->numareas * sizeof(byte));
	}	//end else
		//
	AAS_EntityOrigin(srcnum, srcorg);
	AAS_EntityOrigin(rangenum, rangeorg);
	AAS_EntityOrigin(enemynum, enemyorg);
	//
	srcarea = BotFuzzyPointReachabilityArea(srcorg);
	rangearea = BotFuzzyPointReachabilityArea(rangeorg);
	enemyarea = BotFuzzyPointReachabilityArea(enemyorg);
	//
	besttraveltime = 0;
	bestarea = 0;
	enemytraveltime = AAS_AreaTravelTimeToGoalArea(srcarea, srcorg, enemyarea, travelflags);
	//
	badtravelflags = ~travelflags;
	//
	curupdate = &aasworld->areaupdate[rangearea];
	curupdate->areanum = rangearea;
	VectorCopy(rangeorg, curupdate->start);
	curupdate->areatraveltimes = aasworld->areatraveltimes[srcarea][0];
	curupdate->tmptraveltime = 0;
	//put the area to start with in the current read list
	curupdate->next = NULL;
	curupdate->prev = NULL;
	updateliststart = curupdate;
	updatelistend = curupdate;
	//while there are updates in the current list, flip the lists
	while (updateliststart)
	{
		curupdate = updateliststart;
		//
		if (curupdate->next)
		{
			curupdate->next->prev = NULL;
		}
		else
		{
			updatelistend = NULL;
		}
		updateliststart = curupdate->next;
		//
		curupdate->inlist = qfalse;
		//check all reversed reachability links
		numreach = aasworld->areasettings[curupdate->areanum].numreachableareas;
		reach = &aasworld->reachability[aasworld->areasettings[curupdate->areanum].firstreachablearea];
		//
		for (i = 0; i < numreach; i++, reach++)
		{
			//if an undesired travel type is used
			if (aasworld->travelflagfortype[reach->traveltype] & badtravelflags)
			{
				continue;
			}
			//
			if (AAS_AreaContentsTravelFlags(reach->areanum) & badtravelflags)
			{
				continue;
			}
			// dont pass through ladder areas
			if (aasworld->areasettings[reach->areanum].areaflags & AREA_LADDER)
			{
				continue;
			}
			//
			if (aasworld->areasettings[reach->areanum].areaflags & AREA_DISABLED)
			{
				continue;
			}
			//number of the area the reachability leads to
			nextareanum = reach->areanum;
			// if this moves us into the enemies area, skip it
			if (nextareanum == enemyarea)
			{
				continue;
			}
			// if we've already been to this area
			if (aasworld->hidetraveltimes[nextareanum])
			{
				continue;
			}
			//time already travelled plus the traveltime through
			//the current area plus the travel time from the reachability
			if (count++ > MAX_ATTACKAREA_LOOPS)
			{
				//BotImport_Print(PRT_MESSAGE, "AAS_FindAttackSpotWithinRange: exceeded max loops, aborting\n" );
				if (bestarea)
				{
					VectorCopy(aasworld->areawaypoints[bestarea], outpos);
				}
				return bestarea;
			}
			t = curupdate->tmptraveltime +
				AAS_AreaTravelTime(curupdate->areanum, curupdate->start, reach->start) +
				reach->traveltime;
			//
			// if it's too far from rangenum, ignore
			if (Distance(rangeorg, aasworld->areawaypoints[nextareanum]) > rangedist)
			{
				continue;
			}
			//
			// find the traveltime from srcnum
			srctraveltime = AAS_AreaTravelTimeToGoalArea(srcarea, srcorg, nextareanum, travelflags);
			// do this test now, so we can reject the route if it starts out too long
			if (besttraveltime && srctraveltime >= besttraveltime)
			{
				continue;
			}
			//
			// if this area doesn't have a vis list, ignore it
			if (aasworld->areavisibility[nextareanum])
			{
				//if the nextarea can see the enemy area
				if (AAS_AreaVisible(enemyarea, nextareanum))		// now last of all, check that this area is a good attacking spot
				{
					if ((aasworld->visCache[nextareanum] == 2) ||
						(!aasworld->visCache[nextareanum] &&
						 (count += 10) &&				// we are about to use lots of CPU time
						 botimport.BotCheckAttackAtPos(srcnum, enemynum, aasworld->areawaypoints[nextareanum], qfalse, qfalse)))
					{
						aasworld->visCache[nextareanum] = 2;
						besttraveltime = srctraveltime;
						bestarea = nextareanum;
					}
					else
					{
						aasworld->visCache[nextareanum] = 1;
					}
				}	//end if
			}
			aasworld->hidetraveltimes[nextareanum] = t;
			nextupdate = &aasworld->areaupdate[nextareanum];
			nextupdate->areanum = nextareanum;
			nextupdate->tmptraveltime = t;
			//remember where we entered this area
			VectorCopy(reach->end, nextupdate->start);
			//if this update is not in the list yet
			if (!nextupdate->inlist)
			{
				//add the new update to the end of the list
				nextupdate->next = NULL;
				nextupdate->prev = updatelistend;
				if (updatelistend)
				{
					updatelistend->next = nextupdate;
				}
				else
				{
					updateliststart = nextupdate;
				}
				updatelistend = nextupdate;
				nextupdate->inlist = qtrue;
			}	//end if
		}	//end for
	}	//end while
//BotImport_Print(PRT_MESSAGE, "AAS_NearestHideArea: hidearea: %i, %i loops\n", bestarea, count );
	if (bestarea)
	{
		VectorCopy(aasworld->areawaypoints[bestarea], outpos);
	}
	return bestarea;
}	//end of the function AAS_NearestHideArea

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
qboolean AAS_GetRouteFirstVisPos(vec3_t srcpos, vec3_t destpos, int travelflags, vec3_t retpos)
{
	int srcarea, destarea, travarea;
	vec3_t travpos;
	int ftraveltime, freachnum, lasttraveltime;
	aas_reachability_t reach;
	int loops = 0;
#define MAX_GETROUTE_VISPOS_LOOPS   200
	//
	// SRCPOS: enemy
	// DESTPOS: self
	// RETPOS: first area that is visible from destpos, in route from srcpos to destpos
	srcarea = BotFuzzyPointReachabilityArea(srcpos);
	if (!srcarea)
	{
		return qfalse;
	}
	destarea = BotFuzzyPointReachabilityArea(destpos);
	if (!destarea)
	{
		return qfalse;
	}
	if (destarea == srcarea)
	{
		VectorCopy(srcpos, retpos);
		return qtrue;
	}
	//
	//if the srcarea can see the destarea
	if (AAS_AreaVisible(srcarea, destarea))
	{
		VectorCopy(srcpos, retpos);
		return qtrue;
	}
	// if this area doesn't have a vis list, ignore it
	if (!aasworld->areavisibility[destarea])
	{
		return qfalse;
	}
	//
	travarea = srcarea;
	VectorCopy(srcpos, travpos);
	lasttraveltime = -1;
	while ((loops++ < MAX_GETROUTE_VISPOS_LOOPS) && AAS_AreaRouteToGoalArea(travarea, travpos, destarea, travelflags, &ftraveltime, &freachnum))
	{
		if (lasttraveltime >= 0 && ftraveltime >= lasttraveltime)
		{
			return qfalse;	// we may be in a loop
		}
		lasttraveltime = ftraveltime;
		//
		AAS_ReachabilityFromNum(freachnum, &reach);
		if (reach.areanum == destarea)
		{
			VectorCopy(travpos, retpos);
			return qtrue;
		}
		//if the reach area can see the destarea
		if (AAS_AreaVisible(reach.areanum, destarea))
		{
			VectorCopy(reach.end, retpos);
			return qtrue;
		}
		//
		travarea = reach.areanum;
		VectorCopy(reach.end, travpos);
	}
	//
	// unsuccessful
	return qfalse;
}
