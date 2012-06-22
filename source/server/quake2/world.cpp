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

#define EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,q2edict_t,area)

struct q2moveclip_t
{
	vec3_t boxmins, boxmaxs;	// enclose the test object along entire move
	float* mins, * maxs;		// size of the moving object
	vec3_t mins2, maxs2;		// size when clipping against mosnters
	float* start, * end;
	q2trace_t trace;
	q2edict_t* passedict;
	int contentmask;
};

static float* area_mins, * area_maxs;
static q2edict_t** area_list;
static int area_count, area_maxcount;
static int area_type;

void SVQ2_UnlinkEdict(q2edict_t* ent)
{
	if (!ent->area.prev)
	{
		return;		// not linked in anywhere
	}
	RemoveLink(&ent->area);
	ent->area.prev = ent->area.next = NULL;
}

void SVQ2_LinkEdict(q2edict_t* ent)
{
	enum { MAX_TOTAL_ENT_LEAFS = 128 };

	if (ent->area.prev)
	{
		SVQ2_UnlinkEdict(ent);	// unlink from old position

	}
	if (ent == ge->edicts)
	{
		return;		// don't add the world

	}
	if (!ent->inuse)
	{
		return;
	}

	// set the size
	VectorSubtract(ent->maxs, ent->mins, ent->size);

	// encode the size into the entity_state for client prediction
	if (ent->solid == Q2SOLID_BBOX && !(ent->svflags & Q2SVF_DEADMONSTER))
	{
		// assume that x/y are equal and symetric
		int i = ent->maxs[0] / 8;
		if (i < 1)
		{
			i = 1;
		}
		if (i > 31)
		{
			i = 31;
		}

		// z is not symetric
		int j = (-ent->mins[2]) / 8;
		if (j < 1)
		{
			j = 1;
		}
		if (j > 31)
		{
			j = 31;
		}

		// and z maxs can be negative...
		int k = (ent->maxs[2] + 32) / 8;
		if (k < 1)
		{
			k = 1;
		}
		if (k > 63)
		{
			k = 63;
		}

		ent->s.solid = (k << 10) | (j << 5) | i;
	}
	else if (ent->solid == Q2SOLID_BSP)
	{
		ent->s.solid = 31;		// a solid_bbox will never create this value
	}
	else
	{
		ent->s.solid = 0;
	}

	// set the abs box
	if (ent->solid == Q2SOLID_BSP &&
		(ent->s.angles[0] || ent->s.angles[1] || ent->s.angles[2]))
	{
		// expand for rotation
		float max, v;
		int i;

		max = 0;
		for (i = 0; i < 3; i++)
		{
			v = fabs(ent->mins[i]);
			if (v > max)
			{
				max = v;
			}
			v = fabs(ent->maxs[i]);
			if (v > max)
			{
				max = v;
			}
		}
		for (i = 0; i < 3; i++)
		{
			ent->absmin[i] = ent->s.origin[i] - max;
			ent->absmax[i] = ent->s.origin[i] + max;
		}
	}
	else
	{
		// normal
		VectorAdd(ent->s.origin, ent->mins, ent->absmin);
		VectorAdd(ent->s.origin, ent->maxs, ent->absmax);
	}

	// because movement is clipped an epsilon away from an actual edge,
	// we must fully check even when bounding boxes don't quite touch
	ent->absmin[0] -= 1;
	ent->absmin[1] -= 1;
	ent->absmin[2] -= 1;
	ent->absmax[0] += 1;
	ent->absmax[1] += 1;
	ent->absmax[2] += 1;

	// link to PVS leafs
	ent->num_clusters = 0;
	ent->areanum = 0;
	ent->areanum2 = 0;

	//get all leafs, including solids
	int topnode;
	int leafs[MAX_TOTAL_ENT_LEAFS];
	int num_leafs = CM_BoxLeafnums(ent->absmin, ent->absmax,
		leafs, MAX_TOTAL_ENT_LEAFS, &topnode);

	// set areas
	int clusters[MAX_TOTAL_ENT_LEAFS];
	for (int i = 0; i < num_leafs; i++)
	{
		clusters[i] = CM_LeafCluster(leafs[i]);
		int area = CM_LeafArea(leafs[i]);
		if (area)
		{
			// doors may legally straggle two areas,
			// but nothing should evern need more than that
			if (ent->areanum && ent->areanum != area)
			{
				if (ent->areanum2 && ent->areanum2 != area && sv.state == SS_LOADING)
				{
					common->DPrintf("Object touching 3 areas at %f %f %f\n",
						ent->absmin[0], ent->absmin[1], ent->absmin[2]);
				}
				ent->areanum2 = area;
			}
			else
			{
				ent->areanum = area;
			}
		}
	}

	if (num_leafs >= MAX_TOTAL_ENT_LEAFS)
	{
		// assume we missed some leafs, and mark by headnode
		ent->num_clusters = -1;
		ent->headnode = topnode;
	}
	else
	{
		ent->num_clusters = 0;
		for (int i = 0; i < num_leafs; i++)
		{
			if (clusters[i] == -1)
			{
				continue;		// not a visible leaf
			}
			int j;
			for (j = 0; j < i; j++)
			{
				if (clusters[j] == clusters[i])
				{
					break;
				}
			}
			if (j == i)
			{
				if (ent->num_clusters == MAX_ENT_CLUSTERS_Q2)
				{	// assume we missed some leafs, and mark by headnode
					ent->num_clusters = -1;
					ent->headnode = topnode;
					break;
				}

				ent->clusternums[ent->num_clusters++] = clusters[i];
			}
		}
	}

	// if first time, make sure old_origin is valid
	if (!ent->linkcount)
	{
		VectorCopy(ent->s.origin, ent->s.old_origin);
	}
	ent->linkcount++;

	if (ent->solid == Q2SOLID_NOT)
	{
		return;
	}

	// find the first node that the ent's box crosses
	worldSector_t* node = sv_worldSectors;
	while (1)
	{
		if (node->axis == -1)
		{
			break;
		}
		if (ent->absmin[node->axis] > node->dist)
		{
			node = node->children[0];
		}
		else if (ent->absmax[node->axis] < node->dist)
		{
			node = node->children[1];
		}
		else
		{
			break;		// crosses the node
		}
	}

	// link it in
	if (ent->solid == Q2SOLID_TRIGGER)
	{
		InsertLinkBefore(&ent->area, &node->trigger_edicts);
	}
	else
	{
		InsertLinkBefore(&ent->area, &node->solid_edicts);
	}

}

