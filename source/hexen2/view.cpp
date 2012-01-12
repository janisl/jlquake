// view.c -- player eye positioning

/*
 * $Header: /H2 Mission Pack/VIEW.C 4     3/18/98 11:34p Jmonroe $
 */

#include "quakedef.h"

/*

The view is allowed to move slightly from it's true position for bobbing,
but if it exceeds 8 pixels linear distance (spherical, not box), the list of
entities sent from the server may not include everything in the pvs, especially
when crossing a water boudnary.

*/

static Cvar*	scr_ofsx;
static Cvar*	scr_ofsy;
static Cvar*	scr_ofsz;

static Cvar*	cl_rollspeed;
static Cvar*	cl_rollangle;

static Cvar*	cl_bob;
static Cvar*	cl_bobcycle;
static Cvar*	cl_bobup;

static Cvar*	v_kicktime;
static Cvar*	v_kickroll;
static Cvar*	v_kickpitch;

static Cvar*	v_iyaw_cycle;
static Cvar*	v_iroll_cycle;
static Cvar*	v_ipitch_cycle;
static Cvar*	v_iyaw_level;
static Cvar*	v_iroll_level;
static Cvar*	v_ipitch_level;

static Cvar*	v_idlescale;

static Cvar*	r_drawviewmodel;

Cvar*	crosshair;

static Cvar*	v_centermove;
static Cvar*	v_centerspeed;

static Cvar*	v_centerrollspeed;

static Cvar*	cl_polyblend;

static float	v_dmg_time, v_dmg_roll, v_dmg_pitch;

extern	int			in_forward, in_forward2, in_back;

static cshift_t	cshift_empty = { {130,80,50}, 0 };
static cshift_t	cshift_water = { {130,80,50}, 128 };
static cshift_t	cshift_slime = { {0,25,5}, 150 };
static cshift_t	cshift_lava = { {255,80,0}, 150 };

static float		v_blend[4];		// rgba 0.0 - 1.0

/*
===============
V_CalcRoll

Used by view and sv_user
===============
*/
vec3_t	forward, right, up;

float V_CalcRoll (vec3_t angles, vec3_t velocity)
{
	float	sign;
	float	side;
	float	value;
	
	AngleVectors (angles, forward, right, up);
	side = DotProduct (velocity, right);
	sign = side < 0 ? -1 : 1;
	side = Q_fabs(side);
	
	value = cl_rollangle->value;
//	if (cl.inwater)
//		value *= 6;

	if (side < cl_rollspeed->value)
		side = side * value / cl_rollspeed->value;
	else
		side = value;
	
	return side*sign;
	
}


/*
===============
V_CalcBob

===============
*/
static float V_CalcBob (void)
{
	float	bob;
	float	cycle;
	
	cycle = cl.serverTimeFloat - (int)(cl.serverTimeFloat/cl_bobcycle->value)*cl_bobcycle->value;
	cycle /= cl_bobcycle->value;
	if (cycle < cl_bobup->value)
		cycle = M_PI * cycle / cl_bobup->value;
	else
		cycle = M_PI + M_PI*(cycle-cl_bobup->value)/(1.0 - cl_bobup->value);

// bob is proportional to velocity in the xy plane
// (don't count Z, or jumping messes it up)

	bob = sqrt(cl.velocity[0]*cl.velocity[0] + cl.velocity[1]*cl.velocity[1]) * cl_bob->value;
//Con_Printf ("speed: %5.1f\n", Length(cl.velocity));
	bob = bob*0.3 + bob*0.7*sin(cycle);
	if (bob > 4)
		bob = 4;
	else if (bob < -7)
		bob = -7;
	return bob;
	
}


//=============================================================================

void V_StartPitchDrift (void)
{
#if 1
	if (cl.laststop == cl.serverTimeFloat)
	{
		return;		// something else is keeping it from drifting
	}
#endif
	if (cl.nodrift || !cl.pitchvel)
	{
		cl.pitchvel = v_centerspeed->value;
		cl.nodrift = false;
		cl.driftmove = 0;
	}
}

