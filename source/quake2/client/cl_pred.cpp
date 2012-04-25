/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "client.h"


/*
===================
CL_CheckPredictionError
===================
*/
void CL_CheckPredictionError(void)
{
	int frame;
	int delta[3];
	int i;
	int len;

	if (!cl_predict->value || (cl.q2_frame.playerstate.pmove.pm_flags & Q2PMF_NO_PREDICTION))
	{
		return;
	}

	// calculate the last q2usercmd_t we sent that the server has processed
	frame = clc.netchan.incomingAcknowledged;
	frame &= (CMD_BACKUP_Q2 - 1);

	// compare what the server returned with what we had predicted it to be
	VectorSubtract(cl.q2_frame.playerstate.pmove.origin, cl.q2_predicted_origins[frame], delta);

	// save the prediction error for interpolation
	len = abs(delta[0]) + abs(delta[1]) + abs(delta[2]);
	if (len > 640)	// 80 world units
	{	// a teleport or something
		VectorClear(cl.q2_prediction_error);
	}
	else
	{
		if (cl_showmiss->value && (delta[0] || delta[1] || delta[2]))
		{
			Com_Printf("prediction miss on %i: %i\n", cl.q2_frame.serverframe,
				delta[0] + delta[1] + delta[2]);
		}

		VectorCopy(cl.q2_frame.playerstate.pmove.origin, cl.q2_predicted_origins[frame]);

		// save for error itnerpolation
		for (i = 0; i < 3; i++)
			cl.q2_prediction_error[i] = delta[i] * 0.125;
	}
}


/*
====================
CL_ClipMoveToEntities

====================
*/
void CL_ClipMoveToEntities(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, q2trace_t* tr)
{
	int i, x, zd, zu;
	q2trace_t trace;
	float* angles;
	q2entity_state_t* ent;
	int num;
	clipHandle_t cmodel;
	vec3_t bmins, bmaxs;

	for (i = 0; i < cl.q2_frame.num_entities; i++)
	{
		num = (cl.q2_frame.parse_entities + i) & (MAX_PARSE_ENTITIES - 1);
		ent = &cl_parse_entities[num];

		if (!ent->solid)
		{
			continue;
		}

		if (ent->number == cl.playernum + 1)
		{
			continue;
		}

		if (ent->solid == 31)
		{	// special value for bmodel
			cmodel = cl.model_clip[ent->modelindex];
			if (!cmodel)
			{
				continue;
			}
			angles = ent->angles;
		}
		else
		{	// encoded bbox
			x = 8 * (ent->solid & 31);
			zd = 8 * ((ent->solid >> 5) & 31);
			zu = 8 * ((ent->solid >> 10) & 63) - 32;

			bmins[0] = bmins[1] = -x;
			bmaxs[0] = bmaxs[1] = x;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			cmodel = CM_TempBoxModel(bmins, bmaxs);
			angles = vec3_origin;	// boxes don't rotate
		}

		if (tr->allsolid)
		{
			return;
		}

		trace = CM_TransformedBoxTraceQ2(start, end,
			mins, maxs, cmodel,  MASK_PLAYERSOLID,
			ent->origin, angles);

		if (trace.allsolid || trace.startsolid ||
			trace.fraction < tr->fraction)
		{
			trace.ent = (struct edict_t*)ent;
			if (tr->startsolid)
			{
				*tr = trace;
				tr->startsolid = true;
			}
			else
			{
				*tr = trace;
			}
		}
		else if (trace.startsolid)
		{
			tr->startsolid = true;
		}
	}
}


/*
================
CL_PMTrace
================
*/
q2trace_t       CL_PMTrace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	q2trace_t t;

	// check against world
	t = CM_BoxTraceQ2(start, end, mins, maxs, 0, MASK_PLAYERSOLID);
	if (t.fraction < 1.0)
	{
		t.ent = (struct edict_t*)1;
	}

	// check all other solid models
	CL_ClipMoveToEntities(start, mins, maxs, end, &t);

	return t;
}

int     CL_PMpointcontents(vec3_t point)
{
	int i;
	q2entity_state_t* ent;
	int num;
	clipHandle_t cmodel;
	int contents;

	contents = CM_PointContentsQ2(point, 0);

	for (i = 0; i < cl.q2_frame.num_entities; i++)
	{
		num = (cl.q2_frame.parse_entities + i) & (MAX_PARSE_ENTITIES - 1);
		ent = &cl_parse_entities[num];

		if (ent->solid != 31)	// special value for bmodel
		{
			continue;
		}

		cmodel = cl.model_clip[ent->modelindex];
		if (!cmodel)
		{
			continue;
		}

		contents |= CM_TransformedPointContentsQ2(point, cmodel, ent->origin, ent->angles);
	}

	return contents;
}


/*
=================
CL_PredictMovement

Sets cl.predicted_origin and cl.predicted_angles
=================
*/
void CL_PredictMovement(void)
{
	int ack, current;
	int frame;
	int oldframe;
	q2usercmd_t* cmd;
	pmove_t pm;
	int i;
	int step;
	int oldz;

	if (cls.state != CA_ACTIVE)
	{
		return;
	}

	if (cl_paused->value)
	{
		return;
	}

	if (!cl_predict->value || (cl.q2_frame.playerstate.pmove.pm_flags & Q2PMF_NO_PREDICTION))
	{	// just set angles
		for (i = 0; i < 3; i++)
		{
			cl.q2_predicted_angles[i] = cl.viewangles[i] + SHORT2ANGLE(cl.q2_frame.playerstate.pmove.delta_angles[i]);
		}
		return;
	}

	ack = clc.netchan.incomingAcknowledged;
	current = clc.netchan.outgoingSequence;

	// if we are too far out of date, just freeze
	if (current - ack >= CMD_BACKUP_Q2)
	{
		if (cl_showmiss->value)
		{
			Com_Printf("exceeded CMD_BACKUP_Q2\n");
		}
		return;
	}

	// copy current state to pmove
	Com_Memset(&pm, 0, sizeof(pm));
	pm.trace = CL_PMTrace;
	pm.pointcontents = CL_PMpointcontents;

	pm_airaccelerate = String::Atof(cl.q2_configstrings[Q2CS_AIRACCEL]);

	pm.s = cl.q2_frame.playerstate.pmove;

//	SCR_DebugGraph (current - ack - 1, 0);

	frame = 0;

	// run frames
	while (++ack < current)
	{
		frame = ack & (CMD_BACKUP_Q2 - 1);
		cmd = &cl.q2_cmds[frame];

		pm.cmd = *cmd;
		Pmove(&pm);

		// save for debug checking
		VectorCopy(pm.s.origin, cl.q2_predicted_origins[frame]);
	}

	oldframe = (ack - 2) & (CMD_BACKUP_Q2 - 1);
	oldz = cl.q2_predicted_origins[oldframe][2];
	step = pm.s.origin[2] - oldz;
	if (step > 63 && step < 160 && (pm.s.pm_flags & Q2PMF_ON_GROUND))
	{
		cl.q2_predicted_step = step * 0.125;
		cl.q2_predicted_step_time = cls.realtime - cls.q2_frametimeFloat * 500;
	}


	// copy results out for rendering
	cl.q2_predicted_origin[0] = pm.s.origin[0] * 0.125;
	cl.q2_predicted_origin[1] = pm.s.origin[1] * 0.125;
	cl.q2_predicted_origin[2] = pm.s.origin[2] * 0.125;

	VectorCopy(pm.viewangles, cl.q2_predicted_angles);
}
