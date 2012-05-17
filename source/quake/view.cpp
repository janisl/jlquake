/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// view.c -- player eye positioning

#include "quakedef.h"

/*

The view is allowed to move slightly from it's true position for bobbing,
but if it exceeds 8 pixels linear distance (spherical, not box), the list of
entities sent from the server may not include everything in the pvs, especially
when crossing a water boudnary.

*/

static Cvar* scr_ofsx;
static Cvar* scr_ofsy;
static Cvar* scr_ofsz;

static Cvar* cl_bob;
static Cvar* cl_bobcycle;
static Cvar* cl_bobup;

static Cvar* v_kicktime;
static Cvar* v_kickroll;
static Cvar* v_kickpitch;

static Cvar* v_iyaw_cycle;
static Cvar* v_iroll_cycle;
static Cvar* v_ipitch_cycle;
static Cvar* v_iyaw_level;
static Cvar* v_iroll_level;
static Cvar* v_ipitch_level;

static Cvar* v_idlescale;

Cvar* crosshair;
Cvar* cl_crossx;
Cvar* cl_crossy;

static Cvar* gl_cshiftpercent;

static Cvar* r_drawviewmodel;

static Cvar* v_centermove;

static Cvar* cl_polyblend;

static float v_dmg_time, v_dmg_roll, v_dmg_pitch;

static cshift_t cshift_empty = { {130,80,50}, 0 };
static cshift_t cshift_water = { {130,80,50}, 128 };
static cshift_t cshift_slime = { {0,25,5}, 150 };
static cshift_t cshift_lava = { {255,80,0}, 150 };

static float v_blend[4];			// rgba 0.0 - 1.0

