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

Window root;
int actualWidth, actualHeight;
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
	root = RootWindow(dpy, scrnum);

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

	actualWidth = width;
	actualHeight = height;

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

	return RSERR_OK;
}
