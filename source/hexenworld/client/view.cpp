// view.c -- player eye positioning

#include "quakedef.h"
#include "../../client/game/quake_hexen2/view.h"
#include "../../client/game/quake/local.h"

Cvar* cl_crossx;
Cvar* cl_crossy;

static hwplayer_state_t* view_message;

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

	cl.refdef.time = cl.serverTime;

	R_RenderScene(&cl.refdef);

	VQH_DrawColourBlend();
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
	crosshair = Cvar_Get("crosshair", "0", CVAR_ARCHIVE);
	cl_crossx = Cvar_Get("cl_crossx", "0", CVAR_ARCHIVE);
	cl_crossy = Cvar_Get("cl_crossy", "0", CVAR_ARCHIVE);

	V_SharedInit();
}
