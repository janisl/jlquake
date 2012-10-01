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

#include "../../client.h"
#include "view.h"
#include "../quake/local.h"
#include "../hexen2/local.h"

static Cvar* vqh_centermove;
static Cvar* vh2_centerrollspeed;

static Cvar* clqh_bob;
static Cvar* clqh_bobcycle;
static Cvar* clqh_bobup;

Cvar* vqh_kicktime;
Cvar* vqh_kickroll;
Cvar* vqh_kickpitch;

static Cvar* vqh_idlescale;
static Cvar* vqh_iyaw_cycle;
static Cvar* vqh_iroll_cycle;
static Cvar* vqh_ipitch_cycle;
static Cvar* vqh_iyaw_level;
static Cvar* vqh_iroll_level;
static Cvar* vqh_ipitch_level;

static Cvar* vqh_drawviewmodel;

float v_dmg_time, v_dmg_roll, v_dmg_pitch;

//	Moves the client pitch angle towards cl.idealpitch sent by the server.
//
//	If the user is adjusting pitch manually, either with lookup/lookdown,
// mlook and mouse, or klook and keyboard, pitch drifting is constantly stopped.
//
//	Drifting is enabled when the center view key is hit, mlook is released and
// lookspring is non 0, or when
static void VQH_DriftPitch(qwplayer_state_t* qwViewMessage, hwplayer_state_t* hwViewMessage)
{
	if (GGameType & GAME_QuakeWorld)
	{
		if (qwViewMessage->onground == -1 || clc.demoplaying)
		{
			cl.qh_driftmove = 0;
			cl.qh_pitchvel = 0;
			return;
		}
	}
	else if (GGameType & GAME_HexenWorld)
	{
		if (hwViewMessage->onground == -1 || clc.demoplaying)
		{
			cl.qh_driftmove = 0;
			cl.qh_pitchvel = 0;
			return;
		}
	}
	else
	{
		if (!cl.qh_onground || clc.demoplaying)
		{
			cl.qh_driftmove = 0;
			cl.qh_pitchvel = 0;
			return;
		}
	}

	// don't count small mouse motion
	if (cl.qh_nodrift)
	{
		if (GGameType & GAME_QuakeWorld)
		{
			if (abs(cl.qw_frames[(clc.netchan.outgoingSequence - 1) & UPDATE_MASK_QW].cmd.forwardmove) < 200)
			{
				cl.qh_driftmove = 0;
			}
			else
			{
				cl.qh_driftmove += cls.frametime * 0.001;
			}
		}
		else if (GGameType & GAME_HexenWorld)
		{
			if (abs(cl.hw_frames[(clc.netchan.outgoingSequence - 1) & UPDATE_MASK_HW].cmd.forwardmove) < (cl.h2_v.hasted * cl_forwardspeed->value) - 10 || lookspring->value == 0.0)
			{
				cl.qh_driftmove = 0;
			}
			else
			{
				cl.qh_driftmove += cls.frametime * 0.001;
			}

			if (cl.qh_spectator)
			{
				cl.qh_driftmove = 0;
			}
		}
		else if (GGameType & GAME_Hexen2)
		{
			if (abs(cl.qh_cmd.forwardmove) < (cl.h2_v.hasted * cl_forwardspeed->value) - 10)
			{
				cl.qh_driftmove = 0;
			}
			else
			{
				cl.qh_driftmove += cls.frametime * 0.001;
			}
		}
		else
		{
			if (abs(cl.qh_cmd.forwardmove) < cl_forwardspeed->value)
			{
				cl.qh_driftmove = 0;
			}
			else
			{
				cl.qh_driftmove += cls.frametime * 0.001;
			}
		}

		if (cl.qh_driftmove > vqh_centermove->value)
		{
			CLQH_StartPitchDrift();
		}
		return;
	}

	float delta = (GGameType & GAME_QuakeWorld ? 0 : cl.qh_idealpitch) - cl.viewangles[PITCH];

	if (!delta)
	{
		cl.qh_pitchvel = 0;
		return;
	}

	float move = cls.frametime * 0.001 * cl.qh_pitchvel;
	cl.qh_pitchvel += cls.frametime * 0.001 * v_centerspeed->value;

	if (delta > 0)
	{
		if (move > delta)
		{
			cl.qh_pitchvel = 0;
			move = delta;
		}
		cl.viewangles[PITCH] += move;
	}
	else if (delta < 0)
	{
		if (move > -delta)
		{
			cl.qh_pitchvel = 0;
			move = -delta;
		}
		cl.viewangles[PITCH] -= move;
	}
}

