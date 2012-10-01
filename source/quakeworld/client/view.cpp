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
#include "../../client/game/quake_hexen2/view.h"
#include "../../client/game/hexen2/local.h"

Cvar* crosshaircolor;

Cvar* cl_crossx;
Cvar* cl_crossy;

static qwplayer_state_t* view_message;

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

	CLQW_EmitEntities();

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
	cl.qh_simangles[ROLL] = 0;	// FIXME @@@

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
	Cvar_Set("r_lightmap", "0");
	if (!String::Atoi(Info_ValueForKey(cl.qh_serverinfo, "watervis")))
	{
		Cvar_Set("r_wateralpha", "1");
	}

	qwframe_t* view_frame = &cl.qw_frames[clc.netchan.incomingSequence & UPDATE_MASK_QW];
	view_message = &view_frame->playerstate[cl.playernum];

	DropPunchAngle();
	if (cl.qh_intermission)
	{	// intermission / finale rendering
		VQH_CalcIntermissionRefdef();
	}
	else
	{
		VQH_CalcRefdef(view_message, NULL);
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
	crosshaircolor = Cvar_Get("crosshaircolor", "79", CVAR_ARCHIVE);
	crosshair = Cvar_Get("crosshair", "0", CVAR_ARCHIVE);
	cl_crossx = Cvar_Get("cl_crossx", "0", CVAR_ARCHIVE);
	cl_crossy = Cvar_Get("cl_crossy", "0", CVAR_ARCHIVE);

	V_SharedInit();
}
