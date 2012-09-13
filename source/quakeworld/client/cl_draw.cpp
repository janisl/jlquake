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

// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

#include "quakedef.h"

extern Cvar* cl_crossx;
extern Cvar* cl_crossy;
extern Cvar* crosshaircolor;

image_t* cs_texture;	// crosshair texture

/*
===============
Draw_Init
===============
*/
void Draw_Init(void)
{
	char_texture = R_LoadRawFontImageFromWad("conchars", 128, 128);

	cs_texture = R_CreateCrosshairImage();

	conback = R_CachePic("gfx/conback.lmp");

	//
	// get the other pics we need
	//
	draw_backtile = R_PicFromWadRepeat("backtile");
}

void Draw_Crosshair(void)
{
	int x, y;
	extern vrect_t scr_vrect;
	unsigned char* pColor;

	if (crosshair->value == 2)
	{
		x = scr_vrect.x + scr_vrect.width / 2 - 3 + cl_crossx->value;
		y = scr_vrect.y + scr_vrect.height / 2 - 3 + cl_crossy->value;
		pColor = r_palette[crosshaircolor->integer];
		UI_DrawStretchPicWithColour(x - 4, y - 4, 16, 16, cs_texture, pColor);
	}
	else if (crosshair->value)
	{
		UI_DrawChar(scr_vrect.x + scr_vrect.width / 2 - 4 + cl_crossx->value,
			scr_vrect.y + scr_vrect.height / 2 - 4 + cl_crossy->value, '+');
	}
}

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen(void)
{
	UI_Fill(0,0, viddef.width, viddef.height, 0, 0, 0, 0.8);
}

#define NET_GRAPHHEIGHT 32

static void R_LineGraph(int h)
{
	int colour;
	if (h == 10000)
	{
		// yellow
		colour = 0x00ffff;
	}
	else if (h == 9999)
	{
		// red
		colour = 0x0000ff;
	}
	else if (h == 9998)
	{
		// blue
		colour = 0xff0000;
	}
	else
	{
		// white
		colour = 0xffffff;
	}

	if (h > NET_GRAPHHEIGHT)
	{
		h = NET_GRAPHHEIGHT;
	}

	SCR_DebugGraph(h, colour);
}

/*
==============
R_NetGraph
==============
*/
void R_NetGraph(void)
{
	static int lastOutgoingSequence = 0;

	CL_CalcNet();

	for (int a = lastOutgoingSequence + 1; a <= clc.netchan.outgoingSequence; a++)
	{
		int i = a & NET_TIMINGSMASK;
		R_LineGraph(packet_latency[i]);
	}
	lastOutgoingSequence = clc.netchan.outgoingSequence;
	SCR_DrawDebugGraph();
}
