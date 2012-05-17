// view.c -- player eye positioning

#include "quakedef.h"

/*

The view is allowed to move slightly from it's true position for bobbing,
but if it exceeds 8 pixels linear distance (spherical, not box), the list of
entities sent from the server may not include everything in the pvs, especially
when crossing a water boudnary.

*/

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

static Cvar* r_drawviewmodel;

Cvar* crosshair;

Cvar* cl_crossx;
Cvar* cl_crossy;

static Cvar* v_centermove;

static Cvar* v_centerrollspeed;

static Cvar* cl_polyblend;

static float v_dmg_time, v_dmg_roll, v_dmg_pitch;

hwframe_t* view_frame;
static hwplayer_state_t* view_message;

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
//	static	double	bobtime;
	static float bob;
	float cycle;

	if (cl.qh_spectator)
	{
		return 0;
	}

//	if (onground == -1)
//		return bob;		// just use old value

//	bobtime += host_frametime;
//	cycle = bobtime - (int)(bobtime/cl_bobcycle->value)*cl_bobcycle->value;
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

// bob is proportional to simulated velocity in the xy plane
// (don't count Z, or jumping messes it up)

	bob = sqrt(cl.qh_simvel[0] * cl.qh_simvel[0] + cl.qh_simvel[1] * cl.qh_simvel[1]) * cl_bob->value;
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

	if (view_message->onground == -1 || clc.demoplaying)
	{
		cl.qh_driftmove = 0;
		cl.qh_pitchvel = 0;
		return;
	}

