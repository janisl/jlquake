/*
Copyright (C) 1997-2001 Id Software, Inc.

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
// r_main.c
#include "gl_local.h"
#include "../client/client.h"

refimport_t	ri;

float		v_blend[4];			// final blending color

QCvar	*gl_polyblend;


/*
============
R_PolyBlend
============
*/
void R_PolyBlend(refdef_t *fd)
{
	if (!gl_polyblend->value)
		return;
	if (!v_blend[3])
		return;

	R_Draw2DQuad(fd->x, fd->y, fd->width, fd->height, NULL, 0, 0, 0, 0, v_blend[0], v_blend[1], v_blend[2], v_blend[3]);
}

//=======================================================================

/*
@@@@@@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@@@@@@
*/
void R_RenderFrame(refdef_t *fd)
{
	R_RenderScene(fd);

	R_PolyBlend(fd);
}


/*
===============
CL_InitRenderStuff
===============
*/
void CL_InitRenderStuff()
{	
	R_BeginRegistration(&cls.glconfig);

	gl_polyblend = Cvar_Get ("gl_polyblend", "1", 0);

	// let the sound and input subsystems know about the new window
	VID_NewWindow(glConfig.vidWidth, glConfig.vidHeight);

	Draw_InitLocal();
}

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
refexport_t GetRefAPI (refimport_t rimp )
{
	refexport_t	re;

	ri = rimp;

	re.RenderFrame = R_RenderFrame;

	return re;
}

void R_ClearScreen()
{
	qglClearColor(0, 0, 0, 0);
	qglClear(GL_COLOR_BUFFER_BIT);
}