static void VH2_DriftRoll(hwplayer_state_t* hwViewMessage)
{
	if (GGameType & GAME_HexenWorld)
	{
		if (hwViewMessage->onground == -1 || clc.demoplaying)
		{
			if (cl.h2_v.movetype != QHMOVETYPE_FLY)
			{
				cl.h2_rollvel = 0;
				return;
			}
		}
	}
	else
	{
		if (clc.demoplaying)
		{
			return;
		}
	}

	float rollspeed;
	if (GGameType & GAME_HexenWorld && cl.h2_v.movetype == QHMOVETYPE_FLY)
	{
		rollspeed = vh2_centerrollspeed->value * .5;	//slower roll when flying
	}
	else
	{
		rollspeed = vh2_centerrollspeed->value;
	}

	float delta = cl.h2_idealroll - cl.viewangles[ROLL];

	if (!delta)
	{
		cl.h2_rollvel = 0;
		return;
	}

	float move = cls.frametime * 0.001 * cl.h2_rollvel;
	cl.h2_rollvel += cls.frametime * 0.001 * rollspeed;

	if (delta > 0)
	{
		if (move > delta)
		{
			cl.h2_rollvel = 0;
			move = delta;
		}
		cl.viewangles[ROLL] += move;
	}
	else if (delta < 0)
	{
		if (move > -delta)
		{
			cl.h2_rollvel = 0;
			move = -delta;
		}
		cl.viewangles[ROLL] -= move;
	}
}

static float VQH_CalcBob()
{
	static double bobtime;
	static float bob;

	if (GGameType & GAME_Hexen2 && cl.h2_v.movetype == QHMOVETYPE_FLY)
	{
		// no bobbing when you fly
		return 1;
	}

	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld) && cl.qh_spectator)
	{
		return 0;
	}

	if (GGameType & GAME_QuakeWorld && qh_pmove.onground == -1)
	{
		return bob;		// just use old value
	}

	bobtime += cls.frametime * 0.001;
	float cycle = GGameType & GAME_QuakeWorld ? bobtime - (int)(bobtime / clqh_bobcycle->value) * clqh_bobcycle->value :
		cl.qh_serverTimeFloat - (int)(cl.qh_serverTimeFloat / clqh_bobcycle->value) * clqh_bobcycle->value;
	cycle /= clqh_bobcycle->value;
	if (cycle < clqh_bobup->value)
	{
		cycle = M_PI * cycle / clqh_bobup->value;
	}
	else
	{
		cycle = M_PI + M_PI * (cycle - clqh_bobup->value) / (1.0 - clqh_bobup->value);
	}

	// bob is proportional to simulated velocity in the xy plane
	// (don't count Z, or jumping messes it up)
	bob = GGameType & (GAME_QuakeWorld | GAME_HexenWorld) ?
		sqrt(cl.qh_simvel[0] * cl.qh_simvel[0] + cl.qh_simvel[1] * cl.qh_simvel[1]) * clqh_bob->value :
		sqrt(cl.qh_velocity[0] * cl.qh_velocity[0] + cl.qh_velocity[1] * cl.qh_velocity[1]) * clqh_bob->value;
	bob = bob * 0.3 + bob * 0.7 * sin(cycle);
	if (bob > 4)
	{
		bob = 4;
	}
	else if (bob < -7)
	{
		bob = -7;
	}
	return bob;
}

