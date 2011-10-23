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
// view.c -- player eye positioning

#include "quakedef.h"

/*

The view is allowed to move slightly from it's true position for bobbing,
but if it exceeds 8 pixels linear distance (spherical, not box), the list of
entities sent from the server may not include everything in the pvs, especially
when crossing a water boudnary.

*/

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

Cvar*	crosshair;
Cvar*	crosshaircolor;

Cvar*  cl_crossx;
Cvar*  cl_crossy;

static Cvar*	gl_cshiftpercent;

static Cvar*  v_contentblend;

static Cvar*	r_drawviewmodel;

static Cvar*	v_centermove;
static Cvar*	v_centerspeed;

static Cvar*	cl_polyblend;

static float	v_dmg_time, v_dmg_roll, v_dmg_pitch;

extern	int			in_forward, in_forward2, in_back;

frame_t		*view_frame;
static player_state_t		*view_message;

static cshift_t	cshift_empty = { {130,80,50}, 0 };
static cshift_t	cshift_water = { {130,80,50}, 128 };
static cshift_t	cshift_slime = { {0,25,5}, 150 };
static cshift_t	cshift_lava = { {255,80,0}, 150 };

static float		v_blend[4];		// rgba 0.0 - 1.0

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
	side = fabs(side);
	
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
static float V_CalcBob (void)
{
	static	double	bobtime;
	static float	bob;
	float	cycle;
	
	if (cl.spectator)
		return 0;

	if (onground == -1)
		return bob;		// just use old value

	bobtime += host_frametime;
	cycle = bobtime - (int)(bobtime/cl_bobcycle->value)*cl_bobcycle->value;
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

	if (view_message->onground == -1 || cls.demoplayback )
	{
		cl.driftmove = 0;
		cl.pitchvel = 0;
		return;
	}

// don't count small mouse motion
	if (cl.nodrift)
	{
		if ( abs(cl.frames[(cls.netchan.outgoing_sequence-1)&UPDATE_MASK].cmd.forwardmove) < 200)
			cl.driftmove = 0;
		else
			cl.driftmove += host_frametime;
	
		if ( cl.driftmove > v_centermove->value)
		{
			V_StartPitchDrift ();
		}
		return;
	}
	
	delta = 0 - cl.viewangles[PITCH];

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
	float	side;
	float	count;
	
	armor = net_message.ReadByte ();
	blood = net_message.ReadByte ();
	for (i=0 ; i<3 ; i++)
		from[i] = net_message.ReadCoord();

	count = blood*0.5 + armor*0.5;
	if (count < 10)
		count = 10;

	cl.faceanimtime = cl.serverTimeFloat + 0.2;		// but sbar face into pain frame

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
	cl.cshifts[CSHIFT_BONUS].destcolor[0] = 215;
	cl.cshifts[CSHIFT_BONUS].destcolor[1] = 186;
	cl.cshifts[CSHIFT_BONUS].destcolor[2] = 69;
	cl.cshifts[CSHIFT_BONUS].percent = 50;
}

/*
=============
V_SetContentsColor

Underwater, lava, etc each has a color shift
=============
*/
static void V_SetContentsColor (int contents)
{
	if (!v_contentblend->value) {
		cl.cshifts[CSHIFT_CONTENTS] = cshift_empty;
		return;
	}

	switch (contents)
	{
	case BSP29CONTENTS_EMPTY:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_empty;
		break;
	case BSP29CONTENTS_LAVA:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_lava;
		break;
	case BSP29CONTENTS_SOLID:
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
static void V_CalcPowerupCshift (void)
{
	if (cl.stats[STAT_ITEMS] & IT_QUAD)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 0;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 255;
		cl.cshifts[CSHIFT_POWERUP].percent = 30;
	}
	else if (cl.stats[STAT_ITEMS] & IT_SUIT)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 255;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 0;
		cl.cshifts[CSHIFT_POWERUP].percent = 20;
	}
	else if (cl.stats[STAT_ITEMS] & IT_INVISIBILITY)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 100;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 100;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 100;
		cl.cshifts[CSHIFT_POWERUP].percent = 100;
	}
	else if (cl.stats[STAT_ITEMS] & IT_INVULNERABILITY)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 255;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 255;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 0;
		cl.cshifts[CSHIFT_POWERUP].percent = 30;
	}
	else
		cl.cshifts[CSHIFT_POWERUP].percent = 0;
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
		if (!gl_cshiftpercent->value)
			continue;

		a2 = ((cl.cshifts[j].percent * gl_cshiftpercent->value) / 100.0) / 255.0;

