//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "../client.h"
#include "local.h"
#include "../unix_shared.h"
#include <X11/extensions/xf86vmode.h>
#include <X11/extensions/Xxf86dga.h>
#include <GL/glx.h>
#include <pthread.h>
#include <semaphore.h>

// MACROS ------------------------------------------------------------------

#define WOLF_SMP

//	Minimum extension version required
#define GAMMA_MINMAJOR 2
#define GAMMA_MINMINOR 0

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

Display* dpy = NULL;
Window win;

Atom wm_protocols;
Atom wm_delete_window;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static int scrnum;
static GLXContext ctx = NULL;

static XF86VidModeModeInfo** vidmodes;
static int num_vidmodes;
static bool vidmode_active = false;
static bool vidmode_ext = false;
static int vidmode_MajorVersion = 0;						// major and minor of XF86VidExtensions
static int vidmode_MinorVersion = 0;

// gamma value of the X display before we start playing with it
static XF86VidModeGamma vidmode_InitialGamma;

static bool fontbase_init = false;

#ifdef WOLF_SMP
static sem_t renderCommandsEvent;
static sem_t renderCompletedEvent;
static sem_t renderActiveEvent;
#else
static pthread_mutex_t smpMutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t renderCommandsEvent = PTHREAD_COND_INITIALIZER;
static pthread_cond_t renderCompletedEvent = PTHREAD_COND_INITIALIZER;
#endif

static void (* glimpRenderThread)();

static volatile void* smpData = NULL;
#ifndef WOLF_SMP
static volatile bool smpDataReady;
#endif

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	qXErrorHandler
//
//	the default X error handler exits the application
//	I found out that on some hosts some operations would raise X errors (GLXUnsupportedPrivateRequest)
//	but those don't seem to be fatal .. so the default would be to just ignore them
//	our implementation mimics the default handler behaviour (not completely cause I'm lazy)
//
//==========================================================================

static int qXErrorHandler(Display* dpy, XErrorEvent* ev)
{
	static char buf[1024];
	XGetErrorText(dpy, ev->error_code, buf, 1024);
	Log::write("X Error of failed request: %s\n", buf);
	Log::write("  Major opcode of failed request: %d\n", ev->request_code, buf);
	Log::write("  Minor opcode of failed request: %d\n", ev->minor_code);
	Log::write("  Serial number of failed request: %d\n", ev->serial);
	return 0;
}

