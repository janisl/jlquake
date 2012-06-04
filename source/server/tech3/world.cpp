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

/*
===============================================================================

ENTITY CHECKING

To avoid linearly searching through lists of entities during environment testing,
the world is carved up with an evenly spaced, axially aligned bsp tree.  Entities
are kept in chains either at the final leafs, or at the first node that splits
them, which prevents having to deal with multiple fragments of a single entity.

===============================================================================
*/

void SVT3_SectorList_f()
{
	for (int i = 0; i < AREA_NODES; i++)
	{
		worldSector_t* sec = &sv_worldSectors[i];

		int c = 0;
		for (q3svEntity_t* ent = sec->entities; ent; ent = ent->nextEntityInWorldSector)
		{
			c++;
		}
		common->Printf("sector %i: %i entities\n", i, c);
	}
}

void SVT3_UnlinkSvEntity(q3svEntity_t* ent)
{
	q3svEntity_t* scan;
	worldSector_t* ws;

	ws = ent->worldSector;
	if (!ws)
	{
		return;		// not linked in anywhere
	}
	ent->worldSector = NULL;

	if (ws->entities == ent)
	{
		ws->entities = ent->nextEntityInWorldSector;
		return;
	}

	for (scan = ws->entities; scan; scan = scan->nextEntityInWorldSector)
	{
		if (scan->nextEntityInWorldSector == ent)
		{
			scan->nextEntityInWorldSector = ent->nextEntityInWorldSector;
			return;
		}
	}

	common->Printf("WARNING: SV_UnlinkEntity: not found in worldSector\n");
}

/*
============================================================================

AREA QUERY

Fills in a list of all entities who's absmin / absmax intersects the given
bounds.  This does NOT mean that they actually touch in the case of bmodels.
============================================================================
*/

struct areaParms_t
{
	const float* mins;
	const float* maxs;
	int* list;
	int count;
	int maxcount;
};

static void SVT3_AreaEntities_r(worldSector_t* node, areaParms_t* ap)
{
	q3svEntity_t* check, * next;
	int count;

	count = 0;

	for (check = node->entities; check; check = next)
	{
		next = check->nextEntityInWorldSector;

		if (GGameType & GAME_WolfSP)
		{
			wssharedEntity_t* gcheck = SVWS_GEntityForSvEntity(check);

			if (gcheck->r.absmin[0] > ap->maxs[0] ||
				gcheck->r.absmin[1] > ap->maxs[1] ||
				gcheck->r.absmin[2] > ap->maxs[2] ||
				gcheck->r.absmax[0] < ap->mins[0] ||
				gcheck->r.absmax[1] < ap->mins[1] ||
				gcheck->r.absmax[2] < ap->mins[2])
			{
				continue;
			}
		}
		else if (GGameType & GAME_WolfMP)
		{
			wmsharedEntity_t* gcheck = SVWM_GEntityForSvEntity(check);

			if (gcheck->r.absmin[0] > ap->maxs[0] ||
				gcheck->r.absmin[1] > ap->maxs[1] ||
				gcheck->r.absmin[2] > ap->maxs[2] ||
				gcheck->r.absmax[0] < ap->mins[0] ||
				gcheck->r.absmax[1] < ap->mins[1] ||
				gcheck->r.absmax[2] < ap->mins[2])
			{
				continue;
			}
		}
		else if (GGameType & GAME_ET)
		{
			etsharedEntity_t* gcheck = SVET_GEntityForSvEntity(check);

			if (gcheck->r.absmin[0] > ap->maxs[0] ||
				gcheck->r.absmin[1] > ap->maxs[1] ||
				gcheck->r.absmin[2] > ap->maxs[2] ||
				gcheck->r.absmax[0] < ap->mins[0] ||
				gcheck->r.absmax[1] < ap->mins[1] ||
				gcheck->r.absmax[2] < ap->mins[2])
			{
				continue;
			}
		}
		else
		{
			q3sharedEntity_t* gcheck = SVQ3_GEntityForSvEntity(check);

			if (gcheck->r.absmin[0] > ap->maxs[0] ||
				gcheck->r.absmin[1] > ap->maxs[1] ||
				gcheck->r.absmin[2] > ap->maxs[2] ||
				gcheck->r.absmax[0] < ap->mins[0] ||
				gcheck->r.absmax[1] < ap->mins[1] ||
				gcheck->r.absmax[2] < ap->mins[2])
			{
				continue;
			}
		}

		if (ap->count == ap->maxcount)
		{
			common->Printf("SV_AreaEntities: MAXCOUNT\n");
			return;
		}

		ap->list[ap->count] = check - sv.q3_svEntities;
		ap->count++;
	}

	if (node->axis == -1)
	{
		return;		// terminal node
	}

	// recurse down both sides
	if (ap->maxs[node->axis] > node->dist)
	{
		SVT3_AreaEntities_r(node->children[0], ap);
	}
	if (ap->mins[node->axis] < node->dist)
	{
		SVT3_AreaEntities_r(node->children[1], ap);
	}
}

int SVT3_AreaEntities(const vec3_t mins, const vec3_t maxs, int* entityList, int maxcount)
{
	areaParms_t ap;

	ap.mins = mins;
	ap.maxs = maxs;
	ap.list = entityList;
	ap.count = 0;
	ap.maxcount = maxcount;

	SVT3_AreaEntities_r(sv_worldSectors, &ap);

	return ap.count;
}

