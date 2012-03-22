/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

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

#include <termios.h>
#include <sys/ioctl.h>
#ifdef __linux__
  #include <sys/stat.h>
  #include <sys/vt.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>

// bk001204
#include <dlfcn.h>

// bk001206 - from my Heretic2 by way of Ryan's Fakk2
// Needed for the new X11_PendingInput() function.
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "../renderer/tr_local.h"
#include "../client/client.h"
#include "linux_local.h" // bk001130

#include "unix_glw.h"

#include <GL/glx.h>

#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include <X11/extensions/xf86dga.h>
#include <X11/extensions/xf86vmode.h>
#include "../../client/unix_shared.h"

#define WINDOW_CLASS_NAME   "Return to Castle Wolfenstein"

typedef enum
{
	RSERR_OK,

	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,

	RSERR_UNKNOWN
} rserr_t;

glwstate_t glw_state;

static int scrnum;
static GLXContext ctx = NULL;

Cvar  *r_previousglDriver;

qboolean vidmode_ext = qfalse;
static int vidmode_MajorVersion = 0, vidmode_MinorVersion = 0; // major and minor of XF86VidExtensions

// gamma value of the X display before we start playing with it
static XF86VidModeGamma vidmode_InitialGamma;

static int win_x, win_y;

static XF86VidModeModeInfo **vidmodes;
//static int default_dotclock_vidmode; // bk001204 - unused
static int num_vidmodes;
static qboolean vidmode_active = qfalse;

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

// NOTE TTimo for the tty console input, we didn't rely on those ..
//   it's not very surprising actually cause they are not used otherwise
void KBD_Init( void ) {
}

void KBD_Close( void ) {
}

/*****************************************************************************/

