// view.c -- player eye positioning

#include "quakedef.h"
#include "glquake.h"

/*

The view is allowed to move slightly from it's true position for bobbing,
but if it exceeds 8 pixels linear distance (spherical, not box), the list of
entities sent from the server may not include everything in the pvs, especially
when crossing a water boudnary.

*/

QCvar*	cl_rollspeed;
QCvar*	cl_rollangle;

QCvar*	cl_bob;
QCvar*	cl_bobcycle;
QCvar*	cl_bobup;

QCvar*	v_kicktime;
QCvar*	v_kickroll;
QCvar*	v_kickpitch;

QCvar*	v_iyaw_cycle;
QCvar*	v_iroll_cycle;
QCvar*	v_ipitch_cycle;
QCvar*	v_iyaw_level;
QCvar*	v_iroll_level;
QCvar*	v_ipitch_level;

QCvar*	v_idlescale;

QCvar*	crosshair;

QCvar*  cl_crossx;
QCvar*  cl_crossy;

float	v_dmg_time, v_dmg_roll, v_dmg_pitch;

float	v_targAngle;
float	v_targPitch;
float	v_targDist = 0.0;

extern	int			in_forward, in_forward2, in_back;

frame_t		*view_frame;
player_state_t		*view_message;

/*
===============
V_CalcRoll

===============
*/
float V_CalcRoll (vec3_t angles, vec3_t velocity)
{
	vec3_t	forward, right, up;
	float	sign;
	float	side;
	float	value;
	
	AngleVectors (angles, forward, right, up);
	side = DotProduct (velocity, right);
	sign = side < 0 ? -1 : 1;
	side = Q_fabs(side);
	
	value = cl_rollangle->value;

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
float V_CalcBob (void)
{
//	static	double	bobtime;
	static float	bob;
	float	cycle;
	
	if (cl.spectator)
		return 0;

//	if (onground == -1)
//		return bob;		// just use old value

//	bobtime += host_frametime;
//	cycle = bobtime - (int)(bobtime/cl_bobcycle->value)*cl_bobcycle->value;
	cycle = cl.time - (int)(cl.time/cl_bobcycle->value)*cl_bobcycle->value;
	cycle /= cl_bobcycle->value;
	if (cycle < cl_bobup->value)
		cycle = M_PI * cycle / cl_bobup->value;
	else
		cycle = M_PI + M_PI*(cycle-cl_bobup->value)/(1.0 - cl_bobup->value);

// bob is proportional to simulated velocity in the xy plane
// (don't count Z, or jumping messes it up)

	bob = sqrt(cl.simvel[0]*cl.simvel[0] + cl.simvel[1]*cl.simvel[1]) * cl_bob->value;
	bob = bob*0.3 + bob*0.7*sin(cycle);
	if (bob > 4)
		bob = 4;
	else if (bob < -7)
		bob = -7;
	return bob;
	
}


//=============================================================================


QCvar*	v_centermove;
QCvar*	v_centerspeed;

QCvar*	v_centerrollspeed;

void V_StartPitchDrift (void)
{
#if 1
	if (cl.laststop == cl.time)
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
	cl.laststop = cl.time;
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
void V_DriftPitch (void)
{
	float		delta, move;

	if (view_message->onground == -1 || cls.demoplayback )
	{
		cl.driftmove = 0;
		cl.pitchvel = 0;
		return;
	}

// don't count small mouse motion
	if (cl.nodrift)
	{
		if ( Q_fabs(cl.frames[(cls.netchan.outgoing_sequence-1)&UPDATE_MASK].cmd.forwardmove) < (cl.v.hasted*cl_forwardspeed->value)-10 || lookspring->value == 0.0)
			cl.driftmove = 0;
		else
			cl.driftmove += host_frametime;

		if (cl.spectator)
		{
			cl.driftmove = 0;
		}

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
void V_DriftRoll (void)
{
	float		delta, move;
	float		rollspeed;

	if (view_message->onground == -1 || cls.demoplayback )
	{
		if(cl.v.movetype != MOVETYPE_FLY)
		{
			cl.rollvel = 0;
			return;
		}
	}


	if(cl.v.movetype != MOVETYPE_FLY)
		rollspeed = v_centerrollspeed->value;
	else
		rollspeed = v_centerrollspeed->value * .5;	//slower roll when flying

	delta = cl.idealroll - cl.viewangles[ROLL];

	if (!delta)
	{
		cl.rollvel = 0;
		return;
	}


	move = host_frametime * cl.rollvel; 
	cl.rollvel += host_frametime * rollspeed;

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
===============
V_ParseTarget
===============
*/

void V_ParseTarget(void)
{
	v_targAngle = net_message.ReadByte();
	v_targPitch = net_message.ReadByte();
	v_targDist = net_message.ReadByte();
}


/*
============================================================================== 
 
						PALETTE FLASHES 
 
============================================================================== 
*/ 
 
 
cshift_t	cshift_empty = { {130,80,50}, 0 };
cshift_t	cshift_water = { {130,80,50}, 128 };
cshift_t	cshift_slime = { {0,25,5}, 150 };
cshift_t	cshift_lava = { {255,80,0}, 150 };

float		v_blend[4];		// rgba 0.0 - 1.0

void BuildGammaTable (float g)
{
	int		i, inf;
	
	if (g == 1.0)
	{
		for (i=0 ; i<256 ; i++)
			s_gammatable[i] = i;
		return;
	}
	
	for (i=0 ; i<256 ; i++)
	{
		inf = 255 * pow ( (i+0.5)/255.5 , (double)g ) + 0.5;
		if (inf < 0)
			inf = 0;
		if (inf > 255)
			inf = 255;
		s_gammatable[i] = inf;
	}
}

/*
=================
V_CheckGamma
=================
*/
qboolean V_CheckGamma (void)
{
	static float oldgammavalue;
	
	if (r_gamma->value == oldgammavalue)
		return false;
	oldgammavalue = r_gamma->value;
	
	BuildGammaTable (r_gamma->value);
	vid.recalc_refdef = 1;				// force a surface cache flush
	
	return true;
}


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
	float	side;
	float	count;
	
	armor = net_message.ReadByte ();
	blood = net_message.ReadByte ();
	for (i=0 ; i<3 ; i++)
		from[i] = net_message.ReadCoord();

	count = blood*0.5 + armor*0.5;
	if (count < 10)
		count = 10;

	cl.faceanimtime = cl.time + 0.2;		// but sbar face into pain frame

	cl.cshifts[CSHIFT_DAMAGE].percent += 3*count;
	if (cl.cshifts[CSHIFT_DAMAGE].percent < 0)
		cl.cshifts[CSHIFT_DAMAGE].percent = 0;
	if (cl.cshifts[CSHIFT_DAMAGE].percent > 150)
		cl.cshifts[CSHIFT_DAMAGE].percent = 150;

	if (armor > blood)		
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 200;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 100;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 100;
	}
	else if (armor)
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 220;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 50;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 50;
	}
	else
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 255;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 0;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 0;
	}

//
// calculate view angle kicks
//
	VectorSubtract (from, cl.simorg, from);
	VectorNormalize (from);
	
	AngleVectors (cl.simangles, forward, right, up);

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
void V_cshift_f (void)
{
	cshift_empty.destcolor[0] = QStr::Atoi(Cmd_Argv(1));
	cshift_empty.destcolor[1] = QStr::Atoi(Cmd_Argv(2));
	cshift_empty.destcolor[2] = QStr::Atoi(Cmd_Argv(3));
	cshift_empty.percent = QStr::Atoi(Cmd_Argv(4));
}


/*
==================
V_BonusFlash_f

When you run over an item, the server sends this command
==================
*/
void V_BonusFlash_f (void)
{
	cl.cshifts[CSHIFT_BONUS].destcolor[0] = 215;
	cl.cshifts[CSHIFT_BONUS].destcolor[1] = 186;
	cl.cshifts[CSHIFT_BONUS].destcolor[2] = 69;
	cl.cshifts[CSHIFT_BONUS].percent = 50;
}

void V_DarkFlash_f (void)
{
	cl.cshifts[CSHIFT_BONUS].destcolor[0] = 0;
	cl.cshifts[CSHIFT_BONUS].destcolor[1] = 0;
	cl.cshifts[CSHIFT_BONUS].destcolor[2] = 0;
	cl.cshifts[CSHIFT_BONUS].percent = 255;
}

/*
=============
V_SetContentsColor

Underwater, lava, etc each has a color shift
=============
*/
void V_SetContentsColor (int contents)
{
	switch (contents)
	{
	case BSP29CONTENTS_EMPTY:
	case BSP29CONTENTS_SOLID:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_empty;
		break;
	case BSP29CONTENTS_LAVA:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_lava;
		break;
	case BSP29CONTENTS_SLIME:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_slime;
		break;
	default:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_water;
	}
}

/*
=============
V_CalcPowerupCshift
=============
*/
void V_CalcPowerupCshift(void)
{
/*	if (cl.items & IT_QUAD)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 0;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 255;
		cl.cshifts[CSHIFT_POWERUP].percent = 30;
	}
	else if (cl.items & IT_SUIT)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 255;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 0;
		cl.cshifts[CSHIFT_POWERUP].percent = 20;
	}
*/
	if((int)cl.v.artifact_active&ARTFLAG_DIVINE_INTERVENTION)
	{
		cl.cshifts[CSHIFT_BONUS].destcolor[0] = 255;
		cl.cshifts[CSHIFT_BONUS].destcolor[1] = 255;
		cl.cshifts[CSHIFT_BONUS].destcolor[2] = 255;
		cl.cshifts[CSHIFT_BONUS].percent = 256;
	}

	if((int)cl.v.artifact_active&ARTFLAG_FROZEN)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 20;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 70;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 255;
		cl.cshifts[CSHIFT_POWERUP].percent = 65;
	}
	else if((int)cl.v.artifact_active&ARTFLAG_STONED)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 205;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 205;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 205;
		//cl.cshifts[CSHIFT_POWERUP].percent = 80;
		cl.cshifts[CSHIFT_POWERUP].percent = 11000;
	}
	else if((int)cl.v.artifact_active&ART_INVISIBILITY)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 100;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 100;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 100;
		cl.cshifts[CSHIFT_POWERUP].percent = 100;
	}
	else if((int)cl.v.artifact_active&ART_INVINCIBILITY)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 255;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 255;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 0;
		cl.cshifts[CSHIFT_POWERUP].percent = 30;
	}
	else
	{
		cl.cshifts[CSHIFT_POWERUP].percent = 0;
	}
}