static void SVQ2_AreaEdicts_r(worldSector_t* node)
{
	// touch linked edicts
	link_t* start;
	if (area_type == Q2AREA_SOLID)
	{
		start = &node->solid_edicts;
	}
	else
	{
		start = &node->trigger_edicts;
	}

	link_t* next;
	for (link_t* l = start->next; l != start; l = next)
	{
		next = l->next;
		q2edict_t* check = EDICT_FROM_AREA(l);

		if (check->solid == Q2SOLID_NOT)
		{
			continue;		// deactivated
		}
		if (check->absmin[0] > area_maxs[0] ||
			check->absmin[1] > area_maxs[1] ||
			check->absmin[2] > area_maxs[2] ||
			check->absmax[0] < area_mins[0] ||
			check->absmax[1] < area_mins[1] ||
			check->absmax[2] < area_mins[2])
		{
			continue;		// not touching

		}
		if (area_count == area_maxcount)
		{
			common->Printf("SVQ2_AreaEdicts: MAXCOUNT\n");
			return;
		}

		area_list[area_count] = check;
		area_count++;
	}

	if (node->axis == -1)
	{
		return;		// terminal node

	}
	// recurse down both sides
	if (area_maxs[node->axis] > node->dist)
	{
		SVQ2_AreaEdicts_r(node->children[0]);
	}
	if (area_mins[node->axis] < node->dist)
	{
		SVQ2_AreaEdicts_r(node->children[1]);
	}
}

int SVQ2_AreaEdicts(vec3_t mins, vec3_t maxs, q2edict_t** list,
	int maxcount, int areatype)
{
	area_mins = mins;
	area_maxs = maxs;
	area_list = list;
	area_count = 0;
	area_maxcount = maxcount;
	area_type = areatype;

	SVQ2_AreaEdicts_r(sv_worldSectors);

	return area_count;
}

//	Returns a headnode that can be used for testing or clipping an
// object of mins/maxs size.
// Offset is filled in to contain the adjustment that must be added to the
// testing object's origin to get a point to use with the returned hull.
static clipHandle_t SVQ2_HullForEntity(q2edict_t* ent)
{
	// decide which clipping hull to use, based on the size
	if (ent->solid == Q2SOLID_BSP)
	{
		// explicit hulls in the BSP model
		clipHandle_t model = sv.models[ent->s.modelindex];

		if (!model)
		{
			common->FatalError("MOVETYPE_PUSH with a non bsp model");
		}

		return model;
	}

	// create a temp hull from bounding box sizes
	return CM_TempBoxModel(ent->mins, ent->maxs);
}

