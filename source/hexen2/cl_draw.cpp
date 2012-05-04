
// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

/*
 * $Header: /H2 Mission Pack/gl_draw.c 2     2/26/98 3:09p Jmonroe $
 */

#include "quakedef.h"

image_t* draw_backtile;

image_t* char_smalltexture;
image_t* char_menufonttexture;

image_t* conback;

/*
===============
Draw_Init
===============
*/
void Draw_Init(void)
{
	char_texture = R_LoadRawFontImageFromFile("gfx/menu/conchars.lmp", 256, 128);
	char_smalltexture = R_LoadRawFontImageFromWad("tinyfont", 128, 32);
	char_menufonttexture = R_LoadBigFontImage("gfx/menu/bigfont2.lmp");

	conback = R_CachePic("gfx/menu/conback.lmp");

	draw_backtile = R_CachePicRepeat("gfx/menu/backtile.lmp");
}

/*
================
Draw_String
================
*/
void Draw_String(int x, int y, const char* str)
{
	while (*str)
	{
		UI_DrawChar(x, y, *str);
		str++;
		x += 8;
	}
}

void Draw_RedString(int x, int y, const char* str)
{
	while (*str)
	{
		UI_DrawChar(x, y, ((unsigned char)(*str)) + 256);
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
void Draw_SmallCharacter(int x, int y, int num)
{
	if (num < 32)
	{
		num = 0;
	}
	else if (num >= 'a' && num <= 'z')
	{
		num -= 64;
	}
	else if (num > '_')
	{
		num = 0;
	}
	else
	{
		num -= 32;
	}

	if (num == 0)
	{
		return;
	}

	UI_DrawCharBase(x, y, num, 8, 8, char_smalltexture, 16, 4);
}

//==========================================================================
//
// Draw_SmallString
//
//==========================================================================
void Draw_SmallString(int x, int y, const char* str)
{
	while (*str)
	{
		Draw_SmallCharacter(x, y, *str);
		str++;
		x += 6;
	}
}

int M_DrawBigCharacter(int x, int y, int num, int numNext)
{
	int add;

	if (num == ' ')
	{
		return 32;
	}

	if (num == '/')
	{
		num = 26;
	}
	else
	{
		num -= 65;
	}

	if (num < 0 || num >= 27)	// only a-z and /
	{
		return 0;
	}

	if (numNext == '/')
	{
		numNext = 26;
	}
	else
	{
		numNext -= 65;
	}

	UI_DrawCharBase(x, y, num, 20, 20, char_menufonttexture, 8, 4);

	if (numNext < 0 || numNext >= 27)
	{
		return 0;
	}

	add = 0;
	if (num == (int)'C' - 65 && numNext == (int)'P' - 65)
	{
		add = 3;
	}

	return BigCharWidth[num][numNext] + add;
}

/*
================
Draw_ConsoleBackground

================
*/
void Draw_ConsoleBackground(int lines)
{
	UI_DrawStretchPic(0, lines - viddef.height, viddef.width, viddef.height, conback);

	int y = lines - 14;
	char ver[80];
	sprintf(ver, "JLHexen II %s", JLQUAKE_VERSION_STRING);
	int x = viddef.width - (String::Length(ver) * 8 + 11);
	Draw_RedString(x, y, ver);
}

//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen(void)
{
	UI_Fill(0, 0, viddef.width, viddef.height, 208.0 / 255.0, 180.0 / 255.0, 80.0 / 255.0, 0.2);
	for (int c = 0; c < 40; c++)
	{
		int x = rand() % viddef.width - 20;
		int y = rand() % viddef.height - 20;
		int w = (rand() % 40) + 20;
		int h = (rand() % 40) + 20;
		UI_Fill(x, y, w, h, 208.0 / 255.0, 180.0 / 255.0, 80.0 / 255.0, 0.035);
	}
}
