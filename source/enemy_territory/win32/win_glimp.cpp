#include <assert.h>
#include "../renderer/tr_local.h"
#include "../qcommon/qcommon.h"
#include "resource.h"
#include "win_local.h"

static void     GLW_InitExtensions( void );

/*
* Find the first occurrence of find in s.
*/
// bk001130 - from cvs1.17 (mkv), const
// bk001130 - made first argument const
// fretn - comes from linux_glimp.c msvc really needs this (well ... mine does \o/ )
static const char *Q_stristr( const char *s, const char *find ) {
	register char c, sc;
	register size_t len;

	if ( ( c = *find++ ) != 0 ) {
		if ( c >= 'a' && c <= 'z' ) {
			c -= ( 'a' - 'A' );
		}
		len = String::Length( find );
		do
		{
			do
			{
				if ( ( sc = *s++ ) == 0 ) {
					return NULL;
				}
				if ( sc >= 'a' && sc <= 'z' ) {
					sc -= ( 'a' - 'A' );
				}
			} while ( sc != c );
		} while ( String::NICmp( s, find, len ) != 0 );
		s--;
	}
	return s;
}

/*
** GLW_StartDriverAndSetMode
*/
static qboolean GLW_StartDriverAndSetMode( int mode,
										   int colorbits,
										   qboolean _cdsFullscreen ) {
	rserr_t err;

	err = GLimp_SetMode( r_mode->integer, colorbits, _cdsFullscreen );

	switch ( err )
	{
	case RSERR_INVALID_FULLSCREEN:
		ri.Printf( PRINT_ALL, "...WARNING: fullscreen unavailable in this mode\n" );
		return qfalse;
	case RSERR_INVALID_MODE:
		ri.Printf( PRINT_ALL, "...WARNING: could not set the given mode (%d)\n", mode );
		return qfalse;
	default:
		break;
	}
	return qtrue;
}

/*
** GLW_InitExtensions
*/
static void GLW_InitExtensions( void ) {
	if ( !r_allowExtensions->integer ) {
		ri.Printf( PRINT_ALL, "*** IGNORING OPENGL EXTENSIONS ***\n" );
		return;
	}

	// GL_NV_fog_distance
	if ( strstr( glConfig.extensions_string, "GL_NV_fog_distance" ) ) {
		if ( r_ext_NV_fog_dist->integer ) {
			glConfig.NVFogAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_NV_fog_distance\n" );
		} else {
			ri.Printf( PRINT_ALL, "...ignoring GL_NV_fog_distance\n" );
			ri.Cvar_Set( "r_ext_NV_fog_dist", "0" );
		}
	} else {
		ri.Printf( PRINT_ALL, "...GL_NV_fog_distance not found\n" );
		ri.Cvar_Set( "r_ext_NV_fog_dist", "0" );
	}

	// GL_EXT_texture_filter_anisotropic
	if ( Q_stristr( glConfig.extensions_string, "GL_EXT_texture_filter_anisotropic" ) ) {
		if ( r_ext_texture_filter_anisotropic->integer ) {
			glConfig.anisotropicAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_EXT_texture_filter_anisotropic\n" );
		} else {
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_filter_anisotropic\n" );
			ri.Cvar_Set( "r_ext_texture_filter_anisotropic", "0" );
		}
	} else {
		ri.Printf( PRINT_ALL, "... GL_EXT_texture_filter_anisotropic not found\n" );
		ri.Cvar_Set( "r_ext_texture_filter_anisotropic", "0" );
	}
}

/*
** GLW_LoadOpenGL
**
** GLimp_win.c internal function that attempts to load and use
** a specific OpenGL DLL.
*/
static qboolean GLW_LoadOpenGL() {
	qboolean _cdsFullscreen;

	//
	// determine if we're on a standalone driver
	//
	glConfig.driverType = GLDRV_ICD;

	_cdsFullscreen = r_fullscreen->integer;

	// create the window and set up the context
	if ( !GLW_StartDriverAndSetMode( r_mode->integer, r_colorbits->integer, _cdsFullscreen ) ) {
		// if we're on a 24/32-bit desktop and we're going fullscreen on an ICD,
		// try it again but with a 16-bit desktop
		if ( r_colorbits->integer != 16 ||
				_cdsFullscreen != qtrue ||
				r_mode->integer != 3 ) {
			if ( !GLW_StartDriverAndSetMode( 3, 16, qtrue ) ) {
				goto fail;
			}
		}
	}

	QGL_Init();
	return qtrue;
fail:

	QGL_Shutdown();

	return qfalse;
}

