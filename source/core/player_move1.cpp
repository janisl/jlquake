//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "core.h"

movevars_t movevars;
qhplayermove_t qh_pmove;
qhpml_t qh_pml;

vec3_t pmqh_player_mins;
vec3_t pmqh_player_maxs;

vec3_t pmqh_player_maxs_crouch;

void PMQH_Init()
{
	if (GGameType & GAME_Quake)
	{
		VectorSet(pmqh_player_mins, -16, -16, -24);
		VectorSet(pmqh_player_maxs, 16, 16, 32);
	}
	else
	{
		VectorSet(pmqh_player_mins, -16, -16, 0);
		VectorSet(pmqh_player_maxs, 16, 16, 56);
		VectorSet(pmqh_player_maxs_crouch, 16, 16, 28);
	}
}

q1trace_t PMQH_TestPlayerMove(const vec3_t start, const vec3_t end)
{
	// fill in a default trace
	q1trace_t total;
	Com_Memset(&total, 0, sizeof(q1trace_t));
	total.fraction = 1;
	total.entityNum = -1;
	VectorCopy(end, total.endpos);

	for (int i = 0; i < qh_pmove.numphysent; i++)
	{
		qhphysent_t* pe = &qh_pmove.physents[i];
		// get the clipping hull
		clipHandle_t hull;
		vec3_t clip_mins;
		vec3_t clip_maxs;
		if (pe->model >= 0)
		{
			if ((GGameType & GAME_Hexen2) && qh_pmove.crouched)
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
			vec3_t mins, maxs;
			if ((GGameType & GAME_Hexen2) && qh_pmove.crouched)
			{
				VectorSubtract (pe->mins, pmqh_player_maxs_crouch, mins);
			}
			else
			{
				VectorSubtract(pe->mins, pmqh_player_maxs, mins);
			}
			VectorSubtract(pe->maxs, pmqh_player_mins, maxs);
			hull = CM_TempBoxModel(mins, maxs);
			clip_mins[2] = 0;
		}

		vec3_t offset;
		VectorCopy(pe->origin, offset);

		vec3_t start_l, end_l;
		VectorSubtract(start, offset, start_l);
		VectorSubtract(end, offset, end_l);

		if (GGameType & GAME_Hexen2)
		{
			if (pe->model >= 0 && (Q_fabs(pe->angles[0]) > 1 || Q_fabs(pe->angles[1]) > 1 || Q_fabs(pe->angles[2]) > 1))
			{
				vec3_t forward, right, up;
				AngleVectors(pe->angles, forward, right, up);

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

			// rjr will need to adjust for player when going into different hulls
			start_l[2] -= clip_mins[2];
			end_l[2] -= clip_mins[2];
		}

		// fill in a default trace
		q1trace_t trace;
		Com_Memset(&trace, 0, sizeof(q1trace_t));
		trace.fraction = 1;
		trace.allsolid = true;
		VectorCopy(end, trace.endpos);
		if (GGameType & GAME_Hexen2)
		{
			// rjr will need to adjust for player when going into different hulls
			trace.endpos[2] -= clip_mins[2];
		}

		// trace a line through the apropriate clipping hull
		CM_HullCheckQ1(hull, start_l, end_l, &trace);

		if (GGameType & GAME_Hexen2)
		{
			// rjr will need to adjust for player when going into different hulls
			trace.endpos[2] += clip_mins[2];

			if (pe->model >= 0 && (Q_fabs(pe->angles[0]) > 1 || Q_fabs(pe->angles[1]) > 1 || Q_fabs(pe->angles[2]) > 1))
			{
				if (trace.fraction != 1)
				{
					vec3_t a;
					VectorSubtract(vec3_origin, pe->angles, a);
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

		if (trace.allsolid)
		{
			trace.startsolid = true;
		}
		if (trace.startsolid)
		{
			trace.fraction = 0;
		}

		// did we clip the move?
		if (trace.fraction < total.fraction)
		{
			// fix trace up by the offset
			VectorAdd(trace.endpos, offset, trace.endpos);
			total = trace;
			total.entityNum = i;
		}
	}

	return total;
}

//	Returns false if the given player position is not valid (in solid)
bool PMQH_TestPlayerPosition(const vec3_t pos)
{
	if (GGameType & GAME_Hexen2)
	{
		q1trace_t trace = PMQH_TestPlayerMove(pos, pos);
		return !(trace.allsolid || trace.startsolid);
	}

	for (int i = 0; i < qh_pmove.numphysent; i++)
	{
		qhphysent_t* pe = &qh_pmove.physents[i];
		// get the clipping hull
		clipHandle_t hull;
		if (pe->model >= 0)
		{
			hull = CM_ModelHull(qh_pmove.physents[i].model, 1);
		}
		else
		{
			vec3_t mins, maxs;
			VectorSubtract(pe->mins, pmqh_player_maxs, mins);
			VectorSubtract(pe->maxs, pmqh_player_mins, maxs);
			hull = CM_TempBoxModel(mins, maxs);
		}

		vec3_t test;
		VectorSubtract(pos, pe->origin, test);

		if (CM_PointContentsQ1(test, hull) == BSP29CONTENTS_SOLID)
		{
			return false;
		}
	}

	return true;
}
