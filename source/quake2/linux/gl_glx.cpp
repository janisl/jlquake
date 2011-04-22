/*
** GLW_IMP.C
**
** This file contains ALL Linux specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Shutdown
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

#include "../../client/unix_shared.h"

extern	unsigned	sys_frame_time;

// this is inside the renderer shared lib, so these are called from vid_so

/*
** GLimp_SetMode
*/
int GLimp_SetMode(int mode, qboolean fullscreen )
{
	// destroy the existing window
	GLimp_Shutdown ();

	return GLW_SetMode(mode, r_colorbits->integer, fullscreen);
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
	GLimp_SwapBuffers();
}

void Sys_SendKeyEvents (void)
{
	HandleEvents();

	// grab frame time 
	sys_frame_time = Sys_Milliseconds_();
}