//		a2 = (cl.cshifts[j].percent/2)/255.0;
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
	cl.viewent.angles[YAW] = viewangles[YAW];
	cl.viewent.angles[PITCH] = -viewangles[PITCH];
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

	cl.viewent.angles[ROLL] -= v_idlescale->value * sin(cl.serverTimeFloat*v_iroll_cycle->value) * v_iroll_level->value;
	cl.viewent.angles[PITCH] -= v_idlescale->value * sin(cl.serverTimeFloat*v_ipitch_cycle->value) * v_ipitch_level->value;
	cl.viewent.angles[YAW] -= v_idlescale->value * sin(cl.serverTimeFloat*v_iyaw_cycle->value) * v_iyaw_level->value;
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
		
	side = V_CalcRoll (cl.simangles, cl.simvel);
	viewangles[ROLL] += side;

	if (v_dmg_time > 0)
	{
		viewangles[ROLL] += v_dmg_time/v_kicktime->value*v_dmg_roll;
		viewangles[PITCH] += v_dmg_time/v_kicktime->value*v_dmg_pitch;
		v_dmg_time -= host_frametime;
	}

}


/*
==================
V_CalcIntermissionRefdef

==================
*/
static void V_CalcIntermissionRefdef (void)
{
	entity_t	*view;
	float		old;

// view is the weapon model
	view = &cl.viewent;

	VectorCopy (cl.simorg, cl.refdef.vieworg);
	vec3_t viewangles;
	VectorCopy (cl.simangles, viewangles);
	view->model = 0;

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
	entity_t	*view;
	int			i;
	vec3_t		forward, right, up;
	float		bob;
	static float oldz = 0;

	V_DriftPitch ();

// view is the weapon model (only visible from inside body)
	view = &cl.viewent;

	bob = V_CalcBob ();
	
// refresh position from simulated origin
	VectorCopy (cl.simorg, cl.refdef.vieworg);

	cl.refdef.vieworg[2] += bob;

// never let it sit exactly on a node line, because a water plane can
// dissapear when viewed with the eye exactly on it.
// the server protocol only specifies to 1/8 pixel, so add 1/16 in each axis
	cl.refdef.vieworg[0] += 1.0/16;
	cl.refdef.vieworg[1] += 1.0/16;
	cl.refdef.vieworg[2] += 1.0/16;

	vec3_t viewangles;
	VectorCopy (cl.simangles, viewangles);
	V_CalcViewRoll(viewangles);
	V_AddIdle(viewangles);

	if (view_message->flags & PF_GIB)
		cl.refdef.vieworg[2] += 8;	// gib view height
	else if (view_message->flags & PF_DEAD)
		cl.refdef.vieworg[2] -= 16;	// corpse view height
	else
		cl.refdef.vieworg[2] += 22;	// view height

	if (view_message->flags & PF_DEAD)		// PF_GIB will also set PF_DEAD
		viewangles[ROLL] = 80;	// dead view angle


// offsets
	AngleVectors (cl.simangles, forward, right, up);
	
// set up gun position
	VectorCopy (cl.simangles, view->angles);
	
	CalcGunAngle(viewangles);

	VectorCopy (cl.simorg, view->origin);
	view->origin[2] += 22;

	for (i=0 ; i<3 ; i++)
	{
		view->origin[i] += forward[i]*bob*0.4;
//		view->origin[i] += right[i]*bob*0.4;
//		view->origin[i] += up[i]*bob*0.8;
	}
	view->origin[2] += bob;

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

	if (view_message->flags & (PF_GIB|PF_DEAD) )
 		view->model = 0;
 	else
		view->model = cl.model_precache[cl.stats[STAT_WEAPON]];
	view->frame = view_message->weaponframe;

// set up the refresh position
	viewangles[PITCH] += cl.punchangle;
	AnglesToAxis(viewangles, cl.refdef.viewaxis);

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
		cl.refdef.vieworg[2] += oldz - cl.simorg[2];
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

	if (!Cam_DrawViewModel())
		return;

	if (cl.stats[STAT_ITEMS] & IT_INVISIBILITY)
		return;

	if (cl.stats[STAT_HEALTH] <= 0)
		return;

	if (!cl.viewent.model)
		return;

	refEntity_t gun;

	Com_Memset(&gun, 0, sizeof(gun));
	gun.reType = RT_MODEL;
	gun.renderfx = RF_MINLIGHT | RF_FIRST_PERSON | RF_DEPTHHACK;
	VectorCopy(cl.viewent.origin, gun.origin);
	gun.hModel = cl.viewent.model;
	CL_SetRefEntAxis(&gun, cl.viewent.angles);
	gun.frame = cl.viewent.frame;
	gun.skinNum = cl.viewent.skinnum;
	gun.shaderTime = cl.viewent.syncbase;

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
	Cvar_Set("r_fullbright", "0");
	Cvar_Set("r_lightmap", "0");
	if (!String::Atoi(Info_ValueForKey(cl.serverinfo, "watervis")))
	{
		Cvar_Set("r_wateralpha", "1");
	}

	for (int i = 0; i < MAX_LIGHTSTYLES_Q1; i++)
	{
		float Val = cl_lightstylevalue[i] / 256.0;
		R_AddLightStyleToScene(i, Val, Val, Val);
	}
	CL_AddParticles();

	CL_AnimateLight();

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
//	if (cl.simangles[ROLL])
//		Sys_Error ("cl.simangles[ROLL]");	// DEBUG
cl.simangles[ROLL] = 0;	// FIXME @@@ 

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
	CL_AddViewModel();

	cdlight_t* l = cl_dlights;
	for (int i = 0; i < MAX_DLIGHTS; i++, l++)
	{
		if (l->die < cl.serverTime || !l->radius)
		{
			continue;
		}
		R_AddLightToScene(l->origin, l->radius, 1, 1, 1);
	}

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
	Cmd_AddCommand ("centerview", V_StartPitchDrift);

	v_centermove = Cvar_Get("v_centermove", "0.15", 0);
	v_centerspeed = Cvar_Get("v_centerspeed", "500", 0);

	v_iyaw_cycle = Cvar_Get("v_iyaw_cycle", "2", 0);
	v_iroll_cycle = Cvar_Get("v_iroll_cycle", "0.5", 0);
	v_ipitch_cycle = Cvar_Get("v_ipitch_cycle", "1", 0);
	v_iyaw_level = Cvar_Get("v_iyaw_level", "0.3", 0);
	v_iroll_level = Cvar_Get("v_iroll_level", "0.1", 0);
	v_ipitch_level = Cvar_Get("v_ipitch_level", "0.3", 0);

	v_contentblend = Cvar_Get("v_contentblend", "1", 0);

	v_idlescale = Cvar_Get("v_idlescale", "0", 0);
	crosshaircolor = Cvar_Get("crosshaircolor", "79", CVAR_ARCHIVE);
	crosshair = Cvar_Get("crosshair", "0", CVAR_ARCHIVE);
	cl_crossx = Cvar_Get("cl_crossx", "0", CVAR_ARCHIVE);
	cl_crossy = Cvar_Get("cl_crossy", "0", CVAR_ARCHIVE);
	gl_cshiftpercent = Cvar_Get("gl_cshiftpercent", "100", 0);

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
