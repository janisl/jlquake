// screen.c -- master for refresh, status bar, console, chat, notify, etc

/*
 * $Header: /H2 Mission Pack/gl_screen.c 20    3/18/98 11:34p Jmonroe $
 */

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

qboolean info_up = false;

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

//=============================================================================

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
			clh2_total_loading_size = 0;
			clh2_loading_stage = 0;
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

	if (cl.qh_intermission > 1 || cl.qh_intermission <= 12)
	{
		VQH_RenderView();
	}

	//
	// draw any areas not covered by the refresh
	//
	SCR_TileClear();

	if (scr_draw_loading)
	{
		SbarH2_Draw();
		MQH_FadeScreen();
		SCRH2_DrawLoading();
	}
	else if (cl.qh_intermission >= 1 && cl.qh_intermission <= 12)
	{
		SBH2_IntermissionOverlay();
		if (cl.qh_intermission < 12)
		{
			Con_DrawConsole();
			UI_DrawMenu();
		}
	}
	else
	{
		SCR_DrawNet();
		SCR_DrawFPS();
		SCRQH_DrawTurtle();
		SCRH2_DrawPause();
		SCR_CheckDrawCenterString();
		SbarH2_Draw();
		SCRH2_Plaque_Draw(clh2_plaquemessage,0);
		Con_DrawConsole();
		UI_DrawMenu();

#ifdef MISSIONPACK
		if (info_up)
		{
			SCRH2_UpdateInfoMessage();
			SCRH2_Info_Plaque_Draw(scrh2_infomessage);
		}
#endif
	}

	if (clh2_loading_stage)
	{
		SCRH2_DrawLoading();
	}

	R_EndFrame(NULL, NULL);
}

void VID_Init()
{
	R_BeginRegistration(&cls.glconfig);

	CLH2_InitColourShadeTables();

	Sys_ShowConsole(0, false);

	if (COM_CheckParm("-scale2d"))
	{
		viddef.height = 200;
		viddef.width = 320;
	}
	else
	{
		viddef.height = cls.glconfig.vidHeight;
		viddef.width = cls.glconfig.vidWidth;
	}
}
