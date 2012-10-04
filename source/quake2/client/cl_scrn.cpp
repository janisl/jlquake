/*
Copyright (C) 1997-2001 Id Software, Inc.

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
// cl_scrn.c -- master for refresh, status bar, console, chat, notify, etc

/*

  full screen console
  put up loading plaque
  blanked background with loading plaque
  blanked background with menu
  cinematics
  full screen image for quit and victory

  end of unit intermissions

  */

#include "client.h"

qboolean scr_initialized;			// ready to draw

void SCR_Loading_f(void);


/*
===============================================================================

BAR GRAPHS

===============================================================================
*/

/*
=================
SCR_Sky_f

Set a specific sky and rotation speed
=================
*/
void SCR_Sky_f(void)
{
	float rotate;
	vec3_t axis;

	if (Cmd_Argc() < 2)
	{
		common->Printf("Usage: sky <basename> <rotate> <axis x y z>\n");
		return;
	}
	if (Cmd_Argc() > 2)
	{
		rotate = String::Atof(Cmd_Argv(2));
	}
	else
	{
		rotate = 0;
	}
	if (Cmd_Argc() == 6)
	{
		axis[0] = String::Atof(Cmd_Argv(3));
		axis[1] = String::Atof(Cmd_Argv(4));
		axis[2] = String::Atof(Cmd_Argv(5));
	}
	else
	{
		axis[0] = 0;
		axis[1] = 0;
		axis[2] = 1;
	}

	R_SetSky(Cmd_Argv(1), rotate, axis);
}

//============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init(void)
{
	scr_netgraph = Cvar_Get("netgraph", "0", 0);
	scrq2_timegraph = Cvar_Get("timegraph", "0", 0);
	scrq2_debuggraph = Cvar_Get("debuggraph", "0", 0);
	SCR_InitCommon();

	Cmd_AddCommand("loading",SCR_Loading_f);
	Cmd_AddCommand("sky",SCR_Sky_f);

	scr_initialized = true;
}

//=============================================================================

/*
================
SCR_Loading_f
================
*/
void SCR_Loading_f(void)
{
	SCRQ2_BeginLoadingPlaque(false);
}

#define ICON_WIDTH  24
#define ICON_HEIGHT 24
#define ICON_SPACE  8

//=======================================================

static void SCR_DrawScreen(stereoFrame_t stereoFrame, float separation)
{
	R_BeginFrame(stereoFrame);

	if (scr_draw_loading == 2)
	{	//  loading plaque over black screen
		int w, h;

		UI_Fill(0, 0, viddef.width, viddef.height, 0, 0, 0, 1);
		scr_draw_loading = false;
		R_GetPicSize(&w, &h, "loading");
		UI_DrawNamedPic((viddef.width - w) / 2, (viddef.height - h) / 2, "loading");
	}
	// if a cinematic is supposed to be running, handle menus
	// and console specially
	else if (SCR_DrawCinematic())
	{
		if (in_keyCatchers & KEYCATCH_UI)
		{
			UI_DrawMenu();
		}
		else if (in_keyCatchers & KEYCATCH_CONSOLE)
		{
			Con_DrawConsole();
		}
	}
	else
	{
		// do 3D refresh drawing, and then update the screen
		SCR_CalcVrect();

		// clear any dirty part of the background
		SCR_TileClear();

		VQ2_RenderView(separation);

		SCRQ2_DrawHud();

		if (scrq2_timegraph->value)
		{
			SCR_DebugGraph(cls.q2_frametimeFloat * 300, 0);
		}

		if (scrq2_debuggraph->value || scrq2_timegraph->value || scr_netgraph->value)
		{
			SCR_DrawDebugGraph();
		}

		SCRQ2_DrawPause();

		Con_DrawConsole();

		UI_DrawMenu();

		SCRQ2_DrawLoading();
	}
	SCR_DrawFPS();
}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void SCR_UpdateScreen(void)
{
	// if the screen is disabled (loading plaque is up, or vid mode changing)
	// do nothing at all
	if (cls.disable_screen)
	{
		if (Sys_Milliseconds_() - cls.disable_screen > 120000)
		{
			cls.disable_screen = 0;
			common->Printf("Loading plaque timed out.\n");
		}
		return;
	}

	if (!scr_initialized)
	{
		return;				// not initialized yet

	}
	/*
	** range check cl_camera_separation so we don't inadvertently fry someone's
	** brain
	*/
	if (cl_stereo_separation->value > 1.0)
	{
		Cvar_SetValueLatched("cl_stereo_separation", 1.0);
	}
	else if (cl_stereo_separation->value < 0)
	{
		Cvar_SetValueLatched("cl_stereo_separation", 0.0);
	}

	if (cls.glconfig.stereoEnabled)
	{
		SCR_DrawScreen(STEREO_LEFT, -cl_stereo_separation->value / 2);
		SCR_DrawScreen(STEREO_RIGHT, cl_stereo_separation->value / 2);
	}
	else
	{
		SCR_DrawScreen(STEREO_CENTER, 0);
	}

	R_EndFrame(NULL, NULL);

	if (cls.state == CA_ACTIVE && cl.q2_refresh_prepped && cl.q2_frame.valid)
	{
		CL_UpdateParticles(800);
	}
}

/*
===============
Draw_InitLocal
===============
*/
void Draw_InitLocal(void)
{
	char_texture = R_LoadQuake2FontImage("pics/conchars.pcx");
}
