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

extern fileHandle_t log_fp;

void QGL_Log(const char* Fmt, ...);

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
}

#define GR_NUM_BOARDS 0x0f

static qboolean GlideIsValid( void )
{
	HMODULE hGlide;

    if ( ( hGlide = LoadLibrary("Glide3X") ) != 0 ) 
	{
		// FIXME: 3Dfx needs to fix this shit
		return qtrue;
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
	if ( isEnabled && enable )
	{
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
		QGL_SharedLogOn();
	}
	else
	{
		QGL_SharedLogOff();
	}
}

/*
** GLimp_LogComment
*/
void GLimp_LogComment( char *comment ) 
{
	if (log_fp)
	{
		QGL_Log("%s", comment );
	}
}

#pragma warning (default : 4113 4133 4047 )
