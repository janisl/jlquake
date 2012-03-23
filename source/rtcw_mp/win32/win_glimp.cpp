#include <assert.h>
#include "../renderer/tr_local.h"
#include "../qcommon/qcommon.h"
#include "resource.h"
#include "win_local.h"

static void     GLW_InitExtensions( void );

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

	ri.Printf( PRINT_ALL, "Initializing OpenGL extensions\n" );

	// GL_S3_s3tc
	glConfig.textureCompression = TC_NONE;
	// RF, check for GL_EXT_texture_compression_s3tc
	if ( strstr( glConfig.extensions_string, "GL_EXT_texture_compression_s3tc" ) ) {
		if ( r_ext_compressed_textures->integer ) {
			glConfig.textureCompression = TC_EXT_COMP_S3TC;
			ri.Printf( PRINT_ALL, "...using GL_EXT_texture_compression_s3tc\n" );
		} else
		{
			glConfig.textureCompression = TC_NONE;
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_compression_s3tc\n" );
		}
	}
	/* RF, disabled this section, since this method produces very ugly results on nvidia hardware
	else if ( strstr( glConfig.extensions_string, "GL_S3_s3tc" ) )
	{
		if ( r_ext_compressed_textures->integer )
		{
			glConfig.textureCompression = TC_S3TC;
			ri.Printf( PRINT_ALL, "...using GL_S3_s3tc\n" );
		}
		else
		{
			glConfig.textureCompression = TC_NONE;
			ri.Printf( PRINT_ALL, "...ignoring GL_S3_s3tc\n" );
		}
	}
	*/
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_compression_s3tc not found\n" );
	}

	// GL_EXT_texture_env_add
	glConfig.textureEnvAddAvailable = qfalse;
	if ( strstr( glConfig.extensions_string, "EXT_texture_env_add" ) ) {
		if ( r_ext_texture_env_add->integer ) {
			glConfig.textureEnvAddAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_EXT_texture_env_add\n" );
		} else
		{
			glConfig.textureEnvAddAvailable = qfalse;
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_env_add\n" );
		}
	} else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_env_add not found\n" );
	}

	// WGL_EXT_swap_control
	qwglSwapIntervalEXT = ( BOOL ( WINAPI * )(int) )GLimp_GetProcAddress( "wglSwapIntervalEXT" );
	if ( qwglSwapIntervalEXT ) {
		ri.Printf( PRINT_ALL, "...using WGL_EXT_swap_control\n" );
		r_swapInterval->modified = qtrue;   // force a set next frame
	} else
	{
		ri.Printf( PRINT_ALL, "...WGL_EXT_swap_control not found\n" );
	}

	// GL_ARB_multitexture
	qglMultiTexCoord2fARB = NULL;
	qglActiveTextureARB = NULL;
	qglClientActiveTextureARB = NULL;
	if ( strstr( glConfig.extensions_string, "GL_ARB_multitexture" )  ) {
		if ( r_ext_multitexture->integer ) {
			qglMultiTexCoord2fARB = ( PFNGLMULTITEXCOORD2FARBPROC ) GLimp_GetProcAddress( "glMultiTexCoord2fARB" );
			qglActiveTextureARB = ( PFNGLACTIVETEXTUREARBPROC ) GLimp_GetProcAddress( "glActiveTextureARB" );
			qglClientActiveTextureARB = ( PFNGLCLIENTACTIVETEXTUREARBPROC ) GLimp_GetProcAddress( "glClientActiveTextureARB" );

			if ( qglActiveTextureARB ) {
				qglGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &glConfig.maxActiveTextures );

				if ( glConfig.maxActiveTextures > 1 ) {
					ri.Printf( PRINT_ALL, "...using GL_ARB_multitexture\n" );
				} else
				{
					qglMultiTexCoord2fARB = NULL;
					qglActiveTextureARB = NULL;
					qglClientActiveTextureARB = NULL;
					ri.Printf( PRINT_ALL, "...not using GL_ARB_multitexture, < 2 texture units\n" );
				}
			}
		} else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_ARB_multitexture\n" );
		}
	} else
	{
		ri.Printf( PRINT_ALL, "...GL_ARB_multitexture not found\n" );
	}

	// GL_EXT_compiled_vertex_array
	qglLockArraysEXT = NULL;
	qglUnlockArraysEXT = NULL;
	if ( strstr( glConfig.extensions_string, "GL_EXT_compiled_vertex_array" ) ) {
		if ( r_ext_compiled_vertex_array->integer ) {
			ri.Printf( PRINT_ALL, "...using GL_EXT_compiled_vertex_array\n" );
			qglLockArraysEXT = ( void ( APIENTRY * )( int, int ) )GLimp_GetProcAddress( "glLockArraysEXT" );
			qglUnlockArraysEXT = ( void ( APIENTRY * )( void ) )GLimp_GetProcAddress( "glUnlockArraysEXT" );
			if ( !qglLockArraysEXT || !qglUnlockArraysEXT ) {
				ri.Error( ERR_FATAL, "bad getprocaddress" );
			}
		} else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_compiled_vertex_array\n" );
		}
	} else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_compiled_vertex_array not found\n" );
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
		ri.Error( ERR_FATAL, "GLW_StartOpenGL() - could not load OpenGL subsystem\n" );
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
