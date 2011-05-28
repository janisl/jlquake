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

image_t*	translate_texture;
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

byte		menuplyr_pixels[4096];

/*
================
Draw_CachePic
================
*/
image_t* Draw_CachePic (char *path)
{
	// HACK HACK HACK --- we need to keep the bytes for
	// the translatable player picture just for the menu
	// configuration dialog
	byte* TransPixels = NULL;
	if (!QStr::Cmp(path, "gfx/menuplyr.lmp"))
		TransPixels = menuplyr_pixels;

	image_t* pic = R_FindImageFile(path, false, false, GL_CLAMP, false, IMG8MODE_Normal, TransPixels);
	if (!pic)
		Sys_Error ("Draw_CachePic: failed to load %s", path);
	return pic;
}


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

	R_InitFunctionTables();
	R_InitFogTable();
	R_NoiseInit();
	R_InitImages();

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
	conback->width = vid.conwidth;
	conback->height = vid.conheight;

	// free loaded console
	Hunk_FreeToLowMark (start);

	//
	// get the other pics we need
	//
	draw_disc = R_PicFromWad ("disc");
	draw_backtile = R_PicFromWad ("backtile");
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

	if (num == 32)
		return;		// space

	num &= 255;
	
	if (y <= -8)
		return;			// totally off screen

	row = num>>4;
	col = num&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;

	GL_Bind (char_texture);

	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);

	qglBegin (GL_QUADS);
	qglTexCoord2f (fcol, frow);
	qglVertex2f (x, y);
	qglTexCoord2f (fcol + size, frow);
	qglVertex2f (x+8, y);
	qglTexCoord2f (fcol + size, frow + size);
	qglVertex2f (x+8, y+8);
	qglTexCoord2f (fcol, frow + size);
	qglVertex2f (x, y+8);
	qglEnd ();
}

/*
================
Draw_String
================
*/
void Draw_String (int x, int y, char *str)
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

		qglBegin (GL_QUADS);
		qglTexCoord2f (0, 0);
		qglVertex2f (x - 4, y - 4);
		qglTexCoord2f (1, 0);
		qglVertex2f (x+12, y-4);
		qglTexCoord2f (1, 1);
		qglVertex2f (x+12, y+12);
		qglTexCoord2f (0, 1);
		qglVertex2f (x - 4, y+12);
		qglEnd ();
		
		GL_TexEnv(GL_REPLACE);
	} else if (crosshair->value)
		Draw_Character (scr_vrect.x + scr_vrect.width/2-4 + cl_crossx->value, 
			scr_vrect.y + scr_vrect.height/2-4 + cl_crossy->value, 
			'+');
}


/*
================
Draw_DebugChar

Draws a single character directly to the upper right corner of the screen.
This is for debugging lockups by drawing different chars in different parts
of the code.
================
*/
void Draw_DebugChar (char num)
{
}

/*
=============
Draw_Pic
=============
*/
void Draw_Pic (int x, int y, image_t* pic)
{
	if (scrap_dirty)
		R_ScrapUpload();
	qglColor4f (1,1,1,1);
	GL_Bind (pic);
	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);
	qglBegin (GL_QUADS);
	qglTexCoord2f (pic->sl, pic->tl);
	qglVertex2f (x, y);
	qglTexCoord2f (pic->sh, pic->tl);
	qglVertex2f (x+pic->width, y);
	qglTexCoord2f (pic->sh, pic->th);
	qglVertex2f (x+pic->width, y+pic->height);
	qglTexCoord2f (pic->sl, pic->th);
	qglVertex2f (x, y+pic->height);
	qglEnd ();
}

/*
=============
Draw_AlphaPic
=============
*/
void Draw_AlphaPic (int x, int y, image_t* pic, float alpha)
{
	if (scrap_dirty)
		R_ScrapUpload();
	GL_State(GLS_DEFAULT | GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	qglCullFace(GL_FRONT);
	qglColor4f (1,1,1,alpha);
	GL_Bind (pic);
	qglBegin (GL_QUADS);
	qglTexCoord2f (pic->sl, pic->tl);
	qglVertex2f (x, y);
	qglTexCoord2f (pic->sh, pic->tl);
	qglVertex2f (x+pic->width, y);
	qglTexCoord2f (pic->sh, pic->th);
	qglVertex2f (x+pic->width, y+pic->height);
	qglTexCoord2f (pic->sl, pic->th);
	qglVertex2f (x, y+pic->height);
	qglEnd ();
	qglColor4f (1,1,1,1);
	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);
}

void Draw_SubPic(int x, int y, image_t* pic, int srcx, int srcy, int width, int height)
{
	float newsl, newtl, newsh, newth;
	float oldglwidth, oldglheight;

	if (scrap_dirty)
		R_ScrapUpload();
	
	oldglwidth = pic->sh - pic->sl;
	oldglheight = pic->th - pic->tl;

	newsl = pic->sl + (srcx*oldglwidth)/pic->width;
	newsh = newsl + (width*oldglwidth)/pic->width;

	newtl = pic->tl + (srcy*oldglheight)/pic->height;
	newth = newtl + (height*oldglheight)/pic->height;
	
	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);
	
	qglColor4f (1,1,1,1);
	GL_Bind (pic);
	qglBegin (GL_QUADS);
	qglTexCoord2f (newsl, newtl);
	qglVertex2f (x, y);
	qglTexCoord2f (newsh, newtl);
	qglVertex2f (x+width, y);
	qglTexCoord2f (newsh, newth);
	qglVertex2f (x+width, y+height);
	qglTexCoord2f (newsl, newth);
	qglVertex2f (x, y+height);
	qglEnd ();
}

