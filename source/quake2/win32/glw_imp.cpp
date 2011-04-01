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
#include "glw_win.h"
#include "winquake.h"

static qboolean GLimp_SwitchFullscreen( int width, int height );
qboolean GLimp_InitGL (void);

glwstate_t glw_state;

extern QCvar *vid_fullscreen;
extern QCvar *vid_ref;

static qboolean VerifyDriver( void )
{
	char buffer[1024];

	QStr::Cpy( buffer, (char*)qglGetString( GL_RENDERER ) );
	QStr::ToLower( buffer );
	if ( QStr::Cmp( buffer, "gdi generic" ) == 0 )
		if ( !glw_state.mcd_accelerated )
			return false;
	return true;
}

/*
** VID_CreateWindow
*/
qboolean VID_CreateWindow( int width, int height, qboolean fullscreen )
{
	GLW_SharedCreateWindow(width, height, fullscreen);

	// init all the gl stuff for the window
	if (!GLimp_InitGL ())
	{
		ri.Con_Printf( PRINT_ALL, "VID_CreateWindow() - GLimp_InitGL failed\n");
		return false;
	}

	SetForegroundWindow( GMainWindow );
	SetFocus( GMainWindow );

	// let the sound and input subsystems know about the new window
	ri.Vid_NewWindow (width, height);

	return true;
}


