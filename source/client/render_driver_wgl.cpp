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
#include "win_shared.h"

// MACROS ------------------------------------------------------------------

//	Copied from resources.h
#define IDI_ICON1                       1

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

HDC		maindc;
HGLRC	baseRC;

int		 desktopBitsPixel;
int		 desktopWidth, desktopHeight;

static bool		s_classRegistered = false;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	GLW_ChoosePFD
//
//	Helper function that replaces ChoosePixelFormat.
//
//==========================================================================

#define MAX_PFDS 256

int GLW_ChoosePFD(HDC hDC, PIXELFORMATDESCRIPTOR* pPFD)
{
	PIXELFORMATDESCRIPTOR pfds[MAX_PFDS+1];
	int maxPFD = 0;
	int i;
	int bestMatch = 0;

	GLog.Write("...GLW_ChoosePFD( %d, %d, %d )\n", (int)pPFD->cColorBits, (int)pPFD->cDepthBits, (int)pPFD->cStencilBits);

	// count number of PFDs
	maxPFD = DescribePixelFormat(hDC, 1, sizeof(PIXELFORMATDESCRIPTOR), &pfds[0]);
	if (maxPFD > MAX_PFDS)
	{
		GLog.Write(S_COLOR_YELLOW "...numPFDs > MAX_PFDS (%d > %d)\n", maxPFD, MAX_PFDS);
		maxPFD = MAX_PFDS;
	}

	GLog.Write("...%d PFDs found\n", maxPFD - 1);

	// grab information
	for (i = 1; i <= maxPFD; i++)
	{
		DescribePixelFormat(hDC, i, sizeof(PIXELFORMATDESCRIPTOR), &pfds[i]);
	}

	// look for a best match
	for (i = 1; i <= maxPFD; i++)
	{
		//
		// make sure this has hardware acceleration
		//
		if ((pfds[i].dwFlags & PFD_GENERIC_FORMAT ) != 0)
		{
			if (!r_allowSoftwareGL->integer)
			{
				if (r_verbose->integer)
				{
					GLog.Write("...PFD %d rejected, software acceleration\n", i);
				}
				continue;
			}
		}

		// verify pixel type
		if (pfds[i].iPixelType != PFD_TYPE_RGBA)
		{
			if (r_verbose->integer)
			{
				GLog.Write("...PFD %d rejected, not RGBA\n", i);
			}
			continue;
		}

		// verify proper flags
		if (((pfds[i].dwFlags & pPFD->dwFlags) & pPFD->dwFlags) != pPFD->dwFlags)
		{
			if (r_verbose->integer)
			{
				GLog.Write("...PFD %d rejected, improper flags (%x instead of %x)\n", i, pfds[i].dwFlags, pPFD->dwFlags);
			}
			continue;
		}

		// verify enough bits
		if (pfds[i].cDepthBits < 15)
		{
			continue;
		}
		if ((pfds[i].cStencilBits < 4) && (pPFD->cStencilBits > 0))
		{
			continue;
		}

		//
		// selection criteria (in order of priority):
		// 
		//  PFD_STEREO
		//  colorBits
		//  depthBits
		//  stencilBits
		//
		if (bestMatch)
		{
			// check stereo
			if ((pfds[i].dwFlags & PFD_STEREO) && !(pfds[bestMatch].dwFlags & PFD_STEREO) && (pPFD->dwFlags & PFD_STEREO))
			{
				bestMatch = i;
				continue;
			}
			
			if (!(pfds[i].dwFlags & PFD_STEREO) && (pfds[bestMatch].dwFlags & PFD_STEREO) && (pPFD->dwFlags & PFD_STEREO))
			{
				bestMatch = i;
				continue;
			}

			// check color
			if (pfds[bestMatch].cColorBits != pPFD->cColorBits)
			{
				// prefer perfect match
				if (pfds[i].cColorBits == pPFD->cColorBits)
				{
					bestMatch = i;
					continue;
				}
				// otherwise if this PFD has more bits than our best, use it
				else if (pfds[i].cColorBits > pfds[bestMatch].cColorBits)
				{
					bestMatch = i;
					continue;
				}
			}

			// check depth
			if (pfds[bestMatch].cDepthBits != pPFD->cDepthBits)
			{
				// prefer perfect match
				if (pfds[i].cDepthBits == pPFD->cDepthBits)
				{
					bestMatch = i;
					continue;
				}
				// otherwise if this PFD has more bits than our best, use it
				else if (pfds[i].cDepthBits > pfds[bestMatch].cDepthBits)
				{
					bestMatch = i;
					continue;
				}
			}

			// check stencil
			if (pfds[bestMatch].cStencilBits != pPFD->cStencilBits)
			{
				// prefer perfect match
				if (pfds[i].cStencilBits == pPFD->cStencilBits)
				{
					bestMatch = i;
					continue;
				}
				// otherwise if this PFD has more bits than our best, use it
				else if ((pfds[i].cStencilBits > pfds[bestMatch].cStencilBits) &&
					 (pPFD->cStencilBits > 0))
				{
					bestMatch = i;
					continue;
				}
			}
		}
		else
		{
			bestMatch = i;
		}
	}
	
	if (!bestMatch)
	{
		return 0;
	}

	if ((pfds[bestMatch].dwFlags & PFD_GENERIC_FORMAT) != 0)
	{
		if (!r_allowSoftwareGL->integer)
		{
			GLog.Write("...no hardware acceleration found\n");
			return 0;
		}
		else
		{
			GLog.Write("...using software emulation\n");
		}
	}
	else if (pfds[bestMatch].dwFlags & PFD_GENERIC_ACCELERATED)
	{
		GLog.Write("...MCD acceleration found\n");
	}
	else
	{
		GLog.Write("...hardware acceleration found\n");
	}

	*pPFD = pfds[bestMatch];

	return bestMatch;
}

