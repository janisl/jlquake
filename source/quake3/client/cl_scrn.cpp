/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// cl_scrn.c -- master for refresh, status bar, console, chat, notify, etc

#include "client.h"

qboolean scr_initialized;			// ready to draw

Cvar* cl_timegraph;
Cvar* cl_debuggraph;

/*
=================
SCR_DrawDemoRecording
=================
*/
void SCR_DrawDemoRecording(void)
{
	char string[1024];
	int pos;

	if (!clc.demorecording)
	{
		return;
	}
	if (clc.q3_spDemoRecording)
	{
		return;
	}

	pos = FS_FTell(clc.demofile);
	sprintf(string, "RECORDING %s: %ik", clc.q3_demoName, pos / 1024);

	SCR_DrawStringExt(320 - String::Length(string) * 4, 20, 8, string, g_color_table[7], true);
}

//=============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init(void)
{
	cl_timegraph = Cvar_Get("timegraph", "0", CVAR_CHEAT);
	cl_debuggraph = Cvar_Get("debuggraph", "0", CVAR_CHEAT);
	SCR_InitCommon();

	scr_initialized = true;
}


//=======================================================

/*
==================
SCR_DrawScreenField

This will be called twice if rendering in stereo mode
==================
*/
void SCR_DrawScreenField(stereoFrame_t stereoFrame)
{
	R_BeginFrame(stereoFrame);

	// wide aspect ratio screens need to have the sides cleared
	// unless they are displaying game renderings
	if (cls.state != CA_ACTIVE)
	{
		if (cls.glconfig.vidWidth * 480 > cls.glconfig.vidHeight * 640)
		{
			R_SetColor(g_color_table[0]);
			R_StretchPic(0, 0, cls.glconfig.vidWidth, cls.glconfig.vidHeight, 0, 0, 0, 0, cls.whiteShader);
			R_SetColor(NULL);
		}
	}

	if (!uivm)
	{
		common->DPrintf("draw screen without UI loaded\n");
		return;
	}

	// if the menu is going to cover the entire screen, we
	// don't need to render anything under it
	if (!VM_Call(uivm, UI_IS_FULLSCREEN))
	{
		switch (cls.state)
		{
		default:
			common->FatalError("SCR_DrawScreenField: bad cls.state");
			break;
		case CA_CINEMATIC:
			SCR_DrawCinematic();
			break;
		case CA_DISCONNECTED:
			// force menu up
			S_StopAllSounds();
			VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN);
			break;
		case CA_CONNECTING:
		case CA_CHALLENGING:
		case CA_CONNECTED:
			// connecting clients will only show the connection dialog
			// refresh to update the time
			VM_Call(uivm, UI_REFRESH, cls.realtime);
			VM_Call(uivm, UI_DRAW_CONNECT_SCREEN, false);
			break;
		case CA_LOADING:
		case CA_PRIMED:
			// draw the game information screen and loading progress
			CL_CGameRendering(stereoFrame);

			// also draw the connection information, so it doesn't
			// flash away too briefly on local or lan games
			// refresh to update the time
			VM_Call(uivm, UI_REFRESH, cls.realtime);
			VM_Call(uivm, UI_DRAW_CONNECT_SCREEN, true);
			break;
		case CA_ACTIVE:
			CL_CGameRendering(stereoFrame);
			SCR_DrawDemoRecording();
			break;
		}
	}

	// the menu draws next
	if (in_keyCatchers & KEYCATCH_UI && uivm)
	{
		VM_Call(uivm, UI_REFRESH, cls.realtime);
	}

	// console draws next
	Con_DrawConsole();

	// debug graph can be drawn on top of anything
	if (cl_debuggraph->integer || cl_timegraph->integer || cl_debugMove->integer)
	{
		SCR_DrawDebugGraph();
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
	static int recursive;

	if (!scr_initialized)
	{
		return;				// not initialized yet
	}

	if (++recursive > 2)
	{
		common->FatalError("SCR_UpdateScreen: recursively called");
	}
	recursive = 1;

	// if running in stereo, we need to draw the frame twice
	if (cls.glconfig.stereoEnabled)
	{
		SCR_DrawScreenField(STEREO_LEFT);
		SCR_DrawScreenField(STEREO_RIGHT);
	}
	else
	{
		SCR_DrawScreenField(STEREO_CENTER);
	}

	if (t3com_speeds->integer)
	{
		R_EndFrame(&time_frontend, &time_backend);
	}
	else
	{
		R_EndFrame(NULL, NULL);
	}

	recursive = 0;
}
