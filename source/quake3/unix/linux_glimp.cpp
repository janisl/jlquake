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

#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include "../../client/unix_shared.h"

// bk001206 - not needed anymore
// static qboolean autorepeaton = qtrue;

/*
* Find the first occurrence of find in s.
*/
// bk001130 - from cvs1.17 (mkv), const
// bk001130 - made first argument const
static const char *Q_stristr( const char *s, const char *find)
{
  register char c, sc;
  register size_t len;

  if ((c = *find++) != 0)
  {
    if (c >= 'a' && c <= 'z')
    {
      c -= ('a' - 'A');
    }
    len = QStr::Length(find);
    do
    {
      do
      {
        if ((sc = *s++) == 0)
          return NULL;
        if (sc >= 'a' && sc <= 'z')
        {
          sc -= ('a' - 'A');
        }
      } while (sc != c);
    } while (QStr::NICmp(s, find, len) != 0);
    s--;
  }
  return s;
}

static void HandleEvents(void)
{
  int b;
  XEvent event;
  qboolean dowarp = qfalse;
  int dx, dy;
  int t = 0; // default to 0 in case we don't set
	
  if (!dpy)
    return;

  while (XPending(dpy))
  {
    XNextEvent(dpy, &event);
	SharedHandleEvents(event);
    switch (event.type)
    {
    case MotionNotify:
			t = Sys_XTimeToSysTime(event.xkey.time);
      if (mouse_active)
      {
        if (in_dgamouse->value)
        {
          if (abs(event.xmotion.x_root) > 1)
            mx += event.xmotion.x_root * 2;
          else
            mx += event.xmotion.x_root;
          if (abs(event.xmotion.y_root) > 1)
            my += event.xmotion.y_root * 2;
          else
            my += event.xmotion.y_root;
          if (t - mouseResetTime > MOUSE_RESET_DELAY )
          {
            Sys_QueEvent( t, SE_MOUSE, mx, my, 0, NULL );
          }
          mx = my = 0;
        } else
        {
          // If it's a center motion, we've just returned from our warp
          if (event.xmotion.x == glConfig.vidWidth/2 &&
              event.xmotion.y == glConfig.vidHeight/2)
          {
            mwx = glConfig.vidWidth/2;
            mwy = glConfig.vidHeight/2;
            if (t - mouseResetTime > MOUSE_RESET_DELAY )
            {
              Sys_QueEvent( t, SE_MOUSE, mx, my, 0, NULL );
            }
            mx = my = 0;
            break;
          }

          dx = ((int)event.xmotion.x - mwx);
          dy = ((int)event.xmotion.y - mwy);
          if (abs(dx) > 1)
            mx += dx * 2;
          else
            mx += dx;
          if (abs(dy) > 1)
            my += dy * 2;
          else
            my += dy;

          mwx = event.xmotion.x;
          mwy = event.xmotion.y;
          dowarp = qtrue;
        }
      }
      break;

    case CreateNotify :
      win_x = event.xcreatewindow.x;
      win_y = event.xcreatewindow.y;
      break;

    case ConfigureNotify :
      win_x = event.xconfigure.x;
      win_y = event.xconfigure.y;
      break;
    }
  }

  if (dowarp)
  {
    XWarpPointer(dpy,None,win,0,0,0,0, 
                 (glConfig.vidWidth/2),(glConfig.vidHeight/2));
  }
}

// NOTE TTimo for the tty console input, we didn't rely on those .. 
//   it's not very surprising actually cause they are not used otherwise
void KBD_Init(void)
{
}

void KBD_Close(void)
{
}
/*****************************************************************************/

