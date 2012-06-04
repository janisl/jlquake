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
#include "../tech3/local.h"
#include "local.h"

void SVWS_UnlinkEntity(wssharedEntity_t* gEnt)
{
	q3svEntity_t* ent = SVWS_SvEntityForGentity(gEnt);

	gEnt->r.linked = false;

	SVT3_UnlinkSvEntity(ent);
}

void SVWS_LinkEntity(wssharedEntity_t* gEnt)
{
	q3svEntity_t* ent = SVWS_SvEntityForGentity(gEnt);

	// Ridah, sanity check for possible currentOrigin being reset bug
	if (!gEnt->r.bmodel && VectorCompare(gEnt->r.currentOrigin, vec3_origin))
	{
		common->DPrintf("WARNING: BBOX entity is being linked at world origin, this is probably a bug\n");
	}

	if (ent->worldSector)
	{
		SVWS_UnlinkEntity(gEnt);		// unlink from old position
	}

	// encode the size into the wsentityState_t for client prediction
	if (gEnt->r.bmodel)
	{
		gEnt->s.solid = Q3SOLID_BMODEL;			// a solid_box will never create this value
	}
	else if (gEnt->r.contents & (BSP46CONTENTS_SOLID | BSP46CONTENTS_BODY))
	{
		// assume that x/y are equal and symetric
		int i = gEnt->r.maxs[0];
		if (i < 1)
		{
			i = 1;
		}
		if (i > 255)
		{
			i = 255;
		}

		// z is not symetric
		int j = (-gEnt->r.mins[2]);
		if (j < 1)
		{
			j = 1;
		}
		if (j > 255)
		{
			j = 255;
		}

		// and z maxs can be negative...
		int k = (gEnt->r.maxs[2] + 32);
		if (k < 1)
		{
			k = 1;
		}
		if (k > 255)
		{
			k = 255;
		}

		gEnt->s.solid = (k << 16) | (j << 8) | i;
	}
	else
	{
		gEnt->s.solid = 0;
	}

	// get the position
	float* origin = gEnt->r.currentOrigin;
	float* angles = gEnt->r.currentAngles;

	// set the abs box
	if (gEnt->r.bmodel && (angles[0] || angles[1] || angles[2]))
	{
		// expand for rotation
		float max;
		int i;

		max = RadiusFromBounds(gEnt->r.mins, gEnt->r.maxs);
		for (i = 0; i < 3; i++)
		{
			gEnt->r.absmin[i] = origin[i] - max;
			gEnt->r.absmax[i] = origin[i] + max;
		}
	}
	else
	{
		// normal
		VectorAdd(origin, gEnt->r.mins, gEnt->r.absmin);
		VectorAdd(origin, gEnt->r.maxs, gEnt->r.absmax);
	}

	// because movement is clipped an epsilon away from an actual edge,
	// we must fully check even when bounding boxes don't quite touch
	gEnt->r.absmin[0] -= 1;
	gEnt->r.absmin[1] -= 1;
	gEnt->r.absmin[2] -= 1;
	gEnt->r.absmax[0] += 1;
	gEnt->r.absmax[1] += 1;
	gEnt->r.absmax[2] += 1;

	// link to PVS leafs
	ent->numClusters = 0;
	ent->lastCluster = 0;
	ent->areanum = -1;
	ent->areanum2 = -1;

	//get all leafs, including solids
	enum { MAX_TOTAL_ENT_LEAFS = 128 };
	int leafs[MAX_TOTAL_ENT_LEAFS];
	int lastLeaf;
	int num_leafs = CM_BoxLeafnums(gEnt->r.absmin, gEnt->r.absmax,
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
			if (ent->areanum != -1 && ent->areanum != area)
			{
				if (ent->areanum2 != -1 && ent->areanum2 != area && sv.state == SS_LOADING)
				{
					common->DPrintf("Object %i touching 3 areas at %f %f %f\n",
						gEnt->s.number,
						gEnt->r.absmin[0], gEnt->r.absmin[1], gEnt->r.absmin[2]);
				}
				ent->areanum2 = area;
			}
			else
			{
				ent->areanum = area;
			}
		}
	}

	// store as many explicit clusters as we can
	ent->numClusters = 0;
	int i;
	for (i = 0; i < num_leafs; i++)
	{
		int cluster = CM_LeafCluster(leafs[i]);
		if (cluster != -1)
		{
			ent->clusternums[ent->numClusters++] = cluster;
			if (ent->numClusters == MAX_ENT_CLUSTERS_Q3)
			{
				break;
			}
		}
	}

	// store off a last cluster if we need to
	if (i != num_leafs)
	{
		ent->lastCluster = CM_LeafCluster(lastLeaf);
	}

	gEnt->r.linkcount++;

	// find the first world sector node that the ent's box crosses
	worldSector_t* node = sv_worldSectors;
	while (1)
	{
		if (node->axis == -1)
		{
			break;
		}
		if (gEnt->r.absmin[node->axis] > node->dist)
		{
			node = node->children[0];
		}
		else if (gEnt->r.absmax[node->axis] < node->dist)
		{
			node = node->children[1];
		}
		else
		{
			break;		// crosses the node
		}
	}

	// link it in
	ent->worldSector = node;
	ent->nextEntityInWorldSector = node->entities;
	node->entities = ent;

	gEnt->r.linked = true;
}

