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

// screen.c -- master for refresh, status bar, console, chat, notify, etc

#include "quakedef.h"
#include "../client/game/quake_hexen2/view.h"

/*

background clear
rendering
turtle/net/ram icons
sbar
centerprint / slow centerprint
notify lines
intermission / finale overlay
loading plaque
console
menu

required background clears
required update regions


syncronous draw mode or async
One off screen buffer, with updates either copied or xblited
Need to double buffer?


async draw will require the refresh area to be cleared, because it will be
xblited, but sync draw can just ignore it.

sync
draw

CenterPrint ()
SlowPrint ()
Screen_Update ();
common->Printf ();

net
turn off messages option

the refresh is allways rendered, unless the console is full screen


console is:
    notify lines
    half
    full


*/

qboolean scr_initialized;			// ready to draw

qboolean con_forcedup;			// because no entities to refresh

/*
==================
SCR_Init
==================
*/
void SCR_Init(void)
{
	SCR_InitCommon();

	scr_initialized = true;
}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.

WARNING: be very careful calling this from elsewhere, because the refresh
needs almost the entire 256k of stack space!
==================
*/
void SCR_UpdateScreen(void)
{
	if (cls.disable_screen)
	{
		if (realtime * 1000 - cls.disable_screen > 60000)
		{
			cls.disable_screen = 0;
			common->Printf("load failed.\n");
		}
		else
		{
			return;
		}
	}

	if (!scr_initialized)
	{
		return;				// not initialized yet

	}
	R_BeginFrame(STEREO_CENTER);

	//
	// determine size of refresh window
	//
	SCR_CalcVrect();

//
// do 3D refresh drawing, and then update the screen
//
	con_forcedup = cls.state != CA_ACTIVE || clc.qh_signon != SIGNONS;

	VQH_RenderView();

	//
	// draw any areas not covered by the refresh
	//
	SCR_TileClear();

	if (scr_draw_loading)
	{
		SCRQ1_DrawLoading();
		SbarQ1_Draw();
	}
	else if (cl.qh_intermission == 1 && in_keyCatchers == 0)
	{
		SbarQ1_IntermissionOverlay();
	}
	else if (cl.qh_intermission == 2 && in_keyCatchers == 0)
	{
		SbarQ1_FinaleOverlay();
		SCR_CheckDrawCenterString();
	}
	else
	{
		SCR_DrawNet();
		SCR_DrawFPS();
		SCRQH_DrawTurtle();
		SCRQ1_DrawPause();
		SCR_CheckDrawCenterString();
		SbarQ1_Draw();
		Con_DrawConsole();
		UI_DrawMenu();
	}

	R_EndFrame(NULL, NULL);
}

void VID_Init()
{
	R_BeginRegistration(&cls.glconfig);

	Sys_ShowConsole(0, false);

	int i;
	if ((i = COM_CheckParm("-conwidth")) != 0)
	{
		viddef.width = String::Atoi(COM_Argv(i + 1));
	}
	else
	{
		viddef.width = 640;
	}

	viddef.width &= 0xfff8;	// make it a multiple of eight

	if (viddef.width < 320)
	{
		viddef.width = 320;
	}

	// pick a conheight that matches with correct aspect
	viddef.height = viddef.width / cls.glconfig.windowAspect;

	if ((i = COM_CheckParm("-conheight")) != 0)
	{
		viddef.height = String::Atoi(COM_Argv(i + 1));
	}
	if (viddef.height < 200)
	{
		viddef.height = 200;
	}

	if (viddef.height > cls.glconfig.vidHeight)
	{
		viddef.height = cls.glconfig.vidHeight;
	}
	if (viddef.width > cls.glconfig.vidWidth)
	{
		viddef.width = cls.glconfig.vidWidth;
	}
}
