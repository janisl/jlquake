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
#include "../progsvm/progsvm.h"
#include "local.h"

/*

entities never clip against themselves, or their owner

line of sight checks trace->crosscontent, but bullets don't

*/

struct qhmoveclip_t
{
	vec3_t boxmins, boxmaxs;	// enclose the test object along entire move
	const float* mins;		// size of the moving object
	const float* maxs;
	vec3_t mins2, maxs2;		// size when clipping against mosnters
	const float* start;
	const float* end;
	q1trace_t trace;
	int type;
	qhedict_t* passedict;
};

Cvar* sys_quake2;

void SVQH_UnlinkEdict(qhedict_t* ent)
{
	if (!ent->area.prev)
	{
		return;		// not linked in anywhere
	}
	RemoveLink(&ent->area);
	ent->area.prev = ent->area.next = NULL;
}

static void SVQH_TouchLinks(qhedict_t* ent, worldSector_t* node)
{
	// touch linked edicts
	link_t* next;
	for (link_t* l = node->trigger_edicts.next; l != &node->trigger_edicts; l = next)
	{
		if (!l)
		{
			//my area got removed out from under me!
			break;
		}
		next = l->next;
		qhedict_t* touch = QHEDICT_FROM_AREA(l);
		if (touch == ent)
		{
			continue;
		}
		if (!touch->GetTouch() || touch->GetSolid() != QHSOLID_TRIGGER)
		{
			continue;
		}
		if (ent->v.absmin[0] > touch->v.absmax[0] ||
			ent->v.absmin[1] > touch->v.absmax[1] ||
			ent->v.absmin[2] > touch->v.absmax[2] ||
			ent->v.absmax[0] < touch->v.absmin[0] ||
			ent->v.absmax[1] < touch->v.absmin[1] ||
			ent->v.absmax[2] < touch->v.absmin[2])
		{
			continue;
		}

		int old_self = *pr_globalVars.self;
		int old_other = *pr_globalVars.other;

		*pr_globalVars.self = EDICT_TO_PROG(touch);
		*pr_globalVars.other = EDICT_TO_PROG(ent);
		*pr_globalVars.time = sv.qh_time;
		PR_ExecuteProgram(touch->GetTouch());

		*pr_globalVars.self = old_self;
		*pr_globalVars.other = old_other;
	}

	// recurse down both sides
	if (node->axis == -1)
	{
		return;
	}

	if (ent->v.absmax[node->axis] > node->dist)
	{
		SVQH_TouchLinks(ent, node->children[0]);
	}
	if (ent->v.absmin[node->axis] < node->dist)
	{
		SVQH_TouchLinks(ent, node->children[1]);
	}
}

void SVQH_LinkEdict(qhedict_t* ent, bool touch_triggers)
{
	if (ent->area.prev)
	{
		SVQH_UnlinkEdict(ent);	// unlink from old position

	}
	if (ent == sv.qh_edicts)
	{
		return;		// don't add the world

	}
	if (ent->free)
	{
		return;
	}

	// set the abs box
	if (GGameType & GAME_Hexen2 && ent->GetSolid() == QHSOLID_BSP &&
		(ent->GetAngles()[0] || ent->GetAngles()[1] || ent->GetAngles()[2]))
	{	// expand for rotation
		float max = 0;
		for (int i = 0; i < 3; i++)
		{
			float v = Q_fabs(ent->GetMins()[i]);
			if (v > max)
			{
				max = v;
			}
			v = Q_fabs(ent->GetMaxs()[i]);
			if (v > max)
			{
				max = v;
			}
		}
		for (int i = 0; i < 3; i++)
		{
			ent->v.absmin[i] = ent->GetOrigin()[i] - max;
			ent->v.absmax[i] = ent->GetOrigin()[i] + max;
		}
	}
	else
	{
		VectorAdd(ent->GetOrigin(), ent->GetMins(), ent->v.absmin);
		VectorAdd(ent->GetOrigin(), ent->GetMaxs(), ent->v.absmax);
	}

	//
	// to make items easier to pick up and allow them to be grabbed off
	// of shelves, the abs sizes are expanded
	//
	if ((int)ent->GetFlags() & QHFL_ITEM)
	{
		ent->v.absmin[0] -= 15;
		ent->v.absmin[1] -= 15;
		ent->v.absmax[0] += 15;
		ent->v.absmax[1] += 15;
	}
	else
	{
		// because movement is clipped an epsilon away from an actual edge,
		// we must fully check even when bounding boxes don't quite touch
		ent->v.absmin[0] -= 1;
		ent->v.absmin[1] -= 1;
		ent->v.absmin[2] -= 1;
		ent->v.absmax[0] += 1;
		ent->v.absmax[1] += 1;
		ent->v.absmax[2] += 1;
	}

	// link to PVS leafs
	ent->num_leafs = 0;
	if (ent->v.modelindex)
	{
		ent->num_leafs = CM_BoxLeafnums(ent->v.absmin, ent->v.absmax, ent->LeafNums, MAX_ENT_LEAFS);
	}

	if (ent->GetSolid() == QHSOLID_NOT)
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
		if (ent->v.absmin[node->axis] > node->dist)
		{
			node = node->children[0];
		}
		else if (ent->v.absmax[node->axis] < node->dist)
		{
			node = node->children[1];
		}
		else
		{
			break;		// crosses the node
		}
	}

	// link it in
	if (ent->GetSolid() == QHSOLID_TRIGGER)
	{
		InsertLinkBefore(&ent->area, &node->trigger_edicts);
	}
	else
	{
		InsertLinkBefore(&ent->area, &node->solid_edicts);
	}

	// if touch_triggers, touch all entities at this node and decend for more
	if (touch_triggers)
	{
		SVQH_TouchLinks(ent, sv_worldSectors);
	}
}

