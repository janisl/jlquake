/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
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
#include "glquake.h"

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
QCvar*          scr_showturtle;
QCvar*          scr_showpause;
QCvar*          scr_printspeed;
QCvar*			scr_allowsnap;
QCvar*	        show_fps;
extern QCvar*	crosshair;

qboolean        scr_initialized;                // ready to draw

image_t          *scr_net;
image_t          *scr_turtle;

vrect_t         scr_vrect;

qboolean        scr_disabled_for_loading;
qboolean        scr_drawloading;
float           scr_disabled_time;

void SCR_RSShot_f (void);

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char            scr_centerstring[1024];
float           scr_centertime_start;   // for slow victory printing
float           scr_centertime_off;
int                     scr_center_lines;
int                     scr_erase_lines;
int                     scr_erase_center;

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

// count the number of lines for centering
	scr_center_lines = 1;
	while (*str)
	{
		if (*str == '\n')
			scr_center_lines++;
		str++;
	}
}


void SCR_DrawCenterString (void)
{
	char    *start;
	int             l;
	int             j;
	int             x, y;
	int             remaining;

// the finale prints the characters one at a time
	if (cl.intermission)
		remaining = scr_printspeed->value * (cl.time - scr_centertime_start);
	else
		remaining = 9999;

	scr_erase_center = 0;
	start = scr_centerstring;

	if (scr_center_lines <= 4)
		y = viddef.height*0.35;
	else
		y = 48;

	do      
	{
	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (start[l] == '\n' || !start[l])
				break;
		x = (viddef.width - l*8)/2;
		for (j=0 ; j<l ; j++, x+=8)
		{
			Draw_Character (x, y, start[j]);        
			if (!remaining--)
				return;
		}
			
		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;                // skip the \n
	} while (1);
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
====================
CalcFov
====================
*/
float CalcFov (float fov_x, float width, float height)
{
        float   a;
        float   x;

        if (fov_x < 1 || fov_x > 179)
                Sys_Error ("Bad fov: %f", fov_x);

        x = width/tan(fov_x/360*M_PI);

        a = atan (height/x);

        a = a*360/M_PI;

        return a;
}

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
	qboolean		full = false;


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
		size = 120;
	else
		size = scr_viewsize->value;

	if (size >= 120)
		sb_lines = 0;           // no status bar at all
	else if (size >= 110)
		sb_lines = 24;          // no inventory
	else
		sb_lines = 24+16+8;

	if (scr_viewsize->value >= 100.0) {
		full = true;
		size = 100.0;
	} else
		size = scr_viewsize->value;
	if (cl.intermission)
	{
		full = true;
		size = 100.0;
		sb_lines = 0;
	}
	size /= 100.0;

	if (!cl_sbar->value && full)
		h = viddef.height;
	else
		h = viddef.height - sb_lines;

	scr_vrect.width = viddef.width * size;
	if (scr_vrect.width < 96)
	{
		size = 96.0 / scr_vrect.width;
		scr_vrect.width = 96;      // min for icons
	}

	scr_vrect.height = viddef.height * size;
	if (cl_sbar->value || !full) {
  		if (scr_vrect.height > (int)viddef.height - sb_lines)
  			scr_vrect.height = viddef.height - sb_lines;
	} else if (scr_vrect.height > (int)viddef.height)
			scr_vrect.height = viddef.height;
	scr_vrect.x = (viddef.width - scr_vrect.width)/2;
	if (full)
		scr_vrect.y = 0;
	else 
		scr_vrect.y = (h - scr_vrect.height)/2;

	r_refdef.x = scr_vrect.x * glConfig.vidWidth / viddef.width;
	r_refdef.y = scr_vrect.y * glConfig.vidHeight / viddef.height;
	r_refdef.width = scr_vrect.width * glConfig.vidWidth / viddef.width;
	r_refdef.height = scr_vrect.height * glConfig.vidHeight / viddef.height;
	r_refdef.fov_x = scr_fov->value;
	r_refdef.fov_y = CalcFov (r_refdef.fov_x, r_refdef.width, r_refdef.height);
}


/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void SCR_SizeUp_f (void)
{
	Cvar_SetValue ("viewsize",scr_viewsize->value+10);
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
}