//	Roll is induced by movement and damage
static void VQH_CalcViewRoll(vec3_t viewangles)
{
	float side = GGameType & (GAME_QuakeWorld | GAME_HexenWorld) ? VQH_CalcRoll(cl.qh_simangles, cl.qh_simvel) :
		VQH_CalcRoll(GGameType & GAME_Hexen2 ? h2cl_entities[cl.viewentity].state.angles : clq1_entities[cl.viewentity].state.angles, cl.qh_velocity);
	viewangles[ROLL] += side;

	if (v_dmg_time > 0)
	{
		viewangles[ROLL] += v_dmg_time / vqh_kicktime->value * v_dmg_roll;
		viewangles[PITCH] += v_dmg_time / vqh_kicktime->value * v_dmg_pitch;
		v_dmg_time -= cls.frametime * 0.001;
	}

	if (!(GGameType & GAME_QuakeWorld) && (GGameType & GAME_Hexen2 ? cl.h2_v.health : cl.qh_stats[Q1STAT_HEALTH]) <= 0 && (!(GGameType & GAME_HexenWorld) || !cl.qh_spectator))
	{
		viewangles[ROLL] = 80;	// dead view angle
		return;
	}
}

//	Idle swaying
static void VQH_AddIdle(vec3_t viewangles)
{
	viewangles[ROLL] += vqh_idlescale->value * sin(cl.qh_serverTimeFloat * vqh_iroll_cycle->value) * vqh_iroll_level->value;
	viewangles[PITCH] += vqh_idlescale->value * sin(cl.qh_serverTimeFloat * vqh_ipitch_cycle->value) * vqh_ipitch_level->value;
	viewangles[YAW] += vqh_idlescale->value * sin(cl.qh_serverTimeFloat * vqh_iyaw_cycle->value) * vqh_iyaw_level->value;

	if (GGameType & GAME_QuakeWorld)
	{
		cl.q1_viewent.state.angles[ROLL] -= vqh_idlescale->value * sin(cl.qh_serverTimeFloat * vqh_iroll_cycle->value) * vqh_iroll_level->value;
		cl.q1_viewent.state.angles[PITCH] -= vqh_idlescale->value * sin(cl.qh_serverTimeFloat * vqh_ipitch_cycle->value) * vqh_ipitch_level->value;
		cl.q1_viewent.state.angles[YAW] -= vqh_idlescale->value * sin(cl.qh_serverTimeFloat * vqh_iyaw_cycle->value) * vqh_iyaw_level->value;
	}
}

static void VQH_BoundOffsets()
{
	if (GGameType & GAME_Hexen2)
	{
		h2entity_t* ent = &h2cl_entities[cl.viewentity];

		// absolutely bound refresh reletive to entity clipping hull
		// so the view can never be inside a solid wall
		if (cl.refdef.vieworg[0] < ent->state.origin[0] - 14)
		{
			cl.refdef.vieworg[0] = ent->state.origin[0] - 14;
		}
		else if (cl.refdef.vieworg[0] > ent->state.origin[0] + 14)
		{
			cl.refdef.vieworg[0] = ent->state.origin[0] + 14;
		}
		if (cl.refdef.vieworg[1] < ent->state.origin[1] - 14)
		{
			cl.refdef.vieworg[1] = ent->state.origin[1] - 14;
		}
		else if (cl.refdef.vieworg[1] > ent->state.origin[1] + 14)
		{
			cl.refdef.vieworg[1] = ent->state.origin[1] + 14;
		}
		if (cl.refdef.vieworg[2] < ent->state.origin[2] - 0)
		{
			cl.refdef.vieworg[2] = ent->state.origin[2] - 0;
		}
		else if (cl.refdef.vieworg[2] > ent->state.origin[2] + 86)
		{
			cl.refdef.vieworg[2] = ent->state.origin[2] + 86;
		}
	}
	else
	{
		q1entity_t* ent = &clq1_entities[cl.viewentity];

		// absolutely bound refresh reletive to entity clipping hull
		// so the view can never be inside a solid wall
		if (cl.refdef.vieworg[0] < ent->state.origin[0] - 14)
		{
			cl.refdef.vieworg[0] = ent->state.origin[0] - 14;
		}
		else if (cl.refdef.vieworg[0] > ent->state.origin[0] + 14)
		{
			cl.refdef.vieworg[0] = ent->state.origin[0] + 14;
		}
		if (cl.refdef.vieworg[1] < ent->state.origin[1] - 14)
		{
			cl.refdef.vieworg[1] = ent->state.origin[1] - 14;
		}
		else if (cl.refdef.vieworg[1] > ent->state.origin[1] + 14)
		{
			cl.refdef.vieworg[1] = ent->state.origin[1] + 14;
		}
		if (cl.refdef.vieworg[2] < ent->state.origin[2] - 22)
		{
			cl.refdef.vieworg[2] = ent->state.origin[2] - 22;
		}
		else if (cl.refdef.vieworg[2] > ent->state.origin[2] + 30)
		{
			cl.refdef.vieworg[2] = ent->state.origin[2] + 30;
		}
	}
}

