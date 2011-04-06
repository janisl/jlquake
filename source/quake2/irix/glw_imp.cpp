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

static qboolean			doShm;
static Colormap			x_cmap;

int current_framebuffer;
static int				x_shmeventtype;
//static XShmSegmentInfo	x_shminfo;

static qboolean			oktodraw = false;
static qboolean			X11_active = false;

static int p_mouse_x, p_mouse_y;
static QCvar	*_windowed_mouse;

static QCvar *sensitivity;
static QCvar *lookstrafe;
static QCvar *m_side;
static QCvar *m_yaw;
static QCvar *m_pitch;
static QCvar *m_forward;
static QCvar *freelook;

int config_notify=0;
int config_notify_width;
int config_notify_height;
						      
typedef unsigned short PIXEL;

// Console variables that we need to access from this module

/*****************************************************************************/
/* MOUSE                                                                     */
/*****************************************************************************/

// this is inside the renderer shared lib, so these are called from vid_so

static int p_mouse_x, p_mouse_y;

static QCvar	*_windowed_mouse;

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

	in_dgamouse = Cvar_Get("in_dgamouse", "1", 0);

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

void GetEvent(void)
{
	XEvent x_event;
	int b;
   
	XNextEvent(dpy, &x_event);
	SharedHandleEvents(x_event);
	switch(x_event.type) {
	case MotionNotify:
		if (_windowed_mouse->value) {
			mx += ((int)x_event.xmotion.x - (int)(glConfig.vidWidth/2));
			my += ((int)x_event.xmotion.y - (int)(glConfig.vidHeight/2));

			/* move the mouse to the window center again */
			XSelectInput(dpy,win, X_MASK & ~PointerMotionMask);
			XWarpPointer(dpy,None,win,0,0,0,0, 
				(glConfig.vidWidth/2),(glConfig.vidHeight/2));
			XSelectInput(dpy,win, X_MASK);
		} else {
			mx = ((int)x_event.xmotion.x - (int)p_mouse_x);
			my = ((int)x_event.xmotion.y - (int)p_mouse_y);
			p_mouse_x=x_event.xmotion.x;
			p_mouse_y=x_event.xmotion.y;
		}
		break;

	case ConfigureNotify:
		config_notify_width = x_event.xconfigure.width;
		config_notify_height = x_event.xconfigure.height;
		config_notify = 1;
		break;

	default:
		if (doShm && x_event.type == x_shmeventtype)
			oktodraw = true;
	}

	if (vidmode_active || _windowed_mouse->value)
	{
		IN_ActivateMouse();
	}
	else
	{
		IN_DeactivateMouse();
	}
}

/*****************************************************************************/

/*****************************************************************************/
/* KEYBOARD                                                                  */
/*****************************************************************************/

void KBD_Init()
{
	_windowed_mouse = Cvar_Get ("_windowed_mouse", "0", CVAR_ARCHIVE);
}

void KBD_Update(void)
{
// get events from x server
	if (dpy)
	{
		while (XPending(dpy)) 
			GetEvent();
	}
}

void KBD_Close(void)
{
}


void Real_IN_Init()
{
	int mtype;
	int i;

	fprintf(stderr, "GL Real_IN_Init\n");

	// mouse variables
	_windowed_mouse = Cvar_Get ("_windowed_mouse", "0", CVAR_ARCHIVE);
    in_mouse = Cvar_Get ("in_mouse", "1", CVAR_ARCHIVE);
	freelook = Cvar_Get( "freelook", "0", 0 );
	lookstrafe = Cvar_Get ("lookstrafe", "0", 0);
	sensitivity = Cvar_Get ("sensitivity", "3", 0);
	m_pitch = Cvar_Get ("m_pitch", "0.022", 0);
	m_yaw = Cvar_Get ("m_yaw", "0.022", 0);
	m_forward = Cvar_Get ("m_forward", "1", 0);
	m_side = Cvar_Get ("m_side", "0.8", 0);
	in_nograb = Cvar_Get ("in_nograb", "0", 0);
	// turn on-off sub-frame timing of X events
	in_subframe = Cvar_Get ("in_subframe", "1", CVAR_ARCHIVE);

	mouse_avail = true;
}

void IN_Shutdown(void)
{
	mouse_avail = false;

	Cmd_RemoveCommand ("force_centerview");
}

/*
===========
IN_Move
===========
*/
void IN_Move ()
{
	if (!mouse_avail)
		return;
   
	CL_MouseEvent(mx, my);
	mx = my = 0;
}

void IN_Frame (void)
{
}

/*****************************************************************************/
/* INPUT                                                                     */
/*****************************************************************************/

// This if fake, it's acutally done by the Refresh load
void IN_Init (void)
{
	in_joystick	= Cvar_Get ("in_joystick", "0", CVAR_ARCHIVE);
}

void IN_Commands (void)
{
}

void IN_Activate (qboolean active)
{
}

void Do_Key_Event(int key, qboolean down)
{
	Key_Event(key, down, Sys_Milliseconds_());
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
