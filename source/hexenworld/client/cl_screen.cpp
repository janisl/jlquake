
// screen.c -- master for refresh, status bar, console, chat, notify, etc

#include "quakedef.h"

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
Cvar* show_fps;

static Cvar* cl_netgraph;

qboolean scr_initialized;						// ready to draw

image_t* scr_net;
image_t* scr_turtle;

vrect_t scr_vrect;

qboolean scr_drawloading;

const char* plaquemessage = NULL;	// Pointer to current plaque message

void Plaque_Draw(const char* message, qboolean AlwaysDraw);

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char scr_centerstring[1024];
float scr_centertime_start;			// for slow victory printing
float scr_centertime_off;
int scr_center_lines;
int scr_erase_lines;
int scr_erase_center;

static int lines;
static int StartC[20],EndC[20];

void FindTextBreaks(const char* message, int Width)
{
	int length,pos,start,lastspace,oldlast;

	length = String::Length(message);
	lines = pos = start = 0;
	lastspace = -1;

	while (1)
	{
		if (pos - start >= Width || message[pos] == '@' ||
			message[pos] == 0)
		{
			oldlast = lastspace;
			if (message[pos] == '@' || lastspace == -1 || message[pos] == 0)
			{
				lastspace = pos;
			}

			StartC[lines] = start;
			EndC[lines] = lastspace;
			lines++;

			if (message[pos] == '@')
			{
				start = pos + 1;
			}
			else if (oldlast == -1)
			{
				start = lastspace;
			}
			else
			{
				start = lastspace + 1;
			}

			lastspace = -1;
		}

		if (message[pos] == 32)
		{
			lastspace = pos;
		}
		else if (message[pos] == 0)
		{
			break;
		}

		pos++;
	}
}

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

	FindTextBreaks(scr_centerstring, 38);
	scr_center_lines = lines;
}

void M_Print2(int cx, int cy, const char* str)
{
	UI_DrawString(cx + ((viddef.width - 320) >> 1), cy + ((viddef.height - 200) >> 1), str, 256);
}

void SCR_DrawCenterString(void)
{
	int i;
	int bx, by;
	int remaining;
	char temp[80];

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

	FindTextBreaks(scr_centerstring, 38);

	by = ((25 - lines) * 8) / 2;
	for (i = 0; i < lines; i++,by += 8)
	{
		String::NCpy(temp,&scr_centerstring[StartC[i]],EndC[i] - StartC[i]);
		temp[EndC[i] - StartC[i]] = 0;
		bx = ((40 - String::Length(temp)) * 8) / 2;
		M_Print2(bx, by, temp);
	}
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


// force the status bar to redraw
	SbarH2_InvChanged();

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
		size = 110;
	}
	else
	{
		size = scr_viewsize->value;
	}

//	if (size >= 120)
//		sbqh_lines = 0;		// no status bar at all
//	else if (size >= 110)
//		sbqh_lines = 24;		// no inventory
//	else
//		sbqh_lines = 24+16+8;

	if (size >= 110)
	{	// No status bar
		sbqh_lines = 0;
	}
	else
	{
		sbqh_lines = 36;
	}

	size = scr_viewsize->value > 100 ? 100 : scr_viewsize->value;
	if (cl.qh_intermission)
	{
		size = 100;

		sbqh_lines = 0;
	}
	size /= 100;

	h = viddef.height - sbqh_lines;
	scr_vrect.width = viddef.width * size;
	if (scr_vrect.width < 96)
	{
		size = 96.0 / viddef.width;
		scr_vrect.width = 96;	// min for icons
	}

	scr_vrect.height = viddef.height * size;
	if (scr_vrect.height > (int)viddef.height - sbqh_lines)
	{
		scr_vrect.height = viddef.height - sbqh_lines;
	}

	scr_vrect.x = (viddef.width - scr_vrect.width) / 2;
	scr_vrect.y = (h - scr_vrect.height) / 2;

	cl.refdef.x = scr_vrect.x * cls.glconfig.vidWidth / viddef.width;
	cl.refdef.y = scr_vrect.y * cls.glconfig.vidHeight / viddef.height;
	cl.refdef.width = scr_vrect.width * cls.glconfig.vidWidth / viddef.width;
	cl.refdef.height = scr_vrect.height * cls.glconfig.vidHeight / viddef.height;
	cl.refdef.fov_x = scr_fov->value;
	cl.refdef.fov_y = 2 * atan((float)cl.refdef.height / cl.refdef.width) * (cl.refdef.fov_x * 2) / M_PI;
}

