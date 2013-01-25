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

#include "../../common/Common.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"
#include "../../common/content_types.h"
#include "../../common/crc.h"
#include "../../common/endian.h"
#include "local.h"
#include "../tech3/local.h"

/*

  area routing cache:
  stores the distances within one cluster to a specific goal area
  this goal area is in this same cluster and could be a cluster portal
  for every cluster there's a list with routing cache for every area
  in that cluster (including the portals of that cluster)
  area cache stores aasworld->clusters[?].numreachabilityareas travel times

  portal routing cache:
  stores the distances of all portals to a specific goal area
  this goal area could be in any cluster and could also be a cluster portal
  for every area (aasworld->numareas) the portal cache stores
  aasworld->numportals travel times

*/

#define ROUTING_DEBUG

//travel time in hundreths of a second = distance * 100 / speed
#define DISTANCEFACTOR_CROUCH       1.3f	//crouch speed = 100
#define DISTANCEFACTOR_SWIM         1		//should be 0.66, swim speed = 150
#define DISTANCEFACTOR_WALK         0.33f	//walk speed = 300

// Ridah, scale traveltimes with ground steepness of area
#define GROUNDSTEEPNESS_TIMESCALE       20	// this is the maximum scale, 1 being the usual for a flat ground

//maximum number of routing updates each frame
#define MAX_FRAMEROUTINGUPDATES_WOLF    100

#define RCID                        ( ( 'C' << 24 ) + ( 'R' << 16 ) + ( 'E' << 8 ) + 'M' )
#define RCVERSION_Q3                2
#define RCVERSION_WS                15
#define RCVERSION_WM                12
#define RCVERSION_ET                16

//the route cache header
//this header is followed by numportalcache + numareacache aas_routingcache_t
//structures that store routing cache
struct routecacheheader_q3_t {
	int ident;
	int version;
	int numareas;
	int numclusters;
	int areacrc;
	int clustercrc;
	int numportalcache;
	int numareacache;
};

//the route cache header
//this header is followed by numportalcache + numareacache aas_routingcache_t
//structures that store routing cache
struct routecacheheader_wolf_t {
	int ident;
	int version;
	int numareas;
	int numclusters;
	int areacrc;
	int clustercrc;
	int reachcrc;
	int numportalcache;
	int numareacache;
};

struct aas_routingcache_f_t {
	int size;									//size of the routing cache
	float time;									//last time accessed or updated
	int cluster;								//cluster the cache is for
	int areanum;								//area the cache is created for
	vec3_t origin;								//origin within the area
	float starttraveltime;						//travel time to start with
	int travelflags;							//combinations of the travel flags
	int prev, next;
	int reachabilities;				//reachabilities used for routing
	unsigned short int traveltimes[ 1 ];			//travel time for every area (variable sized)
};

#ifdef ROUTING_DEBUG
static int numareacacheupdates;
static int numportalcacheupdates;
#endif

static int routingcachesize;
static int max_routingcachesize;
static int max_frameroutingupdates;

void AAS_RoutingInfo() {
#ifdef ROUTING_DEBUG
	BotImport_Print( PRT_MESSAGE, "%d area cache updates\n", numareacacheupdates );
	BotImport_Print( PRT_MESSAGE, "%d portal cache updates\n", numportalcacheupdates );
	BotImport_Print( PRT_MESSAGE, "%d bytes routing cache\n", routingcachesize );
#endif
}

// returns the number of the area in the cluster
// assumes the given area is in the given cluster or a portal of the cluster
static inline int AAS_ClusterAreaNum( int cluster, int areanum ) {
	int areacluster = aasworld->areasettings[ areanum ].cluster;
	if ( areacluster > 0 ) {
		return aasworld->areasettings[ areanum ].clusterareanum;
	} else   {
		int side = aasworld->portals[ -areacluster ].frontcluster != cluster;
		return aasworld->portals[ -areacluster ].clusterareanum[ side ];
	}
}

static void AAS_InitTravelFlagFromType() {
	for ( int i = 0; i < MAX_TRAVELTYPES; i++ ) {
		aasworld->travelflagfortype[ i ] = TFL_INVALID;
	}
	aasworld->travelflagfortype[ TRAVEL_INVALID ] = TFL_INVALID;
	aasworld->travelflagfortype[ TRAVEL_WALK ] = TFL_WALK;
	aasworld->travelflagfortype[ TRAVEL_CROUCH ] = TFL_CROUCH;
	aasworld->travelflagfortype[ TRAVEL_BARRIERJUMP ] = TFL_BARRIERJUMP;
	aasworld->travelflagfortype[ TRAVEL_JUMP ] = TFL_JUMP;
	aasworld->travelflagfortype[ TRAVEL_LADDER ] = TFL_LADDER;
	aasworld->travelflagfortype[ TRAVEL_WALKOFFLEDGE ] = TFL_WALKOFFLEDGE;
	aasworld->travelflagfortype[ TRAVEL_SWIM ] = TFL_SWIM;
	aasworld->travelflagfortype[ TRAVEL_WATERJUMP ] = TFL_WATERJUMP;
	aasworld->travelflagfortype[ TRAVEL_TELEPORT ] = TFL_TELEPORT;
	aasworld->travelflagfortype[ TRAVEL_ELEVATOR ] = TFL_ELEVATOR;
	aasworld->travelflagfortype[ TRAVEL_ROCKETJUMP ] = TFL_ROCKETJUMP;
	aasworld->travelflagfortype[ TRAVEL_BFGJUMP ] = TFL_BFGJUMP;
	aasworld->travelflagfortype[ TRAVEL_GRAPPLEHOOK ] = TFL_GRAPPLEHOOK;
	aasworld->travelflagfortype[ TRAVEL_DOUBLEJUMP ] = TFL_DOUBLEJUMP;
	aasworld->travelflagfortype[ TRAVEL_RAMPJUMP ] = TFL_RAMPJUMP;
	aasworld->travelflagfortype[ TRAVEL_STRAFEJUMP ] = TFL_STRAFEJUMP;
	aasworld->travelflagfortype[ TRAVEL_JUMPPAD ] = TFL_JUMPPAD;
	aasworld->travelflagfortype[ TRAVEL_FUNCBOB ] = TFL_FUNCBOB;
}

int AAS_TravelFlagForType( int traveltype ) {
	traveltype &= TRAVELTYPE_MASK;
	if ( traveltype < 0 || traveltype >= MAX_TRAVELTYPES ) {
		return TFL_INVALID;
	}
	return aasworld->travelflagfortype[ traveltype ];
}

static inline float AAS_RoutingTime() {
	return AAS_Time();
}

static void AAS_LinkCache( aas_routingcache_t* cache ) {
	if ( aasworld->newestcache ) {
		aasworld->newestcache->time_next = cache;
		cache->time_prev = aasworld->newestcache;
	} else   {
		aasworld->oldestcache = cache;
		cache->time_prev = NULL;
	}
	cache->time_next = NULL;
	aasworld->newestcache = cache;
}

static void AAS_UnlinkCache( aas_routingcache_t* cache ) {
	if ( cache->time_next ) {
		cache->time_next->time_prev = cache->time_prev;
	} else   {
		aasworld->newestcache = cache->time_prev;
	}
	if ( cache->time_prev ) {
		cache->time_prev->time_next = cache->time_next;
	} else   {
		aasworld->oldestcache = cache->time_next;
	}
	cache->time_next = NULL;
	cache->time_prev = NULL;
}

static aas_routingcache_t* AAS_AllocRoutingCache( int numtraveltimes ) {
	int size = sizeof ( aas_routingcache_t ) +
			   numtraveltimes * sizeof ( unsigned short int ) +
			   numtraveltimes * sizeof ( unsigned char );

	routingcachesize += size;

	aas_routingcache_t* cache = ( aas_routingcache_t* )Mem_ClearedAlloc( size );
	cache->reachabilities = ( unsigned char* )cache + sizeof ( aas_routingcache_t ) +
							numtraveltimes * sizeof ( unsigned short int );
	cache->size = size;
	return cache;
}

static void AAS_FreeRoutingCache( aas_routingcache_t* cache ) {
	AAS_UnlinkCache( cache );
	routingcachesize -= cache->size;
	Mem_Free( cache );
}

static void AAS_RemoveRoutingCacheInCluster( int clusternum ) {
	int i;
	aas_routingcache_t* cache, * nextcache;
	aas_cluster_t* cluster;

	if ( !aasworld->clusterareacache ) {
		return;
	}
	cluster = &aasworld->clusters[ clusternum ];
	for ( i = 0; i < cluster->numareas; i++ ) {
		for ( cache = aasworld->clusterareacache[ clusternum ][ i ]; cache; cache = nextcache ) {
			nextcache = cache->next;
			AAS_FreeRoutingCache( cache );
		}
		aasworld->clusterareacache[ clusternum ][ i ] = NULL;
	}
}

static void AAS_RemoveRoutingCacheUsingArea( int areanum ) {
	int clusternum = aasworld->areasettings[ areanum ].cluster;
	if ( clusternum > 0 ) {
		//remove all the cache in the cluster the area is in
		AAS_RemoveRoutingCacheInCluster( clusternum );
	} else   {
		// if this is a portal remove all cache in both the front and back cluster
		AAS_RemoveRoutingCacheInCluster( aasworld->portals[ -clusternum ].frontcluster );
		AAS_RemoveRoutingCacheInCluster( aasworld->portals[ -clusternum ].backcluster );
	}
	// remove all portal cache
	for ( int i = 0; i < aasworld->numareas; i++ ) {
		//refresh portal cache
		aas_routingcache_t* nextcache;
		for ( aas_routingcache_t* cache = aasworld->portalcache[ i ]; cache; cache = nextcache ) {
			nextcache = cache->next;
			AAS_FreeRoutingCache( cache );
		}
		aasworld->portalcache[ i ] = NULL;
	}
}

static void AAS_ClearClusterTeamFlags( int areanum ) {
	int clusternum = aasworld->areasettings[ areanum ].cluster;
	if ( clusternum > 0 ) {
		aasworld->clusterTeamTravelFlags[ clusternum ] = -1;	// recalculate
	}
}

int AAS_EnableRoutingArea( int areanum, int enable ) {
	if ( areanum <= 0 || areanum >= aasworld->numareas ) {
		if ( bot_developer ) {
			BotImport_Print( PRT_ERROR, "AAS_EnableRoutingArea: areanum %d out of range\n", areanum );
		}
		return 0;
	}

	int bitflag;	// flag to set or clear
	if ( GGameType & GAME_ET ) {
		if ( ( enable & 1 ) || ( enable < 0 ) ) {
			// clear all flags
			bitflag = ETAREA_AVOID | AREA_DISABLED | ETAREA_TEAM_AXIS | ETAREA_TEAM_ALLIES | ETAREA_TEAM_AXIS_DISGUISED | ETAREA_TEAM_ALLIES_DISGUISED;
		} else if ( enable & 0x10 )     {
			bitflag = ETAREA_AVOID;
		} else if ( enable & 0x20 )     {
			bitflag = ETAREA_TEAM_AXIS;
		} else if ( enable & 0x40 )     {
			bitflag = ETAREA_TEAM_ALLIES;
		} else if ( enable & 0x80 )     {
			bitflag = ETAREA_TEAM_AXIS_DISGUISED;
		} else if ( enable & 0x100 )     {
			bitflag = ETAREA_TEAM_ALLIES_DISGUISED;
		} else   {
			bitflag = AREA_DISABLED;
		}

		// remove avoidance flag
		enable &= 1;
	} else   {
		bitflag = AREA_DISABLED;
	}

	int flags = aasworld->areasettings[ areanum ].areaflags & bitflag;
	if ( enable < 0 ) {
		return !flags;
	}

	if ( enable ) {
		aasworld->areasettings[ areanum ].areaflags &= ~bitflag;
	} else   {
		aasworld->areasettings[ areanum ].areaflags |= bitflag;
	}

	// if the status of the area changed
	if ( ( flags & bitflag ) != ( aasworld->areasettings[ areanum ].areaflags & bitflag ) ) {
		//remove all routing cache involving this area
		AAS_RemoveRoutingCacheUsingArea( areanum );
		if ( GGameType & GAME_ET ) {
			// recalculate the team flags that are used in this cluster
			AAS_ClearClusterTeamFlags( areanum );
		}
	}
	return !flags;
}

void AAS_EnableAllAreas() {
	for ( int i = 0; i < aasworld->numareas; i++ ) {
		if ( aasworld->areasettings[ i ].areaflags & AREA_DISABLED ) {
			AAS_EnableRoutingArea( i, true );
		}
	}
}

static int AAS_GetAreaContentsTravelFlags( int areanum ) {
	int contents = aasworld->areasettings[ areanum ].contents;
	int tfl = 0;
	if ( contents & AREACONTENTS_WATER ) {
		tfl |= TFL_WATER;
	} else if ( contents & AREACONTENTS_SLIME )     {
		tfl |= TFL_SLIME;
	} else if ( contents & AREACONTENTS_LAVA )     {
		tfl |= TFL_LAVA;
	} else   {
		tfl |= TFL_AIR;
	}
	if ( contents & AREACONTENTS_DONOTENTER ) {
		tfl |= TFL_DONOTENTER;
	}
	if ( GGameType & GAME_Quake3 ) {
		if ( contents & Q3AREACONTENTS_NOTTEAM1 ) {
			tfl |= Q3TFL_NOTTEAM1;
		}
		if ( contents & Q3AREACONTENTS_NOTTEAM2 ) {
			tfl |= Q3TFL_NOTTEAM2;
		}
		if ( aasworld->areasettings[ areanum ].areaflags & Q3AREA_BRIDGE ) {
			tfl |= Q3TFL_BRIDGE;
		}
	} else   {
		if ( contents & WOLFAREACONTENTS_DONOTENTER_LARGE ) {
			tfl |= WOLFTFL_DONOTENTER_LARGE;
		}
	}
	return tfl;
}

static void AAS_InitAreaContentsTravelFlags() {
	if ( aasworld->areacontentstravelflags ) {
		Mem_Free( aasworld->areacontentstravelflags );
	}
	aasworld->areacontentstravelflags = ( int* )Mem_ClearedAlloc( aasworld->numareas * sizeof ( int ) );

	for ( int i = 0; i < aasworld->numareas; i++ ) {
		aasworld->areacontentstravelflags[ i ] = AAS_GetAreaContentsTravelFlags( i );
	}
}

int AAS_AreaContentsTravelFlags( int areanum ) {
	return aasworld->areacontentstravelflags[ areanum ];
}

