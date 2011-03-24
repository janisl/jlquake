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
#define QGL
#include "../ref_gl/gl_local.h"

static fileHandle_t log_fp;

void ( APIENTRY * qglLockArraysEXT)( int, int);
void ( APIENTRY * qglUnlockArraysEXT) ( void );

void ( APIENTRY * qglPointParameterfEXT)( GLenum param, GLfloat value );
void ( APIENTRY * qglPointParameterfvEXT)( GLenum param, const GLfloat *value );
void ( APIENTRY * qglColorTableEXT)( int, int, int, int, int, const void * );
void ( APIENTRY * qglSelectTextureSGIS)( GLenum );
void ( APIENTRY * qglMTexCoord2fSGIS)( GLenum, GLfloat, GLfloat );

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

	FS_Printf(log_fp, "%s", String);
}

/*
** QGL_Shutdown
**
** Unloads the specified DLL then nulls out all the proc pointers.
*/
void QGL_Shutdown( void )
{
	QGL_SharedShutdown();
}

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
	gl_config.allow_cds = true;

	QGL_SharedInit();

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
		if ( !log_fp )
		{
			struct tm *newtime;
			time_t aclock;

			time( &aclock );
			newtime = localtime( &aclock );

			asctime( newtime );

			log_fp = FS_FOpenFileWrite("gl.log");

			FS_Printf( log_fp, "%s\n", asctime( newtime ) );
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
	FS_Printf( log_fp, "*** R_BeginFrame ***\n");
}


