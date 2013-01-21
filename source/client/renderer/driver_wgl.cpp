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

#include "local.h"
#include "../windows_shared.h"
#include "../../common/Common.h"
#include "../../common/strings.h"
#include "../../common/command_buffer.h"
#include "../sound/public.h"
#include "../input/public.h"

// MACROS ------------------------------------------------------------------

#define WINDOW_CLASS_NAME   "JLQuake"

enum
{
	TRY_PFD_SUCCESS     = 0,
	TRY_PFD_FAIL_SOFT   = 1,
	TRY_PFD_FAIL_HARD   = 2,
};

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static HDC maindc;
static HGLRC baseRC;

static bool s_alttab_disabled;

static int desktopBitsPixel;
static int desktopWidth;
static int desktopHeight;

static bool s_classRegistered = false;
static bool pixelFormatSet;
static bool cdsFullscreen;

static Cvar* vid_xpos;				// X coordinate of window position
static Cvar* vid_ypos;				// Y coordinate of window position

static bool fontbase_init = false;

static quint16 s_oldHardwareGamma[3][256];

static HANDLE renderCommandsEvent;
static HANDLE renderCompletedEvent;
static HANDLE renderActiveEvent;

static void (* glimpRenderThread)();

static HANDLE renderThreadHandle;
static DWORD renderThreadId;

static void* smpData;
static int wglErrors;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	WIN_DisableAltTab
//
//==========================================================================

static void WIN_DisableAltTab()
{
	if (s_alttab_disabled)
	{
		return;
	}

	RegisterHotKey(0, 0, MOD_ALT, VK_TAB);

	s_alttab_disabled = true;
}

//==========================================================================
//
//	WIN_EnableAltTab
//
//==========================================================================

static void WIN_EnableAltTab()
{
	if (!s_alttab_disabled)
	{
		return;
	}

	UnregisterHotKey(0, 0);

	s_alttab_disabled = false;
}

//==========================================================================
//
//	AppActivate
//
//==========================================================================

static void AppActivate(bool fActive, bool minimize)
{
	Minimized = minimize;

	common->DPrintf("AppActivate: %i\n", fActive);

	Key_ClearStates();	// FIXME!!!

	// we don't want to act like we're active if we're minimized
	ActiveApp = fActive && !Minimized;

	// minimize/restore mouse-capture on demand
	IN_Activate(ActiveApp);
	CDAudio_Activate(ActiveApp);
	SNDDMA_Activate();
}

//==========================================================================
//
//	MainWndProc
//
//	main window procedure
//
//==========================================================================

