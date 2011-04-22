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
/*
** GLW_IMP.C
**
** This file contains ALL Win32 specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Shutdown
**
*/
#include <assert.h>
#include <windows.h>
#include "../ref_gl/gl_local.h"
#include "../../client/windows_shared.h"

/*
** GLimp_SetMode
*/
int GLimp_SetMode(int mode, qboolean fullscreen)
{
	ri.Con_Printf( PRINT_ALL, "Initializing OpenGL display\n");

	// destroy the existing window
	if (GMainWindow)
	{
		GLimp_Shutdown ();
	}

	return GLW_SetMode(mode, r_colorbits->integer, fullscreen);
}

/*
** GLimp_EndFrame
** 
** Responsible for doing a swapbuffers and possibly for other stuff
** as yet to be determined.  Probably better not to make this a GLimp
** function and instead do a call to GLimp_SwapBuffers.
*/
void GLimp_EndFrame (void)
{
	int		err;

	err = qglGetError();
	assert( err == GL_NO_ERROR );

	if ( QStr::ICmp( gl_drawbuffer->string, "GL_BACK" ) == 0 )
	{
		GLimp_SwapBuffers();
	}
}