int SVQH_PointContents(vec3_t p)
{
	int cont = CM_PointContentsQ1(p, 0);
	if (!(GGameType & GAME_QuakeWorld) && cont <= BSP29CONTENTS_CURRENT_0 && cont >= BSP29CONTENTS_CURRENT_DOWN)
	{
		cont = BSP29CONTENTS_WATER;
	}
	return cont;
}

//	Returns a hull that can be used for testing or clipping an object of mins/maxs
// size.
//	Offset is filled in to contain the adjustment that must be added to the
// testing object's origin to get a point to use with the returned hull.
static clipHandle_t SVQH_HullForEntity(qhedict_t* ent, const vec3_t mins,
	const vec3_t maxs, vec3_t offset, qhedict_t* move_ent)
{
	clipHandle_t model;
	vec3_t size;
	vec3_t hullmins, hullmaxs;
	clipHandle_t hull;

	// decide which clipping hull to use, based on the size
	if (ent->GetSolid() == QHSOLID_BSP)
	{
		// explicit hulls in the BSP model
		if (ent->GetMoveType() != QHMOVETYPE_PUSH)
		{
			common->Error("QHSOLID_BSP without QHMOVETYPE_PUSH");
		}

		model = sv.models[(int)ent->v.modelindex];

		if ((int)ent->v.modelindex != 1 && !model)
		{
			common->Error("QHMOVETYPE_PUSH with a non bsp model");
		}

		VectorSubtract(maxs, mins, size);
		vec3_t clip_mins;
		vec3_t clip_maxs;
		if (GGameType & GAME_Hexen2 && move_ent->GetHull())	// Entity is specifying which hull to use
		{
			int index = move_ent->GetHull() - 1;
			hull = CM_ModelHull(model, index, clip_mins, clip_maxs);
		}
		else	// Using the old way uses size to determine hull to use
		{
			if (size[0] < 3)
			{
				hull = CM_ModelHull(model, 0, clip_mins, clip_maxs);
			}
			else if (GGameType & GAME_Hexen2 && GGameType & GAME_H2Portals &&
				(size[0] <= 8) && ((int)(sv.qh_edicts->GetSpawnFlags()) & 1))	// Pentacles
			{
				hull = CM_ModelHull(model, 4, clip_mins, clip_maxs);
			}
			else if (GGameType & GAME_Hexen2 && size[0] <= 32 && size[2] <= 28)	// Half Player
			{
				hull = CM_ModelHull(model, 3, clip_mins, clip_maxs);
			}
			else if (size[0] <= 32)
			{
				hull = CM_ModelHull(model, 1, clip_mins, clip_maxs);
			}
			else if (GGameType & GAME_Hexen2)	// Golem
			{
				hull = CM_ModelHull(model, 5, clip_mins, clip_maxs);
			}
			else
			{
				hull = CM_ModelHull(model, 2, clip_mins, clip_maxs);
			}
		}

		// calculate an offset value to center the origin
		VectorSubtract(clip_mins, mins, offset);
		if (GGameType & GAME_Hexen2 && !(GGameType & GAME_HexenWorld) &&
			(int)move_ent->GetFlags() & QHFL_MONSTER)
		{
			if (offset[0] != 0 || offset[1] != 0)
			{
				offset[0] = 0;
				offset[1] = 0;
			}
		}
		VectorAdd(offset, ent->GetOrigin(), offset);
	}
	else
	{	// create a temp hull from bounding box sizes

		VectorSubtract(ent->GetMins(), maxs, hullmins);
		VectorSubtract(ent->GetMaxs(), mins, hullmaxs);
		hull = CM_TempBoxModel(hullmins, hullmaxs);

		VectorCopy(ent->GetOrigin(), offset);
	}


	return hull;
}