/*
=============
Draw_TransPic
=============
*/
void Draw_TransPic (int x, int y, image_t* pic)
{

	if (x < 0 || (unsigned)(x + pic->width) > vid.width || y < 0 ||
		 (unsigned)(y + pic->height) > vid.height)
	{
		Sys_Error ("Draw_TransPic: bad coordinates");
	}
		
	Draw_Pic (x, y, pic);
}


/*
=============
Draw_TransPicTranslate

Only used for the player color selection menu
=============
*/
void Draw_TransPicTranslate (int x, int y, image_t* pic, byte *translation)
{
	int				v, u, c;
	unsigned		trans[64*64], *dest;
	byte			*src;
	int				p;

	c = pic->width * pic->height;

	dest = trans;
	for (v=0 ; v<64 ; v++, dest += 64)
	{
		src = &menuplyr_pixels[ ((v*pic->height)>>6) *pic->width];
		for (u=0 ; u<64 ; u++)
		{
			p = src[(u*pic->width)>>6];
			if (p == 255)
				dest[u] = p;
			else
				dest[u] =  d_8to24table[translation[p]];
		}
	}

	if (!translate_texture)
	{
		// save a texture slot for translated picture
		translate_texture = R_CreateImage("*translate_pic", (byte*)trans, 64, 64, false, false, GL_CLAMP, false);
	}
	else
	{
		R_ReUploadImage(translate_texture, (byte*)trans);
	}

	GL_Bind (translate_texture);

	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);

	qglColor3f (1,1,1);
	qglBegin (GL_QUADS);
	qglTexCoord2f (0, 0);
	qglVertex2f (x, y);
	qglTexCoord2f (1, 0);
	qglVertex2f (x+pic->width, y);
	qglTexCoord2f (1, 1);
	qglVertex2f (x+pic->width, y+pic->height);
	qglTexCoord2f (0, 1);
	qglVertex2f (x, y+pic->height);
	qglEnd ();
}


/*
================
Draw_ConsoleBackground

================
*/
void Draw_ConsoleBackground (int lines)
{
	char ver[80];
	int x, i;
	int y;

	y = (vid.height * 3) >> 2;
	if (lines > y)
		Draw_Pic(0, lines-vid.height, conback);
	else
		Draw_AlphaPic (0, lines - vid.height, conback, (float)(1.2 * lines)/y);

	// hack the version number directly into the pic
//	y = lines-186;
	y = lines-14;
	if (!cls.download) {
#ifdef __linux__
		sprintf (ver, "LinuxGL (%4.2f) QuakeWorld", LINUX_VERSION);
#else
		sprintf (ver, "GL (%4.2f) QuakeWorld", GLQUAKE_VERSION);
#endif
		x = vid.conwidth - (QStr::Length(ver)*8 + 11) - (vid.conwidth*8/320)*7;
		for (i=0 ; i<QStr::Length(ver) ; i++)
			Draw_Character (x + i * 8, y, ver[i] | 0x80);
	}
}


/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear (int x, int y, int w, int h)
{
	qglColor3f (1,1,1);
	GL_Bind (draw_backtile);
	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);	
	qglBegin (GL_QUADS);
	qglTexCoord2f (x/64.0, y/64.0);
	qglVertex2f (x, y);
	qglTexCoord2f ( (x+w)/64.0, y/64.0);
	qglVertex2f (x+w, y);
	qglTexCoord2f ( (x+w)/64.0, (y+h)/64.0);
	qglVertex2f (x+w, y+h);
	qglTexCoord2f ( x/64.0, (y+h)/64.0 );
	qglVertex2f (x, y+h);
	qglEnd ();
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );	
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

	qglBegin (GL_QUADS);

	qglVertex2f (x,y);
	qglVertex2f (x+w, y);
	qglVertex2f (x+w, y+h);
	qglVertex2f (x, y+h);

	qglEnd ();
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
	qglBegin (GL_QUADS);

	qglVertex2f (0,0);
	qglVertex2f (vid.width, 0);
	qglVertex2f (vid.width, vid.height);
	qglVertex2f (0, vid.height);

	qglEnd ();
	qglColor4f (1,1,1,1);
	qglEnable (GL_TEXTURE_2D);

	Sbar_Changed();
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
	Draw_Pic (vid.width - 24, 0, draw_disc);
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

/*
================
GL_Set2D

Setup as if the screen was 320*200
================
*/
void GL_Set2D (void)
{
	qglViewport (glx, gly, glwidth, glheight);

	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity ();
	qglOrtho  (0, vid.width, vid.height, 0, -99999, 99999);

	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity ();

	qglDisable (GL_CULL_FACE);
	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);

	qglColor4f (1,1,1,1);
}

//====================================================================

/*
================
GL_LoadTexture
================
*/
image_t* GL_LoadTexture(char *identifier, int width, int height, byte *data, qboolean mipmap)
{
	// see if the texture is allready present
	if (identifier[0])
	{
		image_t* glt = R_FindImage(identifier);
		if (glt)
		{
			if (width != glt->width || height != glt->height)
			{
				Sys_Error ("GL_LoadTexture: cache mismatch");
			}
			return glt;
		}
	}

	return R_CreateImage(identifier, (byte*)data, width, height, mipmap, mipmap, mipmap ? GL_REPEAT : GL_CLAMP, false);
}

/****************************************/

int Draw_GetWidth(image_t* pic)
{
	return pic->width;
}

int Draw_GetHeight(image_t* pic)
{
	return pic->height;
}