// don't count small mouse motion
	if (cl.qh_nodrift)
	{
		if (Q_fabs(cl.hw_frames[(clc.netchan.outgoingSequence - 1) & UPDATE_MASK_HW].cmd.forwardmove) < (cl.h2_v.hasted * cl_forwardspeed->value) - 10 || lookspring->value == 0.0)
		{
			cl.qh_driftmove = 0;
		}
		else
		{
			cl.qh_driftmove += host_frametime;
		}

		if (cl.qh_spectator)
		{
			cl.qh_driftmove = 0;
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
===============
V_DriftRoll

Moves the client pitch angle towards cl.idealroll sent by the server.

If the user is adjusting pitch manually, either with lookup/lookdown,
mlook and mouse, or klook and keyboard, pitch drifting is constantly stopped.

===============
*/
static void V_DriftRoll(void)
{
	float delta, move;
	float rollspeed;

	if (view_message->onground == -1 || clc.demoplaying)
	{
		if (cl.h2_v.movetype != QHMOVETYPE_FLY)
		{
			cl.h2_rollvel = 0;
			return;
		}
	}


	if (cl.h2_v.movetype != QHMOVETYPE_FLY)
	{
		rollspeed = v_centerrollspeed->value;
	}
	else
	{
		rollspeed = v_centerrollspeed->value * .5;	//slower roll when flying

	}
	delta = cl.h2_idealroll - cl.viewangles[ROLL];

	if (!delta)
	{
		cl.h2_rollvel = 0;
		return;
	}


	move = host_frametime * cl.h2_rollvel;
	cl.h2_rollvel += host_frametime * rollspeed;

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
	VectorSubtract(from, cl.qh_simorg, from);
	VectorNormalize(from);

	AngleVectors(cl.qh_simangles, forward, right, up);

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

static void V_DarkFlash_f(void)
{
	cl.qh_cshifts[CSHIFT_BONUS].destcolor[0] = 0;
	cl.qh_cshifts[CSHIFT_BONUS].destcolor[1] = 0;
	cl.qh_cshifts[CSHIFT_BONUS].destcolor[2] = 0;
	cl.qh_cshifts[CSHIFT_BONUS].percent = 255;
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
/*	if (cl.items & IT_QUAD)
    {
        cl.qh_cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
        cl.qh_cshifts[CSHIFT_POWERUP].destcolor[1] = 0;
        cl.qh_cshifts[CSHIFT_POWERUP].destcolor[2] = 255;
        cl.qh_cshifts[CSHIFT_POWERUP].percent = 30;
    }
    else if (cl.items & IT_SUIT)
    {
        cl.qh_cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
        cl.qh_cshifts[CSHIFT_POWERUP].destcolor[1] = 255;
        cl.qh_cshifts[CSHIFT_POWERUP].destcolor[2] = 0;
        cl.qh_cshifts[CSHIFT_POWERUP].percent = 20;
    }
*/
	if ((int)cl.h2_v.artifact_active & HWARTFLAG_DIVINE_INTERVENTION)
	{
		cl.qh_cshifts[CSHIFT_BONUS].destcolor[0] = 255;
		cl.qh_cshifts[CSHIFT_BONUS].destcolor[1] = 255;
		cl.qh_cshifts[CSHIFT_BONUS].destcolor[2] = 255;
		cl.qh_cshifts[CSHIFT_BONUS].percent = 256;
	}

	if ((int)cl.h2_v.artifact_active & HWARTFLAG_FROZEN)
	{
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[0] = 20;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[1] = 70;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[2] = 255;
		cl.qh_cshifts[CSHIFT_POWERUP].percent = 65;
	}
	else if ((int)cl.h2_v.artifact_active & HWARTFLAG_STONED)
	{
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[0] = 205;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[1] = 205;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[2] = 205;
		//cl.qh_cshifts[CSHIFT_POWERUP].percent = 80;
		cl.qh_cshifts[CSHIFT_POWERUP].percent = 11000;
	}
	else if ((int)cl.h2_v.artifact_active & HWARTFLAG_INVISIBILITY)
	{
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[0] = 100;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[1] = 100;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[2] = 100;
		cl.qh_cshifts[CSHIFT_POWERUP].percent = 100;
	}
	else if ((int)cl.h2_v.artifact_active & HWARTFLAG_INVINCIBILITY)
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
		if (cl.qh_cshifts[j].percent > 10000)
		{	// Set percent for grayscale
			cl.qh_cshifts[j].percent = 80;
		}

		a2 = cl.qh_cshifts[j].percent / 255.0;
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
	cl.h2_viewent.state.angles[YAW] = viewangles[YAW];
	cl.h2_viewent.state.angles[PITCH] = -viewangles[PITCH];

	cl.h2_viewent.state.angles[ROLL] -= v_idlescale->value * sin(cl.qh_serverTimeFloat * v_iroll_cycle->value) * v_iroll_level->value;
	cl.h2_viewent.state.angles[PITCH] -= v_idlescale->value * sin(cl.qh_serverTimeFloat * v_ipitch_cycle->value) * v_ipitch_level->value;
	cl.h2_viewent.state.angles[YAW] -= v_idlescale->value * sin(cl.qh_serverTimeFloat * v_iyaw_cycle->value) * v_iyaw_level->value;
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

//	cl.viewent.angles[ROLL] -= v_idlescale->value * sin(cl.time*v_iroll_cycle->value) * v_iroll_level->value;
//	cl.viewent.angles[PITCH] -= v_idlescale->value * sin(cl.time*v_ipitch_cycle->value) * v_ipitch_level->value;
//	cl.viewent.angles[YAW] -= v_idlescale->value * sin(cl.time*v_iyaw_cycle->value) * v_iyaw_level->value;
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

	side = VQH_CalcRoll(cl.qh_simangles, cl.qh_simvel);
	viewangles[ROLL] += side;

	if (v_dmg_time > 0)
	{
		viewangles[ROLL] += v_dmg_time / v_kicktime->value * v_dmg_roll;
		viewangles[PITCH] += v_dmg_time / v_kicktime->value * v_dmg_pitch;
		v_dmg_time -= host_frametime;
	}

	if (cl.h2_v.health <= 0 && !cl.qh_spectator)
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
	h2entity_t* view;
	float old;

// view is the weapon model
	view = &cl.h2_viewent;

	VectorCopy(cl.qh_simorg, cl.refdef.vieworg);
	vec3_t viewangles;
	VectorCopy(cl.qh_simangles, viewangles);
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
	h2entity_t* view;
	int i;
	vec3_t forward, right, up;
	float bob;
	static float oldz = 0;

	if (!cl.h2_v.cameramode)
	{
		V_DriftPitch();
		V_DriftRoll();
	}

// view is the weapon model (only visible from inside body)
	view = &cl.h2_viewent;

	if (cl.h2_v.movetype != QHMOVETYPE_FLY)
	{
		bob = V_CalcBob();
	}
	else	// no bobbing when you fly
	{
		bob = 1;
	}

// refresh position from simulated origin
	VectorCopy(cl.qh_simorg, cl.refdef.vieworg);

	cl.refdef.vieworg[2] += bob;

// never let it sit exactly on a node line, because a water plane can
// dissapear when viewed with the eye exactly on it.
// the server protocol only specifies to 1/8 pixel, so add 1/16 in each axis
	cl.refdef.vieworg[0] += 1.0 / 32;
	cl.refdef.vieworg[1] += 1.0 / 32;
	cl.refdef.vieworg[2] += 1.0 / 32;

	vec3_t viewangles;
	VectorCopy(cl.qh_simangles, viewangles);
	V_CalcViewRoll(viewangles);
	V_AddIdle(viewangles);

	if (cl.qh_spectator)
	{
		cl.refdef.vieworg[2] += 50;	// view height
	}
	else
	{
		if (view_message->flags & HWPF_CROUCH)
		{
			cl.refdef.vieworg[2] += 24;	// gib view height
		}
		else if (view_message->flags & HWPF_DEAD)
		{
			cl.refdef.vieworg[2] += 8;	// corpse view height
		}
		else
		{
			cl.refdef.vieworg[2] += 50;	// view height

		}
		if (view_message->flags & HWPF_DEAD)		// PF_GIB will also set HWPF_DEAD
		{
			viewangles[ROLL] = 80;	// dead view angle
		}
	}

// offsets
	AngleVectors(cl.qh_simangles, forward, right, up);

// set up gun position
	VectorCopy(cl.qh_simangles, view->state.angles);

	CalcGunAngle(viewangles);

	VectorCopy(cl.refdef.vieworg, view->state.origin);
//	view->origin[2] += 56;

	for (i = 0; i < 3; i++)
	{
		view->state.origin[i] += forward[i] * bob * 0.4;
//		view->origin[i] += right[i]*bob*0.4;
//		view->origin[i] += up[i]*bob*0.8;
	}
// rjr no idea why commenting this out works, as it is in the original q1 codebase
// view->origin[2] += bob;

// fudge position around to keep amount of weapon visible
// roughly equal with different FOV
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

	if (view_message->flags & (HWPF_DEAD))
	{
		view->state.modelindex = 0;
	}
	else
	{
		view->state.modelindex = cl.qh_stats[STAT_WEAPON];
	}
	view->state.frame = view_message->weaponframe;

	// Place weapon in powered up mode
	if ((cl.hw_frames[clc.netchan.incomingSequence & UPDATE_MASK_HW].playerstate[cl.playernum].drawflags & H2MLS_MASKIN) == H2MLS_POWERMODE)
	{
		view->state.drawflags = (view->state.drawflags & H2MLS_MASKOUT) | H2MLS_POWERMODE;
	}
	else
	{
		view->state.drawflags = (view->state.drawflags & H2MLS_MASKOUT) | 0;
	}

// set up the refresh position
	viewangles[PITCH] += cl.qh_punchangle;
	AnglesToAxis(viewangles, cl.refdef.viewaxis);

// smooth out stair step ups
	if ((view_message->onground != -1) && (cl.qh_simorg[2] - oldz > 0))
	{
		float steptime;

		steptime = host_frametime;

		oldz += steptime * 80;
		if (oldz > cl.qh_simorg[2])
		{
			oldz = cl.qh_simorg[2];
		}
		if (cl.qh_simorg[2] - oldz > 12)
		{
			oldz = cl.qh_simorg[2] - 12;
		}
		cl.refdef.vieworg[2] += oldz - cl.qh_simorg[2];
		view->state.origin[2] += oldz - cl.qh_simorg[2];
	}
	else
	{
		oldz = cl.qh_simorg[2];
	}
}

/*
=============
DropPunchAngle
=============
*/
void DropPunchAngle(void)
{
	cl.qh_punchangle -= 10 * host_frametime;
	if (cl.qh_punchangle < 0)
	{
		cl.qh_punchangle = 0;
	}
}

/*
=============
CL_AddViewModel
=============
*/
static void CL_AddViewModel()
{
	if (!r_drawviewmodel->value)
	{
		return;
	}

	if (cl.qh_spectator)
	{
		return;
	}

//rjr	if (cl.items & IT_INVISIBILITY)
//rjr		return;

	if (cl.h2_v.health <= 0)
	{
		return;
	}

	if (!cl.h2_viewent.state.modelindex)
	{
		return;
	}

	refEntity_t gun;

	Com_Memset(&gun, 0, sizeof(gun));
	gun.reType = RT_MODEL;
	gun.renderfx = RF_MINLIGHT | RF_FIRST_PERSON | RF_DEPTHHACK;
	VectorCopy(cl.h2_viewent.state.origin, gun.origin);
	gun.hModel = cl.model_draw[cl.h2_viewent.state.modelindex];
	gun.frame = cl.h2_viewent.state.frame;
	gun.skinNum = cl.h2_viewent.state.skinnum;
	gun.syncBase = cl.h2_viewent.syncbase;
	CLH2_SetRefEntAxis(&gun, cl.h2_viewent.state.angles, vec3_origin, cl.h2_viewent.state.scale, 0, cl.h2_viewent.state.abslight, cl.h2_viewent.state.drawflags);
	CLH2_HandleCustomSkin(&gun, -1);

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
	Cvar_Set("r_fullbright", "0");

	CL_RunLightStyles();

	CL_AddLightStyles();
	CL_AddParticles();

	V_SetContentsColor(CM_PointContentsQ1(cl.refdef.vieworg, 0));
	V_CalcBlend();

	cl.refdef.time = cl.serverTime;

	R_RenderScene(&cl.refdef);

	R_PolyBlend();
}

/*
==================
V_RenderView

The player's clipping box goes from (-16 -16 -24) to (16 16 32) from
the entity origin, so any view position inside that will be valid
==================
*/
void V_RenderView(void)
{
//	if (cl.simangles[ROLL])
//		Sys_Error ("cl.simangles[ROLL]");	// DEBUG
//rjrcl.simangles[ROLL] = 0;	// FIXME @@@

	if (cls.state != CA_ACTIVE)
	{
		return;
	}

	view_frame = &cl.hw_frames[clc.netchan.incomingSequence & UPDATE_MASK_HW];
	view_message = &view_frame->playerstate[cl.playernum];

	DropPunchAngle();
	if (cl.qh_intermission)
	{	// intermission / finale rendering
		V_CalcIntermissionRefdef();
	}
	else
	{
		V_CalcRefdef();
	}
	CL_AddViewModel();

	CLH2_UpdateEffects();

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
	Cmd_AddCommand("df", V_DarkFlash_f);

	v_centermove = Cvar_Get("v_centermove", "0.15", 0);
	v_centerrollspeed = Cvar_Get("v_centerrollspeed","125", 0);

	v_iyaw_cycle = Cvar_Get("v_iyaw_cycle", "2", 0);
	v_iroll_cycle = Cvar_Get("v_iroll_cycle", "0.5", 0);
	v_ipitch_cycle = Cvar_Get("v_ipitch_cycle", "1", 0);
	v_iyaw_level = Cvar_Get("v_iyaw_level", "0.3", 0);
	v_iroll_level = Cvar_Get("v_iroll_level", "0.1", 0);
	v_ipitch_level = Cvar_Get("v_ipitch_level", "0.3", 0);

	v_idlescale = Cvar_Get("v_idlescale", "0", 0);
	crosshair = Cvar_Get("crosshair", "0", CVAR_ARCHIVE);
	cl_crossx = Cvar_Get("cl_crossx", "0", CVAR_ARCHIVE);
	cl_crossy = Cvar_Get("cl_crossy", "0", CVAR_ARCHIVE);

	VQH_InitRollCvars();
	cl_bob = Cvar_Get("cl_bob","0.02", 0);
	cl_bobcycle = Cvar_Get("cl_bobcycle","0.6", 0);
	cl_bobup = Cvar_Get("cl_bobup","0.5", 0);

	v_kicktime = Cvar_Get("v_kicktime", "0.5", 0);
	v_kickroll = Cvar_Get("v_kickroll", "0.6", 0);
	v_kickpitch = Cvar_Get("v_kickpitch", "0.6", 0);

	r_drawviewmodel = Cvar_Get("r_drawviewmodel", "1", 0);

	cl_polyblend = Cvar_Get("cl_polyblend", "1", 0);
}
