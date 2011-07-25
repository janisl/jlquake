
// screen.c -- master for refresh, status bar, console, chat, notify, etc

#include "quakedef.h"
#include "glquake.h"

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
Con_Printf ();

net 
turn off messages option

the refresh is allways rendered, unless the console is full screen


console is:
	notify lines
	half
	full
	

*/

float           scr_con_current;
float           scr_conlines;           // lines of console to display

QCvar*          scr_viewsize;
QCvar*          scr_fov;
QCvar*          scr_conspeed;
QCvar*          scr_centertime;
QCvar*          scr_showram;
QCvar*          scr_showturtle;
QCvar*          scr_showpause;
QCvar*          scr_printspeed;
QCvar*	show_fps;
extern  QCvar*	crosshair;

qboolean        scr_initialized;                // ready to draw

image_t          *scr_ram;
image_t          *scr_net;
image_t          *scr_turtle;

vrect_t         scr_vrect;

qboolean        scr_disabled_for_loading;
qboolean        scr_drawloading;
float           scr_disabled_time;

void Plaque_Draw (const char *message, qboolean AlwaysDraw);

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char		scr_centerstring[1024];
float		scr_centertime_start;	// for slow victory printing
float		scr_centertime_off;
int			scr_center_lines;
int			scr_erase_lines;
int			scr_erase_center;

static int lines;
static int StartC[20],EndC[20];

