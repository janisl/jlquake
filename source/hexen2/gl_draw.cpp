
// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

/*
 * $Header: /H2 Mission Pack/gl_draw.c 2     2/26/98 3:09p Jmonroe $
 */

#include "quakedef.h"
#include "glquake.h"

#define MAX_DISC 18

byte		*draw_chars;				// 8*8 graphic characters
byte		*draw_smallchars;			// Small characters for status bar
byte		*draw_menufont; 			// Big Menu Font
image_t		*draw_disc[MAX_DISC] =
{
	NULL  // make the first one null for sure
};
image_t		*draw_backtile;

image_t*	translate_texture[NUM_CLASSES];
image_t*	char_texture;
image_t*	char_smalltexture;
image_t*	char_menufonttexture;

image_t		*conback;

//=============================================================================
/* Support Routines */

/*
 * Geometry for the player/skin selection screen image.
 */
#define PLAYER_PIC_WIDTH 68
#define PLAYER_PIC_HEIGHT 114
#define PLAYER_DEST_WIDTH 128
#define PLAYER_DEST_HEIGHT 128

byte		menuplyr_pixels[NUM_CLASSES][PLAYER_PIC_WIDTH*PLAYER_PIC_HEIGHT];

image_t *Draw_PicFromFile (char *name)
{
	return R_FindImageFile(name, false, false, GL_CLAMP);
}

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
	/* garymct */
	byte* TransPixels = NULL;
	if (!QStr::Cmp(path, "gfx/menu/netp1.lmp"))
		TransPixels = menuplyr_pixels[0];
	else if (!QStr::Cmp(path, "gfx/menu/netp2.lmp"))
		TransPixels = menuplyr_pixels[1];
	else if (!QStr::Cmp(path, "gfx/menu/netp3.lmp"))
		TransPixels = menuplyr_pixels[2];
	else if (!QStr::Cmp(path, "gfx/menu/netp4.lmp"))
		TransPixels = menuplyr_pixels[3];
	else if (!QStr::Cmp(path, "gfx/menu/netp5.lmp"))
		TransPixels = menuplyr_pixels[4];

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

	row = num>>5;
	col = num&31;
	source = draw_chars + (row<<11) + (col<<3);

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
		source += 256;
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
	int		x, y;
	char	ver[40];
	char temp[MAX_QPATH];

	R_InitBackEndData();
	R_CommonInit2();
	R_InitFreeType();

	// load the console background and the charset
	// by hand, because we need to write the version
	// string into the background before turning
	// it into a texture
	draw_chars = COM_LoadHunkFile ("gfx/menu/conchars.lmp");
	for (i=0 ; i<256*128 ; i++)
		if (draw_chars[i] == 0)
			draw_chars[i] = 255;	// proper transparent color

	byte* draw_chars32 = R_ConvertImage8To32(draw_chars, 256, 128, IMG8MODE_Normal);
	char_texture = R_CreateImage("charset", draw_chars32, 256, 128, false, false, GL_CLAMP, false);
	delete[] draw_chars32;
	GL_Bind(char_texture);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


	draw_smallchars = (byte*)R_GetWadLumpByName("tinyfont");
	for (i=0 ; i<128*32 ; i++)
		if (draw_smallchars[i] == 0)
			draw_smallchars[i] = 255;	// proper transparent color

	// now turn them into textures
	byte* draw_smallchars32 = R_ConvertImage8To32(draw_smallchars, 128, 32, IMG8MODE_Normal);
	char_smalltexture = R_CreateImage("smallcharset", draw_smallchars32, 128, 32, false, false, GL_CLAMP, false);
	delete[] draw_smallchars32;

	char_menufonttexture = R_FindImageFile("gfx/menu/bigfont2.lmp", false, false, GL_CLAMP, false, IMG8MODE_Holey);

	int cbwidth;
	int cbheight;
	byte* pic32;
	R_LoadImage("gfx/menu/conback.lmp", &pic32, &cbwidth, &cbheight);
	if (!pic32)
		Sys_Error ("Couldn't load gfx/menu/conback.lmp");

	// hack the version number directly into the pic

	dest = pic32 + (320 - 43 + 320*186) * 4;
	sprintf (ver, "%4.2f", HEXEN2_VERSION);