static void AAS_CreateReversedReachability() {
#ifdef DEBUG
	int starttime = Sys_Milliseconds();
#endif
	//free reversed links that have already been created
	if ( aasworld->reversedreachability ) {
		Mem_Free( aasworld->reversedreachability );
	}
	//allocate memory for the reversed reachability links
	char* ptr = ( char* )Mem_ClearedAlloc( aasworld->numareas * sizeof ( aas_reversedreachability_t ) +
		aasworld->reachabilitysize * sizeof ( aas_reversedlink_t ) );
	//
	aasworld->reversedreachability = ( aas_reversedreachability_t* )ptr;
	//pointer to the memory for the reversed links
	ptr += aasworld->numareas * sizeof ( aas_reversedreachability_t );
	//check all reachabilities of all areas
	for ( int i = 1; i < aasworld->numareas; i++ ) {
		//settings of the area
		aas_areasettings_t* settings = &aasworld->areasettings[ i ];

		if ( GGameType & GAME_Quake3 && settings->numreachableareas >= 128 ) {
			BotImport_Print( PRT_WARNING, "area %d has more than 128 reachabilities\n", i );
		}
		//create reversed links for the reachabilities
		for ( int n = 0; n < settings->numreachableareas && ( !( GGameType & GAME_Quake3 ) || n < 128 ); n++ ) {
			// Gordon: Temp hack for b0rked last area in
			if ( GGameType & GAME_ET && ( settings->firstreachablearea < 0 || settings->firstreachablearea >= aasworld->reachabilitysize ) ) {
				BotImport_Print( PRT_WARNING, "settings->firstreachablearea out of range\n" );
				continue;
			}

			//reachability link
			aas_reachability_t* reach = &aasworld->reachability[ settings->firstreachablearea + n ];

			if ( GGameType & GAME_ET && ( reach->areanum < 0 || reach->areanum >= aasworld->reachabilitysize ) ) {
				BotImport_Print( PRT_WARNING, "reach->areanum out of range\n" );
				continue;
			}

			aas_reversedlink_t* revlink = ( aas_reversedlink_t* )ptr;
			ptr += sizeof ( aas_reversedlink_t );

			revlink->areanum = i;
			revlink->linknum = settings->firstreachablearea + n;
			revlink->next = aasworld->reversedreachability[ reach->areanum ].first;
			aasworld->reversedreachability[ reach->areanum ].first = revlink;
			aasworld->reversedreachability[ reach->areanum ].numlinks++;
		}
	}
#ifdef DEBUG
	BotImport_Print( PRT_MESSAGE, "reversed reachability %d msec\n", Sys_Milliseconds() - starttime );
#endif
}

static float AAS_AreaGroundSteepnessScale( int areanum ) {
	return ( 1.0 + aasworld->areasettings[ areanum ].groundsteepness * ( float )( GROUNDSTEEPNESS_TIMESCALE - 1 ) );
}

unsigned short int AAS_AreaTravelTime( int areanum, const vec3_t start, const vec3_t end ) {
	float dist = VectorDistance( start, end );

	if ( GGameType & ( GAME_WolfSP | GAME_WolfMP ) ) {
		// Ridah, factor in the groundsteepness now
		dist *= AAS_AreaGroundSteepnessScale( areanum );
	}

	//if crouch only area
	if ( AAS_AreaCrouch( areanum ) ) {
		dist *= DISTANCEFACTOR_CROUCH;
	}
	//if swim area
	else if ( AAS_AreaSwim( areanum ) ) {
		dist *= DISTANCEFACTOR_SWIM;
	}
	//normal walk area
	else {
		dist *= DISTANCEFACTOR_WALK;
	}

	int intdist = GGameType & GAME_WolfSP ? ( int )ceil( dist ) : ( int )dist;
	//make sure the distance isn't zero
	if ( intdist <= 0 ) {
		intdist = 1;
	}
	return intdist;
}

static void AAS_CalculateAreaTravelTimes() {
#ifdef DEBUG
	int starttime = Sys_Milliseconds();
#endif
	//if there are still area travel times, free the memory
	if ( aasworld->areatraveltimes ) {
		Mem_Free( aasworld->areatraveltimes );
	}
	//get the total size of all the area travel times
	int size = aasworld->numareas * sizeof ( unsigned short** );
	for ( int i = 0; i < aasworld->numareas; i++ ) {
		aas_reversedreachability_t* revreach = &aasworld->reversedreachability[ i ];
		//settings of the area
		aas_areasettings_t* settings = &aasworld->areasettings[ i ];

		size += settings->numreachableareas * sizeof ( unsigned short* );

		size += settings->numreachableareas * revreach->numlinks * sizeof ( unsigned short );
	}
	//allocate memory for the area travel times
	char* ptr = ( char* )Mem_ClearedAlloc( size );
	aasworld->areatraveltimes = ( unsigned short*** )ptr;
	ptr += aasworld->numareas * sizeof ( unsigned short** );
	//calcluate the travel times for all the areas
	for ( int i = 0; i < aasworld->numareas; i++ ) {
		//reversed reachabilities of this area
		aas_reversedreachability_t* revreach = &aasworld->reversedreachability[ i ];
		//settings of the area
		aas_areasettings_t* settings = &aasworld->areasettings[ i ];
		//
		if ( !( GGameType & GAME_ET ) || settings->numreachableareas ) {
			aasworld->areatraveltimes[ i ] = ( unsigned short** )ptr;
			ptr += settings->numreachableareas * sizeof ( unsigned short* );

			aas_reachability_t* reach = &aasworld->reachability[ settings->firstreachablearea ];
			for ( int l = 0; l < settings->numreachableareas; l++, reach++ ) {
				aasworld->areatraveltimes[ i ][ l ] = ( unsigned short* )ptr;
				ptr += revreach->numlinks * sizeof ( unsigned short );
				//reachability link
				aas_reversedlink_t* revlink = revreach->first;
				for ( int n = 0; revlink; revlink = revlink->next, n++ ) {
					vec3_t end;
					VectorCopy( aasworld->reachability[ revlink->linknum ].end, end );

					aasworld->areatraveltimes[ i ][ l ][ n ] = AAS_AreaTravelTime( i, end, reach->start );
				}
			}
		}
	}
#ifdef DEBUG
	BotImport_Print( PRT_MESSAGE, "area travel times %d msec\n", Sys_Milliseconds() - starttime );
#endif
}

static int AAS_PortalMaxTravelTime( int portalnum ) {
	aas_portal_t* portal = &aasworld->portals[ portalnum ];
	//reversed reachabilities of this portal area
	aas_reversedreachability_t* revreach = &aasworld->reversedreachability[ portal->areanum ];
	//settings of the portal area
	aas_areasettings_t* settings = &aasworld->areasettings[ portal->areanum ];

	int maxt = 0;
	for ( int l = 0; l < settings->numreachableareas; l++ ) {
		aas_reversedlink_t* revlink = revreach->first;
		for ( int n = 0; revlink; revlink = revlink->next, n++ ) {
			int t = aasworld->areatraveltimes[ portal->areanum ][ l ][ n ];
			if ( t > maxt ) {
				maxt = t;
			}
		}
	}
	return maxt;
}

static void AAS_InitPortalMaxTravelTimes() {
	if ( aasworld->portalmaxtraveltimes ) {
		Mem_Free( aasworld->portalmaxtraveltimes );
	}

	aasworld->portalmaxtraveltimes = ( int* )Mem_ClearedAlloc( aasworld->numportals * sizeof ( int ) );

	for ( int i = 0; i < aasworld->numportals; i++ ) {
		aasworld->portalmaxtraveltimes[ i ] = AAS_PortalMaxTravelTime( i );
	}
}

static bool AAS_FreeOldestCache() {
	aas_routingcache_t* cache;
	for ( cache = aasworld->oldestcache; cache; cache = cache->time_next ) {
		// never free area cache leading towards a portal
		if ( cache->type == CACHETYPE_AREA && aasworld->areasettings[ cache->areanum ].cluster < 0 ) {
			continue;
		}
		break;
	}
	if ( cache ) {
		// unlink the cache
		if ( cache->type == CACHETYPE_AREA ) {
			//number of the area in the cluster
			int clusterareanum = AAS_ClusterAreaNum( cache->cluster, cache->areanum );
			// unlink from cluster area cache
			if ( cache->prev ) {
				cache->prev->next = cache->next;
			} else   {
				aasworld->clusterareacache[ cache->cluster ][ clusterareanum ] = cache->next;
			}
			if ( cache->next ) {
				cache->next->prev = cache->prev;
			}
		} else   {
			// unlink from portal cache
			if ( cache->prev ) {
				cache->prev->next = cache->next;
			} else   {
				aasworld->portalcache[ cache->areanum ] = cache->next;
			}
			if ( cache->next ) {
				cache->next->prev = cache->prev;
			}
		}
		AAS_FreeRoutingCache( cache );
		return true;
	}
	return false;
}

static void AAS_FreeAllClusterAreaCache() {
	//free all cluster cache if existing
	if ( !aasworld->clusterareacache ) {
		return;
	}
	//free caches
	for ( int i = 0; i < aasworld->numclusters; i++ ) {
		aas_cluster_t* cluster = &aasworld->clusters[ i ];
		for ( int j = 0; j < cluster->numareas; j++ ) {
			aas_routingcache_t* nextcache;
			for ( aas_routingcache_t* cache = aasworld->clusterareacache[ i ][ j ]; cache; cache = nextcache ) {
				nextcache = cache->next;
				AAS_FreeRoutingCache( cache );
			}
			aasworld->clusterareacache[ i ][ j ] = NULL;
		}
	}
	//free the cluster cache array
	Mem_Free( aasworld->clusterareacache );
	aasworld->clusterareacache = NULL;
}

static void AAS_InitClusterAreaCache() {
	int size = 0;
	for ( int i = 0; i < aasworld->numclusters; i++ ) {
		size += aasworld->clusters[ i ].numareas;
	}
	//two dimensional array with pointers for every cluster to routing cache
	//for every area in that cluster
	char* ptr = ( char* )Mem_ClearedAlloc(
		aasworld->numclusters * sizeof ( aas_routingcache_t * * ) +
		size * sizeof ( aas_routingcache_t* ) );
	aasworld->clusterareacache = ( aas_routingcache_t*** )ptr;
	ptr += aasworld->numclusters * sizeof ( aas_routingcache_t * * );
	for ( int i = 0; i < aasworld->numclusters; i++ ) {
		aasworld->clusterareacache[ i ] = ( aas_routingcache_t** )ptr;
		ptr += aasworld->clusters[ i ].numareas * sizeof ( aas_routingcache_t* );
	}
}

static void AAS_FreeAllPortalCache() {
	//free all portal cache if existing
	if ( !aasworld->portalcache ) {
		return;
	}
	//free portal caches
	for ( int i = 0; i < aasworld->numareas; i++ ) {
		aas_routingcache_t* nextcache;
		for ( aas_routingcache_t* cache = aasworld->portalcache[ i ]; cache; cache = nextcache ) {
			nextcache = cache->next;
			AAS_FreeRoutingCache( cache );
		}
		aasworld->portalcache[ i ] = NULL;
	}
	Mem_Free( aasworld->portalcache );
	aasworld->portalcache = NULL;
}

static void AAS_InitPortalCache() {
	aasworld->portalcache = ( aas_routingcache_t** )Mem_ClearedAlloc(
		aasworld->numareas * sizeof ( aas_routingcache_t* ) );
}

// run-length compression on zeros
static int AAS_CompressVis( const byte* vis, int numareas, byte* dest ) {
	byte* dest_p = dest;

	for ( int j = 0; j < numareas; j++ ) {
		*dest_p++ = vis[ j ];
		byte check = vis[ j ];

		int rep = 1;
		for ( j++; j < numareas; j++ ) {
			if ( vis[ j ] != check || rep == 255 ) {
				break;
			} else   {
				rep++;
			}
		}
		*dest_p++ = rep;
		j--;
	}
	return dest_p - dest;
}

static void AAS_DecompressVis( const byte* in, int numareas, byte* decompressed ) {
	// initialize the vis data, only set those that are visible
	Com_Memset( decompressed, 0, numareas );

	byte* out = decompressed;
	byte* end = decompressed + numareas;

	do {
		byte c = in[ 1 ];
		if ( !c ) {
			AAS_Error( "DecompressVis: 0 repeat" );
		}
		if ( *in ) {	// we need to set these bits
			Com_Memset( out, 1, c );
		}
		in += 2;
		out += c;
	} while ( out < end );
}

// just center to center visibility checking...
// FIXME: implement a correct full vis
static void AAS_CreateVisibility( bool waypointsOnly ) {
	int numAreas = aasworld->numareas;
	int numAreaBits = ( ( numAreas + 8 ) >> 3 );

	byte* validareas = NULL;
	if ( !waypointsOnly ) {
		validareas = ( byte* )Mem_ClearedAlloc( numAreas * sizeof ( byte ) );
	}

	aasworld->areawaypoints = ( vec3_t* )Mem_ClearedAlloc( numAreas * sizeof ( vec3_t ) );
	int totalsize = numAreas * sizeof ( byte* );

	vec3_t endpos, mins, maxs;
	int numvalid = 0;
	for ( int i = 1; i < numAreas; i++ ) {
		if ( !AAS_AreaReachability( i ) ) {
			continue;
		}

		// find the waypoint
		VectorCopy( aasworld->areas[ i ].center, endpos );
		endpos[ 2 ] -= 256;
		AAS_PresenceTypeBoundingBox( PRESENCE_NORMAL, mins, maxs );
		bsp_trace_t trace = AAS_Trace( aasworld->areas[ i ].center, mins, maxs, endpos, -1, BSP46CONTENTS_SOLID |
			( GGameType & GAME_WolfMP ? 0 : BSP46CONTENTS_PLAYERCLIP | BSP46CONTENTS_MONSTERCLIP ) );
		if ( GGameType & GAME_ET && trace.startsolid && trace.ent < Q3ENTITYNUM_WORLD ) {
			trace = AAS_Trace( aasworld->areas[ i ].center, mins, maxs, endpos, trace.ent,
				BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP | BSP46CONTENTS_MONSTERCLIP );
		}
		if ( !trace.startsolid && trace.fraction < 1 && AAS_PointAreaNum( trace.endpos ) == i ) {
			VectorCopy( trace.endpos, aasworld->areawaypoints[ i ] );
			if ( !waypointsOnly ) {
				validareas[ i ] = 1;
			}
			numvalid++;
		} else   {
			VectorClear( aasworld->areawaypoints[ i ] );
		}
	}

	if ( waypointsOnly ) {
		return;
	}

	aasworld->areavisibility = ( byte** )Mem_ClearedAlloc( numAreas * sizeof ( byte* ) );
	aasworld->decompressedvis = ( byte* )Mem_ClearedAlloc( numAreas * sizeof ( byte ) );

	byte* areaTable = ( byte* )Mem_ClearedAlloc( numAreas * numAreaBits * sizeof ( byte ) );
	// in case it ends up bigger than the decompressedvis, which is rare but possible
	byte* buf = ( byte* )Mem_ClearedAlloc( numAreas * 2 * sizeof ( byte ) );

	for ( int i = 1; i < numAreas; i++ ) {
		if ( !validareas[ i ] ) {
			continue;
		}

		if ( GGameType & GAME_WolfMP && !AAS_AreaReachability( i ) ) {
			continue;
		}

		for ( int j = 1; j < numAreas; j++ ) {
			aasworld->decompressedvis[ j ] = 0;
			if ( i == j ) {
				aasworld->decompressedvis[ j ] = 1;
				areaTable[ ( i * numAreaBits ) + ( j >> 3 ) ] |= ( 1 << ( j & 7 ) );
				continue;
			}

			if ( !validareas[ j ] || ( GGameType & GAME_WolfMP && !AAS_AreaReachability( j ) ) ) {
				if ( GGameType & GAME_WolfMP ) {
					aasworld->decompressedvis[ j ] = 0;
				}
				continue;
			}

			// if we have already checked this combination, copy the result
			if ( !( GGameType & GAME_WolfMP ) && areaTable && ( i > j ) ) {
				// use the reverse result stored in the table
				if ( areaTable[ ( j * numAreaBits ) + ( i >> 3 ) ] & ( 1 << ( i & 7 ) ) ) {
					aasworld->decompressedvis[ j ] = 1;
				}
				// done, move to the next area
				continue;
			}

			// RF, check PVS first, since it's much faster
			if ( !( GGameType & GAME_WolfMP ) && !AAS_inPVS( aasworld->areawaypoints[ i ], aasworld->areawaypoints[ j ] ) ) {
				continue;
			}

			bsp_trace_t trace = AAS_Trace( aasworld->areawaypoints[ i ], NULL, NULL, aasworld->areawaypoints[ j ],
				-1, BSP46CONTENTS_SOLID );
			if ( GGameType & GAME_ET && trace.startsolid && trace.ent < Q3ENTITYNUM_WORLD ) {
				trace = AAS_Trace( aasworld->areas[ i ].center, mins, maxs, endpos, trace.ent,
					BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP | BSP46CONTENTS_MONSTERCLIP );
			}

			if ( trace.fraction >= 1 ) {
				if ( GGameType & GAME_ET ) {
					areaTable[ ( i * numAreaBits ) + ( j >> 3 ) ] |= ( 1 << ( j & 7 ) );
				}
				aasworld->decompressedvis[ j ] = 1;
			} else if ( GGameType & GAME_WolfMP )     {
				aasworld->decompressedvis[ j ] = 0;
			}
		}

		int size = AAS_CompressVis( aasworld->decompressedvis, numAreas, buf );
		aasworld->areavisibility[ i ] = ( byte* )Mem_Alloc( size );
		Com_Memcpy( aasworld->areavisibility[ i ], buf, size );
		totalsize += size;
	}

	Mem_Free( validareas );
	Mem_Free( buf );
	Mem_Free( areaTable );
	BotImport_Print( PRT_MESSAGE, "AAS_CreateVisibility: compressed vis size = %i\n", totalsize );
}