static LRESULT WINAPI MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		GMainWindow = hWnd;
		if (r_fullscreen->integer)
		{
			WIN_DisableAltTab();
		}
		else
		{
			WIN_EnableAltTab();
		}
		break;

	case WM_DESTROY:
		// let sound and input know about this?
		GMainWindow = NULL;
		if (r_fullscreen->integer)
		{
			WIN_EnableAltTab();
		}
		break;

	case WM_CLOSE:
		Cbuf_ExecuteText(EXEC_APPEND, "quit");
		break;

	case WM_ACTIVATE:
		AppActivate(LOWORD(wParam) != WA_INACTIVE, !!HIWORD(wParam));
		break;

	case WM_MOVE:
		if (!r_fullscreen->integer)
		{
			int xPos = (short)LOWORD(lParam);	// horizontal position
			int yPos = (short)HIWORD(lParam);	// vertical position

			RECT r;
			r.left   = 0;
			r.top    = 0;
			r.right  = 1;
			r.bottom = 1;

			int style = GetWindowLong(hWnd, GWL_STYLE);
			AdjustWindowRect(&r, style, FALSE);

			Cvar_SetValue("vid_xpos", xPos + r.left);
			Cvar_SetValue("vid_ypos", yPos + r.top);
			vid_xpos->modified = false;
			vid_ypos->modified = false;
			if (ActiveApp)
			{
				IN_Activate(true);
			}
		}
		break;

	case WM_SYSCOMMAND:
		if (wParam == SC_SCREENSAVE)
		{
			return 0;
		}
		break;

	case WM_SYSKEYDOWN:
		if (wParam == 13)
		{
			if (r_fullscreen)
			{
				Cvar_SetValue("r_fullscreen", !r_fullscreen->integer);
				Cbuf_AddText("vid_restart\n");
			}
			return 0;
		}
		// fall through
		break;

	case MM_MCINOTIFY:
		if (CDAudio_MessageHandler(hWnd, uMsg, wParam, lParam) == 0)
		{
			return 0;
		}
		break;
	}

	if (IN_HandleInputMessage(uMsg, wParam, lParam))
	{
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//==========================================================================
//
//	GLW_ChoosePFD
//
//	Helper function that replaces ChoosePixelFormat.
//
//==========================================================================

#define MAX_PFDS 256

static int GLW_ChoosePFD(HDC hDC, PIXELFORMATDESCRIPTOR* pPFD)
{
	common->Printf("...GLW_ChoosePFD( %d, %d, %d )\n", (int)pPFD->cColorBits, (int)pPFD->cDepthBits, (int)pPFD->cStencilBits);

	// count number of PFDs
	PIXELFORMATDESCRIPTOR pfds[MAX_PFDS + 1];
	int maxPFD = DescribePixelFormat(hDC, 1, sizeof(PIXELFORMATDESCRIPTOR), &pfds[0]);
	if (maxPFD > MAX_PFDS)
	{
		common->Printf(S_COLOR_YELLOW "...numPFDs > MAX_PFDS (%d > %d)\n", maxPFD, MAX_PFDS);
		maxPFD = MAX_PFDS;
	}

	common->Printf("...%d PFDs found\n", maxPFD - 1);

	// grab information
	for (int i = 1; i <= maxPFD; i++)
	{
		DescribePixelFormat(hDC, i, sizeof(PIXELFORMATDESCRIPTOR), &pfds[i]);
	}

	// look for a best match
	int bestMatch = 0;
	for (int i = 1; i <= maxPFD; i++)
	{
		//
		// make sure this has hardware acceleration
		//
		if ((pfds[i].dwFlags & PFD_GENERIC_FORMAT) != 0)
		{
			if (!r_allowSoftwareGL->integer)
			{
				if (r_verbose->integer)
				{
					common->Printf("...PFD %d rejected, software acceleration\n", i);
				}
				continue;
			}
		}

		// verify pixel type
		if (pfds[i].iPixelType != PFD_TYPE_RGBA)
		{
			if (r_verbose->integer)
			{
				common->Printf("...PFD %d rejected, not RGBA\n", i);
			}
			continue;
		}

		// verify proper flags
		if (((pfds[i].dwFlags & pPFD->dwFlags) & pPFD->dwFlags) != pPFD->dwFlags)
		{
			if (r_verbose->integer)
			{
				common->Printf("...PFD %d rejected, improper flags (%x instead of %x)\n", i, pfds[i].dwFlags, pPFD->dwFlags);
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
			common->Printf("...no hardware acceleration found\n");
			return 0;
		}
		else
		{
			common->Printf("...using software emulation\n");
		}
	}
	else if (pfds[bestMatch].dwFlags & PFD_GENERIC_ACCELERATED)
	{
		common->Printf("...MCD acceleration found\n");
	}
	else
	{
		common->Printf("...hardware acceleration found\n");
	}

	*pPFD = pfds[bestMatch];

	return bestMatch;
}

//==========================================================================
//
//	GLW_CreatePFD
//
//	Helper function zeros out then fills in a PFD.
//
//==========================================================================

static void GLW_CreatePFD(PIXELFORMATDESCRIPTOR* pPFD, int colorbits, int depthbits, int stencilbits, bool stereo)
{
	PIXELFORMATDESCRIPTOR src =
	{
		sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
		1,								// version number
		PFD_DRAW_TO_WINDOW |			// support window
		PFD_SUPPORT_OPENGL |			// support OpenGL
		PFD_DOUBLEBUFFER,				// double buffered
		PFD_TYPE_RGBA,					// RGBA type
		24,								// 24-bit color depth
		0, 0, 0, 0, 0, 0,				// color bits ignored
		0,								// no alpha buffer
		0,								// shift bit ignored
		0,								// no accumulation buffer
		0, 0, 0, 0,						// accum bits ignored
		24,								// 24-bit z-buffer
		8,								// 8-bit stencil buffer
		0,								// no auxiliary buffer
		PFD_MAIN_PLANE,					// main layer
		0,								// reserved
		0, 0, 0							// layer masks ignored
	};

	src.cColorBits = colorbits;
	src.cDepthBits = depthbits;
	src.cStencilBits = stencilbits;

	if (stereo)
	{
		common->Printf("...attempting to use stereo\n");
		src.dwFlags |= PFD_STEREO;
		glConfig.stereoEnabled = true;
	}
	else
	{
		glConfig.stereoEnabled = false;
	}

	*pPFD = src;
}

//==========================================================================
//
//	GLW_MakeContext
//
//==========================================================================

static int GLW_MakeContext(PIXELFORMATDESCRIPTOR* pPFD)
{
	//
	// don't putz around with pixelformat if it's already set (e.g. this is a soft
	// reset of the graphics system)
	//
	if (!pixelFormatSet)
	{
		//
		// choose, set, and describe our desired pixel format.
		//
		int pixelformat = GLW_ChoosePFD(maindc, pPFD);
		if (pixelformat == 0)
		{
			common->Printf("...GLW_ChoosePFD failed\n");
			return TRY_PFD_FAIL_SOFT;
		}
		common->Printf("...PIXELFORMAT %d selected\n", pixelformat);

		DescribePixelFormat(maindc, pixelformat, sizeof(*pPFD), pPFD);

		if (SetPixelFormat(maindc, pixelformat, pPFD) == FALSE)
		{
			common->Printf("...SetPixelFormat failed\n");
			return TRY_PFD_FAIL_SOFT;
		}

		pixelFormatSet = true;
	}

	//
	// startup the OpenGL subsystem by creating a context and making it current
	//
	if (!baseRC)
	{
		common->Printf("...creating GL context: ");
		if ((baseRC = wglCreateContext(maindc)) == 0)
		{
			common->Printf("failed\n");
			return TRY_PFD_FAIL_HARD;
		}
		common->Printf("succeeded\n");

		common->Printf("...making context current: ");
		if (!wglMakeCurrent(maindc, baseRC))
		{
			wglDeleteContext(baseRC);
			baseRC = NULL;
			common->Printf("failed\n");
			return TRY_PFD_FAIL_HARD;
		}
		common->Printf("succeeded\n");
	}

	return TRY_PFD_SUCCESS;
}

//==========================================================================
//
//	GLW_InitDriver
//
//	- get a DC if one doesn't exist
//	- create an HGLRC if one doesn't exist
//
//==========================================================================

static bool GLW_InitDriver(int colorbits)
{
	static PIXELFORMATDESCRIPTOR pfd;		// save between frames since 'tr' gets cleared

	common->Printf("Initializing OpenGL driver\n");

	//
	// get a DC for our window if we don't already have one allocated
	//
	if (maindc == NULL)
	{
		common->Printf("...getting DC: ");

		if ((maindc = GetDC(GMainWindow)) == NULL)
		{
			common->Printf("failed\n");
			return false;
		}
		common->Printf("succeeded\n");
	}

	if (colorbits == 0)
	{
		colorbits = desktopBitsPixel;
	}

	//
	// implicitly assume Z-buffer depth == desktop color depth
	//
	int depthbits;
	if (r_depthbits->integer == 0)
	{
		if (colorbits > 16)
		{
			depthbits = 24;
		}
		else
		{
			depthbits = 16;
		}
	}
	else
	{
		depthbits = r_depthbits->integer;
	}

	//
	// do not allow stencil if Z-buffer depth likely won't contain it
	//
	int stencilbits = r_stencilbits->integer;
	if (depthbits < 24)
	{
		stencilbits = 0;
	}

	//
	// make two attempts to set the PIXELFORMAT
	//

	//
	// first attempt: r_colorbits, depthbits, and r_stencilbits
	//
	if (!pixelFormatSet)
	{
		GLW_CreatePFD(&pfd, colorbits, depthbits, stencilbits, !!r_stereo->integer);
		int tpfd = GLW_MakeContext(&pfd);
		if (tpfd != TRY_PFD_SUCCESS)
		{
			if (tpfd == TRY_PFD_FAIL_HARD)
			{
				if (maindc)
				{
					ReleaseDC(GMainWindow, maindc);
					maindc = NULL;
				}

				common->Printf(S_COLOR_YELLOW "...failed hard\n");
				return false;
			}

			//
			// punt if we've already tried the desktop bit depth and no stencil bits
			//
			if ((r_colorbits->integer == desktopBitsPixel) && (stencilbits == 0))
			{
				if (maindc)
				{
					ReleaseDC(GMainWindow, maindc);
					maindc = NULL;
				}

				common->Printf("...failed to find an appropriate PIXELFORMAT\n");

				return false;
			}

			//
			// second attempt: desktop's color bits and no stencil
			//
			if (colorbits > desktopBitsPixel)
			{
				colorbits = desktopBitsPixel;
			}
			GLW_CreatePFD(&pfd, colorbits, depthbits, 0, !!r_stereo->integer);
			if (GLW_MakeContext(&pfd) != TRY_PFD_SUCCESS)
			{
				if (maindc)
				{
					ReleaseDC(GMainWindow, maindc);
					maindc = NULL;
				}

				common->Printf("...failed to find an appropriate PIXELFORMAT\n");

				return false;
			}
		}

		//
		//	Report if stereo is desired but unavailable.
		//
		if (!(pfd.dwFlags & PFD_STEREO) && (r_stereo->integer != 0))
		{
			common->Printf("...failed to select stereo pixel format\n");
			glConfig.stereoEnabled = false;
		}
	}

	//
	//	Store PFD specifics.
	//
	glConfig.colorBits = (int)pfd.cColorBits;
	glConfig.depthBits = (int)pfd.cDepthBits;
	glConfig.stencilBits = (int)pfd.cStencilBits;

	return true;
}

//==========================================================================
//
//	WG_CheckHardwareGamma
//
//	Determines if the underlying hardware supports the Win32 gamma correction API.
//
//==========================================================================

static void WG_CheckHardwareGamma()
{
	glConfig.deviceSupportsGamma = false;

	if (r_ignorehwgamma->integer)
	{
		return;
	}

	HDC hDC = GetDC(GetDesktopWindow());
	glConfig.deviceSupportsGamma = GetDeviceGammaRamp(hDC, s_oldHardwareGamma);
	ReleaseDC(GetDesktopWindow(), hDC);

	if (!glConfig.deviceSupportsGamma)
	{
		return;
	}

	//
	// do a sanity check on the gamma values
	//
	if ((HIBYTE(s_oldHardwareGamma[0][255]) <= HIBYTE(s_oldHardwareGamma[0][0])) ||
		(HIBYTE(s_oldHardwareGamma[1][255]) <= HIBYTE(s_oldHardwareGamma[1][0])) ||
		(HIBYTE(s_oldHardwareGamma[2][255]) <= HIBYTE(s_oldHardwareGamma[2][0])))
	{
		glConfig.deviceSupportsGamma = false;
		common->Printf(S_COLOR_YELLOW "WARNING: device has broken gamma support\n");
	}

	//
	// make sure that we didn't have a prior crash in the game, and if so we need to
	// restore the gamma values to at least a linear value
	//
	if ((HIBYTE(s_oldHardwareGamma[0][181]) == 255))
	{
		common->Printf(S_COLOR_YELLOW "WARNING: suspicious gamma tables, using linear ramp for restoration\n");

		for (int g = 0; g < 255; g++)
		{
			s_oldHardwareGamma[0][g] = g << 8;
			s_oldHardwareGamma[1][g] = g << 8;
			s_oldHardwareGamma[2][g] = g << 8;
		}
	}
}

static void GLW_GenDefaultLists()
{
	HFONT hfont, oldhfont;

	// keep going, we'll probably just leak some stuff
	if (fontbase_init)
	{
		common->DPrintf("ERROR: GLW_GenDefaultLists: font base is already marked initialized\n");
	}

	// create font display lists
	gl_NormalFontBase = glGenLists(256);

	if (gl_NormalFontBase == 0)
	{
		common->Printf("ERROR: couldn't create font (glGenLists)\n");
		return;
	}

	hfont = CreateFont(
		12,	// logical height of font
		6,	// logical average character width
		0,	// angle of escapement
		0,	// base-line orientation angle
		0,	// font weight
		0,	// italic attribute flag
		0,	// underline attribute flag
		0,	// strikeout attribute flag
		0,	// character set identifier
		0,	// output precision
		0,	// clipping precision
		0,	// output quality
		0,	// pitch and family
		"");// pointer to typeface name string

	if (!hfont)
	{
		common->Printf("ERROR: couldn't create font (CreateFont)\n");
		return;
	}

	oldhfont = (HFONT)SelectObject(maindc, hfont);
	wglUseFontBitmaps(maindc, 0, 255, gl_NormalFontBase);

	SelectObject(maindc, oldhfont);
	DeleteObject(hfont);

	fontbase_init = true;
}

//==========================================================================
//
//	GLW_CreateWindow
//
//	Responsible for creating the Win32 window and initializing the OpenGL driver.
//
//==========================================================================

static bool GLW_CreateWindow(int width, int height, int colorbits, bool fullscreen)
{
	//
	// register the window class if necessary
	//
	if (!s_classRegistered)
	{
		vid_xpos = Cvar_Get("vid_xpos", "3", CVAR_ARCHIVE);
		vid_ypos = Cvar_Get("vid_ypos", "22", CVAR_ARCHIVE);

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
			common->FatalError("GLW_CreateWindow: could not register window class");
		}
		s_classRegistered = true;
		common->Printf("...registered window class\n");
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
			stylebits = WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_VISIBLE | WS_SYSMENU;	// | WS_MINIMIZEBOX
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

		GMainWindow = CreateWindowEx(exstyle, WINDOW_CLASS_NAME, R_GetTitleForWindow(),
			stylebits, x, y, w, h, NULL, NULL, global_hInstance, NULL);

		if (!GMainWindow)
		{
			common->FatalError("GLW_CreateWindow() - Couldn't create window");
		}

		ShowWindow(GMainWindow, SW_SHOW);
		UpdateWindow(GMainWindow);
		common->Printf("...created window@%d,%d (%dx%d)\n", x, y, w, h);
	}
	else
	{
		common->Printf("...window already present, CreateWindowEx skipped\n");
	}

	if (!GLW_InitDriver(colorbits))
	{
		ShowWindow(GMainWindow, SW_HIDE);
		DestroyWindow(GMainWindow);
		GMainWindow = NULL;

		return false;
	}

	SetForegroundWindow(GMainWindow);
	SetFocus(GMainWindow);

	WG_CheckHardwareGamma();

	// initialise default lists
	GLW_GenDefaultLists();

	return true;
}

//==========================================================================
//
//	PrintCDSError
//
//==========================================================================

static void PrintCDSError(int value)
{
	switch (value)
	{
	case DISP_CHANGE_RESTART:
		common->Printf("restart required\n");
		break;
	case DISP_CHANGE_BADPARAM:
		common->Printf("bad param\n");
		break;
	case DISP_CHANGE_BADFLAGS:
		common->Printf("bad flags\n");
		break;
	case DISP_CHANGE_FAILED:
		common->Printf("DISP_CHANGE_FAILED\n");
		break;
	case DISP_CHANGE_BADMODE:
		common->Printf("bad mode\n");
		break;
	case DISP_CHANGE_NOTUPDATED:
		common->Printf("not updated\n");
		break;
	default:
		common->Printf("unknown error %d\n", value);
		break;
	}
}

//==========================================================================
//
//	GLimp_SetMode
//
//==========================================================================

rserr_t GLimp_SetMode(int mode, int colorbits, bool fullscreen)
{
	const char* win_fs[] = { "W", "FS" };

	//
	// print out informational messages
	//
	common->Printf("...setting mode %d:", mode);
	if (!R_GetModeInfo(&glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode))
	{
		common->Printf(" invalid mode\n");
		return RSERR_INVALID_MODE;
	}
	common->Printf(" %d %d %s\n", glConfig.vidWidth, glConfig.vidHeight, win_fs[fullscreen]);

	//
	// check our desktop attributes
	//
	HDC hDC = GetDC(GetDesktopWindow());
	desktopBitsPixel = GetDeviceCaps(hDC, BITSPIXEL);
	desktopWidth = GetDeviceCaps(hDC, HORZRES);
	desktopHeight = GetDeviceCaps(hDC, VERTRES);
	ReleaseDC(GetDesktopWindow(), hDC);

	//
	// verify desktop bit depth
	//
	if (desktopBitsPixel < 15 || desktopBitsPixel == 24)
	{
		if (colorbits == 0 || (!cdsFullscreen && colorbits >= 15))
		{
			if (MessageBox(NULL,
					"It is highly unlikely that a correct\n"
					"windowed display can be initialized with\n"
					"the current desktop display depth.  Select\n"
					"'OK' to try anyway.  Press 'Cancel' if you otherwise\n"
					"wish to quit.",
					"Low Desktop Color Depth",
					MB_OKCANCEL | MB_ICONEXCLAMATION) != IDOK)
			{
				return RSERR_INVALID_MODE;
			}
		}
	}

	DEVMODE dm;

	// do a CDS if needed
	if (fullscreen)
	{
		Com_Memset(&dm, 0, sizeof(dm));

		dm.dmSize = sizeof(dm);

		dm.dmPelsWidth = glConfig.vidWidth;
		dm.dmPelsHeight = glConfig.vidHeight;
		dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

		if (r_displayRefresh->integer != 0)
		{
			dm.dmDisplayFrequency = r_displayRefresh->integer;
			dm.dmFields |= DM_DISPLAYFREQUENCY;
		}

		// try to change color depth if possible
		if (colorbits != 0)
		{
			dm.dmBitsPerPel = colorbits;
			dm.dmFields |= DM_BITSPERPEL;
			common->Printf("...using colorsbits of %d\n", colorbits);
		}
		else
		{
			common->Printf("...using desktop display depth of %d\n", desktopBitsPixel);
		}

		//
		//	If we're already in fullscreen then just create the window.
		//
		if (cdsFullscreen)
		{
			common->Printf("...already fullscreen, avoiding redundant CDS\n");

			if (!GLW_CreateWindow(glConfig.vidWidth, glConfig.vidHeight, colorbits, true))
			{
				common->Printf("...restoring display settings\n");
				ChangeDisplaySettings(0, 0);
				cdsFullscreen = false;
				return RSERR_INVALID_MODE;
			}
		}
		//
		// need to call CDS
		//
		else
		{
			common->Printf("...calling CDS: ");

			// try setting the exact mode requested, because some drivers don't report
			// the low res modes in EnumDisplaySettings, but still work
			int cdsRet = ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
			if (cdsRet == DISP_CHANGE_SUCCESSFUL)
			{
				common->Printf("ok\n");

				if (!GLW_CreateWindow(glConfig.vidWidth, glConfig.vidHeight, colorbits, true))
				{
					common->Printf("...restoring display settings\n");
					ChangeDisplaySettings(0, 0);
					return RSERR_INVALID_MODE;
				}

				cdsFullscreen = true;
			}
			else
			{
				common->Printf("failed, ");

				PrintCDSError(cdsRet);

				//
				// the exact mode failed, so scan EnumDisplaySettings for the next largest mode
				//
				common->Printf("...trying next higher resolution:");

				// we could do a better matching job here...
				DEVMODE devmode;
				int modeNum;
				for (modeNum = 0;; modeNum++)
				{
					if (!EnumDisplaySettings(NULL, modeNum, &devmode))
					{
						modeNum = -1;
						break;
					}
					if (devmode.dmPelsWidth >= glConfig.vidWidth &&
						devmode.dmPelsHeight >= glConfig.vidHeight &&
						devmode.dmBitsPerPel >= 15)
					{
						break;
					}
				}

				if (modeNum != -1)
				{
					cdsRet = ChangeDisplaySettings(&devmode, CDS_FULLSCREEN);
				}
				if (modeNum != -1 && cdsRet == DISP_CHANGE_SUCCESSFUL)
				{
					common->Printf(" ok\n");
					if (!GLW_CreateWindow(glConfig.vidWidth, glConfig.vidHeight, colorbits, true))
					{
						common->Printf("...restoring display settings\n");
						ChangeDisplaySettings(0, 0);
						return RSERR_INVALID_MODE;
					}

					cdsFullscreen = true;
				}
				else
				{
					common->Printf(" failed, ");

					PrintCDSError(cdsRet);

					common->Printf("...restoring display settings\n");
					ChangeDisplaySettings(0, 0);

					cdsFullscreen = false;
					glConfig.isFullscreen = false;
					if (!GLW_CreateWindow(glConfig.vidWidth, glConfig.vidHeight, colorbits, false))
					{
						return RSERR_INVALID_MODE;
					}
					return RSERR_INVALID_FULLSCREEN;
				}
			}
		}
	}
	else
	{
		if (cdsFullscreen)
		{
			ChangeDisplaySettings(0, 0);
		}

		cdsFullscreen = false;
		if (!GLW_CreateWindow(glConfig.vidWidth, glConfig.vidHeight, colorbits, false))
		{
			return RSERR_INVALID_MODE;
		}
	}

	//
	// success, now check display frequency
	//
	Com_Memset(&dm, 0, sizeof(dm));
	dm.dmSize = sizeof(dm);
	if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm))
	{
		glConfig.displayFrequency = dm.dmDisplayFrequency;
	}

	glConfig.isFullscreen = fullscreen;

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
//	GLimp_Shutdown()
//
//	This routine does all OS specific shutdown procedures for the OpenGL
// subsystem.
//
//==========================================================================

