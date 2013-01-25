//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../../common/qcommon.h"
#include "../../common/common_defs.h"
#include "local.h"

struct midrangearea_t {
	bool valid;
	unsigned short starttime;
	unsigned short goaltime;
};

static midrangearea_t* midrangeareas;
static int* clusterareas;
static int numclusterareas;

static void AAS_AltRoutingFloodCluster_r( int areanum ) {
	//add the current area to the areas of the current cluster
	clusterareas[ numclusterareas ] = areanum;
	numclusterareas++;
	//remove the area from the mid range areas
	midrangeareas[ areanum ].valid = false;
	//flood to other areas through the faces of this area
	aas_area_t* area = &aasworld->areas[ areanum ];
	for ( int i = 0; i < area->numfaces; i++ ) {
		aas_face_t* face = &aasworld->faces[ abs( aasworld->faceindex[ area->firstface + i ] ) ];
		//get the area at the other side of the face
		int otherareanum;
		if ( face->frontarea == areanum ) {
			otherareanum = face->backarea;
		} else   {
			otherareanum = face->frontarea;
		}
		//if there is an area at the other side of this face
		if ( !otherareanum ) {
			continue;
		}
		//if the other area is not a midrange area
		if ( !midrangeareas[ otherareanum ].valid ) {
			continue;
		}

		AAS_AltRoutingFloodCluster_r( otherareanum );
	}
}

int AAS_AlternativeRouteGoalsQ3( const vec3_t start, int startareanum,
	const vec3_t goal, int goalareanum, int travelflags,
	aas_altroutegoal_t* altroutegoals, int maxaltroutegoals,
	int type ) {
	if ( !startareanum || !goalareanum ) {
		return 0;
	}
	//travel time towards the goal area
	int goaltraveltime = AAS_AreaTravelTimeToGoalArea( startareanum, start, goalareanum, travelflags );
	//clear the midrange areas
	Com_Memset( midrangeareas, 0, aasworld->numareas * sizeof ( midrangearea_t ) );
	int numaltroutegoals = 0;

	int nummidrangeareas = 0;

	for ( int i = 1; i < aasworld->numareas; i++ ) {
		if ( !( type & ALTROUTEGOAL_ALL ) ) {
			if ( !( type & ALTROUTEGOAL_CLUSTERPORTALS && ( aasworld->areasettings[ i ].contents & AREACONTENTS_CLUSTERPORTAL ) ) ) {
				if ( !( type & ALTROUTEGOAL_VIEWPORTALS && ( aasworld->areasettings[ i ].contents & AREACONTENTS_VIEWPORTAL ) ) ) {
					continue;
				}
			}
		}
		//if the area has no reachabilities
		if ( !AAS_AreaReachability( i ) ) {
			continue;
		}
		//tavel time from the area to the start area
		int starttime = AAS_AreaTravelTimeToGoalArea( startareanum, start, i, travelflags );
		if ( !starttime ) {
			continue;
		}
		//if the travel time from the start to the area is greater than the shortest goal travel time
		if ( starttime > ( float )1.1 * goaltraveltime ) {
			continue;
		}
		//travel time from the area to the goal area
		int goaltime = AAS_AreaTravelTimeToGoalArea( i, NULL, goalareanum, travelflags );
		if ( !goaltime ) {
			continue;
		}
		//if the travel time from the area to the goal is greater than the shortest goal travel time
		if ( goaltime > ( float )0.8 * goaltraveltime ) {
			continue;
		}
		//this is a mid range area
		midrangeareas[ i ].valid = true;
		midrangeareas[ i ].starttime = starttime;
		midrangeareas[ i ].goaltime = goaltime;
		Log_Write( "%d midrange area %d", nummidrangeareas, i );
		nummidrangeareas++;
	}

	for ( int i = 1; i < aasworld->numareas; i++ ) {
		if ( !midrangeareas[ i ].valid ) {
			continue;
		}
		//get the areas in one cluster
		numclusterareas = 0;
		AAS_AltRoutingFloodCluster_r( i );
		//now we've got a cluster with areas through which an alternative route could go
		//get the 'center' of the cluster
		vec3_t mid;
		VectorClear( mid );
		for ( int j = 0; j < numclusterareas; j++ ) {
			VectorAdd( mid, aasworld->areas[ clusterareas[ j ] ].center, mid );
		}
		VectorScale( mid, 1.0 / numclusterareas, mid );
		//get the area closest to the center of the cluster
		float bestdist = 999999;
		int bestareanum = 0;
		for ( int j = 0; j < numclusterareas; j++ ) {
			vec3_t dir;
			VectorSubtract( mid, aasworld->areas[ clusterareas[ j ] ].center, dir );
			float dist = VectorLength( dir );
			if ( dist < bestdist ) {
				bestdist = dist;
				bestareanum = clusterareas[ j ];
			}
		}
		//now we've got an area for an alternative route
		//FIXME: add alternative goal origin
		VectorCopy( aasworld->areas[ bestareanum ].center, altroutegoals[ numaltroutegoals ].origin );
		altroutegoals[ numaltroutegoals ].areanum = bestareanum;
		altroutegoals[ numaltroutegoals ].starttraveltime = midrangeareas[ bestareanum ].starttime;
		altroutegoals[ numaltroutegoals ].goaltraveltime = midrangeareas[ bestareanum ].goaltime;
		altroutegoals[ numaltroutegoals ].extratraveltime =
			( midrangeareas[ bestareanum ].starttime + midrangeareas[ bestareanum ].goaltime ) -
			goaltraveltime;
		numaltroutegoals++;

		//don't return more than the maximum alternative route goals
		if ( numaltroutegoals >= maxaltroutegoals ) {
			break;
		}
	}
	return numaltroutegoals;
}