/*
** GLimp_SetMode
*/
int GLimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen )
{
	int width, height;
	const char *win_fs[] = { "W", "FS" };

	//
	// check our desktop attributes
	//
	HDC hDC = GetDC(GetDesktopWindow());
	desktopBitsPixel = GetDeviceCaps(hDC, BITSPIXEL);
	desktopWidth = GetDeviceCaps(hDC, HORZRES);
	desktopHeight = GetDeviceCaps(hDC, VERTRES);
	ReleaseDC(GetDesktopWindow(), hDC);

	ri.Con_Printf( PRINT_ALL, "Initializing OpenGL display\n");

	ri.Con_Printf (PRINT_ALL, "...setting mode %d:", mode );

	if ( !ri.Vid_GetModeInfo( &width, &height, mode ) )
	{
		ri.Con_Printf( PRINT_ALL, " invalid mode\n" );
		return RSERR_INVALID_MODE;
	}

	ri.Con_Printf( PRINT_ALL, " %d %d %s\n", width, height, win_fs[fullscreen] );

	// destroy the existing window
	if (GMainWindow)
	{
		GLimp_Shutdown ();
	}

	// do a CDS if needed
	if ( fullscreen )
	{
		DEVMODE dm;

		ri.Con_Printf( PRINT_ALL, "...attempting fullscreen\n" );

		Com_Memset( &dm, 0, sizeof( dm ) );

		dm.dmSize = sizeof( dm );

		dm.dmPelsWidth  = width;
		dm.dmPelsHeight = height;
		dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;

		if ( gl_bitdepth->value != 0 )
		{
			dm.dmBitsPerPel = gl_bitdepth->value;
			dm.dmFields |= DM_BITSPERPEL;
			ri.Con_Printf( PRINT_ALL, "...using gl_bitdepth of %d\n", ( int ) gl_bitdepth->value );
		}
		else
		{
			HDC hdc = GetDC( NULL );
			int bitspixel = GetDeviceCaps( hdc, BITSPIXEL );

			ri.Con_Printf( PRINT_ALL, "...using desktop display depth of %d\n", bitspixel );

			ReleaseDC( 0, hdc );
		}

		ri.Con_Printf( PRINT_ALL, "...calling CDS: " );
		if ( ChangeDisplaySettings( &dm, CDS_FULLSCREEN ) == DISP_CHANGE_SUCCESSFUL )
		{
			*pwidth = width;
			*pheight = height;

			gl_state.fullscreen = true;

			ri.Con_Printf( PRINT_ALL, "ok\n" );

			if ( !VID_CreateWindow (width, height, true) )
				return RSERR_INVALID_MODE;

			return RSERR_OK;
		}
		else
		{
			*pwidth = width;
			*pheight = height;

			ri.Con_Printf( PRINT_ALL, "failed\n" );

			ri.Con_Printf( PRINT_ALL, "...calling CDS assuming dual monitors:" );

			dm.dmPelsWidth = width * 2;
			dm.dmPelsHeight = height;
			dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

			if ( gl_bitdepth->value != 0 )
			{
				dm.dmBitsPerPel = gl_bitdepth->value;
				dm.dmFields |= DM_BITSPERPEL;
			}

			/*
			** our first CDS failed, so maybe we're running on some weird dual monitor
			** system 
			*/
			if ( ChangeDisplaySettings( &dm, CDS_FULLSCREEN ) != DISP_CHANGE_SUCCESSFUL )
			{
				ri.Con_Printf( PRINT_ALL, " failed\n" );

				ri.Con_Printf( PRINT_ALL, "...setting windowed mode\n" );

				ChangeDisplaySettings( 0, 0 );

				*pwidth = width;
				*pheight = height;
				gl_state.fullscreen = false;
				if ( !VID_CreateWindow (width, height, false) )
					return RSERR_INVALID_MODE;
				return RSERR_INVALID_FULLSCREEN;
			}
			else
			{
				ri.Con_Printf( PRINT_ALL, " ok\n" );
				if ( !VID_CreateWindow (width, height, true) )
					return RSERR_INVALID_MODE;

				gl_state.fullscreen = true;
				return RSERR_OK;
			}
		}
	}
	else
	{
		ri.Con_Printf( PRINT_ALL, "...setting windowed mode\n" );

		ChangeDisplaySettings( 0, 0 );

		*pwidth = width;
		*pheight = height;
		gl_state.fullscreen = false;
		if ( !VID_CreateWindow (width, height, false) )
			return RSERR_INVALID_MODE;
	}

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

	if ( gl_state.fullscreen )
	{
		ChangeDisplaySettings( 0, 0 );
		gl_state.fullscreen = false;
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
#define OSR2_BUILD_NUMBER 1111

	OSVERSIONINFO	vinfo;

	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	glw_state.allowdisplaydepthchange = false;

	if ( GetVersionEx( &vinfo) )
	{
		if ( vinfo.dwMajorVersion > 4 )
		{
			glw_state.allowdisplaydepthchange = true;
		}
		else if ( vinfo.dwMajorVersion == 4 )
		{
			if ( vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
			{
				glw_state.allowdisplaydepthchange = true;
			}
			else if ( vinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
			{
				if ( LOWORD( vinfo.dwBuildNumber ) >= OSR2_BUILD_NUMBER )
				{
					glw_state.allowdisplaydepthchange = true;
				}
			}
		}
	}
	else
	{
		ri.Con_Printf( PRINT_ALL, "GLimp_Init() - GetVersionEx failed\n" );
		return false;
	}

	return true;
}

qboolean GLimp_InitGL (void)
{
    PIXELFORMATDESCRIPTOR pfd;
	QCvar *stereo;
	
	stereo = Cvar_Get( "cl_stereo", "0", 0 );

	GLW_CreatePFD(&pfd, 24, 32, 0, stereo->value != 0);

	/*
	** Get a DC for the specified window
	*/
	if ( maindc != NULL )
		ri.Con_Printf( PRINT_ALL, "GLimp_Init() - non-NULL DC exists\n" );

    if ( ( maindc = GetDC( GMainWindow ) ) == NULL )
	{
		ri.Con_Printf( PRINT_ALL, "GLimp_Init() - GetDC failed\n" );
		return false;
	}

	if (GLW_MakeContext(&pfd))
		goto fail;

	if ( !( pfd.dwFlags & PFD_GENERIC_ACCELERATED ) )
	{
		if ( r_allowSoftwareGL->value )
			glw_state.mcd_accelerated = true;
		else
			glw_state.mcd_accelerated = false;
	}
	else
	{
		glw_state.mcd_accelerated = true;
	}

	/*
	** report if stereo is desired but unavailable
	*/
	if ( !( pfd.dwFlags & PFD_STEREO ) && ( stereo->value != 0 ) ) 
	{
		ri.Con_Printf( PRINT_ALL, "...failed to select stereo pixel format\n" );
		Cvar_SetValueLatched( "cl_stereo", 0 );
		glConfig.stereoEnabled = false;
	}

	if ( !VerifyDriver() )
	{
		ri.Con_Printf( PRINT_ALL, "GLimp_Init() - no hardware acceleration detected\n" );
		goto fail;
	}

	/*
	** print out PFD specifics 
	*/
	ri.Con_Printf( PRINT_ALL, "GL PFD: color(%d-bits) Z(%d-bit)\n", ( int ) pfd.cColorBits, ( int ) pfd.cDepthBits );

	return true;

fail:
	if ( maindc )
	{
		ReleaseDC( GMainWindow, maindc );
		maindc = NULL;
	}
	return false;
}

/*
** GLimp_BeginFrame
*/
void GLimp_BeginFrame( float camera_separation )
{
	if ( gl_bitdepth->modified )
	{
		if ( gl_bitdepth->value != 0 && !glw_state.allowdisplaydepthchange )
		{
			Cvar_SetValueLatched( "gl_bitdepth", 0 );
			ri.Con_Printf( PRINT_ALL, "gl_bitdepth requires Win95 OSR2.x or WinNT 4.x\n" );
		}
		gl_bitdepth->modified = false;
	}

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