static void VQH_CalcGunAngle(vec3_t viewangles)
{
	if (GGameType & GAME_Hexen2)
	{
		cl.h2_viewent.state.angles[YAW] = viewangles[YAW];
		cl.h2_viewent.state.angles[PITCH] = -viewangles[PITCH];

		cl.h2_viewent.state.angles[ROLL] -= vqh_idlescale->value * sin(cl.qh_serverTimeFloat * vqh_iroll_cycle->value) * vqh_iroll_level->value;
		cl.h2_viewent.state.angles[PITCH] -= vqh_idlescale->value * sin(cl.qh_serverTimeFloat * vqh_ipitch_cycle->value) * vqh_ipitch_level->value;
		cl.h2_viewent.state.angles[YAW] -= vqh_idlescale->value * sin(cl.qh_serverTimeFloat * vqh_iyaw_cycle->value) * vqh_iyaw_level->value;
	}
	else
	{
		cl.q1_viewent.state.angles[YAW] = viewangles[YAW];
		cl.q1_viewent.state.angles[PITCH] = -viewangles[PITCH];

		if (!(GGameType & GAME_QuakeWorld))
		{
			cl.q1_viewent.state.angles[ROLL] -= vqh_idlescale->value * sin(cl.qh_serverTimeFloat * vqh_iroll_cycle->value) * vqh_iroll_level->value;
			cl.q1_viewent.state.angles[PITCH] -= vqh_idlescale->value * sin(cl.qh_serverTimeFloat * vqh_ipitch_cycle->value) * vqh_ipitch_level->value;
			cl.q1_viewent.state.angles[YAW] -= vqh_idlescale->value * sin(cl.qh_serverTimeFloat * vqh_iyaw_cycle->value) * vqh_iyaw_level->value;
		}
	}
}

