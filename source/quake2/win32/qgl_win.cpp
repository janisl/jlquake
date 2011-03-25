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
** QGL_WIN.C
**
** This file implements the operating system binding of GL to QGL function
** pointers.  When doing a port of Quake2 you must implement the following
** two functions:
**
** QGL_Init() - loads libraries, assigns function pointers, etc.
** QGL_Shutdown() - unloads libraries, NULLs function pointers
*/
#include <float.h>
#include "../ref_gl/gl_local.h"
#include "glw_win.h"

void ( APIENTRY * qglLockArraysEXT)( int, int);
void ( APIENTRY * qglUnlockArraysEXT) ( void );

BOOL ( WINAPI * qwglSwapIntervalEXT)( int interval );
BOOL ( WINAPI * qwglGetDeviceGammaRampEXT)( unsigned char *, unsigned char *, unsigned char * );
BOOL ( WINAPI * qwglSetDeviceGammaRampEXT)( const unsigned char *, const unsigned char *, const unsigned char * );
void ( APIENTRY * qglPointParameterfEXT)( GLenum param, GLfloat value );
void ( APIENTRY * qglPointParameterfvEXT)( GLenum param, const GLfloat *value );
void ( APIENTRY * qglColorTableEXT)( int, int, int, int, int, const void * );
void ( APIENTRY * qglSelectTextureSGIS)( GLenum );
void ( APIENTRY * qglMTexCoord2fSGIS)( GLenum, GLfloat, GLfloat );

extern fileHandle_t log_fp;

void QGL_Log(const char* Fmt, ...);

/*
** QGL_Shutdown
**
** Unloads the specified DLL then nulls out all the proc pointers.
*/
void QGL_Shutdown( void )
{
	if ( glw_state.hinstOpenGL )
	{
		FreeLibrary( glw_state.hinstOpenGL );
		glw_state.hinstOpenGL = NULL;
	}

	glw_state.hinstOpenGL = NULL;

	if (log_fp )
	{
		FS_FCloseFile(log_fp);
		log_fp = 0;
	}

	QGL_SharedShutdown();

	qwglSwapIntervalEXT	= NULL;

	qwglGetDeviceGammaRampEXT = NULL;
	qwglSetDeviceGammaRampEXT = NULL;
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
** 
*/
qboolean QGL_Init( const char *dllname )
{
	// update 3Dfx gamma irrespective of underlying DLL
	{
		char envbuffer[1024];
		float g;

		g = 2.00 * ( 0.8 - ( vid_gamma->value - 0.5 ) ) + 1.0F;
		QStr::Sprintf( envbuffer, sizeof(envbuffer), "SSTV2_GAMMA=%f", g );
		putenv( envbuffer );
		QStr::Sprintf( envbuffer, sizeof(envbuffer), "SST_GAMMA=%f", g );
		putenv( envbuffer );
	}

	if ( ( glw_state.hinstOpenGL = LoadLibrary( dllname ) ) == 0 )
	{
		char *buf = NULL;

		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &buf, 0, NULL);
		ri.Con_Printf( PRINT_ALL, "%s\n", buf );
		return false;
	}

	gl_config.allow_cds = true;

	QGL_SharedInit();

	qwglSwapIntervalEXT = 0;
	qglPointParameterfEXT = 0;
	qglPointParameterfvEXT = 0;
	qglColorTableEXT = 0;
	qglSelectTextureSGIS = 0;
	qglMTexCoord2fSGIS = 0;

	return true;
}

void GLimp_EnableLogging( qboolean enable )
{
	if ( enable )
	{
		if (!log_fp)
		{
			struct tm *newtime;
			time_t aclock;

			time( &aclock );
			newtime = localtime( &aclock );

			asctime( newtime );

			log_fp = FS_FOpenFileWrite("gl.log");

			QGL_Log("%s\n", asctime(newtime));
		}

		QGL_SharedLogOn();
	}
	else
	{
		QGL_SharedLogOff();
	}
}


void GLimp_LogNewFrame( void )
{
	QGL_Log("*** R_BeginFrame ***\n");
}

#pragma warning (default : 4113 4133 4047 )