/*
=============
V_CalcBlend
=============
*/
void V_CalcBlend (void)
{
	float	r, g, b, a, a2;
	int		j;

	r = 0;
	g = 0;
	b = 0;
	a = 0;

	for (j=0 ; j<NUM_CSHIFTS ; j++)	
	{
		if(cl.cshifts[j].percent > 10000)
		{ // Set percent for grayscale
			cl.cshifts[j].percent = 80;
		}

		a2 = cl.cshifts[j].percent/255.0;
		if (!a2)
			continue;
		a = a + a2*(1-a);
//Con_Printf ("j:%i a:%f\n", j, a);
		a2 = a2/a;
		r = r*(1-a2) + cl.cshifts[j].destcolor[0]*a2;
		g = g*(1-a2) + cl.cshifts[j].destcolor[1]*a2;
		b = b*(1-a2) + cl.cshifts[j].destcolor[2]*a2;
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
	qboolean force;

	V_CalcPowerupCshift ();
	
	is_new = false;
	
	for (i=0 ; i<NUM_CSHIFTS ; i++)
	{
		if (cl.cshifts[i].percent != cl.prev_cshifts[i].percent)
		{
			is_new = true;
			cl.prev_cshifts[i].percent = cl.cshifts[i].percent;
		}
		for (j=0 ; j<3 ; j++)
			if (cl.cshifts[i].destcolor[j] != cl.prev_cshifts[i].destcolor[j])
			{
				is_new = true;
				cl.prev_cshifts[i].destcolor[j] = cl.cshifts[i].destcolor[j];
			}
	}

// drop the damage value
	cl.cshifts[CSHIFT_DAMAGE].percent -= host_frametime*150;
	if (cl.cshifts[CSHIFT_DAMAGE].percent <= 0)
		cl.cshifts[CSHIFT_DAMAGE].percent = 0;

// drop the bonus value
	cl.cshifts[CSHIFT_BONUS].percent -= host_frametime*100;
	if (cl.cshifts[CSHIFT_BONUS].percent <= 0)
		cl.cshifts[CSHIFT_BONUS].percent = 0;

	force = V_CheckGamma ();
	if (!is_new && !force)
		return;

	V_CalcBlend ();
}

/* 
============================================================================== 
 
						VIEW RENDERING 
 
============================================================================== 
*/ 

float angledelta (float a)
{
	a = AngleMod(a);
	if (a > 180)
		a -= 360;
	return a;
}

/*
==================
CalcGunAngle
==================
*/
void CalcGunAngle (void)
{	
	float	yaw, pitch, move;
	static float oldyaw = 0;
	static float oldpitch = 0;
	
	yaw = r_refdef.viewangles[YAW];
	pitch = -r_refdef.viewangles[PITCH];

	yaw = angledelta(yaw - r_refdef.viewangles[YAW]) * 0.4;
	if (yaw > 10)
		yaw = 10;
	if (yaw < -10)
		yaw = -10;
	pitch = angledelta(-pitch - r_refdef.viewangles[PITCH]) * 0.4;
	if (pitch > 10)
		pitch = 10;
	if (pitch < -10)
		pitch = -10;
	move = host_frametime*20;
	if (yaw > oldyaw)
	{
		if (oldyaw + move < yaw)
			yaw = oldyaw + move;
	}
	else
	{
		if (oldyaw - move > yaw)
			yaw = oldyaw - move;
	}
	
	if (pitch > oldpitch)
	{
		if (oldpitch + move < pitch)
			pitch = oldpitch + move;
	}
	else
	{
		if (oldpitch - move > pitch)
			pitch = oldpitch - move;
	}
	
	oldyaw = yaw;
	oldpitch = pitch;

	cl.viewent.angles[YAW] = r_refdef.viewangles[YAW] + yaw;
	cl.viewent.angles[PITCH] = - (r_refdef.viewangles[PITCH] + pitch);

	cl.viewent.angles[ROLL] -= v_idlescale->value * sin(cl.time*v_iroll_cycle->value) * v_iroll_level->value;
	cl.viewent.angles[PITCH] -= v_idlescale->value * sin(cl.time*v_ipitch_cycle->value) * v_ipitch_level->value;
	cl.viewent.angles[YAW] -= v_idlescale->value * sin(cl.time*v_iyaw_cycle->value) * v_iyaw_level->value;
}

/*
==============
V_BoundOffsets
==============
*/
void V_BoundOffsets (void)
{
// absolutely bound refresh reletive to entity clipping hull
// so the view can never be inside a solid wall

	if (r_refdef.vieworg[0] < cl.simorg[0] - 14)
		r_refdef.vieworg[0] = cl.simorg[0] - 14;
	else if (r_refdef.vieworg[0] > cl.simorg[0] + 14)
		r_refdef.vieworg[0] = cl.simorg[0] + 14;
	if (r_refdef.vieworg[1] < cl.simorg[1] - 14)
		r_refdef.vieworg[1] = cl.simorg[1] - 14;
	else if (r_refdef.vieworg[1] > cl.simorg[1] + 14)
		r_refdef.vieworg[1] = cl.simorg[1] + 14;
	if (r_refdef.vieworg[2] < cl.simorg[2] - 0)
		r_refdef.vieworg[2] = cl.simorg[2] - 0;
	else if (r_refdef.vieworg[2] > cl.simorg[2] + 86)
		r_refdef.vieworg[2] = cl.simorg[2] + 86;
}

/*
==============
V_AddIdle

Idle swaying
==============
*/
void V_AddIdle (void)
{
	r_refdef.viewangles[ROLL] += v_idlescale->value * sin(cl.time*v_iroll_cycle->value) * v_iroll_level->value;
	r_refdef.viewangles[PITCH] += v_idlescale->value * sin(cl.time*v_ipitch_cycle->value) * v_ipitch_level->value;
	r_refdef.viewangles[YAW] += v_idlescale->value * sin(cl.time*v_iyaw_cycle->value) * v_iyaw_level->value;

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
void V_CalcViewRoll (void)
{
	float		side;
		
	side = V_CalcRoll (cl.simangles, cl.simvel);
	r_refdef.viewangles[ROLL] += side;

	if (v_dmg_time > 0)
	{
		r_refdef.viewangles[ROLL] += v_dmg_time/v_kicktime->value*v_dmg_roll;
		r_refdef.viewangles[PITCH] += v_dmg_time/v_kicktime->value*v_dmg_pitch;
		v_dmg_time -= host_frametime;
	}

	if (cl.v.health <= 0 && !cl.spectator)
	{
		r_refdef.viewangles[ROLL] = 80;	// dead view angle
		return;
	}
}


/*
==================
V_CalcIntermissionRefdef

==================
*/
void V_CalcIntermissionRefdef (void)
{
	entity_t	*view;
	float		old;

// view is the weapon model
	view = &cl.viewent;

	VectorCopy (cl.simorg, r_refdef.vieworg);
	VectorCopy (cl.simangles, r_refdef.viewangles);
	view->model = NULL;

// allways idle in intermission
	old = v_idlescale->value;
	v_idlescale->value = 1;
	V_AddIdle ();
	v_idlescale->value = old;
}

/*
==================
V_CalcRefdef

==================
*/
void V_CalcRefdef (void)
{
	entity_t	*view;
	int			i;
	vec3_t		forward, right, up;
	float		bob;
	static float oldz = 0;

	if (!cl.v.cameramode)
	{		
		V_DriftPitch ();
		V_DriftRoll ();
	}

// view is the weapon model (only visible from inside body)
	view = &cl.viewent;

	if (cl.v.movetype != MOVETYPE_FLY)
		bob = V_CalcBob ();
	else  // no bobbing when you fly
		bob = 1;

// refresh position from simulated origin
	VectorCopy (cl.simorg, r_refdef.vieworg);

	r_refdef.vieworg[2] += bob;

// never let it sit exactly on a node line, because a water plane can
// dissapear when viewed with the eye exactly on it.
// the server protocol only specifies to 1/8 pixel, so add 1/16 in each axis
	r_refdef.vieworg[0] += 1.0/32;
	r_refdef.vieworg[1] += 1.0/32;
	r_refdef.vieworg[2] += 1.0/32;

	VectorCopy (cl.simangles, r_refdef.viewangles);
	V_CalcViewRoll ();
	V_AddIdle ();

	if (cl.spectator)
	{
		r_refdef.vieworg[2] += 50;	// view height
	}
	else
	{
		if (view_message->flags & PF_CROUCH)
			r_refdef.vieworg[2] += 24;	// gib view height
		else if (view_message->flags & PF_DEAD)
			r_refdef.vieworg[2] += 8;	// corpse view height
		else
			r_refdef.vieworg[2] += 50;	// view height

		if (view_message->flags & PF_DEAD)		// PF_GIB will also set PF_DEAD
			r_refdef.viewangles[ROLL] = 80;	// dead view angle
	}

// offsets
	AngleVectors (cl.simangles, forward, right, up);
	
// set up gun position
	VectorCopy (cl.simangles, view->angles);
	
	CalcGunAngle ();

	VectorCopy (r_refdef.vieworg, view->origin);
//	view->origin[2] += 56;

	for (i=0 ; i<3 ; i++)
	{
		view->origin[i] += forward[i]*bob*0.4;
//		view->origin[i] += right[i]*bob*0.4;
//		view->origin[i] += up[i]*bob*0.8;
	}
// rjr no idea why commenting this out works, as it is in the original q1 codebase
// view->origin[2] += bob;

// fudge position around to keep amount of weapon visible
// roughly equal with different FOV
	if (scr_viewsize->value == 110)
		view->origin[2] += 1;
	else if (scr_viewsize->value == 100)
		view->origin[2] += 2;
	else if (scr_viewsize->value == 90)
		view->origin[2] += 1;
	else if (scr_viewsize->value == 80)
		view->origin[2] += 0.5;

	if (view_message->flags & (PF_DEAD) )
 		view->model = NULL;
 	else
		view->model = cl.model_precache[cl.stats[STAT_WEAPON]];
	view->frame = view_message->weaponframe;
	if (!view->colorshade)
	{
		view->colormap = vid.colormap;
	}
	else
	{
		view->colormap = vid.colormap;
	}

	// Place weapon in powered up mode
	if ((cl.frames[cls.netchan.incoming_sequence&UPDATE_MASK].playerstate[cl.playernum].drawflags & MLS_MASKIN) == MLS_POWERMODE)
		view->drawflags = (view->drawflags & MLS_MASKOUT) | MLS_POWERMODE;
	else
		view->drawflags = (view->drawflags & MLS_MASKOUT) | 0;

// set up the refresh position
	r_refdef.viewangles[PITCH] += cl.punchangle;

// smooth out stair step ups
	if ( (view_message->onground != -1) && (cl.simorg[2] - oldz > 0) )
	{
		float steptime;
		
		steptime = host_frametime;
	
		oldz += steptime * 80;
		if (oldz > cl.simorg[2])
			oldz = cl.simorg[2];
		if (cl.simorg[2] - oldz > 12)
			oldz = cl.simorg[2] - 12;
		r_refdef.vieworg[2] += oldz - cl.simorg[2];
		view->origin[2] += oldz - cl.simorg[2];
	}
	else
		oldz = cl.simorg[2];
}

/*
=============
DropPunchAngle
=============
*/
void DropPunchAngle (void)
{
	cl.punchangle -= 10*host_frametime;
	if (cl.punchangle < 0)
		cl.punchangle = 0;
}

/*
==================
V_RenderView

The player's clipping box goes from (-16 -16 -24) to (16 16 32) from
the entity origin, so any view position inside that will be valid
==================
*/
extern vrect_t scr_vrect;

void V_RenderView (void)
{
//	if (cl.simangles[ROLL])
//		Sys_Error ("cl.simangles[ROLL]");	// DEBUG
//rjrcl.simangles[ROLL] = 0;	// FIXME @@@ 

	if (cls.state != ca_active)
		return;

	view_frame = &cl.frames[cls.netchan.incoming_sequence & UPDATE_MASK];
	view_message = &view_frame->playerstate[cl.playernum];

	DropPunchAngle ();
	if (cl.intermission)
	{	// intermission / finale rendering
		V_CalcIntermissionRefdef ();	
	}
	else
	{
		V_CalcRefdef ();
	}

	CL_UpdateEffects ();

	R_PushDlights ();
	R_RenderView ();
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
	Cmd_AddCommand ("centerview", V_StartPitchDrift);

	v_centermove = Cvar_Get("v_centermove", "0.15", 0);
	v_centerspeed = Cvar_Get("v_centerspeed","500", 0);
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

	cl_rollspeed = Cvar_Get("cl_rollspeed", "200", 0);
	cl_rollangle = Cvar_Get("cl_rollangle", "2.0", 0);
	cl_bob = Cvar_Get("cl_bob","0.02", 0);
	cl_bobcycle = Cvar_Get("cl_bobcycle","0.6", 0);
	cl_bobup = Cvar_Get("cl_bobup","0.5", 0);

	v_kicktime = Cvar_Get("v_kicktime", "0.5", 0);
	v_kickroll = Cvar_Get("v_kickroll", "0.6", 0);
	v_kickpitch = Cvar_Get("v_kickpitch", "0.6", 0);

	BuildGammaTable (1.0);	// no gamma yet
}
