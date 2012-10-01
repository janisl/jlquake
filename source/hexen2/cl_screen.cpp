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


Cvar* scr_fov;
Cvar* scr_centertime;
Cvar* scr_showturtle;
Cvar* scr_showpause;
Cvar* scr_printspeed;
Cvar* show_fps;

qboolean scr_initialized;			// ready to draw

image_t* scr_net;
image_t* scr_turtle;

qboolean scr_drawloading;

static qboolean scr_needfull = false;

int total_loading_size, current_loading_size, loading_stage;

const char* plaquemessage = NULL;	// Pointer to current plaque message

qboolean con_forcedup;			// because no entities to refresh

void Plaque_Draw(const char* message, qboolean AlwaysDraw);
void Info_Plaque_Draw(const char* message);
void Bottom_Plaque_Draw(const char* message);

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
#define MAXLINES 27
static int StartC[MAXLINES],EndC[MAXLINES];

#ifdef MISSIONPACK
#define MAX_INFO 1024
char infomessage[MAX_INFO];
qboolean info_up = false;
#endif

#ifdef MISSIONPACK
void UpdateInfoMessage(void)
{
	unsigned int i;
	unsigned int check;
	char* newmessage;

	String::Cpy(infomessage, "Objectives:");

	if (!prh2_global_info_strings)
	{
		return;
	}

	for (i = 0; i < 32; i++)
	{
		check = (1 << i);

		if (cl.h2_info_mask & check)
		{
			newmessage = &prh2_global_info_strings[prh2_info_string_index[i]];
			String::Cat(infomessage, sizeof(infomessage), "@@");
			String::Cat(infomessage, sizeof(infomessage), newmessage);
		}
	}

	for (i = 0; i < 32; i++)
	{
		check = (1 << i);

		if (cl.h2_info_mask2 & check)
		{
			newmessage = &prh2_global_info_strings[prh2_info_string_index[i + 32]];
			String::Cat(infomessage, sizeof(infomessage), "@@");
			String::Cat(infomessage, sizeof(infomessage), newmessage);
		}
	}
}
#endif

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
			if (lines == MAXLINES)
			{
				return;
			}
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

	if (h2intro_playing)
	{
		Bottom_Plaque_Draw(scr_centerstring);
	}
	else
	{
		SCR_DrawCenterString();
	}
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

/*	if (size >= 120)
        sbqh_lines = 0;		// no status bar at all
    else if (size >= 110)
        sbqh_lines = 24;		// no inventory
    else
        sbqh_lines = 24+16+8;*/

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
	cl.refdef.fov_x = 90;
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
	SbarH2_ViewSizeChanged();
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
==================
SCR_Init
==================
*/
void SCR_Init(void)
{
	scr_viewsize = Cvar_Get("viewsize", "100", CVAR_ARCHIVE);
	scr_fov = Cvar_Get("fov", "90", 0);	// 10 - 170
	scr_centertime = Cvar_Get("scr_centertime", "4", 0);
	scr_showturtle = Cvar_Get("showturtle", "0", 0);
	scr_showpause = Cvar_Get("showpause", "1", 0);
	scr_printspeed = Cvar_Get("scr_printspeed", "8", 0);
	show_fps = Cvar_Get("show_fps", "0", CVAR_ARCHIVE);			// set for running times
	SCR_InitCommon();

//
// register our commands
//
	Cmd_AddCommand("sizeup",SCR_SizeUp_f);
	Cmd_AddCommand("sizedown",SCR_SizeDown_f);

	scr_net = R_PicFromWad("net");
	scr_turtle = R_PicFromWad("turtle");

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
	if (realtime - cl.qh_last_received_message < 0.3)
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

//	if (!cl.paused)
//		return;

//	pic = R_CachePic ("gfx/pause.lmp");
//	UI_DrawPic ( (viddef.width - pic->width)/2,
//		(viddef.height - 48 - pic->height)/2, pic);


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
	int size, count, offset;
	image_t* pic;

	if (!scr_drawloading && loading_stage == 0)
	{
		return;
	}

	pic = R_CachePic("gfx/menu/loading.lmp");
	offset = (viddef.width - R_GetImageWidth(pic)) / 2;
	UI_DrawPic(offset, 0, pic);

	if (loading_stage == 0)
	{
		return;
	}

	if (total_loading_size)
	{
		size = current_loading_size * 106 / total_loading_size;
	}
	else
	{
		size = 0;
	}

	if (loading_stage == 1)
	{
		count = size;
	}
	else
	{
		count = 106;
	}

	UI_FillPal(offset + 42, 87, count, 1, 136);
	UI_FillPal(offset + 42, 87 + 1, count, 4, 138);
	UI_FillPal(offset + 42, 87 + 5, count, 1, 136);

	if (loading_stage == 2)
	{
		count = size;
	}
	else
	{
		count = 0;
	}

	UI_FillPal(offset + 42, 97, count, 1, 168);
	UI_FillPal(offset + 42, 97 + 1, count, 4, 170);
	UI_FillPal(offset + 42, 97 + 5, count, 1, 168);
}



