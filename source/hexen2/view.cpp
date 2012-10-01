// view.c -- player eye positioning

/*
 * $Header: /H2 Mission Pack/VIEW.C 4     3/18/98 11:34p Jmonroe $
 */

#include "quakedef.h"
#include "../client/game/quake_hexen2/view.h"
#include "../client/game/quake/local.h"

/*
================
V_RenderScene

cl.refdef must be set before the first call
================
*/
void V_RenderScene()
{
	R_ClearScene();

	CLH2_EmitEntities();

	VQH_AddViewModel();

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
	if (con_forcedup)
	{
		return;
	}

// don't allow cheats in multiplayer
	if (cl.qh_maxclients > 1)
	{
		Cvar_Set("r_fullbright", "0");
	}

	if (cl.qh_intermission)
	{	// intermission / finale rendering
		VQH_CalcIntermissionRefdef();
	}
	else
	{
		if (!cl.qh_paused /* && (sv.maxclients > 1 || in_keyCatchers == 0) */)
		{
			VQH_CalcRefdef(NULL, NULL);
		}
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

	V_SharedInit();
}
