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

#include "local.h"
#include "../quake3/local.h"
#include "../wolfsp/local.h"
#include "../wolfmp/local.h"
#include "../et/local.h"
#include "../worldsector.h"
#include "../../common/Common.h"
#include "../../common/common_defs.h"
#include "../../common/content_types.h"

struct q3moveclip_t
{
	vec3_t boxmins, boxmaxs;	// enclose the test object along entire move
	const float* mins;
	const float* maxs;	// size of the moving object
	const float* start;
	vec3_t end;
	q3trace_t trace;
	int passEntityNum;
	int contentmask;
	int capsule;
};

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

void SVT3_UnlinkEntity(idEntity3* ent, q3svEntity_t* svent)
{
	ent->SetLinked(false);

	worldSector_t* ws = svent->worldSector;
	if (!ws)
	{
		return;		// not linked in anywhere
	}
	svent->worldSector = NULL;

	if (ws->entities == svent)
	{
		ws->entities = svent->nextEntityInWorldSector;
		return;
	}

	for (q3svEntity_t* scan = ws->entities; scan; scan = scan->nextEntityInWorldSector)
	{
		if (scan->nextEntityInWorldSector == svent)
		{
			scan->nextEntityInWorldSector = svent->nextEntityInWorldSector;
			return;
		}
	}

	common->Printf("WARNING: SV_UnlinkEntity: not found in worldSector\n");
}

