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
**
*/

#include <signal.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/extensions/XShm.h>

#include "../ref_gl/gl_local.h"
#include "../client/keys.h"

#include "../../client/unix_shared.h"

// this is inside the renderer shared lib, so these are called from vid_so

static void signal_handler(int sig)
{
	fprintf(stderr, "Received signal %d, exiting...\n", sig);
	GLimp_Shutdown();
	_exit(0);
}

static void InitSig(void)
{
        struct sigaction sa;
	sigaction(SIGINT, 0, &sa);
	sa.sa_handler = signal_handler;
	sigaction(SIGINT, &sa, 0);
	sigaction(SIGTERM, &sa, 0);
}

/*
** GLimp_SetMode
**
** This initializes the GL implementation specific
** graphics subsystem.
**
*/
int GLimp_SetMode(int mode, qboolean fullscreen)
{
	// destroy the existing window
	GLimp_Shutdown ();

	srandom(getpid());

	if (GLW_SetMode(mode, r_colorbits->integer, fullscreen) != RSERR_OK)
	{
		// failed to set a valid mode in windowed mode
		return RSERR_INVALID_MODE;
	}

	// let the sound and input subsystems know about the new window
	ri.Vid_NewWindow (glConfig.vidWidth, glConfig.vidHeight);

	return RSERR_OK;
}

/*
** GLimp_Init
**
** This routine is responsible for initializing the OS specific portions
** of OpenGL.  
*/
int GLimp_Init( void *hinstance, void *wndproc )
{
// catch signals so i can turn on auto-repeat and stuff
	InitSig();

	return true;
}

/*
** GLimp_BeginFrame
*/
void GLimp_BeginFrame( float camera_seperation )
{
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
	qglFlush();
	glXSwapBuffers( dpy, win );
}

/*
** GLimp_AppActivate
*/
void GLimp_AppActivate( qboolean active )
{
}
