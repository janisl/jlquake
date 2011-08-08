// world.c -- world query functions

/*
 * $Header: /H2 Mission Pack/World.c 12    3/20/98 12:55p Jmonroe $
 */


#include "quakedef.h"

/*

entities never clip against themselves, or their owner

line of sight checks trace->crosscontent, but bullets don't

*/

Cvar*	sys_quake2;

typedef struct
{
	vec3_t		boxmins, boxmaxs;// enclose the test object along entire move
	float		*mins, *maxs;	// size of the moving object
	vec3_t		mins2, maxs2;	// size when clipping against mosnters
	float		*start, *end;
	q1trace_t		trace;
	int			type;
	edict_t		*passedict;
} moveclip_t;


/*
===============================================================================

HULL BOXES

===============================================================================
*/


static	int			move_type;

/*
================
SV_HullForEntity

Returns a hull that can be used for testing or clipping an object of mins/maxs
size.
Offset is filled in to contain the adjustment that must be added to the
testing object's origin to get a point to use with the returned hull.
================
*/
clipHandle_t SV_HullForEntity (edict_t *ent, vec3_t mins, vec3_t maxs, vec3_t offset, edict_t *move_ent)
{
	clipHandle_t	model;
	vec3_t		size;
	vec3_t		hullmins, hullmaxs;
	clipHandle_t	hull;
	int			index;

// decide which clipping hull to use, based on the size
	if (ent->v.solid == SOLID_BSP)
	{	// explicit hulls in the BSP model
		if (ent->v.movetype != MOVETYPE_PUSH)
			Sys_Error ("SOLID_BSP without MOVETYPE_PUSH");

		model = sv.models[(int)ent->v.modelindex];

		if ((int)ent->v.modelindex != 1 && !model)
			Sys_Error ("MOVETYPE_PUSH with a non bsp model");

		VectorSubtract (maxs, mins, size);
//THIS IS WHERE THE MONSTER STEPPING ERROR WAS- IN CHECKBOTTOM,
//A '0 0 0' MINS AND MAXS WERE SENT, BUT HERE, IT LOOKS TO SEE
//IF THE MONSTER HAS A HULL AND CALCULATES THE OFFSET FROM
//THE HULL MINS AND MAXS AND THE PASSED MINS AND MAXS,
//THIS WILL INCORRECTLY OFFSET THE TEST MOVE BY THE MINS AND
//MAXS OF THE MONSTER!  WILL CHECK FOR SIDE EFFECTS...
		vec3_t clip_mins;
		vec3_t clip_maxs;
		if (move_ent->v.hull)  // Entity is specifying which hull to use
		{
			index=move_ent->v.hull-1;
			hull = CM_ModelHull(model, index, clip_mins, clip_maxs);
		}
		else  // Using the old way uses size to determine hull to use
		{
//			if((int)move_ent->v.flags&FL_MONSTER)
//				Con_DPrintf("ERROR: auto-detecing hull for monster!\n");
			if (size[0] < 3) // Point
				hull = CM_ModelHull(model, 0, clip_mins, clip_maxs);
#ifdef MISSIONPACK
			else if ((size[0] <= 8)&&((int)(sv.edicts->v.spawnflags)&1))  // Pentacles
				hull = CM_ModelHull(model, 4, clip_mins, clip_maxs);
#endif
			else if (size[0] <= 32 && size[2] <= 28)  // Half Player
				hull = CM_ModelHull(model, 3, clip_mins, clip_maxs);
			else if (size[0] <= 32)  // Full Player
				hull = CM_ModelHull(model, 1, clip_mins, clip_maxs);
			else // Golem
				hull = CM_ModelHull(model, 5, clip_mins, clip_maxs);
		}
	
// calculate an offset value to center the origin
		VectorSubtract(clip_mins, mins, offset);
		if((int)move_ent->v.flags&FL_MONSTER)
		{
			if(offset[0]!=0||offset[1]!=0)
			{
//				Con_DPrintf("ERROR: Non-zero offset (%f,%f,%f)!!!\n",offset[0],offset[1],offset[2]);
//				if((int)move_ent->v.flags2&524288)
				offset[0]=0;
				offset[1]=0;
			}
		}
		VectorAdd (offset, ent->v.origin, offset);
	}
	else
	{	// create a temp hull from bounding box sizes

		VectorSubtract (ent->v.mins, maxs, hullmins);
		VectorSubtract (ent->v.maxs, mins, hullmaxs);
		hull = CM_TempBoxModel(hullmins, hullmaxs);
		
		VectorCopy (ent->v.origin, offset);
	}


	return hull;
}

