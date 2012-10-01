// view.c -- player eye positioning

#include "quakedef.h"
#include "../../client/game/quake_hexen2/view.h"
#include "../../client/game/quake/local.h"

/*

The view is allowed to move slightly from it's true position for bobbing,
but if it exceeds 8 pixels linear distance (spherical, not box), the list of
entities sent from the server may not include everything in the pvs, especially
when crossing a water boudnary.

*/

Cvar* cl_crossx;
Cvar* cl_crossy;

static Cvar* cl_polyblend;

static hwplayer_state_t* view_message;

static cshift_t cshift_empty = { {130,80,50}, 0 };
static cshift_t cshift_water = { {130,80,50}, 128 };
static cshift_t cshift_slime = { {0,25,5}, 150 };
static cshift_t cshift_lava = { {255,80,0}, 150 };

static float v_blend[4];			// rgba 0.0 - 1.0

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
	v_dmg_roll = count * side * vqh_kickroll->value;

	side = DotProduct(from, forward);
	v_dmg_pitch = count * side * vqh_kickpitch->value;

	v_dmg_time = vqh_kicktime->value;
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
	else if ((int)cl.h2_v.artifact_active & H2ARTFLAG_INVISIBILITY)
	{
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[0] = 100;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[1] = 100;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[2] = 100;
		cl.qh_cshifts[CSHIFT_POWERUP].percent = 100;
	}
	else if ((int)cl.h2_v.artifact_active & H2ARTFLAG_INVINCIBILITY)
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
//common->Printf ("j:%i a:%f\n", j, a);
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
	R_ClearScene();

	CLHW_EmitEntities();

	VQH_AddViewModel();

	CLH2_UpdateEffects();

	CL_AddDLights();

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
	if (cls.state != CA_ACTIVE)
	{
		return;
	}

	if (!cl.qh_validsequence)
	{
		return;
	}

	// don't allow cheats in multiplayer
	Cvar_Set("r_fullbright", "0");

	hwframe_t* view_frame = &cl.hw_frames[clc.netchan.incomingSequence & UPDATE_MASK_HW];
	view_message = &view_frame->playerstate[cl.playernum];

	DropPunchAngle();
	if (cl.qh_intermission)
	{	// intermission / finale rendering
		VQH_CalcIntermissionRefdef();
	}
	else
	{
		VQH_CalcRefdef(NULL, view_message);
	}
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
	VQH_SharedInit();

	Cmd_AddCommand("v_cshift", V_cshift_f);
	Cmd_AddCommand("bf", V_BonusFlash_f);
	Cmd_AddCommand("df", V_DarkFlash_f);

	crosshair = Cvar_Get("crosshair", "0", CVAR_ARCHIVE);
	cl_crossx = Cvar_Get("cl_crossx", "0", CVAR_ARCHIVE);
	cl_crossy = Cvar_Get("cl_crossy", "0", CVAR_ARCHIVE);

	cl_polyblend = Cvar_Get("cl_polyblend", "1", 0);
}