/*
** GLimp_SetGamma
**
** This routine should only be called if glConfig.deviceSupportsGamma is TRUE
*/
void GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] )
{
  // NOTE TTimo we get the gamma value from cvar, because we can't work with the s_gammatable
  //   the API wasn't changed to avoid breaking other OSes
  float g = Cvar_Get("r_gamma", "1.0", 0)->value;
  XF86VidModeGamma gamma;
  assert(glConfig.deviceSupportsGamma);
  gamma.red = g;
  gamma.green = g;
  gamma.blue = g;
  XF86VidModeSetGamma(dpy, scrnum, &gamma);
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
	GLimp_SharedShutdown();

  QGL_Shutdown();
}

/*
** GLW_StartDriverAndSetMode
*/
// bk001204 - prototype needed
int GLW_SetMode(int mode, qboolean fullscreen );
static qboolean GLW_StartDriverAndSetMode(int mode, qboolean fullscreen )
{
  rserr_t err;

	if (fullscreen && in_nograb->value)
	{
		ri.Printf( PRINT_ALL, "Fullscreen not allowed with in_nograb 1\n");
    Cvar_Set( "r_fullscreen", "0" );
    r_fullscreen->modified = qfalse;
    fullscreen = qfalse;		
	}

  err = (rserr_t)GLW_SetMode(mode, fullscreen);

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
int GLW_SetMode(int mode, qboolean fullscreen )
{
  const char*   glstring; // bk001130 - from cvs1.17 (mkv)

  ri.Printf( PRINT_ALL, "Initializing OpenGL display\n");

  ri.Printf (PRINT_ALL, "...setting mode %d:", mode );

  if ( !R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode ) )
  {
    ri.Printf( PRINT_ALL, " invalid mode\n" );
    return RSERR_INVALID_MODE;
  }
  ri.Printf( PRINT_ALL, " %d %d\n", glConfig.vidWidth, glConfig.vidHeight);

	if (GLimp_GLXSharedInit(glConfig.vidWidth, glConfig.vidHeight, fullscreen) != RSERR_OK)
	{
		return RSERR_INVALID_MODE;
	}

  // bk001130 - from cvs1.17 (mkv)
  glstring = (char*)qglGetString (GL_RENDERER);
  ri.Printf( PRINT_ALL, "GL_RENDERER: %s\n", glstring );

  // bk010122 - new software token (Indirect)
  if ( !QStr::ICmp( glstring, "Mesa X11")
       || !QStr::ICmp( glstring, "Mesa GLX Indirect") )
  {
    if ( !r_allowSoftwareGL->integer )
    {
      ri.Printf( PRINT_ALL, "\n\n***********************************************************\n" );
      ri.Printf( PRINT_ALL, " You are using software Mesa (no hardware acceleration)!   \n" );
      ri.Printf( PRINT_ALL, " If this is intentional, add\n" );
      ri.Printf( PRINT_ALL, "       \"+set r_allowSoftwareGL 1\"\n" );
      ri.Printf( PRINT_ALL, " to the command line when starting the game.\n" );
      ri.Printf( PRINT_ALL, "***********************************************************\n");
      GLimp_Shutdown( );
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
static void GLW_InitExtensions( void )
{
  if ( !r_allowExtensions->integer )
  {
    ri.Printf( PRINT_ALL, "*** IGNORING OPENGL EXTENSIONS ***\n" );
    return;
  }

  ri.Printf( PRINT_ALL, "Initializing OpenGL extensions\n" );

  // GL_S3_s3tc
  if ( Q_stristr( glConfig.extensions_string, "GL_S3_s3tc" ) )
  {
    if ( r_ext_compressed_textures->value )
    {
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
  if ( Q_stristr( glConfig.extensions_string, "EXT_texture_env_add" ) )
  {
    if ( r_ext_texture_env_add->integer )
    {
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
  if ( Q_stristr( glConfig.extensions_string, "GL_ARB_multitexture" ) )
  {
    if ( r_ext_multitexture->value )
    {
      qglMultiTexCoord2fARB = (PFNGLMULTITEXCOORD2FARBPROC)GLimp_GetProcAddress("glMultiTexCoord2fARB");
      qglActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)GLimp_GetProcAddress("glActiveTextureARB");
      qglClientActiveTextureARB = (PFNGLCLIENTACTIVETEXTUREARBPROC)GLimp_GetProcAddress("glClientActiveTextureARB");

      if ( qglActiveTextureARB )
      {
        qglGetIntegerv( GL_MAX_ACTIVE_TEXTURES_ARB, &glConfig.maxActiveTextures );

        if ( glConfig.maxActiveTextures > 1 )
        {
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
  if ( Q_stristr( glConfig.extensions_string, "GL_EXT_compiled_vertex_array" ) )
  {
    if ( r_ext_compiled_vertex_array->value )
    {
      ri.Printf( PRINT_ALL, "...using GL_EXT_compiled_vertex_array\n" );
      qglLockArraysEXT = (void (APIENTRY*)(int, int))GLimp_GetProcAddress("glLockArraysEXT");
      qglUnlockArraysEXT = (void (APIENTRY*)())GLimp_GetProcAddress("glUnlockArraysEXT");
      if (!qglLockArraysEXT || !qglUnlockArraysEXT)
      {
        ri.Error (ERR_FATAL, "bad getprocaddress");
      }
    } else
    {
      ri.Printf( PRINT_ALL, "...ignoring GL_EXT_compiled_vertex_array\n" );
    }
  } else
  {
    ri.Printf( PRINT_ALL, "...GL_EXT_compiled_vertex_array not found\n" );
  }

}

static void GLW_InitGamma()
{
  /* Minimum extension version required */
  #define GAMMA_MINMAJOR 2
  #define GAMMA_MINMINOR 0
  
  glConfig.deviceSupportsGamma = qfalse;

  if (vidmode_ext)
  {
    if (vidmode_MajorVersion < GAMMA_MINMAJOR || 
        (vidmode_MajorVersion == GAMMA_MINMAJOR && vidmode_MinorVersion < GAMMA_MINMINOR)) {
      ri.Printf( PRINT_ALL, "XF86 Gamma extension not supported in this version\n");
      return;
    }
    XF86VidModeGetGamma(dpy, scrnum, &vidmode_InitialGamma);
    ri.Printf( PRINT_ALL, "XF86 Gamma extension initialized\n");
    glConfig.deviceSupportsGamma = qtrue;
  }
}

/*
** GLW_LoadOpenGL
**
** GLimp_win.c internal function that that attempts to load and use 
** a specific OpenGL DLL.
*/
static qboolean GLW_LoadOpenGL()
{
  qboolean fullscreen;

  // load the QGL layer
  if ( QGL_Init() )
  {
    fullscreen = r_fullscreen->integer;

    // create the window and set up the context
    if (!GLW_StartDriverAndSetMode(r_mode->integer, fullscreen))
    {
      if (r_mode->integer != 3)
      {
        if (!GLW_StartDriverAndSetMode(3, fullscreen))
        {
          goto fail;
        }
      } else
        goto fail;
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
int qXErrorHandler(Display *dpy, XErrorEvent *ev)
{
  static char buf[1024];
  XGetErrorText(dpy, ev->error_code, buf, 1024);
  ri.Printf( PRINT_ALL, "X Error of failed request: %s\n", buf);
  ri.Printf( PRINT_ALL, "  Major opcode of failed request: %d\n", ev->request_code, buf);
  ri.Printf( PRINT_ALL, "  Minor opcode of failed request: %d\n", ev->minor_code);  
  ri.Printf( PRINT_ALL, "  Serial number of failed request: %d\n", ev->serial);
  return 0;
}

/*
** GLimp_Init
**
** This routine is responsible for initializing the OS specific portions
** of OpenGL.  
*/
void GLimp_Init( void )
{
  char  buf[1024];
  QCvar *lastValidRenderer = Cvar_Get( "r_lastValidRenderer", "(uninitialized)", CVAR_ARCHIVE );

  if (!XInitThreads())
  {
    Com_Printf("GLimp_Init() - XInitThreads() failed, disabling r_smp\n");
    Cvar_Set( "r_smp", "0" );
  }

  InitSig();

  // set up our custom error handler for X failures
  XSetErrorHandler(&qXErrorHandler);

  //
  // load and initialize the specific OpenGL driver
  //
  if (!GLW_LoadOpenGL())
  {
      ri.Error( ERR_FATAL, "GLimp_Init() - could not load OpenGL subsystem\n" );
  }

  // This values force the UI to disable driver selection
  glConfig.driverType = GLDRV_ICD;
  glConfig.hardwareType = GLHW_GENERIC;

  // get our config strings
  QStr::NCpyZ( glConfig.vendor_string, (char*)qglGetString (GL_VENDOR), sizeof( glConfig.vendor_string ) );
  QStr::NCpyZ( glConfig.renderer_string, (char*)qglGetString (GL_RENDERER), sizeof( glConfig.renderer_string ) );
  if (*glConfig.renderer_string && glConfig.renderer_string[QStr::Length(glConfig.renderer_string) - 1] == '\n')
    glConfig.renderer_string[QStr::Length(glConfig.renderer_string) - 1] = 0;
  QStr::NCpyZ( glConfig.version_string, (char*)qglGetString (GL_VERSION), sizeof( glConfig.version_string ) );
  QStr::NCpyZ( glConfig.extensions_string, (char*)qglGetString (GL_EXTENSIONS), sizeof( glConfig.extensions_string ) );

  //
  // chipset specific configuration
  //
  QStr::Cpy( buf, glConfig.renderer_string );
  QStr::ToLower( buf );

  //
  // NOTE: if changing cvars, do it within this block.  This allows them
  // to be overridden when testing driver fixes, etc. but only sets
  // them to their default state when the hardware is first installed/run.
  //
  if ( QStr::ICmp( lastValidRenderer->string, glConfig.renderer_string ) )
  {
    Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST" );

      Cvar_Set( "r_picmip", "1" );
  }

  Cvar_Set( "r_lastValidRenderer", glConfig.renderer_string );

  // initialize extensions
  GLW_InitExtensions();
  GLW_InitGamma();

  InitSig(); // not clear why this is at begin & end of function

  return;
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
  // don't flip if drawing to front buffer
  if ( QStr::ICmp( r_drawBuffer->string, "GL_FRONT" ) != 0 )
  {
    glXSwapBuffers(dpy, win);
  }

  // check logging
  QGL_EnableLogging(!!r_logFile->integer);
}

/*
===========================================================

SMP acceleration

===========================================================
*/

static pthread_mutex_t	smpMutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t		renderCommandsEvent = PTHREAD_COND_INITIALIZER;
static pthread_cond_t		renderCompletedEvent = PTHREAD_COND_INITIALIZER;

static void (*glimpRenderThread)( void );

static void *GLimp_RenderThreadWrapper( void *arg )
{
	Com_Printf( "Render thread starting\n" );

  glimpRenderThread();

	glXMakeCurrent( dpy, None, NULL );

	Com_Printf( "Render thread terminating\n" );

	return arg;
}

qboolean GLimp_SpawnRenderThread( void (*function)( void ) )
{
	pthread_t renderThread;
	int ret;

	pthread_mutex_init( &smpMutex, NULL );

	pthread_cond_init( &renderCommandsEvent, NULL );
	pthread_cond_init( &renderCompletedEvent, NULL );

  glimpRenderThread = function;

	ret = pthread_create( &renderThread,
						  NULL,			// attributes
						  GLimp_RenderThreadWrapper,
						  NULL );		// argument
	if ( ret ) {
		ri.Printf( PRINT_ALL, "pthread_create returned %d: %s", ret, strerror( ret ) );
    return qfalse;
	} else {
		ret = pthread_detach( renderThread );
		if ( ret ) {
			ri.Printf( PRINT_ALL, "pthread_detach returned %d: %s", ret, strerror( ret ) );
		}
  }

  return qtrue;
}

static volatile void    *smpData = NULL;
static volatile qboolean smpDataReady;

void *GLimp_RendererSleep( void )
{
	void  *data;

	glXMakeCurrent( dpy, None, NULL );

	pthread_mutex_lock( &smpMutex );
	{
		smpData = NULL;
		smpDataReady = qfalse;

		// after this, the front end can exit GLimp_FrontEndSleep
		pthread_cond_signal( &renderCompletedEvent );

		while ( !smpDataReady ) {
			pthread_cond_wait( &renderCommandsEvent, &smpMutex );
		}

		data = (void *)smpData;
	}
	pthread_mutex_unlock( &smpMutex );

	glXMakeCurrent( dpy, win, ctx );

  return data;
}

void GLimp_FrontEndSleep( void )
{
	pthread_mutex_lock( &smpMutex );
	{
		while ( smpData ) {
			pthread_cond_wait( &renderCompletedEvent, &smpMutex );
		}
	}
	pthread_mutex_unlock( &smpMutex );

	glXMakeCurrent( dpy, win, ctx );
}

void GLimp_WakeRenderer( void *data )
{
	glXMakeCurrent( dpy, None, NULL );

	pthread_mutex_lock( &smpMutex );
	{
		assert( smpData == NULL );
		smpData = data;
		smpDataReady = qtrue;

		// after this, the renderer can continue through GLimp_RendererSleep
		pthread_cond_signal( &renderCommandsEvent );
	}
	pthread_mutex_unlock( &smpMutex );
}

/*****************************************************************************/
/* MOUSE                                                                     */
/*****************************************************************************/

void IN_Init(void) {
	Com_Printf ("\n------- Input Initialization -------\n");
  // mouse variables
  in_mouse = Cvar_Get ("in_mouse", "1", CVAR_ARCHIVE);
  in_dgamouse = Cvar_Get ("in_dgamouse", "1", CVAR_ARCHIVE);
	
	// turn on-off sub-frame timing of X events
	in_subframe = Cvar_Get ("in_subframe", "1", CVAR_ARCHIVE);
	
	// developer feature, allows to break without loosing mouse pointer
	in_nograb = Cvar_Get ("in_nograb", "0", 0);

  // bk001130 - from cvs.17 (mkv), joystick variables
  in_joystick = Cvar_Get ("in_joystick", "0", CVAR_ARCHIVE|CVAR_LATCH);
  // bk001130 - changed this to match win32
  in_joystickDebug = Cvar_Get ("in_debugjoystick", "0", CVAR_TEMP);
  joy_threshold = Cvar_Get ("joy_threshold", "0.15", CVAR_ARCHIVE); // FIXME: in_joythreshold

  if (in_mouse->value)
    mouse_avail = qtrue;
  else
    mouse_avail = qfalse;

  IN_StartupJoystick( ); // bk001130 - from cvs1.17 (mkv)
	Com_Printf ("------------------------------------\n");
}

void IN_Frame (void) {

  // bk001130 - from cvs 1.17 (mkv)
  IN_JoyMove(); // FIXME: disable if on desktop?

  if ( in_keyCatchers & KEYCATCH_CONSOLE )
  {
    // temporarily deactivate if not in the game and
    // running on the desktop
    if (Cvar_VariableValue ("r_fullscreen") == 0)
    {
      IN_DeactivateMouse ();
      return;
    }
  }

  IN_ActivateMouse();
}

void IN_Activate(void)
{
}

// bk001130 - cvs1.17 joystick code (mkv) was here, no linux_joystick.c

void Sys_SendKeyEvents (void) {
  // XEvent event; // bk001204 - unused

  if (!dpy)
    return;
  HandleEvents();
}