/*
===============================================================================

ENTITY AREA CHECKING

===============================================================================
*/

typedef struct areanode_s
{
	int		axis;		// -1 = leaf node
	float	dist;
	struct areanode_s	*children[2];
	link_t	trigger_edicts;
	link_t	solid_edicts;
} areanode_t;

#define	AREA_DEPTH	4
#define	AREA_NODES	32

static	areanode_t	sv_areanodes[AREA_NODES];
static	int			sv_numareanodes;

/*
===============
SV_CreateAreaNode

===============
*/
areanode_t *SV_CreateAreaNode (int depth, vec3_t mins, vec3_t maxs)
{
	areanode_t	*anode;
	vec3_t		size;
	vec3_t		mins1, maxs1, mins2, maxs2;

	anode = &sv_areanodes[sv_numareanodes];
	sv_numareanodes++;

	ClearLink (&anode->trigger_edicts);
	ClearLink (&anode->solid_edicts);
	
	if (depth == AREA_DEPTH)
	{
		anode->axis = -1;
		anode->children[0] = anode->children[1] = NULL;
		return anode;
	}
	
	VectorSubtract (maxs, mins, size);
	if (size[0] > size[1])
		anode->axis = 0;
	else
		anode->axis = 1;
	
	anode->dist = 0.5 * (maxs[anode->axis] + mins[anode->axis]);
	VectorCopy (mins, mins1);	
	VectorCopy (mins, mins2);	
	VectorCopy (maxs, maxs1);	
	VectorCopy (maxs, maxs2);	
	
	maxs1[anode->axis] = mins2[anode->axis] = anode->dist;
	
	anode->children[0] = SV_CreateAreaNode (depth+1, mins2, maxs2);
	anode->children[1] = SV_CreateAreaNode (depth+1, mins1, maxs1);

	return anode;
}

/*
===============
SV_ClearWorld

===============
*/
void SV_ClearWorld (void)
{
	Com_Memset(sv_areanodes, 0, sizeof(sv_areanodes));
	sv_numareanodes = 0;
	vec3_t mins;
	vec3_t maxs;
	CM_ModelBounds(0, mins, maxs);
	SV_CreateAreaNode (0, mins, maxs);
}


/*
===============
SV_UnlinkEdict

===============
*/
void SV_UnlinkEdict (edict_t *ent)
{
	if (!ent->area.prev)
		return;		// not linked in anywhere
	RemoveLink (&ent->area);
	ent->area.prev = ent->area.next = NULL;
}