/*
===============
V_CalcBob

===============
*/
static float V_CalcBob(void)
{
	float bob;
	float cycle;

	cycle = cl.qh_serverTimeFloat - (int)(cl.qh_serverTimeFloat / cl_bobcycle->value) * cl_bobcycle->value;
	cycle /= cl_bobcycle->value;
	if (cycle < cl_bobup->value)
	{
		cycle = M_PI * cycle / cl_bobup->value;
	}
	else
	{
		cycle = M_PI + M_PI * (cycle - cl_bobup->value) / (1.0 - cl_bobup->value);
	}

// bob is proportional to velocity in the xy plane
// (don't count Z, or jumping messes it up)

	bob = sqrt(cl.qh_velocity[0] * cl.qh_velocity[0] + cl.qh_velocity[1] * cl.qh_velocity[1]) * cl_bob->value;
//Con_Printf ("speed: %5.1f\n", Length(cl.velocity));
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


//=============================================================================

/*
===============
V_DriftPitch

Moves the client pitch angle towards cl.idealpitch sent by the server.

If the user is adjusting pitch manually, either with lookup/lookdown,
mlook and mouse, or klook and keyboard, pitch drifting is constantly stopped.

Drifting is enabled when the center view key is hit, mlook is released and
lookspring is non 0, or when
===============
*/
static void V_DriftPitch(void)
{
	float delta, move;

	if (!cl.qh_onground || clc.demoplaying)
	{
		cl.qh_driftmove = 0;
		cl.qh_pitchvel = 0;
		return;
	}

// don't count small mouse motion
	if (cl.qh_nodrift)
	{
		if (Q_fabs(cl.qh_cmd.forwardmove) < cl_forwardspeed->value)
		{
			cl.qh_driftmove = 0;
		}
		else
		{
			cl.qh_driftmove += host_frametime;
		}

		if (cl.qh_driftmove > v_centermove->value)
		{
			CLQH_StartPitchDrift();
		}
		return;
	}

	delta = cl.qh_idealpitch - cl.viewangles[PITCH];

	if (!delta)
	{
		cl.qh_pitchvel = 0;
		return;
	}

	move = host_frametime * cl.qh_pitchvel;
	cl.qh_pitchvel += host_frametime * v_centerspeed->value;

//Con_Printf ("move: %f (%f)\n", move, host_frametime);

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





/*
==============================================================================

                        PALETTE FLASHES

==============================================================================
*/

/*
===============
V_ParseDamage
===============
*/
void V_ParseDamage(void)
{
	int armor, blood;
	vec3_t from;
	int i;
	vec3_t forward, right, up;
	q1entity_t* ent;
	float side;
	float count;

	armor = net_message.ReadByte();
	blood = net_message.ReadByte();
	for (i = 0; i < 3; i++)
		from[i] = net_message.ReadCoord();

	count = blood * 0.5 + armor * 0.5;
	if (count < 10)
	{
		count = 10;
	}

	cl.q1_faceanimtime = cl.qh_serverTimeFloat + 0.2;		// but sbar face into pain frame

	cl.qh_cshifts[CSHIFT_DAMAGE].percent += 3 * count;
	if (cl.qh_cshifts[CSHIFT_DAMAGE].percent < 0)
	{
		cl.qh_cshifts[CSHIFT_DAMAGE].percent = 0;
	}
	if (cl.qh_cshifts[CSHIFT_DAMAGE].percent > 150)
	{
		cl.qh_cshifts[CSHIFT_DAMAGE].percent = 150;
	}

	if (armor > blood)
	{
		cl.qh_cshifts[CSHIFT_DAMAGE].destcolor[0] = 200;
		cl.qh_cshifts[CSHIFT_DAMAGE].destcolor[1] = 100;
		cl.qh_cshifts[CSHIFT_DAMAGE].destcolor[2] = 100;
	}
	else if (armor)
	{
		cl.qh_cshifts[CSHIFT_DAMAGE].destcolor[0] = 220;
		cl.qh_cshifts[CSHIFT_DAMAGE].destcolor[1] = 50;
		cl.qh_cshifts[CSHIFT_DAMAGE].destcolor[2] = 50;
	}
	else
	{
		cl.qh_cshifts[CSHIFT_DAMAGE].destcolor[0] = 255;
		cl.qh_cshifts[CSHIFT_DAMAGE].destcolor[1] = 0;
		cl.qh_cshifts[CSHIFT_DAMAGE].destcolor[2] = 0;
	}

//
// calculate view angle kicks
//
	ent = &clq1_entities[cl.viewentity];

	VectorSubtract(from, ent->state.origin, from);
	VectorNormalize(from);

	AngleVectors(ent->state.angles, forward, right, up);

	side = DotProduct(from, right);
	v_dmg_roll = count * side * v_kickroll->value;

	side = DotProduct(from, forward);
	v_dmg_pitch = count * side * v_kickpitch->value;

	v_dmg_time = v_kicktime->value;
}


/*
==================
V_cshift_f
==================
*/
static void V_cshift_f(void)
{
	cshift_empty.destcolor[0] = String::Atoi(Cmd_Argv(1));
	cshift_empty.destcolor[1] = String::Atoi(Cmd_Argv(2));
	cshift_empty.destcolor[2] = String::Atoi(Cmd_Argv(3));
	cshift_empty.percent = String::Atoi(Cmd_Argv(4));
}


/*
==================
V_BonusFlash_f

When you run over an item, the server sends this command
==================
*/
static void V_BonusFlash_f(void)
{
	cl.qh_cshifts[CSHIFT_BONUS].destcolor[0] = 215;
	cl.qh_cshifts[CSHIFT_BONUS].destcolor[1] = 186;
	cl.qh_cshifts[CSHIFT_BONUS].destcolor[2] = 69;
	cl.qh_cshifts[CSHIFT_BONUS].percent = 50;
}

/*
=============
V_SetContentsColor

Underwater, lava, etc each has a color shift
=============
*/
static void V_SetContentsColor(int contents)
{
	switch (contents)
	{
	case BSP29CONTENTS_EMPTY:
	case BSP29CONTENTS_SOLID:
		cl.qh_cshifts[CSHIFT_CONTENTS] = cshift_empty;
		break;
	case BSP29CONTENTS_LAVA:
		cl.qh_cshifts[CSHIFT_CONTENTS] = cshift_lava;
		break;
	case BSP29CONTENTS_SLIME:
		cl.qh_cshifts[CSHIFT_CONTENTS] = cshift_slime;
		break;
	default:
		cl.qh_cshifts[CSHIFT_CONTENTS] = cshift_water;
	}
}

/*
=============
V_CalcPowerupCshift
=============
*/
static void V_CalcPowerupCshift(void)
{
	if (cl.q1_items & IT_QUAD)
	{
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[1] = 0;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[2] = 255;
		cl.qh_cshifts[CSHIFT_POWERUP].percent = 30;
	}
	else if (cl.q1_items & IT_SUIT)
	{
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[1] = 255;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[2] = 0;
		cl.qh_cshifts[CSHIFT_POWERUP].percent = 20;
	}
	else if (cl.q1_items & IT_INVISIBILITY)
	{
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[0] = 100;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[1] = 100;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[2] = 100;
		cl.qh_cshifts[CSHIFT_POWERUP].percent = 100;
	}
	else if (cl.q1_items & IT_INVULNERABILITY)
	{
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[0] = 255;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[1] = 255;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[2] = 0;
		cl.qh_cshifts[CSHIFT_POWERUP].percent = 30;
	}
	else
	{
		cl.qh_cshifts[CSHIFT_POWERUP].percent = 0;
	}
}

/*
=============
V_CalcBlend
=============
*/
static void V_CalcBlend(void)
{
	float r, g, b, a, a2;
	int j;

	r = 0;
	g = 0;
	b = 0;
	a = 0;

	for (j = 0; j < NUM_CSHIFTS; j++)
	{
		if (!gl_cshiftpercent->value)
		{
			continue;
		}

		a2 = ((cl.qh_cshifts[j].percent * gl_cshiftpercent->value) / 100.0) / 255.0;

//		a2 = cl.qh_cshifts[j].percent/255.0;
		if (!a2)
		{
			continue;
		}
		a = a + a2 * (1 - a);
//Con_Printf ("j:%i a:%f\n", j, a);
		a2 = a2 / a;
		r = r * (1 - a2) + cl.qh_cshifts[j].destcolor[0] * a2;
		g = g * (1 - a2) + cl.qh_cshifts[j].destcolor[1] * a2;
		b = b * (1 - a2) + cl.qh_cshifts[j].destcolor[2] * a2;
	}

	v_blend[0] = r / 255.0;
	v_blend[1] = g / 255.0;
	v_blend[2] = b / 255.0;
	v_blend[3] = a;
	if (v_blend[3] > 1)
	{
		v_blend[3] = 1;
	}
	if (v_blend[3] < 0)
	{
		v_blend[3] = 0;
	}
}

/*
=============
V_UpdatePalette
=============
*/
void V_UpdatePalette(void)
{
	int i, j;
	qboolean is_new;

	V_CalcPowerupCshift();

	is_new = false;

	for (i = 0; i < NUM_CSHIFTS; i++)
	{
		if (cl.qh_cshifts[i].percent != cl.qh_prev_cshifts[i].percent)
		{
			is_new = true;
			cl.qh_prev_cshifts[i].percent = cl.qh_cshifts[i].percent;
		}
		for (j = 0; j < 3; j++)
			if (cl.qh_cshifts[i].destcolor[j] != cl.qh_prev_cshifts[i].destcolor[j])
			{
				is_new = true;
				cl.qh_prev_cshifts[i].destcolor[j] = cl.qh_cshifts[i].destcolor[j];
			}
	}

// drop the damage value
	cl.qh_cshifts[CSHIFT_DAMAGE].percent -= host_frametime * 150;
	if (cl.qh_cshifts[CSHIFT_DAMAGE].percent <= 0)
	{
		cl.qh_cshifts[CSHIFT_DAMAGE].percent = 0;
	}

// drop the bonus value
	cl.qh_cshifts[CSHIFT_BONUS].percent -= host_frametime * 100;
	if (cl.qh_cshifts[CSHIFT_BONUS].percent <= 0)
	{
		cl.qh_cshifts[CSHIFT_BONUS].percent = 0;
	}

	if (!is_new)
	{
		return;
	}

	V_CalcBlend();
}


/*
==============================================================================

                        VIEW RENDERING

==============================================================================
*/

/*
==================
CalcGunAngle
==================
*/
static void CalcGunAngle(vec3_t viewangles)
{
	cl.q1_viewent.state.angles[YAW] = viewangles[YAW];
	cl.q1_viewent.state.angles[PITCH] = -viewangles[PITCH];

	cl.q1_viewent.state.angles[ROLL] -= v_idlescale->value * sin(cl.qh_serverTimeFloat * v_iroll_cycle->value) * v_iroll_level->value;
	cl.q1_viewent.state.angles[PITCH] -= v_idlescale->value * sin(cl.qh_serverTimeFloat * v_ipitch_cycle->value) * v_ipitch_level->value;
	cl.q1_viewent.state.angles[YAW] -= v_idlescale->value * sin(cl.qh_serverTimeFloat * v_iyaw_cycle->value) * v_iyaw_level->value;
}

/*
==============
V_BoundOffsets
==============
*/
static void V_BoundOffsets(void)
{
	q1entity_t* ent;

	ent = &clq1_entities[cl.viewentity];

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

/*
==============
V_AddIdle

Idle swaying
==============
*/
static void V_AddIdle(vec3_t viewangles)
{
	viewangles[ROLL] += v_idlescale->value * sin(cl.qh_serverTimeFloat * v_iroll_cycle->value) * v_iroll_level->value;
	viewangles[PITCH] += v_idlescale->value * sin(cl.qh_serverTimeFloat * v_ipitch_cycle->value) * v_ipitch_level->value;
	viewangles[YAW] += v_idlescale->value * sin(cl.qh_serverTimeFloat * v_iyaw_cycle->value) * v_iyaw_level->value;
}


/*
==============
V_CalcViewRoll

Roll is induced by movement and damage
==============
*/
static void V_CalcViewRoll(vec3_t viewangles)
{
	float side;

	side = VQH_CalcRoll(clq1_entities[cl.viewentity].state.angles, cl.qh_velocity);
	viewangles[ROLL] += side;

	if (v_dmg_time > 0)
	{
		viewangles[ROLL] += v_dmg_time / v_kicktime->value * v_dmg_roll;
		viewangles[PITCH] += v_dmg_time / v_kicktime->value * v_dmg_pitch;
		v_dmg_time -= host_frametime;
	}

	if (cl.qh_stats[Q1STAT_HEALTH] <= 0)
	{
		viewangles[ROLL] = 80;	// dead view angle
		return;
	}

}


/*
==================
V_CalcIntermissionRefdef

==================
*/
static void V_CalcIntermissionRefdef(void)
{
	q1entity_t* ent, * view;
	float old;

// ent is the player model (visible when out of body)
	ent = &clq1_entities[cl.viewentity];
// view is the weapon model (only visible from inside body)
	view = &cl.q1_viewent;

	VectorCopy(ent->state.origin, cl.refdef.vieworg);
	vec3_t viewangles;
	VectorCopy(ent->state.angles, viewangles);
	view->state.modelindex = 0;

// allways idle in intermission
	old = v_idlescale->value;
	v_idlescale->value = 1;
	V_AddIdle(viewangles);
	AnglesToAxis(viewangles, cl.refdef.viewaxis);
	v_idlescale->value = old;
}

/*
==================
V_CalcRefdef

==================
*/
static void V_CalcRefdef(void)
{
	q1entity_t* ent, * view;
	int i;
	vec3_t forward, right, up;
	vec3_t angles;
	float bob;
	static float oldz = 0;

	V_DriftPitch();

// ent is the player model (visible when out of body)
	ent = &clq1_entities[cl.viewentity];
// view is the weapon model (only visible from inside body)
	view = &cl.q1_viewent;


// transform the view offset by the model's matrix to get the offset from
// model origin for the view
	ent->state.angles[YAW] = cl.viewangles[YAW];	// the model should face
	// the view dir
	ent->state.angles[PITCH] = -cl.viewangles[PITCH];	// the model should face
	// the view dir


	bob = V_CalcBob();

// refresh position
	VectorCopy(ent->state.origin, cl.refdef.vieworg);
	cl.refdef.vieworg[2] += cl.qh_viewheight + bob;

// never let it sit exactly on a node line, because a water plane can
// dissapear when viewed with the eye exactly on it.
// the server protocol only specifies to 1/16 pixel, so add 1/32 in each axis
	cl.refdef.vieworg[0] += 1.0 / 32;
	cl.refdef.vieworg[1] += 1.0 / 32;
	cl.refdef.vieworg[2] += 1.0 / 32;

	vec3_t viewangles;
	VectorCopy(cl.viewangles, viewangles);
	V_CalcViewRoll(viewangles);
	V_AddIdle(viewangles);

// offsets
	angles[PITCH] = -ent->state.angles[PITCH];	// because entity pitches are
	//  actually backward
	angles[YAW] = ent->state.angles[YAW];
	angles[ROLL] = ent->state.angles[ROLL];

	AngleVectors(angles, forward, right, up);

	for (i = 0; i < 3; i++)
		cl.refdef.vieworg[i] += scr_ofsx->value * forward[i]
								+ scr_ofsy->value * right[i]
								+ scr_ofsz->value * up[i];


	V_BoundOffsets();

// set up gun position
	VectorCopy(cl.viewangles, view->state.angles);

	CalcGunAngle(viewangles);

	VectorCopy(ent->state.origin, view->state.origin);
	view->state.origin[2] += cl.qh_viewheight;

	for (i = 0; i < 3; i++)
	{
		view->state.origin[i] += forward[i] * bob * 0.4;
//		view->origin[i] += right[i]*bob*0.4;
//		view->origin[i] += up[i]*bob*0.8;
	}
	view->state.origin[2] += bob;

// fudge position around to keep amount of weapon visible
// roughly equal with different FOV

#if 0
	if (cl.model_precache[cl.stats[Q1STAT_WEAPON]] && String::Cmp(cl.model_precache[cl.stats[Q1STAT_WEAPON]]->name,  "progs/v_shot2.mdl"))
#endif
	if (scr_viewsize->value == 110)
	{
		view->state.origin[2] += 1;
	}
	else if (scr_viewsize->value == 100)
	{
		view->state.origin[2] += 2;
	}
	else if (scr_viewsize->value == 90)
	{
		view->state.origin[2] += 1;
	}
	else if (scr_viewsize->value == 80)
	{
		view->state.origin[2] += 0.5;
	}

	view->state.modelindex = cl.qh_stats[Q1STAT_WEAPON];
	view->state.frame = cl.qh_stats[Q1STAT_WEAPONFRAME];
	view->state.colormap = 0;

// set up the refresh position
	VectorAdd(viewangles, cl.qh_punchangles, viewangles);
	AnglesToAxis(viewangles, cl.refdef.viewaxis);

// smooth out stair step ups
	if (cl.qh_onground && ent->state.origin[2] - oldz > 0)
	{
		float steptime;

		steptime = cl.qh_serverTimeFloat - cl.qh_oldtime;
		if (steptime < 0)
		{
//FIXME		I_Error ("steptime < 0");
			steptime = 0;
		}

		oldz += steptime * 80;
		if (oldz > ent->state.origin[2])
		{
			oldz = ent->state.origin[2];
		}
		if (ent->state.origin[2] - oldz > 12)
		{
			oldz = ent->state.origin[2] - 12;
		}
		cl.refdef.vieworg[2] += oldz - ent->state.origin[2];
		view->state.origin[2] += oldz - ent->state.origin[2];
	}
	else
	{
		oldz = ent->state.origin[2];
	}

	if (chase_active->value)
	{
		Chase_Update();
	}
}

/*
=============
R_DrawViewModel
=============
*/
static void CL_AddViewModel()
{
	if (!r_drawviewmodel->value)
	{
		return;
	}

	if (cl.q1_items & IT_INVISIBILITY)
	{
		return;
	}

	if (cl.qh_stats[Q1STAT_HEALTH] <= 0)
	{
		return;
	}

	if (!cl.q1_viewent.state.modelindex)
	{
		return;
	}

	if (chase_active->value)
	{
		return;
	}

	refEntity_t gun;

	Com_Memset(&gun, 0, sizeof(gun));
	gun.reType = RT_MODEL;
	gun.renderfx = RF_MINLIGHT | RF_FIRST_PERSON | RF_DEPTHHACK;
	VectorCopy(cl.q1_viewent.state.origin, gun.origin);
	gun.hModel = cl.model_draw[cl.q1_viewent.state.modelindex];
	CLQ1_SetRefEntAxis(&gun, cl.q1_viewent.state.angles);
	gun.frame = cl.q1_viewent.state.frame;
	gun.syncBase = cl.q1_viewent.syncbase;
	gun.skinNum = cl.q1_viewent.state.skinnum;

	R_AddRefEntityToScene(&gun);
}

/*
============
R_PolyBlend
============
*/
static void R_PolyBlend(void)
{
	if (!cl_polyblend->value)
	{
		return;
	}
	if (!v_blend[3])
	{
		return;
	}

	R_Draw2DQuad(cl.refdef.x, cl.refdef.y, cl.refdef.width, cl.refdef.height, NULL, 0, 0, 0, 0, v_blend[0], v_blend[1], v_blend[2], v_blend[3]);
}

/*
================
V_RenderScene

cl.refdef must be set before the first call
================
*/
void V_RenderScene()
{
	// don't allow cheats in multiplayer
	if (cl.qh_maxclients > 1)
	{
		Cvar_Set("r_fullbright", "0");
	}

	CL_RunLightStyles();

	CL_AddLightStyles();
	CL_AddParticles();

	V_SetContentsColor(CM_PointContentsQ1(cl.refdef.vieworg, 0));
	V_CalcBlend();

	cl.refdef.time = cl.serverTime;

	R_RenderScene(&cl.refdef);

	R_PolyBlend();
}

void V_RenderView(void)
{
	if (con_forcedup)
	{
		return;
	}

// don't allow cheats in multiplayer
	if (cl.qh_maxclients > 1)
	{
		Cvar_Set("scr_ofsx", "0");
		Cvar_Set("scr_ofsy", "0");
		Cvar_Set("scr_ofsz", "0");
	}

	if (cl.qh_intermission)
	{	// intermission / finale rendering
		V_CalcIntermissionRefdef();
	}
	else
	{
		if (!cl.qh_paused /* && (sv.maxclients > 1 || in_keyCatchers == 0) */)
		{
			V_CalcRefdef();
		}
	}
	CL_AddViewModel();

	CL_AddDLights();

	V_RenderScene();
}

//============================================================================

/*
=============
V_Init
=============
*/
void V_Init(void)
{
	Cmd_AddCommand("v_cshift", V_cshift_f);
	Cmd_AddCommand("bf", V_BonusFlash_f);

	v_centermove = Cvar_Get("v_centermove", "0.15", 0);

	v_iyaw_cycle = Cvar_Get("v_iyaw_cycle", "2", 0);
	v_iroll_cycle = Cvar_Get("v_iroll_cycle", "0.5", 0);
	v_ipitch_cycle = Cvar_Get("v_ipitch_cycle", "1", 0);
	v_iyaw_level = Cvar_Get("v_iyaw_level", "0.3", 0);
	v_iroll_level = Cvar_Get("v_iroll_level", "0.1", 0);
	v_ipitch_level = Cvar_Get("v_ipitch_level", "0.3", 0);

	v_idlescale = Cvar_Get("v_idlescale", "0", 0);
	crosshair = Cvar_Get("crosshair", "0", CVAR_ARCHIVE);
	cl_crossx = Cvar_Get("cl_crossx", "0", 0);
	cl_crossy = Cvar_Get("cl_crossy", "0", 0);
	gl_cshiftpercent = Cvar_Get("gl_cshiftpercent", "100", 0);

	scr_ofsx = Cvar_Get("scr_ofsx", "0", 0);
	scr_ofsy = Cvar_Get("scr_ofsy", "0", 0);
	scr_ofsz = Cvar_Get("scr_ofsz", "0", 0);
	VQH_InitRollCvars();
	cl_bob = Cvar_Get("cl_bob", "0.02", 0);
	cl_bobcycle = Cvar_Get("cl_bobcycle", "0.6", 0);
	cl_bobup = Cvar_Get("cl_bobup", "0.5", 0);

	v_kicktime = Cvar_Get("v_kicktime", "0.5", 0);
	v_kickroll = Cvar_Get("v_kickroll", "0.6", 0);
	v_kickpitch = Cvar_Get("v_kickpitch", "0.6", 0);

	r_drawviewmodel = Cvar_Get("r_drawviewmodel", "1", 0);

	cl_polyblend = Cvar_Get("cl_polyblend", "1", 0);
}