void V_StopPitchDrift (void)
{
	cl.laststop = cl.serverTimeFloat;
	cl.nodrift = true;
	cl.pitchvel = 0;
}

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
static void V_DriftPitch (void)
{
	float		delta, move;

	if (noclip_anglehack || !cl.onground || cls.demoplayback )
	{
		cl.driftmove = 0;
		cl.pitchvel = 0;
		return;
	}

// don't count small mouse motion
	if (cl.nodrift)
	{
		if ( Q_fabs(cl.cmd.forwardmove) < (cl.v.hasted*cl_forwardspeed->value)-10)
			cl.driftmove = 0;
		else
			cl.driftmove += host_frametime;
	
		if ( cl.driftmove > v_centermove->value)
		{
			V_StartPitchDrift ();
		}
		return;
	}
	
	delta = cl.idealpitch - cl.viewangles[PITCH];

	if (!delta)
	{
		cl.pitchvel = 0;
		return;
	}

	move = host_frametime * cl.pitchvel;
	cl.pitchvel += host_frametime * v_centerspeed->value;
	
//Con_Printf ("move: %f (%f)\n", move, host_frametime);

	if (delta > 0)
	{
		if (move > delta)
		{
			cl.pitchvel = 0;
			move = delta;
		}
		cl.viewangles[PITCH] += move;
	}
	else if (delta < 0)
	{
		if (move > -delta)
		{
			cl.pitchvel = 0;
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
static void V_DriftRoll (void)
{
	float		delta, move;

	if (noclip_anglehack || cls.demoplayback )
	{
		return;
	}

	delta = cl.idealroll - cl.viewangles[ROLL];

	if (!delta)
	{
		cl.rollvel = 0;
		return;
	}


	move = host_frametime * cl.rollvel; 
	cl.rollvel += host_frametime * v_centerrollspeed->value;

	if (delta > 0)
	{
		if (move > delta)
		{
			cl.rollvel = 0;
			move = delta;
		}
		cl.viewangles[ROLL] += move;
	}
	else if (delta < 0)
	{
		if (move > -delta)
		{
			cl.rollvel = 0;
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
void V_ParseDamage (void)
{
	int		armor, blood;
	vec3_t	from;
	int		i;
	vec3_t	forward, right, up;
	h2entity_t	*ent;
	float	side;
	float	count;
	
	armor = net_message.ReadByte();
	blood = net_message.ReadByte();
	for (i=0 ; i<3 ; i++)
		from[i] = net_message.ReadCoord();

	count = blood*0.5 + armor*0.5;
	if (count < 10)
		count = 10;

	cl.faceanimtime = cl.serverTimeFloat + 0.2;		// but sbar face into pain frame

	cl.qh_cshifts[CSHIFT_DAMAGE].percent += 3*count;
	if (cl.qh_cshifts[CSHIFT_DAMAGE].percent < 0)
		cl.qh_cshifts[CSHIFT_DAMAGE].percent = 0;
	if (cl.qh_cshifts[CSHIFT_DAMAGE].percent > 150)
		cl.qh_cshifts[CSHIFT_DAMAGE].percent = 150;

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
	ent = &h2cl_entities[cl.viewentity];
	
	VectorSubtract (from, ent->state.origin, from);
	VectorNormalize (from);
	
	AngleVectors (ent->state.angles, forward, right, up);

	side = DotProduct (from, right);
	v_dmg_roll = count*side*v_kickroll->value;
	
	side = DotProduct (from, forward);
	v_dmg_pitch = count*side*v_kickpitch->value;

	v_dmg_time = v_kicktime->value;
}


/*
==================
V_cshift_f
==================
*/
static void V_cshift_f (void)
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
static void V_BonusFlash_f (void)
{
	cl.qh_cshifts[CSHIFT_BONUS].destcolor[0] = 215;
	cl.qh_cshifts[CSHIFT_BONUS].destcolor[1] = 186;
	cl.qh_cshifts[CSHIFT_BONUS].destcolor[2] = 69;
	cl.qh_cshifts[CSHIFT_BONUS].percent = 50;
}

static void V_DarkFlash_f (void)
{
	cl.qh_cshifts[CSHIFT_BONUS].destcolor[0] = 0;
	cl.qh_cshifts[CSHIFT_BONUS].destcolor[1] = 0;
	cl.qh_cshifts[CSHIFT_BONUS].destcolor[2] = 0;
	cl.qh_cshifts[CSHIFT_BONUS].percent = 255;
}

void V_WhiteFlash_f(void)
{
	cl.qh_cshifts[CSHIFT_BONUS].destcolor[0] = 255;
	cl.qh_cshifts[CSHIFT_BONUS].destcolor[1] = 255;
	cl.qh_cshifts[CSHIFT_BONUS].destcolor[2] = 255;
	cl.qh_cshifts[CSHIFT_BONUS].percent = 255;
}

/*
=============
V_SetContentsColor

Underwater, lava, etc each has a color shift
=============
*/
static void V_SetContentsColor (int contents)
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
	if((int)cl.v.artifact_active&ARTFLAG_DIVINE_INTERVENTION)
	{
		cl.qh_cshifts[CSHIFT_BONUS].destcolor[0] = 255;
		cl.qh_cshifts[CSHIFT_BONUS].destcolor[1] = 255;
		cl.qh_cshifts[CSHIFT_BONUS].destcolor[2] = 255;
		cl.qh_cshifts[CSHIFT_BONUS].percent = 256;
	}

	if((int)cl.v.artifact_active&ARTFLAG_FROZEN)
	{
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[0] = 20;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[1] = 70;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[2] = 255;
		cl.qh_cshifts[CSHIFT_POWERUP].percent = 65;
	}
	else if((int)cl.v.artifact_active&ARTFLAG_STONED)
	{
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[0] = 205;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[1] = 205;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[2] = 205;
		//cl.qh_cshifts[CSHIFT_POWERUP].percent = 80;
		cl.qh_cshifts[CSHIFT_POWERUP].percent = 11000;
	}
	else if((int)cl.v.artifact_active&ART_INVISIBILITY)
	{
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[0] = 100;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[1] = 100;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[2] = 100;
		cl.qh_cshifts[CSHIFT_POWERUP].percent = 100;
	}
	else if((int)cl.v.artifact_active&ART_INVINCIBILITY)
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
static void V_CalcBlend (void)
{
	float	r, g, b, a, a2;
	int		j;

	r = 0;
	g = 0;
	b = 0;
	a = 0;

	for (j=0 ; j<NUM_CSHIFTS ; j++)	
	{
		if(cl.qh_cshifts[j].percent > 10000)
		{ // Set percent for grayscale
			cl.qh_cshifts[j].percent = 80;
		}

		a2 = cl.qh_cshifts[j].percent/255.0;
		if (!a2)
			continue;
		a = a + a2*(1-a);
//Con_Printf ("j:%i a:%f\n", j, a);
		a2 = a2/a;
		r = r*(1-a2) + cl.qh_cshifts[j].destcolor[0]*a2;
		g = g*(1-a2) + cl.qh_cshifts[j].destcolor[1]*a2;
		b = b*(1-a2) + cl.qh_cshifts[j].destcolor[2]*a2;
	}

	v_blend[0] = r/255.0;
	v_blend[1] = g/255.0;
	v_blend[2] = b/255.0;
	v_blend[3] = a;
	if (v_blend[3] > 1)
		v_blend[3] = 1;
	if (v_blend[3] < 0)
		v_blend[3] = 0;
}

/*
=============
V_UpdatePalette
=============
*/
void V_UpdatePalette (void)
{
	int		i, j;
	qboolean	is_new;

	V_CalcPowerupCshift ();
	
	is_new = false;
	
	for (i=0 ; i<NUM_CSHIFTS ; i++)
	{
		if (cl.qh_cshifts[i].percent != cl.qh_prev_cshifts[i].percent)
		{
			is_new = true;
			cl.qh_prev_cshifts[i].percent = cl.qh_cshifts[i].percent;
		}
		for (j=0 ; j<3 ; j++)
			if (cl.qh_cshifts[i].destcolor[j] != cl.qh_prev_cshifts[i].destcolor[j])
			{
				is_new = true;
				cl.qh_prev_cshifts[i].destcolor[j] = cl.qh_cshifts[i].destcolor[j];
			}
	}
	
// drop the damage value
	cl.qh_cshifts[CSHIFT_DAMAGE].percent -= host_frametime*150;
	if (cl.qh_cshifts[CSHIFT_DAMAGE].percent <= 0)
		cl.qh_cshifts[CSHIFT_DAMAGE].percent = 0;

// drop the bonus value
	cl.qh_cshifts[CSHIFT_BONUS].percent -= host_frametime*100;
	if (cl.qh_cshifts[CSHIFT_BONUS].percent <= 0)
		cl.qh_cshifts[CSHIFT_BONUS].percent = 0;

	if (!is_new)
		return;

	V_CalcBlend ();
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
	cl.viewent.state.angles[YAW] = viewangles[YAW];
	cl.viewent.state.angles[PITCH] = -viewangles[PITCH];

	cl.viewent.state.angles[ROLL] -= v_idlescale->value * sin(cl.serverTimeFloat*v_iroll_cycle->value) * v_iroll_level->value;
	cl.viewent.state.angles[PITCH] -= v_idlescale->value * sin(cl.serverTimeFloat*v_ipitch_cycle->value) * v_ipitch_level->value;
	cl.viewent.state.angles[YAW] -= v_idlescale->value * sin(cl.serverTimeFloat*v_iyaw_cycle->value) * v_iyaw_level->value;
}

/*
==============
V_BoundOffsets
==============
*/
static void V_BoundOffsets (void)
{
	h2entity_t	*ent;
	
	ent = &h2cl_entities[cl.viewentity];

// absolutely bound refresh reletive to entity clipping hull
// so the view can never be inside a solid wall

	if (cl.refdef.vieworg[0] < ent->state.origin[0] - 14)
		cl.refdef.vieworg[0] = ent->state.origin[0] - 14;
	else if (cl.refdef.vieworg[0] > ent->state.origin[0] + 14)
		cl.refdef.vieworg[0] = ent->state.origin[0] + 14;
	if (cl.refdef.vieworg[1] < ent->state.origin[1] - 14)
		cl.refdef.vieworg[1] = ent->state.origin[1] - 14;
	else if (cl.refdef.vieworg[1] > ent->state.origin[1] + 14)
		cl.refdef.vieworg[1] = ent->state.origin[1] + 14;

	if (cl.refdef.vieworg[2] < ent->state.origin[2] - 0)
		cl.refdef.vieworg[2] = ent->state.origin[2] - 0;
	else if (cl.refdef.vieworg[2] > ent->state.origin[2] + 86)
		cl.refdef.vieworg[2] = ent->state.origin[2] + 86;
}

/*
==============
V_AddIdle

Idle swaying
==============
*/
static void V_AddIdle(vec3_t viewangles)
{
	viewangles[ROLL] += v_idlescale->value * sin(cl.serverTimeFloat*v_iroll_cycle->value) * v_iroll_level->value;
	viewangles[PITCH] += v_idlescale->value * sin(cl.serverTimeFloat*v_ipitch_cycle->value) * v_ipitch_level->value;
	viewangles[YAW] += v_idlescale->value * sin(cl.serverTimeFloat*v_iyaw_cycle->value) * v_iyaw_level->value;
}


/*
==============
V_CalcViewRoll

Roll is induced by movement and damage
==============
*/
static void V_CalcViewRoll(vec3_t viewangles)
{
	float		side;
		
	side = V_CalcRoll (h2cl_entities[cl.viewentity].state.angles, cl.velocity);
	viewangles[ROLL] += side;

	if (v_dmg_time > 0)
	{
		viewangles[ROLL] += v_dmg_time/v_kicktime->value*v_dmg_roll;
		viewangles[PITCH] += v_dmg_time/v_kicktime->value*v_dmg_pitch;
		v_dmg_time -= host_frametime;
	}

	if (cl.v.health <= 0)
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
static void V_CalcIntermissionRefdef (void)
{
	h2entity_t	*ent, *view;
	float		old;

// ent is the player model (visible when out of body)
	ent = &h2cl_entities[cl.viewentity];
// view is the weapon model (only visible from inside body)
	view = &cl.viewent;

	VectorCopy (ent->state.origin, cl.refdef.vieworg);
	vec3_t viewangles;
	VectorCopy (ent->state.angles, viewangles);
	view->state.modelindex = 0;
	cl.refdef.vieworg[2] += cl.viewheight;

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
static void V_CalcRefdef (void)
{
	h2entity_t	*ent, *view;
	int			i;
	vec3_t		forward, right, up;
	vec3_t		angles;
	float		bob;
	static float oldz = 0;

	if (!cl.v.cameramode)
	{		
		V_DriftPitch ();
		V_DriftRoll ();
	}

// ent is the player model (visible when out of body)
	ent = &h2cl_entities[cl.viewentity];
// view is the weapon model (only visible from inside body)
	view = &cl.viewent;
	

// transform the view offset by the model's matrix to get the offset from
// model origin for the view
	ent->state.angles[YAW] = cl.viewangles[YAW];	// the model should face
										// the view dir
	ent->state.angles[PITCH] = -cl.viewangles[PITCH];	// the model should face
										// the view dir
										
	
	if (cl.v.movetype != MOVETYPE_FLY)
		bob = V_CalcBob ();
	else  // no bobbing when you fly
		bob = 1;

	
// refresh position
	VectorCopy (ent->state.origin, cl.refdef.vieworg);
	cl.refdef.vieworg[2] += cl.viewheight + bob;

// never let it sit exactly on a node line, because a water plane can
// dissapear when viewed with the eye exactly on it.
// the server protocol only specifies to 1/16 pixel, so add 1/32 in each axis
	cl.refdef.vieworg[0] += 1.0/32;
	cl.refdef.vieworg[1] += 1.0/32;
	cl.refdef.vieworg[2] += 1.0/32;

	vec3_t viewangles;
	VectorCopy (cl.viewangles, viewangles);
	V_CalcViewRoll(viewangles);
	V_AddIdle(viewangles);

// offsets
	angles[PITCH] = -ent->state.angles[PITCH];	// because entity pitches are
											//  actually backward
	angles[YAW] = ent->state.angles[YAW];
	angles[ROLL] = ent->state.angles[ROLL];

	AngleVectors (angles, forward, right, up);

	for (i=0 ; i<3 ; i++)
		cl.refdef.vieworg[i] += scr_ofsx->value*forward[i]
			+ scr_ofsy->value*right[i]
			+ scr_ofsz->value*up[i];
	
	
	V_BoundOffsets ();
		
// set up gun position
	VectorCopy (cl.viewangles, view->state.angles);
	
	CalcGunAngle(viewangles);

	VectorCopy (ent->state.origin, view->state.origin);
	view->state.origin[2] += cl.viewheight;

	for (i=0 ; i<3 ; i++)
	{
		view->state.origin[i] += forward[i]*bob*0.4;
//		view->origin[i] += right[i]*bob*0.4;
//		view->origin[i] += up[i]*bob*0.8;
	}
	view->state.origin[2] += bob;

	
// fudge position around to keep amount of weapon visible
// roughly equal with different FOV

	if (scr_viewsize->value == 110)
		view->state.origin[2] += 1;
	else if (scr_viewsize->value == 100)
		view->state.origin[2] += 2;
	else if (scr_viewsize->value == 90)
		view->state.origin[2] += 1;
	else if (scr_viewsize->value == 80)
		view->state.origin[2] += 0.5;

	view->state.modelindex = cl.stats[STAT_WEAPON];
	view->state.frame = cl.stats[STAT_WEAPONFRAME];

	// Place weapon in powered up mode
	if ((ent->state.drawflags & H2MLS_MASKIN) == H2MLS_POWERMODE)
		view->state.drawflags = (view->state.drawflags & H2MLS_MASKOUT) | H2MLS_POWERMODE;
	else
		view->state.drawflags = (view->state.drawflags & H2MLS_MASKOUT) | 0;

// set up the refresh position
	VectorAdd(viewangles, cl.punchangle, viewangles);
	AnglesToAxis(viewangles, cl.refdef.viewaxis);

// smooth out stair step ups
if (cl.onground && ent->state.origin[2] - oldz > 0)
{
	float steptime;
	
	steptime = cl.serverTimeFloat - cl.oldtime;
	if (steptime < 0)
//FIXME		I_Error ("steptime < 0");
		steptime = 0;

	oldz += steptime * 80;
	if (oldz > ent->state.origin[2])
		oldz = ent->state.origin[2];
	if (ent->state.origin[2] - oldz > 12)
		oldz = ent->state.origin[2] - 12;
	cl.refdef.vieworg[2] += oldz - ent->state.origin[2];
	view->state.origin[2] += oldz - ent->state.origin[2];
}
else
	oldz = ent->state.origin[2];

	if (chase_active->value)
		Chase_Update ();
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

	if (cl.items & IT_INVISIBILITY)
		return;

	if (cl.v.health <= 0)
		return;

	if (!cl.viewent.state.modelindex)
		return;

	if (chase_active->value)
		return;

	refEntity_t gun;

	Com_Memset(&gun, 0, sizeof(gun));
	gun.reType = RT_MODEL;
	gun.renderfx = RF_MINLIGHT | RF_FIRST_PERSON | RF_DEPTHHACK;
	VectorCopy(cl.viewent.state.origin, gun.origin);
	gun.hModel = cl.model_precache[cl.viewent.state.modelindex];
	gun.frame = cl.viewent.state.frame;
	gun.shaderTime = cl.viewent.syncbase;
	gun.skinNum = cl.viewent.state.skinnum;
	CLH2_SetRefEntAxis(&gun, cl.viewent.state.angles, vec3_origin, cl.viewent.state.scale, cl.viewent.state.colormap, cl.viewent.state.abslight, cl.viewent.state.drawflags);
	CLH2_HandleCustomSkin(&gun, -1);

	R_AddRefEntityToScene(&gun);
}

/*
============
R_PolyBlend
============
*/
static void R_PolyBlend (void)
{
	if (!cl_polyblend->value)
		return;
	if (!v_blend[3])
		return;

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

/*
==================
V_RenderView

The player's clipping box goes from (-16 -16 -24) to (16 16 32) from
the entity origin, so any view position inside that will be valid
==================
*/
void V_RenderView (void)
{
	if (con_forcedup)
		return;

// don't allow cheats in multiplayer
	if (cl.qh_maxclients > 1)
	{
		Cvar_Set ("scr_ofsx", "0");
		Cvar_Set ("scr_ofsy", "0");
		Cvar_Set ("scr_ofsz", "0");
	}

	if (cl.intermission)
	{	// intermission / finale rendering
		V_CalcIntermissionRefdef ();	
	}
	else
	{
		if (!cl.paused /* && (sv.maxclients > 1 || in_keyCatchers == 0) */ )
			V_CalcRefdef ();
	}
	CL_AddViewModel();

	CL_AddDLights();

	V_RenderScene ();
}

//============================================================================

/*
=============
V_Init
=============
*/
void V_Init (void)
{
	Cmd_AddCommand ("v_cshift", V_cshift_f);	
	Cmd_AddCommand ("bf", V_BonusFlash_f);
	Cmd_AddCommand ("df", V_DarkFlash_f);
	Cmd_AddCommand ("wf", V_WhiteFlash_f);

	Cmd_AddCommand ("centerview", V_StartPitchDrift);

	v_centermove = Cvar_Get("v_centermove", "0.15", 0);
	v_centerspeed = Cvar_Get("v_centerspeed", "500", 0);
	v_centerrollspeed = Cvar_Get("v_centerrollspeed", "125", 0);

	v_iyaw_cycle = Cvar_Get("v_iyaw_cycle", "2", 0);
	v_iroll_cycle = Cvar_Get("v_iroll_cycle", "0.5", 0);
	v_ipitch_cycle = Cvar_Get("v_ipitch_cycle", "1", 0);
	v_iyaw_level = Cvar_Get("v_iyaw_level", "0.3", 0);
	v_iroll_level = Cvar_Get("v_iroll_level", "0.1", 0);
	v_ipitch_level = Cvar_Get("v_ipitch_level", "0.3", 0);

	v_idlescale = Cvar_Get("v_idlescale", "0", 0);
	crosshair = Cvar_Get("crosshair", "0", CVAR_ARCHIVE);

	scr_ofsx = Cvar_Get("scr_ofsx","0", 0);
	scr_ofsy = Cvar_Get("scr_ofsy","0", 0);
	scr_ofsz = Cvar_Get("scr_ofsz","0", 0);
	cl_rollspeed = Cvar_Get("cl_rollspeed", "200", 0);
	cl_rollangle = Cvar_Get("cl_rollangle", "2.0", 0);
	cl_bob = Cvar_Get("cl_bob","0.02", 0);
	cl_bobcycle = Cvar_Get("cl_bobcycle","0.6", 0);
	cl_bobup = Cvar_Get("cl_bobup","0.5", 0);

	v_kicktime = Cvar_Get("v_kicktime", "0.5", 0);
	v_kickroll = Cvar_Get("v_kickroll", "0.6", 0);
	v_kickpitch = Cvar_Get("v_kickpitch", "0.6", 0);

	r_drawviewmodel = Cvar_Get("r_drawviewmodel", "1", 0);

	cl_polyblend = Cvar_Get("cl_polyblend", "1", 0);
}