/*
====================
SV_TouchLinks
====================
*/
void SV_TouchLinks ( edict_t *ent, areanode_t *node )
{
	link_t		*l, *lnext;
	edict_t		*touch;
	int			old_self, old_other;

// touch linked edicts
	for (l = node->trigger_edicts.next ; l != &node->trigger_edicts ; l = lnext)
	{
		if (!l)
		{	//my area got removed out from under me!
			break;
		}
		lnext = l->next;
		touch = EDICT_FROM_AREA(l);
		if (touch == ent)
			continue;
		if (!touch->v.touch || touch->v.solid != SOLID_TRIGGER)
			continue;
		if (ent->v.absmin[0] > touch->v.absmax[0]
		|| ent->v.absmin[1] > touch->v.absmax[1]
		|| ent->v.absmin[2] > touch->v.absmax[2]
		|| ent->v.absmax[0] < touch->v.absmin[0]
		|| ent->v.absmax[1] < touch->v.absmin[1]
		|| ent->v.absmax[2] < touch->v.absmin[2] )
			continue;
			
		old_self = pr_global_struct->self;
		old_other = pr_global_struct->other;

		pr_global_struct->self = EDICT_TO_PROG(touch);
		pr_global_struct->other = EDICT_TO_PROG(ent);
		pr_global_struct->time = sv.time;
		PR_ExecuteProgram (touch->v.touch);

		pr_global_struct->self = old_self;
		pr_global_struct->other = old_other;
	}
	
// recurse down both sides
	if (node->axis == -1)
		return;
	
	if ( ent->v.absmax[node->axis] > node->dist )
		SV_TouchLinks ( ent, node->children[0] );
	if ( ent->v.absmin[node->axis] < node->dist )
		SV_TouchLinks ( ent, node->children[1] );
}

/*
===============
SV_LinkEdict

===============
*/
void SV_LinkEdict (edict_t *ent, qboolean touch_triggers)
{
	areanode_t	*node;

	if (ent->area.prev)
		SV_UnlinkEdict (ent);	// unlink from old position
		
	if (ent == sv.edicts)
		return;		// don't add the world

	if (ent->free)
		return;

// set the abs box

	if (ent->v.solid == SOLID_BSP && 
	(ent->v.angles[0] || ent->v.angles[1] || ent->v.angles[2]) )
	{	// expand for rotation
		float		max, v;
		int			i;

		max = 0;
		for (i=0 ; i<3 ; i++)
		{
			v =Q_fabs( ent->v.mins[i]);
			if (v > max)
				max = v;
			v =Q_fabs( ent->v.maxs[i]);
			if (v > max)
				max = v;
		}
		for (i=0 ; i<3 ; i++)
		{
			ent->v.absmin[i] = ent->v.origin[i] - max;
			ent->v.absmax[i] = ent->v.origin[i] + max;
		}
	}
	else
	{
		VectorAdd (ent->v.origin, ent->v.mins, ent->v.absmin);	
		VectorAdd (ent->v.origin, ent->v.maxs, ent->v.absmax);
	}

//
// to make items easier to pick up and allow them to be grabbed off
// of shelves, the abs sizes are expanded
//
	if ((int)ent->v.flags & FL_ITEM)
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

	if (ent->v.solid == SOLID_NOT)
		return;

// find the first node that the ent's box crosses
	node = sv_areanodes;
	while (1)
	{
		if (node->axis == -1)
			break;
		if (ent->v.absmin[node->axis] > node->dist)
			node = node->children[0];
		else if (ent->v.absmax[node->axis] < node->dist)
			node = node->children[1];
		else
			break;		// crosses the node
	}
	
// link it in	

	if (ent->v.solid == SOLID_TRIGGER)
		InsertLinkBefore (&ent->area, &node->trigger_edicts);
	else
		InsertLinkBefore (&ent->area, &node->solid_edicts);
	
// if touch_triggers, touch all entities at this node and decend for more
	if (touch_triggers)
		SV_TouchLinks ( ent, sv_areanodes );
}



/*
==================
SV_PointContents

==================
*/
int SV_PointContents (vec3_t p)
{
	int cont = CM_PointContentsQ1(p, 0);
	if (cont <= BSP29CONTENTS_CURRENT_0 && cont >= BSP29CONTENTS_CURRENT_DOWN)
		cont = BSP29CONTENTS_WATER;
	return cont;
}

//===========================================================================

