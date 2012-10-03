
// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

/*
 * $Header: /H2 Mission Pack/gl_draw.c 2     2/26/98 3:09p Jmonroe $
 */

#include "quakedef.h"
#include "../client/game/quake_hexen2/view.h"

/*
===============
Draw_Init
===============
*/
void Draw_Init(void)
{
	char_texture = R_LoadRawFontImageFromFile("gfx/menu/conchars.lmp", 256, 128);
	char_smalltexture = R_LoadRawFontImageFromWad("tinyfont", 128, 32);

	draw_backtile = R_CachePicRepeat("gfx/menu/backtile.lmp");
	Con_InitBackgroundImage();
	MQH_InitImages();
	VQH_InitCrosshairTexture();
	SCRQH_InitImages();
}