static void AAS_FreeAreaVisibility() {
	if ( aasworld->areavisibility ) {
		for ( int i = 0; i < aasworld->numareas; i++ ) {
			if ( aasworld->areavisibility[ i ] ) {
				Mem_Free( aasworld->areavisibility[ i ] );
			}
		}
	}
	if ( aasworld->areavisibility ) {
		Mem_Free( aasworld->areavisibility );
	}
	aasworld->areavisibility = NULL;
	if ( aasworld->decompressedvis ) {
		Mem_Free( aasworld->decompressedvis );
	}
	aasworld->decompressedvis = NULL;
}

int AAS_AreaVisible( int srcarea, int destarea ) {
	if ( GGameType & GAME_Quake3 ) {
		return false;
	}
	if ( GGameType & GAME_ET && !aasworld->areavisibility ) {
		return true;
	}
	if ( srcarea != aasworld->decompressedvisarea ) {
		if ( !aasworld->areavisibility[ srcarea ] ) {
			return false;
		}
		AAS_DecompressVis( aasworld->areavisibility[ srcarea ],
			aasworld->numareas, aasworld->decompressedvis );
		aasworld->decompressedvisarea = srcarea;
	}
	return aasworld->decompressedvis[ destarea ];
}

static void AAS_InitRoutingUpdate() {
	//free routing update fields if already existing
	if ( aasworld->areaupdate ) {
		Mem_Free( aasworld->areaupdate );
	}

	if ( GGameType & GAME_Quake3 ) {
		int maxreachabilityareas = 0;
		for ( int i = 0; i < aasworld->numclusters; i++ ) {
			if ( aasworld->clusters[ i ].numreachabilityareas > maxreachabilityareas ) {
				maxreachabilityareas = aasworld->clusters[ i ].numreachabilityareas;
			}
		}
		//allocate memory for the routing update fields
		aasworld->areaupdate = ( aas_routingupdate_t* )Mem_ClearedAlloc(
			maxreachabilityareas * sizeof ( aas_routingupdate_t ) );
	} else   {
		// Ridah, had to change it to numareas for hidepos checking
		aasworld->areaupdate = ( aas_routingupdate_t* )Mem_ClearedAlloc(
			aasworld->numareas * sizeof ( aas_routingupdate_t ) );
	}

	if ( aasworld->portalupdate ) {
		Mem_Free( aasworld->portalupdate );
	}
	//allocate memory for the portal update fields
	aasworld->portalupdate = ( aas_routingupdate_t* )Mem_ClearedAlloc(
		( aasworld->numportals + 1 ) * sizeof ( aas_routingupdate_t ) );
}

void AAS_WriteRouteCache() {
	int numportalcache = 0;
	for ( int i = 0; i < aasworld->numareas; i++ ) {
		for ( aas_routingcache_t* cache = aasworld->portalcache[ i ]; cache; cache = cache->next ) {
			numportalcache++;
		}
	}
	int numareacache = 0;
	for ( int i = 0; i < aasworld->numclusters; i++ ) {
		aas_cluster_t* cluster = &aasworld->clusters[ i ];
		for ( int j = 0; j < cluster->numareas; j++ ) {
			for ( aas_routingcache_t* cache = aasworld->clusterareacache[ i ][ j ]; cache; cache = cache->next ) {
				numareacache++;
			}
		}
	}
	// open the file for writing
	char filename[ MAX_QPATH ];
	String::Sprintf( filename, MAX_QPATH, "maps/%s.rcd", aasworld->mapname );
	fileHandle_t fp;
	FS_FOpenFileByMode( filename, &fp, FS_WRITE );
	if ( !fp ) {
		AAS_Error( "Unable to open file: %s\n", filename );
		return;
	}

	//create the header
	if ( GGameType & GAME_Quake3 ) {
		routecacheheader_q3_t routecacheheader;
		routecacheheader.ident = RCID;
		routecacheheader.version = RCVERSION_Q3;
		routecacheheader.numareas = aasworld->numareas;
		routecacheheader.numclusters = aasworld->numclusters;
		routecacheheader.areacrc = CRC_Block( ( unsigned char* )aasworld->areas, sizeof ( aas_area_t ) * aasworld->numareas );
		routecacheheader.clustercrc = CRC_Block( ( unsigned char* )aasworld->clusters, sizeof ( aas_cluster_t ) * aasworld->numclusters );
		routecacheheader.numportalcache = numportalcache;
		routecacheheader.numareacache = numareacache;
		//write the header
		FS_Write( &routecacheheader, sizeof ( routecacheheader_q3_t ), fp );
	} else   {
		routecacheheader_wolf_t routecacheheader;
		routecacheheader.ident = RCID;
		if ( GGameType & GAME_WolfSP ) {
			routecacheheader.version = RCVERSION_WS;
		} else if ( GGameType & GAME_WolfMP )     {
			routecacheheader.version = RCVERSION_WM;
		} else   {
			routecacheheader.version = RCVERSION_ET;
		}
		routecacheheader.numareas = aasworld->numareas;
		routecacheheader.numclusters = aasworld->numclusters;
		routecacheheader.areacrc = CRC_Block( ( unsigned char* )aasworld->areas, sizeof ( aas_area_t ) * aasworld->numareas );
		routecacheheader.clustercrc = CRC_Block( ( unsigned char* )aasworld->clusters, sizeof ( aas_cluster_t ) * aasworld->numclusters );
		routecacheheader.reachcrc = CRC_Block( ( unsigned char* )aasworld->reachability, sizeof ( aas_reachability_t ) * aasworld->reachabilitysize );
		routecacheheader.numportalcache = numportalcache;
		routecacheheader.numareacache = numareacache;
		//write the header
		FS_Write( &routecacheheader, sizeof ( routecacheheader_wolf_t ), fp );
	}

	//write all the cache
	for ( int i = 0; i < aasworld->numareas; i++ ) {
		for ( aas_routingcache_t* cache = aasworld->portalcache[ i ]; cache; cache = cache->next ) {
			FS_Write( cache, cache->size, fp );
		}
	}
	for ( int i = 0; i < aasworld->numclusters; i++ ) {
		aas_cluster_t* cluster = &aasworld->clusters[ i ];
		for ( int j = 0; j < cluster->numareas; j++ ) {
			for ( aas_routingcache_t* cache = aasworld->clusterareacache[ i ][ j ]; cache; cache = cache->next ) {
				FS_Write( cache, cache->size, fp );
			}
		}
	}

	if ( !( GGameType & GAME_Quake3 ) ) {
		// in case it ends up bigger than the decompressedvis, which is rare but possible
		byte* buf = ( byte* )Mem_ClearedAlloc( aasworld->numareas * 2 * sizeof ( byte ) );
		// write the visareas
		for ( int i = 0; i < aasworld->numareas; i++ ) {
			if ( !aasworld->areavisibility[ i ] ) {
				int size = 0;
				FS_Write( &size, sizeof ( int ), fp );
				continue;
			}
			AAS_DecompressVis( aasworld->areavisibility[ i ], aasworld->numareas, aasworld->decompressedvis );
			int size = AAS_CompressVis( aasworld->decompressedvis, aasworld->numareas, buf );
			FS_Write( &size, sizeof ( int ), fp );
			FS_Write( buf, size, fp );
		}
		Mem_Free( buf );
		// write the waypoints
		FS_Write( aasworld->areawaypoints, sizeof ( vec3_t ) * aasworld->numareas, fp );
	}

	FS_FCloseFile( fp );
	BotImport_Print( PRT_MESSAGE, "\nroute cache written to %s\n", filename );
}

static aas_routingcache_t* AAS_ReadCache( fileHandle_t fp ) {
	if ( GGameType & GAME_WolfSP ) {
		int i, size, realSize, numtraveltimes;
		aas_routingcache_t* cache;

		FS_Read( &size, sizeof ( size ), fp );
		size = LittleLong( size );
		numtraveltimes = ( size - sizeof ( aas_routingcache_f_t ) ) / 3;
		realSize = size - sizeof ( aas_routingcache_f_t ) + sizeof ( aas_routingcache_t );
		cache = ( aas_routingcache_t* )Mem_ClearedAlloc( realSize );
		cache->size = realSize;
		aas_routingcache_f_t* fcache = ( aas_routingcache_f_t* )Mem_ClearedAlloc( size );
		FS_Read( ( unsigned char* )fcache + sizeof ( size ), size - sizeof ( size ), fp );

		//	Can't really use sizeof(aas_routingcache_f_t) - sizeof(short) because
		// size of aas_routingcache_f_t is 4 byte aligned.
		Com_Memcpy( cache->traveltimes, fcache->traveltimes, size - 48 );

		cache->time = LittleFloat( fcache->time );
		cache->cluster = LittleLong( fcache->cluster );
		cache->areanum = LittleLong( fcache->areanum );
		cache->origin[ 0 ] = LittleFloat( fcache->origin[ 0 ] );
		cache->origin[ 1 ] = LittleFloat( fcache->origin[ 1 ] );
		cache->origin[ 2 ] = LittleFloat( fcache->origin[ 2 ] );
		cache->starttraveltime = LittleFloat( fcache->starttraveltime );
		cache->travelflags = LittleLong( fcache->travelflags );
		Mem_Free( fcache );

		//	The way pointer was assigned created 4 wasted bytes after traveltimes.
		// Can't use sizeof(aas_routingcache_t) because on 64 bit it's
		// 8 byte aligned.
		cache->reachabilities = ( unsigned char* )cache->traveltimes + 4 + numtraveltimes * 2;

		//DAJ BUGFIX for missing byteswaps for traveltimes
		size = ( size - sizeof ( aas_routingcache_f_t ) ) / 3 + 1;
		for ( i = 0; i < size; i++ ) {
			cache->traveltimes[ i ] = LittleShort( cache->traveltimes[ i ] );
		}
		AAS_LinkCache( cache );
		return cache;
	} else if ( GGameType & GAME_WolfMP )     {
		int size, i;
		aas_routingcache_t* cache;

		FS_Read( &size, sizeof ( size ), fp );
		cache = ( aas_routingcache_t* )Mem_ClearedAlloc( size );
		cache->size = size;
		FS_Read( ( unsigned char* )cache + sizeof ( size ), size - sizeof ( size ), fp );
		//	cache->reachabilities = (unsigned char *) cache + sizeof(aas_routingcache_t) - sizeof(unsigned short) +
		//		(size - sizeof(aas_routingcache_t) + sizeof(unsigned short)) / 3 * 2;
		cache->reachabilities = ( unsigned char* )cache + sizeof ( aas_routingcache_t ) +
								( ( size - sizeof ( aas_routingcache_t ) ) / 3 ) * 2;

		//DAJ BUGFIX for missing byteswaps for traveltimes
		size = ( size - sizeof ( aas_routingcache_t ) ) / 3 + 1;
		for ( i = 0; i < size; i++ ) {
			cache->traveltimes[ i ] = LittleShort( cache->traveltimes[ i ] );
		}
		return cache;
	} else if ( GGameType & GAME_ET )     {
		int size, i;
		aas_routingcache_t* cache;

		FS_Read( &size, sizeof ( size ), fp );
		size = LittleLong( size );
		cache = ( aas_routingcache_t* )Mem_ClearedAlloc( size );
		cache->size = size;
		FS_Read( ( unsigned char* )cache + sizeof ( size ), size - sizeof ( size ), fp );

		if ( 1 != LittleLong( 1 ) ) {
			cache->time = LittleFloat( cache->time );
			cache->cluster = LittleLong( cache->cluster );
			cache->areanum = LittleLong( cache->areanum );
			cache->origin[ 0 ] = LittleFloat( cache->origin[ 0 ] );
			cache->origin[ 1 ] = LittleFloat( cache->origin[ 1 ] );
			cache->origin[ 2 ] = LittleFloat( cache->origin[ 2 ] );
			cache->starttraveltime = LittleFloat( cache->starttraveltime );
			cache->travelflags = LittleLong( cache->travelflags );
		}

		//	cache->reachabilities = (unsigned char *) cache + sizeof(aas_routingcache_t) - sizeof(unsigned short) +
		//		(size - sizeof(aas_routingcache_t) + sizeof(unsigned short)) / 3 * 2;
		cache->reachabilities = ( unsigned char* )cache + sizeof ( aas_routingcache_t ) +
								( ( size - sizeof ( aas_routingcache_t ) ) / 3 ) * 2;

		//DAJ BUGFIX for missing byteswaps for traveltimes
		size = ( size - sizeof ( aas_routingcache_t ) ) / 3 + 1;
		for ( i = 0; i < size; i++ ) {
			cache->traveltimes[ i ] = LittleShort( cache->traveltimes[ i ] );
		}
		return cache;
	} else   {
		int size;
		aas_routingcache_t* cache;

		FS_Read( &size, sizeof ( size ), fp );
		cache = ( aas_routingcache_t* )Mem_ClearedAlloc( size );
		cache->size = size;
		FS_Read( ( unsigned char* )cache + sizeof ( size ), size - sizeof ( size ), fp );
		cache->reachabilities = ( unsigned char* )cache + sizeof ( aas_routingcache_t ) - sizeof ( unsigned short ) +
								( size - sizeof ( aas_routingcache_t ) + sizeof ( unsigned short ) ) / 3 * 2;
		return cache;
	}
}

static bool ReadRouteCacheHeaderQ3( const char* filename, fileHandle_t fp, int& numportalcache, int& numareacache ) {
	routecacheheader_q3_t routecacheheader;
	FS_Read( &routecacheheader, sizeof ( routecacheheader_q3_t ), fp );
	if ( routecacheheader.ident != RCID ) {
		AAS_Error( "%s is not a route cache dump\n", filename );
		return false;
	}
	if ( routecacheheader.version != RCVERSION_Q3 ) {
		AAS_Error( "route cache dump has wrong version %d, should be %d", routecacheheader.version, RCVERSION_Q3 );
		return false;
	}
	if ( routecacheheader.numareas != aasworld->numareas ) {
		return false;
	}
	if ( routecacheheader.numclusters != aasworld->numclusters ) {
		return false;
	}
	if ( routecacheheader.areacrc !=
		 CRC_Block( ( unsigned char* )aasworld->areas, sizeof ( aas_area_t ) * aasworld->numareas ) ) {
		return false;
	}
	if ( routecacheheader.clustercrc !=
		 CRC_Block( ( unsigned char* )aasworld->clusters, sizeof ( aas_cluster_t ) * aasworld->numclusters ) ) {
		return false;
	}
	numportalcache = routecacheheader.numportalcache;
	numareacache = routecacheheader.numareacache;
	return true;
}

