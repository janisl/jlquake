// gl_vidnt.c -- NT GL vid component

#include "quakedef.h"
#include "glquake.h"
#include "../../client/windows_shared.h"
#include "resource.h"
#include <commctrl.h>

//====================================

int VID_SetMode(unsigned char *palette)
{
// so Con_Printfs don't mess us up by forcing vid and snd updates
	qboolean temp = scr_disabled_for_loading;
	scr_disabled_for_loading = true;

	bool fullscreen = !!r_fullscreen->integer;

	if (GLW_SetMode(r_mode->integer, r_colorbits->integer, fullscreen) != RSERR_OK)
		Sys_Error ("Couldn't set fullscreen DIB mode");

	if (vid.conheight > glConfig.vidHeight)
		vid.conheight = glConfig.vidHeight;
	if (vid.conwidth > glConfig.vidWidth)
		vid.conwidth = glConfig.vidWidth;
	vid.width = vid.conwidth;
	vid.height = vid.conheight;

	vid.numpages = 2;

	scr_disabled_for_loading = temp;

	VID_SetPalette (palette);

	vid.recalc_refdef = 1;

	return true;
}

void GL_EndRendering (void)
{
	GLimp_SwapBuffers();
}

void	VID_Shutdown (void)
{
	GLimp_Shutdown();

	QGL_Shutdown();
}


/*
===================================================================

MAIN WINDOW

===================================================================
*/

void GL_Init();
/*
===================
VID_Init
===================
*/
void	VID_Init (unsigned char *palette)
{
	R_SharedRegister();

	InitCommonControls();
	VID_SetPalette (palette);

	int i;
	if ((i = COM_CheckParm("-conwidth")) != 0)
		vid.conwidth = QStr::Atoi(COM_Argv(i+1));
	else
		vid.conwidth = 640;

	vid.conwidth &= 0xfff8; // make it a multiple of eight

	if (vid.conwidth < 320)
		vid.conwidth = 320;

	// pick a conheight that matches with correct aspect
	vid.conheight = vid.conwidth*3 / 4;

	if ((i = COM_CheckParm("-conheight")) != 0)
		vid.conheight = QStr::Atoi(COM_Argv(i+1));
	if (vid.conheight < 200)
		vid.conheight = 200;

	vid.colormap = host_colormap;

	Sys_ShowConsole(0, false);

	VID_SetMode(palette);

	GL_Init ();

	VID_SetPalette (palette);
}
