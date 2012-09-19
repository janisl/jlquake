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
#include "../../client/game/et/ui_public.h"

qboolean scr_initialized;			// ready to draw

Cvar* cl_timegraph;
Cvar* cl_debuggraph;

/*
** SCR_Strlen -- skips color escape codes
*/
static int SCR_Strlen(const char* str)
{
	const char* s = str;
	int count = 0;

	while (*s)
	{
		if (Q_IsColorString(s))
		{
			s += 2;
		}
		else
		{
			count++;
			s++;
		}
	}

	return count;
}

/*
** SCR_GetBigStringWidth
*/
int SCR_GetBigStringWidth(const char* str)
{
	return SCR_Strlen(str) * 16;
}


//===============================================================================

/*
=================
SCR_DrawDemoRecording
=================
*/
void SCR_DrawDemoRecording(void)
{
	if (!clc.demorecording)
	{
		return;
	}

	//bani
	Cvar_Set("cl_demooffset", va("%d", FS_FTell(clc.demofile)));
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
/*	if ( cls.state != CA_ACTIVE ) {
        if ( cls.glconfig.vidWidth * 480 > cls.glconfig.vidHeight * 640 ) {
            R_SetColor( g_color_table[0] );
            R_StretchPic( 0, 0, cls.glconfig.vidWidth, cls.glconfig.vidHeight, 0, 0, 0, 0, cls.whiteShader );
            R_SetColor( NULL );
        }
    }*/

	if (!uivm)
	{
		common->DPrintf("draw screen without UI loaded\n");
		return;
	}

	// if the menu is going to cover the entire screen, we
	// don't need to render anything under it
	if (!UIT3_IsFullscreen())
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
			UIT3_SetActiveMenu(UIMENU_MAIN);
			break;
		case CA_CONNECTING:
		case CA_CHALLENGING:
		case CA_CONNECTED:
			// connecting clients will only show the connection dialog
			// refresh to update the time
			UIT3_Refresh(cls.realtime);
			UIT3_DrawConnectScreen(false);
			break;
//			// Ridah, if the cgame is valid, fall through to there
//			if (!cls.cgameStarted || !com_sv_running->integer) {
//				// connecting clients will only show the connection dialog
//				VM_Call( uivm, UI_DRAW_CONNECT_SCREEN, false );
//				break;
//			}
		case CA_LOADING:
		case CA_PRIMED:
			// draw the game information screen and loading progress
			CLT3_CGameRendering(stereoFrame);

			// also draw the connection information, so it doesn't
			// flash away too briefly on local or lan games
			//if (!com_sv_running->value || Cvar_VariableIntegerValue("sv_cheats"))	// Ridah, don't draw useless text if not in dev mode
			UIT3_Refresh(cls.realtime);
			UIT3_DrawConnectScreen(true);
			break;
		case CA_ACTIVE:
			CLT3_CGameRendering(stereoFrame);
			SCR_DrawDemoRecording();
			break;
		}
	}

	// the menu draws next
	if (in_keyCatchers & KEYCATCH_UI && uivm)
	{
		UIT3_Refresh(cls.realtime);
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
		SCR_DrawScreenField(STEREO_LEFT);
		SCR_DrawScreenField(STEREO_RIGHT);
	}
	else
	{
		SCR_DrawScreenField(STEREO_CENTER);
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
