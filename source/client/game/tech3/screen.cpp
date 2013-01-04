//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "local.h"
#include "../../client_main.h"
#include "../../cinematic/public.h"
#include "../../ui/ui.h"
#include "../../ui/console.h"

static void SCRT3_DrawDemoRecording()
{
	char string[1024];
	int pos;

	if (!clc.demorecording)
	{
		return;
	}
	if (GGameType & GAME_Quake3 && clc.q3_spDemoRecording)
	{
		return;
	}

	pos = FS_FTell(clc.demofile);
	if (GGameType & GAME_ET)
	{
		Cvar_Set("cl_demooffset", va("%d", pos));
	}
	else
	{
		sprintf(string, "RECORDING %s: %ik", clc.q3_demoName, pos / 1024);

		if (GGameType & GAME_WolfMP)
		{
			SCR_DrawStringExt(5, 470, 8, string, g_color_table[7], true);
		}
		else
		{
			SCR_DrawStringExt(320 - String::Length(string) * 4, 20, 8, string, g_color_table[7], true);
		}
	}
}

//	This will be called twice if rendering in stereo mode
void SCRT3_DrawScreenField(stereoFrame_t stereoFrame)
{
	R_BeginFrame(stereoFrame);

	// wide aspect ratio screens need to have the sides cleared
	// unless they are displaying game renderings
	if (!(GGameType & GAME_ET) && cls.state != CA_ACTIVE)
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
	if (!UI_IsFullscreen())
	{
		switch (cls.state)
		{
		default:
			common->FatalError("SCRT3_DrawScreenField: bad cls.state");
			break;
		case CA_CINEMATIC:
			SCR_DrawCinematic();
			break;
		case CA_DISCONNECTED:
			// force menu up
			S_StopAllSounds();
			UI_SetMainMenu();
			break;
		case CA_CONNECTING:
		case CA_CHALLENGING:
		case CA_CONNECTED:
			// connecting clients will only show the connection dialog
			// refresh to update the time
			UIT3_Refresh(cls.realtime);
			UIT3_DrawConnectScreen(false);
			break;
		case CA_LOADING:
		case CA_PRIMED:
			// draw the game information screen and loading progress
			CLT3_CGameRendering(stereoFrame);

			// also draw the connection information, so it doesn't
			// flash away too briefly on local or lan games
			// refresh to update the time
			UIT3_Refresh(cls.realtime);
			UIT3_DrawConnectScreen(true);
			break;
		case CA_ACTIVE:
			CLT3_CGameRendering(stereoFrame);
			SCRT3_DrawDemoRecording();
			break;
		}
	}

	// the menu draws next
	if (in_keyCatchers & KEYCATCH_UI)
	{
		UI_DrawMenu();
	}

	// console draws next
	Con_DrawConsole();

	// debug graph can be drawn on top of anything
	if (cl_debuggraph->integer || cl_timegraph->integer || cl_debugMove->integer)
	{
		SCR_DrawDebugGraph();
	}
}
