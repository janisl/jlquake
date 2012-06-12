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
#include "../quake3/local.h"
#include "../wolfsp//local.h"
#include "../wolfmp//local.h"
#include "../et/local.h"

vm_t* gvm = NULL;							// game virtual machine

idEntity3* SVT3_EntityNum(int number)
{
	qassert(number >= 0);
	qassert(number < MAX_GENTITIES_Q3);
	qassert(sv.q3_entities[number]);
	return sv.q3_entities[number];
}

idEntity3* SVT3_EntityForSvEntity(const q3svEntity_t* svEnt)
{
	int num = svEnt - sv.q3_svEntities;
	return SVT3_EntityNum(num);
}

//	Also checks portalareas so that doors block sight
bool SVT3_inPVS(const vec3_t p1, const vec3_t p2)
{
	int leafnum = CM_PointLeafnum(p1);
	int cluster = CM_LeafCluster(leafnum);
	int area1 = CM_LeafArea(leafnum);
	byte* mask = CM_ClusterPVS(cluster);

	leafnum = CM_PointLeafnum(p2);
	cluster = CM_LeafCluster(leafnum);
	int area2 = CM_LeafArea(leafnum);

	if (mask && (!(mask[cluster >> 3] & (1 << (cluster & 7)))))
	{
		return false;
	}
	if (!CM_AreasConnected(area1, area2))
	{
		return false;		// a door blocks sight
	}
	return true;
}

bool SVT3_inPVSIgnorePortals(const vec3_t p1, const vec3_t p2)
{
	int leafnum = CM_PointLeafnum(p1);
	int cluster = CM_LeafCluster(leafnum);
	byte* mask = CM_ClusterPVS(cluster);

	leafnum = CM_PointLeafnum(p2);
	cluster = CM_LeafCluster(leafnum);

	if (mask && (!(mask[cluster >> 3] & (1 << (cluster & 7)))))
	{
		return false;
	}
	return true;
}

bool SVT3_BotVisibleFromPos(vec3_t srcpos, int srcnum, vec3_t destpos, int destnum, bool updateVisPos)
{
	if (GGameType & GAME_WolfSP)
	{
		return SVWS_BotVisibleFromPos(srcpos, srcnum, destpos, destnum, updateVisPos);
	}
	if (GGameType & GAME_WolfMP)
	{
		return SVWM_BotVisibleFromPos(srcpos, srcnum, destpos, destnum, updateVisPos);
	}
	if (GGameType & GAME_ET)
	{
		return SVET_BotVisibleFromPos(srcpos, srcnum, destpos, destnum, updateVisPos);
	}
	return false;
}

bool SVT3_BotCheckAttackAtPos(int entnum, int enemy, vec3_t pos, bool ducking, bool allowHitWorld)
{
	if (GGameType & GAME_WolfSP)
	{
		return SVWS_BotCheckAttackAtPos(entnum, enemy, pos, ducking, allowHitWorld);
	}
	if (GGameType & GAME_WolfMP)
	{
		return SVWM_BotCheckAttackAtPos(entnum, enemy, pos, ducking, allowHitWorld);
	}
	if (GGameType & GAME_ET)
	{
		return SVET_BotCheckAttackAtPos(entnum, enemy, pos, ducking, allowHitWorld);
	}
	return false;
}
