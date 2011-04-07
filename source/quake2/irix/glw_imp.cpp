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
#include "../linux/rw_linux.h"

#include "../../client/unix_shared.h"

static Colormap			x_cmap;

int current_framebuffer;
static int				x_shmeventtype;
//static XShmSegmentInfo	x_shminfo;

static qboolean			X11_active = false;

static int p_mouse_x, p_mouse_y;

typedef unsigned short PIXEL;

// Console variables that we need to access from this module

/*****************************************************************************/
/* MOUSE                                                                     */
/*****************************************************************************/

// this is inside the renderer shared lib, so these are called from vid_so

static int p_mouse_x, p_mouse_y;

int XShmQueryExtension(Display *);
int XShmGetEventBase(Display *);

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
*/
int GLimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen )
{
	int width, height;

	fprintf(stderr, "GLimp_SetMode\n");

	ri.Con_Printf( PRINT_ALL, "Initializing OpenGL display\n");

	ri.Con_Printf (PRINT_ALL, "...setting mode %d:", mode );

	if ( !ri.Vid_GetModeInfo( &width, &height, mode ) )
	{
		ri.Con_Printf( PRINT_ALL, " invalid mode\n" );
		return RSERR_INVALID_MODE;
	}

	ri.Con_Printf( PRINT_ALL, " %d %d\n", width, height );

	// destroy the existing window
	GLimp_Shutdown ();

	*pwidth = width;
	*pheight = height;

	if ( !GLimp_InitGraphics( fullscreen ) ) {
		// failed to set a valid mode in windowed mode
		return RSERR_INVALID_MODE;
	}
/* 	ctx = glXCreateContext( dpy, visinfo, 0, True ); */

	// let the sound and input subsystems know about the new window
	ri.Vid_NewWindow (width, height);

	return RSERR_OK;
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

// ========================================================================
// makes a null cursor
// ========================================================================

/*
** GLimp_InitGraphics
**
** This initializes the GL implementation specific
** graphics subsystem.
**
** The necessary width and height parameters are grabbed from
** glConfig.vidWidth and glConfig.vidHeight.
*/
qboolean GLimp_InitGraphics( qboolean fullscreen )
{
	fprintf(stderr, "GLimp_InitGraphics\n");

	srandom(getpid());

	// let the sound and input subsystems know about the new window
	ri.Vid_NewWindow (glConfig.vidWidth, glConfig.vidHeight);

	if (GLimp_GLXSharedInit(glConfig.vidWidth, glConfig.vidHeight, fullscreen) != RSERR_OK)
	{
		return false;
	}

	current_framebuffer = 0;

	X11_active = true;

	return true;
}

/*****************************************************************************/
/* KEYBOARD                                                                  */
/*****************************************************************************/

void KBD_Init()
{
}

void KBD_Update(void)
{
	HandleEvents();
}

void KBD_Close(void)
{
}


void Real_IN_Init()
{
}

//===============================================================================

/*
================
Sys_MakeCodeWriteable
================
*/
void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{

	int r;
	unsigned long addr;
	int psize = getpagesize();

	addr = (startaddr & ~(psize-1)) - psize;

//	fprintf(stderr, "writable code %lx(%lx)-%lx, length=%lx\n", startaddr,
//			addr, startaddr+length, length);

	r = mprotect((char*)addr, length + startaddr - addr + psize, 7);

	if (r < 0)
    		Sys_Error("Protection change failed\n");

}
