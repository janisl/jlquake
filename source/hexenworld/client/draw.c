
// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

#include "quakedef.h"
#include "r_shared.h"

typedef struct {
	vrect_t	rect;
	int		width;
	int		height;
	byte	*ptexbytes;
	int		rowbytes;
} rectdesc_t;

static rectdesc_t	r_rectdesc;

byte *draw_smallchars; // Small characters for status bar

byte		*draw_chars;				// 8*8 graphic characters
qpic_t		*draw_disc[MAX_DISC] = 
{
	NULL  // make the first one null for sure
};
qpic_t		*draw_backtile;

int	trans_level = 0;

//=============================================================================
/* Support Routines */

typedef struct cachepic_s
{
	char		name[MAX_QPATH];
	cache_user_t	cache;
} cachepic_t;

#define	MAX_CACHED_PICS		256
cachepic_t	menu_cachepics[MAX_CACHED_PICS];
int			menu_numcachepics;


qpic_t	*Draw_PicFromWad (char *name)
{
	return W_GetLumpName (name);
}

/*
================
Draw_CachePic
================
*/
qpic_t	*Draw_CachePic (char *path)
{
	cachepic_t	*pic;
	int			i;
	qpic_t		*dat;
	
	for (pic=menu_cachepics, i=0 ; i<menu_numcachepics ; pic++, i++)
		if (!strcmp (path, pic->name))
			break;

	if (i == menu_numcachepics)
	{
		if (menu_numcachepics == MAX_CACHED_PICS)
			Sys_Error ("menu_numcachepics == MAX_CACHED_PICS");
		menu_numcachepics++;
		strcpy (pic->name, path);
	}

	dat = Cache_Check (&pic->cache);

	if (dat)
		return dat;

//
// load the pic from disk
//
	COM_LoadCacheFile (path, &pic->cache);
	
	dat = (qpic_t *)pic->cache.data;
	if (!dat)
	{
		Sys_Error ("Draw_CachePic: failed to load %s", path);
	}

	SwapPic (dat);

	return dat;
}



