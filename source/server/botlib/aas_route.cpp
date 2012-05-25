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

#include "../server.h"
#include "local.h"

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

#define RCID                        (('C' << 24) + ('R' << 16) + ('E' << 8) + 'M')
#define RCVERSION_Q3                2
#define RCVERSION_WS                15
#define RCVERSION_WM                12
#define RCVERSION_ET                16

//the route cache header
//this header is followed by numportalcache + numareacache aas_routingcache_t
//structures that store routing cache
struct routecacheheader_q3_t
{
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
struct routecacheheader_wolf_t
{
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

struct aas_routingcache_f_t
{
	int size;									//size of the routing cache
	float time;									//last time accessed or updated
	int cluster;								//cluster the cache is for
	int areanum;								//area the cache is created for
	vec3_t origin;								//origin within the area
	float starttraveltime;						//travel time to start with
	int travelflags;							//combinations of the travel flags
	int prev, next;
	int reachabilities;				//reachabilities used for routing
	unsigned short int traveltimes[1];			//travel time for every area (variable sized)
};

#ifdef ROUTING_DEBUG
int numareacacheupdates;
int numportalcacheupdates;
#endif	//ROUTING_DEBUG

int routingcachesize;
int max_routingcachesize;
int max_frameroutingupdates;

void AAS_RoutingInfo()
{
#ifdef ROUTING_DEBUG
	BotImport_Print(PRT_MESSAGE, "%d area cache updates\n", numareacacheupdates);
	BotImport_Print(PRT_MESSAGE, "%d portal cache updates\n", numportalcacheupdates);
	BotImport_Print(PRT_MESSAGE, "%d bytes routing cache\n", routingcachesize);
#endif
}

// returns the number of the area in the cluster
// assumes the given area is in the given cluster or a portal of the cluster
/*inline*/ int AAS_ClusterAreaNum(int cluster, int areanum)
{
	int areacluster = aasworld->areasettings[areanum].cluster;
	if (areacluster > 0)
	{
		return aasworld->areasettings[areanum].clusterareanum;
	}
	else
	{
		int side = aasworld->portals[-areacluster].frontcluster != cluster;
		return aasworld->portals[-areacluster].clusterareanum[side];
	}
}

void AAS_InitTravelFlagFromType()
{
	for (int i = 0; i < MAX_TRAVELTYPES; i++)
	{
		aasworld->travelflagfortype[i] = TFL_INVALID;
	}
	aasworld->travelflagfortype[TRAVEL_INVALID] = TFL_INVALID;
	aasworld->travelflagfortype[TRAVEL_WALK] = TFL_WALK;
	aasworld->travelflagfortype[TRAVEL_CROUCH] = TFL_CROUCH;
	aasworld->travelflagfortype[TRAVEL_BARRIERJUMP] = TFL_BARRIERJUMP;
	aasworld->travelflagfortype[TRAVEL_JUMP] = TFL_JUMP;
	aasworld->travelflagfortype[TRAVEL_LADDER] = TFL_LADDER;
	aasworld->travelflagfortype[TRAVEL_WALKOFFLEDGE] = TFL_WALKOFFLEDGE;
	aasworld->travelflagfortype[TRAVEL_SWIM] = TFL_SWIM;
	aasworld->travelflagfortype[TRAVEL_WATERJUMP] = TFL_WATERJUMP;
	aasworld->travelflagfortype[TRAVEL_TELEPORT] = TFL_TELEPORT;
	aasworld->travelflagfortype[TRAVEL_ELEVATOR] = TFL_ELEVATOR;
	aasworld->travelflagfortype[TRAVEL_ROCKETJUMP] = TFL_ROCKETJUMP;
	aasworld->travelflagfortype[TRAVEL_BFGJUMP] = TFL_BFGJUMP;
	aasworld->travelflagfortype[TRAVEL_GRAPPLEHOOK] = TFL_GRAPPLEHOOK;
	aasworld->travelflagfortype[TRAVEL_DOUBLEJUMP] = TFL_DOUBLEJUMP;
	aasworld->travelflagfortype[TRAVEL_RAMPJUMP] = TFL_RAMPJUMP;
	aasworld->travelflagfortype[TRAVEL_STRAFEJUMP] = TFL_STRAFEJUMP;
	aasworld->travelflagfortype[TRAVEL_JUMPPAD] = TFL_JUMPPAD;
	aasworld->travelflagfortype[TRAVEL_FUNCBOB] = TFL_FUNCBOB;
}

int AAS_TravelFlagForType(int traveltype)
{
	traveltype &= TRAVELTYPE_MASK;
	if (traveltype < 0 || traveltype >= MAX_TRAVELTYPES)
	{
		return TFL_INVALID;
	}
	return aasworld->travelflagfortype[traveltype];
}

static inline float AAS_RoutingTime()
{
	return AAS_Time();
}

static void AAS_LinkCache(aas_routingcache_t* cache)
{
	if (aasworld->newestcache)
	{
		aasworld->newestcache->time_next = cache;
		cache->time_prev = aasworld->newestcache;
	}
	else
	{
		aasworld->oldestcache = cache;
		cache->time_prev = NULL;
	}
	cache->time_next = NULL;
	aasworld->newestcache = cache;
}

static void AAS_UnlinkCache(aas_routingcache_t* cache)
{
	if (cache->time_next)
	{
		cache->time_next->time_prev = cache->time_prev;
	}
	else
	{
		aasworld->newestcache = cache->time_prev;
	}
	if (cache->time_prev)
	{
		cache->time_prev->time_next = cache->time_next;
	}
	else
	{
		aasworld->oldestcache = cache->time_next;
	}
	cache->time_next = NULL;
	cache->time_prev = NULL;
}

static aas_routingcache_t* AAS_AllocRoutingCache(int numtraveltimes)
{
	int size = sizeof(aas_routingcache_t) +
		numtraveltimes * sizeof(unsigned short int) +
		numtraveltimes * sizeof(unsigned char);

	routingcachesize += size;

	aas_routingcache_t* cache = (aas_routingcache_t*)Mem_ClearedAlloc(size);
	cache->reachabilities = (unsigned char*)cache + sizeof(aas_routingcache_t) +
		numtraveltimes * sizeof(unsigned short int);
	cache->size = size;
	return cache;
}

static void AAS_FreeRoutingCache(aas_routingcache_t* cache)
{
	AAS_UnlinkCache(cache);
	routingcachesize -= cache->size;
	Mem_Free(cache);
}

static void AAS_RemoveRoutingCacheInCluster(int clusternum)
{
	int i;
	aas_routingcache_t* cache, * nextcache;
	aas_cluster_t* cluster;

	if (!aasworld->clusterareacache)
	{
		return;
	}
	cluster = &aasworld->clusters[clusternum];
	for (i = 0; i < cluster->numareas; i++)
	{
		for (cache = aasworld->clusterareacache[clusternum][i]; cache; cache = nextcache)
		{
			nextcache = cache->next;
			AAS_FreeRoutingCache(cache);
		}
		aasworld->clusterareacache[clusternum][i] = NULL;
	}
}

void AAS_RemoveRoutingCacheUsingArea(int areanum)
{
	int clusternum = aasworld->areasettings[areanum].cluster;
	if (clusternum > 0)
	{
		//remove all the cache in the cluster the area is in
		AAS_RemoveRoutingCacheInCluster(clusternum);
	}
	else
	{
		// if this is a portal remove all cache in both the front and back cluster
		AAS_RemoveRoutingCacheInCluster(aasworld->portals[-clusternum].frontcluster);
		AAS_RemoveRoutingCacheInCluster(aasworld->portals[-clusternum].backcluster);
	}
	// remove all portal cache
	for (int i = 0; i < aasworld->numareas; i++)
	{
		//refresh portal cache
		aas_routingcache_t* nextcache;
		for (aas_routingcache_t* cache = aasworld->portalcache[i]; cache; cache = nextcache)
		{
			nextcache = cache->next;
			AAS_FreeRoutingCache(cache);
		}
		aasworld->portalcache[i] = NULL;
	}
}

void AAS_ClearClusterTeamFlags(int areanum)
{
	int clusternum = aasworld->areasettings[areanum].cluster;
	if (clusternum > 0)
	{
		aasworld->clusterTeamTravelFlags[clusternum] = -1;	// recalculate
	}
}

int AAS_EnableRoutingArea(int areanum, int enable)
{
	if (areanum <= 0 || areanum >= aasworld->numareas)
	{
		if (bot_developer)
		{
			BotImport_Print(PRT_ERROR, "AAS_EnableRoutingArea: areanum %d out of range\n", areanum);
		}
		return 0;
	}

	int bitflag;	// flag to set or clear
	if (GGameType & GAME_ET)
	{
		if ((enable & 1) || (enable < 0))
		{
			// clear all flags
			bitflag = ETAREA_AVOID | AREA_DISABLED | ETAREA_TEAM_AXIS | ETAREA_TEAM_ALLIES | ETAREA_TEAM_AXIS_DISGUISED | ETAREA_TEAM_ALLIES_DISGUISED;
		}
		else if (enable & 0x10)
		{
			bitflag = ETAREA_AVOID;
		}
		else if (enable & 0x20)
		{
			bitflag = ETAREA_TEAM_AXIS;
		}
		else if (enable & 0x40)
		{
			bitflag = ETAREA_TEAM_ALLIES;
		}
		else if (enable & 0x80)
		{
			bitflag = ETAREA_TEAM_AXIS_DISGUISED;
		}
		else if (enable & 0x100)
		{
			bitflag = ETAREA_TEAM_ALLIES_DISGUISED;
		}
		else
		{
			bitflag = AREA_DISABLED;
		}

		// remove avoidance flag
		enable &= 1;
	}
	else
	{
		bitflag = AREA_DISABLED;
	}

	int flags = aasworld->areasettings[areanum].areaflags & bitflag;
	if (enable < 0)
	{
		return !flags;
	}

	if (enable)
	{
		aasworld->areasettings[areanum].areaflags &= ~bitflag;
	}
	else
	{
		aasworld->areasettings[areanum].areaflags |= bitflag;
	}

	// if the status of the area changed
	if ((flags & bitflag) != (aasworld->areasettings[areanum].areaflags & bitflag))
	{
		//remove all routing cache involving this area
		AAS_RemoveRoutingCacheUsingArea(areanum);
		if (GGameType & GAME_ET)
		{
			// recalculate the team flags that are used in this cluster
			AAS_ClearClusterTeamFlags(areanum);
		}
	}
	return !flags;
}

void AAS_EnableAllAreas()
{
	for (int i = 0; i < aasworld->numareas; i++)
	{
		if (aasworld->areasettings[i].areaflags & AREA_DISABLED)
		{
			AAS_EnableRoutingArea(i, true);
		}
	}
}

static int AAS_GetAreaContentsTravelFlags(int areanum)
{
	int contents = aasworld->areasettings[areanum].contents;
	int tfl = 0;
	if (contents & AREACONTENTS_WATER)
	{
		tfl |= TFL_WATER;
	}
	else if (contents & AREACONTENTS_SLIME)
	{
		tfl |= TFL_SLIME;
	}
	else if (contents & AREACONTENTS_LAVA)
	{
		tfl |= TFL_LAVA;
	}
	else
	{
		tfl |= TFL_AIR;
	}
	if (contents & AREACONTENTS_DONOTENTER)
	{
		tfl |= TFL_DONOTENTER;
	}
	if (GGameType & GAME_Quake3)
	{
		if (contents & Q3AREACONTENTS_NOTTEAM1)
		{
			tfl |= Q3TFL_NOTTEAM1;
		}
		if (contents & Q3AREACONTENTS_NOTTEAM2)
		{
			tfl |= Q3TFL_NOTTEAM2;
		}
		if (aasworld->areasettings[areanum].areaflags & Q3AREA_BRIDGE)
		{
			tfl |= Q3TFL_BRIDGE;
		}
	}
	else
	{
		if (contents & WOLFAREACONTENTS_DONOTENTER_LARGE)
		{
			tfl |= WOLFTFL_DONOTENTER_LARGE;
		}
	}
	return tfl;
}

void AAS_InitAreaContentsTravelFlags()
{
	if (aasworld->areacontentstravelflags)
	{
		Mem_Free(aasworld->areacontentstravelflags);
	}
	aasworld->areacontentstravelflags = (int*)Mem_ClearedAlloc(aasworld->numareas * sizeof(int));

	for (int i = 0; i < aasworld->numareas; i++)
	{
		aasworld->areacontentstravelflags[i] = AAS_GetAreaContentsTravelFlags(i);
	}
}

int AAS_AreaContentsTravelFlags(int areanum)
{
	return aasworld->areacontentstravelflags[areanum];
}

void AAS_CreateReversedReachability()
{
#ifdef DEBUG
	int starttime = Sys_Milliseconds();
#endif
	//free reversed links that have already been created
	if (aasworld->reversedreachability)
	{
		Mem_Free(aasworld->reversedreachability);
	}
	//allocate memory for the reversed reachability links
	char* ptr = (char*)Mem_ClearedAlloc(aasworld->numareas * sizeof(aas_reversedreachability_t) +
		aasworld->reachabilitysize * sizeof(aas_reversedlink_t));
	//
	aasworld->reversedreachability = (aas_reversedreachability_t*)ptr;
	//pointer to the memory for the reversed links
	ptr += aasworld->numareas * sizeof(aas_reversedreachability_t);
	//check all reachabilities of all areas
	for (int i = 1; i < aasworld->numareas; i++)
	{
		//settings of the area
		aas_areasettings_t* settings = &aasworld->areasettings[i];

		if (GGameType & GAME_Quake3 && settings->numreachableareas >= 128)
		{
			BotImport_Print(PRT_WARNING, "area %d has more than 128 reachabilities\n", i);
		}
		//create reversed links for the reachabilities
		for (int n = 0; n < settings->numreachableareas && (!(GGameType & GAME_Quake3) || n < 128); n++)
		{
			// Gordon: Temp hack for b0rked last area in
			if (GGameType & GAME_ET && (settings->firstreachablearea < 0 || settings->firstreachablearea >= aasworld->reachabilitysize))
			{
				BotImport_Print(PRT_WARNING, "settings->firstreachablearea out of range\n");
				continue;
			}

			//reachability link
			aas_reachability_t* reach = &aasworld->reachability[settings->firstreachablearea + n];

			if (GGameType & GAME_ET && (reach->areanum < 0 || reach->areanum >= aasworld->reachabilitysize))
			{
				BotImport_Print(PRT_WARNING, "reach->areanum out of range\n");
				continue;
			}

			aas_reversedlink_t* revlink = (aas_reversedlink_t*)ptr;
			ptr += sizeof(aas_reversedlink_t);

			revlink->areanum = i;
			revlink->linknum = settings->firstreachablearea + n;
			revlink->next = aasworld->reversedreachability[reach->areanum].first;
			aasworld->reversedreachability[reach->areanum].first = revlink;
			aasworld->reversedreachability[reach->areanum].numlinks++;
		}
	}
#ifdef DEBUG
	BotImport_Print(PRT_MESSAGE, "reversed reachability %d msec\n", Sys_Milliseconds() - starttime);
#endif
}

float AAS_AreaGroundSteepnessScale(int areanum)
{
	return (1.0 + aasworld->areasettings[areanum].groundsteepness * (float)(GROUNDSTEEPNESS_TIMESCALE - 1));
}

static int AAS_PortalMaxTravelTime(int portalnum)
{
	aas_portal_t* portal = &aasworld->portals[portalnum];
	//reversed reachabilities of this portal area
	aas_reversedreachability_t* revreach = &aasworld->reversedreachability[portal->areanum];
	//settings of the portal area
	aas_areasettings_t* settings = &aasworld->areasettings[portal->areanum];

	int maxt = 0;
	for (int l = 0; l < settings->numreachableareas; l++)
	{
		aas_reversedlink_t* revlink = revreach->first;
		for (int n = 0; revlink; revlink = revlink->next, n++)
		{
			int t = aasworld->areatraveltimes[portal->areanum][l][n];
			if (t > maxt)
			{
				maxt = t;
			}
		}
	}
	return maxt;
}

void AAS_InitPortalMaxTravelTimes()
{
	if (aasworld->portalmaxtraveltimes)
	{
		Mem_Free(aasworld->portalmaxtraveltimes);
	}

	aasworld->portalmaxtraveltimes = (int*)Mem_ClearedAlloc(aasworld->numportals * sizeof(int));

	for (int i = 0; i < aasworld->numportals; i++)
	{
		aasworld->portalmaxtraveltimes[i] = AAS_PortalMaxTravelTime(i);
	}
}

bool AAS_FreeOldestCache()
{
	aas_routingcache_t* cache;
	for (cache = aasworld->oldestcache; cache; cache = cache->time_next)
	{
		// never free area cache leading towards a portal
		if (cache->type == CACHETYPE_AREA && aasworld->areasettings[cache->areanum].cluster < 0)
		{
			continue;
		}
		break;
	}
	if (cache)
	{
		// unlink the cache
		if (cache->type == CACHETYPE_AREA)
		{
			//number of the area in the cluster
			int clusterareanum = AAS_ClusterAreaNum(cache->cluster, cache->areanum);
			// unlink from cluster area cache
			if (cache->prev)
			{
				cache->prev->next = cache->next;
			}
			else
			{
				aasworld->clusterareacache[cache->cluster][clusterareanum] = cache->next;
			}
			if (cache->next)
			{
				cache->next->prev = cache->prev;
			}
		}
		else
		{
			// unlink from portal cache
			if (cache->prev)
			{
				cache->prev->next = cache->next;
			}
			else
			{
				aasworld->portalcache[cache->areanum] = cache->next;
			}
			if (cache->next)
			{
				cache->next->prev = cache->prev;
			}
		}
		AAS_FreeRoutingCache(cache);
		return true;
	}
	return false;
}

static void AAS_FreeAllClusterAreaCache()
{
	//free all cluster cache if existing
	if (!aasworld->clusterareacache)
	{
		return;
	}
	//free caches
	for (int i = 0; i < aasworld->numclusters; i++)
	{
		aas_cluster_t* cluster = &aasworld->clusters[i];
		for (int j = 0; j < cluster->numareas; j++)
		{
			aas_routingcache_t* nextcache;
			for (aas_routingcache_t* cache = aasworld->clusterareacache[i][j]; cache; cache = nextcache)
			{
				nextcache = cache->next;
				AAS_FreeRoutingCache(cache);
			}
			aasworld->clusterareacache[i][j] = NULL;
		}
	}
	//free the cluster cache array
	Mem_Free(aasworld->clusterareacache);
	aasworld->clusterareacache = NULL;
}

void AAS_InitClusterAreaCache()
{
	int size = 0;
	for (int i = 0; i < aasworld->numclusters; i++)
	{
		size += aasworld->clusters[i].numareas;
	}
	//two dimensional array with pointers for every cluster to routing cache
	//for every area in that cluster
	char* ptr = (char*)Mem_ClearedAlloc(
		aasworld->numclusters * sizeof(aas_routingcache_t * *) +
		size * sizeof(aas_routingcache_t*));
	aasworld->clusterareacache = (aas_routingcache_t***)ptr;
	ptr += aasworld->numclusters * sizeof(aas_routingcache_t * *);
	for (int i = 0; i < aasworld->numclusters; i++)
	{
		aasworld->clusterareacache[i] = (aas_routingcache_t**)ptr;
		ptr += aasworld->clusters[i].numareas * sizeof(aas_routingcache_t*);
	}
}

static void AAS_FreeAllPortalCache()
{
	//free all portal cache if existing
	if (!aasworld->portalcache)
	{
		return;
	}
	//free portal caches
	for (int i = 0; i < aasworld->numareas; i++)
	{
		aas_routingcache_t* nextcache;
		for (aas_routingcache_t* cache = aasworld->portalcache[i]; cache; cache = nextcache)
		{
			nextcache = cache->next;
			AAS_FreeRoutingCache(cache);
		}
		aasworld->portalcache[i] = NULL;
	}
	Mem_Free(aasworld->portalcache);
	aasworld->portalcache = NULL;
}

void AAS_InitPortalCache()
{
	aasworld->portalcache = (aas_routingcache_t**)Mem_ClearedAlloc(
		aasworld->numareas * sizeof(aas_routingcache_t*));
}

// run-length compression on zeros
int AAS_CompressVis(const byte* vis, int numareas, byte* dest)
{
	byte* dest_p = dest;

	for (int j = 0; j < numareas; j++)
	{
		*dest_p++ = vis[j];
		byte check = vis[j];

		int rep = 1;
		for (j++; j < numareas; j++)
		{
			if (vis[j] != check || rep == 255)
			{
				break;
			}
			else
			{
				rep++;
			}
		}
		*dest_p++ = rep;
		j--;
	}
	return dest_p - dest;
}

static void AAS_DecompressVis(const byte* in, int numareas, byte* decompressed)
{
	// initialize the vis data, only set those that are visible
	Com_Memset(decompressed, 0, numareas);

	byte* out = decompressed;
	byte* end = decompressed + numareas;

	do
	{
		byte c = in[1];
		if (!c)
		{
			AAS_Error("DecompressVis: 0 repeat");
		}
		if (*in)	// we need to set these bits
		{
			Com_Memset(out, 1, c);
		}
		in += 2;
		out += c;
	}
	while (out < end);
}

static void AAS_FreeAreaVisibility()
{
	if (aasworld->areavisibility)
	{
		for (int i = 0; i < aasworld->numareas; i++)
		{
			if (aasworld->areavisibility[i])
			{
				Mem_Free(aasworld->areavisibility[i]);
			}
		}
	}
	if (aasworld->areavisibility)
	{
		Mem_Free(aasworld->areavisibility);
	}
	aasworld->areavisibility = NULL;
	if (aasworld->decompressedvis)
	{
		Mem_Free(aasworld->decompressedvis);
	}
	aasworld->decompressedvis = NULL;
}

int AAS_AreaVisible(int srcarea, int destarea)
{
	if (GGameType & GAME_Quake3)
	{
		return false;
	}
	if (GGameType & GAME_ET && !aasworld->areavisibility)
	{
		return true;
	}
	if (srcarea != aasworld->decompressedvisarea)
	{
		if (!aasworld->areavisibility[srcarea])
		{
			return false;
		}
		AAS_DecompressVis(aasworld->areavisibility[srcarea],
			aasworld->numareas, aasworld->decompressedvis);
		aasworld->decompressedvisarea = srcarea;
	}
	return aasworld->decompressedvis[destarea];
}

void AAS_InitRoutingUpdate()
{
	//free routing update fields if already existing
	if (aasworld->areaupdate)
	{
		Mem_Free(aasworld->areaupdate);
	}

	if (GGameType & GAME_Quake3)
	{
		int maxreachabilityareas = 0;
		for (int i = 0; i < aasworld->numclusters; i++)
		{
			if (aasworld->clusters[i].numreachabilityareas > maxreachabilityareas)
			{
				maxreachabilityareas = aasworld->clusters[i].numreachabilityareas;
			}
		}
		//allocate memory for the routing update fields
		aasworld->areaupdate = (aas_routingupdate_t*)Mem_ClearedAlloc(
			maxreachabilityareas * sizeof(aas_routingupdate_t));
	}
	else
	{
		// Ridah, had to change it to numareas for hidepos checking
		aasworld->areaupdate = (aas_routingupdate_t*)Mem_ClearedAlloc(
			aasworld->numareas * sizeof(aas_routingupdate_t));
	}

	if (aasworld->portalupdate)
	{
		Mem_Free(aasworld->portalupdate);
	}
	//allocate memory for the portal update fields
	aasworld->portalupdate = (aas_routingupdate_t*)Mem_ClearedAlloc(
		(aasworld->numportals + 1) * sizeof(aas_routingupdate_t));
}

void AAS_WriteRouteCache()
{
	int numportalcache = 0;
	for (int i = 0; i < aasworld->numareas; i++)
	{
		for (aas_routingcache_t* cache = aasworld->portalcache[i]; cache; cache = cache->next)
		{
			numportalcache++;
		}
	}
	int numareacache = 0;
	for (int i = 0; i < aasworld->numclusters; i++)
	{
		aas_cluster_t* cluster = &aasworld->clusters[i];
		for (int j = 0; j < cluster->numareas; j++)
		{
			for (aas_routingcache_t* cache = aasworld->clusterareacache[i][j]; cache; cache = cache->next)
			{
				numareacache++;
			}
		}
	}
	// open the file for writing
	char filename[MAX_QPATH];
	String::Sprintf(filename, MAX_QPATH, "maps/%s.rcd", aasworld->mapname);
	fileHandle_t fp;
	FS_FOpenFileByMode(filename, &fp, FS_WRITE);
	if (!fp)
	{
		AAS_Error("Unable to open file: %s\n", filename);
		return;
	}

	//create the header
	if (GGameType & GAME_Quake3)
	{
		routecacheheader_q3_t routecacheheader;
		routecacheheader.ident = RCID;
		routecacheheader.version = RCVERSION_Q3;
		routecacheheader.numareas = aasworld->numareas;
		routecacheheader.numclusters = aasworld->numclusters;
		routecacheheader.areacrc = CRC_Block((unsigned char*)aasworld->areas, sizeof(aas_area_t) * aasworld->numareas);
		routecacheheader.clustercrc = CRC_Block((unsigned char*)aasworld->clusters, sizeof(aas_cluster_t) * aasworld->numclusters);
		routecacheheader.numportalcache = numportalcache;
		routecacheheader.numareacache = numareacache;
		//write the header
		FS_Write(&routecacheheader, sizeof(routecacheheader_q3_t), fp);
	}
	else
	{
		routecacheheader_wolf_t routecacheheader;
		routecacheheader.ident = RCID;
		if (GGameType & GAME_WolfSP)
		{
			routecacheheader.version = RCVERSION_WS;
		}
		else if (GGameType & GAME_WolfMP)
		{
			routecacheheader.version = RCVERSION_WM;
		}
		else
		{
			routecacheheader.version = RCVERSION_ET;
		}
		routecacheheader.numareas = aasworld->numareas;
		routecacheheader.numclusters = aasworld->numclusters;
		routecacheheader.areacrc = CRC_Block((unsigned char*)aasworld->areas, sizeof(aas_area_t) * aasworld->numareas);
		routecacheheader.clustercrc = CRC_Block((unsigned char*)aasworld->clusters, sizeof(aas_cluster_t) * aasworld->numclusters);
		routecacheheader.reachcrc = CRC_Block((unsigned char*)aasworld->reachability, sizeof(aas_reachability_t) * aasworld->reachabilitysize);
		routecacheheader.numportalcache = numportalcache;
		routecacheheader.numareacache = numareacache;
		//write the header
		FS_Write(&routecacheheader, sizeof(routecacheheader_wolf_t), fp);
	}

	//write all the cache
	for (int i = 0; i < aasworld->numareas; i++)
	{
		for (aas_routingcache_t* cache = aasworld->portalcache[i]; cache; cache = cache->next)
		{
			FS_Write(cache, cache->size, fp);
		}
	}
	for (int i = 0; i < aasworld->numclusters; i++)
	{
		aas_cluster_t* cluster = &aasworld->clusters[i];
		for (int j = 0; j < cluster->numareas; j++)
		{
			for (aas_routingcache_t* cache = aasworld->clusterareacache[i][j]; cache; cache = cache->next)
			{
				FS_Write(cache, cache->size, fp);
			}
		}
	}

	if (!(GGameType & GAME_Quake3))
	{
		// in case it ends up bigger than the decompressedvis, which is rare but possible
		byte* buf = (byte*)Mem_ClearedAlloc(aasworld->numareas * 2 * sizeof(byte));
		// write the visareas
		for (int i = 0; i < aasworld->numareas; i++)
		{
			if (!aasworld->areavisibility[i])
			{
				int size = 0;
				FS_Write(&size, sizeof(int), fp);
				continue;
			}
			AAS_DecompressVis(aasworld->areavisibility[i], aasworld->numareas, aasworld->decompressedvis);
			int size = AAS_CompressVis(aasworld->decompressedvis, aasworld->numareas, buf);
			FS_Write(&size, sizeof(int), fp);
			FS_Write(buf, size, fp);
		}
		Mem_Free(buf);
		// write the waypoints
		FS_Write(aasworld->areawaypoints, sizeof(vec3_t) * aasworld->numareas, fp);
	}

	FS_FCloseFile(fp);
	BotImport_Print(PRT_MESSAGE, "\nroute cache written to %s\n", filename);
}

static aas_routingcache_t* AAS_ReadCache(fileHandle_t fp)
{
	if (GGameType & GAME_WolfSP)
	{
		int i, size, realSize, numtraveltimes;
		aas_routingcache_t* cache;

		FS_Read(&size, sizeof(size), fp);
		size = LittleLong(size);
		numtraveltimes = (size - sizeof(aas_routingcache_f_t)) / 3;
		realSize = size - sizeof(aas_routingcache_f_t) + sizeof(aas_routingcache_t);
		cache = (aas_routingcache_t*)Mem_ClearedAlloc(realSize);
		cache->size = realSize;
		aas_routingcache_f_t* fcache = (aas_routingcache_f_t*)Mem_ClearedAlloc(size);
		FS_Read((unsigned char*)fcache + sizeof(size), size - sizeof(size), fp);

		//	Can't really use sizeof(aas_routingcache_f_t) - sizeof(short) because
		// size of aas_routingcache_f_t is 4 byte aligned.
		Com_Memcpy(cache->traveltimes, fcache->traveltimes, size - 48);

		cache->time = LittleFloat(fcache->time);
		cache->cluster = LittleLong(fcache->cluster);
		cache->areanum = LittleLong(fcache->areanum);
		cache->origin[0] = LittleFloat(fcache->origin[0]);
		cache->origin[1] = LittleFloat(fcache->origin[1]);
		cache->origin[2] = LittleFloat(fcache->origin[2]);
		cache->starttraveltime = LittleFloat(fcache->starttraveltime);
		cache->travelflags = LittleLong(fcache->travelflags);
		Mem_Free(fcache);

		//	The way pointer was assigned created 4 wasted bytes after traveltimes.
		// Can't use sizeof(aas_routingcache_t) because on 64 bit it's
		// 8 byte aligned.
		cache->reachabilities = (unsigned char*)cache->traveltimes + 4 + numtraveltimes * 2;

		//DAJ BUGFIX for missing byteswaps for traveltimes
		size = (size - sizeof(aas_routingcache_f_t)) / 3 + 1;
		for (i = 0; i < size; i++)
		{
			cache->traveltimes[i] = LittleShort(cache->traveltimes[i]);
		}
		AAS_LinkCache(cache);
		return cache;
	}
	else if (GGameType & GAME_WolfMP)
	{
		int size, i;
		aas_routingcache_t* cache;

		FS_Read(&size, sizeof(size), fp);
		cache = (aas_routingcache_t*)Mem_ClearedAlloc(size);
		cache->size = size;
		FS_Read((unsigned char*)cache + sizeof(size), size - sizeof(size), fp);
	//	cache->reachabilities = (unsigned char *) cache + sizeof(aas_routingcache_t) - sizeof(unsigned short) +
	//		(size - sizeof(aas_routingcache_t) + sizeof(unsigned short)) / 3 * 2;
		cache->reachabilities = (unsigned char*)cache + sizeof(aas_routingcache_t) +
								((size - sizeof(aas_routingcache_t)) / 3) * 2;

		//DAJ BUGFIX for missing byteswaps for traveltimes
		size = (size - sizeof(aas_routingcache_t)) / 3 + 1;
		for (i = 0; i < size; i++)
		{
			cache->traveltimes[i] = LittleShort(cache->traveltimes[i]);
		}
		return cache;
	}
	else if (GGameType & GAME_ET)
	{
		int size, i;
		aas_routingcache_t* cache;

		FS_Read(&size, sizeof(size), fp);
		size = LittleLong(size);
		cache = (aas_routingcache_t*)Mem_ClearedAlloc(size);
		cache->size = size;
		FS_Read((unsigned char*)cache + sizeof(size), size - sizeof(size), fp);

		if (1 != LittleLong(1))
		{
			cache->time = LittleFloat(cache->time);
			cache->cluster = LittleLong(cache->cluster);
			cache->areanum = LittleLong(cache->areanum);
			cache->origin[0] = LittleFloat(cache->origin[0]);
			cache->origin[1] = LittleFloat(cache->origin[1]);
			cache->origin[2] = LittleFloat(cache->origin[2]);
			cache->starttraveltime = LittleFloat(cache->starttraveltime);
			cache->travelflags = LittleLong(cache->travelflags);
		}

	//	cache->reachabilities = (unsigned char *) cache + sizeof(aas_routingcache_t) - sizeof(unsigned short) +
	//		(size - sizeof(aas_routingcache_t) + sizeof(unsigned short)) / 3 * 2;
		cache->reachabilities = (unsigned char*)cache + sizeof(aas_routingcache_t) +
								((size - sizeof(aas_routingcache_t)) / 3) * 2;

		//DAJ BUGFIX for missing byteswaps for traveltimes
		size = (size - sizeof(aas_routingcache_t)) / 3 + 1;
		for (i = 0; i < size; i++)
		{
			cache->traveltimes[i] = LittleShort(cache->traveltimes[i]);
		}
		return cache;
	}
	else
	{
		int size;
		aas_routingcache_t* cache;

		FS_Read(&size, sizeof(size), fp);
		cache = (aas_routingcache_t*)Mem_ClearedAlloc(size);
		cache->size = size;
		FS_Read((unsigned char*)cache + sizeof(size), size - sizeof(size), fp);
		cache->reachabilities = (unsigned char*)cache + sizeof(aas_routingcache_t) - sizeof(unsigned short) +
								(size - sizeof(aas_routingcache_t) + sizeof(unsigned short)) / 3 * 2;
		return cache;
	}
}

static bool ReadRouteCacheHeaderQ3(const char* filename, fileHandle_t fp, int& numportalcache, int& numareacache)
{
	routecacheheader_q3_t routecacheheader;
	FS_Read(&routecacheheader, sizeof(routecacheheader_q3_t), fp);
	if (routecacheheader.ident != RCID)
	{
		AAS_Error("%s is not a route cache dump\n", filename);
		return false;
	}
	if (routecacheheader.version != RCVERSION_Q3)
	{
		AAS_Error("route cache dump has wrong version %d, should be %d", routecacheheader.version, RCVERSION_Q3);
		return false;
	}
	if (routecacheheader.numareas != aasworld->numareas)
	{
		return false;
	}
	if (routecacheheader.numclusters != aasworld->numclusters)
	{
		return false;
	}
	if (routecacheheader.areacrc !=
		CRC_Block((unsigned char*)aasworld->areas, sizeof(aas_area_t) * aasworld->numareas))
	{
		return false;
	}
	if (routecacheheader.clustercrc !=
		CRC_Block((unsigned char*)aasworld->clusters, sizeof(aas_cluster_t) * aasworld->numclusters))
	{
		return false;
	}
	numportalcache = routecacheheader.numportalcache;
	numareacache = routecacheheader.numareacache;
	return true;
}

static bool ReadRouteCacheHeaderWolf(const char* filename, fileHandle_t fp, int& numportalcache, int& numareacache)
{
	routecacheheader_wolf_t routecacheheader;
	FS_Read(&routecacheheader, sizeof(routecacheheader_wolf_t), fp);

	if (GGameType & GAME_WolfSP)
	{
		// GJD: route cache data MUST be written on a PC because I've not altered the writing code.

		routecacheheader.areacrc = LittleLong(routecacheheader.areacrc);
		routecacheheader.clustercrc = LittleLong(routecacheheader.clustercrc);
		routecacheheader.ident = LittleLong(routecacheheader.ident);
		routecacheheader.numareacache = LittleLong(routecacheheader.numareacache);
		routecacheheader.numareas = LittleLong(routecacheheader.numareas);
		routecacheheader.numclusters = LittleLong(routecacheheader.numclusters);
		routecacheheader.numportalcache = LittleLong(routecacheheader.numportalcache);
		routecacheheader.reachcrc = LittleLong(routecacheheader.reachcrc);
		routecacheheader.version = LittleLong(routecacheheader.version);
	}

	if (routecacheheader.ident != RCID)
	{
		common->Printf("%s is not a route cache dump\n", filename);			// not an aas_error because we want to continue
		return false;												// and remake them by returning false here
	}
	if (GGameType & GAME_WolfSP && routecacheheader.version != RCVERSION_WS)
	{
		common->Printf("route cache dump has wrong version %d, should be %d", routecacheheader.version, RCVERSION_WS);
		return false;
	}
	if (GGameType & GAME_WolfMP && routecacheheader.version != RCVERSION_WM)
	{
		common->Printf("route cache dump has wrong version %d, should be %d", routecacheheader.version, RCVERSION_WM);
		return false;
	}
	if (GGameType & GAME_ET && routecacheheader.version != RCVERSION_ET)
	{
		common->Printf("route cache dump has wrong version %d, should be %d", routecacheheader.version, RCVERSION_ET);
		return false;
	}
	if (routecacheheader.numareas != aasworld->numareas)
	{
		return false;
	}
	if (routecacheheader.numclusters != aasworld->numclusters)
	{
		return false;
	}
	// the crc table stuff is endian orientated....
	if (LittleLong(1) == 1)
	{
		if (routecacheheader.areacrc !=
			CRC_Block((unsigned char*)aasworld->areas, sizeof(aas_area_t) * aasworld->numareas))
		{
			return false;
		}
		if (routecacheheader.clustercrc !=
			CRC_Block((unsigned char*)aasworld->clusters, sizeof(aas_cluster_t) * aasworld->numclusters))
		{
			return false;
		}
		if (routecacheheader.reachcrc !=
			CRC_Block((unsigned char*)aasworld->reachability, sizeof(aas_reachability_t) * aasworld->reachabilitysize))
		{
			return false;
		}
	}
	numportalcache = routecacheheader.numportalcache;
	numareacache = routecacheheader.numareacache;
	return true;
}

int AAS_ReadRouteCache()
{
	int i, clusterareanum, size;
	fileHandle_t fp;
	char filename[MAX_QPATH];
	aas_routingcache_t* cache;

	String::Sprintf(filename, MAX_QPATH, "maps/%s.rcd", aasworld->mapname);
	FS_FOpenFileByMode(filename, &fp, FS_READ);
	if (!fp)
	{
		return false;
	}	//end if
	int numportalcache;
	int numareacache;
	if (GGameType & GAME_Quake3)
	{
		if (!ReadRouteCacheHeaderQ3(filename, fp, numportalcache, numareacache))
		{
			FS_FCloseFile(fp);
			return false;
		}
	}
	else
	{
		if (!ReadRouteCacheHeaderWolf(filename, fp, numportalcache, numareacache))
		{
			FS_FCloseFile(fp);
			return false;
		}
	}
	//read all the portal cache
	for (i = 0; i < numportalcache; i++)
	{
		cache = AAS_ReadCache(fp);
		cache->next = aasworld->portalcache[cache->areanum];
		cache->prev = NULL;
		if (aasworld->portalcache[cache->areanum])
		{
			aasworld->portalcache[cache->areanum]->prev = cache;
		}
		aasworld->portalcache[cache->areanum] = cache;
	}	//end for
		//read all the cluster area cache
	for (i = 0; i < numareacache; i++)
	{
		cache = AAS_ReadCache(fp);
		clusterareanum = AAS_ClusterAreaNum(cache->cluster, cache->areanum);
		cache->next = aasworld->clusterareacache[cache->cluster][clusterareanum];
		cache->prev = NULL;
		if (aasworld->clusterareacache[cache->cluster][clusterareanum])
		{
			aasworld->clusterareacache[cache->cluster][clusterareanum]->prev = cache;
		}
		aasworld->clusterareacache[cache->cluster][clusterareanum] = cache;
	}	//end for

	if (!(GGameType & GAME_Quake3))
	{
		// read the visareas
		aasworld->areavisibility = (byte**)Mem_ClearedAlloc(aasworld->numareas * sizeof(byte*));
		aasworld->decompressedvis = (byte*)Mem_ClearedAlloc(aasworld->numareas * sizeof(byte));
		for (i = 0; i < aasworld->numareas; i++)
		{
			FS_Read(&size, sizeof(size), fp);
			if (GGameType & GAME_WolfSP)
			{
				size = LittleLong(size);
			}
			if (size)
			{
				aasworld->areavisibility[i] = (byte*)Mem_Alloc(size);
				FS_Read(aasworld->areavisibility[i], size, fp);
			}
		}
		// read the area waypoints
		aasworld->areawaypoints = (vec3_t*)Mem_ClearedAlloc(aasworld->numareas * sizeof(vec3_t));
		FS_Read(aasworld->areawaypoints, aasworld->numareas * sizeof(vec3_t), fp);
		if (GGameType & GAME_WolfSP && 1 != LittleLong(1))
		{
			for (i = 0; i < aasworld->numareas; i++)
			{
				aasworld->areawaypoints[i][0] = LittleFloat(aasworld->areawaypoints[i][0]);
				aasworld->areawaypoints[i][1] = LittleFloat(aasworld->areawaypoints[i][1]);
				aasworld->areawaypoints[i][2] = LittleFloat(aasworld->areawaypoints[i][2]);
			}
		}
	}

	FS_FCloseFile(fp);
	return true;
}

void AAS_FreeRoutingCaches()
{
	// free all the existing cluster area cache
	AAS_FreeAllClusterAreaCache();
	// free all the existing portal cache
	AAS_FreeAllPortalCache();
	// free all the existing area visibility data
	AAS_FreeAreaVisibility();
	// free cached travel times within areas
	if (aasworld->areatraveltimes)
	{
		Mem_Free(aasworld->areatraveltimes);
	}
	aasworld->areatraveltimes = NULL;
	// free cached maximum travel time through cluster portals
	if (aasworld->portalmaxtraveltimes)
	{
		Mem_Free(aasworld->portalmaxtraveltimes);
	}
	aasworld->portalmaxtraveltimes = NULL;
	// free reversed reachability links
	if (aasworld->reversedreachability)
	{
		Mem_Free(aasworld->reversedreachability);
	}
	aasworld->reversedreachability = NULL;
	// free routing algorithm memory
	if (aasworld->areaupdate)
	{
		Mem_Free(aasworld->areaupdate);
	}
	aasworld->areaupdate = NULL;
	if (aasworld->portalupdate)
	{
		Mem_Free(aasworld->portalupdate);
	}
	aasworld->portalupdate = NULL;
	// free lists with areas the reachabilities go through
	if (aasworld->reachabilityareas)
	{
		Mem_Free(aasworld->reachabilityareas);
	}
	aasworld->reachabilityareas = NULL;
	// free the reachability area index
	if (aasworld->reachabilityareaindex)
	{
		Mem_Free(aasworld->reachabilityareaindex);
	}
	aasworld->reachabilityareaindex = NULL;
	// free area contents travel flags look up table
	if (aasworld->areacontentstravelflags)
	{
		Mem_Free(aasworld->areacontentstravelflags);
	}
	aasworld->areacontentstravelflags = NULL;
	// free area waypoints
	if (aasworld->areawaypoints)
	{
		Mem_Free(aasworld->areawaypoints);
	}
	aasworld->areawaypoints = NULL;
}

// update the given routing cache
static void AAS_UpdateAreaRoutingCache(aas_routingcache_t* areacache)
{
#ifdef ROUTING_DEBUG
	numareacacheupdates++;
#endif
	//number of reachability areas within this cluster
	int numreachabilityareas = aasworld->clusters[areacache->cluster].numreachabilityareas;

	if (GGameType & GAME_Quake3)
	{
		aasworld->frameroutingupdates++;
	}

	int badtravelflags = ~areacache->travelflags;

	int clusterareanum = AAS_ClusterAreaNum(areacache->cluster, areacache->areanum);
	if (clusterareanum >= numreachabilityareas)
	{
		return;
	}

	unsigned short int startareatraveltimes[128];//NOTE: not more than 128 reachabilities per area allowed
	Com_Memset(startareatraveltimes, 0, sizeof(startareatraveltimes));

	aas_routingupdate_t* curupdate = &aasworld->areaupdate[clusterareanum];
	curupdate->areanum = areacache->areanum;
	if (GGameType & GAME_Quake3)
	{
		curupdate->areatraveltimes = startareatraveltimes;
	}
	else
	{
		curupdate->areatraveltimes = aasworld->areatraveltimes[areacache->areanum][0];
	}
	curupdate->tmptraveltime = areacache->starttraveltime;

	areacache->traveltimes[clusterareanum] = areacache->starttraveltime;
	//put the area to start with in the current read list
	curupdate->next = NULL;
	curupdate->prev = NULL;
	aas_routingupdate_t* updateliststart = curupdate;
	aas_routingupdate_t* updatelistend = curupdate;
	//while there are updates in the current list
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

		curupdate->inlist = false;
		//check all reversed reachability links
		aas_reversedreachability_t* revreach = &aasworld->reversedreachability[curupdate->areanum];

		aas_reversedlink_t* revlink = revreach->first;
		for (int i = 0; revlink; revlink = revlink->next, i++)
		{
			int linknum = revlink->linknum;
			aas_reachability_t* reach = &aasworld->reachability[linknum];
			//if there is used an undesired travel type
			if (AAS_TravelFlagForType(reach->traveltype) & badtravelflags)
			{
				continue;
			}
			//if not allowed to enter the next area
			if (aasworld->areasettings[reach->areanum].areaflags & AREA_DISABLED)
			{
				continue;
			}
			//if the next area has a not allowed travel flag
			if (AAS_AreaContentsTravelFlags(reach->areanum) & badtravelflags)
			{
				continue;
			}
			//number of the area the reversed reachability leads to
			int nextareanum = revlink->areanum;
			//get the cluster number of the area
			int cluster = aasworld->areasettings[nextareanum].cluster;
			//don't leave the cluster
			if (cluster > 0 && cluster != areacache->cluster)
			{
				continue;
			}
			//get the number of the area in the cluster
			clusterareanum = AAS_ClusterAreaNum(areacache->cluster, nextareanum);
			if (clusterareanum >= numreachabilityareas)
			{
				continue;
			}
			//time already travelled plus the traveltime through
			//the current area plus the travel time from the reachability
			unsigned short int t = curupdate->tmptraveltime +
				curupdate->areatraveltimes[i] +
				reach->traveltime;
			if (GGameType & GAME_ET)
			{
				//if trying to avoid this area
				if (aasworld->areasettings[reach->areanum].areaflags & ETAREA_AVOID)
				{
					t += 1000;
				}
				else if ((aasworld->areasettings[reach->areanum].areaflags & ETAREA_AVOID_AXIS) && (areacache->travelflags & ETTFL_TEAM_AXIS))
				{
					t += 200;	// + (curupdate->areatraveltimes[i] + reach->traveltime) * 30;
				}
				else if ((aasworld->areasettings[reach->areanum].areaflags & ETAREA_AVOID_ALLIES) && (areacache->travelflags & ETTFL_TEAM_ALLIES))
				{
					t += 200;	// + (curupdate->areatraveltimes[i] + reach->traveltime) * 30;
				}
			}

			if (!(GGameType & GAME_Quake3))
			{
				aasworld->frameroutingupdates++;
			}

			if ((!(GGameType & GAME_ET) || aasworld->areatraveltimes[nextareanum]) &&
				(!areacache->traveltimes[clusterareanum] ||
				areacache->traveltimes[clusterareanum] > t))
			{
				areacache->traveltimes[clusterareanum] = t;
				areacache->reachabilities[clusterareanum] = linknum - aasworld->areasettings[nextareanum].firstreachablearea;
				aas_routingupdate_t* nextupdate = &aasworld->areaupdate[clusterareanum];
				nextupdate->areanum = nextareanum;
				nextupdate->tmptraveltime = t;
				nextupdate->areatraveltimes = aasworld->areatraveltimes[nextareanum][linknum -
					aasworld->areasettings[nextareanum].firstreachablearea];
				if (!nextupdate->inlist)
				{
					// we add the update to the end of the list
					// we could also use a B+ tree to have a real sorted list
					// on travel time which makes for faster routing updates
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
					nextupdate->inlist = true;
				}
			}
		}
	}
}

aas_routingcache_t* AAS_GetAreaRoutingCache(int clusternum, int areanum, int travelflags, bool forceUpdate)
{
	int clusterareanum;
	aas_routingcache_t* cache, * clustercache;

	//number of the area in the cluster
	clusterareanum = AAS_ClusterAreaNum(clusternum, areanum);
	//pointer to the cache for the area in the cluster
	clustercache = aasworld->clusterareacache[clusternum][clusterareanum];
	if (GGameType & GAME_ET)
	{
		// RF, remove team-specific flags which don't exist in this cluster
		travelflags &= ~ETTFL_TEAM_FLAGS | aasworld->clusterTeamTravelFlags[clusternum];
	}
	//find the cache without undesired travel flags
	for (cache = clustercache; cache; cache = cache->next)
	{
		//if there aren't used any undesired travel types for the cache
		if (cache->travelflags == travelflags)
		{
			break;
		}
	}
	//if there was no cache
	if (!cache)
	{
		//NOTE: the number of routing updates is limited per frame
		if (!forceUpdate && (aasworld->frameroutingupdates > max_frameroutingupdates))
		{
			return NULL;
		}

		cache = AAS_AllocRoutingCache(aasworld->clusters[clusternum].numreachabilityareas);
		cache->cluster = clusternum;
		cache->areanum = areanum;
		VectorCopy(aasworld->areas[areanum].center, cache->origin);
		cache->starttraveltime = 1;
		cache->travelflags = travelflags;
		cache->prev = NULL;
		cache->next = clustercache;
		if (clustercache)
		{
			clustercache->prev = cache;
		}
		aasworld->clusterareacache[clusternum][clusterareanum] = cache;
		AAS_UpdateAreaRoutingCache(cache);
	}
	else
	{
		AAS_UnlinkCache(cache);
	}
	//the cache has been accessed
	cache->time = AAS_RoutingTime();
	cache->type = CACHETYPE_AREA;
	AAS_LinkCache(cache);
	return cache;
}

static void AAS_UpdatePortalRoutingCache(aas_routingcache_t* portalcache)
{
#ifdef ROUTING_DEBUG
	numportalcacheupdates++;
#endif

	aas_routingupdate_t* curupdate = &aasworld->portalupdate[aasworld->numportals];
	curupdate->cluster = portalcache->cluster;
	curupdate->areanum = portalcache->areanum;
	curupdate->tmptraveltime = portalcache->starttraveltime;
	if (!(GGameType & GAME_ET))
	{
		//if the start area is a cluster portal, store the travel time for that portal
		int clusternum = aasworld->areasettings[portalcache->areanum].cluster;
		if (clusternum < 0)
		{
			portalcache->traveltimes[-clusternum] = portalcache->starttraveltime;
		}
	}
	//put the area to start with in the current read list
	curupdate->next = NULL;
	curupdate->prev = NULL;
	aas_routingupdate_t* updateliststart = curupdate;
	aas_routingupdate_t* updatelistend = curupdate;
	//while there are updates in the current list
	while (updateliststart)
	{
		curupdate = updateliststart;
		//remove the current update from the list
		if (curupdate->next)
		{
			curupdate->next->prev = NULL;
		}
		else
		{
			updatelistend = NULL;
		}
		updateliststart = curupdate->next;
		//current update is removed from the list
		curupdate->inlist = false;

		aas_cluster_t* cluster = &aasworld->clusters[curupdate->cluster];

		aas_routingcache_t* cache = AAS_GetAreaRoutingCache(curupdate->cluster,
			curupdate->areanum, portalcache->travelflags, true);
		//take all portals of the cluster
		for (int i = 0; i < cluster->numportals; i++)
		{
			int portalnum = aasworld->portalindex[cluster->firstportal + i];
			aas_portal_t* portal = &aasworld->portals[portalnum];
			//if this is the portal of the current update continue
			if (!(GGameType & GAME_ET) && portal->areanum == curupdate->areanum)
			{
				continue;
			}

			int clusterareanum = AAS_ClusterAreaNum(curupdate->cluster, portal->areanum);
			if (clusterareanum >= cluster->numreachabilityareas)
			{
				continue;
			}

			unsigned short int t = cache->traveltimes[clusterareanum];
			if (!t)
			{
				continue;
			}
			t += curupdate->tmptraveltime;

			if (!portalcache->traveltimes[portalnum] ||
				portalcache->traveltimes[portalnum] > t)
			{
				portalcache->traveltimes[portalnum] = t;
				if (!(GGameType & GAME_Quake3))
				{
					portalcache->reachabilities[portalnum] = cache->reachabilities[clusterareanum];
				}
				aas_routingupdate_t* nextupdate = &aasworld->portalupdate[portalnum];
				if (portal->frontcluster == curupdate->cluster)
				{
					nextupdate->cluster = portal->backcluster;
				}
				else
				{
					nextupdate->cluster = portal->frontcluster;
				}
				nextupdate->areanum = portal->areanum;
				//add travel time through the actual portal area for the next update
				nextupdate->tmptraveltime = t + aasworld->portalmaxtraveltimes[portalnum];
				if (!nextupdate->inlist)
				{
					// we add the update to the end of the list
					// we could also use a B+ tree to have a real sorted list
					// on travel time which makes for faster routing updates
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
					nextupdate->inlist = true;
				}
			}
		}
	}
}

aas_routingcache_t* AAS_GetPortalRoutingCache(int clusternum, int areanum, int travelflags)
{
	//find the cached portal routing if existing
	aas_routingcache_t* cache;
	for (cache = aasworld->portalcache[areanum]; cache; cache = cache->next)
	{
		if (cache->travelflags == travelflags)
		{
			break;
		}
	}
	//if the portal routing isn't cached
	if (!cache)
	{
		cache = AAS_AllocRoutingCache(aasworld->numportals);
		cache->cluster = clusternum;
		cache->areanum = areanum;
		VectorCopy(aasworld->areas[areanum].center, cache->origin);
		cache->starttraveltime = 1;
		cache->travelflags = travelflags;
		//add the cache to the cache list
		cache->prev = NULL;
		cache->next = aasworld->portalcache[areanum];
		if (aasworld->portalcache[areanum])
		{
			aasworld->portalcache[areanum]->prev = cache;
		}
		aasworld->portalcache[areanum] = cache;
		//update the cache
		AAS_UpdatePortalRoutingCache(cache);
	}
	else
	{
		AAS_UnlinkCache(cache);
	}
	//the cache has been accessed
	cache->time = AAS_RoutingTime();
	cache->type = CACHETYPE_PORTAL;
	AAS_LinkCache(cache);
	return cache;
}

void AAS_ReachabilityFromNum(int num, aas_reachability_t* reach)
{
	if (!aasworld->initialized)
	{
		Com_Memset(reach, 0, sizeof(aas_reachability_t));
		return;
	}
	if (num < 0 || num >= aasworld->reachabilitysize)
	{
		Com_Memset(reach, 0, sizeof(aas_reachability_t));
		return;
	}
	Com_Memcpy(reach, &aasworld->reachability[num], sizeof(aas_reachability_t));;
}

int AAS_NextAreaReachability(int areanum, int reachnum)
{
	if (!aasworld->initialized)
	{
		return 0;
	}

	if (areanum <= 0 || areanum >= aasworld->numareas)
	{
		BotImport_Print(PRT_ERROR, "AAS_NextAreaReachability: areanum %d out of range\n", areanum);
		return 0;
	}

	aas_areasettings_t* settings = &aasworld->areasettings[areanum];
	if (!reachnum)
	{
		return settings->firstreachablearea;
	}
	if (reachnum < settings->firstreachablearea)
	{
		BotImport_Print(PRT_FATAL, "AAS_NextAreaReachability: reachnum < settings->firstreachableara");
		return 0;
	}
	reachnum++;
	if (reachnum >= settings->firstreachablearea + settings->numreachableareas)
	{
		return 0;
	}
	return reachnum;
}

int AAS_NextModelReachability(int num, int modelnum)
{
	if (num <= 0)
	{
		num = 1;
	}
	else if (num >= aasworld->reachabilitysize)
	{
		return 0;
	}
	else
	{
		num++;
	}

	for (int i = num; i < aasworld->reachabilitysize; i++)
	{
		if ((aasworld->reachability[i].traveltype & TRAVELTYPE_MASK) == TRAVEL_ELEVATOR)
		{
			if (aasworld->reachability[i].facenum == modelnum)
			{
				return i;
			}
		}
		else if ((aasworld->reachability[i].traveltype & TRAVELTYPE_MASK) == TRAVEL_FUNCBOB)
		{
			if ((aasworld->reachability[i].facenum & 0x0000FFFF) == modelnum)
			{
				return i;
			}
		}
	}
	return 0;
}