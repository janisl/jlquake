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


Cvar* scr_showturtle;
Cvar* scr_showpause;
Cvar* show_fps;

qboolean scr_initialized;			// ready to draw

image_t* scr_net;
image_t* scr_turtle;

qboolean scr_drawloading;

int total_loading_size, current_loading_size, loading_stage;

const char* plaquemessage = NULL;	// Pointer to current plaque message

qboolean con_forcedup;			// because no entities to refresh

void Plaque_Draw(const char* message, qboolean AlwaysDraw);
void Info_Plaque_Draw(const char* message);

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

/*
==================
SCR_Init
==================
*/
void SCR_Init(void)
{
	scr_showturtle = Cvar_Get("showturtle", "0", 0);
	scr_showpause = Cvar_Get("showpause", "1", 0);
	show_fps = Cvar_Get("show_fps", "0", CVAR_ARCHIVE);			// set for running times
	SCR_InitCommon();

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
	SCR_ClearCenterString();
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

	SCRH2_FindTextBreaks(message, PLAQUE_WIDTH);

	by = ((25 - scrh2_lines) * 8) / 2;
	MQH_DrawTextBox2(32, by - 16, 30, scrh2_lines + 2);

	for (i = 0; i < scrh2_lines; i++,by += 8)
	{
		String::NCpy(temp,&message[scrh2_StartC[i]],scrh2_EndC[i] - scrh2_StartC[i]);
		temp[scrh2_EndC[i] - scrh2_StartC[i]] = 0;
		bx = ((40 - String::Length(temp)) * 8) / 2;
		MH2_Print2(bx, by, temp);
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

	SCRH2_FindTextBreaks(message, PLAQUE_WIDTH + 4);

	if (scrh2_lines == MAXLINES_H2)
	{
		common->DPrintf("Info_Plaque_Draw: line overflow error\n");
		scrh2_lines = MAXLINES_H2 - 1;
	}

	by = ((25 - scrh2_lines) * 8) / 2;
	MQH_DrawTextBox2(15, by - 16, PLAQUE_WIDTH + 4 + 4, scrh2_lines + 2);

	for (i = 0; i < scrh2_lines; i++,by += 8)
	{
		String::NCpy(temp,&message[scrh2_StartC[i]],scrh2_EndC[i] - scrh2_StartC[i]);
		temp[scrh2_EndC[i] - scrh2_StartC[i]] = 0;
		bx = ((40 - String::Length(temp)) * 8) / 2;
		MH2_Print2(bx, by, temp);
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

	SCRH2_FindTextBreaks(message, 38);

	if (cl.qh_intermission == 8)
	{
		by = 16;
	}
	else
	{
		by = ((25 - scrh2_lines) * 8) / 2;
	}

	for (i = 0; i < scrh2_lines; i++,by += 8)
	{
		size = scrh2_EndC[i] - scrh2_StartC[i];
		String::NCpy(temp,&message[scrh2_StartC[i]],size);

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

	if (i == scrh2_lines && elapsed >= 300 && cl.qh_intermission >= 6 && cl.qh_intermission <= 7)
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