void FindTextBreaks(const char *message, int Width)
{
	int length,pos,start,lastspace,oldlast;

	length = QStr::Length(message);
	lines = pos = start = 0;
	lastspace = -1;

	while(1)
	{
		if (pos-start >= Width || message[pos] == '@' ||
			message[pos] == 0)
		{
			oldlast = lastspace;
			if (message[pos] == '@' || lastspace == -1 || message[pos] == 0)
				lastspace = pos;

			StartC[lines] = start;
			EndC[lines] = lastspace;
			lines++;

			if (message[pos] == '@')
				start = pos + 1;
			else if (oldlast == -1)
				start = lastspace;
			else
				start = lastspace + 1;

			lastspace = -1;
		}

		if (message[pos] == 32) lastspace = pos;
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
void SCR_CenterPrint (char *str)
{
	QStr::NCpy(scr_centerstring, str, sizeof(scr_centerstring)-1);
	scr_centertime_off = scr_centertime->value;
	scr_centertime_start = cl.time;

	FindTextBreaks(scr_centerstring, 38);
	scr_center_lines = lines;
}

void SCR_DrawCenterString (void)
{
	int		i;
	int		bx, by;
	int		remaining;
	char	temp[80];

// the finale prints the characters one at a time
	if (cl.intermission)
		remaining = scr_printspeed->value * (cl.time - scr_centertime_start);
	else
		remaining = 9999;

	scr_erase_center = 0;

	FindTextBreaks(scr_centerstring, 38);

	by = ((25-lines) * 8) / 2;
	for(i=0;i<lines;i++,by+=8)
	{
		QStr::NCpy(temp,&scr_centerstring[StartC[i]],EndC[i]-StartC[i]);
		temp[EndC[i]-StartC[i]] = 0;
		bx = ((40-QStr::Length(temp)) * 8) / 2;
	  	M_Print2 (bx, by, temp);
	}
}

void SCR_CheckDrawCenterString (void)
{
	if (scr_center_lines > scr_erase_lines)
		scr_erase_lines = scr_center_lines;

	scr_centertime_off -= host_frametime;
	
	if (scr_centertime_off <= 0 && !cl.intermission)
		return;
	if (in_keyCatchers != 0)
		return;

	SCR_DrawCenterString ();
}

//=============================================================================

/*
=================
SCR_CalcRefdef

Must be called whenever vid changes
Internal use only
=================
*/
static void SCR_CalcRefdef (void)
{
	float           size;
	int             h;


// force the status bar to redraw
	Sbar_Changed ();

//========================================
	
// bound viewsize
	if (scr_viewsize->value < 30)
		Cvar_Set ("viewsize","30");
	if (scr_viewsize->value > 120)
		Cvar_Set ("viewsize","120");

// bound field of view
	if (scr_fov->value < 10)
		Cvar_Set ("fov","10");
	if (scr_fov->value > 170)
		Cvar_Set ("fov","170");

// intermission is always full screen	
	if (cl.intermission)
		size = 110;
	else
		size = scr_viewsize->value;

//	if (size >= 120)
//		sb_lines = 0;		// no status bar at all
//	else if (size >= 110)
//		sb_lines = 24;		// no inventory
//	else
//		sb_lines = 24+16+8;

	if (size >= 110)
	{ // No status bar
		sb_lines = 0;
	}
	else
	{
		sb_lines = 36;
	}

	size = scr_viewsize->value > 100 ? 100 : scr_viewsize->value;
	if (cl.intermission)
	{
		size = 100;

		sb_lines = 0;
	}
	size /= 100;

	h = viddef.height - sb_lines;
	scr_vrect.width = viddef.width * size;
	if (scr_vrect.width < 96)
	{
		size = 96.0 / viddef.width;
		scr_vrect.width = 96;	// min for icons
	}

	scr_vrect.height = viddef.height * size;
	if (scr_vrect.height > (int)viddef.height - sb_lines)
		scr_vrect.height = viddef.height - sb_lines;

	scr_vrect.x = (viddef.width - scr_vrect.width)/2;
	scr_vrect.y = (h - scr_vrect.height)/2;

	r_refdef.x = scr_vrect.x * glConfig.vidWidth / viddef.width;
	r_refdef.y = scr_vrect.y * glConfig.vidHeight / viddef.height;
	r_refdef.width = scr_vrect.width * glConfig.vidWidth / viddef.width;
	r_refdef.height = scr_vrect.height * glConfig.vidHeight / viddef.height;
	r_refdef.fov_x = scr_fov->value;
	r_refdef.fov_y = 2 * atan((float)r_refdef.height / r_refdef.width) * (r_refdef.fov_x * 2) / M_PI;
}

/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void SCR_SizeUp_f (void)
{
	if (scr_viewsize->value < 110)
	{
		Cvar_SetValue ("viewsize",scr_viewsize->value+10);
		SB_ViewSizeChanged();
	}
}


/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
void SCR_SizeDown_f (void)
{
	Cvar_SetValue ("viewsize",scr_viewsize->value-10);
	SB_ViewSizeChanged();
}

//============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init (void)
{
	scr_viewsize = Cvar_Get("viewsize","100", CVAR_ARCHIVE);
	scr_fov = Cvar_Get("fov", "90", 0); // 10 - 170
	scr_conspeed = Cvar_Get("scr_conspeed", "300", 0);
	scr_centertime = Cvar_Get("scr_centertime", "4", 0);
	scr_showram = Cvar_Get("showram", "1", 0);
	scr_showturtle = Cvar_Get("showturtle", "0", 0);
	scr_showpause = Cvar_Get("showpause", "1", 0);
	scr_printspeed = Cvar_Get("scr_printspeed", "8", 0);

//
// register our commands
//
	Cmd_AddCommand ("sizeup",SCR_SizeUp_f);
	Cmd_AddCommand ("sizedown",SCR_SizeDown_f);

	scr_ram = R_PicFromWad ("ram");
	scr_net = R_PicFromWad ("net");
	scr_turtle = R_PicFromWad ("turtle");

	show_fps = Cvar_Get("show_fps", "0", 0);			// set for running times
	cl_sbar		= Cvar_Get("cl_sbar", "0", CVAR_ARCHIVE);

	scr_initialized = true;
}



/*
==============
SCR_DrawRam
==============
*/
void SCR_DrawRam (void)
{
	if (!scr_showram->value)
		return;

	if (!r_cache_thrash)
		return;

	UI_DrawPic (scr_vrect.x+32, scr_vrect.y, scr_ram);
}

/*
==============
SCR_DrawTurtle
==============
*/
void SCR_DrawTurtle (void)
{
	static int      count;
	
	if (!scr_showturtle->value)
		return;

	if (host_frametime < 0.1)
	{
		count = 0;
		return;
	}

	count++;
	if (count < 3)
		return;

	UI_DrawPic (scr_vrect.x, scr_vrect.y, scr_turtle);
}

/*
==============
SCR_DrawNet
==============
*/
void SCR_DrawNet (void)
{
	if (cls.netchan.outgoing_sequence - cls.netchan.incoming_acknowledged < UPDATE_BACKUP-1)
		return;
	if (cls.demoplayback)
		return;

	UI_DrawPic (scr_vrect.x+64, scr_vrect.y, scr_net);
}

void SCR_DrawFPS (void)
{
	extern QCvar* show_fps;
	static double lastframetime;
	double t;
	extern int fps_count;
	static int lastfps;
	int x, y;
	char st[80];

	if (!show_fps->value)
		return;

	t = Sys_DoubleTime();
	if ((t - lastframetime) >= 1.0) {
		lastfps = fps_count;
		fps_count = 0;
		lastframetime = t;
	}

	sprintf(st, "%3d FPS", lastfps);
	x = viddef.width - QStr::Length(st) * 8 - 8;
	y = viddef.height - sb_lines - 8;
	Draw_String(x, y, st);
}


/*
==============
DrawPause
==============
*/
void SCR_DrawPause (void)
{
	image_t	*pic;
	float delta;
	static qboolean newdraw = false;
	int finaly;
	static float LogoPercent,LogoTargetPercent;

	if (!scr_showpause->value)		// turn off for screenshots
		return;

	if (!cl.paused)
	{
		newdraw = false;
		return;
	}

	if (!newdraw)
	{
		newdraw = true;
		LogoTargetPercent= 1;
		LogoPercent = 0;
	}

	pic = R_CachePic ("gfx/menu/paused.lmp");
//	UI_DrawPic ( (viddef.width - pic->width)/2, 
//		(viddef.height - 48 - pic->height)/2, pic);

	if (LogoPercent < LogoTargetPercent)
	{
		delta = ((LogoTargetPercent-LogoPercent)/.5)*host_frametime;
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

	finaly = ((float)pic->height * LogoPercent) - pic->height;
	UI_DrawPic( (viddef.width - pic->width)/2, finaly, pic);
}

/*
==============
SCR_DrawLoading
==============
*/
void SCR_DrawLoading (void)
{
	image_t  *pic;

	if (!scr_drawloading)
		return;
		
	pic = R_CachePic ("gfx/loading.lmp");
	UI_DrawPic ( (viddef.width - pic->width)/2, 
		(viddef.height - 48 - pic->height)/2, pic);
}



//=============================================================================


/*
==================
SCR_SetUpToDrawConsole
==================
*/
void SCR_SetUpToDrawConsole (void)
{
	Con_CheckResize ();
	
	if (scr_drawloading)
		return;         // never a console with loading plaque
		
// decide on the height of the console
	if (cls.state != ca_active)
	{
		scr_conlines = viddef.height;              // full screen
		scr_con_current = scr_conlines;
	}
	else if (in_keyCatchers & KEYCATCH_CONSOLE)
		scr_conlines = viddef.height/2;    // half screen
	else
		scr_conlines = 0;                               // none visible
	
	if (scr_conlines < scr_con_current)
	{
		scr_con_current -= scr_conspeed->value*host_frametime;
		if (scr_conlines > scr_con_current)
			scr_con_current = scr_conlines;

	}
	else if (scr_conlines > scr_con_current)
	{
		scr_con_current += scr_conspeed->value*host_frametime;
		if (scr_conlines < scr_con_current)
			scr_con_current = scr_conlines;
	}
}
	
/*
==================
SCR_DrawConsole
==================
*/
void SCR_DrawConsole (void)
{
	if (scr_con_current)
	{
		Con_DrawConsole (scr_con_current);
	}
	else
	{
		if (in_keyCatchers == 0 || in_keyCatchers == KEYCATCH_MESSAGE)
			Con_DrawNotify ();      // only draw notify in game
	}
}


//=============================================================================

const char    *scr_notifystring;
qboolean        scr_drawdialog;

void SCR_DrawNotifyString (void)
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
int SCR_ModalMessage (const char *text)
{
	scr_notifystring = text;
 
// draw a fresh screen
	scr_drawdialog = true;
	SCR_UpdateScreen ();
	scr_drawdialog = false;
	
	S_ClearSoundBuffer ();               // so dma doesn't loop current sound

	do
	{
		key_count = -1;         // wait for a key down and up
		Sys_SendKeyEvents ();
		IN_ProcessEvents();
	} while (key_lastpress != 'y' && key_lastpress != 'n' && key_lastpress != K_ESCAPE);

	SCR_UpdateScreen ();

	return key_lastpress == 'y';
}


//=============================================================================

/*
===============
SCR_BringDownConsole

Brings the console down and fades the palettes back to normal
================
*/
void SCR_BringDownConsole (void)
{
	int             i;
	
	scr_centertime_off = 0;
	
	for (i=0 ; i<20 && scr_conlines != scr_con_current ; i++)
		SCR_UpdateScreen ();

	cl.cshifts[0].percent = 0;              // no area contents palette on next frame
}

void SCR_TileClear (void)
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
			UI_TileClear(0, 0, scr_vrect.x, viddef.height - sb_lines, draw_backtile);
			// right
			UI_TileClear(scr_vrect.x + scr_vrect.width, 0, 
				viddef.width - scr_vrect.x + scr_vrect.width, 
				viddef.height - sb_lines, draw_backtile);
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
				viddef.height - sb_lines - 
				(scr_vrect.height + scr_vrect.y), draw_backtile);
		}
	}
}