//==========================================================================
//
//	GLW_SharedCreateWindow
//
//==========================================================================

void GLW_SharedCreateWindow(int width, int height, bool fullscreen)
{
	//
	// register the window class if necessary
	//
	if (!s_classRegistered)
	{
		WNDCLASS wc;

		Com_Memset(&wc, 0, sizeof(wc));

		wc.style = 0;
		wc.lpfnWndProc = MainWndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = global_hInstance;
		wc.hIcon = LoadIcon(global_hInstance, MAKEINTRESOURCE(IDI_ICON1));
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)COLOR_GRAYTEXT;
		wc.lpszMenuName = 0;
		wc.lpszClassName = WINDOW_CLASS_NAME;

		if (!RegisterClass(&wc))
		{
			throw QException("GLW_CreateWindow: could not register window class");
		}
		s_classRegistered = true;
		GLog.Write("...registered window class\n");
	}

	//
	// create the HWND if one does not already exist
	//
	if (!GMainWindow)
	{
		//
		// compute width and height
		//
		RECT r;
		r.left = 0;
		r.top = 0;
		r.right  = width;
		r.bottom = height;

		int exstyle;
		int stylebits;
		if (fullscreen)
		{
			exstyle = WS_EX_TOPMOST;
			stylebits = WS_POPUP | WS_VISIBLE | WS_SYSMENU;
		}
		else
		{
			exstyle = 0;
			stylebits = WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_VISIBLE | WS_SYSMENU;// | WS_MINIMIZEBOX
			AdjustWindowRect(&r, stylebits, FALSE);
		}

		int w = r.right - r.left;
		int h = r.bottom - r.top;

		int x, y;
		if (fullscreen)
		{
			x = 0;
			y = 0;
		}
		else
		{
			QCvar* vid_xpos = Cvar_Get("vid_xpos", "", 0);
			QCvar* vid_ypos = Cvar_Get("vid_ypos", "", 0);
			x = vid_xpos->integer;
			y = vid_ypos->integer;

			// adjust window coordinates if necessary 
			// so that the window is completely on screen
			if (x < 0)
			{
				x = 0;
			}
			if (y < 0)
			{
				y = 0;
			}

			if (w < desktopWidth && h < desktopHeight)
			{
				if (x + w > desktopWidth)
				{
					x = (desktopWidth - w);
				}
				if (y + h > desktopHeight)
				{
					y = (desktopHeight - h);
				}
			}
		}

		const char* Caption;
		if (GGameType & GAME_QuakeWorld)
		{
			Caption = "QuakeWorld";
		}
		else if (GGameType & GAME_Quake)
		{
			Caption = "Quake";
		}
		else if (GGameType & GAME_HexenWorld)
		{
			Caption = "HexenWorld";
		}
		else if (GGameType & GAME_Hexen2)
		{
			Caption = "Hexen II";
		}
		else if (GGameType & GAME_Quake2)
		{
			Caption = "Quake 2";
		}
		else if (GGameType & GAME_Quake3)
		{
			Caption = "Quake 3: Arena";
		}
		else
		{
			Caption = "Unknown";
		}

		GMainWindow = CreateWindowEx(exstyle, WINDOW_CLASS_NAME, Caption, stylebits,
			x, y, w, h, NULL, NULL, global_hInstance, NULL);

		if (!GMainWindow)
		{
			throw QException("GLW_CreateWindow() - Couldn't create window");
		}

		ShowWindow(GMainWindow, SW_SHOW);
		UpdateWindow(GMainWindow);
		GLog.Write("...created window@%d,%d (%dx%d)\n", x, y, w, h);
	}
	else
	{
		GLog.Write("...window already present, CreateWindowEx skipped\n");
	}
}

//==========================================================================
//
//	GLimp_GetProcAddress
//
//==========================================================================

void* GLimp_GetProcAddress(const char* Name)
{
	return (void*)wglGetProcAddress(Name);
}