int SVQ2_PointContents(vec3_t p)
{
	// get base contents from world
	int contents = CM_PointContentsQ2(p, 0);

	// or in contents from all the other entities
	q2edict_t* touch[MAX_EDICTS_Q2];
	int num = SVQ2_AreaEdicts(p, p, touch, MAX_EDICTS_Q2, Q2AREA_SOLID);

	for (int i = 0; i < num; i++)
	{
		q2edict_t* hit = touch[i];

		// might intersect, so do an exact clip
		clipHandle_t model = SVQ2_HullForEntity(hit);
		float* angles = hit->s.angles;
		if (hit->solid != Q2SOLID_BSP)
		{
			angles = vec3_origin;	// boxes don't rotate

		}
		int c2 = CM_TransformedPointContentsQ2(p, model, hit->s.origin, hit->s.angles);

		contents |= c2;
	}

	return contents;
}

static void SVQ2_ClipMoveToEntities(q2moveclip_t* clip)
{
	q2edict_t* touchlist[MAX_EDICTS_Q2];
	int num = SVQ2_AreaEdicts(clip->boxmins, clip->boxmaxs, touchlist,
		MAX_EDICTS_Q2, Q2AREA_SOLID);

	// be careful, it is possible to have an entity in this
	// list removed before we get to it (killtriggered)
	for (int i = 0; i < num; i++)
	{
		q2edict_t* touch = touchlist[i];
		if (touch->solid == Q2SOLID_NOT)
		{
			continue;
		}
		if (touch == clip->passedict)
		{
			continue;
		}
		if (clip->trace.allsolid)
		{
			return;
		}
		if (clip->passedict)
		{
			if (touch->owner == clip->passedict)
			{
				continue;	// don't clip against own missiles
			}
			if (clip->passedict->owner == touch)
			{
				continue;	// don't clip against owner
			}
		}

		if (!(clip->contentmask & BSP38CONTENTS_DEADMONSTER) &&
			(touch->svflags & Q2SVF_DEADMONSTER))
		{
			continue;
		}

		// might intersect, so do an exact clip
		clipHandle_t model = SVQ2_HullForEntity(touch);
		float* angles = touch->s.angles;
		if (touch->solid != Q2SOLID_BSP)
		{
			angles = vec3_origin;	// boxes don't rotate

		}
		q2trace_t trace;
		if (touch->svflags & Q2SVF_MONSTER)
		{
			trace = CM_TransformedBoxTraceQ2(clip->start, clip->end,
				clip->mins2, clip->maxs2, model, clip->contentmask,
				touch->s.origin, angles);
		}
		else
		{
			trace = CM_TransformedBoxTraceQ2(clip->start, clip->end,
				clip->mins, clip->maxs, model,  clip->contentmask,
				touch->s.origin, angles);
		}

		if (trace.allsolid || trace.startsolid ||
			trace.fraction < clip->trace.fraction)
		{
			trace.ent = touch;
			if (clip->trace.startsolid)
			{
				clip->trace = trace;
				clip->trace.startsolid = true;
			}
			else
			{
				clip->trace = trace;
			}
		}
		else if (trace.startsolid)
		{
			clip->trace.startsolid = true;
		}
	}
}

void SVQ2_TraceBounds(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, vec3_t boxmins, vec3_t boxmaxs)
{
	for (int i = 0; i < 3; i++)
	{
		if (end[i] > start[i])
		{
			boxmins[i] = start[i] + mins[i] - 1;
			boxmaxs[i] = end[i] + maxs[i] + 1;
		}
		else
		{
			boxmins[i] = end[i] + mins[i] - 1;
			boxmaxs[i] = start[i] + maxs[i] + 1;
		}
	}
}

//	Moves the given mins/maxs volume through the world from start to end.
//	Passedict and edicts owned by passedict are explicitly not checked.
q2trace_t SVQ2_Trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, q2edict_t* passedict, int contentmask)
{
	if (!mins)
	{
		mins = vec3_origin;
	}
	if (!maxs)
	{
		maxs = vec3_origin;
	}

	q2moveclip_t clip;
	Com_Memset(&clip, 0, sizeof(q2moveclip_t));

	// clip to world
	clip.trace = CM_BoxTraceQ2(start, end, mins, maxs, 0, contentmask);
	clip.trace.ent = ge->edicts;
	if (clip.trace.fraction == 0)
	{
		// blocked by the world
		return clip.trace;

	}
	clip.contentmask = contentmask;
	clip.start = start;
	clip.end = end;
	clip.mins = mins;
	clip.maxs = maxs;
	clip.passedict = passedict;

	VectorCopy(mins, clip.mins2);
	VectorCopy(maxs, clip.maxs2);

	// create the bounding box of the entire move
	SVQ2_TraceBounds(start, clip.mins2, clip.maxs2, end, clip.boxmins, clip.boxmaxs);

	// clip to other solid entities
	SVQ2_ClipMoveToEntities(&clip);

	return clip.trace;
}