static bool ReadRouteCacheHeaderWolf( const char* filename, fileHandle_t fp, int& numportalcache, int& numareacache ) {
	routecacheheader_wolf_t routecacheheader;
	FS_Read( &routecacheheader, sizeof ( routecacheheader_wolf_t ), fp );

	if ( GGameType & GAME_WolfSP ) {
		// GJD: route cache data MUST be written on a PC because I've not altered the writing code.

		routecacheheader.areacrc = LittleLong( routecacheheader.areacrc );
		routecacheheader.clustercrc = LittleLong( routecacheheader.clustercrc );
		routecacheheader.ident = LittleLong( routecacheheader.ident );
		routecacheheader.numareacache = LittleLong( routecacheheader.numareacache );
		routecacheheader.numareas = LittleLong( routecacheheader.numareas );
		routecacheheader.numclusters = LittleLong( routecacheheader.numclusters );
		routecacheheader.numportalcache = LittleLong( routecacheheader.numportalcache );
		routecacheheader.reachcrc = LittleLong( routecacheheader.reachcrc );
		routecacheheader.version = LittleLong( routecacheheader.version );
	}

	if ( routecacheheader.ident != RCID ) {
		common->Printf( "%s is not a route cache dump\n", filename );			// not an aas_error because we want to continue
		return false;												// and remake them by returning false here
	}
	if ( GGameType & GAME_WolfSP && routecacheheader.version != RCVERSION_WS ) {
		common->Printf( "route cache dump has wrong version %d, should be %d", routecacheheader.version, RCVERSION_WS );
		return false;
	}
	if ( GGameType & GAME_WolfMP && routecacheheader.version != RCVERSION_WM ) {
		common->Printf( "route cache dump has wrong version %d, should be %d", routecacheheader.version, RCVERSION_WM );
		return false;
	}
	if ( GGameType & GAME_ET && routecacheheader.version != RCVERSION_ET ) {
		common->Printf( "route cache dump has wrong version %d, should be %d", routecacheheader.version, RCVERSION_ET );
		return false;
	}
	if ( routecacheheader.numareas != aasworld->numareas ) {
		return false;
	}
	if ( routecacheheader.numclusters != aasworld->numclusters ) {
		return false;
	}
	// the crc table stuff is endian orientated....
	if ( LittleLong( 1 ) == 1 ) {
		if ( routecacheheader.areacrc !=
			 CRC_Block( ( unsigned char* )aasworld->areas, sizeof ( aas_area_t ) * aasworld->numareas ) ) {
			return false;
		}
		if ( routecacheheader.clustercrc !=
			 CRC_Block( ( unsigned char* )aasworld->clusters, sizeof ( aas_cluster_t ) * aasworld->numclusters ) ) {
			return false;
		}
		if ( routecacheheader.reachcrc !=
			 CRC_Block( ( unsigned char* )aasworld->reachability, sizeof ( aas_reachability_t ) * aasworld->reachabilitysize ) ) {
			return false;
		}
	}
	numportalcache = routecacheheader.numportalcache;
	numareacache = routecacheheader.numareacache;
	return true;
}

static bool AAS_ReadRouteCache() {
	int i, clusterareanum, size;
	fileHandle_t fp;
	char filename[ MAX_QPATH ];
	aas_routingcache_t* cache;

	String::Sprintf( filename, MAX_QPATH, "maps/%s.rcd", aasworld->mapname );
	FS_FOpenFileByMode( filename, &fp, FS_READ );
	if ( !fp ) {
		return false;
	}	//end if
	int numportalcache;
	int numareacache;
	if ( GGameType & GAME_Quake3 ) {
		if ( !ReadRouteCacheHeaderQ3( filename, fp, numportalcache, numareacache ) ) {
			FS_FCloseFile( fp );
			return false;
		}
	} else   {
		if ( !ReadRouteCacheHeaderWolf( filename, fp, numportalcache, numareacache ) ) {
			FS_FCloseFile( fp );
			return false;
		}
	}
	//read all the portal cache
	for ( i = 0; i < numportalcache; i++ ) {
		cache = AAS_ReadCache( fp );
		cache->next = aasworld->portalcache[ cache->areanum ];
		cache->prev = NULL;
		if ( aasworld->portalcache[ cache->areanum ] ) {
			aasworld->portalcache[ cache->areanum ]->prev = cache;
		}
		aasworld->portalcache[ cache->areanum ] = cache;
	}
	//read all the cluster area cache
	for ( i = 0; i < numareacache; i++ ) {
		cache = AAS_ReadCache( fp );
		clusterareanum = AAS_ClusterAreaNum( cache->cluster, cache->areanum );
		cache->next = aasworld->clusterareacache[ cache->cluster ][ clusterareanum ];
		cache->prev = NULL;
		if ( aasworld->clusterareacache[ cache->cluster ][ clusterareanum ] ) {
			aasworld->clusterareacache[ cache->cluster ][ clusterareanum ]->prev = cache;
		}
		aasworld->clusterareacache[ cache->cluster ][ clusterareanum ] = cache;
	}

	if ( !( GGameType & GAME_Quake3 ) ) {
		// read the visareas
		aasworld->areavisibility = ( byte** )Mem_ClearedAlloc( aasworld->numareas * sizeof ( byte* ) );
		aasworld->decompressedvis = ( byte* )Mem_ClearedAlloc( aasworld->numareas * sizeof ( byte ) );
		for ( i = 0; i < aasworld->numareas; i++ ) {
			FS_Read( &size, sizeof ( size ), fp );
			if ( GGameType & GAME_WolfSP ) {
				size = LittleLong( size );
			}
			if ( size ) {
				aasworld->areavisibility[ i ] = ( byte* )Mem_Alloc( size );
				FS_Read( aasworld->areavisibility[ i ], size, fp );
			}
		}
		// read the area waypoints
		aasworld->areawaypoints = ( vec3_t* )Mem_ClearedAlloc( aasworld->numareas * sizeof ( vec3_t ) );
		FS_Read( aasworld->areawaypoints, aasworld->numareas * sizeof ( vec3_t ), fp );
		if ( GGameType & GAME_WolfSP && 1 != LittleLong( 1 ) ) {
			for ( i = 0; i < aasworld->numareas; i++ ) {
				aasworld->areawaypoints[ i ][ 0 ] = LittleFloat( aasworld->areawaypoints[ i ][ 0 ] );
				aasworld->areawaypoints[ i ][ 1 ] = LittleFloat( aasworld->areawaypoints[ i ][ 1 ] );
				aasworld->areawaypoints[ i ][ 2 ] = LittleFloat( aasworld->areawaypoints[ i ][ 2 ] );
			}
		}
	}

	FS_FCloseFile( fp );
	return true;
}

void AAS_FreeRoutingCaches() {
	// free all the existing cluster area cache
	AAS_FreeAllClusterAreaCache();
	// free all the existing portal cache
	AAS_FreeAllPortalCache();
	// free all the existing area visibility data
	AAS_FreeAreaVisibility();
	// free cached travel times within areas
	if ( aasworld->areatraveltimes ) {
		Mem_Free( aasworld->areatraveltimes );
	}
	aasworld->areatraveltimes = NULL;
	// free cached maximum travel time through cluster portals
	if ( aasworld->portalmaxtraveltimes ) {
		Mem_Free( aasworld->portalmaxtraveltimes );
	}
	aasworld->portalmaxtraveltimes = NULL;
	// free reversed reachability links
	if ( aasworld->reversedreachability ) {
		Mem_Free( aasworld->reversedreachability );
	}
	aasworld->reversedreachability = NULL;
	// free routing algorithm memory
	if ( aasworld->areaupdate ) {
		Mem_Free( aasworld->areaupdate );
	}
	aasworld->areaupdate = NULL;
	if ( aasworld->portalupdate ) {
		Mem_Free( aasworld->portalupdate );
	}
	aasworld->portalupdate = NULL;
	// free lists with areas the reachabilities go through
	if ( aasworld->reachabilityareas ) {
		Mem_Free( aasworld->reachabilityareas );
	}
	aasworld->reachabilityareas = NULL;
	// free the reachability area index
	if ( aasworld->reachabilityareaindex ) {
		Mem_Free( aasworld->reachabilityareaindex );
	}
	aasworld->reachabilityareaindex = NULL;
	// free area contents travel flags look up table
	if ( aasworld->areacontentstravelflags ) {
		Mem_Free( aasworld->areacontentstravelflags );
	}
	aasworld->areacontentstravelflags = NULL;
	// free area waypoints
	if ( aasworld->areawaypoints ) {
		Mem_Free( aasworld->areawaypoints );
	}
	aasworld->areawaypoints = NULL;
}

// update the given routing cache
static void AAS_UpdateAreaRoutingCache( aas_routingcache_t* areacache ) {
#ifdef ROUTING_DEBUG
	numareacacheupdates++;
#endif
	//number of reachability areas within this cluster
	int numreachabilityareas = aasworld->clusters[ areacache->cluster ].numreachabilityareas;

	if ( GGameType & GAME_Quake3 ) {
		aasworld->frameroutingupdates++;
	}

	int badtravelflags = ~areacache->travelflags;

	int clusterareanum = AAS_ClusterAreaNum( areacache->cluster, areacache->areanum );
	if ( clusterareanum >= numreachabilityareas ) {
		return;
	}

	unsigned short int startareatraveltimes[ 128 ];	//NOTE: not more than 128 reachabilities per area allowed
	Com_Memset( startareatraveltimes, 0, sizeof ( startareatraveltimes ) );

	aas_routingupdate_t* curupdate = &aasworld->areaupdate[ clusterareanum ];
	curupdate->areanum = areacache->areanum;
	if ( GGameType & GAME_Quake3 ) {
		curupdate->areatraveltimes = startareatraveltimes;
	} else   {
		curupdate->areatraveltimes = aasworld->areatraveltimes[ areacache->areanum ][ 0 ];
	}
	curupdate->tmptraveltime = areacache->starttraveltime;

	areacache->traveltimes[ clusterareanum ] = areacache->starttraveltime;
	//put the area to start with in the current read list
	curupdate->next = NULL;
	curupdate->prev = NULL;
	aas_routingupdate_t* updateliststart = curupdate;
	aas_routingupdate_t* updatelistend = curupdate;
	//while there are updates in the current list
	while ( updateliststart ) {
		curupdate = updateliststart;
		//
		if ( curupdate->next ) {
			curupdate->next->prev = NULL;
		} else   {
			updatelistend = NULL;
		}
		updateliststart = curupdate->next;

		curupdate->inlist = false;
		//check all reversed reachability links
		aas_reversedreachability_t* revreach = &aasworld->reversedreachability[ curupdate->areanum ];

		aas_reversedlink_t* revlink = revreach->first;
		for ( int i = 0; revlink; revlink = revlink->next, i++ ) {
			int linknum = revlink->linknum;
			aas_reachability_t* reach = &aasworld->reachability[ linknum ];
			//if there is used an undesired travel type
			if ( AAS_TravelFlagForType( reach->traveltype ) & badtravelflags ) {
				continue;
			}
			//if not allowed to enter the next area
			if ( aasworld->areasettings[ reach->areanum ].areaflags & AREA_DISABLED ) {
				continue;
			}
			//if the next area has a not allowed travel flag
			if ( AAS_AreaContentsTravelFlags( reach->areanum ) & badtravelflags ) {
				continue;
			}
			//number of the area the reversed reachability leads to
			int nextareanum = revlink->areanum;
			//get the cluster number of the area
			int cluster = aasworld->areasettings[ nextareanum ].cluster;
			//don't leave the cluster
			if ( cluster > 0 && cluster != areacache->cluster ) {
				continue;
			}
			//get the number of the area in the cluster
			clusterareanum = AAS_ClusterAreaNum( areacache->cluster, nextareanum );
			if ( clusterareanum >= numreachabilityareas ) {
				continue;
			}
			//time already travelled plus the traveltime through
			//the current area plus the travel time from the reachability
			unsigned short int t = curupdate->tmptraveltime +
								   curupdate->areatraveltimes[ i ] +
								   reach->traveltime;
			if ( GGameType & GAME_ET ) {
				//if trying to avoid this area
				if ( aasworld->areasettings[ reach->areanum ].areaflags & ETAREA_AVOID ) {
					t += 1000;
				} else if ( ( aasworld->areasettings[ reach->areanum ].areaflags & ETAREA_AVOID_AXIS ) && ( areacache->travelflags & ETTFL_TEAM_AXIS ) )           {
					t += 200;	// + (curupdate->areatraveltimes[i] + reach->traveltime) * 30;
				} else if ( ( aasworld->areasettings[ reach->areanum ].areaflags & ETAREA_AVOID_ALLIES ) && ( areacache->travelflags & ETTFL_TEAM_ALLIES ) )           {
					t += 200;	// + (curupdate->areatraveltimes[i] + reach->traveltime) * 30;
				}
			}

			if ( !( GGameType & GAME_Quake3 ) ) {
				aasworld->frameroutingupdates++;
			}

			if ( ( !( GGameType & GAME_ET ) || aasworld->areatraveltimes[ nextareanum ] ) &&
				 ( !areacache->traveltimes[ clusterareanum ] ||
				   areacache->traveltimes[ clusterareanum ] > t ) ) {
				areacache->traveltimes[ clusterareanum ] = t;
				areacache->reachabilities[ clusterareanum ] = linknum - aasworld->areasettings[ nextareanum ].firstreachablearea;
				aas_routingupdate_t* nextupdate = &aasworld->areaupdate[ clusterareanum ];
				nextupdate->areanum = nextareanum;
				nextupdate->tmptraveltime = t;
				nextupdate->areatraveltimes = aasworld->areatraveltimes[ nextareanum ][ linknum -
																						aasworld->areasettings[ nextareanum ].firstreachablearea ];
				if ( !nextupdate->inlist ) {
					// we add the update to the end of the list
					// we could also use a B+ tree to have a real sorted list
					// on travel time which makes for faster routing updates
					nextupdate->next = NULL;
					nextupdate->prev = updatelistend;
					if ( updatelistend ) {
						updatelistend->next = nextupdate;
					} else   {
						updateliststart = nextupdate;
					}
					updatelistend = nextupdate;
					nextupdate->inlist = true;
				}
			}
		}
	}
}

static aas_routingcache_t* AAS_GetAreaRoutingCache( int clusternum, int areanum, int travelflags, bool forceUpdate ) {
	int clusterareanum;
	aas_routingcache_t* cache, * clustercache;

	//number of the area in the cluster
	clusterareanum = AAS_ClusterAreaNum( clusternum, areanum );
	//pointer to the cache for the area in the cluster
	clustercache = aasworld->clusterareacache[ clusternum ][ clusterareanum ];
	if ( GGameType & GAME_ET ) {
		// RF, remove team-specific flags which don't exist in this cluster
		travelflags &= ~ETTFL_TEAM_FLAGS | aasworld->clusterTeamTravelFlags[ clusternum ];
	}
	//find the cache without undesired travel flags
	for ( cache = clustercache; cache; cache = cache->next ) {
		//if there aren't used any undesired travel types for the cache
		if ( cache->travelflags == travelflags ) {
			break;
		}
	}
	//if there was no cache
	if ( !cache ) {
		//NOTE: the number of routing updates is limited per frame
		if ( !forceUpdate && ( aasworld->frameroutingupdates > max_frameroutingupdates ) ) {
			return NULL;
		}

		cache = AAS_AllocRoutingCache( aasworld->clusters[ clusternum ].numreachabilityareas );
		cache->cluster = clusternum;
		cache->areanum = areanum;
		VectorCopy( aasworld->areas[ areanum ].center, cache->origin );
		cache->starttraveltime = 1;
		cache->travelflags = travelflags;
		cache->prev = NULL;
		cache->next = clustercache;
		if ( clustercache ) {
			clustercache->prev = cache;
		}
		aasworld->clusterareacache[ clusternum ][ clusterareanum ] = cache;
		AAS_UpdateAreaRoutingCache( cache );
	} else   {
		AAS_UnlinkCache( cache );
	}
	//the cache has been accessed
	cache->time = AAS_RoutingTime();
	cache->type = CACHETYPE_AREA;
	AAS_LinkCache( cache );
	return cache;
}

