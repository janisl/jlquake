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

// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

#include "quakedef.h"

extern Cvar*	crosshair;
extern Cvar*	cl_crossx;
extern Cvar*	cl_crossy;
extern Cvar*	crosshaircolor;

image_t		*draw_backtile;

image_t*	char_texture;
image_t*	cs_texture; // crosshair texture

image_t		*conback;

/*
===============
Draw_Init
===============
*/
void Draw_Init (void)
{
	char_texture = R_LoadRawFontImageFromWad("conchars", 128, 128);

	cs_texture = R_CreateCrosshairImage();

	conback = R_CachePic("gfx/conback.lmp");

	//
	// get the other pics we need
	//
	draw_backtile = R_PicFromWadRepeat("backtile");
}

/*
================
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Character (int x, int y, int num)
{
	num &= 255;

	if (num == 32)
		return;		// space

	UI_DrawChar(x, y, num, 8, 8, char_texture, 16, 16);
}

/*
================
Draw_String
================
*/
void Draw_String (int x, int y, const char *str)
{
	while (*str)
	{
		Draw_Character (x, y, *str);
		str++;
		x += 8;
	}
}

/*
================
Draw_Alt_String
================
*/
void Draw_Alt_String (int x, int y, char *str)
{
	while (*str)
	{
		Draw_Character (x, y, (*str) | 0x80);
		str++;
		x += 8;
	}
}

void Draw_Crosshair(void)
{
	int x, y;
	extern vrect_t		scr_vrect;
	unsigned char *pColor;

	if (crosshair->value == 2)
	{
		x = scr_vrect.x + scr_vrect.width / 2 - 3 + cl_crossx->value; 
		y = scr_vrect.y + scr_vrect.height / 2 - 3 + cl_crossy->value;
		pColor = r_palette[crosshaircolor->integer];
		UI_DrawStretchPicWithColour(x - 4, y - 4, 16, 16, cs_texture, pColor);
	}
	else if (crosshair->value)
	{
		Draw_Character(scr_vrect.x + scr_vrect.width / 2 - 4 + cl_crossx->value, 
			scr_vrect.y + scr_vrect.height / 2 - 4 + cl_crossy->value, '+');
	}
}

/*
================
Draw_ConsoleBackground

================
*/
void Draw_ConsoleBackground(int lines)
{
	int y = (viddef.height * 3) >> 2;
	if (lines > y)
	{
		UI_DrawStretchPic(0, lines - viddef.height, viddef.width, viddef.height, conback);
	}
	else
	{
		UI_DrawStretchPic(0, lines - viddef.height, viddef.width, viddef.height, conback, (float)(1.2 * lines) / y);
	}

	y = lines - 14;
	if (!cls.download)
	{
		char ver[80];
		sprintf(ver, "JLQuakeWorld %s", JLQUAKE_VERSION_STRING);
		int x = viddef.width - (String::Length(ver) * 8 + 11);
		Draw_Alt_String(x, y, ver);
	}
}

//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen (void)
{
	UI_Fill(0 ,0, viddef.width, viddef.height, 0, 0, 0, 0.8);
}

#define NET_GRAPHHEIGHT 32

static void R_LineGraph(int x, int y, int h)
{
	int r, g, b;
	if (h == 10000)
	{
		// yellow
		r = 1;
		g = 1;
		b = 0;
	}
	else if (h == 9999)
	{
		// red
		r = 1;
		g = 0;
		b = 0;
	}
	else if (h == 9998)
	{
		// blue
		r = 0;
		g = 0;
		b = 1;
	}
	else
	{
		// white
		r = 1;
		g = 1;
		b = 1;
	}

	if (h > NET_GRAPHHEIGHT)
	{
		h = NET_GRAPHHEIGHT;
	}

	UI_Fill(8 + x, y + NET_GRAPHHEIGHT - h, 1, h, r, g, b, 1);
}

/*
==============
R_NetGraph
==============
*/
void R_NetGraph (void)
{
	int		x, y;
	char st[80];

	x = 0;
	int lost = CL_CalcNet();

	x =	-((viddef.width - 320)>>1);
	y = viddef.height - sb_lines - 24 - NET_GRAPHHEIGHT - 1;

	M_DrawTextBox (x, y, NET_TIMINGS/8, NET_GRAPHHEIGHT/8 + 1);
	y += 8;

	sprintf(st, "%3i%% packet loss", lost);
	Draw_String(8, y, st);
	y += 8;

	for (int a = 0; a < NET_TIMINGS; a++)
	{
		int i = (clc.netchan.outgoingSequence - a) & NET_TIMINGSMASK;
		R_LineGraph(NET_TIMINGS - 1 - a, y, packet_latency[i]);
	}
}
