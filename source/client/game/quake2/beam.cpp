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

#include "../../client.h"
#include "local.h"

enum { MAX_BEAMS_Q2 = 32 };

struct q2beam_t
{
	int entity;
	int dest_entity;
	qhandle_t model;
	int endtime;
	vec3_t offset;
	vec3_t start;
	vec3_t end;
};

static q2beam_t clq2_beams[MAX_BEAMS_Q2];
//PMM - added this for player-linked beams.  Currently only used by the plasma beam
static q2beam_t clq2_playerbeams[MAX_BEAMS_Q2];

static qhandle_t clq2_mod_parasite_segment;
static qhandle_t clq2_mod_grapple_cable;
static qhandle_t clq2_mod_heatbeam;
static qhandle_t clq2_mod_lightning;

void CLQ2_ClearBeams()
{
	Com_Memset(clq2_beams, 0, sizeof(clq2_beams));
	Com_Memset(clq2_playerbeams, 0, sizeof(clq2_playerbeams));
}

void CLQ2_RegisterBeamModels()
{
	clq2_mod_parasite_segment = R_RegisterModel("models/monsters/parasite/segment/tris.md2");
	clq2_mod_grapple_cable = R_RegisterModel("models/ctf/segment/tris.md2");
	clq2_mod_heatbeam = R_RegisterModel("models/proj/beam/tris.md2");
	clq2_mod_lightning = R_RegisterModel("models/proj/lightning/tris.md2");
}

static void CLQ2_InitBeam(q2beam_t* b, int ent, int destEnt, qhandle_t model,
	int duration, vec3_t start, vec3_t end, vec3_t offset)
{
	b->entity = ent;
	b->dest_entity = destEnt;
	b->model = model;
	b->endtime = cl_common->serverTime + duration;
	VectorCopy(start, b->start);
	VectorCopy(end, b->end);
	VectorCopy(offset, b->offset);
}

static void CLQ2_NewBeamInternal(q2beam_t* beams, int ent, int destEnt, qhandle_t model,
	int duration, vec3_t start, vec3_t end, vec3_t offset)
{
	// override any beam with the same source AND destination entities
	q2beam_t* b = beams;
	for (int i = 0; i < MAX_BEAMS_Q2; i++, b++)
	{
		if (b->entity == ent && b->dest_entity == destEnt)
		{
			CLQ2_InitBeam(b, ent, destEnt, model, 200, start, end, offset);
			return;
		}
	}

	b = beams;
	for (int i = 0; i < MAX_BEAMS_Q2; i++, b++)
	{
		if (!b->model || b->endtime < cl_common->serverTime)
		{
			CLQ2_InitBeam(b, ent, destEnt, model, duration, start, end, offset);
			return;
		}
	}
	Log::write("beam list overflow!\n");
}

static void CLQ2_NewBeam(int ent, int destEnt, qhandle_t model, vec3_t start, vec3_t end, vec3_t offset)
{
	CLQ2_NewBeamInternal(clq2_beams, ent, destEnt, model, 200, start, end, offset);
}

static void CLQ2_NewPlayerBeam(int ent, qhandle_t model, vec3_t start, vec3_t end, vec3_t offset)
{
	// PMM - this needs to be 100 to prevent multiple heatbeams
	CLQ2_NewBeamInternal(clq2_playerbeams, ent, 0, model, 100, start, end, offset);
}

void CLQ2_ParasiteBeam(int ent, vec3_t start, vec3_t end)
{
	CLQ2_NewBeam(ent, 0, clq2_mod_parasite_segment, start, end, vec3_origin);
}

void CLQ2_GrappleCableBeam(int ent, vec3_t start, vec3_t end, vec3_t offset)
{
	CLQ2_NewBeam(ent, 0, clq2_mod_grapple_cable, start, end, offset);
}

void CLQ2_HeatBeam(int ent, vec3_t start, vec3_t end)
{
	vec3_t offset;
	VectorSet(offset, 2, 7, -3);
	CLQ2_NewPlayerBeam(ent, clq2_mod_heatbeam, start, end, offset);
}

void CLQ2_MonsterHeatBeam(int ent, vec3_t start, vec3_t end)
{
	vec3_t offset;
	VectorSet(offset, 0, 0, 0);
	CLQ2_NewPlayerBeam(ent, clq2_mod_heatbeam, start, end, offset);
}

void CLQ2_LightningBeam(int srcEnt, int destEnt, vec3_t start, vec3_t end)
{
	CLQ2_NewBeam(srcEnt, destEnt, clq2_mod_lightning, start, end, vec3_origin);
}

