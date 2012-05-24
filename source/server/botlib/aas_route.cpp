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

/*__inline*/ float AAS_RoutingTime()
{
	return AAS_Time();
}

void AAS_LinkCache(aas_routingcache_t* cache)
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

void AAS_UnlinkCache(aas_routingcache_t* cache)
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