//	sprintf (ver, "(gl %4.2f) %4.2f", (float)GLQUAKE_VERSION, (float)VERSION);
//	dest = cb->data + 320*186 + 320 - 11 - 8*QStr::Length(ver);
	y = QStr::Length(ver);
	for (x=0 ; x<y ; x++)
		Draw_CharToConback (ver[x], dest+(x<<5));

	conback = R_CreateImage("conback", pic32, cbwidth, cbheight, false, false, GL_CLAMP, false);
	delete[] pic32;
	conback->width = vid.conwidth;
	conback->height = vid.conheight;

	//
	// get the other pics we need
	//
	for(i=MAX_DISC-1;i>=0;i--)
	{
		sprintf(temp,"gfx/menu/skull%d.lmp",i);
		draw_disc[i] = Draw_PicFromFile (temp);
	}

	draw_backtile = Draw_PicFromFile ("gfx/menu/backtile.lmp");
}



/*
================
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Character (int x, int y, unsigned int num)
{
	byte			*dest;
	byte			*source;
	unsigned short	*pusdest;
	int				drawline;
	int				row, col;
	float			frow, fcol, xsize,ysize;

	if (num == 32)
		return; 	// space

	num &= 511;

	if (y <= -8)
		return; 		// totally off screen

	row = num>>5;
	col = num&31;

	xsize = 0.03125;
	ysize = 0.0625;
	fcol = col*xsize;
	frow = row*ysize;

	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);

	GL_Bind (char_texture);

	qglBegin (GL_QUADS);
	qglTexCoord2f (fcol, frow);
	qglVertex2f (x, y);
	qglTexCoord2f (fcol + xsize, frow);
	qglVertex2f (x+8, y);
	qglTexCoord2f (fcol + xsize, frow + ysize);
	qglVertex2f (x+8, y+8);
	qglTexCoord2f (fcol, frow + ysize);
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


//==========================================================================
//
// Draw_SmallCharacter
//
// Draws a small character that is clipped at the bottom edge of the
// screen.
//
//==========================================================================
void Draw_SmallCharacter (int x, int y, int num)
{
	byte			*dest;
	byte			*source;
	unsigned short	*pusdest;
	int				drawline;
	int				row, col;
	float			frow, fcol, xsize,ysize;

	if(num < 32)
	{
		num = 0;
	}
	else if(num >= 'a' && num <= 'z')
	{
		num -= 64;
	}
	else if(num > '_')
	{
		num = 0;
	}
	else
	{
		num -= 32;
	}

	if (num == 0) return;

	if (y <= -8)
		return; 		// totally off screen

	if(y >= vid.height)
	{ // Totally off screen
		return;
	}

	row = num>>4;
	col = num&15;

	xsize = 0.0625;
	ysize = 0.25;
	fcol = col*xsize;
	frow = row*ysize;

	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);

	GL_Bind (char_smalltexture);

	qglBegin (GL_QUADS);
	qglTexCoord2f (fcol, frow);
	qglVertex2f (x, y);
	qglTexCoord2f (fcol + xsize, frow);
	qglVertex2f (x+8, y);
	qglTexCoord2f (fcol + xsize, frow + ysize);
	qglVertex2f (x+8, y+8);
	qglTexCoord2f (fcol, frow + ysize);
	qglVertex2f (x, y+8);
	qglEnd ();
}

//==========================================================================
//
// Draw_SmallString
//
//==========================================================================
void Draw_SmallString(int x, int y, char *str)
{
	while (*str)
	{
		Draw_SmallCharacter (x, y, *str);
		str++;
		x += 6;
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
Draw_Pic
=============
*/
void Draw_Pic (int x, int y, image_t* pic)
{
//	byte			*dest, *source;
//	unsigned short	*pusdest;
//	int				v, u;

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

void Draw_PicCropped(int x, int y, image_t* pic)
{
	int height;
	float th,tl;

	if((x < 0) || (x+pic->width > vid.width))
	{
		Sys_Error("Draw_PicCropped: bad coordinates");
	}

	if (y >= (int)vid.height || y+pic->height < 0)
	{ // Totally off screen
		return;
	}

	if (scrap_dirty)
		R_ScrapUpload();

	// rjr tl/th need to be computed based upon pic->tl and pic->th
	//     cuz the piece may come from the scrap
	if(y+pic->height > vid.height)
	{
		height = vid.height-y;
		tl = 0;
		th = (height-0.01)/pic->height;
	}
	else if (y < 0)
	{
		height = pic->height+y;
		y = -y;
		tl = (y-0.01)/pic->height;
		th = (pic->height-0.01)/pic->height;
		y = 0;
	}
	else
	{
		height = pic->height;
		tl = pic->tl;
		th = pic->th;//(height-0.01)/pic->height;
	}

	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);

	qglColor4f (1,1,1,1);
	GL_Bind (pic);
	qglBegin (GL_QUADS);
	qglTexCoord2f (pic->sl, tl);
	qglVertex2f (x, y);
	qglTexCoord2f (pic->sh, tl);
	qglVertex2f (x+pic->width, y);
	qglTexCoord2f (pic->sh, th);
	qglVertex2f (x+pic->width, y+height);
	qglTexCoord2f (pic->sl, th);
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
//	byte	*dest, *source, tbyte;
//	unsigned short	*pusdest;
//	int				v, u;

	if (x < 0 || (unsigned)(x + pic->width) > vid.width || y < 0 ||
		 (unsigned)(y + pic->height) > vid.height)
	{
		Sys_Error ("Draw_TransPic: bad coordinates");
	}

	Draw_Pic (x, y, pic);
}

