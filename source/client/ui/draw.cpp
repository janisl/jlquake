//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../client.h"

viddef_t viddef;

image_t* char_texture;

vec4_t g_color_table[32] =
{
	{ 0.0,  0.0,    0.0,    1.0 },		// 0 - black		0
	{ 1.0,  0.0,    0.0,    1.0 },		// 1 - red			1
	{ 0.0,  1.0,    0.0,    1.0 },		// 2 - green		2
	{ 1.0,  1.0,    0.0,    1.0 },		// 3 - yellow		3
	{ 0.0,  0.0,    1.0,    1.0 },		// 4 - blue			4
	{ 0.0,  1.0,    1.0,    1.0 },		// 5 - cyan			5
	{ 1.0,  0.0,    1.0,    1.0 },		// 6 - purple		6
	{ 1.0,  1.0,    1.0,    1.0 },		// 7 - white		7
	{ 1.0,  0.5,    0.0,    1.0 },		// 8 - orange		8
	{ 0.5,  0.5,    0.5,    1.0 },		// 9 - md.grey		9
	{ 0.75, 0.75,   0.75,   1.0 },		// : - lt.grey		10		// lt grey for names
	{ 0.75, 0.75,   0.75,   1.0 },		// ; - lt.grey		11
	{ 0.0,  0.5,    0.0,    1.0 },		// < - md.green		12
	{ 0.5,  0.5,    0.0,    1.0 },		// = - md.yellow	13
	{ 0.0,  0.0,    0.5,    1.0 },		// > - md.blue		14
	{ 0.5,  0.0,    0.0,    1.0 },		// ? - md.red		15
	{ 0.5,  0.25,   0.0,    1.0 },		// @ - md.orange	16
	{ 1.0,  0.6f,   0.1f,   1.0 },		// A - lt.orange	17
	{ 0.0,  0.5,    0.5,    1.0 },		// B - md.cyan		18
	{ 0.5,  0.0,    0.5,    1.0 },		// C - md.purple	19
	{ 0.0,  0.5,    1.0,    1.0 },		// D				20
	{ 0.5,  0.0,    1.0,    1.0 },		// E				21
	{ 0.2f, 0.6f,   0.8f,   1.0 },		// F				22
	{ 0.8f, 1.0,    0.8f,   1.0 },		// G				23
	{ 0.0,  0.4,    0.2f,   1.0 },		// H				24
	{ 1.0,  0.0,    0.2f,   1.0 },		// I				25
	{ 0.7f, 0.1f,   0.1f,   1.0 },		// J				26
	{ 0.6f, 0.2f,   0.0,    1.0 },		// K				27
	{ 0.8f, 0.6f,   0.2f,   1.0 },		// L				28
	{ 0.6f, 0.6f,   0.2f,   1.0 },		// M				29
	{ 1.0,  1.0,    0.75,   1.0 },		// N				30
	{ 1.0,  1.0,    0.5,    1.0 },		// O				31
};

//	Adjusted for resolution and screen aspect ratio
void UI_AdjustFromVirtualScreen(float* x, float* y, float* w, float* h)
{
	// scale for screen sizes
	float xscale = (float)cls.glconfig.vidWidth / viddef.width;
	float yscale = (float)cls.glconfig.vidHeight / viddef.height;
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

void UI_DrawPic(int x, int y, image_t* pic, float alpha)
{
	UI_DrawStretchPic(x, y, R_GetImageWidth(pic), R_GetImageHeight(pic), pic, alpha);
}

void UI_DrawNamedPic(int x, int y, const char* pic)
{
	image_t* gl = R_RegisterPic(pic);
	if (!gl)
	{
		Log::write("Can't find pic: %s\n", pic);
		return;
	}
	UI_DrawPic(x, y, gl);
}

static void DoQuad(float x, float y, float width, float height,
	image_t* image, float s1, float t1, float s2, float t2,
	float r, float g, float b, float a)
{
	UI_AdjustFromVirtualScreen(&x, &y, &width, &height);

	R_Draw2DQuad(x, y, width, height, image, s1, t1, s2, t2, r, g, b, a);
}

void UI_DrawStretchPic(int x, int y, int w, int h, image_t* pic, float alpha)
{
	DoQuad(x, y, w, h, pic, 0, 0, 1, 1, 1, 1, 1, alpha);
}

void UI_DrawStretchNamedPic(int x, int y, int w, int h, const char* pic)
{
	image_t* gl = R_RegisterPic(pic);
	if (!gl)
	{
		Log::write("Can't find pic: %s\n", pic);
		return;
	}
	UI_DrawStretchPic(x, y, w, h, gl);
}

void UI_DrawStretchPicWithColour(int x, int y, int w, int h, image_t* pic, byte* colour)
{
	DoQuad(x, y, w, h, pic, 0, 0, 1, 1, colour[0] / 255.0, colour[1] / 255.0, colour[2] / 255.0, colour[3] / 255.0);
}

void UI_DrawSubPic(int x, int y, image_t* pic, int srcx, int srcy, int width, int height)
{
	float newsl = (float)srcx / (float)R_GetImageWidth(pic);
	float newsh = newsl + (float)width / (float)R_GetImageWidth(pic);

	float newtl = (float)srcy / (float)R_GetImageHeight(pic);
	float newth = newtl + (float)height / (float)R_GetImageHeight(pic);

	DoQuad(x, y, width, height, pic, newsl, newtl, newsh, newth, 1, 1, 1, 1);
}

//	This repeats a 64*64 tile graphic to fill the screen around a sized down
// refresh window.
void UI_TileClear(int x, int y, int w, int h, image_t* pic)
{
	DoQuad(x, y, w, h, pic, x / 64.0, y / 64.0, (x + w) / 64.0, (y + h) / 64.0, 1, 1, 1, 1);
}

void UI_NamedTileClear(int x, int y, int w, int h, const char* pic)
{
	image_t* image = R_RegisterPicRepeat(pic);
	if (!image)
	{
		Log::write("Can't find pic: %s\n", pic);
		return;
	}
	UI_TileClear(x, y, w, h, image);
}

void UI_Fill(int x, int y, int w, int h, float r, float g, float b, float a)
{
	DoQuad(x, y, w, h, NULL, 0, 0, 0, 0, r, g, b, a);
}

void UI_FillPal(int x, int y, int w, int h, int c)
{
	if ((unsigned)c > 255)
	{
		throw Exception("UI_FillPal: bad color");
	}
	UI_Fill(x, y, w, h, r_palette[c][0] / 255.0, r_palette[c][1] / 255.0, r_palette[c][2] / 255.0, 1);
}

void UI_DrawCharBase(int x, int y, int num, int w, int h, image_t* image, int numberOfColumns,
	int numberOfRows, float r, float g, float b, float a)
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

	DoQuad(x, y, w, h, image, fcol, frow, fcol + xsize, frow + ysize, r, g, b, a);
}

