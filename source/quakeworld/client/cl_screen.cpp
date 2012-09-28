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
Cvar* scr_centertime;
Cvar* scr_showturtle;
Cvar* scr_showpause;
Cvar* scr_printspeed;
Cvar* scr_allowsnap;
Cvar* show_fps;

static Cvar* cl_netgraph;

qboolean scr_initialized;						// ready to draw

image_t* scr_net;
image_t* scr_turtle;

vrect_t scr_vrect;

qboolean scr_drawloading;

void SCR_RSShot_f(void);

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char scr_centerstring[1024];
float scr_centertime_start;				// for slow victory printing
float scr_centertime_off;
int scr_center_lines;
int scr_erase_lines;
int scr_erase_center;

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint(char* str)
{
	String::NCpy(scr_centerstring, str, sizeof(scr_centerstring) - 1);
	scr_centertime_off = scr_centertime->value;
	scr_centertime_start = cl.qh_serverTimeFloat;

// count the number of lines for centering
	scr_center_lines = 1;
	while (*str)
	{
		if (*str == '\n')
		{
			scr_center_lines++;
		}
		str++;
	}
}


void SCR_DrawCenterString(void)
{
	char* start;
	int l;
	int j;
	int x, y;
	int remaining;

// the finale prints the characters one at a time
	if (cl.qh_intermission)
	{
		remaining = scr_printspeed->value * (cl.qh_serverTimeFloat - scr_centertime_start);
	}
	else
	{
		remaining = 9999;
	}

	scr_erase_center = 0;
	start = scr_centerstring;

	if (scr_center_lines <= 4)
	{
		y = viddef.height * 0.35;
	}
	else
	{
		y = 48;
	}

	do
	{
		// scan the width of the line
		for (l = 0; l < 40; l++)
			if (start[l] == '\n' || !start[l])
			{
				break;
			}
		x = (viddef.width - l * 8) / 2;
		for (j = 0; j < l; j++, x += 8)
		{
			UI_DrawChar(x, y, start[j]);
			if (!remaining--)
			{
				return;
			}
		}

		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
		{
			break;
		}
		start++;				// skip the \n
	}
	while (1);
}

void SCR_CheckDrawCenterString(void)
{
	if (scr_center_lines > scr_erase_lines)
	{
		scr_erase_lines = scr_center_lines;
	}

	scr_centertime_off -= host_frametime;

	if (scr_centertime_off <= 0 && !cl.qh_intermission)
	{
		return;
	}
	if (in_keyCatchers != 0)
	{
		return;
	}

	SCR_DrawCenterString();
}

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
	float size;
	int h;
	qboolean full = false;


//========================================

// bound viewsize
	if (scr_viewsize->value < 30)
	{
		Cvar_Set("viewsize","30");
	}
	if (scr_viewsize->value > 120)
	{
		Cvar_Set("viewsize","120");
	}

// bound field of view
	if (scr_fov->value < 10)
	{
		Cvar_Set("fov","10");
	}
	if (scr_fov->value > 170)
	{
		Cvar_Set("fov","170");
	}

// intermission is always full screen
	if (cl.qh_intermission)
	{
		size = 120;
	}
	else
	{
		size = scr_viewsize->value;
	}

	if (size >= 120)
	{
		sbqh_lines = 0;			// no status bar at all
	}
	else if (size >= 110)
	{
		sbqh_lines = 24;			// no inventory
	}
	else
	{
		sbqh_lines = 24 + 16 + 8;
	}

	if (scr_viewsize->value >= 100.0)
	{
		full = true;
		size = 100.0;
	}
	else
	{
		size = scr_viewsize->value;
	}
	if (cl.qh_intermission)
	{
		full = true;
		size = 100.0;
		sbqh_lines = 0;
	}
	size /= 100.0;

	if (!clqh_sbar->value && full)
	{
		h = viddef.height;
	}
	else
	{
		h = viddef.height - sbqh_lines;
	}

	scr_vrect.width = viddef.width * size;
	if (scr_vrect.width < 96)
	{
		size = 96.0 / scr_vrect.width;
		scr_vrect.width = 96;		// min for icons
	}

	scr_vrect.height = viddef.height * size;
	if (clqh_sbar->value || !full)
	{
		if (scr_vrect.height > (int)viddef.height - sbqh_lines)
		{
			scr_vrect.height = viddef.height - sbqh_lines;
		}
	}
	else if (scr_vrect.height > (int)viddef.height)
	{
		scr_vrect.height = viddef.height;
	}
	scr_vrect.x = (viddef.width - scr_vrect.width) / 2;
	if (full)
	{
		scr_vrect.y = 0;
	}
	else
	{
		scr_vrect.y = (h - scr_vrect.height) / 2;
	}

	cl.refdef.x = scr_vrect.x * cls.glconfig.vidWidth / viddef.width;
	cl.refdef.y = scr_vrect.y * cls.glconfig.vidHeight / viddef.height;
	cl.refdef.width = scr_vrect.width * cls.glconfig.vidWidth / viddef.width;
	cl.refdef.height = scr_vrect.height * cls.glconfig.vidHeight / viddef.height;
	cl.refdef.fov_x = scr_fov->value;
	cl.refdef.fov_y = CalcFov(cl.refdef.fov_x, cl.refdef.width, cl.refdef.height);
}


/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void SCR_SizeUp_f(void)
{
	Cvar_SetValue("viewsize",scr_viewsize->value + 10);
}


/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
void SCR_SizeDown_f(void)
{
	Cvar_SetValue("viewsize",scr_viewsize->value - 10);
}

/*
====================
R_TimeRefresh_f

For program optimization
====================
*/
static void R_TimeRefresh_f(void)
{
	int i;
	float start, stop, time;

	start = Sys_DoubleTime();
	vec3_t viewangles;
	viewangles[0] = 0;
	viewangles[1] = 0;
	viewangles[2] = 0;
	for (i = 0; i < 128; i++)
	{
		viewangles[1] = i / 128.0 * 360.0;
		AnglesToAxis(viewangles, cl.refdef.viewaxis);
		R_BeginFrame(STEREO_CENTER);
		V_RenderScene();
		R_EndFrame(NULL, NULL);
	}

	stop = Sys_DoubleTime();
	time = stop - start;
	common->Printf("%f seconds (%f fps)\n", time, 128 / time);
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
	scr_fov = Cvar_Get("fov","90", 0);	// 10 - 170
	scr_centertime = Cvar_Get("scr_centertime","2", 0);
	scr_showturtle = Cvar_Get("showturtle","0", 0);
	scr_showpause = Cvar_Get("showpause","1", 0);
	scr_printspeed = Cvar_Get("scr_printspeed","8", 0);
	scr_allowsnap = Cvar_Get("scr_allowsnap", "1", 0);
	SCR_InitCommon();

//
// register our commands
//
	Cmd_AddCommand("snap",SCR_RSShot_f);
	Cmd_AddCommand("sizeup",SCR_SizeUp_f);
	Cmd_AddCommand("sizedown",SCR_SizeDown_f);
	Cmd_AddCommand("timerefresh", R_TimeRefresh_f);

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
	V_RenderView();

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
		if (crosshair->value)
		{
			Draw_Crosshair();
		}

		SCR_DrawNet();
		SCR_DrawFPS();
		SCR_DrawTurtle();
		SCR_DrawPause();
		SCR_CheckDrawCenterString();
		SbarQ1_Draw();
		Con_DrawConsole();
		UI_DrawMenu();
	}

	V_UpdatePalette();

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
