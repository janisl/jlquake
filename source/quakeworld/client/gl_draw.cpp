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
#include "glquake.h"

extern QCvar*	crosshair;
extern QCvar*	cl_crossx;
extern QCvar*	cl_crossy;
extern QCvar*	crosshaircolor;

byte		*draw_chars;				// 8*8 graphic characters
image_t		*draw_disc;
image_t		*draw_backtile;

image_t*	char_texture;
image_t*	cs_texture; // crosshair texture

static byte cs_data[64] = {
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff,
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};


image_t		*conback;

//=============================================================================
/* Support Routines */

void Draw_CharToConback (int num, byte *dest)
{
	int		row, col;
	byte	*source;
	int		drawline;
	int		x;

	row = num>>4;
	col = num&15;
	source = draw_chars + (row<<10) + (col<<3);

	drawline = 8;

	while (drawline--)
	{
		for (x=0 ; x<8 ; x++)
			if (source[x] != 255)
			{
				int p = (0x60 + source[x]) & 0xff;
				dest[x * 4 + 0] = r_palette[p][0];
				dest[x * 4 + 1] = r_palette[p][1];
				dest[x * 4 + 2] = r_palette[p][2];
				dest[x * 4 + 3] = r_palette[p][3];
			}
		source += 128;
		dest += 320 * 4;
	}

}

/*
===============
Draw_Init
===============
*/
void Draw_Init (void)
{
	int		i;
	byte	*dest;
	int		x;
	char	ver[40];
	int start;

	// load the console background and the charset
	// by hand, because we need to write the version
	// string into the background before turning
	// it into a texture
	draw_chars = (byte*)R_GetWadLumpByName ("conchars");
	for (i=0 ; i<256*64 ; i++)
		if (draw_chars[i] == 0)
			draw_chars[i] = 255;	// proper transparent color

	// now turn them into textures
	byte* draw_chars32 = R_ConvertImage8To32(draw_chars, 128, 128, IMG8MODE_Normal);
	char_texture = R_CreateImage("charset", draw_chars32, 128, 128, false, false, GL_CLAMP, false);
	GL_Bind(char_texture);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	delete[] draw_chars32;
//	Draw_CrosshairAdjust();
	byte* cs_data32 = R_ConvertImage8To32(cs_data, 8, 8, IMG8MODE_Normal);
	cs_texture = R_CreateImage("crosshair", cs_data32, 8, 8, false, false, GL_CLAMP, false);
	delete[] cs_data32;

	start = Hunk_LowMark ();

	int cbwidth;
	int cbheight;
	byte* pic32;
	R_LoadImage("gfx/conback.lmp", &pic32, &cbwidth, &cbheight);
	if (!pic32)
		Sys_Error ("Couldn't load gfx/conback.lmp");

	sprintf (ver, "%4.2f", VERSION);
	dest = pic32 + (320 + 320*186 - 11 - 8*QStr::Length(ver)) * 4;
	for (x=0 ; x<QStr::Length(ver) ; x++)
		Draw_CharToConback (ver[x], dest+(x<<5));

	conback = R_CreateImage("conback", pic32, cbwidth, cbheight, false, false, GL_CLAMP, false);
	delete[] pic32;

	// free loaded console
	Hunk_FreeToLowMark (start);

	//
	// get the other pics we need
	//
	draw_disc = R_PicFromWad ("disc");
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
	int				row, col;
	float			frow, fcol, size;

	num &= 255;

	if (num == 32)
		return;		// space

	if (y <= -8)
		return;			// totally off screen

	row = num>>4;
	col = num&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;

	GL_Bind (char_texture);

	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);

	qglColor4f (1,1,1,1);
	DoQuad(x, y, fcol, frow, x + 8, y + 8, fcol + size, frow + size);
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

	if (crosshair->value == 2) {
		x = scr_vrect.x + scr_vrect.width/2 - 3 + cl_crossx->value; 
		y = scr_vrect.y + scr_vrect.height/2 - 3 + cl_crossy->value;

		GL_TexEnv(GL_MODULATE);
		pColor = (unsigned char *) &d_8to24table[(byte) crosshaircolor->value];
		qglColor4ubv ( pColor );
		GL_Bind (cs_texture);

		GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);

		DoQuad(x - 4, y - 4, 0, 0, x + 12, y + 12, 1, 1);
		
		GL_TexEnv(GL_REPLACE);
	} else if (crosshair->value)
		Draw_Character (scr_vrect.x + scr_vrect.width/2-4 + cl_crossx->value, 
			scr_vrect.y + scr_vrect.height/2-4 + cl_crossy->value, 
			'+');
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

	// hack the version number directly into the pic
	y = lines - 14;
	if (!cls.download)
	{
		char ver[80];
#ifdef __linux__
		sprintf (ver, "LinuxGL (%4.2f) QuakeWorld", LINUX_VERSION);
#else
		sprintf (ver, "GL (%4.2f) QuakeWorld", GLQUAKE_VERSION);
#endif
		int x = viddef.width - (QStr::Length(ver) * 8 + 11) - (viddef.width * 8 / 320) * 7;
		for (int i = 0; i < QStr::Length(ver); i++)
		{
			Draw_Character(x + i * 8, y, ver[i] | 0x80);
		}
	}
}

/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int x, int y, int w, int h, int c)
{
	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);
	qglDisable (GL_TEXTURE_2D);
	qglColor3f (host_basepal[c*3]/255.0,
		host_basepal[c*3+1]/255.0,
		host_basepal[c*3+2]/255.0);

	DoQuad(x, y, 0, 0, x + w, y + h, 0, 0);
	qglColor3f (1,1,1);
	qglEnable (GL_TEXTURE_2D);
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
	DoQuad(0 ,0, 0, 0, viddef.width, viddef.height, 0, 0);
	qglColor4f (1,1,1,1);
	qglEnable (GL_TEXTURE_2D);
}

//=============================================================================

/*
================
Draw_BeginDisc

Draws the little blue disc in the corner of the screen.
Call before beginning any disc IO.
================
*/
void Draw_BeginDisc (void)
{
	if (!draw_disc)
		return;
	qglDrawBuffer  (GL_FRONT);
	UI_DrawPic (viddef.width - 24, 0, draw_disc);
	qglDrawBuffer  (GL_BACK);
}


/*
================
Draw_EndDisc

Erases the disc icon.
Call after completing any disc IO
================
*/
void Draw_EndDisc (void)
{
}