void GLimp_Shutdown()
{
	const char* success[] = { "failed", "success" };

	common->Printf("Shutting down OpenGL subsystem\n");

	// delete display lists
	GLW_DeleteDefaultLists();

	// restore gamma.
	if (glConfig.deviceSupportsGamma)
	{
		HDC hDC = GetDC(GetDesktopWindow());
		SetDeviceGammaRamp(hDC, s_oldHardwareGamma);
		ReleaseDC(GetDesktopWindow(), hDC);
		glConfig.deviceSupportsGamma = false;
	}

	// set current context to NULL
	int retVal = wglMakeCurrent(NULL, NULL) != 0;

	common->Printf("...wglMakeCurrent( NULL, NULL ): %s\n", success[retVal]);

	// delete HGLRC
	if (baseRC)
	{
		retVal = wglDeleteContext(baseRC) != 0;
		common->Printf("...deleting GL context: %s\n", success[retVal]);
		baseRC = NULL;
	}

	// release DC
	if (maindc)
	{
		retVal = ReleaseDC(GMainWindow, maindc) != 0;
		common->Printf("...releasing DC: %s\n", success[retVal]);
		maindc = NULL;
	}

	// destroy window
	if (GMainWindow)
	{
		common->Printf("...destroying window\n");
		ShowWindow(GMainWindow, SW_HIDE);
		DestroyWindow(GMainWindow);
		GMainWindow = NULL;
		pixelFormatSet = false;
	}

	// reset display settings
	if (cdsFullscreen)
	{
		common->Printf("...resetting display\n");
		ChangeDisplaySettings(0, 0);
		cdsFullscreen = false;
	}

	Com_Memset(&glConfig, 0, sizeof(glConfig));
	Com_Memset(&glState, 0, sizeof(glState));
}