void Draw_TransPicCropped(int x, int y, image_t* pic)
{
	Draw_PicCropped (x, y, pic);
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
	unsigned		trans[PLAYER_DEST_WIDTH * PLAYER_DEST_HEIGHT], *dest;
	byte			*src;
	int				p;

	extern int setup_class;

	c = pic->width * pic->height;

	dest = trans;
	for (v=0 ; v<64 ; v++, dest += 64)
	{
		src = &menuplyr_pixels[setup_class-1][ ((v*pic->height)>>6) *pic->width];
		for (u=0 ; u<64 ; u++)
		{
			p = src[(u*pic->width)>>6];
			if (p == 255)
				dest[u] = p;
			else
				dest[u] =  d_8to24table[translation[p]];
		}
	}

	{
		int x, y;
		
		for( x = 0; x < PLAYER_PIC_WIDTH; x++ )
			for( y = 0; y < PLAYER_PIC_HEIGHT; y++ )
			{
				trans[y * PLAYER_DEST_WIDTH + x] = d_8to24table[translation[menuplyr_pixels[setup_class-1][y * PLAYER_PIC_WIDTH + x]]];
			}
	}

	// save a texture slot for translated picture
	if (!translate_texture[setup_class-1])
	{
		translate_texture[setup_class-1] = R_CreateImage("*translate_pic", (byte*)trans, PLAYER_DEST_WIDTH, PLAYER_DEST_HEIGHT, false, false, GL_CLAMP, false);
	}
	else
	{
		R_ReUploadImage(translate_texture[setup_class-1], (byte*)trans);
	}

	GL_Bind(translate_texture[setup_class-1]);

	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);

	qglColor3f (1,1,1);
	qglBegin (GL_QUADS);
	qglTexCoord2f (0, 0);
	qglVertex2f (x, y);
	qglTexCoord2f (( float )PLAYER_PIC_WIDTH / PLAYER_DEST_WIDTH, 0);
	qglVertex2f (x+pic->width, y);
	qglTexCoord2f (( float )PLAYER_PIC_WIDTH / PLAYER_DEST_WIDTH, ( float )PLAYER_PIC_HEIGHT / PLAYER_DEST_HEIGHT);
	qglVertex2f (x+pic->width, y+pic->height);
	qglTexCoord2f (0, ( float )PLAYER_PIC_HEIGHT / PLAYER_DEST_HEIGHT);
	qglVertex2f (x, y+pic->height);
	qglEnd ();
}


