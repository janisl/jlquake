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
		SCRT3_DrawScreenField(STEREO_LEFT);
		SCRT3_DrawScreenField(STEREO_RIGHT);
	}
	else
	{
		SCRT3_DrawScreenField(STEREO_CENTER);
	}

	if (com_speeds->integer)
	{
		R_EndFrame(&time_frontend, &time_backend);
	}
	else
	{
		R_EndFrame(NULL, NULL);
	}

	recursive = 0;
}
