// screen.c -- master for refresh, status bar, console, chat, notify, etc

/*
 * $Header: /H2 Mission Pack/gl_screen.c 20    3/18/98 11:34p Jmonroe $
 */

#include "quakedef.h"
#include "../client/game/quake_hexen2/view.h"

/*

background clear
rendering
turtle/net/ram icons
sbar
centerprint / slow centerprint
notify lines
intermission / finale overlay
loading plaque
console
menu

required background clears
required update regions


syncronous draw mode or async
One off screen buffer, with updates either copied or xblited
Need to double buffer?


async draw will require the refresh area to be cleared, because it will be
xblited, but sync draw can just ignore it.

sync
draw

CenterPrint ()
SlowPrint ()
Screen_Update ();
common->Printf ();

net
turn off messages option

the refresh is allways rendered, unless the console is full screen


console is:
    notify lines
    half
    full


*/

/*
==================
SCR_Init
==================
*/
void SCR_Init(void)
{
	SCR_InitCommon();

	scr_initialized = true;
}

void VID_Init()
{
	R_BeginRegistration(&cls.glconfig);

	CLH2_InitColourShadeTables();

	Sys_ShowConsole(0, false);

	if (COM_CheckParm("-scale2d"))
	{
		viddef.height = 200;
		viddef.width = 320;
	}
	else
	{
		viddef.height = cls.glconfig.vidHeight;
		viddef.width = cls.glconfig.vidWidth;
	}
}
