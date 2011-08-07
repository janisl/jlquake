// r_misc.c

#include "quakedef.h"
#include "../client/render_local.h"

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
