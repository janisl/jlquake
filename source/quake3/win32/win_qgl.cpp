/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
/*
** QGL_WIN.C
**
** This file implements the operating system binding of GL to QGL function
** pointers.  When doing a port of Quake3 you must implement the following
** two functions:
**
** QGL_Init() - loads libraries, assigns function pointers, etc.
** QGL_Shutdown() - unloads libraries, NULLs function pointers
*/
#include <float.h>
#include "../renderer/tr_local.h"
#include "glw_win.h"

void QGL_EnableLogging( qboolean enable );

int ( WINAPI * qwglSwapIntervalEXT)( int interval );
BOOL  ( WINAPI * qwglGetDeviceGammaRamp3DFX)( HDC, LPVOID );
BOOL  ( WINAPI * qwglSetDeviceGammaRamp3DFX)( HDC, LPVOID );

int   ( WINAPI * qwglChoosePixelFormat )(HDC, CONST PIXELFORMATDESCRIPTOR *);
int   ( WINAPI * qwglDescribePixelFormat) (HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
int   ( WINAPI * qwglGetPixelFormat)(HDC);
BOOL  ( WINAPI * qwglSetPixelFormat)(HDC, int, CONST PIXELFORMATDESCRIPTOR *);
BOOL  ( WINAPI * qwglSwapBuffers)(HDC);

BOOL  ( WINAPI * qwglCopyContext)(HGLRC, HGLRC, UINT);
HGLRC ( WINAPI * qwglCreateContext)(HDC);
HGLRC ( WINAPI * qwglCreateLayerContext)(HDC, int);
BOOL  ( WINAPI * qwglDeleteContext)(HGLRC);
HGLRC ( WINAPI * qwglGetCurrentContext)(VOID);
HDC   ( WINAPI * qwglGetCurrentDC)(VOID);
PROC  ( WINAPI * qwglGetProcAddress)(LPCSTR);
BOOL  ( WINAPI * qwglMakeCurrent)(HDC, HGLRC);
BOOL  ( WINAPI * qwglShareLists)(HGLRC, HGLRC);
BOOL  ( WINAPI * qwglUseFontBitmaps)(HDC, DWORD, DWORD, DWORD);

BOOL  ( WINAPI * qwglUseFontOutlines)(HDC, DWORD, DWORD, DWORD, FLOAT,
                                           FLOAT, int, LPGLYPHMETRICSFLOAT);

BOOL ( WINAPI * qwglDescribeLayerPlane)(HDC, int, int, UINT,
                                            LPLAYERPLANEDESCRIPTOR);
int  ( WINAPI * qwglSetLayerPaletteEntries)(HDC, int, int, int,
                                                CONST COLORREF *);
int  ( WINAPI * qwglGetLayerPaletteEntries)(HDC, int, int, int,
                                                COLORREF *);
BOOL ( WINAPI * qwglRealizeLayerPalette)(HDC, int, BOOL);
BOOL ( WINAPI * qwglSwapLayerBuffers)(HDC, UINT);

//==========================================================================
//
//	QGL_Log
//
//==========================================================================

void QGL_Log(const char* Fmt, ...)
{
	va_list		ArgPtr;
	char		String[1024];
	
	va_start(ArgPtr, Fmt);
	vsprintf(String, Fmt, ArgPtr);
	va_end(ArgPtr);

	fprintf(glw_state.log_fp, "%s", String);
}

/*
** QGL_Shutdown
**
** Unloads the specified DLL then nulls out all the proc pointers.  This
** is only called during a hard shutdown of the OGL subsystem (e.g. vid_restart).
*/
void QGL_Shutdown( void )
{
	ri.Printf( PRINT_ALL, "...shutting down QGL\n" );

	if ( glw_state.hinstOpenGL )
	{
		ri.Printf( PRINT_ALL, "...unloading OpenGL DLL\n" );
		FreeLibrary( glw_state.hinstOpenGL );
	}

	glw_state.hinstOpenGL = NULL;

	QGL_SharedShutdown();

	qwglCopyContext              = NULL;
	qwglCreateContext            = NULL;
	qwglCreateLayerContext       = NULL;
	qwglDeleteContext            = NULL;
	qwglDescribeLayerPlane       = NULL;
	qwglGetCurrentContext        = NULL;
	qwglGetCurrentDC             = NULL;
	qwglGetLayerPaletteEntries   = NULL;
	qwglGetProcAddress           = NULL;
	qwglMakeCurrent              = NULL;
	qwglRealizeLayerPalette      = NULL;
	qwglSetLayerPaletteEntries   = NULL;
	qwglShareLists               = NULL;
	qwglSwapLayerBuffers         = NULL;
	qwglUseFontBitmaps           = NULL;
	qwglUseFontOutlines          = NULL;

	qwglChoosePixelFormat        = NULL;
	qwglDescribePixelFormat      = NULL;
	qwglGetPixelFormat           = NULL;
	qwglSetPixelFormat           = NULL;
	qwglSwapBuffers              = NULL;
}

#define GR_NUM_BOARDS 0x0f

static qboolean GlideIsValid( void )
{
	HMODULE hGlide;
//	int numBoards;
//	void (__stdcall *grGet)(unsigned int, unsigned int, int*);

    if ( ( hGlide = LoadLibrary("Glide3X") ) != 0 ) 
	{
		// FIXME: 3Dfx needs to fix this shit
		return qtrue;

#if 0
        grGet = (void *)GetProcAddress( hGlide, "_grGet@12");

		if ( grGet )
		{
	        grGet( GR_NUM_BOARDS, sizeof(int), &numBoards);
		}
		else
		{
			// if we've reached this point, something is seriously wrong
			ri.Printf( PRINT_WARNING, "WARNING: could not find grGet in GLIDE3X.DLL\n" );
			numBoards = 0;
		}

		FreeLibrary( hGlide );
		hGlide = NULL;

		if ( numBoards > 0 )
		{
			return qtrue;
		}

		ri.Printf( PRINT_WARNING, "WARNING: invalid Glide installation!\n" );
#endif
    }

	return qfalse;
} 

#	pragma warning (disable : 4113 4133 4047 )
#	define GPA( a ) GetProcAddress( glw_state.hinstOpenGL, a )

/*
** QGL_Init
**
** This is responsible for binding our qgl function pointers to 
** the appropriate GL stuff.  In Windows this means doing a 
** LoadLibrary and a bunch of calls to GetProcAddress.  On other
** operating systems we need to do the right thing, whatever that
** might be.
*/
qboolean QGL_Init( const char *dllname )
{
	char systemDir[1024];
	char libName[1024];

	GetSystemDirectory( systemDir, sizeof( systemDir ) );

	assert( glw_state.hinstOpenGL == 0 );

	ri.Printf( PRINT_ALL, "...initializing QGL\n" );

	// NOTE: this assumes that 'dllname' is lower case (and it should be)!
	if ( strstr( dllname, _3DFX_DRIVER_NAME ) )
	{
		if ( !GlideIsValid() )
		{
			ri.Printf( PRINT_ALL, "...WARNING: missing Glide installation, assuming no 3Dfx available\n" );
			return qfalse;
		}
	}

	if ( dllname[0] != '!' )
	{
		QStr::Sprintf( libName, sizeof( libName ), "%s\\%s", systemDir, dllname );
	}
	else
	{
		QStr::NCpyZ( libName, dllname, sizeof( libName ) );
	}

	ri.Printf( PRINT_ALL, "...calling LoadLibrary( '%s.dll' ): ", libName );

	if ( ( glw_state.hinstOpenGL = LoadLibrary( dllname ) ) == 0 )
	{
		ri.Printf( PRINT_ALL, "failed\n" );
		return qfalse;
	}
	ri.Printf( PRINT_ALL, "succeeded\n" );

	QGL_SharedInit();

	qwglCopyContext              = (BOOL  ( WINAPI *)(HGLRC, HGLRC, UINT))GPA( "wglCopyContext" );
	qwglCreateContext            = (HGLRC ( WINAPI *)(HDC))GPA( "wglCreateContext" );
	qwglCreateLayerContext       = (HGLRC ( WINAPI *)(HDC, int))GPA( "wglCreateLayerContext" );
	qwglDeleteContext            = (BOOL  ( WINAPI *)(HGLRC))GPA( "wglDeleteContext" );
	qwglDescribeLayerPlane       = (BOOL ( WINAPI *)(HDC, int, int, UINT, LPLAYERPLANEDESCRIPTOR))GPA( "wglDescribeLayerPlane" );
	qwglGetCurrentContext        = (HGLRC ( WINAPI *)(VOID))GPA( "wglGetCurrentContext" );
	qwglGetCurrentDC             = (HDC   ( WINAPI *)(VOID))GPA( "wglGetCurrentDC" );
	qwglGetLayerPaletteEntries   = (int  ( WINAPI *)(HDC, int, int, int, COLORREF *))GPA( "wglGetLayerPaletteEntries" );
	qwglGetProcAddress           = (PROC  ( WINAPI *)(LPCSTR))GPA( "wglGetProcAddress" );
	qwglMakeCurrent              = (BOOL  ( WINAPI *)(HDC, HGLRC))GPA( "wglMakeCurrent" );
	qwglRealizeLayerPalette      = (BOOL ( WINAPI *)(HDC, int, BOOL))GPA( "wglRealizeLayerPalette" );
	qwglSetLayerPaletteEntries   = (int  ( WINAPI *)(HDC, int, int, int, CONST COLORREF *))GPA( "wglSetLayerPaletteEntries" );
	qwglShareLists               = (BOOL  ( WINAPI *)(HGLRC, HGLRC))GPA( "wglShareLists" );
	qwglSwapLayerBuffers         = (BOOL ( WINAPI *)(HDC, UINT))GPA( "wglSwapLayerBuffers" );
	qwglUseFontBitmaps           = (BOOL  ( WINAPI *)(HDC, DWORD, DWORD, DWORD))GPA( "wglUseFontBitmapsA" );
	qwglUseFontOutlines          = (BOOL  ( WINAPI *)(HDC, DWORD, DWORD, DWORD, FLOAT, FLOAT, int, LPGLYPHMETRICSFLOAT))GPA( "wglUseFontOutlinesA" );

	qwglChoosePixelFormat        = (int   ( WINAPI *)(HDC, CONST PIXELFORMATDESCRIPTOR *))GPA( "wglChoosePixelFormat" );
	qwglDescribePixelFormat      = (int   ( WINAPI *) (HDC, int, UINT, LPPIXELFORMATDESCRIPTOR))GPA( "wglDescribePixelFormat" );
	qwglGetPixelFormat           = (int   ( WINAPI *)(HDC))GPA( "wglGetPixelFormat" );
	qwglSetPixelFormat           = (BOOL  ( WINAPI *)(HDC, int, CONST PIXELFORMATDESCRIPTOR *))GPA( "wglSetPixelFormat" );
	qwglSwapBuffers              = (BOOL  ( WINAPI *)(HDC))GPA( "wglSwapBuffers" );

	qwglSwapIntervalEXT = 0;
	qglActiveTextureARB = 0;
	qglClientActiveTextureARB = 0;
	qglMultiTexCoord2fARB = 0;
	qglLockArraysEXT = 0;
	qglUnlockArraysEXT = 0;
	qwglGetDeviceGammaRamp3DFX = NULL;
	qwglSetDeviceGammaRamp3DFX = NULL;

	// check logging
	QGL_EnableLogging( r_logFile->integer );

	return qtrue;
}

void QGL_EnableLogging( qboolean enable )
{
	static qboolean isEnabled;

	// return if we're already active
	if ( isEnabled && enable ) {
		// decrement log counter and stop if it has reached 0
		ri.Cvar_Set( "r_logFile", va("%d", r_logFile->integer - 1 ) );
		if ( r_logFile->integer ) {
			return;
		}
		enable = qfalse;
	}

	// return if we're already disabled
	if ( !enable && !isEnabled )
		return;

	isEnabled = enable;

	if ( enable )
	{
		if ( !glw_state.log_fp )
		{
			struct tm *newtime;
			time_t aclock;
			char buffer[1024];
			QCvar	*basedir;

			time( &aclock );
			newtime = localtime( &aclock );

			asctime( newtime );

			basedir = ri.Cvar_Get( "fs_basepath", "", 0 );
			QStr::Sprintf( buffer, sizeof(buffer), "%s/gl.log", basedir->string ); 
			glw_state.log_fp = fopen( buffer, "wt" );

			fprintf( glw_state.log_fp, "%s\n", asctime( newtime ) );
		}

		QGL_SharedLogOn();
	}
	else
	{
		if ( glw_state.log_fp )	{
			fprintf( glw_state.log_fp, "*** CLOSING LOG ***\n" );
			fclose( glw_state.log_fp );
			glw_state.log_fp = NULL;
		}
		QGL_SharedLogOff();
	}
}

#pragma warning (default : 4113 4133 4047 )



