/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

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
	etsharedEntity_t* touch;
	int passOwnerNum;
	q3trace_t trace;
	clipHandle_t clipHandle;
	float* origin, * angles;

	num = SVT3_AreaEntities(clip->boxmins, clip->boxmaxs, touchlist, MAX_GENTITIES_Q3);

	if (clip->passEntityNum != Q3ENTITYNUM_NONE)
	{
		passOwnerNum = (SVET_GentityNum(clip->passEntityNum))->r.ownerNum;
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
		touch = SVET_GentityNum(touchlist[i]);

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
		clipHandle = SVET_ClipHandleForEntity(touch);

		// ydnar: non-worldspawn entities must not use world as clip model!
		if (clipHandle == 0)
		{
			continue;
		}

		// DHM - Nerve :: If clipping against BBOX, set to correct contents
		CM_SetTempBoxModelContents(clipHandle, touch->r.contents);

		origin = touch->r.currentOrigin;
		angles = touch->r.currentAngles;


		if (!touch->r.bmodel)
		{
			angles = vec3_origin;	// boxes don't rotate
		}

#ifdef __MACOS__
		// compiler bug with const
		CM_TransformedBoxTraceQ3(&trace, (float*)clip->start, (float*)clip->end,
			(float*)clip->mins, (float*)clip->maxs, clipHandle,  clip->contentmask,
			origin, angles, clip->capsule);
#else
		CM_TransformedBoxTraceQ3(&trace, clip->start, clip->end,
			clip->mins, clip->maxs, clipHandle,  clip->contentmask,
			origin, angles, clip->capsule);
#endif
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

		// DHM - Nerve :: Reset contents to default
		CM_SetTempBoxModelContents(clipHandle, BSP46CONTENTS_BODY);
	}
}


/*
==================
SV_Trace

Moves the given mins/maxs volume through the world from start to end.
passEntityNum and entities owned by passEntityNum are explicitly not checked.
==================
*/
void SV_Trace(q3trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask, int capsule)
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

	memset(&clip, 0, sizeof(q3moveclip_t));

	// clip to world
	CM_BoxTraceQ3(&clip.trace, start, end, mins, maxs, 0, contentmask, capsule);
	clip.trace.entityNum = clip.trace.fraction != 1.0 ? Q3ENTITYNUM_WORLD : Q3ENTITYNUM_NONE;
	if (clip.trace.fraction == 0 || passEntityNum == -2)
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
	etsharedEntity_t* hit;
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
		hit = SVET_GentityNum(touch[i]);
		// might intersect, so do an exact clip
		clipHandle = SVET_ClipHandleForEntity(hit);

		// ydnar: non-worldspawn entities must not use world as clip model!
		if (clipHandle == 0)
		{
			continue;
		}

		angles = hit->r.currentAngles;
		if (!hit->r.bmodel)
		{
			angles = vec3_origin;	// boxes don't rotate
		}

		c2 = CM_TransformedPointContentsQ3(p, clipHandle, hit->r.currentOrigin, hit->r.currentAngles);
		// Gordon: s.origin/angles is base origin/angles, need to use the current origin/angles for moving entity based water, or water locks in movement start position.
//		c2 = CM_TransformedPointContentsQ3 (p, clipHandle, hit->s.origin, hit->s.angles);

		contents |= c2;
	}

	return contents;
}
