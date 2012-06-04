/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// world.c -- world query functions

#include "server.h"

/*
====================
SV_ClipMoveToEntities

====================
*/
void SV_ClipMoveToEntities(q3moveclip_t* clip)
{
	int i, num;
	int touchlist[MAX_GENTITIES_Q3];
	q3sharedEntity_t* touch;
	int passOwnerNum;
	q3trace_t trace;
	clipHandle_t clipHandle;
	float* origin, * angles;

	num = SVT3_AreaEntities(clip->boxmins, clip->boxmaxs, touchlist, MAX_GENTITIES_Q3);

	if (clip->passEntityNum != Q3ENTITYNUM_NONE)
	{
		passOwnerNum = (SVQ3_GentityNum(clip->passEntityNum))->r.ownerNum;
		if (passOwnerNum == Q3ENTITYNUM_NONE)
		{
			passOwnerNum = -1;
		}
	}
	else
	{
		passOwnerNum = -1;
	}

	for (i = 0; i < num; i++)
	{
		if (clip->trace.allsolid)
		{
			return;
		}
		touch = SVQ3_GentityNum(touchlist[i]);

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

		// might intersect, so do an exact clip
		clipHandle = SVQ3_ClipHandleForEntity(touch);

		origin = touch->r.currentOrigin;
		angles = touch->r.currentAngles;


		if (!touch->r.bmodel)
		{
			angles = vec3_origin;	// boxes don't rotate
		}

		CM_TransformedBoxTraceQ3(&trace, (float*)clip->start, (float*)clip->end,
			(float*)clip->mins, (float*)clip->maxs, clipHandle,  clip->contentmask,
			origin, angles, clip->capsule);

		if (trace.allsolid)
		{
			clip->trace.allsolid = qtrue;
			trace.entityNum = touch->s.number;
		}
		else if (trace.startsolid)
		{
			clip->trace.startsolid = qtrue;
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


/*
==================
SV_Trace

Moves the given mins/maxs volume through the world from start to end.
passEntityNum and entities owned by passEntityNum are explicitly not checked.
==================
*/
void SV_Trace(q3trace_t* results, const vec3_t start, vec3_t mins, vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask, int capsule)
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
	if (clip.trace.fraction == 0)
	{
		*results = clip.trace;
		return;		// blocked immediately by the world
	}

	clip.contentmask = contentmask;
	clip.start = start;
//	VectorCopy( clip.trace.endpos, clip.end );
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
	SV_ClipMoveToEntities(&clip);

	*results = clip.trace;
}



/*
=============
SV_PointContents
=============
*/
int SV_PointContents(const vec3_t p, int passEntityNum)
{
	int touch[MAX_GENTITIES_Q3];
	q3sharedEntity_t* hit;
	int i, num;
	int contents, c2;
	clipHandle_t clipHandle;
	float* angles;

	// get base contents from world
	contents = CM_PointContentsQ3(p, 0);

	// or in contents from all the other entities
	num = SVT3_AreaEntities(p, p, touch, MAX_GENTITIES_Q3);

	for (i = 0; i < num; i++)
	{
		if (touch[i] == passEntityNum)
		{
			continue;
		}
		hit = SVQ3_GentityNum(touch[i]);
		// might intersect, so do an exact clip
		clipHandle = SVQ3_ClipHandleForEntity(hit);
		angles = hit->s.angles;
		if (!hit->r.bmodel)
		{
			angles = vec3_origin;	// boxes don't rotate
		}

		c2 = CM_TransformedPointContentsQ3(p, clipHandle, hit->s.origin, hit->s.angles);

		contents |= c2;
	}

	return contents;
}