void SVT3_LinkEntity(idEntity3* ent, q3svEntity_t* svent)
{
	// Ridah, sanity check for possible currentOrigin being reset bug
	if (!ent->GetBModel() && VectorCompare(ent->GetCurrentOrigin(), vec3_origin))
	{
		common->DPrintf("WARNING: BBOX entity is being linked at world origin, this is probably a bug\n");
	}

	if (svent->worldSector)
	{
		SVT3_UnlinkEntity(ent, svent);		// unlink from old position
	}

	// encode the size into the q3entityState_t for client prediction
	if (ent->GetBModel())
	{
		ent->SetSolid(Q3SOLID_BMODEL);		// a solid_box will never create this value

		// Gordon: for the origin only bmodel checks
		svent->originCluster = CM_LeafCluster(CM_PointLeafnum(ent->GetCurrentOrigin()));
	}
	else if (ent->GetContents() & (BSP46CONTENTS_SOLID | BSP46CONTENTS_BODY))
	{
		// assume that x/y are equal and symetric
		int i = ent->GetMaxs()[0];
		if (i < 1)
		{
			i = 1;
		}
		if (i > 255)
		{
			i = 255;
		}

		// z is not symetric
		int j = (-ent->GetMins()[2]);
		if (j < 1)
		{
			j = 1;
		}
		if (j > 255)
		{
			j = 255;
		}

		// and z maxs can be negative...
		int k = (ent->GetMaxs()[2] + 32);
		if (k < 1)
		{
			k = 1;
		}
		if (k > 255)
		{
			k = 255;
		}

		ent->SetSolid((k << 16) | (j << 8) | i);
	}
	else
	{
		ent->SetSolid(0);
	}

	// get the position
	const float* origin = ent->GetCurrentOrigin();
	const float* angles = ent->GetCurrentAngles();

	// set the abs box
	if (ent->GetBModel() && (angles[0] || angles[1] || angles[2]))
	{
		// expand for rotation
		float max;
		int i;

		max = RadiusFromBounds(ent->GetMins(), ent->GetMaxs());
		for (i = 0; i < 3; i++)
		{
			ent->GetAbsMin()[i] = origin[i] - max;
			ent->GetAbsMax()[i] = origin[i] + max;
		}
	}
	else
	{
		// normal
		VectorAdd(origin, ent->GetMins(), ent->GetAbsMin());
		VectorAdd(origin, ent->GetMaxs(), ent->GetAbsMax());
	}

	// because movement is clipped an epsilon away from an actual edge,
	// we must fully check even when bounding boxes don't quite touch
	ent->GetAbsMin()[0] -= 1;
	ent->GetAbsMin()[1] -= 1;
	ent->GetAbsMin()[2] -= 1;
	ent->GetAbsMax()[0] += 1;
	ent->GetAbsMax()[1] += 1;
	ent->GetAbsMax()[2] += 1;

	// link to PVS leafs
	svent->numClusters = 0;
	svent->lastCluster = 0;
	svent->areanum = -1;
	svent->areanum2 = -1;

	//get all leafs, including solids
	enum { MAX_TOTAL_ENT_LEAFS = 128 };
	int leafs[MAX_TOTAL_ENT_LEAFS];
	int lastLeaf;
	int num_leafs = CM_BoxLeafnums(ent->GetAbsMin(), ent->GetAbsMax(),
		leafs, MAX_TOTAL_ENT_LEAFS, NULL, &lastLeaf);

	// if none of the leafs were inside the map, the
	// entity is outside the world and can be considered unlinked
	if (!num_leafs)
	{
		return;
	}

	// set areas, even from clusters that don't fit in the entity array
	for (int i = 0; i < num_leafs; i++)
	{
		int area = CM_LeafArea(leafs[i]);
		if (area != -1)
		{
			// doors may legally straggle two areas,
			// but nothing should evern need more than that
			if (svent->areanum != -1 && svent->areanum != area)
			{
				if (svent->areanum2 != -1 && svent->areanum2 != area && sv.state == SS_LOADING)
				{
					common->DPrintf("Object %i touching 3 areas at %f %f %f\n",
						ent->GetNumber(),
						ent->GetAbsMin()[0], ent->GetAbsMin()[1], ent->GetAbsMin()[2]);
				}
				svent->areanum2 = area;
			}
			else
			{
				svent->areanum = area;
			}
		}
	}

	// store as many explicit clusters as we can
	svent->numClusters = 0;
	int i;
	for (i = 0; i < num_leafs; i++)
	{
		int cluster = CM_LeafCluster(leafs[i]);
		if (cluster != -1)
		{
			svent->clusternums[svent->numClusters++] = cluster;
			if (svent->numClusters == MAX_ENT_CLUSTERS_Q3)
			{
				break;
			}
		}
	}

	// store off a last cluster if we need to
	if (i != num_leafs)
	{
		svent->lastCluster = CM_LeafCluster(lastLeaf);
	}

	ent->IncLinkCount();

	// find the first world sector node that the ent's box crosses
	worldSector_t* node = sv_worldSectors;
	while (1)
	{
		if (node->axis == -1)
		{
			break;
		}
		if (ent->GetAbsMin()[node->axis] > node->dist)
		{
			node = node->children[0];
		}
		else if (ent->GetAbsMax()[node->axis] < node->dist)
		{
			node = node->children[1];
		}
		else
		{
			break;		// crosses the node
		}
	}

	// link it in
	svent->worldSector = node;
	svent->nextEntityInWorldSector = node->entities;
	node->entities = svent;

	ent->SetLinked(true);
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
	q3svEntity_t* next;
	for (q3svEntity_t* check = node->entities; check; check = next)
	{
		next = check->nextEntityInWorldSector;

		idEntity3* gcheck = SVT3_EntityForSvEntity(check);

		if (gcheck->GetAbsMin()[0] > ap->maxs[0] ||
			gcheck->GetAbsMin()[1] > ap->maxs[1] ||
			gcheck->GetAbsMin()[2] > ap->maxs[2] ||
			gcheck->GetAbsMax()[0] < ap->mins[0] ||
			gcheck->GetAbsMax()[1] < ap->mins[1] ||
			gcheck->GetAbsMax()[2] < ap->mins[2])
		{
			continue;
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

//	Returns a headnode that can be used for testing or clipping to a
// given entity.  If the entity is a bsp model, the headnode will
// be returned, otherwise a custom box tree will be constructed.
clipHandle_t SVT3_ClipHandleForEntity(const idEntity3* ent)
{
	if (ent->GetBModel())
	{
		// explicit hulls in the BSP model
		return CM_InlineModel(ent->GetModelIndex());
	}
	if (ent->GetSvFlagCapsule())
	{
		// create a temp capsule from bounding box sizes
		return CM_TempBoxModel(ent->GetMins(), ent->GetMaxs(), true);
	}

	// create a temp tree from bounding box sizes
	return CM_TempBoxModel(ent->GetMins(), ent->GetMaxs(), false);
}

void SVT3_ClipToEntity(q3trace_t* trace, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int entityNum, int contentmask, int capsule)
{
	idEntity3* touch = SVT3_EntityNum(entityNum);

	Com_Memset(trace, 0, sizeof(q3trace_t));

	// if it doesn't have any brushes of a type we
	// are looking for, ignore it
	if (!(contentmask & touch->GetContents()))
	{
		trace->fraction = 1.0;
		return;
	}

	// might intersect, so do an exact clip
	clipHandle_t clipHandle = SVT3_ClipHandleForEntity(touch);

	const float* origin = touch->GetCurrentOrigin();
	const float* angles = touch->GetCurrentAngles();

	if (!touch->GetBModel())
	{
		angles = vec3_origin;	// boxes don't rotate
	}

	CM_TransformedBoxTraceQ3(trace, start, end,
		mins, maxs, clipHandle, contentmask,
		origin, angles, capsule);

	if (trace->fraction < 1)
	{
		trace->entityNum = touch->GetNumber();
	}
}

static void SVT3_ClipMoveToEntities(q3moveclip_t* clip)
{
	int touchlist[MAX_GENTITIES_Q3];
	int num = SVT3_AreaEntities(clip->boxmins, clip->boxmaxs, touchlist, MAX_GENTITIES_Q3);

	int passOwnerNum;
	if (clip->passEntityNum != Q3ENTITYNUM_NONE && clip->passEntityNum != -1)
	{
		passOwnerNum = SVT3_EntityNum(clip->passEntityNum)->GetOwnerNum();
		if (passOwnerNum == Q3ENTITYNUM_NONE)
		{
			passOwnerNum = -1;
		}
	}
	else
	{
		passOwnerNum = -1;
	}

	for (int i = 0; i < num; i++)
	{
		if (clip->trace.allsolid)
		{
			return;
		}
		idEntity3* touch = SVT3_EntityNum(touchlist[i]);

		// see if we should ignore this entity
		if (clip->passEntityNum != Q3ENTITYNUM_NONE)
		{
			if (touchlist[i] == clip->passEntityNum)
			{
				continue;	// don't clip against the pass entity
			}
			if (touch->GetOwnerNum() == clip->passEntityNum)
			{
				continue;	// don't clip against own missiles
			}
			if (touch->GetOwnerNum() == passOwnerNum)
			{
				continue;	// don't clip against other missiles from our owner
			}
		}

		// if it doesn't have any brushes of a type we
		// are looking for, ignore it
		if (!(clip->contentmask & touch->GetContents()))
		{
			continue;
		}

		// RF, special case, ignore chairs if we are carrying them
		if (touch->IsETypeProp() && touch->GetOtherEntityNum() == clip->passEntityNum + 1)
		{
			continue;
		}

		// might intersect, so do an exact clip
		clipHandle_t clipHandle = SVT3_ClipHandleForEntity(touch);

		// ydnar: non-worldspawn entities must not use world as clip model!
		if (clipHandle == 0)
		{
			continue;
		}

		// DHM - Nerve :: If clipping against BBOX, set to correct contents
		touch->SetTempBoxModelContents(clipHandle);

		const float* origin = touch->GetCurrentOrigin();
		const float* angles = touch->GetCurrentAngles();

		if (!touch->GetBModel())
		{
			angles = vec3_origin;	// boxes don't rotate
		}

		q3trace_t trace;
		CM_TransformedBoxTraceQ3(&trace, clip->start, clip->end,
			clip->mins, clip->maxs, clipHandle,  clip->contentmask,
			origin, angles, clip->capsule);

		if (trace.allsolid)
		{
			clip->trace.allsolid = true;
			trace.entityNum = touch->GetNumber();
		}
		else if (trace.startsolid)
		{
			clip->trace.startsolid = true;
			trace.entityNum = touch->GetNumber();
		}

		if (trace.fraction < clip->trace.fraction)
		{
			qboolean oldStart;

			// make sure we keep a startsolid from a previous trace
			oldStart = clip->trace.startsolid;

			trace.entityNum = touch->GetNumber();
			clip->trace = trace;
			clip->trace.startsolid |= oldStart;
		}

		// DHM - Nerve :: Reset contents to default
		CM_SetTempBoxModelContents(clipHandle, BSP46CONTENTS_BODY);
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
	SVT3_ClipMoveToEntities(&clip);

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
		idEntity3* hit = SVT3_EntityNum(touch[i]);
		// might intersect, so do an exact clip
		clipHandle_t clipHandle = SVT3_ClipHandleForEntity(hit);

		// ydnar: non-worldspawn entities must not use world as clip model!
		if (clipHandle == 0)
		{
			continue;
		}

		int c2 = CM_TransformedPointContentsQ3(p, clipHandle,
			GGameType & GAME_ET ? hit->GetCurrentOrigin() : hit->GetOrigin(),
			GGameType & GAME_ET ? hit->GetCurrentAngles() : hit->GetAngles());

		contents |= c2;
	}

	return contents;
}
