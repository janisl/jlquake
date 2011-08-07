// r_misc.c

#include "quakedef.h"
#include "../client/render_local.h"

unsigned	d_8to24TranslucentTable[256];

float RTint[256],GTint[256],BTint[256];

//
// screen size info
//
refdef_t	r_refdef;

/*
===============
CL_InitRenderStuff
===============
*/
void CL_InitRenderStuff (void)
{
	R_InitParticles ();

	playerTranslation = (byte *)COM_LoadHunkFile ("gfx/player.lmp");
	if (!playerTranslation)
		Sys_Error ("Couldn't load gfx/player.lmp");
}

void D_ShowLoadingSize(void)
{
	if (!tr.registered)
		return;

	qglDrawBuffer  (GL_FRONT);

	SCR_DrawLoading();

	qglDrawBuffer  (GL_BACK);
}
