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

rserr_t GLimp_GLXSharedInit()
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

	return RSERR_OK;
}
