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
** GLimp_Init
** GLimp_Shutdown
** GLimp_SwitchFullscreen
**
*/
#include <assert.h>
#include <windows.h>
#include "../ref_gl/gl_local.h"
#include "winquake.h"

rserr_t GLW_SharedSetMode(int colorbits, bool fullscreen);

static qboolean GLimp_SwitchFullscreen( int width, int height );

extern QCvar *vid_fullscreen;
extern QCvar *vid_ref;

/*
** GLimp_SetMode
*/
int GLimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen )
{
	ri.Con_Printf( PRINT_ALL, "Initializing OpenGL display\n");

	ri.Con_Printf (PRINT_ALL, "...setting mode %d:", mode );

	if ( !ri.Vid_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, mode ) )
	{
		ri.Con_Printf( PRINT_ALL, " invalid mode\n" );
		return RSERR_INVALID_MODE;
	}

	*pwidth = glConfig.vidWidth;
	*pheight = glConfig.vidHeight;

	// destroy the existing window
	if (GMainWindow)
	{
		GLimp_Shutdown ();
	}

	rserr_t r = GLW_SharedSetMode(gl_bitdepth->integer, fullscreen);
	if (r != RSERR_OK)
		return r;

	// let the sound and input subsystems know about the new window
	ri.Vid_NewWindow (glConfig.vidWidth, glConfig.vidHeight);

	return RSERR_OK;
}

/*
** GLimp_Shutdown
**
** This routine does all OS specific shutdown procedures for the OpenGL
** subsystem.  Under OpenGL this means NULLing out the current DC and
** HGLRC, deleting the rendering context, and releasing the DC acquired
** for the window.  The state structure is also nulled out.
**
*/
void GLimp_Shutdown( void )
{
	if ( !wglMakeCurrent( NULL, NULL ) )
		ri.Con_Printf( PRINT_ALL, "ref_gl::R_Shutdown() - wglMakeCurrent failed\n");
	if ( baseRC )
	{
		if (!wglDeleteContext(baseRC))
			ri.Con_Printf( PRINT_ALL, "ref_gl::R_Shutdown() - wglDeleteContext failed\n");
		baseRC = NULL;
	}
	if (maindc)
	{
		if ( !ReleaseDC( GMainWindow, maindc ) )
			ri.Con_Printf( PRINT_ALL, "ref_gl::R_Shutdown() - ReleaseDC failed\n" );
		maindc   = NULL;
	}
	if (GMainWindow)
	{
		DestroyWindow (	GMainWindow );
		GMainWindow = NULL;
	}

	UnregisterClass (WINDOW_CLASS_NAME, global_hInstance);

	if ( cdsFullscreen )
	{
		ChangeDisplaySettings( 0, 0 );
		cdsFullscreen = false;
	}
}


/*
** GLimp_Init
**
** This routine is responsible for initializing the OS specific portions
** of OpenGL.  Under Win32 this means dealing with the pixelformats and
** doing the wgl interface stuff.
*/
qboolean GLimp_Init(void*, void*)
{
	return true;
}

/*
** GLimp_BeginFrame
*/
void GLimp_BeginFrame( float camera_separation )
{
	if ( camera_separation < 0 && glConfig.stereoEnabled )
	{
		qglDrawBuffer( GL_BACK_LEFT );
	}
	else if ( camera_separation > 0 && glConfig.stereoEnabled )
	{
		qglDrawBuffer( GL_BACK_RIGHT );
	}
	else
	{
		qglDrawBuffer( GL_BACK );
	}
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
		if ( !SwapBuffers( maindc ) )
			ri.Sys_Error( ERR_FATAL, "GLimp_EndFrame() - SwapBuffers() failed!\n" );
	}
}

/*
** GLimp_AppActivate
*/
void GLimp_AppActivate( qboolean active )
{
	if ( active )
	{
		SetForegroundWindow( GMainWindow );
		ShowWindow( GMainWindow, SW_RESTORE );
	}
	else
	{
		if ( vid_fullscreen->value )
			ShowWindow( GMainWindow, SW_MINIMIZE );
	}
}
