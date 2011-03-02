/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "quakedef.h"

extern	vec3_t player_mins;
extern	vec3_t player_maxs;

/*
================
PM_TestPlayerPosition

Returns false if the given player position is not valid (in solid)
================
*/
qboolean PM_TestPlayerPosition (vec3_t pos)
{
	int			i;
	physent_t	*pe;
	vec3_t		mins, maxs, test;
	clipHandle_t	hull;

	for (i=0 ; i< pmove.numphysent ; i++)
	{
		pe = &pmove.physents[i];
	// get the clipping hull
		if (pe->model >= 0)
		{
			hull = CM_ModelHull(pmove.physents[i].model, 1);
		}
		else
		{
			VectorSubtract(pe->mins, player_maxs, mins);
			VectorSubtract(pe->maxs, player_mins, maxs);
			hull = CM_TempBoxModel(mins, maxs);
		}

		VectorSubtract(pos, pe->origin, test);

		if (CM_PointContentsQ1(test, hull) == BSP29CONTENTS_SOLID)
			return false;
	}

	return true;
}

/*
================
PM_PlayerMove
================
*/
q1trace_t PM_PlayerMove (vec3_t start, vec3_t end)
{
	q1trace_t	trace, total;
	vec3_t		offset;
	vec3_t		start_l, end_l;
	clipHandle_t	hull;
	int			i;
	physent_t	*pe;
	vec3_t		mins, maxs;

// fill in a default trace
	Com_Memset(&total, 0, sizeof(q1trace_t));
	total.fraction = 1;
	total.entityNum = -1;
	VectorCopy (end, total.endpos);

	for (i=0 ; i< pmove.numphysent ; i++)
	{
		pe = &pmove.physents[i];
		// get the clipping hull
		if (pe->model >= 0)
		{
			vec3_t clip_mins;
			vec3_t clip_maxs;
			hull = CM_ModelHull(pmove.physents[i].model, 1, clip_mins, clip_maxs);
		}
		else
		{
			VectorSubtract(pe->mins, player_maxs, mins);
			VectorSubtract(pe->maxs, player_mins, maxs);
			hull = CM_TempBoxModel(mins, maxs);
		}

	// PM_HullForEntity (ent, mins, maxs, offset);
	VectorCopy (pe->origin, offset);

		VectorSubtract(start, offset, start_l);
		VectorSubtract(end, offset, end_l);

	// fill in a default trace
		Com_Memset(&trace, 0, sizeof(q1trace_t));
		trace.fraction = 1;
		trace.allsolid = true;
//		trace.startsolid = true;
		VectorCopy (end, trace.endpos);

	// trace a line through the apropriate clipping hull
		CM_HullCheckQ1(hull, start_l, end_l, &trace);

		if (trace.allsolid)
			trace.startsolid = true;
		if (trace.startsolid)
			trace.fraction = 0;

	// did we clip the move?
		if (trace.fraction < total.fraction)
		{
			// fix trace up by the offset
			VectorAdd (trace.endpos, offset, trace.endpos);
			total = trace;
			total.entityNum = i;
		}

	}

	return total;
}


