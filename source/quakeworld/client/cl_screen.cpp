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
#include "../../client/game/quake_hexen2/view.h"

#include <time.h>

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

Cvar* scr_fov;
Cvar* scr_showturtle;
Cvar* scr_showpause;
Cvar* scr_allowsnap;
Cvar* show_fps;

static Cvar* cl_netgraph;

qboolean scr_initialized;						// ready to draw

image_t* scr_net;
image_t* scr_turtle;

qboolean scr_drawloading;

void SCR_RSShot_f(void);

//=============================================================================

/*
=================
SCR_CalcRefdef

Must be called whenever vid changes
Internal use only
=================
*/
static void SCR_CalcRefdef(void)
{
	// bound field of view
	if (scr_fov->value < 10)
	{
		Cvar_Set("fov","10");
	}
	if (scr_fov->value > 170)
	{
		Cvar_Set("fov","170");
	}

	SCR_CalcVrect();

	cl.refdef.x = scr_vrect.x * cls.glconfig.vidWidth / viddef.width;
	cl.refdef.y = scr_vrect.y * cls.glconfig.vidHeight / viddef.height;
	cl.refdef.width = scr_vrect.width * cls.glconfig.vidWidth / viddef.width;
	cl.refdef.height = scr_vrect.height * cls.glconfig.vidHeight / viddef.height;
	cl.refdef.fov_x = scr_fov->value;
	cl.refdef.fov_y = CalcFov(cl.refdef.fov_x, cl.refdef.width, cl.refdef.height);
}

/*
==================
SCR_Init
==================
*/
void SCR_Init(void)
{
	scr_fov = Cvar_Get("fov","90", 0);	// 10 - 170
	scr_showturtle = Cvar_Get("showturtle","0", 0);
	scr_showpause = Cvar_Get("showpause","1", 0);
	scr_allowsnap = Cvar_Get("scr_allowsnap", "1", 0);
	SCR_InitCommon();

	Cmd_AddCommand("snap",SCR_RSShot_f);

	scr_net = R_PicFromWad("net");
	scr_turtle = R_PicFromWad("turtle");

	show_fps = Cvar_Get("show_fps", "0", CVAR_ARCHIVE);			// set for running times
	clqh_sbar     = Cvar_Get("cl_sbar", "0", CVAR_ARCHIVE);

	cl_netgraph = Cvar_Get("cl_netgraph", "0", 0);

	scr_initialized = true;
}

/*
==============
SCR_DrawTurtle
==============
*/
void SCR_DrawTurtle(void)
{
	static int count;

	if (!scr_showturtle->value)
	{
		return;
	}

	if (host_frametime < 0.1)
	{
		count = 0;
		return;
	}

	count++;
	if (count < 3)
	{
		return;
	}

	UI_DrawPic(scr_vrect.x, scr_vrect.y, scr_turtle);
}

/*
==============
SCR_DrawNet
==============
*/
void SCR_DrawNet(void)
{
	if (clc.netchan.outgoingSequence - clc.netchan.incomingAcknowledged < UPDATE_BACKUP_QW - 1)
	{
		return;
	}
	if (clc.demoplaying)
	{
		return;
	}

	UI_DrawPic(scr_vrect.x + 64, scr_vrect.y, scr_net);
}

void SCR_DrawFPS(void)
{
	static double lastframetime;
	double t;
	extern int fps_count;
	static int lastfps;
	int x, y;
	char st[80];

	if (!show_fps->value)
	{
		return;
	}

	t = Sys_DoubleTime();
	if ((t - lastframetime) >= 1.0)
	{
		lastfps = fps_count;
		fps_count = 0;
		lastframetime = t;
	}

	sprintf(st, "%3d FPS", lastfps);
	x = viddef.width - String::Length(st) * 8 - 8;
	y = viddef.height - sbqh_lines - 8;
	UI_DrawString(x, y, st);
}


/*
==============
DrawPause
==============
*/
void SCR_DrawPause(void)
{
	image_t* pic;

	if (!scr_showpause->value)				// turn off for screenshots
	{
		return;
	}

	if (!cl.qh_paused)
	{
		return;
	}

	pic = R_CachePic("gfx/pause.lmp");
	UI_DrawPic((viddef.width - R_GetImageWidth(pic)) / 2,
		(viddef.height - 48 - R_GetImageHeight(pic)) / 2, pic);
}



/*
==============
SCR_DrawLoading
==============
*/
void SCR_DrawLoading(void)
{
	image_t* pic;

	if (!scr_drawloading)
	{
		return;
	}

	pic = R_CachePic("gfx/loading.lmp");
	UI_DrawPic((viddef.width - R_GetImageWidth(pic)) / 2,
		(viddef.height - 48 - R_GetImageHeight(pic)) / 2, pic);
}



//=============================================================================

/*
==================
SCR_RSShot_f
==================
*/
void SCR_RSShot_f(void)
{
	if (CL_IsUploading())
	{
		return;	// already one pending
	}

	if (cls.state < CA_LOADING)
	{
		return;	// gotta be connected
	}

	if (!scr_allowsnap->integer)
	{
		CL_AddReliableCommand("snap\n");
		common->Printf("Refusing remote screen shot request.\n");
		return;
	}

	common->Printf("Remote screen shot requested.\n");

	time_t now;
	time(&now);

	Array<byte> buffer;
	R_CaptureRemoteScreenShot(ctime(&now), cls.servername, clqh_name->string, buffer);
	CL_StartUpload(buffer.Ptr(), buffer.Num());
}




//=============================================================================

void SCR_TileClear(void)
{
	if (scr_vrect.x > 0)
	{
		// left
		UI_TileClear(0, 0, scr_vrect.x, viddef.height - sbqh_lines, draw_backtile);
		// right
		UI_TileClear(scr_vrect.x + scr_vrect.width, 0,
			viddef.width - scr_vrect.x + scr_vrect.width,
			viddef.height - sbqh_lines, draw_backtile);
	}
	if (scr_vrect.y > 0)
	{
		// top
		UI_TileClear(scr_vrect.x, 0,
			scr_vrect.x + scr_vrect.width,
			scr_vrect.y, draw_backtile);
		// bottom
		UI_TileClear(scr_vrect.x,
			scr_vrect.y + scr_vrect.height,
			scr_vrect.width,
			viddef.height - sbqh_lines -
			(scr_vrect.height + scr_vrect.y), draw_backtile);
	}
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

	if (!scr_initialized || !cls.glconfig.vidWidth)
	{
		return;							// not initialized yet

	}
	R_BeginFrame(STEREO_CENTER);

	//
	// determine size of refresh window
	//
	SCR_CalcRefdef();

//
// do 3D refresh drawing, and then update the screen
//
	VQH_RenderView();

	//
	// draw any areas not covered by the refresh
	//
	SCR_TileClear();

	if (cl_netgraph->value)
	{
		R_NetGraph();
	}

	if (scr_drawloading)
	{
		SCR_DrawLoading();
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
		SCR_DrawTurtle();
		SCR_DrawPause();
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
