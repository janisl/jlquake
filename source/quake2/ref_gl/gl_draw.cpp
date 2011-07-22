/*
Copyright (C) 1997-2001 Id Software, Inc.

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

// draw.c

#include "gl_local.h"

image_t		*draw_chars;

/*
===============
Draw_InitLocal
===============
*/
void Draw_InitLocal (void)
{
	// load console characters (don't bilerp characters)
	draw_chars = R_FindImageFile("pics/conchars.pcx", false, false, GL_CLAMP);
	GL_Bind(draw_chars);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

/*
================
Draw_Char

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Char (int x, int y, int num)
{
	int				row, col;
	float			frow, fcol, size;

	num &= 255;
	
	if ( (num&127) == 32 )
		return;		// space

	if (y <= -8)
		return;			// totally off screen

	row = num>>4;
	col = num&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;

	GL_Bind (draw_chars);

	qglColor4f (1,1,1,1);
	DoQuad(x, y, fcol, frow, x + 8, y + 8, fcol + size, frow + size);
}

/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear(int x, int y, int w, int h, const char *pic)
{
	image_t* image = UI_RegisterPicRepeat(pic);
	if (!image)
	{
		ri.Con_Printf (PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	GL_Bind(image);
	qglColor4f (1,1,1,1);
	DoQuad(x, y, x / 64.0, y / 64.0, x + w, y + h, (x + w) / 64.0, (y + h) / 64.0);
}

/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_FillRgb(int x, int y, int w, int h, int r, int g, int b)
{
	qglDisable(GL_TEXTURE_2D);

	qglColor3f(r / 255.0, g / 255.0, b / 255.0);

	DoQuad(x, y, 0, 0, x + w, y + h, 0, 0);
	qglColor3f(1, 1, 1);
	qglEnable(GL_TEXTURE_2D);
}

/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int x, int y, int w, int h, int c)
{
	union
	{
		unsigned	c;
		byte		v[4];
	} color;

	if ( (unsigned)c > 255)
		ri.Sys_Error (ERR_FATAL, "Draw_Fill: bad color");

	color.c = d_8to24table[c];
	Draw_FillRgb(x, y, w, h, color.v[0], color.v[1], color.v[2]);
}

//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen (void)
{
	GL_State(GLS_DEFAULT | GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	qglDisable (GL_TEXTURE_2D);
	qglColor4f (0, 0, 0, 0.8);
	DoQuad(0, 0, 0, 0, glConfig.vidWidth, glConfig.vidHeight, 0, 0);
	qglColor4f (1,1,1,1);
	qglEnable (GL_TEXTURE_2D);
	GL_State(GLS_DEFAULT | GLS_DEPTHTEST_DISABLE | GLS_ATEST_GE_80);
}
