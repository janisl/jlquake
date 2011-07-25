//**************************************************************************
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "client.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

viddef_t	viddef;

vec4_t g_color_table[8] =
{
	{0.0, 0.0, 0.0, 1.0},
	{1.0, 0.0, 0.0, 1.0},
	{0.0, 1.0, 0.0, 1.0},
	{1.0, 1.0, 0.0, 1.0},
	{0.0, 0.0, 1.0, 1.0},
	{0.0, 1.0, 1.0, 1.0},
	{1.0, 0.0, 1.0, 1.0},
	{1.0, 1.0, 1.0, 1.0},
};

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	UI_AdjustFromVirtualScreen
//
//	Adjusted for resolution and screen aspect ratio
//
//==========================================================================

void UI_AdjustFromVirtualScreen(float* x, float* y, float* w, float* h)
{
	// scale for screen sizes
	float xscale = (float)cls_common->glconfig.vidWidth / viddef.width;
	float yscale = (float)cls_common->glconfig.vidHeight / viddef.height;
	if (x)
	{
		*x *= xscale;
	}
	if (y)
	{
		*y *= yscale;
	}
	if (w)
	{
		*w *= xscale;
	}
	if (h)
	{
		*h *= yscale;
	}
}

//==========================================================================
//
//	UI_DrawPic
//
//==========================================================================

void UI_DrawPic(int x, int y, image_t* pic, float alpha)
{
	UI_DrawStretchPic(x, y, R_GetImageWidth(pic), R_GetImageHeight(pic), pic, alpha);
}

//==========================================================================
//
//	UI_DrawNamedPic
//
//==========================================================================

void UI_DrawNamedPic(int x, int y, const char* pic)
{
	image_t* gl = R_RegisterPic(pic);
	if (!gl)
	{
		GLog.Write("Can't find pic: %s\n", pic);
		return;
	}
	UI_DrawPic(x, y, gl);
}

//==========================================================================
//
//	UI_DrawStretchPic
//
//==========================================================================

void UI_DrawStretchPic(int x, int y, int w, int h, image_t* pic, float alpha)
{
	R_Draw2DQuad(x, y, w, h, pic, 0, 0, 1, 1, 1, 1, 1, alpha);
}

//==========================================================================
//
//	UI_DrawStretchNamedPic
//
//==========================================================================

void UI_DrawStretchNamedPic(int x, int y, int w, int h, const char* pic)
{
	image_t* gl = R_RegisterPic(pic);
	if (!gl)
	{
		GLog.Write("Can't find pic: %s\n", pic);
		return;
	}
	UI_DrawStretchPic(x, y, w, h, gl);
}

//==========================================================================
//
//	UI_DrawStretchPicWithColour
//
//==========================================================================

void UI_DrawStretchPicWithColour(int x, int y, int w, int h, image_t* pic, byte* colour)
{
	R_Draw2DQuad(x, y, w, h, pic, 0, 0, 1, 1, colour[0] / 255.0, colour[1] / 255.0, colour[2] / 255.0, colour[3] / 255.0);
}

//==========================================================================
//
//	UI_DrawSubPic
//
//==========================================================================

void UI_DrawSubPic(int x, int y, image_t* pic, int srcx, int srcy, int width, int height)
{
	float newsl = (float)srcx / (float)R_GetImageWidth(pic);
	float newsh = newsl + (float)width / (float)R_GetImageWidth(pic);

	float newtl = (float)srcy / (float)R_GetImageHeight(pic);
	float newth = newtl + (float)height / (float)R_GetImageHeight(pic);
	
	R_Draw2DQuad(x, y, width, height, pic, newsl, newtl, newsh, newth, 1, 1, 1, 1);
}

//==========================================================================
//
//	UI_TileClear
//
//	This repeats a 64*64 tile graphic to fill the screen around a sized down
// refresh window.
//
//==========================================================================

void UI_TileClear(int x, int y, int w, int h, image_t* pic)
{
	R_Draw2DQuad(x, y, w, h, pic, x / 64.0, y / 64.0, (x + w) / 64.0, (y + h) / 64.0, 1, 1, 1, 1);
}

//==========================================================================
//
//	UI_NamedTileClear
//
//==========================================================================

void UI_NamedTileClear(int x, int y, int w, int h, const char* pic)
{
	image_t* image = R_RegisterPicRepeat(pic);
	if (!image)
	{
		GLog.Write("Can't find pic: %s\n", pic);
		return;
	}
	UI_TileClear(x, y, w, h, image);
}

//==========================================================================
//
//	UI_Fill
//
//==========================================================================

void UI_Fill(int x, int y, int w, int h, float r, float g, float b, float a)
{
	R_Draw2DQuad(x, y, w, h, NULL, 0, 0, 0, 0, r, g, b, a);
}

//==========================================================================
//
//	UI_FillPal
//
//==========================================================================

void UI_FillPal(int x, int y, int w, int h, int c)
{
	if ((unsigned)c > 255)
	{
		throw QException("UI_FillPal: bad color");
	}
	UI_Fill(x, y, w, h, r_palette[c][0] / 255.0, r_palette[c][1] / 255.0, r_palette[c][2] / 255.0, 1);
}

//==========================================================================
//
//	UI_DrawChar
//
//==========================================================================

