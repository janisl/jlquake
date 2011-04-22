// gl_vidnt.c -- NT GL vid component

#include "quakedef.h"
#include "glquake.h"
#include "../client/windows_shared.h"
#include "resource.h"

#define BASEWIDTH		320
#define BASEHEIGHT		200

void GL_Init (void);

//====================================

int VID_SetMode(unsigned char *palette)
{
// so Con_Printfs don't mess us up by forcing vid and snd updates
	qboolean temp = scr_disabled_for_loading;
	scr_disabled_for_loading = true;

	bool fullscreen = !!r_fullscreen->integer;

	if (GLW_SetMode(r_mode->integer, r_colorbits->integer, fullscreen) != RSERR_OK)
		Sys_Error ("Couldn't set fullscreen DIB mode");

	if (COM_CheckParm ("-scale2d")) {
		vid.height = vid.conheight = BASEHEIGHT ;//modelist[modenum].height; // BASEHEIGHT;
		vid.width = vid.conwidth =   BASEWIDTH  ;//modelist[modenum].width; //  BASEWIDTH ;
	} else {
		vid.height = vid.conheight = glConfig.vidHeight; // BASEHEIGHT;
		vid.width = vid.conwidth =   glConfig.vidWidth; //  BASEWIDTH ;
	}
	vid.numpages = 2;

	scr_disabled_for_loading = temp;

	VID_SetPalette (palette);

	vid.recalc_refdef = 1;

	return true;
}

//====================================


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

	vid_initialized = true;

	vid.colormap = host_colormap;

	Sys_ShowConsole(0, false);

	VID_SetMode(palette);

	GL_Init ();

	VID_SetPalette (palette);
}
