
// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

/*
 * $Header: /H2 Mission Pack/gl_draw.c 2     2/26/98 3:09p Jmonroe $
 */

#include "quakedef.h"

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