/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void SCR_SizeUp_f(void)
{
	if (scr_viewsize->value < 110)
	{
		Cvar_SetValue("viewsize",scr_viewsize->value + 10);
		SbarH2_ViewSizeChanged();
	}
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
	SbarH2_ViewSizeChanged();
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

	if (cls.state != CA_ACTIVE)
	{
		common->Printf("Not connected to a server\n");
		return;
	}

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
	scr_viewsize = Cvar_Get("viewsize","100", CVAR_ARCHIVE);
	scr_fov = Cvar_Get("fov", "90", 0);	// 10 - 170
	scr_centertime = Cvar_Get("scr_centertime", "4", 0);
	scr_showturtle = Cvar_Get("showturtle", "0", 0);
	scr_showpause = Cvar_Get("showpause", "1", 0);
	scr_printspeed = Cvar_Get("scr_printspeed", "8", 0);
	SCR_InitCommon();

//
// register our commands
//
	Cmd_AddCommand("sizeup",SCR_SizeUp_f);
	Cmd_AddCommand("sizedown",SCR_SizeDown_f);
	Cmd_AddCommand("timerefresh", R_TimeRefresh_f);

	scr_net = R_PicFromWad("net");
	scr_turtle = R_PicFromWad("turtle");

	show_fps = Cvar_Get("show_fps", "0", CVAR_ARCHIVE);			// set for running times

	cl_netgraph = Cvar_Get("cl_netgraph","0", 0);

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
	if (clc.netchan.outgoingSequence - clc.netchan.incomingAcknowledged < UPDATE_BACKUP_HW - 1)
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
	float delta;
	static qboolean newdraw = false;
	int finaly;
	static float LogoPercent,LogoTargetPercent;

	if (!scr_showpause->value)		// turn off for screenshots
	{
		return;
	}

	if (!cl.qh_paused)
	{
		newdraw = false;
		return;
	}

	if (!newdraw)
	{
		newdraw = true;
		LogoTargetPercent = 1;
		LogoPercent = 0;
	}

	pic = R_CachePic("gfx/menu/paused.lmp");
//	UI_DrawPic ( (viddef.width - pic->width)/2,
//		(viddef.height - 48 - pic->height)/2, pic);

	if (LogoPercent < LogoTargetPercent)
	{
		delta = ((LogoTargetPercent - LogoPercent) / .5) * host_frametime;
		if (delta < 0.004)
		{
			delta = 0.004;
		}
		LogoPercent += delta;
		if (LogoPercent > LogoTargetPercent)
		{
			LogoPercent = LogoTargetPercent;
		}
	}

	finaly = ((float)R_GetImageHeight(pic) * LogoPercent) - R_GetImageHeight(pic);
	UI_DrawPic((viddef.width - R_GetImageWidth(pic)) / 2, finaly, pic);
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

const char* scr_notifystring;
qboolean scr_drawdialog;

void SCR_DrawNotifyString(void)
{
	Plaque_Draw(scr_notifystring,1);
}

/*
==================
SCR_ModalMessage

Displays a text string in the center of the screen and waits for a Y or N
keypress.
==================
*/
int SCR_ModalMessage(const char* text)
{
	scr_notifystring = text;

// draw a fresh screen
	scr_drawdialog = true;
	SCR_UpdateScreen();
	scr_drawdialog = false;

	S_ClearSoundBuffer(true);				// so dma doesn't loop current sound

	do
	{
		key_count = -1;			// wait for a key down and up
		Sys_SendKeyEvents();
		IN_ProcessEvents();
	}
	while (key_lastpress != 'y' && key_lastpress != 'n' && key_lastpress != K_ESCAPE);

	SCR_UpdateScreen();

	return key_lastpress == 'y';
}


//=============================================================================

void SCR_TileClear(void)
{
	if (viddef.width > 320)
	{
		if (scr_vrect.x > 0)
		{
			// left
			UI_TileClear(0, 0, scr_vrect.x, viddef.height, draw_backtile);
			// right
			UI_TileClear(scr_vrect.x + scr_vrect.width, 0,
				viddef.width - scr_vrect.x + scr_vrect.width,
				viddef.height, draw_backtile);
		}
//		if (scr_vrect.y > 0)
		{
			// top
			UI_TileClear(scr_vrect.x, 0,
				scr_vrect.x + scr_vrect.width, scr_vrect.y, draw_backtile);
			// bottom
			UI_TileClear(scr_vrect.x,
				scr_vrect.y + scr_vrect.height,
				scr_vrect.width,
				viddef.height - (scr_vrect.height + scr_vrect.y), draw_backtile);
		}
	}
	else
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
}

// This is also located in screen.c
#define PLAQUE_WIDTH 26

void Plaque_Draw(const char* message, qboolean AlwaysDraw)
{
	int i;
	char temp[80];
	int bx,by;

	if (con.displayFrac == 1 && !AlwaysDraw)
	{
		return;		// console is full screen

	}
	if (!*message)
	{
		return;
	}

	FindTextBreaks(message, PLAQUE_WIDTH);

	by = ((25 - lines) * 8) / 2;
	MQH_DrawTextBox2(32, by - 16, 30, lines + 2);

	for (i = 0; i < lines; i++,by += 8)
	{
		String::NCpy(temp,&message[StartC[i]],EndC[i] - StartC[i]);
		temp[EndC[i] - StartC[i]] = 0;
		bx = ((40 - String::Length(temp)) * 8) / 2;
		M_Print2(bx, by, temp);
	}
}

void I_Print(int cx, int cy, char* str)
{
	UI_DrawString(cx + ((viddef.width - 320) >> 1), cy + ((viddef.height - 200) >> 1), str, 256);
}

//==========================================================================
//
// SB_IntermissionOverlay
//
//==========================================================================

void SB_IntermissionOverlay(void)
{
	SbarH2_DeathmatchOverlay();
}

//==========================================================================
//
// SB_FinaleOverlay
//
//==========================================================================

void SB_FinaleOverlay(void)
{
	image_t* pic;

	pic = R_CachePic("gfx/finale.lmp");
	UI_DrawPic((viddef.width - R_GetImageWidth(pic)) / 2, 16, pic);
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
	if (cl.qh_intermission < 1 || cl.qh_intermission > 12)
	{
		V_RenderView();
	}

	//
	// draw any areas not covered by the refresh
	//
	SCR_TileClear();

	if (cl_netgraph->value)
	{
		R_NetGraph();
	}

	if (scr_drawdialog)
	{
		SbarH2_Draw();
		MQH_FadeScreen();
		SCR_DrawNotifyString();
	}
	else if (scr_drawloading)
	{
		SbarH2_Draw();
		MQH_FadeScreen();
		SCR_DrawLoading();
	}
	else if (cl.qh_intermission == 1 && in_keyCatchers == 0)
	{
	}
	else if (cl.qh_intermission == 2 && in_keyCatchers == 0)
	{
		SCR_CheckDrawCenterString();
	}
	else
	{
		if (crosshair->value)
		{
			Draw_Crosshair();
		}

		SCR_DrawFPS();
		SCR_DrawTurtle();
		SCR_DrawPause();
		SCR_CheckDrawCenterString();
		SbarH2_Draw();
		Plaque_Draw(plaquemessage,0);
		SCR_DrawNet();
		Con_DrawConsole();
		MQH_Draw();
	}

	V_UpdatePalette();

	R_EndFrame(NULL, NULL);
}

/*
===================
VID_Init
===================
*/

void VID_Init()
{
	R_BeginRegistration(&cls.glconfig);

	CLH2_InitColourShadeTables();

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
	viddef.height = viddef.width * 3 / 4;

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