// This is also located in screen.c
#define PLAQUE_WIDTH 26

void Plaque_Draw (const char *message, qboolean AlwaysDraw)
{
	int i;
	char temp[80];
	int bx,by;

	if (scr_con_current == viddef.height && !AlwaysDraw)
		return;		// console is full screen

	if (!*message) 
		return;

	FindTextBreaks(message, PLAQUE_WIDTH);

	by = ((25-lines) * 8) / 2;
	M_DrawTextBox2 (32, by-16, 30, lines+2);

	for(i=0;i<lines;i++,by+=8)
	{
		QStr::NCpy(temp,&message[StartC[i]],EndC[i]-StartC[i]);
		temp[EndC[i]-StartC[i]] = 0;
		bx = ((40-QStr::Length(temp)) * 8) / 2;
	  	M_Print2 (bx, by, temp);
	}
}


void I_DrawCharacter (int cx, int line, int num)
{
	Draw_Character ( cx + ((viddef.width - 320)>>1), line + ((viddef.height - 200)>>1), num);
}

void I_Print (int cx, int cy, char *str)
{
	while (*str)
	{
		I_DrawCharacter (cx, cy, ((unsigned char)(*str))+256);
		str++;
		cx += 8;
	}
}

//==========================================================================
//
// SB_IntermissionOverlay
//
//==========================================================================