void UI_DrawChar(int x, int y, int num, float r, float g, float b, float a)
{
	if (GGameType & GAME_Hexen2)
	{
		num &= 511;

		if ((num & 255) == 32)
		{
			return;		// space

		}
		UI_DrawCharBase(x, y, num, 8, 8, char_texture, 32, 16, r, g, b, a);
	}
	else
	{
		num &= 255;

		if ((num & 127) == 32)
		{
			return;		// space
		}
		UI_DrawCharBase(x, y, num, 8, 8, char_texture, 16, 16, r, g, b, a);
	}
}

void UI_DrawString(int x, int y, const char* str, int mask)
{
	vec4_t color;
	Vector4Set(color, 1, 1, 1, 1);
	while (*str)
	{
		if (Q_IsColorString(str))
		{
			if (*(str + 1) == COLOR_NULL)
			{
				Vector4Set(color, 1, 1, 1, 1);
			}
			else
			{
				Com_Memcpy(color, g_color_table[ColorIndex(*(str + 1))], sizeof(color));
			}
			str += 2;
			continue;
		}
		UI_DrawChar(x, y, ((byte)*str) | mask, color[0], color[1], color[2], color[3]);
		str++;
		x += 8;
	}
}

void SCR_FillRect(float x, float y, float width, float height, const float* color)
{
	R_SetColor(color);

	UI_AdjustFromVirtualScreen(&x, &y, &width, &height);
	R_StretchPic(x, y, width, height, 0, 0, 0, 0, cls.whiteShader);

	R_SetColor(NULL);
}

void SCR_DrawPic(float x, float y, float width, float height, qhandle_t hShader)
{
	UI_AdjustFromVirtualScreen(&x, &y, &width, &height);
	R_StretchPic(x, y, width, height, 0, 0, 1, 1, hShader);
}

void SCR_DrawNamedPic(float x, float y, float width, float height, const char* picname)
{
	qassert(width != 0);
	qhandle_t hShader = R_RegisterShader(picname);
	UI_AdjustFromVirtualScreen(&x, &y, &width, &height);
	R_StretchPic(x, y, width, height, 0, 0, 1, 1, hShader);
}

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

	R_StretchPic(ax, ay, aw, ah, fcol, frow, fcol + size, frow + size, cls.charSetShader);
}

//	small chars are drawn at native screen resolution
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

	R_StretchPic(x, y, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, fcol, frow, fcol + size, frow + size, cls.charSetShader);
}

//	Draws a multi-colored string with a drop shadow, optionally forcing
// to a fixed color.
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
				if (*(s + 1) == COLOR_NULL)
				{
					Com_Memcpy(color, setColor, sizeof(color));
				}
				else
				{
					Com_Memcpy(color, g_color_table[ColorIndex(*(s + 1))], sizeof(color));
					color[3] = setColor[3];
				}
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

void SCR_DrawBigString(int x, int y, const char* s, float alpha)
{
	float color[4];
	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	SCR_DrawStringExt(x, y, BIGCHAR_WIDTH, s, color, false);
}

void SCR_DrawBigStringColor(int x, int y, const char* s, vec4_t color)
{
	SCR_DrawStringExt(x, y, BIGCHAR_WIDTH, s, color, true);
}

static void SCR_DrawSmallStringExt(int x, int y, const char* string, float* setColor, bool forceColor)
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
				if (*(s + 1) == COLOR_NULL)
				{
					Com_Memcpy(color, setColor, sizeof(color));
				}
				else
				{
					Com_Memcpy(color, g_color_table[ColorIndex(*(s + 1))], sizeof(color));
					color[3] = setColor[3];
				}
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

void SCR_DrawSmallString(int x, int y, const char* string)
{
	float color[4];
	color[0] = color[1] = color[2] = color[3] = 1.0;
	SCR_DrawSmallStringExt(x, y, string, color, false);
}
