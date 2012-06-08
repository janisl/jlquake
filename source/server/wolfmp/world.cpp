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

void SVWM_UnlinkEntity(wmsharedEntity_t* gEnt)
{
	SVT3_UnlinkEntity(SVWM_EntityForGentity(gEnt), SVWM_SvEntityForGentity(gEnt));
}

void SVWM_LinkEntity(wmsharedEntity_t* gEnt)
{
	SVT3_LinkEntity(SVWM_EntityForGentity(gEnt), SVWM_SvEntityForGentity(gEnt));
}

//	Returns a headnode that can be used for testing or clipping to a
// given entity.  If the entity is a bsp model, the headnode will
// be returned, otherwise a custom box tree will be constructed.
clipHandle_t SVWM_ClipHandleForEntity(const wmsharedEntity_t* ent)
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

void SVWM_ClipToEntity(q3trace_t* trace, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int entityNum, int contentmask, int capsule)
{
	wmsharedEntity_t* touch = SVWM_GentityNum(entityNum);

	Com_Memset(trace, 0, sizeof(q3trace_t));

	// if it doesn't have any brushes of a type we
	// are looking for, ignore it
	if (!(contentmask & touch->r.contents))
	{
		trace->fraction = 1.0;
		return;
	}

	// might intersect, so do an exact clip
	clipHandle_t clipHandle = SVWM_ClipHandleForEntity(touch);

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

void SVWM_ClipMoveToEntities(q3moveclip_t* clip)
{
	int touchlist[MAX_GENTITIES_Q3];
	int num = SVT3_AreaEntities(clip->boxmins, clip->boxmaxs, touchlist, MAX_GENTITIES_Q3);

	int passOwnerNum;
	if (clip->passEntityNum != Q3ENTITYNUM_NONE)
	{
		passOwnerNum = (SVWM_GentityNum(clip->passEntityNum))->r.ownerNum;
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
		wmsharedEntity_t* touch = SVWM_GentityNum(touchlist[i]);

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
		clipHandle_t clipHandle = SVWM_ClipHandleForEntity(touch);

		// DHM - Nerve :: If clipping against BBOX, set to correct contents
		CM_SetTempBoxModelContents(clipHandle, touch->r.contents);

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

		// DHM - Nerve :: Reset contents to default
		CM_SetTempBoxModelContents(clipHandle, BSP46CONTENTS_BODY);
	}
}
