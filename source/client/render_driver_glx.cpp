//**************************************************************************
//**
//**	$Id$
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "render_local.h"
#include "unix_shared.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

Display*				dpy = NULL;
int						scrnum;
Window					win;
GLXContext				ctx = NULL;

bool					mouse_avail;
bool					mouse_active;
int						mwx, mwy;
int						mx = 0, my = 0;

QCvar*					in_mouse;
QCvar*					in_dgamouse; // user pref for dga mouse

int						win_x, win_y;

XF86VidModeModeInfo**	vidmodes;
int						num_vidmodes;
bool					vidmode_active = false;
bool					vidmode_ext = false;
int						vidmode_MajorVersion = 0, vidmode_MinorVersion = 0; // major and minor of XF86VidExtensions

// CODE --------------------------------------------------------------------

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
//	GLimp_GLXSharedInit
//
//==========================================================================

rserr_t GLimp_GLXSharedInit(int width, int height, bool fullscreen)
{
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
		GLog.Write("Using XFree86-VidModeExtension Version %d.%d\n", vidmode_MajorVersion, vidmode_MinorVersion);
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
			GLog.Write("Failed to detect XF86DGA Mouse\n");
			Cvar_Set("in_dgamouse", "0");
		}
		else
		{
			GLog.Write("XF86DGA Mouse (Version %d.%d) initialized\n", dga_MajorVersion, dga_MinorVersion);
		}
	}

	int actualWidth = width;
	int actualHeight = height;

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
				if (width > vidmodes[i]->hdisplay ||
					height > vidmodes[i]->vdisplay)
					continue;

				x = width - vidmodes[i]->hdisplay;
				y = height - vidmodes[i]->vdisplay;
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

				GLog.Write("XFree86-VidModeExtension Activated at %dx%d\n",
					actualWidth, actualHeight);
			}
			else
			{
				fullscreen = 0;
				GLog.Write("XFree86-VidModeExtension: No acceptable modes found\n");
			}
		}
		else
		{
			GLog.Write("XFree86-VidModeExtension:  Ignored on non-fullscreen\n");
		}
	}

	int colorbits = !r_colorbits->value ? 24 : r_colorbits->value;
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
			case 2 :
				if (colorbits == 24)
				{
					colorbits = 16;
				}
				break;
			case 1 :
				if (depthbits == 24)
				{
					depthbits = 16;
				}
				else if (depthbits == 16)
				{
					depthbits = 8;
				}
				break;
			case 3 :
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
			GLX_RGBA,         // 0
			GLX_RED_SIZE, 4,      // 1, 2
			GLX_GREEN_SIZE, 4,      // 3, 4
			GLX_BLUE_SIZE, 4,     // 5, 6
			GLX_DOUBLEBUFFER,     // 7
			GLX_DEPTH_SIZE, 1,      // 8, 9
			GLX_STENCIL_SIZE, 1,    // 10, 11
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

		attrib[ATTR_DEPTH_IDX] = tdepthbits; // default to 24 depth
		attrib[ATTR_STENCIL_IDX] = tstencilbits;

		visinfo = glXChooseVisual(dpy, scrnum, attrib);
		if (!visinfo)
		{
			continue;
		}

		GLog.Write("Using %d/%d/%d Color bits, %d depth, %d stencil display.\n", 
			attrib[ATTR_RED_IDX], attrib[ATTR_GREEN_IDX], attrib[ATTR_BLUE_IDX],
			attrib[ATTR_DEPTH_IDX], attrib[ATTR_STENCIL_IDX]);

		glConfig.colorBits = tcolorbits;
		glConfig.depthBits = tdepthbits;
		glConfig.stencilBits = tstencilbits;
		break;
	}

	if (!visinfo)
	{
		GLog.Write("Couldn't get a visual\n");
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

	if (GGameType & GAME_Quake)
	{
		XStoreName(dpy, win, "Quake");
	}
	else if (GGameType & GAME_Hexen2)
	{
		XStoreName(dpy, win, "Hexen II");
	}
	else if (GGameType & GAME_Quake2)
	{
		XStoreName(dpy, win, "Quake II");
	}
	else if (GGameType & GAME_Quake3)
	{
		XStoreName(dpy, win, "Quake III: Arena");
	}

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

	XFlush(dpy);

	XSync(dpy, False); // bk001130 - from cvs1.17 (mkv)
	ctx = glXCreateContext(dpy, visinfo, NULL, True);
	XSync(dpy, False); // bk001130 - from cvs1.17 (mkv)

	/* GH: Free the visinfo after we're done with it */
	XFree(visinfo);

	glXMakeCurrent(dpy, win, ctx);

	return RSERR_OK;
}
