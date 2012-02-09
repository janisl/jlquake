#include "quakedef.h"

/*
================
PM_TestPlayerPosition

Returns false if the given player position is not valid (in solid)
================
*/
qboolean PM_TestPlayerPosition (vec3_t pos)
{
	int			i;
	qhphysent_t	*pe;
	vec3_t		mins, maxs, test;
	clipHandle_t	hull;
	q1trace_t	trace;

	trace = PM_PlayerMove (pos, pos);
	if (trace.allsolid || trace.startsolid)
	{
		return false;
	}

	return true;

	for (i=0 ; i< qh_pmove.numphysent ; i++)
	{
		pe = &qh_pmove.physents[i];
		vec3_t clip_mins;
		vec3_t clip_maxs;
	// get the clipping hull
		if (pe->model >= 0)
		{
			if(qh_pmove.crouched)
			{
				hull = CM_ModelHull(qh_pmove.physents[i].model, 3, clip_mins, clip_maxs);
			}
			else
			{
				hull = CM_ModelHull(qh_pmove.physents[i].model, 1, clip_mins, clip_maxs);
			}
		}
		else
		{
			if(qh_pmove.crouched)
			{
				VectorSubtract (pe->mins, pmqh_player_maxs_crouch, mins);
			}
			else
			{
				VectorSubtract (pe->mins, pmqh_player_maxs, mins);
			}
			VectorSubtract (pe->maxs, pmqh_player_mins, maxs);
			hull = CM_TempBoxModel(mins, maxs);
			clip_mins[2] = 0;
		}

		VectorSubtract (pos, pe->origin, test);
// rjr will need to adjust for player when going into different hulls
		test[2] -= clip_mins[2];

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
	qhphysent_t	*pe;
	vec3_t		mins, maxs;

// fill in a default trace
	Com_Memset(&total, 0, sizeof(q1trace_t));
	total.fraction = 1;
	total.entityNum = -1;
	VectorCopy (end, total.endpos);

	for (i=0 ; i< qh_pmove.numphysent ; i++)
	{
		pe = &qh_pmove.physents[i];
		// get the clipping hull
		vec3_t clip_mins;
		vec3_t clip_maxs;
		if (pe->model >= 0)
		{
			if(qh_pmove.crouched)
			{
				hull = CM_ModelHull(qh_pmove.physents[i].model, 3, clip_mins, clip_maxs);
			}
			else
			{
				hull = CM_ModelHull(qh_pmove.physents[i].model, 1, clip_mins, clip_maxs);
			}
		}
		else
		{
			if(qh_pmove.crouched)
			{
				VectorSubtract (pe->mins, pmqh_player_maxs_crouch, mins);
			}
			else
			{
				VectorSubtract (pe->mins, pmqh_player_maxs, mins);
			}
			VectorSubtract (pe->maxs, pmqh_player_mins, maxs);
			hull = CM_TempBoxModel(mins, maxs);
			clip_mins[2] = 0;
		}

	// PM_HullForEntity (ent, mins, maxs, offset);
		VectorCopy (pe->origin, offset);

		VectorSubtract (start, offset, start_l);
		VectorSubtract (end, offset, end_l);

		if (pe->model >= 0 && (Q_fabs(pe->angles[0]) > 1 || Q_fabs(pe->angles[1]) > 1 || Q_fabs(pe->angles[2]) > 1) )
		{
			vec3_t	forward, right, up;
			vec3_t	temp;

			AngleVectors (pe->angles, forward, right, up);

			VectorCopy (start_l, temp);
			start_l[0] = DotProduct (temp, forward);
			start_l[1] = -DotProduct (temp, right);
			start_l[2] = DotProduct (temp, up);

			VectorCopy (end_l, temp);
			end_l[0] = DotProduct (temp, forward);
			end_l[1] = -DotProduct (temp, right);
			end_l[2] = DotProduct (temp, up);
		}


// rjr will need to adjust for player when going into different hulls
		start_l[2] -= clip_mins[2];
		end_l[2] -= clip_mins[2];

	// fill in a default trace
		Com_Memset(&trace, 0, sizeof(q1trace_t));
		trace.fraction = 1;
		trace.allsolid = true;
//		trace.startsolid = true;
		VectorCopy (end, trace.endpos);
// rjr will need to adjust for player when going into different hulls
		trace.endpos[2] -= clip_mins[2];

	// trace a line through the apropriate clipping hull
		CM_HullCheckQ1(hull, start_l, end_l, &trace);


// rjr will need to adjust for player when going into different hulls
		trace.endpos[2] += clip_mins[2];

		if (pe->model >= 0 && (Q_fabs(pe->angles[0]) > 1 || Q_fabs(pe->angles[1]) > 1 || Q_fabs(pe->angles[2]) > 1) )
		{
			vec3_t	a;
			vec3_t	forward, right, up;
			vec3_t	temp;

			if (trace.fraction != 1)
			{
				VectorSubtract (vec3_origin, pe->angles, a);
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