int M_DrawBigCharacter (int x, int y, int num, int numNext)
{
	byte			*dest;
	byte			*source;
	unsigned short	*pusdest;
	int				drawline;
	int				row, col;
	float			frow, fcol, xsize,ysize;
	int				add;

	if (num == ' ') return 32;

	if (num == '/') num = 26;
	else num -= 65;

	if (num < 0 || num >= 27)  // only a-z and /
		return 0;

	if (numNext == '/') numNext = 26;
	else numNext -= 65;

	row = num/8;
	col = num%8;

	xsize = 0.125;
	ysize = 0.25;
	fcol = col*xsize;
	frow = row*ysize;

	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);

	GL_Bind (char_menufonttexture);

	qglBegin (GL_QUADS);
	qglTexCoord2f (fcol, frow);
	qglVertex2f (x, y);
	qglTexCoord2f (fcol + xsize, frow);
	qglVertex2f (x+20, y);
	qglTexCoord2f (fcol + xsize, frow + ysize);
	qglVertex2f (x+20, y+20);
	qglTexCoord2f (fcol, frow + ysize);
	qglVertex2f (x, y+20);
	qglEnd ();

	if (numNext < 0 || numNext >= 27) return 0;

	add = 0;
	if (num == (int)'C'-65 && numNext == (int)'P'-65)
		add = 3;

	return BigCharWidth[num][numNext] + add;
}

/*
================
Draw_ConsoleBackground

================
*/
void Draw_ConsoleBackground (int lines)
{
	Draw_Pic (0, lines-vid.height, conback);
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
	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);

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
	int bx,by,ex,ey;
	int c;

	GL_State(GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	qglDisable (GL_TEXTURE_2D);

//	qglColor4f (248.0/255.0, 220.0/255.0, 120.0/255.0, 0.2);
	qglColor4f (208.0/255.0, 180.0/255.0, 80.0/255.0, 0.2);
	qglBegin (GL_QUADS);
	qglVertex2f (0,0);
	qglVertex2f (vid.width, 0);
	qglVertex2f (vid.width, vid.height);
	qglVertex2f (0, vid.height);
	qglEnd ();

	qglColor4f (208.0/255.0, 180.0/255.0, 80.0/255.0, 0.035);
	for(c=0;c<40;c++)
	{
		bx = rand() % vid.width-20;
		by = rand() % vid.height-20;
		ex = bx + (rand() % 40) + 20;
		ey = by + (rand() % 40) + 20;
		if (bx < 0) bx = 0;
		if (by < 0) by = 0;
		if (ex > vid.width) ex = vid.width;
		if (ey > vid.height) ey = vid.height;

		qglBegin (GL_QUADS);
		qglVertex2f (bx,by);
		qglVertex2f (ex, by);
		qglVertex2f (ex, ey);
		qglVertex2f (bx, ey);
		qglEnd ();
	}

	qglColor4f (1,1,1,1);
	qglEnable (GL_TEXTURE_2D);

	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80);

	SB_Changed();
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
	static int index = 0;

	if (!draw_disc[index] || loading_stage) return;

	index++;
	if (index >= MAX_DISC) index = 0;

	qglDrawBuffer  (GL_FRONT);

	Draw_Pic (vid.width - 28, 0, draw_disc[index]);

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
	qglViewport (0, 0, glConfig.vidWidth, glConfig.vidHeight);

	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity ();
//	qglOrtho  (0, 320, 200, 0, -99999, 99999);
	qglOrtho  (0, vid.width, vid.height, 0, -99999, 99999);

	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity ();

	qglDisable (GL_CULL_FACE);
	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);

	qglColor4f (1,1,1,1);
}

int Draw_GetWidth(image_t* pic)
{
	return pic->width;
}

int Draw_GetHeight(image_t* pic)
{
	return pic->height;
}