/*
===============
Draw_Init
===============
*/
void Draw_Init (void)
{
	int		i;
	char	temp[MAX_QPATH];

	draw_smallchars = W_GetLumpName("tinyfont");
	draw_chars = COM_LoadHunkFile ("gfx/menu/conchars.lmp");

	// Do this backwards so we don't try and draw the 
	// skull as we are loading

	for(i=MAX_DISC-1;i>=0;i--)
	{
		sprintf(temp,"gfx/menu/skull%d.lmp",i);
		draw_disc[i] = (qpic_t *)COM_LoadHunkFile (temp);
	}

	draw_backtile = (qpic_t	*)COM_LoadHunkFile ("gfx/menu/backtile.lmp");


	r_rectdesc.width = draw_backtile->width;
	r_rectdesc.height = draw_backtile->height;
	r_rectdesc.ptexbytes = draw_backtile->data;
	r_rectdesc.rowbytes = draw_backtile->width;
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

	num &= 511;
	
	if (y <= -8)
		return;			// totally off screen

	if (y > vid.height - 8 || x < 0 || x > vid.width - 8)
		return;
	if (num < 0 || num > 511)
		return;

	row = num>>5;
	col = num&31;
	source = draw_chars + (row<<11) + (col<<3);

	if (y < 0)
	{	// clipped
		drawline = 8 + y;
		source -= 256*y;
		y = 0;
	}
	else
		drawline = 8;


	if (r_pixbytes == 1)
	{
		dest = vid.conbuffer + y*vid.conrowbytes + x;
	
		while (drawline--)
		{
			switch(trans_level)
			{
				case 0:
					if (source[0])
						dest[0] = source[0];
					if (source[1])
						dest[1] = source[1];
					if (source[2])
						dest[2] = source[2];
					if (source[3])
						dest[3] = source[3];
					if (source[4])
						dest[4] = source[4];
					if (source[5])
						dest[5] = source[5];
					if (source[6])
						dest[6] = source[6];
					if (source[7])
						dest[7] = source[7];
					break;
				case 1:
					if (source[0])
						dest[0] = mainTransTable[(((unsigned)dest[0])<<8) + source[0]];
					if (source[1])
						dest[1] = mainTransTable[(((unsigned)dest[1])<<8) + source[1]];
					if (source[2])
						dest[2] = mainTransTable[(((unsigned)dest[2])<<8) + source[2]];
					if (source[3])
						dest[3] = mainTransTable[(((unsigned)dest[3])<<8) + source[3]];
					if (source[4])
						dest[4] = mainTransTable[(((unsigned)dest[4])<<8) + source[4]];
					if (source[5])
						dest[5] = mainTransTable[(((unsigned)dest[5])<<8) + source[5]];
					if (source[6])
						dest[6] = mainTransTable[(((unsigned)dest[6])<<8) + source[6]];
					if (source[7])
						dest[7] = mainTransTable[(((unsigned)dest[7])<<8) + source[7]];
					break;
				case 2:
					if (source[0])
						dest[0] = mainTransTable[(((unsigned)source[0])<<8) + dest[0]];
					if (source[1])
						dest[1] = mainTransTable[(((unsigned)source[1])<<8) + dest[1]];
					if (source[2])
						dest[2] = mainTransTable[(((unsigned)source[2])<<8) + dest[2]];
					if (source[3])
						dest[3] = mainTransTable[(((unsigned)source[3])<<8) + dest[3]];
					if (source[4])
						dest[4] = mainTransTable[(((unsigned)source[4])<<8) + dest[4]];
					if (source[5])
						dest[5] = mainTransTable[(((unsigned)source[5])<<8) + dest[5]];
					if (source[6])
						dest[6] = mainTransTable[(((unsigned)source[6])<<8) + dest[6]];
					if (source[7])
						dest[7] = mainTransTable[(((unsigned)source[7])<<8) + dest[7]];
					break;
			}

			source += 256;
			dest += vid.conrowbytes;
		}
	}
	else
	{
	// FIXME: pre-expand to native format?
		pusdest = (unsigned short *)
				((byte *)vid.conbuffer + y*vid.conrowbytes + (x<<1));

		while (drawline--)
		{
			if (source[0])
				pusdest[0] = d_8to16table[source[0]];
			if (source[1])
				pusdest[1] = d_8to16table[source[1]];
			if (source[2])
				pusdest[2] = d_8to16table[source[2]];
			if (source[3])
				pusdest[3] = d_8to16table[source[3]];
			if (source[4])
				pusdest[4] = d_8to16table[source[4]];
			if (source[5])
				pusdest[5] = d_8to16table[source[5]];
			if (source[6])
				pusdest[6] = d_8to16table[source[6]];
			if (source[7])
				pusdest[7] = d_8to16table[source[7]];

			source += 256;
			pusdest += (vid.conrowbytes >> 1);
		}
	}
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

void Draw_RedString (int x, int y, char *str)
{
	while (*str)
	{
		Draw_Character (x, y, ((unsigned char)(*str))+256);
		str++;
		x += 8;
	}
}

void Draw_Pixel(int x, int y, byte color)
{
	byte			*dest;
	unsigned short	*pusdest;

	if (r_pixbytes == 1)
	{
		dest = vid.conbuffer + y*vid.conrowbytes + x;
		*dest = color;
	}
	else
	{
	// FIXME: pre-expand to native format?
		pusdest = (unsigned short *)
				((byte *)vid.conbuffer + y*vid.conrowbytes + (x<<1));
		*pusdest = d_8to16table[color];
	}
}

void Draw_Crosshair(void)
{
	int x, y;
	extern cvar_t crosshair, cl_crossx, cl_crossy;
	extern vrect_t		scr_vrect;

	if (crosshair.value == 2) {
		x = scr_vrect.x + scr_vrect.width/2 + cl_crossx.value; 
		y = scr_vrect.y + scr_vrect.height/2 + cl_crossy.value;
		Draw_Pixel(x - 1, y, 0x4f);
		Draw_Pixel(x - 3, y, 0x4f);
		Draw_Pixel(x + 1, y, 0x4f);
		Draw_Pixel(x + 3, y, 0x4f);
		Draw_Pixel(x, y - 1, 0x4f);
		Draw_Pixel(x, y - 3, 0x4f);
		Draw_Pixel(x, y + 1, 0x4f);
		Draw_Pixel(x, y + 3, 0x4f);
	} else if (crosshair.value)
		Draw_Character (
			scr_vrect.x + scr_vrect.width/2-4 + cl_crossx.value, 
			scr_vrect.y + scr_vrect.height/2-4 + cl_crossy.value, 
			'+');
}

//==========================================================================
//
// Draw_SmallCharacter
//
// Draws a small character that is clipped at the bottom edge of the
// screen.
//
//==========================================================================

void Draw_SmallCharacter(int x, int y, int num)
{
	byte *dest;
	byte *source;
	unsigned short *pusdest;
	int height;
	int row, col;

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

	if(y >= vid.height)
	{ // Totally off screen
		return;
	}

#ifdef PARANOID
	if((y < 0) || (x < 0) || (x+8 > vid.width))
	{
		Sys_Error("Bad Draw_SmallCharacter: (%d, %d)", x, y);
	}
#endif

	if(y+5 > vid.height)
	{
		height = vid.height-y;
	}
	else
	{
		height = 5;
	}

	row = num>>4;
	col = num&15;
	source = draw_smallchars+(row<<10)+(col<<3);

	if(r_pixbytes == 1)
	{
		dest = vid.buffer+y*vid.rowbytes+x;
		while(height--)
		{
			switch(trans_level)
			{
				case 0:
					if (source[0])
						dest[0] = source[0];
					if (source[1])
						dest[1] = source[1];
					if (source[2])
						dest[2] = source[2];
					if (source[3])
						dest[3] = source[3];
					if (source[4])
						dest[4] = source[4];
					if (source[5])
						dest[5] = source[5];
					if (source[6])
						dest[6] = source[6];
					if (source[7])
						dest[7] = source[7];
					break;
				case 1:
					if (source[0])
						dest[0] = mainTransTable[(((unsigned)dest[0])<<8) + source[0]];
					if (source[1])
						dest[1] = mainTransTable[(((unsigned)dest[1])<<8) + source[1]];
					if (source[2])
						dest[2] = mainTransTable[(((unsigned)dest[2])<<8) + source[2]];
					if (source[3])
						dest[3] = mainTransTable[(((unsigned)dest[3])<<8) + source[3]];
					if (source[4])
						dest[4] = mainTransTable[(((unsigned)dest[4])<<8) + source[4]];
					if (source[5])
						dest[5] = mainTransTable[(((unsigned)dest[5])<<8) + source[5]];
					if (source[6])
						dest[6] = mainTransTable[(((unsigned)dest[6])<<8) + source[6]];
					if (source[7])
						dest[7] = mainTransTable[(((unsigned)dest[7])<<8) + source[7]];
					break;
				case 2:
					if (source[0])
						dest[0] = mainTransTable[(((unsigned)source[0])<<8) + dest[0]];
					if (source[1])
						dest[1] = mainTransTable[(((unsigned)source[1])<<8) + dest[1]];
					if (source[2])
						dest[2] = mainTransTable[(((unsigned)source[2])<<8) + dest[2]];
					if (source[3])
						dest[3] = mainTransTable[(((unsigned)source[3])<<8) + dest[3]];
					if (source[4])
						dest[4] = mainTransTable[(((unsigned)source[4])<<8) + dest[4]];
					if (source[5])
						dest[5] = mainTransTable[(((unsigned)source[5])<<8) + dest[5]];
					if (source[6])
						dest[6] = mainTransTable[(((unsigned)source[6])<<8) + dest[6]];
					if (source[7])
						dest[7] = mainTransTable[(((unsigned)source[7])<<8) + dest[7]];
					break;
			}

			source += 128;
			dest += vid.conrowbytes;
		}
	}
	else
	{ // FIXME: pre-expand to native format?
		pusdest = (unsigned short *)
			((byte *)vid.buffer+y*vid.rowbytes+(x<<1));
		while(height--)
		{
			if(source[0])
				pusdest[0] = d_8to16table[source[0]];
			if(source[1])
				pusdest[1] = d_8to16table[source[1]];
			if(source[2])
				pusdest[2] = d_8to16table[source[2]];
			if(source[3])
				pusdest[3] = d_8to16table[source[3]];
			if(source[4])
				pusdest[4] = d_8to16table[source[4]];
			if(source[5])
				pusdest[5] = d_8to16table[source[5]];
			if(source[6])
				pusdest[6] = d_8to16table[source[6]];
			if(source[7])
				pusdest[7] = d_8to16table[source[7]];
			source += 128;
			pusdest += (vid.conrowbytes>>1);
		}
	}
}

//==========================================================================
//
// Draw_SmallString
//
//==========================================================================

void Draw_SmallString(int x, int y, char *str)
{
	while(*str)
	{
		Draw_SmallCharacter(x, y, *str);
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
	byte			*dest;
	byte			*source;
	int				drawline;	
	extern byte		*draw_chars;
	int				row, col;

	if (!vid.direct)
		return;		// don't have direct FB access, so no debugchars...

	drawline = 8;

	row = num>>5;
	col = num&31;
	source = draw_chars + (row<<10) + (col<<3);

	dest = vid.direct + 312;

	while (drawline--)
	{
		dest[0] = source[0];
		dest[1] = source[1];
		dest[2] = source[2];
		dest[3] = source[3];
		dest[4] = source[4];
		dest[5] = source[5];
		dest[6] = source[6];
		dest[7] = source[7];
		source += 256;
		dest += 320;
	}
}

/*
=============
Draw_Pic
=============
*/
void Draw_Pic (int x, int y, qpic_t *pic)
{
	byte			*dest, *source;
	unsigned short	*pusdest;
	int				v, u;

	if ((x < 0) ||
		(x + pic->width > vid.width) ||
		(y < 0) ||
		(y + pic->height > vid.height))
	{
		Sys_Error ("Draw_Pic: bad coordinates");
	}

	source = pic->data;

	if (r_pixbytes == 1)
	{
		dest = vid.buffer + y * vid.rowbytes + x;

		for (v=0 ; v<pic->height ; v++)
		{
			memcpy (dest, source, pic->width);
			dest += vid.rowbytes;
			source += pic->width;
		}
	}
	else
	{
	// FIXME: pretranslate at load time?
		pusdest = (unsigned short *)vid.buffer + y * (vid.rowbytes >> 1) + x;

		for (v=0 ; v<pic->height ; v++)
		{
			for (u=0 ; u<pic->width ; u++)
			{
				pusdest[u] = d_8to16table[source[u]];
			}

			pusdest += vid.rowbytes >> 1;
			source += pic->width;
		}
	}
}


//==========================================================================
//
// Draw_PicCropped
//
// Draws a qpic_t that is clipped at the bottom/top edges of the screen.
//
//==========================================================================

void Draw_PicCropped(int x, int y, qpic_t *pic)
{
	byte *dest, *source;
	unsigned short *pusdest;
	int v, u;
	int height;

	if((x < 0) || (x+pic->width > vid.width))
	{
		Sys_Error("Draw_PicCropped: bad coordinates");
	}

	if (y >= (int)vid.height || y+pic->height < 0)
	{ // Totally off screen
		return;
	}

	if(y+pic->height > vid.height)
	{
		height = vid.height-y;
	}
	else if (y < 0) 
	{
		height = pic->height+y;
	}
	else
	{
		height = pic->height;
	}

	source = pic->data;
	if (y < 0) 
	{
		source += (pic->width * (-y));
		y = 0;
	}

	if(r_pixbytes == 1)
	{
		dest = vid.buffer+y*vid.rowbytes+x;

		switch(trans_level)
		{
			case 0:
				for(v = 0; v < height; v++)
				{
					memcpy(dest, source, pic->width);
					dest += vid.rowbytes;
					source += pic->width;
				}
				break;
			case 1:
				for(v = 0; v < height; v++)
				{
					for(u = 0;u < pic->width;u++,source++)
					{
						dest[u] = mainTransTable[(((unsigned)dest[u])<<8) + (*source)];
					}
					dest += vid.rowbytes;
				}
				break;
			case 2:
				for(v = 0; v < height; v++)
				{
					for(u = 0;u < pic->width;u++,source++)
					{
						dest[u] = mainTransTable[(((unsigned)(*source))<<8) + dest[u]];
					}
					dest += vid.rowbytes;
				}
				break;
		}
	}
	else
	{ // FIXME: pretranslate at load time?
		pusdest = (unsigned short *)vid.buffer+y*(vid.rowbytes>>1)+x;
		for(v = 0; v < height; v++)
		{
			for(u = 0; u < pic->width; u++)
			{
				pusdest[u] = d_8to16table[source[u]];
			}
			pusdest += vid.rowbytes>>1;
			source += pic->width;
		}
	}
}

//==========================================================================
//
// Draw_SubPicCropped
//
// Draws a qpic_t that is clipped at the bottom/top edges of the screen.
//
//==========================================================================

void Draw_SubPicCropped(int x, int y, int h, qpic_t *pic)
{
	byte *dest, *source;
	unsigned short *pusdest;
	int v, u;
	int height;

	if((x < 0) || (x+pic->width > vid.width))
	{
		Sys_Error("Draw_PicCropped: bad coordinates");
	}

	if (y >= (int)vid.height || y+h < 0)
	{ // Totally off screen
		return;
	}

	if(y+pic->height > vid.height)
	{
		height = vid.height-y;
	}
	else if (y < 0) 
	{
		height = pic->height+y;
	}
	else
	{
		height = pic->height;
	}

	if (height > h)
	{
		height = h;
	}

	source = pic->data;
	if (y < 0) 
	{
		source += (pic->width * (-y));
		y = 0;
	}

	if(r_pixbytes == 1)
	{
		dest = vid.buffer+y*vid.rowbytes+x;

		switch(trans_level)
		{
			case 0:
				for(v = 0; v < height; v++)
				{
					memcpy(dest, source, pic->width);
					dest += vid.rowbytes;
					source += pic->width;
				}
				break;
			case 1:
				for(v = 0; v < height; v++)
				{
					for(u = 0;u < pic->width;u++,source++)
					{
						dest[u] = mainTransTable[(((unsigned)dest[u])<<8) + (*source)];
					}
					dest += vid.rowbytes;
				}
				break;
			case 2:
				for(v = 0; v < height; v++)
				{
					for(u = 0;u < pic->width;u++,source++)
					{
						dest[u] = mainTransTable[(((unsigned)(*source))<<8) + dest[u]];
					}
					dest += vid.rowbytes;
				}
				break;
		}
	}
	else
	{ // FIXME: pretranslate at load time?
		pusdest = (unsigned short *)vid.buffer+y*(vid.rowbytes>>1)+x;
		for(v = 0; v < height; v++)
		{
			for(u = 0; u < pic->width; u++)
			{
				pusdest[u] = d_8to16table[source[u]];
			}
			pusdest += vid.rowbytes>>1;
			source += pic->width;
		}
	}
}

//==========================================================================
//
// Draw_TransPicCropped
//
// Draws a holey qpic_t that is clipped at the bottom edge of the screen.
//
//==========================================================================

void Draw_TransPicCropped(int x, int y, qpic_t *pic)
{
	byte *dest, *source, tbyte;
	unsigned short *pusdest;
	int v, u;
	int height;

	if((x < 0) || (x+pic->width > vid.width))
	{
		Sys_Error("Draw_TransPicCropped: bad coordinates");
	}

	if (y >= (int)vid.height || y+pic->height < 0)
	{ // Totally off screen
		return;
	}

	if(y+pic->height > vid.height)
	{
		height = vid.height-y;
	}
	else if (y < 0) 
	{
		height = pic->height+y;
	}
	else
	{
		height = pic->height;
	}

	source = pic->data;
	if (y < 0) 
	{
		source += (pic->width * (-y));
		y = 0;
	}

	if(r_pixbytes == 1)
	{
		dest = vid.buffer+y*vid.rowbytes+x;
		if(pic->width&7)
		{ // General
			switch(trans_level)
			{
				case 0:
					for(v = 0; v < height; v++)
					{
						for(u = 0; u < pic->width; u++)
						{
							if((tbyte = source[u]) != TRANSPARENT_COLOR)
							{
								dest[u] = tbyte;
							}
						}
						dest += vid.rowbytes;
						source += pic->width;
					}
					break;
				case 1:
					for(v = 0; v < height; v++)
					{
						for(u = 0; u < pic->width; u++)
						{
							if((tbyte = source[u]) != TRANSPARENT_COLOR)
							{
								dest[u] = mainTransTable[(((unsigned)dest[u])<<8) + tbyte];
							}
						}
						dest += vid.rowbytes;
						source += pic->width;
					}
					break;
				case 2:
					for(v = 0; v < height; v++)
					{
						for(u = 0; u < pic->width; u++)
						{
							if((tbyte = source[u]) != TRANSPARENT_COLOR)
							{
								dest[u] = mainTransTable[(((unsigned)tbyte)<<8) + dest[u]];
							}
						}
						dest += vid.rowbytes;
						source += pic->width;
					}
					break;
			}

		}
		else
		{ // Unwound
			for(v = 0; v < height; v++)
			{
				for(u = 0; u < pic->width; u += 8)
				{
					switch(trans_level)
					{
						case 0:
							if((tbyte = source[u]) != TRANSPARENT_COLOR)
								dest[u] = tbyte;
							if((tbyte = source[u+1]) != TRANSPARENT_COLOR)
								dest[u+1] = tbyte;
							if((tbyte = source[u+2]) != TRANSPARENT_COLOR)
								dest[u+2] = tbyte;
							if((tbyte = source[u+3]) != TRANSPARENT_COLOR)
								dest[u+3] = tbyte;
							if((tbyte = source[u+4]) != TRANSPARENT_COLOR)
								dest[u+4] = tbyte;
							if((tbyte = source[u+5]) != TRANSPARENT_COLOR)
								dest[u+5] = tbyte;
							if((tbyte = source[u+6]) != TRANSPARENT_COLOR)
								dest[u+6] = tbyte;
							if((tbyte = source[u+7]) != TRANSPARENT_COLOR)
								dest[u+7] = tbyte;
							break;
						case 1:
							if((tbyte = source[u]) != TRANSPARENT_COLOR)
								dest[u] = mainTransTable[(((unsigned)dest[u])<<8) + tbyte];
							if((tbyte = source[u+1]) != TRANSPARENT_COLOR)
								dest[u+1] = mainTransTable[(((unsigned)dest[u+1])<<8) + tbyte];
							if((tbyte = source[u+2]) != TRANSPARENT_COLOR)
								dest[u+2] = mainTransTable[(((unsigned)dest[u=2])<<8) + tbyte];
							if((tbyte = source[u+3]) != TRANSPARENT_COLOR)
								dest[u+3] = mainTransTable[(((unsigned)dest[u+3])<<8) + tbyte];
							if((tbyte = source[u+4]) != TRANSPARENT_COLOR)
								dest[u+4] = mainTransTable[(((unsigned)dest[u+4])<<8) + tbyte];
							if((tbyte = source[u+5]) != TRANSPARENT_COLOR)
								dest[u+5] = mainTransTable[(((unsigned)dest[u+5])<<8) + tbyte];
							if((tbyte = source[u+6]) != TRANSPARENT_COLOR)
								dest[u+6] = mainTransTable[(((unsigned)dest[u+6])<<8) + tbyte];
							if((tbyte = source[u+7]) != TRANSPARENT_COLOR)
								dest[u+7] = mainTransTable[(((unsigned)dest[u+7])<<8) + tbyte];
							break;
						case 2:
							if((tbyte = source[u]) != TRANSPARENT_COLOR)
								dest[u] = mainTransTable[(((unsigned)tbyte)<<8) + dest[u]];
							if((tbyte = source[u+1]) != TRANSPARENT_COLOR)
								dest[u+1] = mainTransTable[(((unsigned)tbyte)<<8) + dest[u+1]];
							if((tbyte = source[u+2]) != TRANSPARENT_COLOR)
								dest[u+2] = mainTransTable[(((unsigned)tbyte)<<8) + dest[u+2]];
							if((tbyte = source[u+3]) != TRANSPARENT_COLOR)
								dest[u+3] = mainTransTable[(((unsigned)tbyte)<<8) + dest[u+3]];
							if((tbyte = source[u+4]) != TRANSPARENT_COLOR)
								dest[u+4] = mainTransTable[(((unsigned)tbyte)<<8) + dest[u+4]];
							if((tbyte = source[u+5]) != TRANSPARENT_COLOR)
								dest[u+5] = mainTransTable[(((unsigned)tbyte)<<8) + dest[u+5]];
							if((tbyte = source[u+6]) != TRANSPARENT_COLOR)
								dest[u+6] = mainTransTable[(((unsigned)tbyte)<<8) + dest[u+6]];
							if((tbyte = source[u+7]) != TRANSPARENT_COLOR)
								dest[u+7] = mainTransTable[(((unsigned)tbyte)<<8) + dest[u+7]];
							break;
					}
				}
				dest += vid.rowbytes;
				source += pic->width;
			}
		}
	}
	else
	{ // FIXME: pretranslate at load time?
		pusdest = (unsigned short *)vid.buffer+y*(vid.rowbytes>>1)+x;
		for(v = 0; v < height; v++)
		{
			for(u = 0; u < pic->width; u++)
			{
				tbyte = source[u];
				if(tbyte != TRANSPARENT_COLOR)
				{
					pusdest[u] = d_8to16table[tbyte];
				}
			}
			pusdest += vid.rowbytes>>1;
			source += pic->width;
		}
	}
}



/*
=============
Draw_SubPic
=============
*/
void Draw_SubPic(int x, int y, qpic_t *pic, int srcx, int srcy, int width, int height)
{
	byte			*dest, *source;
	unsigned short	*pusdest;
	int				v, u;

	if ((x < 0) ||
		(x + width > vid.width) ||
		(y < 0) ||
		(y + height > vid.height))
	{
		Sys_Error ("Draw_Pic: bad coordinates");
	}

	source = pic->data + srcy * pic->width + srcx;

	if (r_pixbytes == 1)
	{
		dest = vid.buffer + y * vid.rowbytes + x;

		for (v=0 ; v<height ; v++)
		{
			memcpy (dest, source, width);
			dest += vid.rowbytes;
			source += pic->width;
		}
	}
	else
	{
	// FIXME: pretranslate at load time?
		pusdest = (unsigned short *)vid.buffer + y * (vid.rowbytes >> 1) + x;

		for (v=0 ; v<height ; v++)
		{
			for (u=srcx ; u<(srcx+width) ; u++)
			{
				pusdest[u] = d_8to16table[source[u]];
			}

			pusdest += vid.rowbytes >> 1;
			source += pic->width;
		}
	}
}

/*
=============
Draw_TransPic
=============
*/
void Draw_TransPic (int x, int y, qpic_t *pic)
{
	byte	*dest, *source, tbyte;
	unsigned short	*pusdest;
	int				v, u;

	if (x < 0 || (unsigned)(x + pic->width) > vid.width || y < 0 ||
		 (unsigned)(y + pic->height) > vid.height)
	{
		Sys_Error ("Draw_TransPic: bad coordinates");
	}
		
	source = pic->data;

	if (r_pixbytes == 1)
	{
		dest = vid.buffer + y * vid.rowbytes + x;

		if (pic->width & 7)
		{	// general
			for (v=0 ; v<pic->height ; v++)
			{
				for (u=0 ; u<pic->width ; u++)
					if ( (tbyte=source[u]) != TRANSPARENT_COLOR)
						dest[u] = tbyte;
	
				dest += vid.rowbytes;
				source += pic->width;
			}
		}
		else
		{	// unwound
			for (v=0 ; v<pic->height ; v++)
			{
				for (u=0 ; u<pic->width ; u+=8)
				{
					if ( (tbyte=source[u]) != TRANSPARENT_COLOR)
						dest[u] = tbyte;
					if ( (tbyte=source[u+1]) != TRANSPARENT_COLOR)
						dest[u+1] = tbyte;
					if ( (tbyte=source[u+2]) != TRANSPARENT_COLOR)
						dest[u+2] = tbyte;
					if ( (tbyte=source[u+3]) != TRANSPARENT_COLOR)
						dest[u+3] = tbyte;
					if ( (tbyte=source[u+4]) != TRANSPARENT_COLOR)
						dest[u+4] = tbyte;
					if ( (tbyte=source[u+5]) != TRANSPARENT_COLOR)
						dest[u+5] = tbyte;
					if ( (tbyte=source[u+6]) != TRANSPARENT_COLOR)
						dest[u+6] = tbyte;
					if ( (tbyte=source[u+7]) != TRANSPARENT_COLOR)
						dest[u+7] = tbyte;
				}
				dest += vid.rowbytes;
				source += pic->width;
			}
		}
	}
	else
	{
	// FIXME: pretranslate at load time?
		pusdest = (unsigned short *)vid.buffer + y * (vid.rowbytes >> 1) + x;

		for (v=0 ; v<pic->height ; v++)
		{
			for (u=0 ; u<pic->width ; u++)
			{
				tbyte = source[u];

				if (tbyte != TRANSPARENT_COLOR)
				{
					pusdest[u] = d_8to16table[tbyte];
				}
			}

			pusdest += vid.rowbytes >> 1;
			source += pic->width;
		}
	}
}


/*
=============
Draw_TransPicTranslate
=============
*/
void Draw_TransPicTranslate (int x, int y, qpic_t *pic, byte *translation)
{
	byte	*dest, *source, tbyte;
	unsigned short	*pusdest;
	int				v, u;

	if (x < 0 || (unsigned)(x + pic->width) > vid.width || y < 0 ||
		 (unsigned)(y + pic->height) > vid.height)
	{
		Sys_Error ("Draw_TransPic: bad coordinates");
	}
		
	source = pic->data;

	if (r_pixbytes == 1)
	{
		dest = vid.buffer + y * vid.rowbytes + x;

		if (pic->width & 7)
		{	// general
			for (v=0 ; v<pic->height ; v++)
			{
				for (u=0 ; u<pic->width ; u++)
					if ( (tbyte=source[u]) != TRANSPARENT_COLOR)
						dest[u] = translation[tbyte];

				dest += vid.rowbytes;
				source += pic->width;
			}
		}
		else
		{	// unwound
			for (v=0 ; v<pic->height ; v++)
			{
				for (u=0 ; u<pic->width ; u+=8)
				{
					if ( (tbyte=source[u]) != TRANSPARENT_COLOR)
						dest[u] = translation[tbyte];
					if ( (tbyte=source[u+1]) != TRANSPARENT_COLOR)
						dest[u+1] = translation[tbyte];
					if ( (tbyte=source[u+2]) != TRANSPARENT_COLOR)
						dest[u+2] = translation[tbyte];
					if ( (tbyte=source[u+3]) != TRANSPARENT_COLOR)
						dest[u+3] = translation[tbyte];
					if ( (tbyte=source[u+4]) != TRANSPARENT_COLOR)
						dest[u+4] = translation[tbyte];
					if ( (tbyte=source[u+5]) != TRANSPARENT_COLOR)
						dest[u+5] = translation[tbyte];
					if ( (tbyte=source[u+6]) != TRANSPARENT_COLOR)
						dest[u+6] = translation[tbyte];
					if ( (tbyte=source[u+7]) != TRANSPARENT_COLOR)
						dest[u+7] = translation[tbyte];
				}
				dest += vid.rowbytes;
				source += pic->width;
			}
		}
	}
	else
	{
	// FIXME: pretranslate at load time?
		pusdest = (unsigned short *)vid.buffer + y * (vid.rowbytes >> 1) + x;

		for (v=0 ; v<pic->height ; v++)
		{
			for (u=0 ; u<pic->width ; u++)
			{
				tbyte = source[u];

				if (tbyte != TRANSPARENT_COLOR)
				{
					pusdest[u] = d_8to16table[tbyte];
				}
			}

			pusdest += vid.rowbytes >> 1;
			source += pic->width;
		}
	}
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
			if (source[x])
				dest[x] = 0x60 + source[x];
		source += 256;
		dest += 320;
	}

}

/*
================
Draw_ConsoleBackground

================
*/
void Draw_ConsoleBackground (int lines)
{
	int				x, y, v;
	byte			*src, *dest;
	unsigned short	*pusdest;
	int				f, fstep;
	qpic_t			*conback;
	char			ver[100];
	static			char saveback[320*8];

	conback = Draw_CachePic ("gfx/menu/conback.lmp");

// hack the version number directly into the pic

	//sprintf (ver, "start commands with a \\ character %4.2f", VERSION);

#if defined(__linux__)
	if (cls.download) {
		sprintf (ver, "%4.2f", VERSION);
		dest = conback->data + 320 + 320*186 - 11 - 8*strlen(ver);
	} else {
		sprintf (ver, "Linux HexenWorld (%4.2f) %4.2f", LINUX_VERSION, VERSION);
		dest = conback->data + 320 - (strlen(ver)*8 + 11) + 320*186;
	}
#else
	if (cls.download) {
		sprintf (ver, "%4.2f", VERSION);
		dest = conback->data + 320 + 320*186 - 11 - 8*strlen(ver);
	} else {
		sprintf (ver, "HexenWorld %4.2f", VERSION);
		dest = conback->data + 320 - (strlen(ver)*8 + 11) + 320*186;
	}
#endif

	memcpy(saveback, conback->data + 320*186, 320*8);
	for (x=0 ; x<strlen(ver) ; x++)
		Draw_CharToConback (ver[x], dest+(x<<3));
	
// draw the pic
	if (r_pixbytes == 1)
	{
		dest = vid.conbuffer;

		for (y=0 ; y<lines ; y++, dest += vid.conrowbytes)
		{
			v = (vid.conheight - lines + y)*200/vid.conheight;
			src = conback->data + v*320;
			if (vid.conwidth == 320)
				memcpy (dest, src, vid.conwidth);
			else
			{
				f = 0;
				fstep = 320*0x10000/vid.conwidth;
				for (x=0 ; x<vid.conwidth ; x+=4)
				{
					dest[x] = src[f>>16];
					f += fstep;
					dest[x+1] = src[f>>16];
					f += fstep;
					dest[x+2] = src[f>>16];
					f += fstep;
					dest[x+3] = src[f>>16];
					f += fstep;
				}
			}
		}
	}
	else
	{
		pusdest = (unsigned short *)vid.conbuffer;

		for (y=0 ; y<lines ; y++, pusdest += (vid.conrowbytes >> 1))
		{
		// FIXME: pre-expand to native format?
		// FIXME: does the endian switching go away in production?
			v = (vid.conheight - lines + y)*200/vid.conheight;
			src = conback->data + v*320;
			f = 0;
			fstep = 320*0x10000/vid.conwidth;
			for (x=0 ; x<vid.conwidth ; x+=4)
			{
				pusdest[x] = d_8to16table[src[f>>16]];
				f += fstep;
				pusdest[x+1] = d_8to16table[src[f>>16]];
				f += fstep;
				pusdest[x+2] = d_8to16table[src[f>>16]];
				f += fstep;
				pusdest[x+3] = d_8to16table[src[f>>16]];
				f += fstep;
			}
		}
	}
	// put it back
	memcpy(conback->data + 320*186, saveback, 320*8);
}


/*
==============
R_DrawRect8
==============
*/
void R_DrawRect8 (vrect_t *prect, int rowbytes, byte *psrc,
	int transparent)
{
	byte	t;
	int		i, j, srcdelta, destdelta;
	byte	*pdest;

	pdest = vid.buffer + (prect->y * vid.rowbytes) + prect->x;

	srcdelta = rowbytes - prect->width;
	destdelta = vid.rowbytes - prect->width;

	if (transparent)
	{
		for (i=0 ; i<prect->height ; i++)
		{
			for (j=0 ; j<prect->width ; j++)
			{
				t = *psrc;
				if (t != TRANSPARENT_COLOR)
				{
					*pdest = t;
				}

				psrc++;
				pdest++;
			}

			psrc += srcdelta;
			pdest += destdelta;
		}
	}
	else
	{
		for (i=0 ; i<prect->height ; i++)
		{
			memcpy (pdest, psrc, prect->width);
			psrc += rowbytes;
			pdest += vid.rowbytes;
		}
	}
}


/*
==============
R_DrawRect16
==============
*/
void R_DrawRect16 (vrect_t *prect, int rowbytes, byte *psrc,
	int transparent)
{
	byte			t;
	int				i, j, srcdelta, destdelta;
	unsigned short	*pdest;

// FIXME: would it be better to pre-expand native-format versions?

	pdest = (unsigned short *)vid.buffer +
			(prect->y * (vid.rowbytes >> 1)) + prect->x;

	srcdelta = rowbytes - prect->width;
	destdelta = (vid.rowbytes >> 1) - prect->width;

	if (transparent)
	{
		for (i=0 ; i<prect->height ; i++)
		{
			for (j=0 ; j<prect->width ; j++)
			{
				t = *psrc;
				if (t != TRANSPARENT_COLOR)
				{
					*pdest = d_8to16table[t];
				}

				psrc++;
				pdest++;
			}

			psrc += srcdelta;
			pdest += destdelta;
		}
	}
	else
	{
		for (i=0 ; i<prect->height ; i++)
		{
			for (j=0 ; j<prect->width ; j++)
			{
				*pdest = d_8to16table[*psrc];
				psrc++;
				pdest++;
			}

			psrc += srcdelta;
			pdest += destdelta;
		}
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
	int				width, height, tileoffsetx, tileoffsety;
	byte			*psrc;
	vrect_t			vr;

	r_rectdesc.rect.x = x;
	r_rectdesc.rect.y = y;
	r_rectdesc.rect.width = w;
	r_rectdesc.rect.height = h;

	vr.y = r_rectdesc.rect.y;
	height = r_rectdesc.rect.height;

	tileoffsety = vr.y % r_rectdesc.height;

	while (height > 0)
	{
		vr.x = r_rectdesc.rect.x;
		width = r_rectdesc.rect.width;

		if (tileoffsety != 0)
			vr.height = r_rectdesc.height - tileoffsety;
		else
			vr.height = r_rectdesc.height;

		if (vr.height > height)
			vr.height = height;

		tileoffsetx = vr.x % r_rectdesc.width;

		while (width > 0)
		{
			if (tileoffsetx != 0)
				vr.width = r_rectdesc.width - tileoffsetx;
			else
				vr.width = r_rectdesc.width;

			if (vr.width > width)
				vr.width = width;

			psrc = r_rectdesc.ptexbytes +
					(tileoffsety * r_rectdesc.rowbytes) + tileoffsetx;

			if (r_pixbytes == 1)
			{
				R_DrawRect8 (&vr, r_rectdesc.rowbytes, psrc, 0);
			}
			else
			{
				R_DrawRect16 (&vr, r_rectdesc.rowbytes, psrc, 0);
			}

			vr.x += vr.width;
			width -= vr.width;
			tileoffsetx = 0;	// only the left tile can be left-clipped
		}

		vr.y += vr.height;
		height -= vr.height;
		tileoffsety = 0;		// only the top tile can be top-clipped
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
	byte			*dest;
	unsigned short	*pusdest;
	unsigned		uc;
	int				u, v;

	if (x < 0 || x + w > vid.width ||
		y < 0 || y + h > vid.height) {
		Con_Printf("Bad Draw_Fill(%d, %d, %d, %d, %c)\n",
			x, y, w, h, c);
		return;
	}

	if (r_pixbytes == 1)
	{
		dest = vid.buffer + y*vid.rowbytes + x;
		switch(trans_level)
		{
			case 0:
				for (v=0 ; v<h ; v++, dest += vid.rowbytes)
					for (u=0 ; u<w ; u++)
					{
						dest[u] = c;
					}
				break;
			case 1:
				for (v=0 ; v<h ; v++, dest += vid.rowbytes)
					for (u=0 ; u<w ; u++)
				{
					dest[u] = mainTransTable[(((unsigned)dest[u])<<8) + c];
				}
				break;
			case 2:
				for (v=0 ; v<h ; v++, dest += vid.rowbytes)
					for (u=0 ; u<w ; u++)
				{
					dest[u] = mainTransTable[(c<<8) + dest[u]];
				}
				break;
		}
	}
	else
	{
		uc = d_8to16table[c];

		pusdest = (unsigned short *)vid.buffer + y * (vid.rowbytes >> 1) + x;
		for (v=0 ; v<h ; v++, pusdest += (vid.rowbytes >> 1))
			for (u=0 ; u<w ; u++)
				pusdest[u] = uc;
	}
}
//=============================================================================

extern byte *mainTransTable; // in r_main.c

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen (void)
{
	int			x,y,i;
	byte		*pbuf;
	int temp[2048], *pos;

	VID_UnlockBuffer ();
	S_ExtraUpdate ();
	VID_LockBuffer ();

	for(x=0;x<2048;x++)
		temp[x] = (164 + rand() % 6) * 256;
	i = 0;

	for (y=0 ; y<vid.height ; y++)
	{
		pbuf = (byte *)(vid.buffer + vid.rowbytes*y);
		pos = &temp[rand() % 256];

		if ((y & 127) == 127)
		{
			VID_UnlockBuffer ();
			S_ExtraUpdate ();
			VID_LockBuffer ();
		}

		for (x=0 ; x<vid.width ; x++,pbuf++)
		{
//			if ((x & 3) != t)
//				pbuf[x] = 0;
			*pbuf = mainTransTable[(*pos)+(*pbuf)];
			pos++;
//			pbuf[x] = mainTransTable[((170+(rand() % 6))*256)+pbuf[x]];
//			pbuf[x] = mainTransTable[159 + (256*pbuf[x])];
		}
	}

	VID_UnlockBuffer ();
	S_ExtraUpdate ();
	VID_LockBuffer ();
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

	if (!draw_disc[index])// rjr || loading_stage)
	{
		return;
	}

	index++;
	if (index >= MAX_DISC) index = 0;

	D_BeginDirectRect (vid.width - 28, 0, draw_disc[index]->data, 28, 24);
//rjr	scr_topupdate = 0;
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
	if (!draw_disc[0]) return;

	return;

	D_EndDirectRect (vid.width - 28, 0, 28, 24);
//rjr	scr_topupdate = 0;
}


