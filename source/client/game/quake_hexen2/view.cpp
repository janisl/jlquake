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

#include "view.h"
#include "../../client_main.h"
#include "../../ui/ui.h"
#include "../../chase.h"
#include "../particles.h"
#include "../camera.h"
#include "../dynamic_lights.h"
#include "../light_styles.h"
#include "../quake/local.h"
#include "../hexen2/local.h"

//	The view is allowed to move slightly from it's true position for bobbing,
// but if it exceeds 8 pixels linear distance (spherical, not box), the list
// of entities sent from the server may not include everything in the pvs,
// especially when crossing a water boudnary.

static Cvar* v_contentblend;
static Cvar* gl_cshiftpercent;

static Cvar* vqh_centermove;
static Cvar* vh2_centerrollspeed;

static Cvar* clqh_bob;
static Cvar* clqh_bobcycle;
static Cvar* clqh_bobup;

static Cvar* vqh_kicktime;
static Cvar* vqh_kickroll;
static Cvar* vqh_kickpitch;

static Cvar* vqh_idlescale;
static Cvar* vqh_iyaw_cycle;
static Cvar* vqh_iroll_cycle;
static Cvar* vqh_ipitch_cycle;
static Cvar* vqh_iyaw_level;
static Cvar* vqh_iroll_level;
static Cvar* vqh_ipitch_level;

static Cvar* vqh_drawviewmodel;

static Cvar* crosshaircolor;
static Cvar* cl_crossx;
static Cvar* cl_crossy;

static Cvar* scr_fov;

static Cvar* scr_showturtle;

static float v_dmg_time;
static float v_dmg_roll;
static float v_dmg_pitch;

static cshift_t cshift_empty = { {130,80,50}, 0 };
static cshift_t cshift_water = { {130,80,50}, 128 };
static cshift_t cshift_slime = { {0,25,5}, 150 };
static cshift_t cshift_lava = { {255,80,0}, 150 };

static image_t* cs_texture;	// crosshair texture

static image_t* scr_turtle;

/*
==============================================================================

                        PALETTE FLASHES

==============================================================================
*/

void VQH_ParseDamage(QMsg& message)
{
	int armor = message.ReadByte();
	int blood = message.ReadByte();
	vec3_t from;
	message.ReadPos(from);

	float count = blood * 0.5 + armor * 0.5;
	if (count < 10)
	{
		count = 10;
	}

	if (GGameType & GAME_Quake)
	{
		cl.q1_faceanimtime = cl.qh_serverTimeFloat + 0.2;		// but sbar face into pain frame
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
	vec3_t forward, right, up;
	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		VectorSubtract(from, cl.qh_simorg, from);
		VectorNormalize(from);

		AngleVectors(cl.qh_simangles, forward, right, up);
	}
	else if (GGameType & GAME_Quake)
	{
		q1entity_t* ent = &clq1_entities[cl.viewentity];

		VectorSubtract(from, ent->state.origin, from);
		VectorNormalize(from);

		AngleVectors(ent->state.angles, forward, right, up);
	}
	else
	{
		h2entity_t* ent = &h2cl_entities[cl.viewentity];

		VectorSubtract(from, ent->state.origin, from);
		VectorNormalize(from);

		AngleVectors(ent->state.angles, forward, right, up);
	}

	float side = DotProduct(from, right);
	v_dmg_roll = count * side * vqh_kickroll->value;

	side = DotProduct(from, forward);
	v_dmg_pitch = count * side * vqh_kickpitch->value;

	v_dmg_time = vqh_kicktime->value;
}

static void V_cshift_f()
{
	cshift_empty.destcolor[0] = String::Atoi(Cmd_Argv(1));
	cshift_empty.destcolor[1] = String::Atoi(Cmd_Argv(2));
	cshift_empty.destcolor[2] = String::Atoi(Cmd_Argv(3));
	cshift_empty.percent = String::Atoi(Cmd_Argv(4));
}

//	When you run over an item, the server sends this command
static void V_BonusFlash_f()
{
	cl.qh_cshifts[CSHIFT_BONUS].destcolor[0] = 215;
	cl.qh_cshifts[CSHIFT_BONUS].destcolor[1] = 186;
	cl.qh_cshifts[CSHIFT_BONUS].destcolor[2] = 69;
	cl.qh_cshifts[CSHIFT_BONUS].percent = 50;
}

