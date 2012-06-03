// world.c -- world query functions

#include "qwsvdef.h"

/*

entities never clip against themselves, or their owner

line of sight checks trace->crosscontent, but bullets don't

*/


typedef struct
{
	vec3_t boxmins, boxmaxs;	// enclose the test object along entire move
	float* mins, * maxs;		// size of the moving object
	vec3_t mins2, maxs2;		// size when clipping against mosnters
	float* start, * end;
	q1trace_t trace;
	int type;
	qhedict_t* passedict;
} moveclip_t;


/*
===============================================================================

HULL BOXES

===============================================================================
*/


static int move_type;

/*
================
SV_HullForEntity

Returns a hull that can be used for testing or clipping an object of mins/maxs
size.
Offset is filled in to contain the adjustment that must be added to the
testing object's origin to get a point to use with the returned hull.
================
*/
clipHandle_t SV_HullForEntity(qhedict_t* ent, vec3_t mins, vec3_t maxs, vec3_t offset, qhedict_t* move_ent)
{
	clipHandle_t model;
	vec3_t size;
	vec3_t hullmins, hullmaxs;
	clipHandle_t hull;
	int index;

// decide which clipping hull to use, based on the size
	if (ent->GetSolid() == SOLID_BSP)
	{	// explicit hulls in the BSP model
		if (ent->GetMoveType() != QHMOVETYPE_PUSH)
		{
			SV_Error("SOLID_BSP without QHMOVETYPE_PUSH");
		}

		model = sv.models[(int)ent->v.modelindex];

		if (ent->v.modelindex != 1 && !model)
		{
			SV_Error("QHMOVETYPE_PUSH with a non bsp model");
		}

		VectorSubtract(maxs, mins, size);
		vec3_t clip_mins;
		vec3_t clip_maxs;
		if (move_ent->GetHull())	// Entity is specifying which hull to use
		{
			index = move_ent->GetHull() - 1;
			hull = CM_ModelHull(model, index, clip_mins, clip_maxs);
		}
		else	// Using the old way uses size to determine hull to use
		{

			if (size[0] < 3)// Point
			{
				hull = CM_ModelHull(model, 0, clip_mins, clip_maxs);
			}
			else if (size[0] <= 32 && size[2] <= 28)	// Half Player
			{
				hull = CM_ModelHull(model, 3, clip_mins, clip_maxs);
			}
			else if (size[0] <= 32)	// Full Player
			{
				hull = CM_ModelHull(model, 1, clip_mins, clip_maxs);
			}
			else// Golumn
			{
				hull = CM_ModelHull(model, 5, clip_mins, clip_maxs);
			}
		}

// calculate an offset value to center the origin
//		if(!move_ent->v.flags&262144)//FL_IGNORESIZEOFS
//		{
		VectorSubtract(clip_mins, mins, offset);
		VectorAdd(offset, ent->GetOrigin(), offset);
//		}//otherwise, just make it use it's specified hull
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

/*
===============================================================================

ENTITY AREA CHECKING

===============================================================================
*/


areanode_t sv_areanodes[AREA_NODES];
int sv_numareanodes;

/*
===============
SV_CreateAreaNode

===============
*/
areanode_t* SV_CreateAreaNode(int depth, vec3_t mins, vec3_t maxs)
{
	areanode_t* anode;
	vec3_t size;
	vec3_t mins1, maxs1, mins2, maxs2;

	anode = &sv_areanodes[sv_numareanodes];
	sv_numareanodes++;

	ClearLink(&anode->trigger_edicts);
	ClearLink(&anode->solid_edicts);

	if (depth == AREA_DEPTH)
	{
		anode->axis = -1;
		anode->children[0] = anode->children[1] = NULL;
		return anode;
	}

	VectorSubtract(maxs, mins, size);
	if (size[0] > size[1])
	{
		anode->axis = 0;
	}
	else
	{
		anode->axis = 1;
	}

	anode->dist = 0.5 * (maxs[anode->axis] + mins[anode->axis]);
	VectorCopy(mins, mins1);
	VectorCopy(mins, mins2);
	VectorCopy(maxs, maxs1);
	VectorCopy(maxs, maxs2);

	maxs1[anode->axis] = mins2[anode->axis] = anode->dist;

	anode->children[0] = SV_CreateAreaNode(depth + 1, mins2, maxs2);
	anode->children[1] = SV_CreateAreaNode(depth + 1, mins1, maxs1);

	return anode;
}

/*
===============
SV_ClearWorld

===============
*/
void SV_ClearWorld(void)
{
	Com_Memset(sv_areanodes, 0, sizeof(sv_areanodes));
	sv_numareanodes = 0;
	vec3_t mins;
	vec3_t maxs;
	CM_ModelBounds(0, mins, maxs);
	SV_CreateAreaNode(0, mins, maxs);
}


/*
===============
SV_UnlinkEdict

===============
*/
void SV_UnlinkEdict(qhedict_t* ent)
{
	if (!ent->area.prev)
	{
		return;		// not linked in anywhere
	}
	RemoveLink(&ent->area);
	ent->area.prev = ent->area.next = NULL;
}


/*
====================
SV_TouchLinks
====================
*/
void SV_TouchLinks(qhedict_t* ent, areanode_t* node)
{
	link_t* l, * next;
	qhedict_t* touch;
	int old_self, old_other;

// touch linked edicts
	for (l = node->trigger_edicts.next; l != &node->trigger_edicts; l = next)
	{
		next = l->next;
		touch = EDICT_FROM_AREA(l);
		if (touch == ent)
		{
			continue;
		}
		if (!touch->GetTouch() || touch->GetSolid() != SOLID_TRIGGER)
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

		old_self = pr_global_struct->self;
		old_other = pr_global_struct->other;

		pr_global_struct->self = EDICT_TO_PROG(touch);
		pr_global_struct->other = EDICT_TO_PROG(ent);
		pr_global_struct->time = sv.time;
		PR_ExecuteProgram(touch->GetTouch());

		pr_global_struct->self = old_self;
		pr_global_struct->other = old_other;
	}

// recurse down both sides
	if (node->axis == -1)
	{
		return;
	}

	if (ent->v.absmax[node->axis] > node->dist)
	{
		SV_TouchLinks(ent, node->children[0]);
	}
	if (ent->v.absmin[node->axis] < node->dist)
	{
		SV_TouchLinks(ent, node->children[1]);
	}
}

/*
===============
SV_LinkEdict

===============
*/
void SV_LinkEdict(qhedict_t* ent, qboolean touch_triggers)
{
	areanode_t* node;

	if (ent->area.prev)
	{
		SV_UnlinkEdict(ent);	// unlink from old position

	}
	if (ent == sv.edicts)
	{
		return;		// don't add the world

	}
	if (ent->free)
	{
		return;
	}

// set the abs box
	if (ent->GetSolid() == SOLID_BSP &&
		(ent->GetAngles()[0] || ent->GetAngles()[1] || ent->GetAngles()[2]))
	{	// expand for rotation
		float max, v;
		int i;

		max = 0;
		for (i = 0; i < 3; i++)
		{
			v = Q_fabs(ent->GetMins()[i]);
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
		for (i = 0; i < 3; i++)
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
	if ((int)ent->GetFlags() & FL_ITEM)
	{
		ent->v.absmin[0] -= 15;
		ent->v.absmin[1] -= 15;
		ent->v.absmax[0] += 15;
		ent->v.absmax[1] += 15;
	}
	else
	{	// because movement is clipped an epsilon away from an actual edge,
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

	if (ent->GetSolid() == SOLID_NOT)
	{
		return;
	}

// find the first node that the ent's box crosses
	node = sv_areanodes;
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

	if (ent->GetSolid() == SOLID_TRIGGER)
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
		SV_TouchLinks(ent, sv_areanodes);
	}
}



/*
==================
SV_PointContents

==================
*/
int SV_PointContents(vec3_t p)
{
	int cont = CM_PointContentsQ1(p, 0);
	if (cont <= BSP29CONTENTS_CURRENT_0 && cont >= BSP29CONTENTS_CURRENT_DOWN)
	{
		cont = BSP29CONTENTS_WATER;
	}
	return cont;
}

//===========================================================================

/*
============
SV_TestEntityPosition

A small wrapper around SV_BoxInSolidEntity that never clips against the
supplied entity.
============
*/
qhedict_t* SV_TestEntityPosition(qhedict_t* ent)
{
	q1trace_t trace;

	trace = SV_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), ent->GetOrigin(), 0, ent);

	if (trace.startsolid)
	{
		return sv.edicts;
	}

	return NULL;
}

/*
==================
SV_ClipMoveToEntity

Handles selection or creation of a clipping hull, and offseting (and
eventually rotation) of the end points
==================
*/
q1trace_t SV_ClipMoveToEntity(qhedict_t* ent, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, qhedict_t* move_ent)
{
	q1trace_t trace;
	vec3_t offset;
	vec3_t start_l, end_l;
	clipHandle_t hull;

// fill in a default trace
	Com_Memset(&trace, 0, sizeof(q1trace_t));
	trace.fraction = 1;
	trace.allsolid = true;
	trace.entityNum = -1;
	VectorCopy(end, trace.endpos);

// get the clipping hull
	hull = SV_HullForEntity(ent, mins, maxs, offset, move_ent);

	VectorSubtract(start, offset, start_l);
	VectorSubtract(end, offset, end_l);

	// rotate start and end into the models frame of reference
	if (ent->GetSolid() == SOLID_BSP &&
		(Q_fabs(ent->GetAngles()[0]) > 1 || Q_fabs(ent->GetAngles()[1]) > 1 || Q_fabs(ent->GetAngles()[2]) > 1))
	{
		vec3_t forward, right, up;
		vec3_t temp;

		AngleVectors(ent->GetAngles(), forward, right, up);

		VectorCopy(start_l, temp);
		start_l[0] = DotProduct(temp, forward);
		start_l[1] = -DotProduct(temp, right);
		start_l[2] = DotProduct(temp, up);

		VectorCopy(end_l, temp);
		end_l[0] = DotProduct(temp, forward);
		end_l[1] = -DotProduct(temp, right);
		end_l[2] = DotProduct(temp, up);
	}

// trace a line through the apropriate clipping hull
	CM_HullCheckQ1(hull, start_l, end_l, &trace);

	if (move_type == MOVE_WATER)
	{
		if (SV_PointContents(trace.endpos) != BSP29CONTENTS_WATER)
		{
			VectorCopy(start_l, trace.endpos);
			trace.fraction = 0;
		}
	}

	// rotate endpos back to world frame of reference
	if (ent->GetSolid() == SOLID_BSP &&
		(Q_fabs(ent->GetAngles()[0]) > 1 || Q_fabs(ent->GetAngles()[1]) > 1 || Q_fabs(ent->GetAngles()[2]) > 1))
	{
		vec3_t a;
		vec3_t forward, right, up;
		vec3_t temp;

		if (trace.fraction != 1)
		{
			VectorSubtract(vec3_origin, ent->GetAngles(), a);
			AngleVectors(a, forward, right, up);

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

// fix trace up by the offset
	if (trace.fraction != 1)
	{
		VectorAdd(trace.endpos, offset, trace.endpos);
	}

// did we clip the move?
	if (trace.fraction < 1 || trace.startsolid)
	{
		trace.entityNum = NUM_FOR_EDICT(ent);
	}

	return trace;
}

//===========================================================================

/*
====================
SV_ClipToLinks

Mins and maxs enclose the entire area swept by the move
====================
*/
void SV_ClipToLinks(areanode_t* node, moveclip_t* clip)
{
	link_t* l, * next;
	qhedict_t* touch;
	q1trace_t trace;

// touch linked edicts
	for (l = node->solid_edicts.next; l != &node->solid_edicts; l = next)
	{
		next = l->next;
		touch = EDICT_FROM_AREA(l);
		if (touch->GetSolid() == SOLID_NOT)
		{
			continue;
		}
		if (touch == clip->passedict)
		{
			continue;
		}
		if (touch->GetSolid() == SOLID_TRIGGER)
		{
			SV_Error("Trigger in clipping list");
		}

		if ((clip->type == MOVE_NOMONSTERS ||
			 clip->type == MOVE_PHASE) && touch->GetSolid() != SOLID_BSP)
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

		if ((int)touch->GetFlags() & FL_MONSTER)
		{
			trace = SV_ClipMoveToEntity(touch, clip->start, clip->mins2, clip->maxs2, clip->end, touch);
		}
		else
		{
			trace = SV_ClipMoveToEntity(touch, clip->start, clip->mins, clip->maxs, clip->end, touch);
		}
		if (trace.allsolid || trace.startsolid ||
			trace.fraction < clip->trace.fraction)
		{
			trace.entityNum = NUM_FOR_EDICT(touch);
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


/*
==================
SV_MoveBounds
==================
*/
void SV_MoveBounds(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, vec3_t boxmins, vec3_t boxmaxs)
{
#if 0
// debug to test against everything
	boxmins[0] = boxmins[1] = boxmins[2] = -9999;
	boxmaxs[0] = boxmaxs[1] = boxmaxs[2] = 9999;
#else
	int i;

	for (i = 0; i < 3; i++)
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
#endif
}

/*
==================
SV_Move
==================
*/
q1trace_t SV_Move(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int type, qhedict_t* passedict)
{
	moveclip_t clip;
	int i;

	Com_Memset(&clip, 0, sizeof(moveclip_t));

	move_type = type;
// clip to world
	clip.trace = SV_ClipMoveToEntity(sv.edicts, start, mins, maxs, end, passedict);

	clip.start = start;
	clip.end = end;
	clip.mins = mins;
	clip.maxs = maxs;
	clip.type = type;
	clip.passedict = passedict;

	if (type == MOVE_MISSILE || type == MOVE_PHASE)
	{
		for (i = 0; i < 3; i++)
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
	SV_ClipToLinks(sv_areanodes, &clip);

	return clip.trace;
}

//=============================================================================

/*
============
SV_TestPlayerPosition

============
*/
qhedict_t* SV_TestPlayerPosition(qhedict_t* ent, vec3_t origin)
{
	clipHandle_t hull;
	qhedict_t* check;
	vec3_t boxmins, boxmaxs;
	vec3_t offset;
	int e;

// check world first
	hull = CM_ModelHull(0, 1);
	if (CM_PointContentsQ1(origin, hull) != BSP29CONTENTS_EMPTY)
	{
		return sv.edicts;
	}

// check all entities
	VectorAdd(origin, ent->GetMins(), boxmins);
	VectorAdd(origin, ent->GetMaxs(), boxmaxs);

	check = NEXT_EDICT(sv.edicts);
	for (e = 1; e < sv.num_edicts; e++, check = NEXT_EDICT(check))
	{
		if (check->free)
		{
			continue;
		}
		if (check->GetSolid() != SOLID_BSP &&
			check->GetSolid() != SOLID_BBOX &&
			check->GetSolid() != SOLID_SLIDEBOX)
		{
			continue;
		}

		if (boxmins[0] > check->v.absmax[0] ||
			boxmins[1] > check->v.absmax[1] ||
			boxmins[2] > check->v.absmax[2] ||
			boxmaxs[0] < check->v.absmin[0] ||
			boxmaxs[1] < check->v.absmin[1] ||
			boxmaxs[2] < check->v.absmin[2])
		{
			continue;
		}

		if (check == ent)
		{
			continue;
		}

		// get the clipping hull
		hull = SV_HullForEntity(check, ent->GetMins(), ent->GetMaxs(), offset, ent);

		VectorSubtract(origin, offset, offset);

		// test the point
		if (CM_PointContentsQ1(offset, hull) != BSP29CONTENTS_EMPTY)
		{
			return check;
		}
	}

	return NULL;
}