static void AAS_UpdatePortalRoutingCache( aas_routingcache_t* portalcache ) {
#ifdef ROUTING_DEBUG
	numportalcacheupdates++;
#endif

	aas_routingupdate_t* curupdate = &aasworld->portalupdate[ aasworld->numportals ];
	curupdate->cluster = portalcache->cluster;
	curupdate->areanum = portalcache->areanum;
	curupdate->tmptraveltime = portalcache->starttraveltime;
	if ( !( GGameType & GAME_ET ) ) {
		//if the start area is a cluster portal, store the travel time for that portal
		int clusternum = aasworld->areasettings[ portalcache->areanum ].cluster;
		if ( clusternum < 0 ) {
			portalcache->traveltimes[ -clusternum ] = portalcache->starttraveltime;
		}
	}
	//put the area to start with in the current read list
	curupdate->next = NULL;
	curupdate->prev = NULL;
	aas_routingupdate_t* updateliststart = curupdate;
	aas_routingupdate_t* updatelistend = curupdate;
	//while there are updates in the current list
	while ( updateliststart ) {
		curupdate = updateliststart;
		//remove the current update from the list
		if ( curupdate->next ) {
			curupdate->next->prev = NULL;
		} else   {
			updatelistend = NULL;
		}
		updateliststart = curupdate->next;
		//current update is removed from the list
		curupdate->inlist = false;

		aas_cluster_t* cluster = &aasworld->clusters[ curupdate->cluster ];

		aas_routingcache_t* cache = AAS_GetAreaRoutingCache( curupdate->cluster,
			curupdate->areanum, portalcache->travelflags, true );
		//take all portals of the cluster
		for ( int i = 0; i < cluster->numportals; i++ ) {
			int portalnum = aasworld->portalindex[ cluster->firstportal + i ];
			aas_portal_t* portal = &aasworld->portals[ portalnum ];
			//if this is the portal of the current update continue
			if ( !( GGameType & GAME_ET ) && portal->areanum == curupdate->areanum ) {
				continue;
			}

			int clusterareanum = AAS_ClusterAreaNum( curupdate->cluster, portal->areanum );
			if ( clusterareanum >= cluster->numreachabilityareas ) {
				continue;
			}

			unsigned short int t = cache->traveltimes[ clusterareanum ];
			if ( !t ) {
				continue;
			}
			t += curupdate->tmptraveltime;

			if ( !portalcache->traveltimes[ portalnum ] ||
				 portalcache->traveltimes[ portalnum ] > t ) {
				portalcache->traveltimes[ portalnum ] = t;
				if ( !( GGameType & GAME_Quake3 ) ) {
					portalcache->reachabilities[ portalnum ] = cache->reachabilities[ clusterareanum ];
				}
				aas_routingupdate_t* nextupdate = &aasworld->portalupdate[ portalnum ];
				if ( portal->frontcluster == curupdate->cluster ) {
					nextupdate->cluster = portal->backcluster;
				} else   {
					nextupdate->cluster = portal->frontcluster;
				}
				nextupdate->areanum = portal->areanum;
				//add travel time through the actual portal area for the next update
				nextupdate->tmptraveltime = t + aasworld->portalmaxtraveltimes[ portalnum ];
				if ( !nextupdate->inlist ) {
					// we add the update to the end of the list
					// we could also use a B+ tree to have a real sorted list
					// on travel time which makes for faster routing updates
					nextupdate->next = NULL;
					nextupdate->prev = updatelistend;
					if ( updatelistend ) {
						updatelistend->next = nextupdate;
					} else   {
						updateliststart = nextupdate;
					}
					updatelistend = nextupdate;
					nextupdate->inlist = true;
				}
			}
		}
	}
}

static aas_routingcache_t* AAS_GetPortalRoutingCache( int clusternum, int areanum, int travelflags ) {
	//find the cached portal routing if existing
	aas_routingcache_t* cache;
	for ( cache = aasworld->portalcache[ areanum ]; cache; cache = cache->next ) {
		if ( cache->travelflags == travelflags ) {
			break;
		}
	}
	//if the portal routing isn't cached
	if ( !cache ) {
		cache = AAS_AllocRoutingCache( aasworld->numportals );
		cache->cluster = clusternum;
		cache->areanum = areanum;
		VectorCopy( aasworld->areas[ areanum ].center, cache->origin );
		cache->starttraveltime = 1;
		cache->travelflags = travelflags;
		//add the cache to the cache list
		cache->prev = NULL;
		cache->next = aasworld->portalcache[ areanum ];
		if ( aasworld->portalcache[ areanum ] ) {
			aasworld->portalcache[ areanum ]->prev = cache;
		}
		aasworld->portalcache[ areanum ] = cache;
		//update the cache
		AAS_UpdatePortalRoutingCache( cache );
	} else   {
		AAS_UnlinkCache( cache );
	}
	//the cache has been accessed
	cache->time = AAS_RoutingTime();
	cache->type = CACHETYPE_PORTAL;
	AAS_LinkCache( cache );
	return cache;
}

void AAS_ReachabilityFromNum( int num, aas_reachability_t* reach ) {
	if ( !aasworld->initialized ) {
		Com_Memset( reach, 0, sizeof ( aas_reachability_t ) );
		return;
	}
	if ( num < 0 || num >= aasworld->reachabilitysize ) {
		Com_Memset( reach, 0, sizeof ( aas_reachability_t ) );
		return;
	}
	Com_Memcpy( reach, &aasworld->reachability[ num ], sizeof ( aas_reachability_t ) );;
}

int AAS_NextAreaReachability( int areanum, int reachnum ) {
	if ( !aasworld->initialized ) {
		return 0;
	}

	if ( areanum <= 0 || areanum >= aasworld->numareas ) {
		BotImport_Print( PRT_ERROR, "AAS_NextAreaReachability: areanum %d out of range\n", areanum );
		return 0;
	}

	aas_areasettings_t* settings = &aasworld->areasettings[ areanum ];
	if ( !reachnum ) {
		return settings->firstreachablearea;
	}
	if ( reachnum < settings->firstreachablearea ) {
		BotImport_Print( PRT_FATAL, "AAS_NextAreaReachability: reachnum < settings->firstreachableara" );
		return 0;
	}
	reachnum++;
	if ( reachnum >= settings->firstreachablearea + settings->numreachableareas ) {
		return 0;
	}
	return reachnum;
}

int AAS_NextModelReachability( int num, int modelnum ) {
	if ( num <= 0 ) {
		num = 1;
	} else if ( num >= aasworld->reachabilitysize )     {
		return 0;
	} else   {
		num++;
	}

	for ( int i = num; i < aasworld->reachabilitysize; i++ ) {
		if ( ( aasworld->reachability[ i ].traveltype & TRAVELTYPE_MASK ) == TRAVEL_ELEVATOR ) {
			if ( aasworld->reachability[ i ].facenum == modelnum ) {
				return i;
			}
		} else if ( ( aasworld->reachability[ i ].traveltype & TRAVELTYPE_MASK ) == TRAVEL_FUNCBOB )         {
			if ( ( aasworld->reachability[ i ].facenum & 0x0000FFFF ) == modelnum ) {
				return i;
			}
		}
	}
	return 0;
}

static void AAS_InitReachabilityAreas() {
	enum { MAX_REACHABILITYPASSAREAS = 32 };
	if ( aasworld->reachabilityareas ) {
		Mem_Free( aasworld->reachabilityareas );
	}
	if ( aasworld->reachabilityareaindex ) {
		Mem_Free( aasworld->reachabilityareaindex );
	}

	aasworld->reachabilityareas = ( aas_reachabilityareas_t* )
								  Mem_ClearedAlloc( aasworld->reachabilitysize * sizeof ( aas_reachabilityareas_t ) );
	aasworld->reachabilityareaindex = ( int* )
									  Mem_ClearedAlloc( aasworld->reachabilitysize * MAX_REACHABILITYPASSAREAS * sizeof ( int ) );
	int numreachareas = 0;
	for ( int i = 0; i < aasworld->reachabilitysize; i++ ) {
		aas_reachability_t* reach = &aasworld->reachability[ i ];
		vec3_t start, end;
		int numareas = 0;
		int areas[ MAX_REACHABILITYPASSAREAS ];
		switch ( reach->traveltype & TRAVELTYPE_MASK ) {
		//trace areas from start to end
		case TRAVEL_BARRIERJUMP:
		case TRAVEL_WATERJUMP:
			VectorCopy( reach->start, end );
			end[ 2 ] = reach->end[ 2 ];
			numareas = AAS_TraceAreas( reach->start, end, areas, NULL, MAX_REACHABILITYPASSAREAS );
			break;
		case TRAVEL_WALKOFFLEDGE:
			VectorCopy( reach->end, start );
			start[ 2 ] = reach->start[ 2 ];
			numareas = AAS_TraceAreas( start, reach->end, areas, NULL, MAX_REACHABILITYPASSAREAS );
			break;
		case TRAVEL_GRAPPLEHOOK:
			numareas = AAS_TraceAreas( reach->start, reach->end, areas, NULL, MAX_REACHABILITYPASSAREAS );
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
		}
		aasworld->reachabilityareas[ i ].firstarea = numreachareas;
		aasworld->reachabilityareas[ i ].numareas = numareas;
		for ( int j = 0; j < numareas; j++ ) {
			aasworld->reachabilityareaindex[ numreachareas++ ] = areas[ j ];
		}
	}
}

bool AAS_AreaRouteToGoalArea( int areanum, const vec3_t origin, int goalareanum, int travelflags,
	int* traveltime, int* reachnum ) {
	if ( !aasworld->initialized ) {
		return false;
	}

	if ( areanum == goalareanum ) {
		*traveltime = 1;
		*reachnum = 0;
		return true;
	}

	if ( areanum <= 0 || areanum >= aasworld->numareas ) {
		if ( bot_developer ) {
			BotImport_Print( PRT_ERROR, "AAS_AreaTravelTimeToGoalArea: areanum %d out of range\n", areanum );
		}
		return false;
	}
	if ( goalareanum <= 0 || goalareanum >= aasworld->numareas ) {
		if ( bot_developer ) {
			BotImport_Print( PRT_ERROR, "AAS_AreaTravelTimeToGoalArea: goalareanum %d out of range\n", goalareanum );
		}
		return false;
	}

	// make sure the routing cache doesn't grow to large
	while ( routingcachesize > max_routingcachesize ) {
		if ( !AAS_FreeOldestCache() ) {
			break;
		}
	}
	//
	if ( AAS_AreaDoNotEnter( areanum ) || AAS_AreaDoNotEnter( goalareanum ) ) {
		travelflags |= TFL_DONOTENTER;
	}
	if ( !( GGameType & GAME_Quake3 ) && ( AAS_AreaDoNotEnterLarge( areanum ) || AAS_AreaDoNotEnterLarge( goalareanum ) ) ) {
		travelflags |= WOLFTFL_DONOTENTER_LARGE;
	}

	int clusternum = aasworld->areasettings[ areanum ].cluster;
	int goalclusternum = aasworld->areasettings[ goalareanum ].cluster;

	//check if the area is a portal of the goal area cluster
	if ( clusternum < 0 && goalclusternum > 0 ) {
		aas_portal_t* portal = &aasworld->portals[ -clusternum ];
		if ( portal->frontcluster == goalclusternum ||
			 portal->backcluster == goalclusternum ) {
			clusternum = goalclusternum;
		}
	}
	//check if the goalarea is a portal of the area cluster
	else if ( clusternum > 0 && goalclusternum < 0 ) {
		aas_portal_t* portal = &aasworld->portals[ -goalclusternum ];
		if ( portal->frontcluster == clusternum ||
			 portal->backcluster == clusternum ) {
			goalclusternum = clusternum;
		}
	}

	//if both areas are in the same cluster
	//NOTE: there might be a shorter route via another cluster!!! but we don't care
	if ( clusternum > 0 && goalclusternum > 0 && clusternum == goalclusternum ) {
		aas_routingcache_t* areacache = AAS_GetAreaRoutingCache( clusternum, goalareanum, travelflags, !!( GGameType & GAME_Quake3 ) );
		// RF, note that the routing cache might be NULL now since we are restricting
		// the updates per frame, hopefully rejected cache's will be requested again
		// when things have settled down
		if ( !areacache ) {
			return false;
		}
		//the number of the area in the cluster
		int clusterareanum = AAS_ClusterAreaNum( clusternum, areanum );
		//the cluster the area is in
		aas_cluster_t* cluster = &aasworld->clusters[ clusternum ];
		//if the area is NOT a reachability area
		if ( clusterareanum >= cluster->numreachabilityareas ) {
			return 0;
		}
		//if it is possible to travel to the goal area through this cluster
		if ( areacache->traveltimes[ clusterareanum ] != 0 ) {
			*reachnum = aasworld->areasettings[ areanum ].firstreachablearea +
						areacache->reachabilities[ clusterareanum ];
			if ( !origin ) {
				*traveltime = areacache->traveltimes[ clusterareanum ];
				return true;
			}
			//
			aas_reachability_t* reach = &aasworld->reachability[ *reachnum ];
			*traveltime = areacache->traveltimes[ clusterareanum ] +
						  AAS_AreaTravelTime( areanum, origin, reach->start );
			return true;
		}
	}

	clusternum = aasworld->areasettings[ areanum ].cluster;
	goalclusternum = aasworld->areasettings[ goalareanum ].cluster;
	//if the goal area is a portal
	if ( goalclusternum < 0 ) {
		//just assume the goal area is part of the front cluster
		aas_portal_t* portal = &aasworld->portals[ -goalclusternum ];
		goalclusternum = portal->frontcluster;
	}
	//get the portal routing cache
	aas_routingcache_t* portalcache = AAS_GetPortalRoutingCache( goalclusternum, goalareanum, travelflags );
	//if the area is a cluster portal, read directly from the portal cache
	if ( clusternum < 0 ) {
		*traveltime = portalcache->traveltimes[ -clusternum ];
		*reachnum = aasworld->areasettings[ areanum ].firstreachablearea +
					portalcache->reachabilities[ -clusternum ];
		return true;
	}

	unsigned short int besttime = 0;
	int bestreachnum = -1;
	//the cluster the area is in
	aas_cluster_t* cluster = &aasworld->clusters[ clusternum ];
	//current area inside the current cluster
	int clusterareanum = AAS_ClusterAreaNum( clusternum, areanum );
	//if the area is NOT a reachability area
	if ( clusterareanum >= cluster->numreachabilityareas ) {
		return false;
	}

	aas_portalindex_t* pPortalnum = aasworld->portalindex + cluster->firstportal;
	//find the portal of the area cluster leading towards the goal area
	for ( int i = 0; i < cluster->numportals; i++, pPortalnum++ ) {
		int portalnum = *pPortalnum;
		//if the goal area isn't reachable from the portal
		if ( !portalcache->traveltimes[ portalnum ] ) {
			continue;
		}
		//
		aas_portal_t* portal = aasworld->portals + portalnum;
		// if the area in disabled
		if ( !( GGameType & GAME_Quake3 ) && aasworld->areasettings[ portal->areanum ].areaflags & AREA_DISABLED ) {
			continue;
		}
		// if there is no reachability out of the area
		if ( GGameType & GAME_ET && !aasworld->areasettings[ portal->areanum ].numreachableareas ) {
			continue;
		}
		//get the cache of the portal area
		aas_routingcache_t* areacache = AAS_GetAreaRoutingCache( clusternum, portal->areanum, travelflags, !!( GGameType & GAME_Quake3 ) );
		// RF, this may be NULL if we were unable to calculate the cache this frame
		if ( !areacache ) {
			return false;
		}
		//if the portal is NOT reachable from this area
		if ( !areacache->traveltimes[ clusterareanum ] ) {
			continue;
		}
		//total travel time is the travel time the portal area is from
		//the goal area plus the travel time towards the portal area
		unsigned short int t = portalcache->traveltimes[ portalnum ] + areacache->traveltimes[ clusterareanum ];
		//FIXME: add the exact travel time through the actual portal area
		//NOTE: for now we just add the largest travel time through the portal area
		//		because we can't directly calculate the exact travel time
		//		to be more specific we don't know which reachability was used to travel
		//		into the portal area
		t += aasworld->portalmaxtraveltimes[ portalnum ];

		// Ridah, needs to be up here
		if ( origin || !( GGameType & GAME_Quake3 ) ) {
			*reachnum = aasworld->areasettings[ areanum ].firstreachablearea +
						areacache->reachabilities[ clusterareanum ];
		}
		if ( origin ) {
			aas_reachability_t* reach = aasworld->reachability + *reachnum;
			t += AAS_AreaTravelTime( areanum, origin, reach->start );
		}
		//if the time is better than the one already found
		if ( !besttime || t < besttime ) {
			bestreachnum = *reachnum;
			besttime = t;
		}
	}
	if ( bestreachnum < 0 ) {
		return false;
	}
	*reachnum = bestreachnum;
	*traveltime = besttime;
	return true;
}

