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
#include "../client/game/quake_hexen2/view.h"
#include "../client/game/hexen2/local.h"

Cvar* cl_crossx;
Cvar* cl_crossy;

/*
==============================================================================

                        VIEW RENDERING

==============================================================================
*/

/*
================
V_RenderScene

cl.refdef must be set before the first call
================
*/
void V_RenderScene()
{
	R_ClearScene();

	CLQ1_EmitEntities();

	VQH_AddViewModel();

	CL_AddDLights();

	CL_RunLightStyles();

	CL_AddLightStyles();
	CL_AddParticles();

	cl.refdef.time = cl.serverTime;

	R_RenderScene(&cl.refdef);

	VQH_DrawColourBlend();
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
	cl_crossx = Cvar_Get("cl_crossx", "0", 0);
	cl_crossy = Cvar_Get("cl_crossy", "0", 0);

	V_SharedInit();
}