//	Handles selection or creation of a clipping hull, and offseting (and
// eventually rotation) of the end points
static q1trace_t SV_ClipMoveToEntity(qhedict_t* ent, const vec3_t start, const vec3_t mins,
	const vec3_t maxs, const vec3_t end, qhedict_t* move_ent, int move_type)
{
	// fill in a default trace
	q1trace_t trace;
	Com_Memset(&trace, 0, sizeof(q1trace_t));
	trace.fraction = 1;
	trace.allsolid = true;
	trace.entityNum = -1;
	VectorCopy(end, trace.endpos);

	// get the clipping hull
	vec3_t offset;
	clipHandle_t hull = SVQH_HullForEntity(ent, mins, maxs, offset, move_ent);

	vec3_t start_l, end_l;
	VectorSubtract(start, offset, start_l);
	VectorSubtract(end, offset, end_l);

	if (GGameType & GAME_Hexen2 && (GGameType & GAME_HexenWorld || sys_quake2->value))
	{
		// rotate start and end into the models frame of reference
		if (ent->GetSolid() == QHSOLID_BSP &&
			(Q_fabs(ent->GetAngles()[0]) > 1 || Q_fabs(ent->GetAngles()[1]) > 1 || Q_fabs(ent->GetAngles()[2]) > 1))
		{
			vec3_t forward, right, up;
			AngleVectors(ent->GetAngles(), forward, right, up);

			vec3_t temp;
			VectorCopy(start_l, temp);
			start_l[0] = DotProduct(temp, forward);
			start_l[1] = -DotProduct(temp, right);
			start_l[2] = DotProduct(temp, up);

			VectorCopy(end_l, temp);
			end_l[0] = DotProduct(temp, forward);
			end_l[1] = -DotProduct(temp, right);
			end_l[2] = DotProduct(temp, up);
		}
	}

	// trace a line through the apropriate clipping hull
	CM_HullCheckQ1(hull, start_l, end_l, &trace);

	if (GGameType & GAME_Hexen2 && move_type == H2MOVE_WATER)
	{
		if (SVQH_PointContents(trace.endpos) != BSP29CONTENTS_WATER)
		{
			VectorCopy(start_l, trace.endpos);
			trace.fraction = 0;
		}
	}

	if (GGameType & GAME_Hexen2 && (GGameType & GAME_HexenWorld || sys_quake2->value))
	{
		// rotate endpos back to world frame of reference
		if (ent->GetSolid() == QHSOLID_BSP &&
			(Q_fabs(ent->GetAngles()[0]) > 1 || Q_fabs(ent->GetAngles()[1]) > 1 || Q_fabs(ent->GetAngles()[2]) > 1))
		{
			if (trace.fraction != 1)
			{
				vec3_t a;
				VectorSubtract(vec3_origin, ent->GetAngles(), a);
				vec3_t forward, right, up;
				AngleVectors(a, forward, right, up);

				vec3_t temp;
				VectorCopy(trace.endpos, temp);
				trace.endpos[0] = DotProduct(temp, forward);
				trace.endpos[1] = -DotProduct(temp, right);
				trace.endpos[2] = DotProduct(temp, up);

				VectorCopy(trace.plane.normal, temp);
				trace.plane.normal[0] = DotProduct(temp, forward);
				trace.plane.normal[1] = -DotProduct(temp, right);
				trace.plane.normal[2] = DotProduct(temp, up);
			}
		}
	}

	// fix trace up by the offset
	if (trace.fraction != 1)
	{
		VectorAdd(trace.endpos, offset, trace.endpos);
	}

	// did we clip the move?
	if (trace.fraction < 1 || trace.startsolid)
	{
		trace.entityNum = QH_NUM_FOR_EDICT(ent);
	}

	return trace;
}

static void SV_MoveBounds(const vec3_t start, const vec3_t mins, const vec3_t maxs,
	const vec3_t end, vec3_t boxmins, vec3_t boxmaxs)
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

