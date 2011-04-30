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

#define GL_COLOR_INDEX8_EXT     0x80E5

byte		*draw_chars;				// 8*8 graphic characters
image_t*	draw_disc;
image_t*	draw_backtile;

image_t*	translate_texture;
image_t*	char_texture;

image_t*	conback;

int	scrap_uploads;

void Scrap_Upload (void)
{
	scrap_uploads++;
	GL_Bind(tr.scrapImage);
	R_UploadImage(scrap_texels, SCRAP_BLOCK_WIDTH, SCRAP_BLOCK_HEIGHT, false, false, false, &tr.scrapImage->internalFormat, &tr.scrapImage->uploadWidth, &tr.scrapImage->uploadHeight);
	scrap_dirty = false;
}

//=============================================================================
/* Support Routines */

byte		menuplyr_pixels[4096];

image_t* Draw_PicFromWad(char *name)
{
	byte* p = (byte*)W_GetLumpName (name);
	int width;
	int height;
	byte* pic32;
	R_LoadPICMem(p, &pic32, &width, &height);
	image_t* img;

	img = R_CreateImage(va("*wad:%s", name), pic32, width, height, false, false, GL_CLAMP, true);
	delete[] pic32;
	return img;
}


/*
================
Draw_CachePic
================
*/
image_t* Draw_CachePic (char *path)
{
	image_t*	pic;
	int			i;

	for (i=0 ; i<tr.numImages; i++)
		if (!QStr::Cmp(path, tr.images[i]->imgName))
			return tr.images[i];

	// HACK HACK HACK --- we need to keep the bytes for
	// the translatable player picture just for the menu
	// configuration dialog
	byte* TransPixels = NULL;
	if (!QStr::Cmp(path, "gfx/menuplyr.lmp"))
		TransPixels = menuplyr_pixels;

//
// load the pic from disk
//
	int width;
	int height;
	byte* pic32;
	R_LoadImage(path, &pic32, &width, &height, IMG8MODE_Normal, TransPixels);
	if (!pic32)
		Sys_Error ("Draw_CachePic: failed to load %s", path);

	pic = R_CreateImage(path, pic32, width, height, false, false, GL_CLAMP, false);
	delete[] pic32;
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

typedef struct
{
	char *name;
	int	minimize, maximize;
} glmode_t;

glmode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

/*
===============
Draw_TextureMode_f
===============
*/
void Draw_TextureMode_f (void)
{
	int		i;
	image_t	*glt;

	if (Cmd_Argc() == 1)
	{
		for (i=0 ; i< 6 ; i++)
			if (gl_filter_min == modes[i].minimize)
			{
				Con_Printf ("%s\n", modes[i].name);
				return;
			}
		Con_Printf ("current filter is unknown???\n");
		return;
	}

	for (i=0 ; i< 6 ; i++)
	{
		if (!QStr::ICmp(modes[i].name, Cmd_Argv(1) ) )
			break;
	}
	if (i == 6)
	{
		Con_Printf ("bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	for (i=0; i<tr.numImages; i++, glt++)
	{
		if (tr.images[i]->mipmap)
		{
			GL_Bind (tr.images[i]);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
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
	byte	*dest, *src;
	int		x, y;
	char	ver[40];
	int		start;
	int		f, fstep;


	Cmd_AddCommand ("gl_texturemode", &Draw_TextureMode_f);

	// load the console background and the charset
	// by hand, because we need to write the version
	// string into the background before turning
	// it into a texture
	draw_chars = (byte*)W_GetLumpName ("conchars");
	for (i=0 ; i<256*64 ; i++)
		if (draw_chars[i] == 0)
			draw_chars[i] = 255;	// proper transparent color

	// now turn them into textures
	char_texture = GL_LoadTexture8("charset", 128, 128, draw_chars, false, true);

	start = Hunk_LowMark();

	int cbwidth;
	int cbheight;
	byte* pic32;
	R_LoadImage("gfx/conback.lmp", &pic32, &cbwidth, &cbheight);
	if (!pic32)
		Sys_Error ("Couldn't load gfx/conback.lmp");

	// hack the version number directly into the pic
#if defined(__linux__)
	sprintf (ver, "(Linux %2.2f, gl %4.2f) %4.2f", (float)LINUX_VERSION, (float)GLQUAKE_VERSION, (float)VERSION);
#else
	sprintf (ver, "(gl %4.2f) %4.2f", (float)GLQUAKE_VERSION, (float)VERSION);
#endif
	dest = pic32 + (320*186 + 320 - 11 - 8*QStr::Length(ver)) * 4;
	y = QStr::Length(ver);
	for (x=0 ; x<y ; x++)
		Draw_CharToConback (ver[x], dest+(x<<5));

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	conback = GL_LoadTexture("***conback", cbwidth, cbheight, pic32, false);
	delete[] pic32;
	conback->sl = 0;
	conback->sh = 1;
	conback->tl = 0;
	conback->th = 1;
	conback->width = vid.conwidth;
	conback->height = vid.conheight;

	// free loaded console
	Hunk_FreeToLowMark(start);

	// save slots for scraps
	tr.scrapImage = R_CreateImage("*scrap", scrap_texels, SCRAP_BLOCK_WIDTH, SCRAP_BLOCK_HEIGHT, false, false, GL_CLAMP, false);

	//
	// get the other pics we need
	//
	draw_disc = Draw_PicFromWad ("disc");
	draw_backtile = Draw_PicFromWad ("backtile");
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
	byte			*dest;
	byte			*source;
	unsigned short	*pusdest;
	int				drawline;	
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
Draw_AlphaPic
=============
*/
void Draw_AlphaPic (int x, int y, image_t* pic, float alpha)
{
	byte			*dest, *source;
	unsigned short	*pusdest;
	int				v, u;

	if (scrap_dirty)
		Scrap_Upload ();
	qglDisable(GL_ALPHA_TEST);
	qglEnable (GL_BLEND);
//	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//	qglCullFace(GL_FRONT);
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
	qglEnable(GL_ALPHA_TEST);
	qglDisable (GL_BLEND);
}


/*
=============
Draw_Pic
=============
*/
void Draw_Pic (int x, int y, image_t* pic)
{
	byte			*dest, *source;
	unsigned short	*pusdest;
	int				v, u;

	if (scrap_dirty)
		Scrap_Upload ();
	qglColor4f (1,1,1,1);
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
}


/*
=============
Draw_TransPic
=============
*/
void Draw_TransPic (int x, int y, image_t* pic)
{
	byte	*dest, *source, tbyte;
	unsigned short	*pusdest;
	int				v, u;

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
		GL_Bind (translate_texture);
	}
	else
	{
		GL_Bind (translate_texture);

		R_UploadImage((byte*)trans, 64, 64, false, false, false, &translate_texture->internalFormat, &translate_texture->uploadWidth, &translate_texture->uploadHeight);
	}

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
	int y = (vid.height * 3) >> 2;

	if (lines > y)
		Draw_Pic(0, lines - vid.height, conback);
	else
		Draw_AlphaPic (0, lines - vid.height, conback, (float)(1.2 * lines)/y);
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
	qglEnable (GL_BLEND);
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
	qglDisable (GL_BLEND);

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

	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_CULL_FACE);
	qglDisable (GL_BLEND);
	qglEnable (GL_ALPHA_TEST);
//	qglDisable (GL_ALPHA_TEST);

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
		for (int i=0; i<tr.numImages; i++)
		{
			image_t* glt = tr.images[i];
			if (!QStr::Cmp(identifier, glt->imgName))
			{
				if (width != glt->width || height != glt->height)
					Sys_Error ("GL_LoadTexture: cache mismatch");
				return glt;
			}
		}
	}

	return R_CreateImage(identifier, (byte*)data, width, height, mipmap, mipmap, mipmap ? GL_REPEAT : GL_CLAMP, false);
}

image_t* GL_LoadTexture8(char *identifier, int width, int height, byte *data, qboolean mipmap, qboolean alpha)
{
	byte* pic32 = R_ConvertImage8To32(data, width, height, IMG8MODE_Normal);
	image_t* ret = GL_LoadTexture(identifier, width, height, pic32, mipmap);
	delete[] pic32;
	return ret;
}

int Draw_GetWidth(image_t* pic)
{
	return pic->width;
}

int Draw_GetHeight(image_t* pic)
{
	return pic->height;
}