static void GLW_GenDefaultLists()
{
	// keep going, we'll probably just leak some stuff
	if (fontbase_init)
	{
		common->DPrintf("ERROR: GLW_GenDefaultLists: font base is already marked initialized\n");
	}

	XFontStruct* fontInfo = XLoadQueryFont(dpy, "-*-helvetica-medium-r-normal-*-12-*-*-*-p-*-*-*");
	if (fontInfo == NULL)
	{
		// try to load other fonts
		fontInfo = XLoadQueryFont(dpy, "-*-helvetica-*-*-*-*-*-*-*-*-*-*-*-*");

		// any font will do !
		if (fontInfo == NULL)
		{
			fontInfo = XLoadQueryFont(dpy, "-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
		}

		if (fontInfo == NULL)
		{
			common->Printf("ERROR: couldn't create font (XLoadQueryFont)\n");
			return;
		}
	}

	unsigned int first = fontInfo->min_char_or_byte2;
	unsigned int last = fontInfo->max_char_or_byte2;
	unsigned int firstrow = fontInfo->min_byte1;
	unsigned int lastrow = fontInfo->max_byte1;
	//	How many chars in the charset
	int maxchars = 256 * lastrow + last;
	gl_NormalFontBase = glGenLists(maxchars + 1);
	if (gl_NormalFontBase == 0)
	{
		common->Printf("ERROR: couldn't create font (glGenLists)\n");
		return;
	}

	//	Get offset to first char in the charset
	int firstbitmap = 256 * firstrow + first;

	//	for each row of chars, call glXUseXFont to build the bitmaps.
	for (int i = firstrow; i <= (int)lastrow; i++)
	{
		// http://www.atomised.org/docs/XFree86-4.2.1/lib_2GL_2glx_2glxcmds_8c-source.html#l00373
		glXUseXFont(fontInfo->fid, firstbitmap, last - first + 1, gl_NormalFontBase + firstbitmap);
		firstbitmap += 256;
	}

	fontbase_init = true;
}

//==========================================================================
//
//	GLimp_SetMode
//
//==========================================================================

rserr_t GLimp_SetMode(int mode, int colorbits, bool fullscreen)
{
	if (!XInitThreads())
	{
		Log::write("...XInitThreads() failed, disabling r_smp\n");
		Cvar_Set("r_smp", "0");
	}

	// set up our custom error handler for X failures
	XSetErrorHandler(&qXErrorHandler);

	if (fullscreen && in_nograb->value)
	{
		Log::write("Fullscreen not allowed with in_nograb 1\n");
		Cvar_Set("r_fullscreen", "0");
		r_fullscreen->modified = false;
		fullscreen = false;
	}

	Log::write("...setting mode %d:", mode);

	if (!R_GetModeInfo(&glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode))
	{
		Log::write(" invalid mode\n");
		return RSERR_INVALID_MODE;
	}
	Log::write(" %d %d\n", glConfig.vidWidth, glConfig.vidHeight);

	// open the display
	if (!(dpy = XOpenDisplay(NULL)))
	{
		fprintf(stderr, "Error couldn't open the X display\n");
		return RSERR_INVALID_MODE;
	}

	scrnum = DefaultScreen(dpy);
	Window root = RootWindow(dpy, scrnum);

	// Get video mode list
	if (!XF86VidModeQueryVersion(dpy, &vidmode_MajorVersion, &vidmode_MinorVersion))
	{
		vidmode_ext = false;
	}
	else
	{
		Log::write("Using XFree86-VidModeExtension Version %d.%d\n", vidmode_MajorVersion, vidmode_MinorVersion);
		vidmode_ext = true;
	}

	// Check for DGA
	int dga_MajorVersion = 0;
	int dga_MinorVersion = 0;
	if (in_dgamouse->value)
	{
		if (!XF86DGAQueryVersion(dpy, &dga_MajorVersion, &dga_MinorVersion))
		{
			// unable to query, probalby not supported
			Log::write("Failed to detect XF86DGA Mouse\n");
			Cvar_Set("in_dgamouse", "0");
		}
		else
		{
			Log::write("XF86DGA Mouse (Version %d.%d) initialized\n", dga_MajorVersion, dga_MinorVersion);
		}
	}

	int actualWidth = glConfig.vidWidth;
	int actualHeight = glConfig.vidHeight;

	if (vidmode_ext)
	{
		int best_fit, best_dist, dist, x, y;

		XF86VidModeGetAllModeLines(dpy, scrnum, &num_vidmodes, &vidmodes);

		// Are we going fullscreen?  If so, let's change video mode
		if (fullscreen)
		{
			best_dist = 9999999;
			best_fit = -1;

			for (int i = 0; i < num_vidmodes; i++)
			{
				if (glConfig.vidWidth > vidmodes[i]->hdisplay ||
					glConfig.vidHeight > vidmodes[i]->vdisplay)
				{
					continue;
				}

				x = glConfig.vidWidth - vidmodes[i]->hdisplay;
				y = glConfig.vidHeight - vidmodes[i]->vdisplay;
				dist = (x * x) + (y * y);
				if (dist < best_dist)
				{
					best_dist = dist;
					best_fit = i;
				}
			}

			if (best_fit != -1)
			{
				actualWidth = vidmodes[best_fit]->hdisplay;
				actualHeight = vidmodes[best_fit]->vdisplay;

				// change to the mode
				XF86VidModeSwitchToMode(dpy, scrnum, vidmodes[best_fit]);
				vidmode_active = true;

				// Move the viewport to top left
				XF86VidModeSetViewPort(dpy, scrnum, 0, 0);

				Log::write("XFree86-VidModeExtension Activated at %dx%d\n",
					actualWidth, actualHeight);
			}
			else
			{
				fullscreen = 0;
				Log::write("XFree86-VidModeExtension: No acceptable modes found\n");
			}
		}
		else
		{
			Log::write("XFree86-VidModeExtension:  Ignored on non-fullscreen\n");
		}
	}

	if (!colorbits)
	{
		colorbits = !r_colorbits->value ? 24 : r_colorbits->value;
	}
	int depthbits = !r_depthbits->value ? 24 : r_depthbits->value;
	int stencilbits = r_stencilbits->value;

	XVisualInfo* visinfo = NULL;
	for (int i = 0; i < 16; i++)
	{
		// 0 - default
		// 1 - minus colorbits
		// 2 - minus depthbits
		// 3 - minus stencil
		if ((i % 4) == 0 && i)
		{
			// one pass, reduce
			switch (i / 4)
			{
			case 2:
				if (colorbits == 24)
				{
					colorbits = 16;
				}
				break;
			case 1:
				if (depthbits == 24)
				{
					depthbits = 16;
				}
				else if (depthbits == 16)
				{
					depthbits = 8;
				}
				break;
			case 3:
				if (stencilbits == 24)
				{
					stencilbits = 16;
				}
				else if (stencilbits == 16)
				{
					stencilbits = 8;
				}
				break;
			}
		}

		int tcolorbits = colorbits;
		int tdepthbits = depthbits;
		int tstencilbits = stencilbits;

		if ((i % 4) == 3)
		{
			// reduce colorbits
			if (tcolorbits == 24)
			{
				tcolorbits = 16;
			}
		}

		if ((i % 4) == 2)
		{
			// reduce depthbits
			if (tdepthbits == 24)
			{
				tdepthbits = 16;
			}
			else if (tdepthbits == 16)
			{
				tdepthbits = 8;
			}
		}

		if ((i % 4) == 1)
		{
			// reduce stencilbits
			if (tstencilbits == 24)
			{
				tstencilbits = 16;
			}
			else if (tstencilbits == 16)
			{
				tstencilbits = 8;
			}
			else
			{
				tstencilbits = 0;
			}
		}

		int attrib[] =
		{
			GLX_RGBA,			// 0
			GLX_RED_SIZE, 4,		// 1, 2
			GLX_GREEN_SIZE, 4,		// 3, 4
			GLX_BLUE_SIZE, 4,		// 5, 6
			GLX_DOUBLEBUFFER,		// 7
			GLX_DEPTH_SIZE, 1,		// 8, 9
			GLX_STENCIL_SIZE, 1,	// 10, 11
			//GLX_SAMPLES_SGIS, 4, /* for better AA */
			None
		};
		// these match in the array
		enum
		{
			ATTR_RED_IDX = 2,
			ATTR_GREEN_IDX = 4,
			ATTR_BLUE_IDX = 6,
			ATTR_DEPTH_IDX = 9,
			ATTR_STENCIL_IDX = 11
		};

		if (tcolorbits == 24)
		{
			attrib[ATTR_RED_IDX] = 8;
			attrib[ATTR_GREEN_IDX] = 8;
			attrib[ATTR_BLUE_IDX] = 8;
		}
		else
		{
			// must be 16 bit
			attrib[ATTR_RED_IDX] = 4;
			attrib[ATTR_GREEN_IDX] = 4;
			attrib[ATTR_BLUE_IDX] = 4;
		}

		attrib[ATTR_DEPTH_IDX] = tdepthbits;// default to 24 depth
		attrib[ATTR_STENCIL_IDX] = tstencilbits;

		visinfo = glXChooseVisual(dpy, scrnum, attrib);
		if (!visinfo)
		{
			continue;
		}

		Log::write("Using %d/%d/%d Color bits, %d depth, %d stencil display.\n",
			attrib[ATTR_RED_IDX], attrib[ATTR_GREEN_IDX], attrib[ATTR_BLUE_IDX],
			attrib[ATTR_DEPTH_IDX], attrib[ATTR_STENCIL_IDX]);

		glConfig.colorBits = tcolorbits;
		glConfig.depthBits = tdepthbits;
		glConfig.stencilBits = tstencilbits;
		break;
	}

	if (!visinfo)
	{
		Log::write("Couldn't get a visual\n");
		return RSERR_INVALID_MODE;
	}

	//	Window attributes
	XSetWindowAttributes attr;
	unsigned long mask;
	attr.background_pixel = BlackPixel(dpy, scrnum);
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
	attr.event_mask = X_MASK;
	if (vidmode_active)
	{
		mask = CWBackPixel | CWColormap | CWSaveUnder | CWBackingStore |
			   CWEventMask | CWOverrideRedirect;
		attr.override_redirect = True;
		attr.backing_store = NotUseful;
		attr.save_under = False;
	}
	else
	{
		mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
	}

	win = XCreateWindow(dpy, root, 0, 0,
		actualWidth, actualHeight,
		0, visinfo->depth, InputOutput,
		visinfo->visual, mask, &attr);

	XStoreName(dpy, win, R_GetTitleForWindow());

	/* GH: Don't let the window be resized */
	XSizeHints sizehints;
	sizehints.flags = PMinSize | PMaxSize;
	sizehints.min_width = sizehints.max_width = actualWidth;
	sizehints.min_height = sizehints.max_height = actualHeight;

	XSetWMNormalHints(dpy, win, &sizehints);

	XMapWindow(dpy, win);

	if (vidmode_active)
	{
		XMoveWindow(dpy, win, 0, 0);
		//XRaiseWindow(dpy, win);
		//XWarpPointer(dpy, None, win, 0, 0, 0, 0, 0, 0);
		//XFlush(dpy);
		// Move the viewport to top left
		//XF86VidModeSetViewPort(dpy, scrnum, 0, 0);
	}

	//	hook to window close
	wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(dpy, win, &wm_delete_window, 1);

	XFlush(dpy);

	XSync(dpy, False);	// bk001130 - from cvs1.17 (mkv)
	ctx = glXCreateContext(dpy, visinfo, NULL, True);
	XSync(dpy, False);	// bk001130 - from cvs1.17 (mkv)

	/* GH: Free the visinfo after we're done with it */
	XFree(visinfo);

	glXMakeCurrent(dpy, win, ctx);

	glConfig.deviceSupportsGamma = false;
	if (vidmode_ext)
	{
		if (vidmode_MajorVersion < GAMMA_MINMAJOR ||
			(vidmode_MajorVersion == GAMMA_MINMAJOR && vidmode_MinorVersion < GAMMA_MINMINOR))
		{
			Log::write("XF86 Gamma extension not supported in this version\n");
		}
		else
		{
			XF86VidModeGetGamma(dpy, scrnum, &vidmode_InitialGamma);
			Log::write("XF86 Gamma extension initialized\n");
			glConfig.deviceSupportsGamma = true;
		}
	}

	//	Check for software GL implementation.
	const char* glstring = (char*)glGetString(GL_RENDERER);
	if (!String::ICmp(glstring, "Mesa X11") ||
		!String::ICmp(glstring, "Mesa GLX Indirect"))
	{
		if (!r_allowSoftwareGL->integer)
		{
			Log::write("\n\n***********************************************************\n");
			Log::write(" You are using software Mesa (no hardware acceleration)!\n");
			Log::write(" If this is intentional, add\n");
			Log::write("       \"+set r_allowSoftwareGL 1\"\n");
			Log::write(" to the command line when starting the game.\n");
			Log::write("***********************************************************\n");
			GLimp_Shutdown();
			return RSERR_INVALID_MODE;
		}
		else
		{
			Log::write("...using software Mesa (r_allowSoftwareGL==1).\n");
		}
	}

	GLW_GenDefaultLists();

	return RSERR_OK;
}

static void GLW_DeleteDefaultLists()
{
	if (!fontbase_init)
	{
		common->DPrintf("ERROR: GLW_DeleteDefaultLists: no font list initialized\n");
		return;
	}
	glDeleteLists(gl_NormalFontBase, 256);
	fontbase_init = false;
}

//==========================================================================
//
//	GLimp_Shutdown
//
//	This routine does all OS specific shutdown procedures for the OpenGL
// subsystem. This means deleting the rendering context, destroying the
// window and restoring video mode. The state structure is also nulled out.
//
//==========================================================================

void GLimp_Shutdown()
{
	IN_DeactivateMouse();
	if (dpy)
	{
		GLW_DeleteDefaultLists();

		if (ctx)
		{
			glXDestroyContext(dpy, ctx);
		}
		if (win)
		{
			XDestroyWindow(dpy, win);
		}
		if (vidmode_active)
		{
			XF86VidModeSwitchToMode(dpy, scrnum, vidmodes[0]);
		}
		if (glConfig.deviceSupportsGamma)
		{
			XF86VidModeSetGamma(dpy, scrnum, &vidmode_InitialGamma);
		}
		// NOTE TTimo opening/closing the display should be necessary only once per run
		//   but it seems QGL_Shutdown gets called in a lot of occasion
		//   in some cases, this XCloseDisplay is known to raise some X errors
		//   ( https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=33 )
		XCloseDisplay(dpy);
	}
	vidmode_active = false;
	dpy = NULL;
	win = 0;
	ctx = NULL;

	Com_Memset(&glConfig, 0, sizeof(glConfig));
	Com_Memset(&glState, 0, sizeof(glState));
}

const char* GLimp_GetSystemExtensionsString()
{
	return glXQueryExtensionsString(dpy, scrnum);
}

//==========================================================================
//
//	GLimp_GetProcAddress
//
//==========================================================================

void* GLimp_GetProcAddress(const char* Name)
{
	return (void*)glXGetProcAddress((const GLubyte*)Name);
}

//==========================================================================
//
//	GLimp_SetGamma
//
//	This routine should only be called if glConfig.deviceSupportsGamma is TRUE
//
//==========================================================================

void GLimp_SetGamma(unsigned char red[256], unsigned char green[256], unsigned char blue[256])
{
	if (!glConfig.deviceSupportsGamma)
	{
		return;
	}

	// NOTE TTimo we get the gamma value from cvar, because we can't work with the s_gammatable
	//   the API wasn't changed to avoid breaking other OSes
	float g = r_gamma->value;
	XF86VidModeGamma gamma;
	gamma.red = g;
	gamma.green = g;
	gamma.blue = g;
	XF86VidModeSetGamma(dpy, scrnum, &gamma);
}

//==========================================================================
//
//	GLimp_SwapBuffers
//
//==========================================================================

void GLimp_SwapBuffers()
{
	glXSwapBuffers(dpy, win);
}

//**************************************************************************
//
//	SMP acceleration
//
//**************************************************************************

//==========================================================================
//
//	GLimp_RenderThreadWrapper
//
//==========================================================================

static void* GLimp_RenderThreadWrapper(void* arg)
{
	Log::write("Render thread starting\n");

	glimpRenderThread();

	glXMakeCurrent(dpy, None, NULL);

	Log::write("Render thread terminating\n");

	return arg;
}

//==========================================================================
//
//	GLimp_SpawnRenderThread
//
//==========================================================================

bool GLimp_SpawnRenderThread(void (* function)())
{
#ifdef WOLF_SMP
	sem_init(&renderCommandsEvent, 0, 0);
	sem_init(&renderCompletedEvent, 0, 0);
	sem_init(&renderActiveEvent, 0, 0);
#else
	pthread_mutex_init(&smpMutex, NULL);

	pthread_cond_init(&renderCommandsEvent, NULL);
	pthread_cond_init(&renderCompletedEvent, NULL);
#endif

	glimpRenderThread = function;

	pthread_t renderThread;
	int ret = pthread_create(&renderThread, NULL, GLimp_RenderThreadWrapper, NULL);
	if (ret)
	{
		Log::write("pthread_create returned %d: %s", ret, strerror(ret));
		return false;
	}

#ifndef WOLF_SMP
	ret = pthread_detach(renderThread);
	if (ret)
	{
		Log::write("pthread_detach returned %d: %s", ret, strerror(ret));
	}
#endif

	return true;
}

//==========================================================================
//
//	GLimp_RendererSleep
//
//==========================================================================

void* GLimp_RendererSleep()
{
#ifdef WOLF_SMP
	// after this, the front end can exit GLimp_FrontEndSleep
	sem_post(&renderCompletedEvent);

	sem_wait(&renderCommandsEvent);

	void* data = (void*)smpData;

	// after this, the main thread can exit GLimp_WakeRenderer
	sem_post(&renderActiveEvent);
#else
	glXMakeCurrent(dpy, None, NULL);

	pthread_mutex_lock(&smpMutex);

	smpData = NULL;
	smpDataReady = false;

	// after this, the front end can exit GLimp_FrontEndSleep
	pthread_cond_signal(&renderCompletedEvent);

	while (!smpDataReady)
	{
		pthread_cond_wait(&renderCommandsEvent, &smpMutex);
	}

	void* data = (void*)smpData;
	pthread_mutex_unlock(&smpMutex);

	glXMakeCurrent(dpy, win, ctx);
#endif

	return data;
}

//==========================================================================
//
//	GLimp_FrontEndSleep
//
//==========================================================================

void GLimp_FrontEndSleep()
{
#ifdef WOLF_SMP
	sem_wait(&renderCompletedEvent);
#else
	pthread_mutex_lock(&smpMutex);
	while (smpData)
	{
		pthread_cond_wait(&renderCompletedEvent, &smpMutex);
	}
	pthread_mutex_unlock(&smpMutex);

	glXMakeCurrent(dpy, win, ctx);
#endif
}

//==========================================================================
//
//	GLimp_WakeRenderer
//
//==========================================================================

void GLimp_WakeRenderer(void* data)
{
#ifdef WOLF_SMP
	smpData = data;

	// after this, the renderer can continue through GLimp_RendererSleep
	sem_post(&renderCommandsEvent);

	sem_wait(&renderActiveEvent);
#else
	glXMakeCurrent(dpy, None, NULL);

	pthread_mutex_lock(&smpMutex);
	qassert(smpData == NULL);
	smpData = data;
	smpDataReady = true;

	// after this, the renderer can continue through GLimp_RendererSleep
	pthread_cond_signal(&renderCommandsEvent);
	pthread_mutex_unlock(&smpMutex);
#endif
}