void UI_DrawChar(int x, int y, int num, int w, int h, image_t* image, int numberOfColumns, int numberOfRows)
{
	if (y <= -h || y >= viddef.height)
	{
		// Totally off screen
		return;
	}

	int row = num / numberOfColumns;
	int col = num % numberOfColumns;

	float xsize = 1.0 / (float)numberOfColumns;
	float ysize = 1.0 / (float)numberOfRows;
	float fcol = col * xsize;
	float frow = row * ysize;

	R_Draw2DQuad(x, y, w, h, image, fcol, frow, fcol + xsize, frow + ysize, 1, 1, 1, 1);
}

//==========================================================================
//
//	SCR_FillRect
//
//==========================================================================

void SCR_FillRect(float x, float y, float width, float height, const float* color)
{
	R_SetColor(color);

	UI_AdjustFromVirtualScreen(&x, &y, &width, &height);
	R_StretchPic(x, y, width, height, 0, 0, 0, 0, cls_common->whiteShader);

	R_SetColor(NULL);
}

//==========================================================================
//
//	SCR_DrawPic
//
//==========================================================================

void SCR_DrawPic(float x, float y, float width, float height, qhandle_t hShader)
{
	UI_AdjustFromVirtualScreen(&x, &y, &width, &height);
	R_StretchPic(x, y, width, height, 0, 0, 1, 1, hShader);
}

//==========================================================================
//
//	SCR_DrawNamedPic
//
//==========================================================================

void SCR_DrawNamedPic(float x, float y, float width, float height, const char* picname)
{
	qassert(width != 0);
	qhandle_t hShader = R_RegisterShader(picname);
	UI_AdjustFromVirtualScreen(&x, &y, &width, &height);
	R_StretchPic(x, y, width, height, 0, 0, 1, 1, hShader);
}

//==========================================================================
//
//	SCR_DrawChar
//
//==========================================================================

static void SCR_DrawChar(int x, int y, float size, int ch)
{
	ch &= 255;

	if (ch == ' ')
	{
		return;
	}

	if (y < -size)
	{
		return;
	}

	float ax = x;
	float ay = y;
	float aw = size;
	float ah = size;
	UI_AdjustFromVirtualScreen(&ax, &ay, &aw, &ah);

	int row = ch >> 4;
	int col = ch & 15;

	float frow = row * 0.0625;
	float fcol = col * 0.0625;
	size = 0.0625;

	R_StretchPic(ax, ay, aw, ah, fcol, frow, fcol + size, frow + size, cls_common->charSetShader);
}

//==========================================================================
//
//	SCR_DrawSmallChar
//
//	small chars are drawn at native screen resolution
//
//==========================================================================

void SCR_DrawSmallChar(int x, int y, int ch)
{
	ch &= 255;

	if (ch == ' ')
	{
		return;
	}

	if (y < -SMALLCHAR_HEIGHT)
	{
		return;
	}

	int row = ch >> 4;
	int col = ch & 15;

	float frow = row * 0.0625;
	float fcol = col * 0.0625;
	float size = 0.0625;

	R_StretchPic(x, y, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, fcol, frow, fcol + size, frow + size, cls_common->charSetShader);
}

//==========================================================================
//
//	SCR_DrawBigString[Color]
//
//	Draws a multi-colored string with a drop shadow, optionally forcing
// to a fixed color.
//
//==========================================================================

void SCR_DrawStringExt(int x, int y, float size, const char* string, float* setColor, bool forceColor)
{
	// draw the drop shadow
	vec4_t color;
	color[0] = color[1] = color[2] = 0;
	color[3] = setColor[3];
	R_SetColor(color);
	const char* s = string;
	int xx = x;
	while (*s)
	{
		if (Q_IsColorString(s))
		{
			s += 2;
			continue;
		}
		SCR_DrawChar(xx + 2, y + 2, size, *s);
		xx += size;
		s++;
	}

	// draw the colored text
	s = string;
	xx = x;
	R_SetColor(setColor);
	while (*s)
	{
		if (Q_IsColorString(s))
		{
			if (!forceColor)
			{
				Com_Memcpy(color, g_color_table[ColorIndex(*(s + 1))], sizeof(color));
				color[3] = setColor[3];
				R_SetColor(color);
			}
			s += 2;
			continue;
		}
		SCR_DrawChar(xx, y, size, *s);
		xx += size;
		s++;
	}
	R_SetColor(NULL);
}

//==========================================================================
//
//	SCR_DrawBigString
//
//==========================================================================

void SCR_DrawBigString(int x, int y, const char* s, float alpha)
{
	float color[4];
	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	SCR_DrawStringExt(x, y, BIGCHAR_WIDTH, s, color, false);
}

//==========================================================================
//
//	SCR_DrawBigStringColor
//
//==========================================================================

void SCR_DrawBigStringColor(int x, int y, const char *s, vec4_t color)
{
	SCR_DrawStringExt(x, y, BIGCHAR_WIDTH, s, color, true);
}

//==========================================================================
//
//	SCR_DrawSmallStringExt
//
//==========================================================================

void SCR_DrawSmallStringExt(int x, int y, const char* string, float* setColor, bool forceColor)
{
	// draw the colored text
	const char* s = string;
	int xx = x;
	R_SetColor(setColor);
	while (*s)
	{
		if (Q_IsColorString(s))
		{
			if (!forceColor)
			{
				vec4_t color;
				Com_Memcpy(color, g_color_table[ColorIndex(*(s + 1))], sizeof(color));
				color[3] = setColor[3];
				R_SetColor(color);
			}
			s += 2;
			continue;
		}
		SCR_DrawSmallChar(xx, y, *s);
		xx += SMALLCHAR_WIDTH;
		s++;
	}
	R_SetColor(NULL);
}
