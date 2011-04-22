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

void Sys_SendKeyEvents (void)
{
	HandleEvents();

	// grab frame time 
	sys_frame_time = Sys_Milliseconds_();
}