//=============================================================================

/*
===============
SCRQH_BeginLoadingPlaque

================
*/
void SCRQH_BeginLoadingPlaque(void)
{
	S_StopAllSounds();

	if (cls.state != CA_ACTIVE)
	{
		return;
	}
	if (clc.qh_signon != SIGNONS)
	{
		return;
	}

// redraw with no console and the loading plaque
	Con_ClearNotify();
	scr_centertime_off = 0;
	con.displayFrac = 0;

	scr_drawloading = true;
	SCR_UpdateScreen();
	scr_drawloading = false;

	cls.disable_screen = realtime * 1000;
}

//=============================================================================

void SCR_TileClear(void)
{
	if (scr_vrect.x > 0)
	{
		UI_TileClear(0,0,scr_vrect.x,viddef.height, draw_backtile);
		UI_TileClear(scr_vrect.x + scr_vrect.width, 0,
			viddef.width - scr_vrect.x + scr_vrect.width,viddef.height, draw_backtile);
	}
//	if (scr_vrect.height < viddef.height-44)
	{
		UI_TileClear(scr_vrect.x, 0, scr_vrect.width, scr_vrect.y, draw_backtile);
		UI_TileClear(scr_vrect.x, scr_vrect.y + scr_vrect.height,
			scr_vrect.width,
			viddef.height - (scr_vrect.y + scr_vrect.height), draw_backtile);
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

void Info_Plaque_Draw(const char* message)
{
	int i;
	char temp[80];
	int bx,by;

	if (con.displayFrac == 1)
	{
		return;		// console is full screen

	}
	if (!*message)
	{
		return;
	}

	scr_needfull = true;

	FindTextBreaks(message, PLAQUE_WIDTH + 4);

	if (lines == MAXLINES)
	{
		common->DPrintf("Info_Plaque_Draw: line overflow error\n");
		lines = MAXLINES - 1;
	}

	by = ((25 - lines) * 8) / 2;
	MQH_DrawTextBox2(15, by - 16, PLAQUE_WIDTH + 4 + 4, lines + 2);

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

void Bottom_Plaque_Draw(const char* message)
{
	int i;
	char temp[80];
	int bx,by;

	if (!*message)
	{
		return;
	}

	scr_needfull = true;

	FindTextBreaks(message, PLAQUE_WIDTH);

	by = (((viddef.height) / 8) - lines - 2) * 8;

	MQH_DrawTextBox(32, by - 16, 30, lines + 2);

	for (i = 0; i < lines; i++,by += 8)
	{
		String::NCpy(temp,&message[StartC[i]],EndC[i] - StartC[i]);
		temp[EndC[i] - StartC[i]] = 0;
		bx = ((40 - String::Length(temp)) * 8) / 2;
		MQH_Print(bx, by, temp);
	}
}
//==========================================================================
//
// SB_IntermissionOverlay
//
//==========================================================================

void SB_IntermissionOverlay(void)
{
	image_t* pic;
	int elapsed, size, bx, by, i;
	const char* message;
	char temp[80];

	if (cl.qh_gametype == QHGAME_DEATHMATCH)
	{
		SbarH2_DeathmatchOverlay();
		return;
	}

	switch (cl.qh_intermission)
	{
	case 1:
		pic = R_CachePic("gfx/meso.lmp");
		break;
	case 2:
		pic = R_CachePic("gfx/egypt.lmp");
		break;
	case 3:
		pic = R_CachePic("gfx/roman.lmp");
		break;
	case 4:
		pic = R_CachePic("gfx/castle.lmp");
		break;
	case 5:
		pic = R_CachePic("gfx/castle.lmp");
		break;
	case 6:
		pic = R_CachePic("gfx/end-1.lmp");
		break;
	case 7:
		pic = R_CachePic("gfx/end-2.lmp");
		break;
	case 8:
		pic = R_CachePic("gfx/end-3.lmp");
		break;
	case 9:
		pic = R_CachePic("gfx/castle.lmp");
		break;
	case 10:
		pic = R_CachePic("gfx/mpend.lmp");
		break;
	case 11:
		pic = R_CachePic("gfx/mpmid.lmp");
		break;
	case 12:
		pic = R_CachePic("gfx/end-3.lmp");
		break;

	default:
		common->FatalError("SB_IntermissionOverlay: Bad episode");
		break;
	}

	UI_DrawPic(((viddef.width - 320) >> 1),((viddef.height - 200) >> 1), pic);

	if (cl.qh_intermission >= 6 && cl.qh_intermission <= 8)
	{
		elapsed = (cl.qh_serverTimeFloat - cl.qh_completed_time) * 20;
		elapsed -= 50;
		if (elapsed < 0)
		{
			elapsed = 0;
		}
	}
	else if (cl.qh_intermission == 12)
	{
		elapsed = (h2_introTime);
		if (h2_introTime < 500)
		{
			h2_introTime += 0.25;
		}
	}
	else
	{
		elapsed = (cl.qh_serverTimeFloat - cl.qh_completed_time) * 20;
	}

	if (cl.qh_intermission <= 4 && cl.qh_intermission + 394 <= prh2_string_count)
	{
		message = &prh2_global_strings[prh2_string_index[cl.qh_intermission + 394]];
	}
	else if (cl.qh_intermission == 5)
	{
		message = &prh2_global_strings[prh2_string_index[ABILITIES_STR_INDEX + NUM_CLASSES * 2 + 1]];
	}
	else if (cl.qh_intermission >= 6 && cl.qh_intermission <= 8 && cl.qh_intermission + 386 <= prh2_string_count)
	{
		message = &prh2_global_strings[prh2_string_index[cl.qh_intermission + 386]];
	}
	else if (cl.qh_intermission == 9)
	{
		message = &prh2_global_strings[prh2_string_index[391]];
	}
	else
	{
		message = "";
	}

	if (cl.qh_intermission == 10)
	{
		message = &prh2_global_strings[prh2_string_index[538]];
	}
	else if (cl.qh_intermission == 11)
	{
		message = &prh2_global_strings[prh2_string_index[545]];
	}
	else if (cl.qh_intermission == 12)
	{
		message = &prh2_global_strings[prh2_string_index[561]];
	}

	FindTextBreaks(message, 38);

	if (cl.qh_intermission == 8)
	{
		by = 16;
	}
	else
	{
		by = ((25 - lines) * 8) / 2;
	}

	for (i = 0; i < lines; i++,by += 8)
	{
		size = EndC[i] - StartC[i];
		String::NCpy(temp,&message[StartC[i]],size);

		if (size > elapsed)
		{
			size = elapsed;
		}
		temp[size] = 0;

		bx = ((40 - String::Length(temp)) * 8) / 2;
		if (cl.qh_intermission < 6 || cl.qh_intermission > 9)
		{
			I_Print(bx, by, temp);
		}
		else
		{
			MQH_PrintWhite(bx, by, temp);
		}

		elapsed -= size;
		if (elapsed <= 0)
		{
			break;
		}
	}

	if (i == lines && elapsed >= 300 && cl.qh_intermission >= 6 && cl.qh_intermission <= 7)
	{
		cl.qh_intermission++;
		cl.qh_completed_time = cl.qh_serverTimeFloat;
	}
//	common->Printf("Time is %10.2f\n",elapsed);
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
			total_loading_size = 0;
			loading_stage = 0;
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
	SCR_CalcRefdef();

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

	if (scr_drawloading)
	{
		SbarH2_Draw();
		MQH_FadeScreen();
		SCR_DrawLoading();
	}
	else if (cl.qh_intermission >= 1 && cl.qh_intermission <= 12)
	{
		SB_IntermissionOverlay();
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
		SCR_DrawTurtle();
		SCR_DrawPause();
		SCR_CheckDrawCenterString();
		SbarH2_Draw();
		Plaque_Draw(plaquemessage,0);
		Con_DrawConsole();
		UI_DrawMenu();

#ifdef MISSIONPACK
		if (info_up)
		{
			UpdateInfoMessage();
			Info_Plaque_Draw(infomessage);
		}
#endif
	}

	if (loading_stage)
	{
		SCR_DrawLoading();
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