void VQH_CalcRefdef(qwplayer_state_t* qwViewMessage, hwplayer_state_t* hwViewMessage)
{
	int i;
	vec3_t forward, right, up;
	static float oldz = 0;

	if (!(GGameType & GAME_Hexen2) || !cl.h2_v.cameramode)
	{
		VQH_DriftPitch(qwViewMessage, hwViewMessage);
		if (GGameType & GAME_Hexen2)
		{
			VH2_DriftRoll(hwViewMessage);
		}
	}

	float bob = VQH_CalcBob();

	// ent is the player model (visible when out of body)
	q1entity_t* q1ent;
	h2entity_t* h2ent;
	if (GGameType & GAME_QuakeWorld)
	{
		// refresh position from simulated origin
		VectorCopy(cl.qh_simorg, cl.refdef.vieworg);
	}
	else if (GGameType & GAME_Quake)
	{
		q1ent = &clq1_entities[cl.viewentity];

		// transform the view offset by the model's matrix to get the offset from
		// model origin for the view
		q1ent->state.angles[YAW] = cl.viewangles[YAW];	// the model should face
		// the view dir
		q1ent->state.angles[PITCH] = -cl.viewangles[PITCH];	// the model should face
		// the view dir

		// refresh position
		VectorCopy(q1ent->state.origin, cl.refdef.vieworg);

		cl.refdef.vieworg[2] += cl.qh_viewheight;
	}
	else if (GGameType & GAME_HexenWorld)
	{
		// refresh position from simulated origin
		VectorCopy(cl.qh_simorg, cl.refdef.vieworg);
	}
	else
	{
		h2ent = &h2cl_entities[cl.viewentity];

		// transform the view offset by the model's matrix to get the offset from
		// model origin for the view
		h2ent->state.angles[YAW] = cl.viewangles[YAW];	// the model should face
		// the view dir
		h2ent->state.angles[PITCH] = -cl.viewangles[PITCH];	// the model should face
		// the view dir

		// refresh position
		VectorCopy(h2ent->state.origin, cl.refdef.vieworg);

		cl.refdef.vieworg[2] += cl.qh_viewheight;
	}

	cl.refdef.vieworg[2] += bob;

	// never let it sit exactly on a node line, because a water plane can
	// dissapear when viewed with the eye exactly on it.
	// the server protocol only specifies to 1/8 pixel, so add 1/16 in each axis
	cl.refdef.vieworg[0] += 1.0 / 16;
	cl.refdef.vieworg[1] += 1.0 / 16;
	cl.refdef.vieworg[2] += 1.0 / 16;

	vec3_t viewangles;
	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		VectorCopy(cl.qh_simangles, viewangles);
	}
	else
	{
		VectorCopy(cl.viewangles, viewangles);
	}
	VQH_CalcViewRoll(viewangles);
	VQH_AddIdle(viewangles);

	if (GGameType & GAME_QuakeWorld)
	{
		if (qwViewMessage->flags & QWPF_GIB)
		{
			cl.refdef.vieworg[2] += 8;	// gib view height
		}
		else if (qwViewMessage->flags & QWPF_DEAD)
		{
			cl.refdef.vieworg[2] -= 16;	// corpse view height
		}
		else
		{
			cl.refdef.vieworg[2] += 22;	// view height
		}
		if (qwViewMessage->flags & QWPF_DEAD)		// QWPF_GIB will also set QWPF_DEAD
		{
			viewangles[ROLL] = 80;	// dead view angle
		}

		// offsets
		AngleVectors(cl.qh_simangles, forward, right, up);
	}
	else if (GGameType & GAME_HexenWorld)
	{
		if (cl.qh_spectator)
		{
			cl.refdef.vieworg[2] += 50;	// view height
		}
		else
		{
			if (hwViewMessage->flags & HWPF_CROUCH)
			{
				cl.refdef.vieworg[2] += 24;	// gib view height
			}
			else if (hwViewMessage->flags & HWPF_DEAD)
			{
				cl.refdef.vieworg[2] += 8;	// corpse view height
			}
			else
			{
				cl.refdef.vieworg[2] += 50;	// view height

			}
			if (hwViewMessage->flags & HWPF_DEAD)		// PF_GIB will also set HWPF_DEAD
			{
				viewangles[ROLL] = 80;	// dead view angle
			}
		}

		// offsets
		AngleVectors(cl.qh_simangles, forward, right, up);
	}
	else if (GGameType & GAME_Quake)
	{
		// offsets
		vec3_t angles;
		angles[PITCH] = -q1ent->state.angles[PITCH];	// because entity pitches are actually backward
		angles[YAW] = q1ent->state.angles[YAW];
		angles[ROLL] = q1ent->state.angles[ROLL];

		AngleVectors(angles, forward, right, up);

		VQH_BoundOffsets();
	}
	else
	{
		// offsets
		vec3_t angles;
		angles[PITCH] = -h2ent->state.angles[PITCH];	// because entity pitches are actually backward
		angles[YAW] = h2ent->state.angles[YAW];
		angles[ROLL] = h2ent->state.angles[ROLL];

		AngleVectors(angles, forward, right, up);

		VQH_BoundOffsets();
	}

	// view is the weapon model (only visible from inside body)
	q1entity_t* q1view;
	h2entity_t* h2view;
	if (GGameType & GAME_Quake)
	{
		q1view = &cl.q1_viewent;

		// set up gun position
		if (GGameType & GAME_QuakeWorld)
		{
			VectorCopy(cl.qh_simangles, q1view->state.angles);
		}
		else
		{
			VectorCopy(cl.viewangles, q1view->state.angles);
		}

		VQH_CalcGunAngle(viewangles);

		if (GGameType & GAME_QuakeWorld)
		{
			VectorCopy(cl.qh_simorg, q1view->state.origin);
			q1view->state.origin[2] += 22;
		}
		else
		{
			VectorCopy(q1ent->state.origin, q1view->state.origin);
			q1view->state.origin[2] += cl.qh_viewheight;
		}

		for (i = 0; i < 3; i++)
		{
			q1view->state.origin[i] += forward[i] * bob * 0.4;
		}
		q1view->state.origin[2] += bob;

		// fudge position around to keep amount of weapon visible
		// roughly equal with different FOV
		if (scr_viewsize->value == 110)
		{
			q1view->state.origin[2] += 1;
		}
		else if (scr_viewsize->value == 100)
		{
			q1view->state.origin[2] += 2;
		}
		else if (scr_viewsize->value == 90)
		{
			q1view->state.origin[2] += 1;
		}
		else if (scr_viewsize->value == 80)
		{
			q1view->state.origin[2] += 0.5;
		}

		if (GGameType & GAME_QuakeWorld)
		{
			if (qwViewMessage->flags & (QWPF_GIB | QWPF_DEAD))
			{
				q1view->state.modelindex = 0;
			}
			else
			{
				q1view->state.modelindex = cl.qh_stats[Q1STAT_WEAPON];
			}
			q1view->state.frame = qwViewMessage->weaponframe;
		}
		else
		{
			q1view->state.modelindex = cl.qh_stats[Q1STAT_WEAPON];
			q1view->state.frame = cl.qh_stats[Q1STAT_WEAPONFRAME];
			q1view->state.colormap = 0;
		}
	}
	else
	{
		h2view = &cl.h2_viewent;

		// set up gun position
		if (GGameType & GAME_HexenWorld)
		{
			VectorCopy(cl.qh_simangles, h2view->state.angles);
		}
		else
		{
			VectorCopy(cl.viewangles, h2view->state.angles);
		}

		VQH_CalcGunAngle(viewangles);

		if (GGameType & GAME_HexenWorld)
		{
			VectorCopy(cl.refdef.vieworg, h2view->state.origin);
		}
		else
		{
			VectorCopy(h2ent->state.origin, h2view->state.origin);
			h2view->state.origin[2] += cl.qh_viewheight;
		}

		for (i = 0; i < 3; i++)
		{
			h2view->state.origin[i] += forward[i] * bob * 0.4;
		}
		if (!(GGameType & GAME_HexenWorld))
		{
			h2view->state.origin[2] += bob;
		}

		// fudge position around to keep amount of weapon visible
		// roughly equal with different FOV
		if (scr_viewsize->value == 110)
		{
			h2view->state.origin[2] += 1;
		}
		else if (scr_viewsize->value == 100)
		{
			h2view->state.origin[2] += 2;
		}
		else if (scr_viewsize->value == 90)
		{
			h2view->state.origin[2] += 1;
		}
		else if (scr_viewsize->value == 80)
		{
			h2view->state.origin[2] += 0.5;
		}

		if (GGameType & GAME_HexenWorld)
		{
			if (hwViewMessage->flags & (HWPF_DEAD))
			{
				h2view->state.modelindex = 0;
			}
			else
			{
				h2view->state.modelindex = cl.qh_stats[Q1STAT_WEAPON];
			}
			h2view->state.frame = hwViewMessage->weaponframe;

			// Place weapon in powered up mode
			if ((cl.hw_frames[clc.netchan.incomingSequence & UPDATE_MASK_HW].playerstate[cl.playernum].drawflags & H2MLS_MASKIN) == H2MLS_POWERMODE)
			{
				h2view->state.drawflags = (h2view->state.drawflags & H2MLS_MASKOUT) | H2MLS_POWERMODE;
			}
			else
			{
				h2view->state.drawflags = (h2view->state.drawflags & H2MLS_MASKOUT) | 0;
			}
		}
		else
		{
			h2view->state.modelindex = cl.qh_stats[Q1STAT_WEAPON];
			h2view->state.frame = cl.qh_stats[Q1STAT_WEAPONFRAME];

			// Place weapon in powered up mode
			if ((h2ent->state.drawflags & H2MLS_MASKIN) == H2MLS_POWERMODE)
			{
				h2view->state.drawflags = (h2view->state.drawflags & H2MLS_MASKOUT) | H2MLS_POWERMODE;
			}
			else
			{
				h2view->state.drawflags = (h2view->state.drawflags & H2MLS_MASKOUT) | 0;
			}
		}
	}

	// set up the refresh position
	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		viewangles[PITCH] += cl.qh_punchangle;
	}
	else
	{
		VectorAdd(viewangles, cl.qh_punchangles, viewangles);
	}
	AnglesToAxis(viewangles, cl.refdef.viewaxis);

	bool onGround;
	float currentZ;
	if (GGameType & GAME_QuakeWorld)
	{
		onGround = qwViewMessage->onground != -1;
		currentZ = cl.qh_simorg[2];
	}
	else if (GGameType & GAME_Quake)
	{
		onGround = cl.qh_onground;
		currentZ = q1ent->state.origin[2];
	}
	else if (GGameType & GAME_HexenWorld)
	{
		onGround = hwViewMessage->onground != -1;
		currentZ = cl.qh_simorg[2];
	}
	else
	{
		onGround = cl.qh_onground;
		currentZ = h2ent->state.origin[2];
	}

	// smooth out stair step ups
	if (onGround && currentZ - oldz > 0)
	{
		float steptime = cls.frametime * 0.001;

		oldz += steptime * 80;
		if (oldz > currentZ)
		{
			oldz = currentZ;
		}
		if (currentZ - oldz > 12)
		{
			oldz = currentZ - 12;
		}
		cl.refdef.vieworg[2] += oldz - currentZ;
		if (GGameType & GAME_Hexen2)
		{
			h2view->state.origin[2] += oldz - currentZ;
		}
		else
		{
			q1view->state.origin[2] += oldz - currentZ;
		}
	}
	else
	{
		oldz = currentZ;
	}

	if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)) && chase_active->value)
	{
		Chase_Update();
	}
}

