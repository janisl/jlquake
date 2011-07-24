
// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

#include "quakedef.h"
#include "glquake.h"

image_t*	draw_backtile;

image_t*	char_texture;
image_t*	cs_texture; // crosshair texture
image_t*	char_smalltexture;
image_t*	char_menufonttexture;

int	trans_level = 0;

static byte cs_data[64] = {
	0xff, 0xff, 0xff, 0x4f, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0x4f, 0xff, 0xff, 0xff, 0xff,
	0x4f, 0xff, 0x4f, 0xff, 0x4f, 0xff, 0x4f, 0xff,
	0xff, 0xff, 0xff, 0x4f, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0x4f, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

image_t		*conback;

//=============================================================================
/* Support Routines */

/*
===============
Draw_Init
===============
*/
void Draw_Init (void)
{
	char_texture = R_LoadRawFontImageFromFile("gfx/menu/conchars.lmp", 256, 128);
	char_smalltexture = R_LoadRawFontImageFromWad("tinyfont", 128, 32);
	char_menufonttexture = R_LoadBigFontImage("gfx/menu/bigfont2.lmp");

	byte* cs_data32 = R_ConvertImage8To32(cs_data, 8, 8, IMG8MODE_Normal);
	cs_texture = R_CreateImage("crosshair", cs_data32, 8, 8, false, false, GL_CLAMP, false);
	delete[] cs_data32;

	conback = UI_CachePic("gfx/menu/conback.lmp");

	draw_backtile = UI_CachePicRepeat("gfx/menu/backtile.lmp");
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
	num &= 511;

	if (num == 32)
		return;		// space

	UI_DrawChar(x, y, num, 8, 8, char_texture, 32, 16);
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

void Draw_RedString (int x, int y, const char *str)
{
	while (*str)
	{
		Draw_Character (x, y, ((unsigned char)(*str))+256);
		str++;
		x += 8;
	}
}

void Draw_Crosshair(void)
{
	extern QCvar* crosshair;
	extern QCvar* cl_crossx;
	extern QCvar* cl_crossy;
	int x, y;
	extern vrect_t		scr_vrect;

	if (crosshair->value == 2) {
		x = scr_vrect.x + scr_vrect.width/2 - 3 + cl_crossx->value; 
		y = scr_vrect.y + scr_vrect.height/2 - 3 + cl_crossy->value;

		UI_DrawStretchPic(x - 4, y - 4, 16, 16, cs_texture);
	} else if (crosshair->value)
		Draw_Character (scr_vrect.x + scr_vrect.width/2-4 + cl_crossx->value, 
			scr_vrect.y + scr_vrect.height/2-4 + cl_crossy->value, 
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
void Draw_SmallCharacter (int x, int y, int num)
{
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

	UI_DrawChar(x, y, num, 8, 8, char_smalltexture, 16, 4);
}

//==========================================================================
//
// Draw_SmallString
//
//==========================================================================
void Draw_SmallString(int x, int y, const char *str)
{
	while (*str)
	{
		Draw_SmallCharacter (x, y, *str);
		str++;
		x += 6;
	}
}

int M_DrawBigCharacter (int x, int y, int num, int numNext)
{
	int				add;

	if (num == ' ') return 32;

	if (num == '/') num = 26;
	else num -= 65;

	if (num < 0 || num >= 27)  // only a-z and /
		return 0;

	if (numNext == '/') numNext = 26;
	else numNext -= 65;

	UI_DrawChar(x, y, num, 20, 20, char_menufonttexture, 8, 4);

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

	y = lines - 14;
	if (!cls.download)
	{
		char ver[80];
		sprintf(ver, "JLHexenWorld %s", JLQUAKE_VERSION_STRING);
		int x = viddef.width - (QStr::Length(ver) * 8 + 11);
		Draw_RedString(x, y, ver);
	}
}

//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen (void)
{
	UI_Fill(0, 0, viddef.width, viddef.height, 208.0/255.0, 180.0/255.0, 80.0/255.0, 0.2);
	for (int c = 0; c < 40; c++)
	{
		int x = rand() % viddef.width - 20;
		int y = rand() % viddef.height - 20;
		int w = (rand() % 40) + 20;
		int h = (rand() % 40) + 20;
		UI_Fill(x, y, w, h, 208.0 / 255.0, 180.0 / 255.0, 80.0 / 255.0, 0.035);
	}
}
