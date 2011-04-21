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
#include <unistd.h>
#include <sys/mman.h>
#include <execinfo.h>

#include "../ref_gl/gl_local.h"
#include "../client/client.h"
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

int XShmQueryExtension(Display *);
int XShmGetEventBase(Display *);


static void signal_handler(int sig, siginfo_t *info, void *secret)
{
	void *trace[64];
	char **messages = (char **)NULL;
	int i, trace_size = 0;
	ucontext_t *uc = (ucontext_t *)secret;

	XAutoRepeatOn(dpy);

#if id386
	/* Do something useful with siginfo_t */
	if (sig == SIGSEGV)
		printf("Received signal %d, faulty address is %p, "
			"from %p\n", sig, info->si_addr, 
			uc->uc_mcontext.gregs[REG_EIP]);
	else
#endif
		printf("Received signal %d, exiting...\n", sig);
		
	trace_size = backtrace(trace, 64);
#if id386
	/* overwrite sigaction with caller's address */
	trace[1] = (void *) uc->uc_mcontext.gregs[REG_EIP];
#endif

	messages = backtrace_symbols(trace, trace_size);
	/* skip first stack frame (points here) */
	printf("[bt] Execution path:\n");
	for (i=1; i<trace_size; ++i)
		printf("[bt] %s\n", messages[i]);

	Sys_Quit();
	exit(0);
}

static void InitSig()
{
	struct sigaction sa;

	sa.sa_sigaction = signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_SIGINFO;

	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGILL, &sa, NULL);
	sigaction(SIGTRAP, &sa, NULL);
	sigaction(SIGIOT, &sa, NULL);
	sigaction(SIGBUS, &sa, NULL);
	sigaction(SIGFPE, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
}

/*
** GLimp_SetMode
**
** This initializes the GL implementation specific
** graphics subsystem.
*/
int GLimp_SetMode(int mode, qboolean fullscreen )
{
	// destroy the existing window
	GLimp_Shutdown ();

	srandom(getpid());

	if (GLW_SetMode(mode, fullscreen) != RSERR_OK)
	{
		// failed to set a valid mode in windowed mode
		return RSERR_INVALID_MODE;
	}

	current_framebuffer = 0;

	X11_active = true;

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