int AAS_AreaTravelTimeToGoalArea( int areanum, const vec3_t origin, int goalareanum, int travelflags ) {
	int traveltime, reachnum;
	if ( AAS_AreaRouteToGoalArea( areanum, origin, goalareanum, travelflags, &traveltime, &reachnum ) ) {
		return traveltime;
	}
	return 0;
}

static int AAS_AreaReachabilityToGoalArea( int areanum, const vec3_t origin, int goalareanum, int travelflags ) {
	int traveltime, reachnum;
	if ( AAS_AreaRouteToGoalArea( areanum, origin, goalareanum, travelflags, &traveltime, &reachnum ) ) {
		return reachnum;
	}
	return 0;
}

int AAS_AreaTravelTimeToGoalAreaCheckLoop( int areanum, const vec3_t origin, int goalareanum, int travelflags, int loopareanum ) {
	int traveltime, reachnum;
	if ( AAS_AreaRouteToGoalArea( areanum, origin, goalareanum, travelflags, &traveltime, &reachnum ) ) {
		aas_reachability_t* reach = &aasworld->reachability[ reachnum ];
		if ( loopareanum && reach->areanum == loopareanum ) {
			// going here will cause a looped route
			return 0;
		}
		return traveltime;
	}
	return 0;
}

// predict the route and stop on one of the stop events
bool AAS_PredictRoute( aas_predictroute_t* route, int areanum, const vec3_t origin,
	int goalareanum, int travelflags, int maxareas, int maxtime,
	int stopevent, int stopcontents, int stoptfl, int stopareanum ) {
	//init output
	route->stopevent = RSE_NONE;
	route->endarea = goalareanum;
	route->endcontents = 0;
	route->endtravelflags = 0;
	VectorCopy( origin, route->endpos );
	route->time = 0;

	int curareanum = areanum;
	vec3_t curorigin;
	VectorCopy( origin, curorigin );

	for ( int i = 0; curareanum != goalareanum && ( !maxareas || i < maxareas ) && i < aasworld->numareas; i++ ) {
		int reachnum = AAS_AreaReachabilityToGoalArea( curareanum, curorigin, goalareanum, travelflags );
		if ( !reachnum ) {
			route->stopevent = RSE_NOROUTE;
			return false;
		}
		aas_reachability_t* reach = &aasworld->reachability[ reachnum ];

		if ( stopevent & RSE_USETRAVELTYPE ) {
			if ( AAS_TravelFlagForType( reach->traveltype ) & stoptfl ) {
				route->stopevent = RSE_USETRAVELTYPE;
				route->endarea = curareanum;
				route->endcontents = aasworld->areasettings[ curareanum ].contents;
				route->endtravelflags = AAS_TravelFlagForType( reach->traveltype );
				VectorCopy( reach->start, route->endpos );
				return true;
			}
			if ( AAS_AreaContentsTravelFlags( reach->areanum ) & stoptfl ) {
				route->stopevent = RSE_USETRAVELTYPE;
				route->endarea = reach->areanum;
				route->endcontents = aasworld->areasettings[ reach->areanum ].contents;
				route->endtravelflags = AAS_AreaContentsTravelFlags( reach->areanum );
				VectorCopy( reach->end, route->endpos );
				route->time += AAS_AreaTravelTime( areanum, origin, reach->start );
				route->time += reach->traveltime;
				return true;
			}
		}
		aas_reachabilityareas_t* reachareas = &aasworld->reachabilityareas[ reachnum ];
		for ( int j = 0; j < reachareas->numareas + 1; j++ ) {
			int testareanum;
			if ( j >= reachareas->numareas ) {
				testareanum = reach->areanum;
			} else   {
				testareanum = aasworld->reachabilityareaindex[ reachareas->firstarea + j ];
			}
			if ( stopevent & RSE_ENTERCONTENTS ) {
				if ( aasworld->areasettings[ testareanum ].contents & stopcontents ) {
					route->stopevent = RSE_ENTERCONTENTS;
					route->endarea = testareanum;
					route->endcontents = aasworld->areasettings[ testareanum ].contents;
					VectorCopy( reach->end, route->endpos );
					route->time += AAS_AreaTravelTime( areanum, origin, reach->start );
					route->time += reach->traveltime;
					return true;
				}
			}
			if ( stopevent & RSE_ENTERAREA ) {
				if ( testareanum == stopareanum ) {
					route->stopevent = RSE_ENTERAREA;
					route->endarea = testareanum;
					route->endcontents = aasworld->areasettings[ testareanum ].contents;
					VectorCopy( reach->start, route->endpos );
					return true;
				}
			}
		}

		route->time += AAS_AreaTravelTime( areanum, origin, reach->start );
		route->time += reach->traveltime;
		route->endarea = reach->areanum;
		route->endcontents = aasworld->areasettings[ reach->areanum ].contents;
		route->endtravelflags = AAS_TravelFlagForType( reach->traveltype );
		VectorCopy( reach->end, route->endpos );

		curareanum = reach->areanum;
		VectorCopy( reach->end, curorigin );

		if ( maxtime && route->time > maxtime ) {
			break;
		}
	}
	if ( curareanum != goalareanum ) {
		return false;
	}
	return true;
}

static void AAS_CreateAllRoutingCache() {
	int numroutingareas = 0;
	int tfl = WOLFTFL_DEFAULT;
	BotImport_Print( PRT_MESSAGE, "AAS_CreateAllRoutingCache\n" );

	for ( int i = 1; i < aasworld->numareas; i++ ) {
		if ( !AAS_AreaReachability( i ) ) {
			continue;
		}
		aas_areasettings_t* areasettings = &aasworld->areasettings[ i ];
		int k;
		for ( k = 0; k < areasettings->numreachableareas; k++ ) {
			aas_reachability_t* reach = &aasworld->reachability[ areasettings->firstreachablearea + k ];
			if ( aasworld->travelflagfortype[ reach->traveltype ] & tfl ) {
				break;
			}
		}
		if ( k >= areasettings->numreachableareas ) {
			continue;
		}
		aasworld->areasettings[ i ].areaflags |= WOLFAREA_USEFORROUTING;
		numroutingareas++;
	}
	for ( int i = 1; i < aasworld->numareas; i++ ) {
		if ( !( aasworld->areasettings[ i ].areaflags & WOLFAREA_USEFORROUTING ) ) {
			continue;
		}
		for ( int j = 1; j < aasworld->numareas; j++ ) {
			if ( i == j ) {
				continue;
			}
			if ( !( aasworld->areasettings[ j ].areaflags & WOLFAREA_USEFORROUTING ) ) {
				continue;
			}
			AAS_AreaTravelTimeToGoalArea( j, aasworld->areawaypoints[ j ], i, tfl );
			aasworld->frameroutingupdates = 0;
		}
	}
}

int AAS_ListAreasInRange( const vec3_t srcpos, int srcarea, float range, int travelflags, vec3_t* outareas, int maxareas ) {
	if ( !aasworld->hidetraveltimes ) {
		aasworld->hidetraveltimes = ( unsigned short int* )Mem_ClearedAlloc( aasworld->numareas * sizeof ( unsigned short int ) );
	} else   {
		memset( aasworld->hidetraveltimes, 0, aasworld->numareas * sizeof ( unsigned short int ) );
	}

	int badtravelflags = ~travelflags;

	aas_routingupdate_t* curupdate = &aasworld->areaupdate[ srcarea ];
	curupdate->areanum = srcarea;
	VectorCopy( srcpos, curupdate->start );
	// GORDON: TEMP: FIXME: temp to stop crash
	if ( srcarea == 0 ) {
		return 0;
	}
	curupdate->areatraveltimes = aasworld->areatraveltimes[ srcarea ][ 0 ];
	curupdate->tmptraveltime = 0;
	//put the area to start with in the current read list
	curupdate->next = NULL;
	curupdate->prev = NULL;
	aas_routingupdate_t* updateliststart = curupdate;
	aas_routingupdate_t* updatelistend = curupdate;
	//while there are updates in the current list, flip the lists
	int count = 0;
	while ( updateliststart ) {
		curupdate = updateliststart;
		//
		if ( curupdate->next ) {
			curupdate->next->prev = NULL;
		} else   {
			updatelistend = NULL;
		}
		updateliststart = curupdate->next;
		//
		curupdate->inlist = false;
		//check all reversed reachability links
		int numreach = aasworld->areasettings[ curupdate->areanum ].numreachableareas;
		aas_reachability_t* reach = &aasworld->reachability[ aasworld->areasettings[ curupdate->areanum ].firstreachablearea ];

		for ( int i = 0; i < numreach; i++, reach++ ) {
			//if an undesired travel type is used
			if ( aasworld->travelflagfortype[ reach->traveltype ] & badtravelflags ) {
				continue;
			}
			//
			if ( AAS_AreaContentsTravelFlags( reach->areanum ) & badtravelflags ) {
				continue;
			}
			//number of the area the reachability leads to
			int nextareanum = reach->areanum;
			// if we've already been to this area
			if ( aasworld->hidetraveltimes[ nextareanum ] ) {
				continue;
			}
			aasworld->hidetraveltimes[ nextareanum ] = 1;
			// if it's too far from srcpos, ignore
			if ( Distance( srcpos, aasworld->areawaypoints[ nextareanum ] ) > range ) {
				continue;
			}

			// if visible from srcarea
			if ( !( aasworld->areasettings[ reach->areanum ].areaflags & AREA_LADDER ) &&
				 !( aasworld->areasettings[ reach->areanum ].areaflags & AREA_DISABLED ) &&
				 ( AAS_AreaVisible( srcarea, nextareanum ) ) ) {
				VectorCopy( aasworld->areawaypoints[ nextareanum ], outareas[ count ] );
				count++;
				if ( count >= maxareas ) {
					break;
				}
			}

			aas_routingupdate_t* nextupdate = &aasworld->areaupdate[ nextareanum ];
			nextupdate->areanum = nextareanum;
			//remember where we entered this area
			VectorCopy( reach->end, nextupdate->start );
			//if this update is not in the list yet
			if ( !nextupdate->inlist ) {
				//add the new update to the end of the list
				nextupdate->next = NULL;
				nextupdate->prev = updatelistend;
				if ( updatelistend ) {
					updatelistend->next = nextupdate;
				} else   {
					updateliststart = nextupdate;
				}
				updatelistend = nextupdate;
				nextupdate->inlist = true;
			}
		}
	}
	return count;
}

int AAS_AvoidDangerArea( const vec3_t srcpos, int srcarea, const vec3_t dangerpos, int dangerarea, float range, int travelflags ) {
	if ( !aasworld->areavisibility ) {
		return 0;
	}
	if ( !srcarea ) {
		return 0;
	}

	if ( !aasworld->hidetraveltimes ) {
		aasworld->hidetraveltimes = ( unsigned short int* )Mem_ClearedAlloc( aasworld->numareas * sizeof ( unsigned short int ) );
	} else   {
		memset( aasworld->hidetraveltimes, 0, aasworld->numareas * sizeof ( unsigned short int ) );
	}

	int badtravelflags = ~travelflags;

	aas_routingupdate_t* curupdate = &aasworld->areaupdate[ srcarea ];
	curupdate->areanum = srcarea;
	VectorCopy( srcpos, curupdate->start );
	curupdate->areatraveltimes = aasworld->areatraveltimes[ srcarea ][ 0 ];
	curupdate->tmptraveltime = 0;
	//put the area to start with in the current read list
	curupdate->next = NULL;
	curupdate->prev = NULL;
	aas_routingupdate_t* updateliststart = curupdate;
	aas_routingupdate_t* updatelistend = curupdate;

	// Mad Doctor I, 11/3/2002.  The source area never needs to be expanded
	// again, so mark it as cut off
	aasworld->hidetraveltimes[ srcarea ] = 1;

	int bestarea = 0;
	int bestTravel = 999999;
	const int maxTime = 5000;
	const int goodTime = 1000;
	vec_t dangerDistance = 0;

	//while there are updates in the current list, flip the lists
	while ( updateliststart ) {
		curupdate = updateliststart;
		//
		if ( curupdate->next ) {
			curupdate->next->prev = NULL;
		} else   {
			updatelistend = NULL;
		}
		updateliststart = curupdate->next;

		curupdate->inlist = false;
		//check all reversed reachability links
		int numreach = aasworld->areasettings[ curupdate->areanum ].numreachableareas;
		aas_reachability_t* reach = &aasworld->reachability[ aasworld->areasettings[ curupdate->areanum ].firstreachablearea ];

		for ( int i = 0; i < numreach; i++, reach++ ) {
			//if an undesired travel type is used
			if ( aasworld->travelflagfortype[ reach->traveltype ] & badtravelflags ) {
				continue;
			}

			if ( AAS_AreaContentsTravelFlags( reach->areanum ) & badtravelflags ) {
				continue;
			}
			// dont pass through ladder areas
			if ( aasworld->areasettings[ reach->areanum ].areaflags & AREA_LADDER ) {
				continue;
			}

			if ( aasworld->areasettings[ reach->areanum ].areaflags & AREA_DISABLED ) {
				continue;
			}
			//number of the area the reachability leads to
			int nextareanum = reach->areanum;
			// if we've already been to this area
			if ( aasworld->hidetraveltimes[ nextareanum ] ) {
				continue;
			}
			aasworld->hidetraveltimes[ nextareanum ] = 1;
			// calc traveltime from srcpos
			int t = curupdate->tmptraveltime +
					AAS_AreaTravelTime( curupdate->areanum, curupdate->start, reach->start ) +
					reach->traveltime;
			if ( t > maxTime ) {
				continue;
			}
			if ( t > bestTravel ) {
				continue;
			}

			// How far is it
			dangerDistance = Distance( dangerpos, aasworld->areawaypoints[ nextareanum ] );

			// if it's safe from dangerpos
			if ( aasworld->areavisibility[ nextareanum ] && ( dangerDistance > range ) ) {
				if ( t < goodTime ) {
					return nextareanum;
				}
				if ( t < bestTravel ) {
					bestTravel = t;
					bestarea = nextareanum;
				}
			}

			aas_routingupdate_t* nextupdate = &aasworld->areaupdate[ nextareanum ];
			nextupdate->areanum = nextareanum;
			//remember where we entered this area
			VectorCopy( reach->end, nextupdate->start );
			//if this update is not in the list yet

			// Mad Doctor I, 11/3/2002.  The inlist field seems to not be inited properly for this function.
			// It causes certain routes to be excluded unnecessarily, so I'm trying to do without it.
			// Note that the hidetraveltimes array seems to cut off duplicates already.
			//if (!nextupdate->inlist)
			{
				//add the new update to the end of the list
				nextupdate->next = NULL;
				nextupdate->prev = updatelistend;
				if ( updatelistend ) {
					updatelistend->next = nextupdate;
				} else   {
					updateliststart = nextupdate;
				}
				updatelistend = nextupdate;
				nextupdate->inlist = true;
			}
		}
	}
	return bestarea;
}