void SB_IntermissionOverlay(void)
{
	image_t	*pic;
	int		elapsed, size, bx, by, i;
	const char	*message;
	char	temp[80];

//rjr	if (cl.gametype == GAME_DEATHMATCH)
	{
		Sbar_DeathmatchOverlay ();
		return;
	}
	
	switch(cl.intermission)
	{
		case 1:
			pic = R_CachePic ("gfx/meso.lmp");
			break;
		case 2:
			pic = R_CachePic ("gfx/egypt.lmp");
			break;
		case 3:
			pic = R_CachePic ("gfx/roman.lmp");
			break;
		case 4:
			pic = R_CachePic ("gfx/castle.lmp");
			break;
		case 5:
			pic = R_CachePic ("gfx/castle.lmp");
			break;
		case 6:
			pic = R_CachePic ("gfx/end-1.lmp");
			break;
		case 7:
			pic = R_CachePic ("gfx/end-2.lmp");
			break;
		case 8:
			pic = R_CachePic ("gfx/end-3.lmp");
			break;
		case 9:
			pic = R_CachePic ("gfx/castle.lmp");
			break;
		case 10://Defender win - wipe out or time limit
			pic = R_CachePic ("gfx/defwin.lmp");
			break;
		case 11://Attacker win - caught crown
			pic = R_CachePic ("gfx/attwin.lmp");
			break;
		case 12://Attacker win 2 - wiped out
			pic = R_CachePic ("gfx/attwin2.lmp");
			break;
		default:
			Sys_Error ("SB_IntermissionOverlay: Bad episode");
			break;
	}
	UI_DrawPic (((viddef.width - 320)>>1),((viddef.height - 200)>>1), pic);

	if (cl.intermission >= 6 && cl.intermission <= 8)
	{
		elapsed = (cl.time - cl.completed_time) * 20;
		elapsed -= 50;
		if (elapsed < 0) elapsed = 0;
	}
	else
	{
		elapsed = (cl.time - cl.completed_time) * 20;
	}

	if (cl.intermission <= 4 && cl.intermission + 394 <= pr_string_count)
		message = &pr_global_strings[pr_string_index[cl.intermission + 394]];
	else if (cl.intermission == 5)
		message = &pr_global_strings[pr_string_index[408]];
	else if (cl.intermission >= 6 && cl.intermission <= 8 && cl.intermission + 386 <= pr_string_count)
		message = &pr_global_strings[pr_string_index[cl.intermission + 386]];
	else if (cl.intermission == 9)
		message = &pr_global_strings[pr_string_index[391]];
	else
		message = "";

	FindTextBreaks(message, 38);

	if (cl.intermission == 8)
		by = 16;
	else
		by = ((25-lines) * 8) / 2;

	for(i=0;i<lines;i++,by+=8)
	{
		size = EndC[i]-StartC[i];
		QStr::NCpy(temp,&message[StartC[i]],size);

		if (size > elapsed) size = elapsed;
		temp[size] = 0;

		bx = ((40-QStr::Length(temp)) * 8) / 2;
	  	if (cl.intermission < 6)
			I_Print (bx, by, temp);
		else
			M_PrintWhite (bx, by, temp);

		elapsed -= size;
		if (elapsed <= 0) break;
	}

	if (i == lines && elapsed >= 300 && cl.intermission >= 6 && cl.intermission <= 7)
	{
		cl.intermission++;
		cl.completed_time = cl.time;
	}
//	Con_Printf("Time is %10.2f\n",elapsed);
}