/*
** GLimp_EndFrame
*/
void GLimp_EndFrame( void ) {
	//
	// swapinterval stuff
	//
	if ( r_swapInterval->modified ) {
		r_swapInterval->modified = qfalse;

		if ( !glConfig.stereoEnabled ) {    // why?
			if ( qwglSwapIntervalEXT ) {
				qwglSwapIntervalEXT( r_swapInterval->integer );
			}
		}
	}


	// don't flip if drawing to front buffer
	if ( String::ICmp( r_drawBuffer->string, "GL_FRONT" ) != 0 ) {
		GLimp_SwapBuffers();
	}

	// check logging
	QGL_EnableLogging( r_logFile->integer );
}


static void GLW_StartOpenGL( void ) {
	//
	// load and initialize the specific OpenGL driver
	//
	if ( !GLW_LoadOpenGL() ) {
		ri.Error( ERR_VID_FATAL, "GLW_StartOpenGL() - could not load OpenGL subsystem\n" );
	}
}

/*
** GLimp_Init
**
** This is the platform specific OpenGL initialization function.  It
** is responsible for loading OpenGL, initializing it, setting
** extensions, creating a window of the appropriate size, doing
** fullscreen manipulations, etc.  Its overall responsibility is
** to make sure that a functional OpenGL subsystem is operating
** when it returns to the ref.
*/
void GLimp_Init( void ) {
	char buf[1024];
	Cvar *lastValidRenderer = ri.Cvar_Get( "r_lastValidRenderer", "(uninitialized)", CVAR_ARCHIVE );
	Cvar  *cv;

	ri.Printf( PRINT_ALL, "Initializing OpenGL subsystem\n" );

	// load appropriate DLL and initialize subsystem
	GLW_StartOpenGL();

	// get our config strings
	String::NCpyZ( glConfig.vendor_string, (char*)qglGetString( GL_VENDOR ), sizeof( glConfig.vendor_string ) );
	String::NCpyZ( glConfig.renderer_string, (char*)qglGetString( GL_RENDERER ), sizeof( glConfig.renderer_string ) );
	String::NCpyZ( glConfig.version_string, (char*)qglGetString( GL_VERSION ), sizeof( glConfig.version_string ) );
	String::NCpyZ( glConfig.extensions_string, (char*)qglGetString( GL_EXTENSIONS ), sizeof( glConfig.extensions_string ) );
	// TTimo - safe check
	if ( String::Length( (char*)qglGetString( GL_EXTENSIONS ) ) >= sizeof( glConfig.extensions_string ) ) {
		Com_Printf( S_COLOR_YELLOW "WARNNING: GL extensions string too long (%d), truncated to %d\n", String::Length( (char*)qglGetString( GL_EXTENSIONS ) ), sizeof( glConfig.extensions_string ) );
	}

	//
	// chipset specific configuration
	//
	String::NCpyZ( buf, glConfig.renderer_string, sizeof( buf ) );
	String::ToLower( buf );

	//
	// NOTE: if changing cvars, do it within this block.  This allows them
	// to be overridden when testing driver fixes, etc. but only sets
	// them to their default state when the hardware is first installed/run.
	//
	if ( String::ICmp( lastValidRenderer->string, glConfig.renderer_string ) ) {
		glConfig.hardwareType = GLHW_GENERIC;

		ri.Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST" );

		// VOODOO GRAPHICS w/ 2MB
		if ( strstr( buf, "voodoo graphics/1 tmu/2 mb" ) ) {
			ri.Cvar_Set( "r_picmip", "2" );
			ri.Cvar_Get( "r_picmip", "1", CVAR_ARCHIVE | CVAR_LATCH2 );
		} else
		{

//----(SA)	FIXME: RETURN TO DEFAULT  Another id build change for DK/DM
			ri.Cvar_Set( "r_picmip", "1" );   //----(SA)	was "1" // JPW NERVE back to 1
//----(SA)

			if ( strstr( buf, "rage 128" ) || strstr( buf, "rage128" ) ) {
				ri.Cvar_Set( "r_finish", "0" );
			}
			// Savage3D and Savage4 should always have trilinear enabled
			else if ( strstr( buf, "savage3d" ) || strstr( buf, "s3 savage4" ) ) {
				ri.Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
			}
		}
	}

	ri.Cvar_Set( "r_lastValidRenderer", glConfig.renderer_string );

	GLW_InitExtensions();
}