/*
============
SV_TestEntityPosition

This could be a lot more efficient...

FIXME!!!  For rotating doors, this is totally inaccurate 
============
*/
edict_t	*SV_TestEntityPosition (edict_t *ent)
{
	q1trace_t	trace;

	trace = SV_Move (ent->v.origin, ent->v.mins, ent->v.maxs, ent->v.origin, 0, ent);
	
	if (trace.startsolid)
	{
//		Con_DPrintf("%s inside check\n", PR_GetString(trace.ent->v.classname));
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
q1trace_t SV_ClipMoveToEntity (edict_t *ent, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, edict_t *move_ent)
{
	q1trace_t		trace;
	vec3_t		offset;
	vec3_t		start_l, end_l;
	clipHandle_t	hull;

// fill in a default trace
	Com_Memset(&trace, 0, sizeof(q1trace_t));
	trace.fraction = 1;
	trace.allsolid = true;
	trace.entityNum = -1;
	VectorCopy (end, trace.endpos);

// get the clipping hull
	hull = SV_HullForEntity (ent, mins, maxs, offset, move_ent);

	VectorSubtract (start, offset, start_l);
	VectorSubtract (end, offset, end_l);

	if (sys_quake2->value)
	{
		// rotate start and end into the models frame of reference
		if (ent->v.solid == SOLID_BSP && 
		(Q_fabs(ent->v.angles[0]) > 1 || Q_fabs(ent->v.angles[1]) > 1 || Q_fabs(ent->v.angles[2]) > 1) )
		{
			vec3_t	forward, right, up;
			vec3_t	temp;

			AngleVectors (ent->v.angles, forward, right, up);

			VectorCopy (start_l, temp);
			start_l[0] = DotProduct (temp, forward);
			start_l[1] = -DotProduct (temp, right);
			start_l[2] = DotProduct (temp, up);

			VectorCopy (end_l, temp);
			end_l[0] = DotProduct (temp, forward);
			end_l[1] = -DotProduct (temp, right);
			end_l[2] = DotProduct (temp, up);
		}
	}

// trace a line through the apropriate clipping hull
	CM_HullCheckQ1(hull, start_l, end_l, &trace);
	if (move_type == MOVE_WATER)
	{
		if (SV_PointContents (trace.endpos) != BSP29CONTENTS_WATER)
		{
			VectorCopy(start_l, trace.endpos);
			trace.fraction = 0;
		}
	}

	if (sys_quake2->value)
	{
		// rotate endpos back to world frame of reference
		if (ent->v.solid == SOLID_BSP && 
		(Q_fabs(ent->v.angles[0]) > 1 || Q_fabs(ent->v.angles[1]) > 1 || Q_fabs(ent->v.angles[2]) > 1) )
		{
			vec3_t	a;
			vec3_t	forward, right, up;
			vec3_t	temp;

			if (trace.fraction != 1)
			{
				VectorSubtract (vec3_origin, ent->v.angles, a);
				AngleVectors (a, forward, right, up);

				VectorCopy (trace.endpos, temp);
				trace.endpos[0] = DotProduct (temp, forward);
				trace.endpos[1] = -DotProduct (temp, right);
				trace.endpos[2] = DotProduct (temp, up);

				VectorCopy (trace.plane.normal, temp);
				trace.plane.normal[0] = DotProduct (temp, forward);
				trace.plane.normal[1] = -DotProduct (temp, right);
				trace.plane.normal[2] = DotProduct (temp, up);
			}
		}
	}

// fix trace up by the offset
	if (trace.fraction != 1)
		VectorAdd (trace.endpos, offset, trace.endpos);

// did we clip the move?
	if (trace.fraction < 1 || trace.startsolid  )
		trace.entityNum = NUM_FOR_EDICT(ent);

	return trace;
}

//===========================================================================

/*
====================
SV_ClipToLinks

Mins and maxs enclose the entire area swept by the move
====================
*/
void SV_ClipToLinks ( areanode_t *node, moveclip_t *clip )
{
	link_t		*l, *next;
	edict_t		*touch;
	q1trace_t		trace;

// touch linked edicts
	for (l = node->solid_edicts.next ; l != &node->solid_edicts ; l = next)
	{
		next = l->next;
		touch = EDICT_FROM_AREA(l);
		if (touch->v.solid == SOLID_NOT)
			continue;
		if (touch == clip->passedict)
			continue;
		if (touch->v.solid == SOLID_TRIGGER)
			Sys_Error ("Trigger in clipping list (%s)", PR_GetString(touch->v.classname));

		if ((clip->type == MOVE_NOMONSTERS ||
			 clip->type == MOVE_PHASE) && touch->v.solid != SOLID_BSP)
			continue;

		if (clip->boxmins[0] > touch->v.absmax[0]
		|| clip->boxmins[1] > touch->v.absmax[1]
		|| clip->boxmins[2] > touch->v.absmax[2]
		|| clip->boxmaxs[0] < touch->v.absmin[0]
		|| clip->boxmaxs[1] < touch->v.absmin[1]
		|| clip->boxmaxs[2] < touch->v.absmin[2] )
			continue;

		if (clip->passedict && clip->passedict->v.size[0] && !touch->v.size[0])
			continue;	// points never interact

	// might intersect, so do an exact clip
		if (clip->trace.allsolid)
			return;
		if (clip->passedict)
		{
		 	if (PROG_TO_EDICT(touch->v.owner) == clip->passedict)
				continue;	// don't clip against own missiles
			if (PROG_TO_EDICT(clip->passedict->v.owner) == touch)
				continue;	// don't clip against owner
		}

		if ((int)touch->v.flags & FL_MONSTER)
			trace = SV_ClipMoveToEntity (touch, clip->start, clip->mins2, clip->maxs2, clip->end, touch);
		else
			trace = SV_ClipMoveToEntity (touch, clip->start, clip->mins, clip->maxs, clip->end, touch);
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
				clip->trace = trace;
		}
		else if (trace.startsolid)
			clip->trace.startsolid = true;
	}
	
// recurse down both sides
	if (node->axis == -1)
		return;

	if ( clip->boxmaxs[node->axis] > node->dist )
		SV_ClipToLinks ( node->children[0], clip );
	if ( clip->boxmins[node->axis] < node->dist )
		SV_ClipToLinks ( node->children[1], clip );
}


/*
==================
SV_MoveBounds
==================
*/
void SV_MoveBounds (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, vec3_t boxmins, vec3_t boxmaxs)
{
#if 0
// debug to test against everything
boxmins[0] = boxmins[1] = boxmins[2] = -9999;
boxmaxs[0] = boxmaxs[1] = boxmaxs[2] = 9999;
#else
	int		i;
	
	for (i=0 ; i<3 ; i++)
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
q1trace_t SV_Move (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int type, edict_t *passedict)
{
	moveclip_t	clip;
	int			i;

//	type = MOVE_WATER;
	Com_Memset( &clip, 0, sizeof ( moveclip_t ) );

	move_type = type;
// clip to world
	clip.trace = SV_ClipMoveToEntity ( sv.edicts, start, mins, maxs, end, passedict );

	clip.start = start;
	clip.end = end;
	clip.mins = mins;
	clip.maxs = maxs;
	clip.type = type;
	clip.passedict = passedict;

	if (type == MOVE_MISSILE || type == MOVE_PHASE)
	{//Larger for projectiles against monsters
		for (i=0 ; i<3 ; i++)
		{
			clip.mins2[i] = -15;
			clip.maxs2[i] = 15;
		}
	}
	else
	{
		VectorCopy (mins, clip.mins2);
		VectorCopy (maxs, clip.maxs2);
	}
	
// create the bounding box of the entire move
	SV_MoveBounds ( start, clip.mins2, clip.maxs2, end, clip.boxmins, clip.boxmaxs );

// clip to entities
	SV_ClipToLinks ( sv_areanodes, &clip );

	return clip.trace;
}