void SVT3_ClipToEntity(q3trace_t* trace, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int entityNum, int contentmask, int capsule)
{
	if (GGameType & GAME_WolfSP)
	{
		SVWS_ClipToEntity(trace, start, mins, maxs, end, entityNum, contentmask, capsule);
	}
	else if (GGameType & GAME_WolfMP)
	{
		SVWM_ClipToEntity(trace, start, mins, maxs, end, entityNum, contentmask, capsule);
	}
	else if (GGameType & GAME_ET)
	{
		SVET_ClipToEntity(trace, start, mins, maxs, end, entityNum, contentmask, capsule);
	}
	else
	{
		SVQ3_ClipToEntity(trace, start, mins, maxs, end, entityNum, contentmask, capsule);
	}
}

//	Moves the given mins/maxs volume through the world from start to end.
// passEntityNum and entities owned by passEntityNum are explicitly not checked.
void SVT3_Trace(q3trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
	const vec3_t end, int passEntityNum, int contentmask, int capsule)
{
	q3moveclip_t clip;
	int i;

	if (!mins)
	{
		mins = vec3_origin;
	}
	if (!maxs)
	{
		maxs = vec3_origin;
	}

	Com_Memset(&clip, 0, sizeof(q3moveclip_t));

	// clip to world
	CM_BoxTraceQ3(&clip.trace, start, end, mins, maxs, 0, contentmask, capsule);
	clip.trace.entityNum = clip.trace.fraction != 1.0 ? Q3ENTITYNUM_WORLD : Q3ENTITYNUM_NONE;
	if (clip.trace.fraction == 0 || (GGameType & GAME_ET && passEntityNum == -2))
	{
		*results = clip.trace;
		return;		// blocked immediately by the world
	}

	clip.contentmask = contentmask;
	clip.start = start;
	VectorCopy(end, clip.end);
	clip.mins = mins;
	clip.maxs = maxs;
	clip.passEntityNum = passEntityNum;
	clip.capsule = capsule;

	// create the bounding box of the entire move
	// we can limit it to the part of the move not
	// already clipped off by the world, which can be
	// a significant savings for line of sight and shot traces
	for (i = 0; i < 3; i++)
	{
		if (end[i] > start[i])
		{
			clip.boxmins[i] = clip.start[i] + clip.mins[i] - 1;
			clip.boxmaxs[i] = clip.end[i] + clip.maxs[i] + 1;
		}
		else
		{
			clip.boxmins[i] = clip.end[i] + clip.mins[i] - 1;
			clip.boxmaxs[i] = clip.start[i] + clip.maxs[i] + 1;
		}
	}

	// clip to other solid entities
	if (GGameType & GAME_WolfSP)
	{
		SVWS_ClipMoveToEntities(&clip);
	}
	else if (GGameType & GAME_WolfMP)
	{
		SVWM_ClipMoveToEntities(&clip);
	}
	else if (GGameType & GAME_ET)
	{
		SVET_ClipMoveToEntities(&clip);
	}
	else
	{
		SVQ3_ClipMoveToEntities(&clip);
	}

	*results = clip.trace;
}

int SVT3_PointContents(const vec3_t p, int passEntityNum)
{
	// get base contents from world
	int contents = CM_PointContentsQ3(p, 0);

	// or in contents from all the other entities
	int touch[MAX_GENTITIES_Q3];
	int num = SVT3_AreaEntities(p, p, touch, MAX_GENTITIES_Q3);

	for (int i = 0; i < num; i++)
	{
		if (touch[i] == passEntityNum)
		{
			continue;
		}
		if (GGameType & GAME_WolfSP)
		{
			wssharedEntity_t* hit = SVWS_GentityNum(touch[i]);
			// might intersect, so do an exact clip
			clipHandle_t clipHandle = SVWS_ClipHandleForEntity(hit);
			float* angles = hit->s.angles;
			if (!hit->r.bmodel)
			{
				angles = vec3_origin;	// boxes don't rotate
			}

			int c2 = CM_TransformedPointContentsQ3(p, clipHandle, hit->s.origin, hit->s.angles);

			contents |= c2;
		}
		else if (GGameType & GAME_WolfMP)
		{
			wmsharedEntity_t* hit = SVWM_GentityNum(touch[i]);
			// might intersect, so do an exact clip
			clipHandle_t clipHandle = SVWM_ClipHandleForEntity(hit);
			float* angles = hit->s.angles;
			if (!hit->r.bmodel)
			{
				angles = vec3_origin;	// boxes don't rotate
			}

			int c2 = CM_TransformedPointContentsQ3(p, clipHandle, hit->s.origin, hit->s.angles);

			contents |= c2;
		}
		else if (GGameType & GAME_ET)
		{
			etsharedEntity_t* hit = SVET_GentityNum(touch[i]);
			// might intersect, so do an exact clip
			clipHandle_t clipHandle = SVET_ClipHandleForEntity(hit);

			// ydnar: non-worldspawn entities must not use world as clip model!
			if (clipHandle == 0)
			{
				continue;
			}

			float* angles = hit->r.currentAngles;
			if (!hit->r.bmodel)
			{
				angles = vec3_origin;	// boxes don't rotate
			}

			int c2 = CM_TransformedPointContentsQ3(p, clipHandle, hit->r.currentOrigin, hit->r.currentAngles);

			contents |= c2;
		}
		else
		{
			q3sharedEntity_t* hit = SVQ3_GentityNum(touch[i]);
			// might intersect, so do an exact clip
			clipHandle_t clipHandle = SVQ3_ClipHandleForEntity(hit);
			float* angles = hit->s.angles;
			if (!hit->r.bmodel)
			{
				angles = vec3_origin;	// boxes don't rotate
			}

			int c2 = CM_TransformedPointContentsQ3(p, clipHandle, hit->s.origin, hit->s.angles);

			contents |= c2;
		}
	}

	return contents;
}