//	Returns a headnode that can be used for testing or clipping to a
// given entity.  If the entity is a bsp model, the headnode will
// be returned, otherwise a custom box tree will be constructed.
clipHandle_t SVWS_ClipHandleForEntity(const wssharedEntity_t* ent)
{
	if (ent->r.bmodel)
	{
		// explicit hulls in the BSP model
		return CM_InlineModel(ent->s.modelindex);
	}
	if (ent->r.svFlags & WOLFSVF_CAPSULE)
	{
		// create a temp capsule from bounding box sizes
		return CM_TempBoxModel(ent->r.mins, ent->r.maxs, true);
	}

	// create a temp tree from bounding box sizes
	return CM_TempBoxModel(ent->r.mins, ent->r.maxs, false);
}

void SVWS_ClipToEntity(q3trace_t* trace, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int entityNum, int contentmask, int capsule)
{
	wssharedEntity_t* touch = SVWS_GentityNum(entityNum);

	Com_Memset(trace, 0, sizeof(q3trace_t));

	// if it doesn't have any brushes of a type we
	// are looking for, ignore it
	if (!(contentmask & touch->r.contents))
	{
		trace->fraction = 1.0;
		return;
	}

	// might intersect, so do an exact clip
	clipHandle_t clipHandle = SVWS_ClipHandleForEntity(touch);

	float* origin = touch->r.currentOrigin;
	float* angles = touch->r.currentAngles;

	if (!touch->r.bmodel)
	{
		angles = vec3_origin;	// boxes don't rotate
	}

	CM_TransformedBoxTraceQ3(trace, start, end,
		mins, maxs, clipHandle, contentmask,
		origin, angles, capsule);

	if (trace->fraction < 1)
	{
		trace->entityNum = touch->s.number;
	}
}

void SVWS_ClipMoveToEntities(q3moveclip_t* clip)
{
	int touchlist[MAX_GENTITIES_Q3];
	int num = SVT3_AreaEntities(clip->boxmins, clip->boxmaxs, touchlist, MAX_GENTITIES_Q3);

	int passOwnerNum;
	if (clip->passEntityNum != Q3ENTITYNUM_NONE)
	{
		passOwnerNum = (SVWS_GentityNum(clip->passEntityNum))->r.ownerNum;
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
		wssharedEntity_t* touch = SVWS_GentityNum(touchlist[i]);

		// see if we should ignore this entity
		if (clip->passEntityNum != Q3ENTITYNUM_NONE)
		{
			if (touchlist[i] == clip->passEntityNum)
			{
				continue;	// don't clip against the pass entity
			}
			if (touch->r.ownerNum == clip->passEntityNum)
			{
				continue;	// don't clip against own missiles
			}
			if (touch->r.ownerNum == passOwnerNum)
			{
				continue;	// don't clip against other missiles from our owner
			}
		}

		// if it doesn't have any brushes of a type we
		// are looking for, ignore it
		if (!(clip->contentmask & touch->r.contents))
		{
			continue;
		}

		// RF, special case, ignore chairs if we are carrying them
		if (touch->s.eType == WSET_PROP && touch->s.otherEntityNum == clip->passEntityNum + 1)
		{
			continue;
		}

		// might intersect, so do an exact clip
		clipHandle_t clipHandle = SVWS_ClipHandleForEntity(touch);

		float* origin = touch->r.currentOrigin;
		float* angles = touch->r.currentAngles;


		if (!touch->r.bmodel)
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
			trace.entityNum = touch->s.number;
		}
		else if (trace.startsolid)
		{
			clip->trace.startsolid = true;
			trace.entityNum = touch->s.number;
		}

		if (trace.fraction < clip->trace.fraction)
		{
			qboolean oldStart;

			// make sure we keep a startsolid from a previous trace
			oldStart = clip->trace.startsolid;

			trace.entityNum = touch->s.number;
			clip->trace = trace;
			clip->trace.startsolid |= oldStart;
		}
	}
}