static void V_DarkFlash_f()
{
	cl.qh_cshifts[CSHIFT_BONUS].destcolor[0] = 0;
	cl.qh_cshifts[CSHIFT_BONUS].destcolor[1] = 0;
	cl.qh_cshifts[CSHIFT_BONUS].destcolor[2] = 0;
	cl.qh_cshifts[CSHIFT_BONUS].percent = 255;
}

static void V_WhiteFlash_f()
{
	cl.qh_cshifts[CSHIFT_BONUS].destcolor[0] = 255;
	cl.qh_cshifts[CSHIFT_BONUS].destcolor[1] = 255;
	cl.qh_cshifts[CSHIFT_BONUS].destcolor[2] = 255;
	cl.qh_cshifts[CSHIFT_BONUS].percent = 255;
}

//	Underwater, lava, etc each has a color shift
static void VQH_SetContentsColor(int contents)
{
	if (!v_contentblend->value)
	{
		cl.qh_cshifts[CSHIFT_CONTENTS] = cshift_empty;
		return;
	}

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

static void VQ1_CalcPowerupCshift()
{
	int items = GGameType & GAME_QuakeWorld ? cl.qh_stats[QWSTAT_ITEMS] : cl.q1_items;

	if (items & Q1IT_QUAD)
	{
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[1] = 0;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[2] = 255;
		cl.qh_cshifts[CSHIFT_POWERUP].percent = 30;
	}
	else if (items & Q1IT_SUIT)
	{
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[1] = 255;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[2] = 0;
		cl.qh_cshifts[CSHIFT_POWERUP].percent = 20;
	}
	else if (items & Q1IT_INVISIBILITY)
	{
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[0] = 100;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[1] = 100;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[2] = 100;
		cl.qh_cshifts[CSHIFT_POWERUP].percent = 100;
	}
	else if (items & Q1IT_INVULNERABILITY)
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

static void VH2_CalcPowerupCshift()
{
	if ((int)cl.h2_v.artifact_active & (GGameType & GAME_HexenWorld ? HWARTFLAG_DIVINE_INTERVENTION : H2ARTFLAG_DIVINE_INTERVENTION))
	{
		cl.qh_cshifts[CSHIFT_BONUS].destcolor[0] = 255;
		cl.qh_cshifts[CSHIFT_BONUS].destcolor[1] = 255;
		cl.qh_cshifts[CSHIFT_BONUS].destcolor[2] = 255;
		cl.qh_cshifts[CSHIFT_BONUS].percent = 256;
	}

	if ((int)cl.h2_v.artifact_active & (GGameType & GAME_HexenWorld ? HWARTFLAG_FROZEN : H2ARTFLAG_FROZEN))
	{
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[0] = 20;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[1] = 70;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[2] = 255;
		cl.qh_cshifts[CSHIFT_POWERUP].percent = 65;
	}
	else if ((int)cl.h2_v.artifact_active & (GGameType & GAME_HexenWorld ? HWARTFLAG_STONED : H2ARTFLAG_STONED))
	{
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[0] = 205;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[1] = 205;
		cl.qh_cshifts[CSHIFT_POWERUP].destcolor[2] = 205;
		//JL FIXME grayscale
		cl.qh_cshifts[CSHIFT_POWERUP].percent = 80;
		//cl.qh_cshifts[CSHIFT_POWERUP].percent = 11000;
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

static void VQH_UpdateCShifts()
{
	// drop the damage value
	cl.qh_cshifts[CSHIFT_DAMAGE].percent -= cls.frametime * 0.15f;
	if (cl.qh_cshifts[CSHIFT_DAMAGE].percent <= 0)
	{
		cl.qh_cshifts[CSHIFT_DAMAGE].percent = 0;
	}

	// drop the bonus value
	cl.qh_cshifts[CSHIFT_BONUS].percent -= cls.frametime * 0.1f;
	if (cl.qh_cshifts[CSHIFT_BONUS].percent <= 0)
	{
		cl.qh_cshifts[CSHIFT_BONUS].percent = 0;
	}
}

static void VQH_CalcBlend(float* blendColour)
{
	float r = 0;
	float g = 0;
	float b = 0;
	float a = 0;

	for (int j = 0; j < NUM_CSHIFTS; j++)
	{
		if (!gl_cshiftpercent->value)
		{
			continue;
		}

		float a2 = (cl.qh_cshifts[j].percent * gl_cshiftpercent->value / 100.0) / 255.0;
		if (!a2)
		{
			continue;
		}
		a = a + a2 * (1 - a);
		a2 = a2 / a;
		r = r * (1 - a2) + cl.qh_cshifts[j].destcolor[0] * a2;
		g = g * (1 - a2) + cl.qh_cshifts[j].destcolor[1] * a2;
		b = b * (1 - a2) + cl.qh_cshifts[j].destcolor[2] * a2;
	}

	blendColour[0] = r / 255.0;
	blendColour[1] = g / 255.0;
	blendColour[2] = b / 255.0;
	blendColour[3] = a;
	if (blendColour[3] > 1)
	{
		blendColour[3] = 1;
	}
	if (blendColour[3] < 0)
	{
		blendColour[3] = 0;
	}
}

static void VQH_DrawColourBlend()
{
	VQH_SetContentsColor(CM_PointContentsQ1(cl.refdef.vieworg, 0));
	if (GGameType & GAME_Hexen2)
	{
		VH2_CalcPowerupCshift();
	}
	else
	{
		VQ1_CalcPowerupCshift();
	}
	VQH_UpdateCShifts();

	float v_blend[4];
	VQH_CalcBlend(v_blend);

	R_PolyBlend(&cl.refdef, v_blend);
}

/*
==============================================================================

                        VIEW RENDERING

==============================================================================
*/

static void VQH_CalcRefdef()
{
	// bound field of view
	if (scr_fov->value < 10)
	{
		Cvar_Set("fov","10");
	}
	if (scr_fov->value > 170)
	{
		Cvar_Set("fov","170");
	}

	cl.refdef.x = scr_vrect.x * cls.glconfig.vidWidth / viddef.width;
	cl.refdef.y = scr_vrect.y * cls.glconfig.vidHeight / viddef.height;
	cl.refdef.width = scr_vrect.width * cls.glconfig.vidWidth / viddef.width;
	cl.refdef.height = scr_vrect.height * cls.glconfig.vidHeight / viddef.height;
	cl.refdef.fov_x = GGameType & GAME_Hexen2 && !(GGameType & GAME_HexenWorld) ? 90 : scr_fov->value;
	cl.refdef.fov_y = CalcFov(cl.refdef.fov_x, cl.refdef.width, cl.refdef.height);
}

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

static void VQH_CalcRefdef(qwplayer_state_t* qwViewMessage, hwplayer_state_t* hwViewMessage)
{
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

	vec3_t forward, right, up;
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

		for (int i = 0; i < 3; i++)
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

		for (int i = 0; i < 3; i++)
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

static void VQH_CalcIntermissionRefdef()
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

static void VQH_AddViewModel()
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

//	cl.refdef must be set before the first call
static void VQH_RenderScene()
{
	R_ClearScene();

	if (GGameType & GAME_QuakeWorld)
	{
		CLQW_EmitEntities();
	}
	else if (GGameType & GAME_Quake)
	{
		CLQ1_EmitEntities();
	}
	else if (GGameType & GAME_HexenWorld)
	{
		CLHW_EmitEntities();
	}
	else
	{
		CLH2_EmitEntities();
	}

	VQH_AddViewModel();

	if (GGameType & GAME_Hexen2)
	{
		CLH2_UpdateEffects();
	}

	CL_AddDLights();

	CL_AddLightStyles();
	CL_AddParticles();

	cl.refdef.time = cl.serverTime;

	R_RenderScene(&cl.refdef);

	VQH_DrawColourBlend();
}

static void VQH_DropPunchAngle()
{
	cl.qh_punchangle -= cls.frametime / 100;
	if (cl.qh_punchangle < 0)
	{
		cl.qh_punchangle = 0;
	}
}

static void VQH_DrawCrosshair()
{
	if (cl.qh_intermission)
	{
		return;
	}

	if (crosshair->value == 2)
	{
		int x = scr_vrect.x + scr_vrect.width / 2 - 3 + cl_crossx->value;
		int y = scr_vrect.y + scr_vrect.height / 2 - 3 + cl_crossy->value;
		unsigned char* pColor = r_palette[crosshaircolor->integer];
		UI_DrawStretchPicWithColour(x - 4, y - 4, 16, 16, cs_texture, pColor);
	}
	else if (crosshair->value)
	{
		UI_DrawChar(scr_vrect.x + scr_vrect.width / 2 - 4 + cl_crossx->value,
			scr_vrect.y + scr_vrect.height / 2 - 4 + cl_crossy->value, '+');
	}
}

//	The player's clipping box goes from (-16 -16 -24) to (16 16 32) from
// the entity origin, so any view position inside that will be valid
void VQH_RenderView()
{
	if (GGameType & GAME_QuakeWorld)
	{
		cl.qh_simangles[ROLL] = 0;	// FIXME @@@
	}

	if (cls.state != CA_ACTIVE || (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)) && clc.qh_signon != SIGNONS))
	{
		return;
	}

	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld) && !cl.qh_validsequence)
	{
		return;
	}

	VQH_CalcRefdef();

	// don't allow cheats in multiplayer
	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld) || cl.qh_maxclients > 1)
	{
		Cvar_SetCheatState();
		if (GGameType & GAME_QuakeWorld && !String::Atoi(Info_ValueForKey(cl.qh_serverinfo, "watervis")))
		{
			Cvar_Set("r_wateralpha", "1");
		}
	}

	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		VQH_DropPunchAngle();
	}
	if (cl.qh_intermission)
	{	// intermission / finale rendering
		VQH_CalcIntermissionRefdef();
	}
	else if (GGameType & GAME_QuakeWorld)
	{
		qwframe_t* view_frame = &cl.qw_frames[clc.netchan.incomingSequence & UPDATE_MASK_QW];
		qwplayer_state_t* view_message = &view_frame->playerstate[cl.playernum];
		VQH_CalcRefdef(view_message, NULL);
	}
	else if (GGameType & GAME_HexenWorld)
	{
		hwframe_t* view_frame = &cl.hw_frames[clc.netchan.incomingSequence & UPDATE_MASK_HW];
		hwplayer_state_t* view_message = &view_frame->playerstate[cl.playernum];
		VQH_CalcRefdef(NULL, view_message);
	}
	else
	{
		if (!cl.qh_paused /* && (sv.maxclients > 1 || in_keyCatchers == 0) */)
		{
			VQH_CalcRefdef(NULL, NULL);
		}
	}
	VQH_RenderScene();

	VQH_DrawCrosshair();
}