//	Mins and maxs enclose the entire area swept by the move
static void SV_ClipToLinks(const worldSector_t* node, qhmoveclip_t* clip)
{
	// touch linked edicts
	link_t* next;
	for (link_t* l = node->solid_edicts.next; l != &node->solid_edicts; l = next)
	{
		next = l->next;
		qhedict_t* touch = QHEDICT_FROM_AREA(l);
		if (touch->GetSolid() == QHSOLID_NOT)
		{
			continue;
		}
		if (touch == clip->passedict)
		{
			continue;
		}
		if (touch->GetSolid() == QHSOLID_TRIGGER)
		{
			common->Error("Trigger in clipping list");
		}

		if ((clip->type == QHMOVE_NOMONSTERS || (GGameType & GAME_Hexen2 &&
			 clip->type == H2MOVE_PHASE)) && touch->GetSolid() != QHSOLID_BSP)
		{
			continue;
		}

		if (clip->boxmins[0] > touch->v.absmax[0] ||
			clip->boxmins[1] > touch->v.absmax[1] ||
			clip->boxmins[2] > touch->v.absmax[2] ||
			clip->boxmaxs[0] < touch->v.absmin[0] ||
			clip->boxmaxs[1] < touch->v.absmin[1] ||
			clip->boxmaxs[2] < touch->v.absmin[2])
		{
			continue;
		}

		if (clip->passedict && clip->passedict->GetSize()[0] && !touch->GetSize()[0])
		{
			continue;	// points never interact

		}
		// might intersect, so do an exact clip
		if (clip->trace.allsolid)
		{
			return;
		}
		if (clip->passedict)
		{
			if (PROG_TO_EDICT(touch->GetOwner()) == clip->passedict)
			{
				continue;	// don't clip against own missiles
			}
			if (PROG_TO_EDICT(clip->passedict->GetOwner()) == touch)
			{
				continue;	// don't clip against owner
			}
		}

		q1trace_t trace;
		if ((int)touch->GetFlags() & QHFL_MONSTER)
		{
			trace = SV_ClipMoveToEntity(touch, clip->start, clip->mins2, clip->maxs2, clip->end, touch, clip->type);
		}
		else
		{
			trace = SV_ClipMoveToEntity(touch, clip->start, clip->mins, clip->maxs, clip->end, touch, clip->type);
		}
		if (trace.allsolid || trace.startsolid ||
			trace.fraction < clip->trace.fraction)
		{
			trace.entityNum = QH_NUM_FOR_EDICT(touch);
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

	// recurse down both sides
	if (node->axis == -1)
	{
		return;
	}

	if (clip->boxmaxs[node->axis] > node->dist)
	{
		SV_ClipToLinks(node->children[0], clip);
	}
	if (clip->boxmins[node->axis] < node->dist)
	{
		SV_ClipToLinks(node->children[1], clip);
	}
}

q1trace_t SVQH_Move(const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
	int type, qhedict_t* passedict)
{
	qhmoveclip_t clip;
	Com_Memset(&clip, 0, sizeof(qhmoveclip_t));

	// clip to world
	clip.trace = SV_ClipMoveToEntity(sv.qh_edicts, start, mins, maxs, end, passedict, type);

	clip.start = start;
	clip.end = end;
	clip.mins = mins;
	clip.maxs = maxs;
	clip.type = type;
	clip.passedict = passedict;

	if (type == QHMOVE_MISSILE || (GGameType & GAME_Hexen2 && type == H2MOVE_PHASE))
	{
		for (int i = 0; i < 3; i++)
		{
			clip.mins2[i] = -15;
			clip.maxs2[i] = 15;
		}
	}
	else
	{
		VectorCopy(mins, clip.mins2);
		VectorCopy(maxs, clip.maxs2);
	}

	// create the bounding box of the entire move
	SV_MoveBounds(start, clip.mins2, clip.maxs2, end, clip.boxmins, clip.boxmaxs);

	// clip to entities
	SV_ClipToLinks(sv_worldSectors, &clip);

	return clip.trace;
}

//	This could be a lot more efficient...
//	A small wrapper around SV_BoxInSolidEntity that never clips against the
// supplied entity.
qhedict_t* SVQH_TestEntityPosition(qhedict_t* ent)
{
	q1trace_t trace = SVQH_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), ent->GetOrigin(), 0, ent);

	if (trace.startsolid)
	{
		return sv.qh_edicts;
	}

	return NULL;
}