void VQH_CalcIntermissionRefdef()
{
	vec3_t viewangles;
	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		VectorCopy(cl.qh_simorg, cl.refdef.vieworg);
		VectorCopy(cl.qh_simangles, viewangles);
	}
	else if (GGameType & GAME_Quake)
	{
		// ent is the player model (visible when out of body)
		q1entity_t* ent = &clq1_entities[cl.viewentity];
		VectorCopy(ent->state.origin, cl.refdef.vieworg);
		VectorCopy(ent->state.angles, viewangles);
	}
	else
	{
		// ent is the player model (visible when out of body)
		h2entity_t* ent = &h2cl_entities[cl.viewentity];
		VectorCopy(ent->state.origin, cl.refdef.vieworg);
		VectorCopy(ent->state.angles, viewangles);
		cl.refdef.vieworg[2] += cl.qh_viewheight;
	}

	// view is the weapon model
	if (GGameType & GAME_Hexen2)
	{
		h2entity_t* view = &cl.h2_viewent;
		view->state.modelindex = 0;
	}
	else
	{
		q1entity_t* view = &cl.q1_viewent;
		view->state.modelindex = 0;
	}

	// allways idle in intermission
	float old = vqh_idlescale->value;
	vqh_idlescale->value = 1;
	VQH_AddIdle(viewangles);
	AnglesToAxis(viewangles, cl.refdef.viewaxis);
	vqh_idlescale->value = old;
}