//==========================================================================
//
// SB_FinaleOverlay
//
//==========================================================================

void SB_FinaleOverlay(void)
{
	image_t	*pic;

	pic = R_CachePic("gfx/finale.lmp");
	UI_DrawPic((viddef.width-pic->width)/2, 16, pic);
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
void SCR_UpdateScreen (void)
{
	if (scr_disabled_for_loading)
	{
		if (realtime - scr_disabled_time > 60)
		{
			scr_disabled_for_loading = false;
			Con_Printf ("load failed.\n");
		}
		else
			return;
	}

	if (!scr_initialized || !con_initialized || !glConfig.vidWidth)
		return;                         // not initialized yet


	//
	// determine size of refresh window
	//
	SCR_CalcRefdef ();

//
// do 3D refresh drawing, and then update the screen
//
	SCR_SetUpToDrawConsole ();
	
	if (cl.intermission < 1 || cl.intermission > 12)
	{
		V_RenderView ();
	}

	//
	// draw any areas not covered by the refresh
	//
	SCR_TileClear ();

	if (r_netgraph->value)
		R_NetGraph ();

	if (scr_drawdialog)
	{
		Sbar_Draw ();
		Draw_FadeScreen ();
		SCR_DrawNotifyString ();
	}
	else if (scr_drawloading)
	{
		Sbar_Draw ();
		Draw_FadeScreen ();
		SCR_DrawLoading ();
	}
	else if (cl.intermission == 1 && in_keyCatchers == 0)
	{
		Sbar_IntermissionOverlay ();
	}
	else if (cl.intermission == 2 && in_keyCatchers == 0)
	{
		Sbar_FinaleOverlay ();
		SCR_CheckDrawCenterString ();
	}
	else
	{
		if (crosshair->value)
			Draw_Crosshair();
		
		SCR_DrawRam ();
		SCR_DrawFPS ();
		SCR_DrawTurtle ();
		SCR_DrawPause ();
		SCR_CheckDrawCenterString ();
		Sbar_Draw ();
		Plaque_Draw(plaquemessage,0);
		SCR_DrawNet ();
		SCR_DrawConsole ();     
		M_Draw ();
		if (errormessage)
			Plaque_Draw(errormessage,1);
	}

	V_UpdatePalette ();

	GL_EndRendering ();
}