/*
** GLimp_SetGamma
**
** This routine should only be called if glConfig.deviceSupportsGamma is TRUE
*/
void GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] ) {
	// NOTE TTimo we get the gamma value from cvar, because we can't work with the s_gammatable
	//   the API wasn't changed to avoid breaking other OSes
	float g = Cvar_Get( "r_gamma", "1.0", 0 )->value;
	XF86VidModeGamma gamma;
	assert( glConfig.deviceSupportsGamma );
	gamma.red = g;
	gamma.green = g;
	gamma.blue = g;
	XF86VidModeSetGamma( dpy, scrnum, &gamma );
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
void GLimp_Shutdown( void ) {
	if ( !ctx || !dpy ) {
		return;
	}
	IN_DeactivateMouse();
	// bk001206 - replaced with H2/Fakk2 solution
	// XAutoRepeatOn(dpy);
	// autorepeaton = qfalse; // bk001130 - from cvs1.17 (mkv)
	if ( dpy ) {
		if ( ctx ) {
			qglXDestroyContext( dpy, ctx );
		}
		if ( win ) {
			XDestroyWindow( dpy, win );
		}
		if ( vidmode_active ) {
			XF86VidModeSwitchToMode( dpy, scrnum, vidmodes[0] );
		}
		if ( glConfig.deviceSupportsGamma ) {
			XF86VidModeSetGamma( dpy, scrnum, &vidmode_InitialGamma );
		}
		// NOTE TTimo opening/closing the display should be necessary only once per run
		//   but it seems QGL_Shutdown gets called in a lot of occasion
		//   in some cases, this XCloseDisplay is known to raise some X errors
		//   ( show_bug.cgi?id=33 )
		XCloseDisplay( dpy );
	}
	vidmode_active = qfalse;
	dpy = NULL;
	win = 0;
	ctx = NULL;

	memset( &glConfig, 0, sizeof( glConfig ) );
	memset( &glState, 0, sizeof( glState ) );

	QGL_Shutdown();
}

/*
** GLimp_LogComment
*/
void GLimp_LogComment( char *comment ) {
	if ( glw_state.log_fp ) {
		fprintf( glw_state.log_fp, "%s", comment );
	}
}

/*
** GLW_StartDriverAndSetMode
*/
// bk001204 - prototype needed
int GLW_SetMode( const char *drivername, int mode, qboolean fullscreen );
static qboolean GLW_StartDriverAndSetMode( const char *drivername,
										   int mode,
										   qboolean fullscreen ) {
	rserr_t err;

	// don't ever bother going into fullscreen with a voodoo card
#if 1   // JDC: I reenabled this
	if ( Q_stristr( drivername, "Voodoo" ) ) {
		ri.Cvar_Set( "r_fullscreen", "0" );
		r_fullscreen->modified = qfalse;
		fullscreen = qfalse;
	}
#endif

	if ( fullscreen && in_nograb->value ) {
		ri.Printf( PRINT_ALL, "Fullscreen not allowed with in_nograb 1\n" );
		ri.Cvar_Set( "r_fullscreen", "0" );
		r_fullscreen->modified = qfalse;
		fullscreen = qfalse;
	}

	err = (rserr_t)GLW_SetMode( drivername, mode, fullscreen );

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
** GLW_SetMode
*/
int GLW_SetMode( const char *drivername, int mode, qboolean fullscreen ) {
	int attrib[] = {
		GLX_RGBA,     // 0
		GLX_RED_SIZE, 4,  // 1, 2
		GLX_GREEN_SIZE, 4,  // 3, 4
		GLX_BLUE_SIZE, 4, // 5, 6
		GLX_DOUBLEBUFFER, // 7
		GLX_DEPTH_SIZE, 1,  // 8, 9
		GLX_STENCIL_SIZE, 1, // 10, 11
		None
	};
	// these match in the array
#define ATTR_RED_IDX 2
#define ATTR_GREEN_IDX 4
#define ATTR_BLUE_IDX 6
#define ATTR_DEPTH_IDX 9
#define ATTR_STENCIL_IDX 11
	Window root;
	XVisualInfo *visinfo;
	XSetWindowAttributes attr;
	XSizeHints sizehints;
	unsigned long mask;
	int colorbits, depthbits, stencilbits;
	int tcolorbits, tdepthbits, tstencilbits;
	int dga_MajorVersion, dga_MinorVersion;
	int actualWidth, actualHeight;
	int i;
	const char*   glstring; // bk001130 - from cvs1.17 (mkv)

	ri.Printf( PRINT_ALL, "Initializing OpenGL display\n" );

	ri.Printf( PRINT_ALL, "...setting mode %d:", mode );

	if ( !R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode ) ) {
		ri.Printf( PRINT_ALL, " invalid mode\n" );
		return RSERR_INVALID_MODE;
	}
	ri.Printf( PRINT_ALL, " %d %d\n", glConfig.vidWidth, glConfig.vidHeight );

	if ( !( dpy = XOpenDisplay( NULL ) ) ) {
		fprintf( stderr, "Error couldn't open the X display\n" );
		return RSERR_INVALID_MODE;
	}

	scrnum = DefaultScreen( dpy );
	root = RootWindow( dpy, scrnum );

	actualWidth = glConfig.vidWidth;
	actualHeight = glConfig.vidHeight;

	// Get video mode list
	if ( !XF86VidModeQueryVersion( dpy, &vidmode_MajorVersion, &vidmode_MinorVersion ) ) {
		vidmode_ext = qfalse;
	} else
	{
		ri.Printf( PRINT_ALL, "Using XFree86-VidModeExtension Version %d.%d\n",
				   vidmode_MajorVersion, vidmode_MinorVersion );
		vidmode_ext = qtrue;
	}

	// Check for DGA
	dga_MajorVersion = 0, dga_MinorVersion = 0;
	if ( in_dgamouse->value ) {
		if ( !XF86DGAQueryVersion( dpy, &dga_MajorVersion, &dga_MinorVersion ) ) {
			// unable to query, probalby not supported
			ri.Printf( PRINT_ALL, "Failed to detect XF86DGA Mouse\n" );
			ri.Cvar_Set( "in_dgamouse", "0" );
		} else
		{
			ri.Printf( PRINT_ALL, "XF86DGA Mouse (Version %d.%d) initialized\n",
					   dga_MajorVersion, dga_MinorVersion );
		}
	}

	if ( vidmode_ext ) {
		int best_fit, best_dist, dist, x, y;

		XF86VidModeGetAllModeLines( dpy, scrnum, &num_vidmodes, &vidmodes );

		// Are we going fullscreen?  If so, let's change video mode
		if ( fullscreen ) {
			best_dist = 9999999;
			best_fit = -1;

			for ( i = 0; i < num_vidmodes; i++ )
			{
				if ( glConfig.vidWidth > vidmodes[i]->hdisplay ||
					 glConfig.vidHeight > vidmodes[i]->vdisplay ) {
					continue;
				}

				x = glConfig.vidWidth - vidmodes[i]->hdisplay;
				y = glConfig.vidHeight - vidmodes[i]->vdisplay;
				dist = ( x * x ) + ( y * y );
				if ( dist < best_dist ) {
					best_dist = dist;
					best_fit = i;
				}
			}

			if ( best_fit != -1 ) {
				actualWidth = vidmodes[best_fit]->hdisplay;
				actualHeight = vidmodes[best_fit]->vdisplay;

				// change to the mode
				XF86VidModeSwitchToMode( dpy, scrnum, vidmodes[best_fit] );
				vidmode_active = qtrue;

				// Move the viewport to top left
				XF86VidModeSetViewPort( dpy, scrnum, 0, 0 );

				ri.Printf( PRINT_ALL, "XFree86-VidModeExtension Activated at %dx%d\n",
						   actualWidth, actualHeight );

			} else
			{
				fullscreen = 0;
				ri.Printf( PRINT_ALL, "XFree86-VidModeExtension: No acceptable modes found\n" );
			}
		} else
		{
			ri.Printf( PRINT_ALL, "XFree86-VidModeExtension:  Ignored on non-fullscreen/Voodoo\n" );
		}
	}


	if ( !r_colorbits->value ) {
		colorbits = 24;
	} else {
		colorbits = r_colorbits->value;
	}

	if ( !String::ICmp( r_glDriver->string, _3DFX_DRIVER_NAME ) ) {
		colorbits = 16;
	}

	if ( !r_depthbits->value ) {
		depthbits = 24;
	} else {
		depthbits = r_depthbits->value;
	}
	stencilbits = r_stencilbits->value;

	for ( i = 0; i < 16; i++ )
	{
		// 0 - default
		// 1 - minus colorbits
		// 2 - minus depthbits
		// 3 - minus stencil
		if ( ( i % 4 ) == 0 && i ) {
			// one pass, reduce
			switch ( i / 4 )
			{
			case 2:
				if ( colorbits == 24 ) {
					colorbits = 16;
				}
				break;
			case 1:
				if ( depthbits == 24 ) {
					depthbits = 16;
				} else if ( depthbits == 16 ) {
					depthbits = 8;
				}
			case 3:
				if ( stencilbits == 24 ) {
					stencilbits = 16;
				} else if ( stencilbits == 16 ) {
					stencilbits = 8;
				}
			}
		}

		tcolorbits = colorbits;
		tdepthbits = depthbits;
		tstencilbits = stencilbits;

		if ( ( i % 4 ) == 3 ) { // reduce colorbits
			if ( tcolorbits == 24 ) {
				tcolorbits = 16;
			}
		}

		if ( ( i % 4 ) == 2 ) { // reduce depthbits
			if ( tdepthbits == 24 ) {
				tdepthbits = 16;
			} else if ( tdepthbits == 16 ) {
				tdepthbits = 8;
			}
		}

		if ( ( i % 4 ) == 1 ) { // reduce stencilbits
			if ( tstencilbits == 24 ) {
				tstencilbits = 16;
			} else if ( tstencilbits == 16 ) {
				tstencilbits = 8;
			} else {
				tstencilbits = 0;
			}
		}

		if ( tcolorbits == 24 ) {
			attrib[ATTR_RED_IDX] = 8;
			attrib[ATTR_GREEN_IDX] = 8;
			attrib[ATTR_BLUE_IDX] = 8;
		} else
		{
			// must be 16 bit
			attrib[ATTR_RED_IDX] = 4;
			attrib[ATTR_GREEN_IDX] = 4;
			attrib[ATTR_BLUE_IDX] = 4;
		}

		attrib[ATTR_DEPTH_IDX] = tdepthbits; // default to 24 depth
		attrib[ATTR_STENCIL_IDX] = tstencilbits;

		visinfo = qglXChooseVisual( dpy, scrnum, attrib );
		if ( !visinfo ) {
			continue;
		}

		ri.Printf( PRINT_ALL, "Using %d/%d/%d Color bits, %d depth, %d stencil display.\n",
				   attrib[ATTR_RED_IDX], attrib[ATTR_GREEN_IDX], attrib[ATTR_BLUE_IDX],
				   attrib[ATTR_DEPTH_IDX], attrib[ATTR_STENCIL_IDX] );

		glConfig.colorBits = tcolorbits;
		glConfig.depthBits = tdepthbits;
		glConfig.stencilBits = tstencilbits;
		break;
	}

	if ( !visinfo ) {
		ri.Printf( PRINT_ALL, "Couldn't get a visual\n" );
		return RSERR_INVALID_MODE;
	}

	/* window attributes */
	attr.background_pixel = BlackPixel( dpy, scrnum );
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap( dpy, root, visinfo->visual, AllocNone );
	attr.event_mask = X_MASK;
	if ( vidmode_active ) {
		mask = CWBackPixel | CWColormap | CWSaveUnder | CWBackingStore |
			   CWEventMask | CWOverrideRedirect;
		attr.override_redirect = True;
		attr.backing_store = NotUseful;
		attr.save_under = False;
	} else {
		mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
	}

	win = XCreateWindow( dpy, root, 0, 0,
						 actualWidth, actualHeight,
						 0, visinfo->depth, InputOutput,
						 visinfo->visual, mask, &attr );

	XStoreName( dpy, win, WINDOW_CLASS_NAME );

	/* GH: Don't let the window be resized */
	sizehints.flags = PMinSize | PMaxSize;
	sizehints.min_width = sizehints.max_width = actualWidth;
	sizehints.min_height = sizehints.max_height = actualHeight;

	XSetWMNormalHints( dpy, win, &sizehints );

	XMapWindow( dpy, win );

	if ( vidmode_active ) {
		XMoveWindow( dpy, win, 0, 0 );
	}

	XFlush( dpy );
	XSync( dpy,False ); // bk001130 - from cvs1.17 (mkv)
	ctx = qglXCreateContext( dpy, visinfo, NULL, True );
	XSync( dpy,False ); // bk001130 - from cvs1.17 (mkv)

	/* GH: Free the visinfo after we're done with it */
	XFree( visinfo );

	qglXMakeCurrent( dpy, win, ctx );

	// bk001130 - from cvs1.17 (mkv)
	glstring = (char*)qglGetString( GL_RENDERER );
	ri.Printf( PRINT_ALL, "GL_RENDERER: %s\n", glstring );

	// bk010122 - new software token (Indirect)
	if ( !String::ICmp( glstring, "Mesa X11" )
		 || !String::ICmp( glstring, "Mesa GLX Indirect" ) ) {
		if ( !r_allowSoftwareGL->integer ) {
			ri.Printf( PRINT_ALL, "\n\n***********************************************************\n" );
			ri.Printf( PRINT_ALL, " You are using software Mesa (no hardware acceleration)!   \n" );
			ri.Printf( PRINT_ALL, " Driver DLL used: %s\n", drivername );
			ri.Printf( PRINT_ALL, " If this is intentional, add\n" );
			ri.Printf( PRINT_ALL, "       \"+set r_allowSoftwareGL 1\"\n" );
			ri.Printf( PRINT_ALL, " to the command line when starting the game.\n" );
			ri.Printf( PRINT_ALL, "***********************************************************\n" );
			GLimp_Shutdown();
			return RSERR_INVALID_MODE;
		} else
		{
			ri.Printf( PRINT_ALL, "...using software Mesa (r_allowSoftwareGL==1).\n" );
		}
	}

	return RSERR_OK;
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
			qglMultiTexCoord2fARB = ( PFNGLMULTITEXCOORD2FARBPROC ) dlsym( glw_state.OpenGLLib, "glMultiTexCoord2fARB" );
			qglActiveTextureARB = ( PFNGLACTIVETEXTUREARBPROC ) dlsym( glw_state.OpenGLLib, "glActiveTextureARB" );
			qglClientActiveTextureARB = ( PFNGLCLIENTACTIVETEXTUREARBPROC ) dlsym( glw_state.OpenGLLib, "glClientActiveTextureARB" );

			if ( qglActiveTextureARB ) {
				qglGetIntegerv( GL_MAX_ACTIVE_TEXTURES_ARB, &glConfig.maxActiveTextures );

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
			qglLockArraysEXT = ( void ( APIENTRY * )( int, int ) )dlsym( glw_state.OpenGLLib, "glLockArraysEXT" );
			qglUnlockArraysEXT = ( void ( APIENTRY * )( void ) )dlsym( glw_state.OpenGLLib, "glUnlockArraysEXT" );
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

}

static void GLW_InitGamma() {
	/* Minimum extension version required */
  #define GAMMA_MINMAJOR 2
  #define GAMMA_MINMINOR 0

	glConfig.deviceSupportsGamma = qfalse;

	if ( vidmode_ext ) {
		if ( vidmode_MajorVersion < GAMMA_MINMAJOR ||
			 ( vidmode_MajorVersion == GAMMA_MINMAJOR && vidmode_MinorVersion < GAMMA_MINMINOR ) ) {
			ri.Printf( PRINT_ALL, "XF86 Gamma extension not supported in this version\n" );
			return;
		}
		XF86VidModeGetGamma( dpy, scrnum, &vidmode_InitialGamma );
		ri.Printf( PRINT_ALL, "XF86 Gamma extension initialized\n" );
		glConfig.deviceSupportsGamma = qtrue;
	}
}

/*
** GLW_LoadOpenGL
**
** GLimp_win.c internal function that that attempts to load and use
** a specific OpenGL DLL.
*/
static qboolean GLW_LoadOpenGL( const char *name ) {
	qboolean fullscreen;

	ri.Printf( PRINT_ALL, "...loading %s: ", name );

	// disable the 3Dfx splash screen and set gamma
	// we do this all the time, but it shouldn't hurt anything
	// on non-3Dfx stuff
	putenv( "FX_GLIDE_NO_SPLASH=0" );

	// Mesa VooDoo hacks
	putenv( "MESA_GLX_FX=fullscreen\n" );

	// load the QGL layer
	if ( QGL_Init( name ) ) {
		fullscreen = r_fullscreen->integer;

		// create the window and set up the context
		if ( !GLW_StartDriverAndSetMode( name, r_mode->integer, fullscreen ) ) {
			if ( r_mode->integer != 3 ) {
				if ( !GLW_StartDriverAndSetMode( name, 3, fullscreen ) ) {
					goto fail;
				}
			} else {
				goto fail;
			}
		}

		return qtrue;
	} else
	{
		ri.Printf( PRINT_ALL, "failed\n" );
	}
fail:

	QGL_Shutdown();

	return qfalse;
}

/*
** XErrorHandler
**   the default X error handler exits the application
**   I found out that on some hosts some operations would raise X errors (GLXUnsupportedPrivateRequest)
**   but those don't seem to be fatal .. so the default would be to just ignore them
**   our implementation mimics the default handler behaviour (not completely cause I'm lazy)
*/
int qXErrorHandler( Display *dpy, XErrorEvent *ev ) {
	static char buf[1024];
	XGetErrorText( dpy, ev->error_code, buf, 1024 );
	ri.Printf( PRINT_ALL, "X Error of failed request: %s\n", buf );
	ri.Printf( PRINT_ALL, "  Major opcode of failed request: %d\n", ev->request_code, buf );
	ri.Printf( PRINT_ALL, "  Minor opcode of failed request: %d\n", ev->minor_code );
	ri.Printf( PRINT_ALL, "  Serial number of failed request: %d\n", ev->serial );
	return 0;
}

/*
** GLimp_Init
**
** This routine is responsible for initializing the OS specific portions
** of OpenGL.
*/
void GLimp_Init( void ) {
	qboolean attemptedlibGL = qfalse;
	qboolean attempted3Dfx = qfalse;
	qboolean success = qfalse;
	char buf[1024];
	Cvar *lastValidRenderer = ri.Cvar_Get( "r_lastValidRenderer", "(uninitialized)", CVAR_ARCHIVE );

	r_previousglDriver = ri.Cvar_Get( "r_previousglDriver", "", CVAR_ROM );

	InitSig();

	// Hack here so that if the UI
	if ( *r_previousglDriver->string ) {
		// The UI changed it on us, hack it back
		// This means the renderer can't be changed on the fly
		ri.Cvar_Set( "r_glDriver", r_previousglDriver->string );
	}

	// set up our custom error handler for X failures
	XSetErrorHandler( &qXErrorHandler );

	//
	// load and initialize the specific OpenGL driver
	//
	if ( !GLW_LoadOpenGL( r_glDriver->string ) ) {
		if ( !String::ICmp( r_glDriver->string, OPENGL_DRIVER_NAME ) ) {
			attemptedlibGL = qtrue;
		} else if ( !String::ICmp( r_glDriver->string, _3DFX_DRIVER_NAME ) ) {
			attempted3Dfx = qtrue;
		}

	#if 0
		// TTimo
		// show_bug.cgi?id=455
		// old legacy load code, was confusing people who had a bad OpenGL setup
		if ( !attempted3Dfx && !success ) {
			attempted3Dfx = qtrue;
			if ( GLW_LoadOpenGL( _3DFX_DRIVER_NAME ) ) {
				ri.Cvar_Set( "r_glDriver", _3DFX_DRIVER_NAME );
				r_glDriver->modified = qfalse;
				success = qtrue;
			}
		}
	#endif

		// try ICD before trying 3Dfx standalone driver
		if ( !attemptedlibGL && !success ) {
			attemptedlibGL = qtrue;
			if ( GLW_LoadOpenGL( OPENGL_DRIVER_NAME ) ) {
				ri.Cvar_Set( "r_glDriver", OPENGL_DRIVER_NAME );
				r_glDriver->modified = qfalse;
				success = qtrue;
			}
		}

		if ( !success ) {
			ri.Error( ERR_FATAL, "GLimp_Init() - could not load OpenGL subsystem\n" );
		}

	}

	// Save it in case the UI stomps it
	ri.Cvar_Set( "r_previousglDriver", r_glDriver->string );

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
	GLW_InitGamma();

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
	// don't flip if drawing to front buffer
	if ( String::ICmp( r_drawBuffer->string, "GL_FRONT" ) != 0 ) {
		qglXSwapBuffers( dpy, win );
	}

	// check logging
	QGL_EnableLogging( (qboolean)r_logFile->integer ); // bk001205 - was ->value
}

#ifdef SMP
/*
===========================================================

SMP acceleration

===========================================================
*/

sem_t renderCommandsEvent;
sem_t renderCompletedEvent;
sem_t renderActiveEvent;

void ( *glimpRenderThread )( void );

void *GLimp_RenderThreadWrapper( void *stub ) {
	glimpRenderThread();
	return NULL;
}


/*
=======================
GLimp_SpawnRenderThread
=======================
*/
pthread_t renderThreadHandle;
qboolean GLimp_SpawnRenderThread( void ( *function )( void ) ) {

	sem_init( &renderCommandsEvent, 0, 0 );
	sem_init( &renderCompletedEvent, 0, 0 );
	sem_init( &renderActiveEvent, 0, 0 );

	glimpRenderThread = function;

	if ( pthread_create( &renderThreadHandle, NULL,
						 GLimp_RenderThreadWrapper, NULL ) ) {
		return qfalse;
	}

	return qtrue;
}

static void  *smpData;
//static	int		glXErrors; // bk001204 - unused

void *GLimp_RendererSleep( void ) {
	void  *data;

	// after this, the front end can exit GLimp_FrontEndSleep
	sem_post( &renderCompletedEvent );

	sem_wait( &renderCommandsEvent );

	data = smpData;

	// after this, the main thread can exit GLimp_WakeRenderer
	sem_post( &renderActiveEvent );

	return data;
}


void GLimp_FrontEndSleep( void ) {
	sem_wait( &renderCompletedEvent );
}


void GLimp_WakeRenderer( void *data ) {
	smpData = data;

	// after this, the renderer can continue through GLimp_RendererSleep
	sem_post( &renderCommandsEvent );

	sem_wait( &renderActiveEvent );
}

#else

void GLimp_RenderThreadWrapper( void *stub ) {}
qboolean GLimp_SpawnRenderThread( void ( *function )( void ) ) {
	return qfalse;
}
void *GLimp_RendererSleep( void ) {
	return NULL;
}
void GLimp_FrontEndSleep( void ) {}
void GLimp_WakeRenderer( void *data ) {}

#endif

void IN_Activate( void ) {
}

// bk010216 - added stubs for non-Linux UNIXes here
// FIXME - use NO_JOYSTICK or something else generic

#if defined( __FreeBSD__ ) // rb010123
void IN_StartupJoystick( void ) {}
void IN_JoyMove( void ) {}
#endif
