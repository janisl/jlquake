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

int scr_draw_loading;

Cvar* scr_showturtle;
Cvar* scr_showpause;

Cvar* scr_netgraph;
Cvar* scr_timegraph;
Cvar* scr_debuggraph;

void SCR_TimeRefresh_f(void);
void SCR_Loading_f(void);


/*
===============================================================================

BAR GRAPHS

===============================================================================
*/

/*
==============
CL_AddNetgraph

A new packet was just parsed
==============
*/
void CL_AddNetgraph(void)
{
	int i;
	int in;
	int ping;

	// if using the debuggraph for something else, don't
	// add the net lines
	if (scr_debuggraph->value || scr_timegraph->value)
	{
		return;
	}

	for (i = 0; i < clc.netchan.dropped; i++)
		SCR_DebugGraph(30, 0x40);

	for (i = 0; i < cl.q2_surpressCount; i++)
		SCR_DebugGraph(30, 0xdf);

	// see what the latency was on this packet
	in = clc.netchan.incomingAcknowledged & (CMD_BACKUP_Q2 - 1);
	ping = cls.realtime - cl.q2_cmd_time[in];
	ping /= 30;
	if (ping > 30)
	{
		ping = 30;
	}
	SCR_DebugGraph(ping, 0xd0);
}

//=============================================================================

/*
=================
SCR_CalcVrect

Sets scr_vrect, the coordinates of the rendered window
=================
*/
static void SCR_CalcVrect(void)
{
	int size;

	// bound viewsize
	if (scr_viewsize->value < 40)
	{
		Cvar_SetLatched("viewsize","40");
	}
	if (scr_viewsize->value > 100)
	{
		Cvar_SetLatched("viewsize","100");
	}

	size = scr_viewsize->value;

	scr_vrect.width = viddef.width * size / 100;
	scr_vrect.width &= ~7;

	scr_vrect.height = viddef.height * size / 100;
	scr_vrect.height &= ~1;

	scr_vrect.x = (viddef.width - scr_vrect.width) / 2;
	scr_vrect.y = (viddef.height - scr_vrect.height) / 2;
}


/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void SCR_SizeUp_f(void)
{
	Cvar_SetValueLatched("viewsize",scr_viewsize->value + 10);
}


/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
void SCR_SizeDown_f(void)
{
	Cvar_SetValueLatched("viewsize",scr_viewsize->value - 10);
}

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
	scr_viewsize = Cvar_Get("viewsize", "100", CVAR_ARCHIVE);
	scr_showturtle = Cvar_Get("scr_showturtle", "0", 0);
	scr_showpause = Cvar_Get("scr_showpause", "1", 0);
	scr_netgraph = Cvar_Get("netgraph", "0", 0);
	scr_timegraph = Cvar_Get("timegraph", "0", 0);
	scr_debuggraph = Cvar_Get("debuggraph", "0", 0);
	SCR_InitCommon();

//
// register our commands
//
	Cmd_AddCommand("timerefresh",SCR_TimeRefresh_f);
	Cmd_AddCommand("loading",SCR_Loading_f);
	Cmd_AddCommand("sizeup",SCR_SizeUp_f);
	Cmd_AddCommand("sizedown",SCR_SizeDown_f);
	Cmd_AddCommand("sky",SCR_Sky_f);

	scr_initialized = true;
}

/*
==============
SCR_DrawPause
==============
*/
void SCR_DrawPause(void)
{
	int w, h;

	if (!scr_showpause->value)		// turn off for screenshots
	{
		return;
	}

	if (!cl_paused->value)
	{
		return;
	}

	R_GetPicSize(&w, &h, "pause");
	UI_DrawNamedPic((viddef.width - w) / 2, viddef.height / 2 + 8, "pause");
}

/*
==============
SCR_DrawLoading
==============
*/
void SCR_DrawLoading(void)
{
	int w, h;

	if (!scr_draw_loading)
	{
		return;
	}

	scr_draw_loading = false;
	R_GetPicSize(&w, &h, "loading");
	UI_DrawNamedPic((viddef.width - w) / 2, (viddef.height - h) / 2, "loading");
}

