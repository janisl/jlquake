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

static int   mouse_x, mouse_y;
static int	old_mouse_x, old_mouse_y;
static int p_mouse_x, p_mouse_y;

static QCvar	*_windowed_mouse;
static QCvar	*m_filter;

static qboolean	mlooking;

// state struct passed in Init
static in_state_t	*in_state;

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


static void Force_CenterView_f (void)
{
	in_state->viewangles[PITCH] = 0;
}

static void RW_IN_MLookDown (void) 
{ 
	mlooking = true; 
}

static void RW_IN_MLookUp (void) 
{
	mlooking = false;
	in_state->IN_CenterView_fp ();
}

void RW_IN_Init(in_state_t *in_state_p)
{
	int mtype;
	int i;

	fprintf(stderr, "GL RW_IN_Init\n");

	in_state = in_state_p;

	// mouse variables
	_windowed_mouse = Cvar_Get ("_windowed_mouse", "0", CVAR_ARCHIVE);
	m_filter = Cvar_Get ("m_filter", "0", 0);
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

	Cmd_AddCommand ("+mlook", RW_IN_MLookDown);
	Cmd_AddCommand ("-mlook", RW_IN_MLookUp);

	Cmd_AddCommand ("force_centerview", Force_CenterView_f);

	mouse_x = mouse_y = 0.0;
	mouse_avail = true;
}

void RW_IN_Shutdown(void)
{
	mouse_avail = false;

	Cmd_RemoveCommand ("force_centerview");
	Cmd_RemoveCommand ("+mlook");
	Cmd_RemoveCommand ("-mlook");
}

/*
===========
IN_Move
===========
*/
void RW_IN_Move (usercmd_t *cmd)
{
	if (!mouse_avail)
		return;
   
	if (m_filter->value)
	{
		mouse_x = (mx + old_mouse_x) * 0.5;
		mouse_y = (my + old_mouse_y) * 0.5;
	} else {
		mouse_x = mx;
		mouse_y = my;
	}

	old_mouse_x = mx;
	old_mouse_y = my;

	if (!mouse_x && !mouse_y)
		return;

	mouse_x *= sensitivity->value;
	mouse_y *= sensitivity->value;

// add mouse X/Y movement to cmd
	if ( (*in_state->in_strafe_state & 1) || 
		(lookstrafe->value && mlooking ))
		cmd->sidemove += m_side->value * mouse_x;
	else
		in_state->viewangles[YAW] -= m_yaw->value * mouse_x;

	if ( (mlooking || freelook->value) && 
		!(*in_state->in_strafe_state & 1))
	{
		in_state->viewangles[PITCH] += m_pitch->value * mouse_y;
	}
	else
	{
		cmd->forwardmove -= m_forward->value * mouse_y;
	}
	mx = my = 0;
}

void RW_IN_Frame (void)
{
}

void RW_IN_Activate(void)
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