// Init the priority queue
static void AAS_DangerPQInit() {
	// Init the distanceFromDanger array if needed
	if ( !aasworld->PQ_accumulator ) {
		// Get memory for this array the safe way.
		aasworld->PQ_accumulator = ( unsigned short int* )Mem_ClearedAlloc( aasworld->numareas * sizeof ( unsigned short int ) );
	}

	// There are no items in the PQ right now
	aasworld->PQ_size = 0;

}

// Put an area into the PQ.  ASSUMES the dangerdistance for the
// area is set ahead of time.
static void AAS_DangerPQInsert( int areaNum ) {
	// Increment the count in the accum
	aasworld->PQ_size++;

	// Put this one at the end
	aasworld->PQ_accumulator[ aasworld->PQ_size ] = areaNum;
}

// Is the Danger Priority Queue empty?
static bool AAS_DangerPQEmpty() {
	// It's not empty if anything is in the accumulator
	if ( aasworld->PQ_size > 0 ) {
		return false;
	}

	return true;
}

// Pull the smallest distance area off of the danger Priority Queue.
static int AAS_DangerPQRemove() {
	int nearest = 1;
	int nearestArea = aasworld->PQ_accumulator[ nearest ];
	int nearestDistance = aasworld->distanceFromDanger[ nearestArea ];

	// Just loop through the items in the PQ
	for ( int j = 2; j <= aasworld->PQ_size; j++ ) {
		// What's the next area?
		int currentArea = aasworld->PQ_accumulator[ j ];

		// What's the danerg distance of it
		int distance = aasworld->distanceFromDanger[ currentArea ];

		// Is this element the best one? Top of the heap, so to speak
		if ( distance < nearestDistance ) {
			// Save this one
			nearest = j;

			// This has the best distance
			nearestDistance = distance;

			// This one is the nearest region so far
			nearestArea = currentArea;
		}
	}

	// Save out the old end of list
	int temp = aasworld->PQ_accumulator[ aasworld->PQ_size ];

	// Put this where the old one was
	aasworld->PQ_accumulator[ nearest ] = temp;

	// Decrement the count
	aasworld->PQ_size--;

	return nearestArea;
}

static void AAS_CalculateDangerZones( const int* dangerSpots, int dangerSpotCount,
	// What is the furthest danger range we care about? (Everything further is safe)
	float dangerRange,
	// A safe distance to init distanceFromDanger to
	unsigned short int definitelySafe ) {
	// Initialize all of the starting danger zones.
	for ( int i = 0; i < dangerSpotCount; i++ ) {
		// Get the area number of this danger spot
		int dangerAreaNum = dangerSpots[ i ];

		// Set it's distance to 0, meaning it's 0 units to danger
		aasworld->distanceFromDanger[ dangerAreaNum ] = 0;

		// Add the zone to the PQ.
		AAS_DangerPQInsert( dangerAreaNum );
	}

	// Go through the Priority Queue, pop off the smallest distance, and expand
	// to the neighboring nodes.  Stop when the PQ is empty.
	while ( !AAS_DangerPQEmpty() ) {
		// Get the smallest distance in the PQ.
		int currentArea = AAS_DangerPQRemove();

		// Check all reversed reachability links
		int numreach = aasworld->areasettings[ currentArea ].numreachableareas;
		aas_reachability_t* reach = &aasworld->reachability[ aasworld->areasettings[ currentArea ].firstreachablearea ];

		// Loop through the neighbors to this node.
		for ( int i = 0; i < numreach; i++, reach++ ) {
			// Number of the area the reachability leads to
			int nextareanum = reach->areanum;

			// How far was it from the last node to this one?
			float distanceFromCurrentNode = Distance( aasworld->areawaypoints[ currentArea ],
				aasworld->areawaypoints[ nextareanum ] );

			// Calculate the distance from danger to this neighbor along the
			// current route.
			int dangerDistance = aasworld->distanceFromDanger[ currentArea ]
								 + ( int )distanceFromCurrentNode;

			// Skip this neighbor if the distance is bigger than we care about.
			if ( dangerDistance > dangerRange ) {
				continue;
			}

			// Store the distance from danger if it's smaller than any previous
			// distance to this node (note that unvisited nodes are inited with
			// a big distance, so this check will be satisfied).
			if ( dangerDistance < aasworld->distanceFromDanger[ nextareanum ] ) {
				// How far was this node from danger before this visit?
				int oldDistance = aasworld->distanceFromDanger[ nextareanum ];

				// Store the new value
				aasworld->distanceFromDanger[ nextareanum ] = dangerDistance;

				// If the neighbor has been calculated already, see if we need to
				// update the priority.
				// Otherwise, insert the neighbor into the PQ.
				if ( oldDistance == definitelySafe ) {
					// Insert this node into the PQ
					AAS_DangerPQInsert( nextareanum );
				}
			}
		}
	}

	// At this point, all of the nodes within our danger range have their
	// distance from danger calculated.
}

// Use this to find a safe spot away from a dangerous situation/enemy.
// This differs from AAS_AvoidDangerArea in the following ways:
// * AAS_Retreat will return the farthest location found even if it does not
//   exceed the desired minimum distance.
// * AAS_Retreat will give preference to nodes on the "safe" side of the danger
int AAS_Retreat( const int* dangerSpots, int dangerSpotCount, const vec3_t srcpos, int srcarea,
	const vec3_t dangerpos, int dangerarea, float range, float dangerRange, int travelflags ) {
	// Choose a safe distance to init distanceFromDanger to
	unsigned short int definitelySafe = 0xFFFF;

	if ( !aasworld->areavisibility ) {
		return 0;
	}

	// Init the hide travel time if needed
	if ( !aasworld->hidetraveltimes ) {
		aasworld->hidetraveltimes = ( unsigned short int* )Mem_ClearedAlloc( aasworld->numareas * sizeof ( unsigned short int ) );
	}
	// Otherwise, it exists already, so just reset the values
	else {
		Com_Memset( aasworld->hidetraveltimes, 0, aasworld->numareas * sizeof ( unsigned short int ) );
	}

	// Init the distanceFromDanger array if needed
	if ( !aasworld->distanceFromDanger ) {
		// Get memory for this array the safe way.
		aasworld->distanceFromDanger = ( unsigned short int* )Mem_ClearedAlloc( aasworld->numareas * sizeof ( unsigned short int ) );
	}

	// Set all the values in the distanceFromDanger array to a safe value
	Com_Memset( aasworld->distanceFromDanger, 0xFF, aasworld->numareas * sizeof ( unsigned short int ) );

	// Init the priority queue
	AAS_DangerPQInit();

	// Set up the distanceFromDanger array
	AAS_CalculateDangerZones( dangerSpots, dangerSpotCount, dangerRange, definitelySafe );

	int badtravelflags = ~travelflags;

	aas_routingupdate_t* curupdate = &aasworld->areaupdate[ srcarea ];
	curupdate->areanum = srcarea;
	VectorCopy( srcpos, curupdate->start );
	curupdate->areatraveltimes = aasworld->areatraveltimes[ srcarea ][ 0 ];
	curupdate->tmptraveltime = 0;
	//put the area to start with in the current read list
	curupdate->next = NULL;
	curupdate->prev = NULL;
	aas_routingupdate_t* updateliststart = curupdate;
	aas_routingupdate_t* updatelistend = curupdate;

	// Mad Doctor I, 11/3/2002.  The source area never needs to be expanded
	// again, so mark it as cut off
	aasworld->hidetraveltimes[ srcarea ] = 1;

	int bestarea = 0;
	const int maxTime = 5000;
	vec_t dangerDistance = 0;
	vec_t sourceDistance = 0;
	float bestDistanceSoFar = 0;

	//while there are updates in the current list, flip the lists
	while ( updateliststart ) {
		curupdate = updateliststart;
		//
		if ( curupdate->next ) {
			curupdate->next->prev = NULL;
		} else   {
			updatelistend = NULL;
		}
		updateliststart = curupdate->next;

		curupdate->inlist = false;
		//check all reversed reachability links
		int numreach = aasworld->areasettings[ curupdate->areanum ].numreachableareas;
		aas_reachability_t* reach = &aasworld->reachability[ aasworld->areasettings[ curupdate->areanum ].firstreachablearea ];

		for ( int i = 0; i < numreach; i++, reach++ ) {
			//if an undesired travel type is used
			if ( aasworld->travelflagfortype[ reach->traveltype ] & badtravelflags ) {
				continue;
			}

			if ( AAS_AreaContentsTravelFlags( reach->areanum ) & badtravelflags ) {
				continue;
			}
			// dont pass through ladder areas
			if ( aasworld->areasettings[ reach->areanum ].areaflags & AREA_LADDER ) {
				continue;
			}

			if ( aasworld->areasettings[ reach->areanum ].areaflags & AREA_DISABLED ) {
				continue;
			}
			//number of the area the reachability leads to
			int nextareanum = reach->areanum;
			// if we've already been to this area
			if ( aasworld->hidetraveltimes[ nextareanum ] ) {
				continue;
			}
			aasworld->hidetraveltimes[ nextareanum ] = 1;
			// calc traveltime from srcpos
			int t = curupdate->tmptraveltime +
					AAS_AreaTravelTime( curupdate->areanum, curupdate->start, reach->start ) +
					reach->traveltime;
			if ( t > maxTime ) {
				continue;
			}

			// How far is it from a danger area?
			dangerDistance = aasworld->distanceFromDanger[ nextareanum ];

			// How far is it from our starting position?
			sourceDistance = Distance( srcpos, aasworld->areawaypoints[ nextareanum ] );

			// If it's safe from dangerpos
			if ( aasworld->areavisibility[ nextareanum ] &&
				 ( sourceDistance > range ) &&
				 ( ( dangerDistance > dangerRange ) ||
				   ( dangerDistance == definitelySafe ) ) ) {
				// Just use this area
				return nextareanum;
			}

			// In case we don't find a perfect one, save the best
			if ( dangerDistance > bestDistanceSoFar ) {
				bestarea = nextareanum;
				bestDistanceSoFar = dangerDistance;
			}

			aas_routingupdate_t* nextupdate = &aasworld->areaupdate[ nextareanum ];
			nextupdate->areanum = nextareanum;
			//remember where we entered this area
			VectorCopy( reach->end, nextupdate->start );
			//if this update is not in the list yet

			//add the new update to the end of the list
			nextupdate->next = NULL;
			nextupdate->prev = updatelistend;
			if ( updatelistend ) {
				updatelistend->next = nextupdate;
			} else   {
				updateliststart = nextupdate;
			}
			updatelistend = nextupdate;
			nextupdate->inlist = true;
		}
	}

	return bestarea;
}

void AAS_InitRouting() {
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
	if ( GGameType & GAME_Quake3 ) {
		//get the areas reachabilities go through
		AAS_InitReachabilityAreas();
	}

#ifdef ROUTING_DEBUG
	numareacacheupdates = 0;
	numportalcacheupdates = 0;
#endif

	routingcachesize = 0;
	max_routingcachesize = 1024 * ( int )LibVarValue( "max_routingcache", GGameType & GAME_ET ? "16384" : "4096" );
	max_frameroutingupdates = GGameType & GAME_ET ? ( int )LibVarGetValue( "bot_frameroutingupdates" ) : MAX_FRAMEROUTINGUPDATES_WOLF;

	// enable this for quick testing of maps without enemies
	if ( GGameType & GAME_ET && LibVarGetValue( "bot_norcd" ) ) {
		// RF, create the waypoints for each area
		AAS_CreateVisibility( true );
	} else   {
		// read any routing cache if available
		if ( !AAS_ReadRouteCache() ) {
			if ( !( GGameType & GAME_Quake3 ) ) {
				aasworld->initialized = true;	// Hack, so routing can compute traveltimes
				AAS_CreateVisibility( false );
				if ( !( GGameType & GAME_ET ) ) {
					AAS_CreateAllRoutingCache();
				}
				aasworld->initialized = false;

				AAS_WriteRouteCache();	// save it so we don't have to create it again
			}
		}
	}
}