const char* GLimp_GetSystemExtensionsString()
{
	return "";
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

//==========================================================================
//
//	GLimp_SetGamma
//
//	This routine should only be called if glConfig.deviceSupportsGamma is TRUE
//
//==========================================================================

void GLimp_SetGamma(unsigned char red[256], unsigned char green[256], unsigned char blue[256])
{
	if (!glConfig.deviceSupportsGamma || r_ignorehwgamma->integer || !maindc)
	{
		return;
	}

	unsigned short table[3][256];
	for (int i = 0; i < 256; i++)
	{
		table[0][i] = (((unsigned short)red[i]) << 8) | red[i];
		table[1][i] = (((unsigned short)green[i]) << 8) | green[i];
		table[2][i] = (((unsigned short)blue[i]) << 8) | blue[i];
	}

	// Windows puts this odd restriction on gamma ramps...
	for (int j = 0; j < 3; j++)
	{
		for (int i = 0; i < 128; i++)
		{
			if (table[j][i] > ((128 + i) << 8))
			{
				table[j][i] = (128 + i) << 8;
			}
		}
		if (table[j][127] > 254 << 8)
		{
			table[j][127] = 254 << 8;
		}
	}

	// enforce constantly increasing
	for (int j = 0; j < 3; j++)
	{
		for (int i = 1; i < 256; i++)
		{
			if (table[j][i] < table[j][i - 1])
			{
				table[j][i] = table[j][i - 1];
			}
		}
	}

	int ret = SetDeviceGammaRamp(maindc, table);
	if (!ret)
	{
		common->Printf("SetDeviceGammaRamp failed.\n");
	}
}

//==========================================================================
//
//	GLimp_SwapBuffers
//
//==========================================================================

void GLimp_SwapBuffers()
{
	SwapBuffers(maindc);
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

static DWORD WINAPI GLimp_RenderThreadWrapper(LPVOID)
{
	glimpRenderThread();

	// unbind the context before we die
	wglMakeCurrent(maindc, NULL);

	return 0;
}

//==========================================================================
//
//	GLimp_SpawnRenderThread
//
//==========================================================================

bool GLimp_SpawnRenderThread(void (* function)())
{
	renderCommandsEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	renderCompletedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	renderActiveEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	glimpRenderThread = function;

	renderThreadHandle = CreateThread(NULL, 0, GLimp_RenderThreadWrapper, 0, 0, &renderThreadId);

	if (!renderThreadHandle)
	{
		return false;
	}

	return true;
}

//==========================================================================
//
//	GLimp_RendererSleep
//
//==========================================================================

void* GLimp_RendererSleep()
{
	if (!wglMakeCurrent(maindc, NULL))
	{
		wglErrors++;
	}

	ResetEvent(renderActiveEvent);

	// after this, the front end can exit GLimp_FrontEndSleep
	SetEvent(renderCompletedEvent);

	WaitForSingleObject(renderCommandsEvent, INFINITE);

	if (!wglMakeCurrent(maindc, baseRC))
	{
		wglErrors++;
	}

	ResetEvent(renderCompletedEvent);
	ResetEvent(renderCommandsEvent);

	void* data = smpData;

	// after this, the main thread can exit GLimp_WakeRenderer
	SetEvent(renderActiveEvent);

	return data;
}

//==========================================================================
//
//	GLimp_FrontEndSleep
//
//==========================================================================

void GLimp_FrontEndSleep()
{
	WaitForSingleObject(renderCompletedEvent, INFINITE);

	if (!wglMakeCurrent(maindc, baseRC))
	{
		wglErrors++;
	}
}

//==========================================================================
//
//	GLimp_WakeRenderer
//
//==========================================================================

void GLimp_WakeRenderer(void* data)
{
	smpData = data;

	if (!wglMakeCurrent(maindc, NULL))
	{
		wglErrors++;
	}

	// after this, the renderer can continue through GLimp_RendererSleep
	SetEvent(renderCommandsEvent);

	WaitForSingleObject(renderActiveEvent, INFINITE);
}