int AAS_AlternativeRouteGoalsET( const vec3_t start, const vec3_t goal, int travelflags,
	aas_altroutegoal_t* altroutegoals, int maxaltroutegoals,
	int color ) {
	int startareanum = AAS_PointAreaNum( start );
	if ( !startareanum ) {
		return 0;
	}
	int goalareanum = AAS_PointAreaNum( goal );
	if ( !goalareanum ) {
		vec3_t dir;
		VectorCopy( goal, dir );
		dir[ 2 ] += 30;
		goalareanum = AAS_PointAreaNum( dir );
		if ( !goalareanum ) {
			return 0;
		}
	}
	//travel time towards the goal area
	int goaltraveltime = AAS_AreaTravelTimeToGoalArea( startareanum, start, goalareanum, travelflags );
	//clear the midrange areas
	Com_Memset( midrangeareas, 0, aasworld->numareas * sizeof ( midrangearea_t ) );
	int numaltroutegoals = 0;

	int nummidrangeareas = 0;

	for ( int i = 1; i < aasworld->numareas; i++ ) {
		//
		if ( !( aasworld->areasettings[ i ].contents & AREACONTENTS_ROUTEPORTAL ) &&
			 !( aasworld->areasettings[ i ].contents & AREACONTENTS_CLUSTERPORTAL ) ) {
			continue;
		}
		//if the area has no reachabilities
		if ( !AAS_AreaReachability( i ) ) {
			continue;
		}
		//tavel time from the area to the start area
		int starttime = AAS_AreaTravelTimeToGoalArea( startareanum, start, i, travelflags );
		if ( !starttime ) {
			continue;
		}
		//if the travel time from the start to the area is greater than the shortest goal travel time
		if ( starttime > 500 + 3.0 * goaltraveltime ) {
			continue;
		}
		//travel time from the area to the goal area
		int goaltime = AAS_AreaTravelTimeToGoalArea( i, NULL, goalareanum, travelflags );
		if ( !goaltime ) {
			continue;
		}
		//if the travel time from the area to the goal is greater than the shortest goal travel time
		if ( goaltime > 500 + 3.0 * goaltraveltime ) {
			continue;
		}
		//this is a mid range area
		midrangeareas[ i ].valid = true;
		midrangeareas[ i ].starttime = starttime;
		midrangeareas[ i ].goaltime = goaltime;
		Log_Write( "%d midrange area %d", nummidrangeareas, i );
		nummidrangeareas++;
	}

	for ( int i = 1; i < aasworld->numareas; i++ ) {
		if ( !midrangeareas[ i ].valid ) {
			continue;
		}
		//get the areas in one cluster
		numclusterareas = 0;
		AAS_AltRoutingFloodCluster_r( i );
		//now we've got a cluster with areas through which an alternative route could go
		//get the 'center' of the cluster
		vec3_t mid;
		VectorClear( mid );
		for ( int j = 0; j < numclusterareas; j++ ) {
			VectorAdd( mid, aasworld->areas[ clusterareas[ j ] ].center, mid );
		}
		VectorScale( mid, 1.0 / numclusterareas, mid );
		//get the area closest to the center of the cluster
		float bestdist = 999999;
		int bestareanum = 0;
		for ( int j = 0; j < numclusterareas; j++ ) {
			vec3_t dir;
			VectorSubtract( mid, aasworld->areas[ clusterareas[ j ] ].center, dir );
			float dist = VectorLength( dir );
			if ( dist < bestdist ) {
				bestdist = dist;
				bestareanum = clusterareas[ j ];
			}
		}
		// make sure the route to the destination isn't in the same direction as the route to the source
		int reachnum, time;
		if ( !AAS_AreaRouteToGoalArea( bestareanum, aasworld->areawaypoints[ bestareanum ], goalareanum, travelflags, &time, &reachnum ) ) {
			continue;
		}
		int a1 = aasworld->reachability[ reachnum ].areanum;
		if ( !AAS_AreaRouteToGoalArea( bestareanum, aasworld->areawaypoints[ bestareanum ], startareanum, travelflags, &time, &reachnum ) ) {
			continue;
		}
		int a2 = aasworld->reachability[ reachnum ].areanum;
		if ( a1 == a2 ) {
			continue;
		}
		//now we've got an area for an alternative route
		//FIXME: add alternative goal origin
		VectorCopy( aasworld->areawaypoints[ bestareanum ], altroutegoals[ numaltroutegoals ].origin );
		altroutegoals[ numaltroutegoals ].areanum = bestareanum;
		altroutegoals[ numaltroutegoals ].starttraveltime = midrangeareas[ bestareanum ].starttime;
		altroutegoals[ numaltroutegoals ].goaltraveltime = midrangeareas[ bestareanum ].goaltime;
		altroutegoals[ numaltroutegoals ].extratraveltime =
			( midrangeareas[ bestareanum ].starttime + midrangeareas[ bestareanum ].goaltime ) -
			goaltraveltime;
		numaltroutegoals++;

		//don't return more than the maximum alternative route goals
		if ( numaltroutegoals >= maxaltroutegoals ) {
			break;
		}
	}
	return numaltroutegoals;
}

void AAS_InitAlternativeRouting() {
	if ( GGameType & ( GAME_WolfSP & GAME_WolfMP ) ) {
		return;
	}
	if ( midrangeareas ) {
		Mem_Free( midrangeareas );
	}
	midrangeareas = ( midrangearea_t* )Mem_Alloc( aasworld->numareas * sizeof ( midrangearea_t ) );
	if ( clusterareas ) {
		Mem_Free( clusterareas );
	}
	clusterareas = ( int* )Mem_Alloc( aasworld->numareas * sizeof ( int ) );
}

void AAS_ShutdownAlternativeRouting() {
	if ( GGameType & ( GAME_WolfSP & GAME_WolfMP ) ) {
		return;
	}
	if ( midrangeareas ) {
		Mem_Free( midrangeareas );
	}
	midrangeareas = NULL;
	if ( clusterareas ) {
		Mem_Free( clusterareas );
	}
	clusterareas = NULL;
	numclusterareas = 0;
}
