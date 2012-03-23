/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/*
** GLW_IMP.C
**
** This file contains ALL Linux specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Init
** GLimp_Shutdown
** GLimp_SwitchFullscreen
** GLimp_SetGamma
**
*/

#include "../renderer/tr_local.h"
#include "../client/client.h"
#include "linux_local.h" // bk001130

// using our local glext.h
// http://oss.sgi.com/projects/ogl-sample/ABI/
#define GL_GLEXT_LEGACY
#define GLX_GLXEXT_LEGACY
#include <GL/glx.h>
#include "../renderer/glext.h"

const char *glx_extensions_string;

/*
* Find the first occurrence of find in s.
*/
// bk001130 - from cvs1.17 (mkv), const
// bk001130 - made first argument const
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
										   qboolean fullscreen ) {
	rserr_t err;

	err = (rserr_t)GLimp_SetMode( mode, 0, fullscreen );

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
	if ( Q_stristr( glConfig.extensions_string, "GL_S3_s3tc" ) ) {
		if ( r_ext_compressed_textures->value ) {
			glConfig.textureCompression = TC_S3TC;
			ri.Printf( PRINT_ALL, "...using GL_S3_s3tc\n" );
		} else
		{
			glConfig.textureCompression = TC_NONE;
			ri.Printf( PRINT_ALL, "...ignoring GL_S3_s3tc\n" );
		}
	} else
	{
		glConfig.textureCompression = TC_NONE;
		ri.Printf( PRINT_ALL, "...GL_S3_s3tc not found\n" );
	}

	// GL_EXT_texture_env_add
	glConfig.textureEnvAddAvailable = qfalse;
	if ( Q_stristr( glConfig.extensions_string, "EXT_texture_env_add" ) ) {
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

	// GL_ARB_multitexture
	qglMultiTexCoord2fARB = NULL;
	qglActiveTextureARB = NULL;
	qglClientActiveTextureARB = NULL;
	if ( Q_stristr( glConfig.extensions_string, "GL_ARB_multitexture" ) ) {
		if ( r_ext_multitexture->value ) {
			qglMultiTexCoord2fARB = ( PFNGLMULTITEXCOORD2FARBPROC ) GLimp_GetProcAddress("glMultiTexCoord2fARB" );
			qglActiveTextureARB = ( PFNGLACTIVETEXTUREARBPROC ) GLimp_GetProcAddress("glActiveTextureARB" );
			qglClientActiveTextureARB = ( PFNGLCLIENTACTIVETEXTUREARBPROC ) GLimp_GetProcAddress("glClientActiveTextureARB" );

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
	if ( Q_stristr( glConfig.extensions_string, "GL_EXT_compiled_vertex_array" ) ) {
		if ( r_ext_compiled_vertex_array->value ) {
			ri.Printf( PRINT_ALL, "...using GL_EXT_compiled_vertex_array\n" );
			qglLockArraysEXT = ( void ( APIENTRY * )( int, int ) )GLimp_GetProcAddress("glLockArraysEXT" );
			qglUnlockArraysEXT = ( void ( APIENTRY * )( void ) )GLimp_GetProcAddress("glUnlockArraysEXT" );
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
	if ( Q_stristr( glConfig.extensions_string, "GL_NV_fog_distance" ) ) {
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

	ri.Printf( PRINT_ALL, "Initializing GLX extensions\n" );

	// GLX_SGI_swap_control
	if ( Q_stristr( glx_extensions_string, "GLX_SGI_swap_control" ) ) {
		qglXSwapIntervalSGI = (int ( *)( int interval ))GLimp_GetProcAddress("glXSwapIntervalSGI");
		ri.Printf( PRINT_ALL, "...using GLX_SGI_swap_control\n" );
	} else {
		ri.Printf( PRINT_ALL, "... GLX_SGI_swap_control not found\n" );
		qglXSwapIntervalSGI = NULL;
	}
}

/*
** GLW_LoadOpenGL
**
** GLimp_win.c internal function that that attempts to load and use
** a specific OpenGL DLL.
*/
static qboolean GLW_LoadOpenGL() {
	qboolean fullscreen;

	// disable the 3Dfx splash screen and set gamma
	// we do this all the time, but it shouldn't hurt anything
	// on non-3Dfx stuff
	putenv( "FX_GLIDE_NO_SPLASH=0" );

	// Mesa VooDoo hacks
	putenv( "MESA_GLX_FX=fullscreen\n" );

	fullscreen = r_fullscreen->integer;

	// create the window and set up the context
	if ( !GLW_StartDriverAndSetMode( r_mode->integer, fullscreen ) ) {
		if ( r_mode->integer != 3 ) {
			if ( !GLW_StartDriverAndSetMode( 3, fullscreen ) ) {
				goto fail;
			}
		} else {
			goto fail;
		}
	}

	QGL_Init();
	return qtrue;
fail:

	QGL_Shutdown();

	return qfalse;
}

/*
** GLimp_Init
**
** This routine is responsible for initializing the OS specific portions
** of OpenGL.
*/
void GLimp_Init( void ) {
	char buf[1024];
	Cvar *lastValidRenderer = ri.Cvar_Get( "r_lastValidRenderer", "(uninitialized)", CVAR_ARCHIVE );

	InitSig();

	//
	// load and initialize the specific OpenGL driver
	//
	if ( !GLW_LoadOpenGL() ) {
		ri.Error( ERR_FATAL, "GLimp_Init() - could not load OpenGL subsystem\n" );
	}

	// This values force the UI to disable driver selection
	glConfig.driverType = GLDRV_ICD;
	glConfig.hardwareType = GLHW_GENERIC;

	// get our config strings
	String::NCpyZ( glConfig.vendor_string, (char*)qglGetString( GL_VENDOR ), sizeof( glConfig.vendor_string ) );
	String::NCpyZ( glConfig.renderer_string, (char*)qglGetString( GL_RENDERER ), sizeof( glConfig.renderer_string ) );
	if ( *glConfig.renderer_string && glConfig.renderer_string[String::Length( glConfig.renderer_string ) - 1] == '\n' ) {
		glConfig.renderer_string[String::Length( glConfig.renderer_string ) - 1] = 0;
	}
	String::NCpyZ( glConfig.version_string, (char*)qglGetString( GL_VERSION ), sizeof( glConfig.version_string ) );
	String::NCpyZ( glConfig.extensions_string, (char*)qglGetString( GL_EXTENSIONS ), sizeof( glConfig.extensions_string ) );
	// TTimo - safe check
	if ( String::Length( (char*)qglGetString( GL_EXTENSIONS ) ) >= sizeof( glConfig.extensions_string ) ) {
		Com_Printf( S_COLOR_YELLOW "WARNNING: GL extensions string too long (%d), truncated to %d\n", String::Length( (char*)qglGetString( GL_EXTENSIONS ) ), sizeof( glConfig.extensions_string ) );
	}

	//bani - glx extensions string
	glx_extensions_string = GLimp_GetSystemExtensionsString();

	//
	// chipset specific configuration
	//
	String::Cpy( buf, glConfig.renderer_string );
	strlwr( buf );

	//
	// NOTE: if changing cvars, do it within this block.  This allows them
	// to be overridden when testing driver fixes, etc. but only sets
	// them to their default state when the hardware is first installed/run.
	//
	if ( String::ICmp( lastValidRenderer->string, glConfig.renderer_string ) ) {
		glConfig.hardwareType = GLHW_GENERIC;

		ri.Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST" );

		// VOODOO GRAPHICS w/ 2MB
		if ( Q_stristr( buf, "voodoo graphics/1 tmu/2 mb" ) ) {
			ri.Cvar_Set( "r_picmip", "2" );
			ri.Cvar_Get( "r_picmip", "1", CVAR_ARCHIVE | CVAR_LATCH2 );
		} else
		{
			ri.Cvar_Set( "r_picmip", "1" );

			if ( Q_stristr( buf, "rage 128" ) || Q_stristr( buf, "rage128" ) ) {
				ri.Cvar_Set( "r_finish", "0" );
			}
			// Savage3D and Savage4 should always have trilinear enabled
			else if ( Q_stristr( buf, "savage3d" ) || Q_stristr( buf, "s3 savage4" ) ) {
				ri.Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
			}
		}
	}

	ri.Cvar_Set( "r_lastValidRenderer", glConfig.renderer_string );

	// initialize extensions
	GLW_InitExtensions();

	InitSig();

	return;
}


/*
** GLimp_EndFrame
**
** Responsible for doing a swapbuffers and possibly for other stuff
** as yet to be determined.  Probably better not to make this a GLimp
** function and instead do a call to GLimp_SwapBuffers.
*/
void GLimp_EndFrame( void ) {
	//
	// swapinterval stuff
	// bani: wtf, wgl lets you turn it off and glx doesnt?!
	//
	if ( r_swapInterval->modified ) {
		r_swapInterval->modified = qfalse;

		if ( !glConfig.stereoEnabled ) {    // why?
			if ( qglXSwapIntervalSGI ) {
				qglXSwapIntervalSGI( r_swapInterval->integer );
			}
		}
	}
	// don't flip if drawing to front buffer
	if ( String::ICmp( r_drawBuffer->string, "GL_FRONT" ) != 0 ) {
		GLimp_SwapBuffers();
	}

	// check logging
	QGL_EnableLogging( (qboolean)r_logFile->integer ); // bk001205 - was ->value
}
