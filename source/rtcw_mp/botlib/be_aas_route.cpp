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
 * name:		be_aas_route.c
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#include "../game/q_shared.h"
#include "../game/botlib.h"
#include "be_interface.h"
#include "../game/be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_CreateVisibility(void);
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
	max_frameroutingupdates = MAX_FRAMEROUTINGUPDATES_WOLF;
		//
	routingcachesize = 0;
	max_routingcachesize = 1024 * (int)LibVarValue("max_routingcache", "4096");
	//
	// Ridah, load or create the routing cache
	if (!AAS_ReadRouteCache())
	{
		aasworld->initialized = qtrue;		// Hack, so routing can compute traveltimes
		AAS_CreateVisibility();
		AAS_CreateAllRoutingCache();
		aasworld->initialized = qfalse;

		AAS_WriteRouteCache();	// save it so we don't have to create it again
	}
	// done.
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
void AAS_CreateVisibility(void)
{
	int i, j, size, totalsize;
	vec3_t endpos, mins, maxs;
	bsp_trace_t trace;
	byte* buf;
	byte* validareas;
	int numvalid = 0;

	buf = (byte*)Mem_ClearedAlloc(aasworld->numareas * 2 * sizeof(byte));				// in case it ends up bigger than the decompressedvis, which is rare but possible
	validareas = (byte*)Mem_ClearedAlloc(aasworld->numareas * sizeof(byte));

	aasworld->areavisibility = (byte**)Mem_ClearedAlloc(aasworld->numareas * sizeof(byte*));
	aasworld->decompressedvis = (byte*)Mem_ClearedAlloc(aasworld->numareas * sizeof(byte));
	aasworld->areawaypoints = (vec3_t*)Mem_ClearedAlloc(aasworld->numareas * sizeof(vec3_t));
	totalsize = aasworld->numareas * sizeof(byte*);
	for (i = 1; i < aasworld->numareas; i++)
	{
		if (!AAS_AreaReachability(i))
		{
			continue;
		}

		// find the waypoint
		VectorCopy(aasworld->areas[i].center, endpos);
		endpos[2] -= 256;
		AAS_PresenceTypeBoundingBox(PRESENCE_NORMAL, mins, maxs);
//		maxs[2] = 0;
		trace = AAS_Trace(aasworld->areas[i].center, mins, maxs, endpos, -1, BSP46CONTENTS_SOLID);
		if (!trace.startsolid && trace.fraction < 1 && AAS_PointAreaNum(trace.endpos) == i)
		{
			VectorCopy(trace.endpos, aasworld->areawaypoints[i]);
			validareas[i] = 1;
			numvalid++;
		}
		else
		{
			continue;
		}
	}

	for (i = 1; i < aasworld->numareas; i++)
	{
		if (!validareas[i])
		{
			continue;
		}

		if (!AAS_AreaReachability(i))
		{
			continue;
		}

		for (j = 1; j < aasworld->numareas; j++)
		{
			if (i == j)
			{
				aasworld->decompressedvis[j] = 1;
				continue;
			}
			if (!validareas[j] || !AAS_AreaReachability(j))
			{
				aasworld->decompressedvis[j] = 0;
				continue;
			}	//end if

			// Ridah, this always returns false?!
			//if (AAS_inPVS( aasworld->areawaypoints[i], aasworld->areawaypoints[j] ))
			trace = AAS_Trace(aasworld->areawaypoints[i], NULL, NULL, aasworld->areawaypoints[j], -1, BSP46CONTENTS_SOLID);
			if (trace.fraction >= 1)
			{
				//if (botimport.inPVS( aasworld->areawaypoints[i], aasworld->areawaypoints[j] ))
				aasworld->decompressedvis[j] = 1;
			}	//end if
			else
			{
				aasworld->decompressedvis[j] = 0;
			}	//end else
		}	//end for
		size = AAS_CompressVis(aasworld->decompressedvis, aasworld->numareas, buf);
		aasworld->areavisibility[i] = (byte*)Mem_Alloc(size);
		memcpy(aasworld->areavisibility[i], buf, size);
		totalsize += size;
	}	//end for
	BotImport_Print(PRT_MESSAGE, "AAS_CreateVisibility: compressed vis size = %i\n", totalsize);
}	//end of the function AAS_CreateVisibility
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_NearestHideArea(int srcnum, vec3_t origin, int areanum, int enemynum, vec3_t enemyorigin, int enemyareanum, int travelflags)
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
	int count = 0;
	#define MAX_HIDEAREA_LOOPS  4000
	//
	// don't run this more than once per frame
	static float lastTime;
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
	besttraveltime = 0;
	bestarea = 0;
	if (enemyareanum)
	{
		enemytraveltime = AAS_AreaTravelTimeToGoalArea(areanum, origin, enemyareanum, travelflags);
	}
	VectorSubtract(enemyorigin, origin, enemyVec);
	enemytraveldist = VectorNormalize(enemyVec);
	startVisible = botimport.AICast_VisibleFromPos(enemyorigin, enemynum, origin, srcnum, qfalse);
	//
	badtravelflags = ~travelflags;
	//
	curupdate = &aasworld->areaupdate[areanum];
	curupdate->areanum = areanum;
	VectorCopy(origin, curupdate->start);
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
			//time already travelled plus the traveltime through
			//the current area plus the travel time from the reachability
			t = curupdate->tmptraveltime +
				AAS_AreaTravelTime(curupdate->areanum, curupdate->start, reach->start) +
				reach->traveltime;
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
							(!aasworld->visCache[nextareanum] && !botimport.AICast_VisibleFromPos(enemyorigin, enemynum, aasworld->areawaypoints[nextareanum], srcnum, qfalse)))
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
				if (count++ > MAX_HIDEAREA_LOOPS)
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
	//
	// don't run this more than once per frame
	static float lastTime;
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
	srcarea = AAS_BestReachableEntityArea(srcnum);
	rangearea = AAS_BestReachableEntityArea(rangenum);
	enemyarea = AAS_BestReachableEntityArea(enemynum);
	//
	AAS_EntityOrigin(srcnum, srcorg);
	AAS_EntityOrigin(rangenum, rangeorg);
	AAS_EntityOrigin(enemynum, enemyorg);
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
						 botimport.AICast_CheckAttackAtPos(srcnum, enemynum, aasworld->areawaypoints[nextareanum], qfalse, qfalse)))
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