void SCRQH_DrawTurtle()
{
	static int count;

	if (!scr_showturtle->value)
	{
		return;
	}

	if (cls.frametime < 100)
	{
		count = 0;
		return;
	}

	count++;
	if (count < 3)
	{
		return;
	}

	UI_DrawPic(scr_vrect.x, scr_vrect.y, scr_turtle);
}

//	For program optimization
static void VQH_TimeRefresh_f()
{
	int i;
	float start, stop, time;

	if (cls.state != CA_ACTIVE)
	{
		common->Printf("Not connected to a server\n");
		return;
	}

	start = Sys_DoubleTime();
	vec3_t viewangles;
	viewangles[0] = 0;
	viewangles[1] = 0;
	viewangles[2] = 0;
	for (i = 0; i < 128; i++)
	{
		viewangles[1] = i / 128.0 * 360.0;
		AnglesToAxis(viewangles, cl.refdef.viewaxis);
		R_BeginFrame(STEREO_CENTER);
		VQH_RenderScene();
		R_EndFrame(NULL, NULL);
	}

	stop = Sys_DoubleTime();
	time = stop - start;
	common->Printf("%f seconds (%f fps)\n", time, 128 / time);
}

void VQH_Init()
{
	Cmd_AddCommand("v_cshift", V_cshift_f);
	Cmd_AddCommand("bf", V_BonusFlash_f);
	if (GGameType & GAME_Hexen2)
	{
		Cmd_AddCommand("df", V_DarkFlash_f);
		if (!(GGameType & GAME_HexenWorld))
		{
			Cmd_AddCommand("wf", V_WhiteFlash_f);
		}
	}
	Cmd_AddCommand("timerefresh", VQH_TimeRefresh_f);

	v_contentblend = Cvar_Get("v_contentblend", "1", 0);
	gl_cshiftpercent = Cvar_Get("gl_cshiftpercent", "100", 0);

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

	crosshaircolor = Cvar_Get("crosshaircolor", "79", CVAR_ARCHIVE);
	cl_crossx = Cvar_Get("cl_crossx", "0", CVAR_ARCHIVE);
	cl_crossy = Cvar_Get("cl_crossy", "0", CVAR_ARCHIVE);

	scr_fov = Cvar_Get("fov", "90", 0);	// 10 - 170

	scr_showturtle = Cvar_Get("showturtle", "0", 0);
}

void VQH_InitCrosshairTexture()
{
	cs_texture = R_CreateCrosshairImage();
	scr_turtle = R_PicFromWad("turtle");
}
