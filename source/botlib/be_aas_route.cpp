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
 * name:		be_aas_route.c
 *
 * desc:		AAS
 *
 * $Archive: /MissionPack/code/botlib/be_aas_route.c $
 *
 *****************************************************************************/

#include "../common/qcommon.h"
#include "l_utils.h"
#include "botlib.h"
#include "be_interface.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
unsigned short int AAS_AreaTravelTime(int areanum, vec3_t start, vec3_t end)
{
	int intdist;

	float dist = VectorDistance(start, end);

	if (GGameType & (GAME_WolfSP | GAME_WolfMP))
	{
		// Ridah, factor in the groundsteepness now
		dist *= AAS_AreaGroundSteepnessScale(areanum);
	}

	//if crouch only area
	if (AAS_AreaCrouch(areanum))
	{
		dist *= DISTANCEFACTOR_CROUCH;
	}
	//if swim area
	else if (AAS_AreaSwim(areanum))
	{
		dist *= DISTANCEFACTOR_SWIM;
	}
	//normal walk area
	else
	{
		dist *= DISTANCEFACTOR_WALK;
	}
	//
	intdist = (int)dist;
	//make sure the distance isn't zero
	if (intdist <= 0)
	{
		intdist = 1;
	}
	return intdist;
}	//end of the function AAS_AreaTravelTime
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_CalculateAreaTravelTimes(void)
{
	int i, l, n, size;
	char* ptr;
	vec3_t end;
	aas_reversedreachability_t* revreach;
	aas_reversedlink_t* revlink;
	aas_reachability_t* reach;
	aas_areasettings_t* settings;
	int starttime;

	starttime = Sys_Milliseconds();
	//if there are still area travel times, free the memory
	if (aasworld->areatraveltimes)
	{
		Mem_Free(aasworld->areatraveltimes);
	}
	//get the total size of all the area travel times
	size = aasworld->numareas * sizeof(unsigned short**);
	for (i = 0; i < aasworld->numareas; i++)
	{
		revreach = &aasworld->reversedreachability[i];
		//settings of the area
		settings = &aasworld->areasettings[i];
		//
		size += settings->numreachableareas * sizeof(unsigned short*);
		//
		size += settings->numreachableareas * revreach->numlinks * sizeof(unsigned short);
	}	//end for
		//allocate memory for the area travel times
	ptr = (char*)Mem_ClearedAlloc(size);
	aasworld->areatraveltimes = (unsigned short***)ptr;
	ptr += aasworld->numareas * sizeof(unsigned short**);
	//calcluate the travel times for all the areas
	for (i = 0; i < aasworld->numareas; i++)
	{
		//reversed reachabilities of this area
		revreach = &aasworld->reversedreachability[i];
		//settings of the area
		settings = &aasworld->areasettings[i];
		//
		aasworld->areatraveltimes[i] = (unsigned short**)ptr;
		ptr += settings->numreachableareas * sizeof(unsigned short*);
		//
		for (l = 0; l < settings->numreachableareas; l++)
		{
			aasworld->areatraveltimes[i][l] = (unsigned short*)ptr;
			ptr += revreach->numlinks * sizeof(unsigned short);
			//reachability link
			reach = &aasworld->reachability[settings->firstreachablearea + l];
			//
			for (n = 0, revlink = revreach->first; revlink; revlink = revlink->next, n++)
			{
				VectorCopy(aasworld->reachability[revlink->linknum].end, end);
				//
				aasworld->areatraveltimes[i][l][n] = AAS_AreaTravelTime(i, end, reach->start);
			}	//end for
		}	//end for
	}	//end for
#ifdef DEBUG
	BotImport_Print(PRT_MESSAGE, "area travel times %d msec\n", Sys_Milliseconds() - starttime);
#endif
}	//end of the function AAS_CalculateAreaTravelTimes
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_CreateAllRoutingCache(void)
{
	int i, j, t;

	aasworld->initialized = true;
	BotImport_Print(PRT_MESSAGE, "AAS_CreateAllRoutingCache\n");
	for (i = 1; i < aasworld->numareas; i++)
	{
		if (!AAS_AreaReachability(i))
		{
			continue;
		}
		for (j = 1; j < aasworld->numareas; j++)
		{
			if (i == j)
			{
				continue;
			}
			if (!AAS_AreaReachability(j))
			{
				continue;
			}
			t = AAS_AreaTravelTimeToGoalArea(i, aasworld->areas[i].center, j, Q3TFL_DEFAULT);
			//Log_Write("traveltime from %d to %d is %d", i, j, t);
		}	//end for
	}	//end for
	aasworld->initialized = false;
}	//end of the function AAS_CreateAllRoutingCache
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
#define MAX_REACHABILITYPASSAREAS       32