//=============================================================================

/*
================
SCRQ2_BeginLoadingPlaque
================
*/
void SCRQ2_BeginLoadingPlaque(bool Clear)
{
	S_StopAllSounds();
	cl.q2_sound_prepped = false;		// don't play ambients
	CDAudio_Stop();
	if (cls.disable_screen)
	{
		return;
	}
	if (com_developer->value)
	{
		return;
	}
	if (cls.state == CA_DISCONNECTED)
	{
		return;	// if at console, don't bring up the plaque
	}
	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		return;
	}
	scr_draw_loading = Clear ? 2 : 1;
	SCR_UpdateScreen();
	cls.disable_screen = Sys_Milliseconds_();
	cls.q2_disable_servercount = cl.servercount;
}

/*
================
SCR_Loading_f
================
*/
void SCR_Loading_f(void)
{
	SCRQ2_BeginLoadingPlaque();
}

void SCR_TimeRefresh_f(void)
{
	int i;
	int start, stop;
	float time;

	if (cls.state != CA_ACTIVE)
	{
		return;
	}

	start = Sys_Milliseconds_();

	vec3_t viewangles;
	viewangles[0] = 0;
	viewangles[1] = 0;
	viewangles[2] = 0;
	if (Cmd_Argc() == 2)
	{	// run without page flipping
		R_BeginFrame(STEREO_CENTER);
		for (i = 0; i < 128; i++)
		{
			viewangles[1] = i / 128.0 * 360.0;
			AnglesToAxis(viewangles, cl.refdef.viewaxis);
			R_RenderScene(&cl.refdef);
		}
		R_EndFrame(NULL, NULL);
	}
	else
	{
		for (i = 0; i < 128; i++)
		{
			viewangles[1] = i / 128.0 * 360.0;
			AnglesToAxis(viewangles, cl.refdef.viewaxis);

			R_BeginFrame(STEREO_CENTER);
			R_RenderScene(&cl.refdef);
			R_EndFrame(NULL, NULL);
		}
	}

	stop = Sys_Milliseconds_();
	time = (stop - start) / 1000.0;
	common->Printf("%f seconds (%f fps)\n", time, 128 / time);
}

/*
==============
SCR_TileClear

Clear any parts of the tiled background that were drawn on last frame
==============
*/
void SCR_TileClear(void)
{
	if (con.displayFrac == 1.0)
	{
		return;		// full screen console
	}
	if (scr_viewsize->value == 100)
	{
		return;		// full screen rendering

	}
	int top = scr_vrect.y;
	int bottom = top + scr_vrect.height;
	int left = scr_vrect.x;
	int right = left + scr_vrect.width;

	if (top > 0)
	{
		// clear above view screen
		UI_NamedTileClear(0, 0, cls.glconfig.vidWidth, top, "backtile");
	}
	if (cls.glconfig.vidHeight > bottom)
	{
		// clear below view screen
		UI_NamedTileClear(0, bottom, cls.glconfig.vidWidth, cls.glconfig.vidHeight - bottom, "backtile");
	}
	if (0 < left)
	{
		// clear left of view screen
		UI_NamedTileClear(0, top, left, bottom - top, "backtile");
	}
	if (cls.glconfig.vidWidth > right)
	{
		// clear left of view screen
		UI_NamedTileClear(right, top, cls.glconfig.vidWidth - right, bottom - top, "backtile");
	}
}


//===============================================================


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

		if (scr_timegraph->value)
		{
			SCR_DebugGraph(cls.q2_frametimeFloat * 300, 0);
		}

		if (scr_debuggraph->value || scr_timegraph->value || scr_netgraph->value)
		{
			SCR_DrawDebugGraph();
		}

		SCR_DrawPause();

		Con_DrawConsole();

		UI_DrawMenu();

		SCR_DrawLoading();
	}
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