//============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init (void)
{
	scr_viewsize = Cvar_Get("viewsize", "100", CVAR_ARCHIVE);
	scr_fov = Cvar_Get("fov","90", 0); // 10 - 170
	scr_conspeed = Cvar_Get("scr_conspeed","300", 0);
	scr_centertime = Cvar_Get("scr_centertime","2", 0);
	scr_showturtle = Cvar_Get("showturtle","0", 0);
	scr_showpause = Cvar_Get("showpause","1", 0);
	scr_printspeed = Cvar_Get("scr_printspeed","8", 0);
	scr_allowsnap = Cvar_Get("scr_allowsnap", "1", 0);

//
// register our commands
//
	Cmd_AddCommand ("snap",SCR_RSShot_f);
	Cmd_AddCommand ("sizeup",SCR_SizeUp_f);
	Cmd_AddCommand ("sizedown",SCR_SizeDown_f);

	scr_net = R_PicFromWad ("net");
	scr_turtle = R_PicFromWad ("turtle");

	show_fps = Cvar_Get("show_fps", "0", CVAR_ARCHIVE);			// set for running times
	cl_sbar		= Cvar_Get("cl_sbar", "0", CVAR_ARCHIVE);

	scr_initialized = true;
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
	image_t  *pic;

	if (!scr_showpause->value)               // turn off for screenshots
		return;

	if (!cl.paused)
		return;

	pic = R_CachePic ("gfx/pause.lmp");
	UI_DrawPic ( (viddef.width - R_GetImageWidth(pic))/2, 
		(viddef.height - 48 - R_GetImageHeight(pic))/2, pic);
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
	UI_DrawPic ( (viddef.width - R_GetImageWidth(pic))/2, 
		(viddef.height - 48 - R_GetImageHeight(pic))/2, pic);
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

/*
Find closest color in the palette for named color
*/
int MipColor(int r, int g, int b)
{
	int i;
	float dist;
	int best;
	float bestdist;
	int r1, g1, b1;
	static int lr = -1, lg = -1, lb = -1;
	static int lastbest;

	if (r == lr && g == lg && b == lb)
		return lastbest;

	bestdist = 256*256*3;

	for (i = 0; i < 256; i++) {
		r1 = host_basepal[i*3] - r;
		g1 = host_basepal[i*3+1] - g;
		b1 = host_basepal[i*3+2] - b;
		dist = r1*r1 + g1*g1 + b1*b1;
		if (dist < bestdist) {
			bestdist = dist;
			best = i;
		}
	}
	lr = r; lg = g; lb = b;
	lastbest = best;
	return best;
}

void SCR_DrawCharToSnap (int num, byte *dest, int width)
{
	int		row, col;
	byte	*source;
	int		drawline;
	int		x;

	row = num>>4;
	col = num&15;
	byte* draw_chars = (byte*)R_GetWadLumpByName("conchars");
	source = draw_chars + (row<<10) + (col<<3);

	drawline = 8;

	while (drawline--)
	{
		for (x=0 ; x<8 ; x++)
			if (source[x])
				dest[x] = source[x];
			else
				dest[x] = 98;
		source += 128;
		dest -= width;
	}

}

void SCR_DrawStringToSnap (const char *s, byte *buf, int x, int y, int width)
{
	byte *dest;
	const unsigned char *p;

	dest = buf + ((y * width) + x);

	p = (const unsigned char *)s;
	while (*p) {
		SCR_DrawCharToSnap(*p++, dest, width);
		dest += 8;
	}
}


/* 
================== 
SCR_RSShot_f
================== 
*/  
void SCR_RSShot_f (void) 
{ 
	int     x, y;
	unsigned char		*src, *dest;
	unsigned char		*newbuf;
	int w, h;
	int dx, dy, dex, dey, nx;
	int r, b, g;
	int count;
	float fracw, frach;
	char st[80];
	time_t now;

	if (CL_IsUploading())
		return; // already one pending

	if (cls.state < ca_onserver)
		return; // gotta be connected

	if (!scr_allowsnap->integer)
	{
		cls.netchan.message.WriteByte(clc_stringcmd);
		cls.netchan.message.Print("snap\n");
		Con_Printf("Refusing remote screen shot request.\n");
		return;
	}

	Con_Printf("Remote screen shot requested.\n");

// 
// save the pcx file 
// 
	newbuf = (byte*)malloc(glConfig.vidHeight * glConfig.vidWidth * 3);

	qglReadPixels (0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGB, GL_UNSIGNED_BYTE, newbuf ); 

	w = (viddef.width < RSSHOT_WIDTH) ? glConfig.vidWidth : RSSHOT_WIDTH;
	h = (viddef.height < RSSHOT_HEIGHT) ? glConfig.vidHeight : RSSHOT_HEIGHT;

	fracw = (float)glConfig.vidWidth / (float)w;
	frach = (float)glConfig.vidHeight / (float)h;

	for (y = 0; y < h; y++) {
		dest = newbuf + (w*3 * y);

		for (x = 0; x < w; x++) {
			r = g = b = 0;

			dx = x * fracw;
			dex = (x + 1) * fracw;
			if (dex == dx) dex++; // at least one
			dy = y * frach;
			dey = (y + 1) * frach;
			if (dey == dy) dey++; // at least one

			count = 0;
			for (/* */; dy < dey; dy++) {
				src = newbuf + (glConfig.vidWidth * 3 * dy) + dx * 3;
				for (nx = dx; nx < dex; nx++) {
					r += *src++;
					g += *src++;
					b += *src++;
					count++;
				}
			}
			r /= count;
			g /= count;
			b /= count;
			*dest++ = r;
			*dest++ = b;
			*dest++ = g;
		}
	}

	// convert to eight bit
	for (y = 0; y < h; y++) {
		src = newbuf + (w * 3 * y);
		dest = newbuf + (w * y);

		for (x = 0; x < w; x++) {
			*dest++ = MipColor(src[0], src[1], src[2]);
			src += 3;
		}
	}

	time(&now);
	QStr::Cpy(st, ctime(&now));
	st[QStr::Length(st) - 1] = 0;
	SCR_DrawStringToSnap (st, newbuf, w - QStr::Length(st)*8, h - 1, w);

	QStr::NCpy(st, cls.servername, sizeof(st));
	st[sizeof(st) - 1] = 0;
	SCR_DrawStringToSnap (st, newbuf, w - QStr::Length(st)*8, h - 11, w);

	QStr::NCpy(st, name->string, sizeof(st));
	st[sizeof(st) - 1] = 0;
	SCR_DrawStringToSnap (st, newbuf, w - QStr::Length(st)*8, h - 21, w);

	QArray<byte> buffer;
	R_SavePCXMem(buffer, newbuf, w, h, host_basepal);
	CL_StartUpload(buffer.Ptr(), buffer.Num());

	free(newbuf);
} 




//=============================================================================


//=============================================================================

const char    *scr_notifystring;
qboolean        scr_drawdialog;

void SCR_DrawNotifyString (void)
{
	const char    *start;
	int             l;
	int             j;
	int             x, y;

	start = scr_notifystring;

	y = viddef.height*0.35;

	do      
	{
	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (start[l] == '\n' || !start[l])
				break;
		x = (viddef.width - l*8)/2;
		for (j=0 ; j<l ; j++, x+=8)
			Draw_Character (x, y, start[j]);        
			
		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;                // skip the \n
	} while (1);
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
	if (scr_vrect.x > 0) {
		// left
		UI_TileClear(0, 0, scr_vrect.x, viddef.height - sb_lines, draw_backtile);
		// right
		UI_TileClear(scr_vrect.x + scr_vrect.width, 0, 
			viddef.width - scr_vrect.x + scr_vrect.width, 
			viddef.height - sb_lines, draw_backtile);
	}
	if (scr_vrect.y > 0) {
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
	
	V_RenderView ();

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
		SCR_DrawLoading ();
		Sbar_Draw ();
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
		
		SCR_DrawNet ();
		SCR_DrawFPS ();
		SCR_DrawTurtle ();
		SCR_DrawPause ();
		SCR_CheckDrawCenterString ();
		Sbar_Draw ();
		SCR_DrawConsole ();     
		M_Draw ();
	}

	V_UpdatePalette ();

	R_EndFrame(NULL, NULL);
}