void VQH_AddViewModel()
{
	if (!vqh_drawviewmodel->value)
	{
		return;
	}

	if (GGameType & GAME_QuakeWorld && !Cam_DrawViewModel())
	{
		return;
	}

	if (GGameType & GAME_Quake && !(GGameType & GAME_QuakeWorld) && cl.q1_items & Q1IT_INVISIBILITY)
	{
		return;
	}

	if (GGameType & GAME_QuakeWorld && cl.qh_stats[QWSTAT_ITEMS] & Q1IT_INVISIBILITY)
	{
		return;
	}

	if (GGameType & GAME_HexenWorld && cl.qh_spectator)
	{
		return;
	}

	if (GGameType & GAME_Quake && cl.qh_stats[Q1STAT_HEALTH] <= 0)
	{
		return;
	}

	if (GGameType & GAME_Hexen2 && cl.h2_v.health <= 0)
	{
		return;
	}

	if (GGameType & GAME_Quake && !cl.q1_viewent.state.modelindex)
	{
		return;
	}

	if (GGameType & GAME_Hexen2 && !cl.h2_viewent.state.modelindex)
	{
		return;
	}

	if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)) && chase_active->value)
	{
		return;
	}

	refEntity_t gun;

	Com_Memset(&gun, 0, sizeof(gun));
	gun.reType = RT_MODEL;
	gun.renderfx = RF_MINLIGHT | RF_FIRST_PERSON | RF_DEPTHHACK;
	if (GGameType & GAME_Hexen2)
	{
		VectorCopy(cl.h2_viewent.state.origin, gun.origin);
		gun.hModel = cl.model_draw[cl.h2_viewent.state.modelindex];
		gun.frame = cl.h2_viewent.state.frame;
		gun.skinNum = cl.h2_viewent.state.skinnum;
		gun.syncBase = cl.h2_viewent.syncbase;
		CLH2_SetRefEntAxis(&gun, cl.h2_viewent.state.angles, vec3_origin, cl.h2_viewent.state.scale, cl.h2_viewent.state.colormap, cl.h2_viewent.state.abslight, cl.h2_viewent.state.drawflags);
		CLH2_HandleCustomSkin(&gun, -1);
	}
	else
	{
		VectorCopy(cl.q1_viewent.state.origin, gun.origin);
		gun.hModel = cl.model_draw[cl.q1_viewent.state.modelindex];
		gun.frame = cl.q1_viewent.state.frame;
		gun.skinNum = cl.q1_viewent.state.skinnum;
		gun.syncBase = cl.q1_viewent.syncbase;
		CLQ1_SetRefEntAxis(&gun, cl.q1_viewent.state.angles);
	}

	R_AddRefEntityToScene(&gun);
}

