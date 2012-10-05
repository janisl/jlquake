/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

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
	static int recursive = 0;

	if (!scr_initialized)
	{
		return;				// not initialized yet
	}

	if (++recursive >= 2)
	{
		recursive = 0;
		// Gordon: i'm breaking this again, because we've removed most of our cases but still have one which will not fix easily
		return;
//		Com_Error( ERR_FATAL, "SCR_UpdateScreen: recursively called" );
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
