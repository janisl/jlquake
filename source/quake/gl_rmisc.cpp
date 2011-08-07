/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// r_misc.c

#include "quakedef.h"
#include "../client/render_local.h"

//
// screen size info
//
refdef_t	r_refdef;

QCvar*	gl_doubleeyes;

/*
===============
CL_InitRenderStuff
===============
*/
void CL_InitRenderStuff (void)
{	
	if (qglActiveTextureARB)
		Cvar_SetValue ("r_texsort", 0.0);

	gl_doubleeyes = Cvar_Get("gl_doubleeys", "1", 0);

	R_InitParticles ();
}

/*
===============
R_NewMap
===============
*/
void R_NewMap (void)
{
	int		i;
	
	for (i=0 ; i<256 ; i++)
		cl_lightstylevalue[i] = 264;		// normal light value

	R_ClearParticles ();

	R_EndRegistration();
}


void VID_Init()
{
	R_BeginRegistration(&cls.glconfig);

	Sys_ShowConsole(0, false);

	int i;
	if ((i = COM_CheckParm("-conwidth")) != 0)
		viddef.width = QStr::Atoi(COM_Argv(i+1));
	else
		viddef.width = 640;

	viddef.width &= 0xfff8; // make it a multiple of eight

	if (viddef.width < 320)
		viddef.width = 320;

	// pick a conheight that matches with correct aspect
	viddef.height = viddef.width / glConfig.windowAspect;

	if ((i = COM_CheckParm("-conheight")) != 0)
		viddef.height = QStr::Atoi(COM_Argv(i+1));
	if (viddef.height < 200)
		viddef.height = 200;

	if (viddef.height > glConfig.vidHeight)
		viddef.height = glConfig.vidHeight;
	if (viddef.width > glConfig.vidWidth)
		viddef.width = glConfig.vidWidth;
}