void CLQ2_AddBeams()
{
	q2beam_t* b = clq2_beams;
	for (int i = 0; i < MAX_BEAMS_Q2; i++, b++)
	{
		if (!b->model || b->endtime < cl_common->serverTime)
		{
			continue;
		}

		// if coming from the player, update the start position
		if (b->entity == cl_common->viewentity)
		{
			VectorCopy(cl_common->refdef.vieworg, b->start);
			b->start[2] -= 22;	// adjust for view height
		}
		vec3_t org;
		VectorAdd(b->start, b->offset, org);

		// calculate pitch and yaw
		vec3_t dist;
		VectorSubtract(b->end, org, dist);

		float yaw, pitch, forward;
		vec3_t _angles;
		if (dist[1] == 0 && dist[0] == 0)
		{
			yaw = 0;
			if (dist[2] > 0)
			{
				pitch = 90;
			}
			else
			{
				pitch = 270;
			}
		}
		else
		{
			VecToAnglesCommon(dist, _angles, forward, yaw, pitch);
			pitch = 360 - pitch;
		}

		// add new entities for the beams
		float d = VectorNormalize(dist);

		refEntity_t ent;
		Com_Memset(&ent, 0, sizeof(ent));
		ent.reType = RT_MODEL;
		float model_length;
		if (b->model == clq2_mod_lightning)
		{
			model_length = 35.0;
			d-= 20.0;  // correction so it doesn't end in middle of tesla
		}
		else
		{
			model_length = 30.0;
		}
		float steps = ceil(d / model_length);
		float len = (d - model_length) / (steps - 1);

		// PMM - special case for lightning model .. if the real length is shorter than the model,
		// flip it around & draw it from the end to the start.  This prevents the model from going
		// through the tesla mine (instead it goes through the target)
		if ((b->model == clq2_mod_lightning) && (d <= model_length))
		{
			VectorCopy(b->end, ent.origin);
			ent.hModel = b->model;
			ent.renderfx = RF_ABSOLUTE_LIGHT;
			ent.radius = 1;
			vec3_t angles;
			angles[0] = pitch;
			angles[1] = yaw;
			angles[2] = rand()%360;
			AnglesToAxis(angles, ent.axis);
			R_AddRefEntityToScene(&ent);
			return;
		}
		while (d > 0)
		{
			VectorCopy(org, ent.origin);
			ent.hModel = b->model;
			vec3_t angles;
			if (b->model == clq2_mod_lightning)
			{
				ent.renderfx = RF_ABSOLUTE_LIGHT;
				ent.radius = 1;
				angles[0] = -pitch;
				angles[1] = yaw + 180.0;
				angles[2] = rand() % 360;
			}
			else
			{
				angles[0] = pitch;
				angles[1] = yaw;
				angles[2] = rand() % 360;
			}
			AnglesToAxis(angles, ent.axis);

			R_AddRefEntityToScene(&ent);

			for (int j = 0; j < 3; j++)
			{
				org[j] += dist[j] * len;
			}
			d -= model_length;
		}
	}
}