int AAS_NearestHideArea( int srcnum, vec3_t origin, int areanum, int enemynum,
	vec3_t enemyorigin, int enemyareanum, int travelflags, float maxdist, const vec3_t distpos ) {
	int MAX_HIDEAREA_LOOPS = ( GGameType & GAME_WolfMP ? 4000 : 3000 );
	static float lastTime;
	static int loopCount;

	if ( !aasworld->areavisibility ) {
		return 0;
	}

	if ( !( GGameType & GAME_WolfMP ) && srcnum < 0 ) {			// hack to force run this call
		srcnum = -srcnum - 1;
		lastTime = 0;
	}
	// don't run this more than once per frame
	if ( lastTime == AAS_Time() && ( GGameType & GAME_WolfMP || loopCount >= MAX_HIDEAREA_LOOPS ) ) {
		return 0;
	}
	if ( !( GGameType & GAME_ET ) || lastTime != AAS_Time() ) {
		loopCount = 0;
	}
	lastTime = AAS_Time();

	if ( !aasworld->hidetraveltimes ) {
		aasworld->hidetraveltimes = ( unsigned short int* )Mem_ClearedAlloc( aasworld->numareas * sizeof ( unsigned short int ) );
	} else   {
		memset( aasworld->hidetraveltimes, 0, aasworld->numareas * sizeof ( unsigned short int ) );
	}

	if ( !aasworld->visCache ) {
		aasworld->visCache = ( byte* )Mem_ClearedAlloc( aasworld->numareas * sizeof ( byte ) );
	} else   {
		memset( aasworld->visCache, 0, aasworld->numareas * sizeof ( byte ) );
	}
	unsigned short int besttraveltime = 0;
	int bestarea = 0;
	if ( enemyareanum ) {
		//JL this looks useless.
		AAS_AreaTravelTimeToGoalArea( areanum, origin, enemyareanum, travelflags );
	}
	vec3_t enemyVec;
	VectorSubtract( enemyorigin, origin, enemyVec );
	float enemytraveldist = VectorNormalize( enemyVec );
	bool startVisible = SVT3_BotVisibleFromPos( enemyorigin, enemynum, origin, srcnum, false );

	int badtravelflags = ~travelflags;

	aas_routingupdate_t* curupdate = &aasworld->areaupdate[ areanum ];
	curupdate->areanum = areanum;
	VectorCopy( origin, curupdate->start );
	// GORDON: TEMP: FIXME: just avoiding a crash for the moment
	if ( GGameType & GAME_ET && areanum == 0 ) {
		return 0;
	}
	curupdate->areatraveltimes = aasworld->areatraveltimes[ areanum ][ 0 ];
	curupdate->tmptraveltime = 0;
	//put the area to start with in the current read list
	curupdate->next = NULL;
	curupdate->prev = NULL;
	aas_routingupdate_t* updateliststart = curupdate;
	aas_routingupdate_t* updatelistend = curupdate;
	//while there are updates in the current list, flip the lists
	while ( updateliststart ) {
		curupdate = updateliststart;

		if ( curupdate->next ) {
			curupdate->next->prev = NULL;
		} else   {
			updatelistend = NULL;
		}
		updateliststart = curupdate->next;

		curupdate->inlist = false;
		//check all reversed reachability links
		int numreach = aasworld->areasettings[ curupdate->areanum ].numreachableareas;
		aas_reachability_t* reach = &aasworld->reachability[ aasworld->areasettings[ curupdate->areanum ].firstreachablearea ];

		for ( int i = 0; i < numreach; i++, reach++ ) {
			//if an undesired travel type is used
			if ( aasworld->travelflagfortype[ reach->traveltype ] & badtravelflags ) {
				continue;
			}

			if ( AAS_AreaContentsTravelFlags( reach->areanum ) & badtravelflags ) {
				continue;
			}
			// dont pass through ladder areas
			if ( !( GGameType & GAME_WolfMP ) && aasworld->areasettings[ reach->areanum ].areaflags & AREA_LADDER ) {
				continue;
			}

			if ( aasworld->areasettings[ reach->areanum ].areaflags & AREA_DISABLED ) {
				continue;
			}
			//number of the area the reachability leads to
			int nextareanum = reach->areanum;
			// if this moves us into the enemies area, skip it
			if ( nextareanum == enemyareanum ) {
				continue;
			}
			// if this moves us outside maxdist
			if ( distpos && ( VectorDistance( reach->end, distpos ) > maxdist ) ) {
				continue;
			}
			//time already travelled plus the traveltime through
			//the current area plus the travel time from the reachability
			unsigned short int t = curupdate->tmptraveltime +
								   AAS_AreaTravelTime( curupdate->areanum, curupdate->start, reach->start ) +
								   reach->traveltime;
			if ( !( GGameType & GAME_WolfMP ) ) {
				// inc the loopCount, we are starting to use a bit of cpu time
				loopCount++;
			}
			// if this isn't the fastest route to this area, ignore
			if ( aasworld->hidetraveltimes[ nextareanum ] && aasworld->hidetraveltimes[ nextareanum ] < t ) {
				continue;
			}
			aasworld->hidetraveltimes[ nextareanum ] = t;
			// if the bestarea is this area, then it must be a longer route, so ignore it
			if ( bestarea == nextareanum ) {
				bestarea = 0;
				besttraveltime = 0;
			}
			// do this test now, so we can reject the route if it starts out too long
			if ( besttraveltime && t >= besttraveltime ) {
				continue;
			}

			//avoid going near the enemy
			vec3_t p;
			ProjectPointOntoVectorFromPoints( enemyorigin, curupdate->start, reach->end, p );
			int j;
			for ( j = 0; j < 3; j++ ) {
				if ( ( p[ j ] > curupdate->start[ j ] + 0.1 && p[ j ] > reach->end[ j ] + 0.1 ) ||
					 ( p[ j ] < curupdate->start[ j ] - 0.1 && p[ j ] < reach->end[ j ] - 0.1 ) ) {
					break;
				}
			}
			vec3_t v2;
			if ( j < 3 ) {
				VectorSubtract( enemyorigin, reach->end, v2 );
			} else   {
				VectorSubtract( enemyorigin, p, v2 );
			}
			float dist2 = VectorLength( v2 );
			//never go through the enemy
			if ( enemytraveldist > 32 && dist2 < enemytraveldist && dist2 < 256 ) {
				continue;
			}

			VectorSubtract( reach->end, origin, v2 );
			if ( enemytraveldist > 32 && DotProduct( v2, enemyVec ) > enemytraveldist / 2 ) {
				continue;
			}

			vec3_t v1;
			VectorSubtract( enemyorigin, curupdate->start, v1 );
			float dist1 = VectorLength( v1 );

			if ( enemytraveldist > 32 && dist2 < dist1 ) {
				t += ( dist1 - dist2 ) * 10;
				// test it again after modifying it
				if ( besttraveltime && t >= besttraveltime ) {
					continue;
				}
			}
			// make sure the hide area doesn't have anyone else in it
			if ( !( GGameType & GAME_WolfSP ) && AAS_IsEntityInArea( srcnum, -1, nextareanum ) ) {
				t += 1000;	// avoid this path/area
			}
			//
			// if we weren't visible when starting, make sure we don't move into their view
			if ( enemyareanum && !startVisible && AAS_AreaVisible( enemyareanum, nextareanum ) ) {
				continue;
			}

			if ( !besttraveltime || besttraveltime > t ) {
				// if this area doesn't have a vis list, ignore it
				if ( aasworld->areavisibility[ nextareanum ] ) {
					//if the nextarea is not visible from the enemy area
					if ( !AAS_AreaVisible( enemyareanum, nextareanum ) ) {		// now last of all, check that this area is a safe hiding spot
						if ( ( aasworld->visCache[ nextareanum ] == 2 ) ||
							 ( !aasworld->visCache[ nextareanum ] && !SVT3_BotVisibleFromPos( enemyorigin, enemynum, aasworld->areawaypoints[ nextareanum ], srcnum, false ) ) ) {
							aasworld->visCache[ nextareanum ] = 2;
							besttraveltime = t;
							bestarea = nextareanum;
						} else   {
							aasworld->visCache[ nextareanum ] = 1;
						}
					}
				}

				// getting down to here is bad for cpu usage
				if ( loopCount++ > MAX_HIDEAREA_LOOPS ) {
					continue;
				}

				// otherwise, add this to the list so we check is reachables
				// disabled, this should only store the raw traveltime, not the adjusted time
				//aasworld->hidetraveltimes[nextareanum] = t;
				aas_routingupdate_t* nextupdate = &aasworld->areaupdate[ nextareanum ];
				nextupdate->areanum = nextareanum;
				nextupdate->tmptraveltime = t;
				//remember where we entered this area
				VectorCopy( reach->end, nextupdate->start );
				//if this update is not in the list yet
				if ( !nextupdate->inlist ) {
					//add the new update to the end of the list
					nextupdate->next = NULL;
					nextupdate->prev = updatelistend;
					if ( updatelistend ) {
						updatelistend->next = nextupdate;
					} else   {
						updateliststart = nextupdate;
					}
					updatelistend = nextupdate;
					nextupdate->inlist = true;
				}
			}
		}
	}
	return bestarea;
}

//	"src" is hiding ent, "dest" is the enemy
bool AAS_RT_GetHidePos( vec3_t srcpos, int srcnum, int srcarea, vec3_t destpos, int destnum, int destarea, vec3_t returnPos ) {
	// use MrE's breadth first method
	int hideareanum = AAS_NearestHideArea( srcnum, srcpos, srcarea, destnum, destpos, destarea, WOLFTFL_DEFAULT, 0, NULL );
	if ( !hideareanum ) {
		return false;
	}
	// we found a valid hiding area
	VectorCopy( aasworld->areawaypoints[ hideareanum ], returnPos );

	return true;
}

int AAS_FindAttackSpotWithinRange( int srcnum, int rangenum, int enemynum, float rangedist,
	int travelflags, float* outpos ) {
	enum { MAX_ATTACKAREA_LOOPS = 200 };
	static float lastTime;

	// don't run this more than once per frame
	if ( lastTime == AAS_Time() ) {
		return 0;
	}
	lastTime = AAS_Time();

	if ( !aasworld->hidetraveltimes ) {
		aasworld->hidetraveltimes = ( unsigned short int* )Mem_ClearedAlloc( aasworld->numareas * sizeof ( unsigned short int ) );
	} else   {
		memset( aasworld->hidetraveltimes, 0, aasworld->numareas * sizeof ( unsigned short int ) );
	}

	if ( !aasworld->visCache ) {
		aasworld->visCache = ( byte* )Mem_ClearedAlloc( aasworld->numareas * sizeof ( byte ) );
	} else   {
		memset( aasworld->visCache, 0, aasworld->numareas * sizeof ( byte ) );
	}

	vec3_t srcorg;
	AAS_EntityOrigin( srcnum, srcorg );
	vec3_t rangeorg;
	AAS_EntityOrigin( rangenum, rangeorg );
	vec3_t enemyorg;
	AAS_EntityOrigin( enemynum, enemyorg );

	int srcarea, rangearea, enemyarea;
	if ( GGameType & GAME_WolfMP ) {
		srcarea = AAS_BestReachableEntityArea( srcnum );
		rangearea = AAS_BestReachableEntityArea( rangenum );
		enemyarea = AAS_BestReachableEntityArea( enemynum );
	} else   {
		srcarea = BotFuzzyPointReachabilityArea( srcorg );
		rangearea = BotFuzzyPointReachabilityArea( rangeorg );
		enemyarea = BotFuzzyPointReachabilityArea( enemyorg );
	}

	unsigned short int besttraveltime = 0;
	int bestarea = 0;
	//JL looks useless
	AAS_AreaTravelTimeToGoalArea( srcarea, srcorg, enemyarea, travelflags );

	int badtravelflags = ~travelflags;

	aas_routingupdate_t* curupdate = &aasworld->areaupdate[ rangearea ];
	curupdate->areanum = rangearea;
	VectorCopy( rangeorg, curupdate->start );
	curupdate->areatraveltimes = aasworld->areatraveltimes[ srcarea ][ 0 ];
	curupdate->tmptraveltime = 0;
	//put the area to start with in the current read list
	curupdate->next = NULL;
	curupdate->prev = NULL;
	aas_routingupdate_t* updateliststart = curupdate;
	aas_routingupdate_t* updatelistend = curupdate;
	//while there are updates in the current list, flip the lists
	int count = 0;
	while ( updateliststart ) {
		curupdate = updateliststart;

		if ( curupdate->next ) {
			curupdate->next->prev = NULL;
		} else   {
			updatelistend = NULL;
		}
		updateliststart = curupdate->next;

		curupdate->inlist = false;
		//check all reversed reachability links
		int numreach = aasworld->areasettings[ curupdate->areanum ].numreachableareas;
		aas_reachability_t* reach = &aasworld->reachability[ aasworld->areasettings[ curupdate->areanum ].firstreachablearea ];

		for ( int i = 0; i < numreach; i++, reach++ ) {
			//if an undesired travel type is used
			if ( aasworld->travelflagfortype[ reach->traveltype ] & badtravelflags ) {
				continue;
			}
			//
			if ( AAS_AreaContentsTravelFlags( reach->areanum ) & badtravelflags ) {
				continue;
			}
			// dont pass through ladder areas
			if ( !( GGameType & GAME_WolfMP ) && aasworld->areasettings[ reach->areanum ].areaflags & AREA_LADDER ) {
				continue;
			}

			if ( aasworld->areasettings[ reach->areanum ].areaflags & AREA_DISABLED ) {
				continue;
			}
			//number of the area the reachability leads to
			int nextareanum = reach->areanum;
			// if this moves us into the enemies area, skip it
			if ( nextareanum == enemyarea ) {
				continue;
			}
			// if we've already been to this area
			if ( aasworld->hidetraveltimes[ nextareanum ] ) {
				continue;
			}
			//time already travelled plus the traveltime through
			//the current area plus the travel time from the reachability
			if ( count++ > MAX_ATTACKAREA_LOOPS ) {
				//BotImport_Print(PRT_MESSAGE, "AAS_FindAttackSpotWithinRange: exceeded max loops, aborting\n" );
				if ( bestarea ) {
					VectorCopy( aasworld->areawaypoints[ bestarea ], outpos );
				}
				return bestarea;
			}
			unsigned short int t = curupdate->tmptraveltime +
								   AAS_AreaTravelTime( curupdate->areanum, curupdate->start, reach->start ) +
								   reach->traveltime;

			// if it's too far from rangenum, ignore
			if ( Distance( rangeorg, aasworld->areawaypoints[ nextareanum ] ) > rangedist ) {
				continue;
			}

			// find the traveltime from srcnum
			unsigned short int srctraveltime = AAS_AreaTravelTimeToGoalArea( srcarea, srcorg, nextareanum, travelflags );
			// do this test now, so we can reject the route if it starts out too long
			if ( besttraveltime && srctraveltime >= besttraveltime ) {
				continue;
			}
			//
			// if this area doesn't have a vis list, ignore it
			if ( aasworld->areavisibility[ nextareanum ] ) {
				//if the nextarea can see the enemy area
				if ( AAS_AreaVisible( enemyarea, nextareanum ) ) {		// now last of all, check that this area is a good attacking spot
					if ( ( aasworld->visCache[ nextareanum ] == 2 ) ||
						 ( !aasworld->visCache[ nextareanum ] &&
						   ( count += 10 ) &&			// we are about to use lots of CPU time
						   SVT3_BotCheckAttackAtPos( srcnum, enemynum, aasworld->areawaypoints[ nextareanum ], false, false ) ) ) {
						aasworld->visCache[ nextareanum ] = 2;
						besttraveltime = srctraveltime;
						bestarea = nextareanum;
					} else   {
						aasworld->visCache[ nextareanum ] = 1;
					}
				}
			}
			aasworld->hidetraveltimes[ nextareanum ] = t;
			aas_routingupdate_t* nextupdate = &aasworld->areaupdate[ nextareanum ];
			nextupdate->areanum = nextareanum;
			nextupdate->tmptraveltime = t;
			//remember where we entered this area
			VectorCopy( reach->end, nextupdate->start );
			//if this update is not in the list yet
			if ( !nextupdate->inlist ) {
				//add the new update to the end of the list
				nextupdate->next = NULL;
				nextupdate->prev = updatelistend;
				if ( updatelistend ) {
					updatelistend->next = nextupdate;
				} else   {
					updateliststart = nextupdate;
				}
				updatelistend = nextupdate;
				nextupdate->inlist = true;
			}
		}
	}
	if ( bestarea ) {
		VectorCopy( aasworld->areawaypoints[ bestarea ], outpos );
	}
	return bestarea;
}

bool AAS_GetRouteFirstVisPos( const vec3_t srcpos, const vec3_t destpos, int travelflags, vec3_t retpos ) {
	enum { MAX_GETROUTE_VISPOS_LOOPS = 200 };
	//
	// SRCPOS: enemy
	// DESTPOS: self
	// RETPOS: first area that is visible from destpos, in route from srcpos to destpos
	int srcarea = BotFuzzyPointReachabilityArea( srcpos );
	if ( !srcarea ) {
		return false;
	}
	int destarea = BotFuzzyPointReachabilityArea( destpos );
	if ( !destarea ) {
		return false;
	}
	if ( destarea == srcarea ) {
		VectorCopy( srcpos, retpos );
		return true;
	}

	//if the srcarea can see the destarea
	if ( AAS_AreaVisible( srcarea, destarea ) ) {
		VectorCopy( srcpos, retpos );
		return true;
	}
	// if this area doesn't have a vis list, ignore it
	if ( !aasworld->areavisibility[ destarea ] ) {
		return false;
	}

	int travarea = srcarea;
	vec3_t travpos;
	VectorCopy( srcpos, travpos );
	int lasttraveltime = -1;
	int loops = 0;
	int ftraveltime, freachnum;
	while ( ( loops++ < MAX_GETROUTE_VISPOS_LOOPS ) && AAS_AreaRouteToGoalArea( travarea, travpos, destarea, travelflags, &ftraveltime, &freachnum ) ) {
		if ( lasttraveltime >= 0 && ftraveltime >= lasttraveltime ) {
			return false;	// we may be in a loop
		}
		lasttraveltime = ftraveltime;

		aas_reachability_t reach;
		AAS_ReachabilityFromNum( freachnum, &reach );
		if ( reach.areanum == destarea ) {
			VectorCopy( travpos, retpos );
			return true;
		}
		//if the reach area can see the destarea
		if ( AAS_AreaVisible( reach.areanum, destarea ) ) {
			VectorCopy( reach.end, retpos );
			return true;
		}

		travarea = reach.areanum;
		VectorCopy( reach.end, travpos );
	}

	// unsuccessful
	return false;
}
