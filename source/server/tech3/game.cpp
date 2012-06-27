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

bool SVT3_EntityContact(const vec3_t mins, const vec3_t maxs, const idEntity3* ent, int capsule)
{
	// check for exact collision
	const float* origin = ent->GetCurrentOrigin();
	const float* angles = ent->GetCurrentAngles();

	clipHandle_t ch = SVT3_ClipHandleForEntity(ent);
	q3trace_t trace;
	CM_TransformedBoxTraceQ3(&trace, vec3_origin, vec3_origin, mins, maxs,
		ch, -1, origin, angles, capsule);

	return trace.startsolid;
}

//	sets mins and maxs for inline bmodels
void SVT3_SetBrushModel(idEntity3* ent, q3svEntity_t* svEnt, const char* name)
{
	if (!name)
	{
		common->Error("SV_SetBrushModel: NULL");
	}

	if (name[0] != '*')
	{
		common->Error("SV_SetBrushModel: %s isn't a brush model", name);
	}


	ent->SetModelIndex(String::Atoi(name + 1));

	clipHandle_t h = CM_InlineModel(ent->GetModelIndex());
	vec3_t mins, maxs;
	CM_ModelBounds(h, mins, maxs);
	ent->SetMins(mins);
	ent->SetMaxs(maxs);
	ent->SetBModel(true);

	// we don't know exactly what is in the brushes
	ent->SetContents(-1);

	SVT3_LinkEntity(ent, svEnt);
}

void SVT3_GetServerinfo(char* buffer, int bufferSize)
{
	if (bufferSize < 1)
	{
		common->Error("SVT3_GetServerinfo: bufferSize == %i", bufferSize);
	}
	String::NCpyZ(buffer, Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE, MAX_INFO_STRING_Q3), bufferSize);
}

void SVT3_AdjustAreaPortalState(q3svEntity_t* svEnt, bool open)
{
	if (svEnt->areanum2 == -1)
	{
		return;
	}
	CM_AdjustAreaPortalState(svEnt->areanum, svEnt->areanum2, open);
}

bool SVT3_GetEntityToken(char* buffer, int length)
{
	const char* s = String::Parse3(&sv.q3_entityParsePoint);
	String::NCpyZ(buffer, s, length);
	if (!sv.q3_entityParsePoint && !s[0])
	{
		return false;
	}
	else
	{
		return true;
	}
}