void VQH_SharedInit()
{
	vqh_centermove = Cvar_Get("v_centermove", "0.15", 0);
	if (GGameType & GAME_Hexen2)
	{
		vh2_centerrollspeed = Cvar_Get("v_centerrollspeed", "125", 0);
	}

	VQH_InitRollCvars();
	clqh_bob = Cvar_Get("cl_bob", "0.02", 0);
	clqh_bobcycle = Cvar_Get("cl_bobcycle", "0.6", 0);
	clqh_bobup = Cvar_Get("cl_bobup", "0.5", 0);

	vqh_kicktime = Cvar_Get("v_kicktime", "0.5", 0);
	vqh_kickroll = Cvar_Get("v_kickroll", "0.6", 0);
	vqh_kickpitch = Cvar_Get("v_kickpitch", "0.6", 0);

	vqh_idlescale = Cvar_Get("v_idlescale", "0", 0);
	vqh_iyaw_cycle = Cvar_Get("v_iyaw_cycle", "2", 0);
	vqh_iroll_cycle = Cvar_Get("v_iroll_cycle", "0.5", 0);
	vqh_ipitch_cycle = Cvar_Get("v_ipitch_cycle", "1", 0);
	vqh_iyaw_level = Cvar_Get("v_iyaw_level", "0.3", 0);
	vqh_iroll_level = Cvar_Get("v_iroll_level", "0.1", 0);
	vqh_ipitch_level = Cvar_Get("v_ipitch_level", "0.3", 0);

	vqh_drawviewmodel = Cvar_Get("r_drawviewmodel", "1", 0);
}