void CLQ2_AddPlayerBeams()
{
	float hand_multiplier;
	if (q2_hand)
	{
		if (q2_hand->value == 2)
		{
			hand_multiplier = 0;
		}
		else if (q2_hand->value == 1)
		{
			hand_multiplier = -1;
		}
		else
		{
			hand_multiplier = 1;
		}
	}
	else 
	{
		hand_multiplier = 1;
	}

	q2beam_t* b = clq2_playerbeams;
	for (int i = 0; i < MAX_BEAMS_Q2; i++, b++)
	{
		if (!b->model || b->endtime < cl_common->serverTime)
		{
			continue;
		}

		vec3_t org;
		if (clq2_mod_heatbeam && (b->model == clq2_mod_heatbeam))
		{
			// if coming from the player, update the start position
			if (b->entity == cl_common->viewentity)
			{
				// set up gun position
				// code straight out of CL_AddViewWeapon
				q2player_state_t* ps = &cl_common->q2_frame.playerstate;
				int j = (cl_common->q2_frame.serverframe - 1) & UPDATE_MASK_Q2;
				q2frame_t* oldframe = &cl_common->q2_frames[j];
				if (oldframe->serverframe != cl_common->q2_frame.serverframe-1 || !oldframe->valid)
				{
					oldframe = &cl_common->q2_frame;		// previous frame was dropped or involid
				}
				q2player_state_t* ops = &oldframe->playerstate;
				for (j = 0; j < 3; j++)
				{
					b->start[j] = cl_common->refdef.vieworg[j] + ops->gunoffset[j]
						+ cl_common->q2_lerpfrac * (ps->gunoffset[j] - ops->gunoffset[j]);
				}
				VectorMA(b->start, -(hand_multiplier * b->offset[0]), cl_common->refdef.viewaxis[1], org);
				VectorMA(org, b->offset[1], cl_common->refdef.viewaxis[0], org);
				VectorMA(org, b->offset[2], cl_common->refdef.viewaxis[2], org);
				if (q2_hand && (q2_hand->value == 2))
				{
					VectorMA(org, -1, cl_common->refdef.viewaxis[2], org);
				}
			}
			else
			{
				VectorCopy(b->start, org);
			}
		}
		else
		{
			// if coming from the player, update the start position
			if (b->entity == cl_common->viewentity)
			{
				VectorCopy(cl_common->refdef.vieworg, b->start);
				b->start[2] -= 22;	// adjust for view height
			}
			VectorAdd(b->start, b->offset, org);
		}

		// calculate pitch and yaw
		vec3_t dist;
		VectorSubtract(b->end, org, dist);

		if (clq2_mod_heatbeam && (b->model == clq2_mod_heatbeam) && (b->entity == cl_common->viewentity))
		{
			vec_t len = VectorLength(dist);
			VectorScale(cl_common->refdef.viewaxis[0], len, dist);
			VectorMA(dist, -(hand_multiplier * b->offset[0]), cl_common->refdef.viewaxis[1], dist);
			VectorMA(dist, b->offset[1], cl_common->refdef.viewaxis[0], dist);
			VectorMA(dist, b->offset[2], cl_common->refdef.viewaxis[2], dist);
			if (q2_hand && (q2_hand->value == 2))
			{
				VectorMA(org, -1, cl_common->refdef.viewaxis[2], org);
			}
		}

		float yaw, pitch, forward;
		vec3_t _angles;
		if (dist[1] == 0 && dist[0] == 0)
		{
			yaw = 0;
			if (dist[2] > 0)
			{
				pitch = 90;
			}
			else
			{
				pitch = 270;
			}
		}
		else
		{
			VecToAnglesCommon(dist, _angles, forward, yaw, pitch);
			pitch = -pitch;
		}

		int framenum;
		if (clq2_mod_heatbeam && (b->model == clq2_mod_heatbeam))
		{
			if (b->entity != cl_common->viewentity)
			{
				framenum = 2;
				vec3_t angles;
				angles[0] = -pitch;
				angles[1] = yaw + 180.0;
				angles[2] = 0;
				vec3_t f, r, u;
				AngleVectors(angles, f, r, u);

				// if it's a non-origin offset, it's a player, so use the hardcoded player offset
				if (!VectorCompare(b->offset, vec3_origin))
				{
					VectorMA(org, -b->offset[0] + 1, r, org);
					VectorMA(org, -b->offset[1], f, org);
					VectorMA(org, -b->offset[2] - 10, u, org);
				}
				else
				{
					// if it's a monster, do the particle effect
					CLQ2_MonsterPlasma_Shell(b->start);
				}
			}
			else
			{
				framenum = 1;
			}
		}

		// if it's the heatbeam, draw the particle effect
		if ((clq2_mod_heatbeam && (b->model == clq2_mod_heatbeam) && (b->entity == cl_common->viewentity)))
		{
			CLQ2_HeatbeamPaticles(org, dist);
		}

		// add new entities for the beams
		float d = VectorNormalize(dist);

		refEntity_t ent;
		Com_Memset(&ent, 0, sizeof(ent));
		ent.reType = RT_MODEL;
		float model_length;
		if (b->model == clq2_mod_heatbeam)
		{
			model_length = 32.0;
		}
		else if (b->model == clq2_mod_lightning)
		{
			model_length = 35.0;
			d-= 20.0;  // correction so it doesn't end in middle of tesla
		}
		else
		{
			model_length = 30.0;
		}
		float steps = ceil(d / model_length);
		float len = (d - model_length) / (steps - 1);

		// PMM - special case for lightning model .. if the real length is shorter than the model,
		// flip it around & draw it from the end to the start.  This prevents the model from going
		// through the tesla mine (instead it goes through the target)
		if ((b->model == clq2_mod_lightning) && (d <= model_length))
		{
			VectorCopy (b->end, ent.origin);
			ent.hModel = b->model;
			ent.renderfx = RF_ABSOLUTE_LIGHT;
			ent.radius = 1;
			vec3_t angles;
			angles[0] = pitch;
			angles[1] = yaw;
			angles[2] = rand() % 360;
			AnglesToAxis(angles, ent.axis);
			R_AddRefEntityToScene(&ent);
			return;
		}
		while (d > 0)
		{
			VectorCopy(org, ent.origin);
			ent.hModel = b->model;
			vec3_t angles;
			if (clq2_mod_heatbeam && (b->model == clq2_mod_heatbeam))
			{
				ent.renderfx = RF_ABSOLUTE_LIGHT;
				ent.radius = 1;
				angles[0] = -pitch;
				angles[1] = yaw + 180.0;
				angles[2] = (cl_common->serverTime) % 360;
				ent.frame = framenum;
			}
			else if (b->model == clq2_mod_lightning)
			{
				ent.renderfx = RF_ABSOLUTE_LIGHT;
				ent.radius = 1;
				angles[0] = -pitch;
				angles[1] = yaw + 180.0;
				angles[2] = rand() % 360;
			}
			else
			{
				angles[0] = pitch;
				angles[1] = yaw;
				angles[2] = rand() % 360;
			}
			AnglesToAxis(angles, ent.axis);
			
			R_AddRefEntityToScene(&ent);

			for (int j = 0; j < 3; j++)
			{
				org[j] += dist[j] * len;
			}
			d -= model_length;
		}
	}
}