void AAS_InitReachabilityAreas(void)
{
	int i, j, numareas, areas[MAX_REACHABILITYPASSAREAS];
	int numreachareas;
	aas_reachability_t* reach;
	vec3_t start, end;

	if (aasworld->reachabilityareas)
	{
		Mem_Free(aasworld->reachabilityareas);
	}
	if (aasworld->reachabilityareaindex)
	{
		Mem_Free(aasworld->reachabilityareaindex);
	}

	aasworld->reachabilityareas = (aas_reachabilityareas_t*)
								 Mem_ClearedAlloc(aasworld->reachabilitysize * sizeof(aas_reachabilityareas_t));
	aasworld->reachabilityareaindex = (int*)
									 Mem_ClearedAlloc(aasworld->reachabilitysize * MAX_REACHABILITYPASSAREAS * sizeof(int));
	numreachareas = 0;
	for (i = 0; i < aasworld->reachabilitysize; i++)
	{
		reach = &aasworld->reachability[i];
		numareas = 0;
		switch (reach->traveltype & TRAVELTYPE_MASK)
		{
		//trace areas from start to end
		case TRAVEL_BARRIERJUMP:
		case TRAVEL_WATERJUMP:
			VectorCopy(reach->start, end);
			end[2] = reach->end[2];
			numareas = AAS_TraceAreas(reach->start, end, areas, NULL, MAX_REACHABILITYPASSAREAS);
			break;
		case TRAVEL_WALKOFFLEDGE:
			VectorCopy(reach->end, start);
			start[2] = reach->start[2];
			numareas = AAS_TraceAreas(start, reach->end, areas, NULL, MAX_REACHABILITYPASSAREAS);
			break;
		case TRAVEL_GRAPPLEHOOK:
			numareas = AAS_TraceAreas(reach->start, reach->end, areas, NULL, MAX_REACHABILITYPASSAREAS);
			break;

		//trace arch
		case TRAVEL_JUMP: break;
		case TRAVEL_ROCKETJUMP: break;
		case TRAVEL_BFGJUMP: break;
		case TRAVEL_JUMPPAD: break;

		//trace from reach->start to entity center, along entity movement
		//and from entity center to reach->end
		case TRAVEL_ELEVATOR: break;
		case TRAVEL_FUNCBOB: break;

		//no areas in between
		case TRAVEL_WALK: break;
		case TRAVEL_CROUCH: break;
		case TRAVEL_LADDER: break;
		case TRAVEL_SWIM: break;
		case TRAVEL_TELEPORT: break;
		default: break;
		}	//end switch
		aasworld->reachabilityareas[i].firstarea = numreachareas;
		aasworld->reachabilityareas[i].numareas = numareas;
		for (j = 0; j < numareas; j++)
		{
			aasworld->reachabilityareaindex[numreachareas++] = areas[j];
		}	//end for
	}	//end for
}	//end of the function AAS_InitReachabilityAreas
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_InitRouting(void)
{
	AAS_InitTravelFlagFromType();
	//
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
	//get the areas reachabilities go through
	AAS_InitReachabilityAreas();
	//
#ifdef ROUTING_DEBUG
	numareacacheupdates = 0;
	numportalcacheupdates = 0;
#endif	//ROUTING_DEBUG
		//
	routingcachesize = 0;
	max_routingcachesize = 1024 * (int)LibVarValue("max_routingcache", "4096");
	// read any routing cache if available
	AAS_ReadRouteCache();
}	//end of the function AAS_InitRouting
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_AreaRouteToGoalArea(int areanum, vec3_t origin, int goalareanum, int travelflags, int* traveltime, int* reachnum)
{
	int clusternum, goalclusternum, portalnum, i, clusterareanum, bestreachnum;
	unsigned short int t, besttime;
	aas_portal_t* portal;
	aas_cluster_t* cluster;
	aas_routingcache_t* areacache, * portalcache;
	aas_reachability_t* reach;

	if (!aasworld->initialized)
	{
		return false;
	}

	if (areanum == goalareanum)
	{
		*traveltime = 1;
		*reachnum = 0;
		return true;
	}
	//
	if (areanum <= 0 || areanum >= aasworld->numareas)
	{
		if (bot_developer)
		{
			BotImport_Print(PRT_ERROR, "AAS_AreaTravelTimeToGoalArea: areanum %d out of range\n", areanum);
		}	//end if
		return false;
	}	//end if
	if (goalareanum <= 0 || goalareanum >= aasworld->numareas)
	{
		if (bot_developer)
		{
			BotImport_Print(PRT_ERROR, "AAS_AreaTravelTimeToGoalArea: goalareanum %d out of range\n", goalareanum);
		}	//end if
		return false;
	}	//end if
		// make sure the routing cache doesn't grow to large
	while (routingcachesize > max_routingcachesize)
	{
		if (!AAS_FreeOldestCache())
		{
			break;
		}
	}
	//
	if (AAS_AreaDoNotEnter(areanum) || AAS_AreaDoNotEnter(goalareanum))
	{
		travelflags |= TFL_DONOTENTER;
	}	//end if
		//NOTE: the number of routing updates is limited per frame
		/*
		if (aasworld->frameroutingupdates > MAX_FRAMEROUTINGUPDATES_Q3)
		{
		#ifdef DEBUG
		    //Log_Write("WARNING: AAS_AreaTravelTimeToGoalArea: frame routing updates overflowed");
		#endif
		    return 0;
		} //end if
		*/
		//
	clusternum = aasworld->areasettings[areanum].cluster;
	goalclusternum = aasworld->areasettings[goalareanum].cluster;
	//check if the area is a portal of the goal area cluster
	if (clusternum < 0 && goalclusternum > 0)
	{
		portal = &aasworld->portals[-clusternum];
		if (portal->frontcluster == goalclusternum ||
			portal->backcluster == goalclusternum)
		{
			clusternum = goalclusternum;
		}	//end if
	}	//end if
		//check if the goalarea is a portal of the area cluster
	else if (clusternum > 0 && goalclusternum < 0)
	{
		portal = &aasworld->portals[-goalclusternum];
		if (portal->frontcluster == clusternum ||
			portal->backcluster == clusternum)
		{
			goalclusternum = clusternum;
		}	//end if
	}	//end if
		//if both areas are in the same cluster
		//NOTE: there might be a shorter route via another cluster!!! but we don't care
	if (clusternum > 0 && goalclusternum > 0 && clusternum == goalclusternum)
	{
		//
		areacache = AAS_GetAreaRoutingCache(clusternum, goalareanum, travelflags, true);
		//the number of the area in the cluster
		clusterareanum = AAS_ClusterAreaNum(clusternum, areanum);
		//the cluster the area is in
		cluster = &aasworld->clusters[clusternum];
		//if the area is NOT a reachability area
		if (clusterareanum >= cluster->numreachabilityareas)
		{
			return 0;
		}
		//if it is possible to travel to the goal area through this cluster
		if (areacache->traveltimes[clusterareanum] != 0)
		{
			*reachnum = aasworld->areasettings[areanum].firstreachablearea +
						areacache->reachabilities[clusterareanum];
			if (!origin)
			{
				*traveltime = areacache->traveltimes[clusterareanum];
				return true;
			}
			reach = &aasworld->reachability[*reachnum];
			*traveltime = areacache->traveltimes[clusterareanum] +
						  AAS_AreaTravelTime(areanum, origin, reach->start);
			//
			return true;
		}	//end if
	}	//end if
		//
	clusternum = aasworld->areasettings[areanum].cluster;
	goalclusternum = aasworld->areasettings[goalareanum].cluster;
	//if the goal area is a portal
	if (goalclusternum < 0)
	{
		//just assume the goal area is part of the front cluster
		portal = &aasworld->portals[-goalclusternum];
		goalclusternum = portal->frontcluster;
	}	//end if
		//get the portal routing cache
	portalcache = AAS_GetPortalRoutingCache(goalclusternum, goalareanum, travelflags);
	//if the area is a cluster portal, read directly from the portal cache
	if (clusternum < 0)
	{
		*traveltime = portalcache->traveltimes[-clusternum];
		*reachnum = aasworld->areasettings[areanum].firstreachablearea +
					portalcache->reachabilities[-clusternum];
		return true;
	}	//end if
		//
	besttime = 0;
	bestreachnum = -1;
	//the cluster the area is in
	cluster = &aasworld->clusters[clusternum];
	//find the portal of the area cluster leading towards the goal area
	for (i = 0; i < cluster->numportals; i++)
	{
		portalnum = aasworld->portalindex[cluster->firstportal + i];
		//if the goal area isn't reachable from the portal
		if (!portalcache->traveltimes[portalnum])
		{
			continue;
		}
		//
		portal = &aasworld->portals[portalnum];
		//get the cache of the portal area
		areacache = AAS_GetAreaRoutingCache(clusternum, portal->areanum, travelflags, true);
		//current area inside the current cluster
		clusterareanum = AAS_ClusterAreaNum(clusternum, areanum);
		//if the area is NOT a reachability area
		if (clusterareanum >= cluster->numreachabilityareas)
		{
			continue;
		}
		//if the portal is NOT reachable from this area
		if (!areacache->traveltimes[clusterareanum])
		{
			continue;
		}
		//total travel time is the travel time the portal area is from
		//the goal area plus the travel time towards the portal area
		t = portalcache->traveltimes[portalnum] + areacache->traveltimes[clusterareanum];
		//FIXME: add the exact travel time through the actual portal area
		//NOTE: for now we just add the largest travel time through the portal area
		//		because we can't directly calculate the exact travel time
		//		to be more specific we don't know which reachability was used to travel
		//		into the portal area
		t += aasworld->portalmaxtraveltimes[portalnum];
		//
		if (origin)
		{
			*reachnum = aasworld->areasettings[areanum].firstreachablearea +
						areacache->reachabilities[clusterareanum];
			reach = aasworld->reachability + *reachnum;
			t += AAS_AreaTravelTime(areanum, origin, reach->start);
		}	//end if
			//if the time is better than the one already found
		if (!besttime || t < besttime)
		{
			bestreachnum = *reachnum;
			besttime = t;
		}	//end if
	}	//end for
	if (bestreachnum < 0)
	{
		return false;
	}
	*reachnum = bestreachnum;
	*traveltime = besttime;
	return true;
}	//end of the function AAS_AreaRouteToGoalArea
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_AreaTravelTimeToGoalArea(int areanum, vec3_t origin, int goalareanum, int travelflags)
{
	int traveltime, reachnum;

	if (AAS_AreaRouteToGoalArea(areanum, origin, goalareanum, travelflags, &traveltime, &reachnum))
	{
		return traveltime;
	}
	return 0;
}	//end of the function AAS_AreaTravelTimeToGoalArea
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_AreaReachabilityToGoalArea(int areanum, vec3_t origin, int goalareanum, int travelflags)
{
	int traveltime, reachnum;

	if (AAS_AreaRouteToGoalArea(areanum, origin, goalareanum, travelflags, &traveltime, &reachnum))
	{
		return reachnum;
	}
	return 0;
}	//end of the function AAS_AreaReachabilityToGoalArea
//===========================================================================
// predict the route and stop on one of the stop events
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_PredictRoute(struct aas_predictroute_s* route, int areanum, vec3_t origin,
	int goalareanum, int travelflags, int maxareas, int maxtime,
	int stopevent, int stopcontents, int stoptfl, int stopareanum)
{
	int curareanum, reachnum, i, j, testareanum;
	vec3_t curorigin;
	aas_reachability_t* reach;
	aas_reachabilityareas_t* reachareas;

	//init output
	route->stopevent = RSE_NONE;
	route->endarea = goalareanum;
	route->endcontents = 0;
	route->endtravelflags = 0;
	VectorCopy(origin, route->endpos);
	route->time = 0;

	curareanum = areanum;
	VectorCopy(origin, curorigin);

	for (i = 0; curareanum != goalareanum && (!maxareas || i < maxareas) && i < aasworld->numareas; i++)
	{
		reachnum = AAS_AreaReachabilityToGoalArea(curareanum, curorigin, goalareanum, travelflags);
		if (!reachnum)
		{
			route->stopevent = RSE_NOROUTE;
			return false;
		}	//end if
		reach = &aasworld->reachability[reachnum];
		//
		if (stopevent & RSE_USETRAVELTYPE)
		{
			if (AAS_TravelFlagForType(reach->traveltype) & stoptfl)
			{
				route->stopevent = RSE_USETRAVELTYPE;
				route->endarea = curareanum;
				route->endcontents = aasworld->areasettings[curareanum].contents;
				route->endtravelflags = AAS_TravelFlagForType(reach->traveltype);
				VectorCopy(reach->start, route->endpos);
				return true;
			}	//end if
			if (AAS_AreaContentsTravelFlags(reach->areanum) & stoptfl)
			{
				route->stopevent = RSE_USETRAVELTYPE;
				route->endarea = reach->areanum;
				route->endcontents = aasworld->areasettings[reach->areanum].contents;
				route->endtravelflags = AAS_AreaContentsTravelFlags(reach->areanum);
				VectorCopy(reach->end, route->endpos);
				route->time += AAS_AreaTravelTime(areanum, origin, reach->start);
				route->time += reach->traveltime;
				return true;
			}	//end if
		}	//end if
		reachareas = &aasworld->reachabilityareas[reachnum];
		for (j = 0; j < reachareas->numareas + 1; j++)
		{
			if (j >= reachareas->numareas)
			{
				testareanum = reach->areanum;
			}
			else
			{
				testareanum = aasworld->reachabilityareaindex[reachareas->firstarea + j];
			}
			if (stopevent & RSE_ENTERCONTENTS)
			{
				if (aasworld->areasettings[testareanum].contents & stopcontents)
				{
					route->stopevent = RSE_ENTERCONTENTS;
					route->endarea = testareanum;
					route->endcontents = aasworld->areasettings[testareanum].contents;
					VectorCopy(reach->end, route->endpos);
					route->time += AAS_AreaTravelTime(areanum, origin, reach->start);
					route->time += reach->traveltime;
					return true;
				}	//end if
			}	//end if
			if (stopevent & RSE_ENTERAREA)
			{
				if (testareanum == stopareanum)
				{
					route->stopevent = RSE_ENTERAREA;
					route->endarea = testareanum;
					route->endcontents = aasworld->areasettings[testareanum].contents;
					VectorCopy(reach->start, route->endpos);
					return true;
				}	//end if
			}	//end if
		}	//end for

		route->time += AAS_AreaTravelTime(areanum, origin, reach->start);
		route->time += reach->traveltime;
		route->endarea = reach->areanum;
		route->endcontents = aasworld->areasettings[reach->areanum].contents;
		route->endtravelflags = AAS_TravelFlagForType(reach->traveltype);
		VectorCopy(reach->end, route->endpos);
		//
		curareanum = reach->areanum;
		VectorCopy(reach->end, curorigin);
		//
		if (maxtime && route->time > maxtime)
		{
			break;
		}
	}	//end while
	if (curareanum != goalareanum)
	{
		return false;
	}
	return true;
}	//end of the function AAS_PredictRoute
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
		return false;
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
					return true;
				}	//end if
				VectorCopy(aasworld->areas[n].center, start);
				if (!AAS_PointAreaNum(start))
				{
					Log_Write("area %d center %f %f %f in solid?", n, start[0], start[1], start[2]);
				}
				VectorCopy(start, end);
				end[2] -= 300;
				trace = AAS_TraceClientBBox(start, end, PRESENCE_CROUCH, -1);
				if (!trace.startsolid && trace.fraction < 1 && AAS_PointAreaNum(trace.endpos) == n)
				{
					if (AAS_AreaGroundFaceArea(n) > 300)
					{
						*goalareanum = n;
						VectorCopy(trace.endpos, goalorigin);
						//BotImport_Print(PRT_MESSAGE, "found random goal area %d\n", *goalareanum);
						return true;
					}	//end if
				}	//end if
			}	//end if
		}	//end if
		n++;
	}	//end for
	return false;
}	//end of the function AAS_RandomGoalArea
