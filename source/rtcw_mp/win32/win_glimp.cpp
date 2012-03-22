/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/*
** WIN_GLIMP.C
**
** This file contains ALL Win32 specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Init
** GLimp_LogComment
** GLimp_Shutdown
**
** Note that the GLW_xxx functions are Windows specific GL-subsystem
** related functions that are relevant ONLY to win_glimp.c
*/
#include <assert.h>
#include "../renderer/tr_local.h"
#include "../qcommon/qcommon.h"
#include "resource.h"
#include "glw_win.h"
#include "win_local.h"

extern void WG_CheckHardwareGamma( void );
extern void WG_RestoreGamma( void );

typedef enum {
	RSERR_OK,

	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,

	RSERR_UNKNOWN
} rserr_t;

#define TRY_PFD_SUCCESS     0
#define TRY_PFD_FAIL_SOFT   1
#define TRY_PFD_FAIL_HARD   2

#define WINDOW_CLASS_NAME	"JLQuake"

static void     GLW_InitExtensions( void );
static rserr_t  GLW_SetMode( int mode,
							 int colorbits,
							 qboolean _cdsFullscreen );

extern bool s_classRegistered;

//
// function declaration
//
void     QGL_EnableLogging( qboolean enable );
qboolean QGL_Init();
void     QGL_Shutdown( void );

//
// variable declarations
//
glwstate_t glw_state;

extern HDC		maindc;
extern HGLRC	baseRC;
extern int		desktopBitsPixel;
extern int		desktopWidth;
extern int		desktopHeight;
extern bool		pixelFormatSet;
extern bool		cdsFullscreen;

/*
** GLW_StartDriverAndSetMode
*/
static qboolean GLW_StartDriverAndSetMode( int mode,
										   int colorbits,
										   qboolean _cdsFullscreen ) {
	rserr_t err;

	err = GLW_SetMode( r_mode->integer, colorbits, _cdsFullscreen );

	switch ( err )
	{
	case RSERR_INVALID_FULLSCREEN:
		ri.Printf( PRINT_ALL, "...WARNING: fullscreen unavailable in this mode\n" );
		return qfalse;
	case RSERR_INVALID_MODE:
		ri.Printf( PRINT_ALL, "...WARNING: could not set the given mode (%d)\n", mode );
		return qfalse;
	default:
		break;
	}
	return qtrue;
}

/*
** ChoosePFD
**
** Helper function that replaces ChoosePixelFormat.
*/
#define MAX_PFDS 256

static int GLW_ChoosePFD( HDC hDC, PIXELFORMATDESCRIPTOR *pPFD ) {
	PIXELFORMATDESCRIPTOR pfds[MAX_PFDS + 1];
	int maxPFD = 0;
	int i;
	int bestMatch = 0;

	ri.Printf( PRINT_ALL, "...GLW_ChoosePFD( %d, %d, %d )\n", ( int ) pPFD->cColorBits, ( int ) pPFD->cDepthBits, ( int ) pPFD->cStencilBits );

	// count number of PFDs
	maxPFD = DescribePixelFormat( hDC, 1, sizeof( PIXELFORMATDESCRIPTOR ), &pfds[0] );
	if ( maxPFD > MAX_PFDS ) {
		ri.Printf( PRINT_WARNING, "...numPFDs > MAX_PFDS (%d > %d)\n", maxPFD, MAX_PFDS );
		maxPFD = MAX_PFDS;
	}

	ri.Printf( PRINT_ALL, "...%d PFDs found\n", maxPFD - 1 );

	// grab information
	for ( i = 1; i <= maxPFD; i++ )
	{
		DescribePixelFormat( hDC, i, sizeof( PIXELFORMATDESCRIPTOR ), &pfds[i] );
	}

	// look for a best match
	for ( i = 1; i <= maxPFD; i++ )
	{
		//
		// make sure this has hardware acceleration
		//
		if ( ( pfds[i].dwFlags & PFD_GENERIC_FORMAT ) != 0 ) {
			if ( !r_allowSoftwareGL->integer ) {
				if ( r_verbose->integer ) {
					ri.Printf( PRINT_ALL, "...PFD %d rejected, software acceleration\n", i );
				}
				continue;
			}
		}

		// verify pixel type
		if ( pfds[i].iPixelType != PFD_TYPE_RGBA ) {
			if ( r_verbose->integer ) {
				ri.Printf( PRINT_ALL, "...PFD %d rejected, not RGBA\n", i );
			}
			continue;
		}

		// verify proper flags
		if ( ( ( pfds[i].dwFlags & pPFD->dwFlags ) & pPFD->dwFlags ) != pPFD->dwFlags ) {
			if ( r_verbose->integer ) {
				ri.Printf( PRINT_ALL, "...PFD %d rejected, improper flags (%x instead of %x)\n", i, pfds[i].dwFlags, pPFD->dwFlags );
			}
			continue;
		}

		// verify enough bits
		if ( pfds[i].cDepthBits < 15 ) {
			continue;
		}
		if ( ( pfds[i].cStencilBits < 4 ) && ( pPFD->cStencilBits > 0 ) ) {
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
		if ( bestMatch ) {
			// check stereo
			if ( ( pfds[i].dwFlags & PFD_STEREO ) && ( !( pfds[bestMatch].dwFlags & PFD_STEREO ) ) && ( pPFD->dwFlags & PFD_STEREO ) ) {
				bestMatch = i;
				continue;
			}

			if ( !( pfds[i].dwFlags & PFD_STEREO ) && ( pfds[bestMatch].dwFlags & PFD_STEREO ) && ( pPFD->dwFlags & PFD_STEREO ) ) {
				bestMatch = i;
				continue;
			}

			// check color
			if ( pfds[bestMatch].cColorBits != pPFD->cColorBits ) {
				// prefer perfect match
				if ( pfds[i].cColorBits == pPFD->cColorBits ) {
					bestMatch = i;
					continue;
				}
				// otherwise if this PFD has more bits than our best, use it
				else if ( pfds[i].cColorBits > pfds[bestMatch].cColorBits ) {
					bestMatch = i;
					continue;
				}
			}

			// check depth
			if ( pfds[bestMatch].cDepthBits != pPFD->cDepthBits ) {
				// prefer perfect match
				if ( pfds[i].cDepthBits == pPFD->cDepthBits ) {
					bestMatch = i;
					continue;
				}
				// otherwise if this PFD has more bits than our best, use it
				else if ( pfds[i].cDepthBits > pfds[bestMatch].cDepthBits ) {
					bestMatch = i;
					continue;
				}
			}

			// check stencil
			if ( pfds[bestMatch].cStencilBits != pPFD->cStencilBits ) {
				// prefer perfect match
				if ( pfds[i].cStencilBits == pPFD->cStencilBits ) {
					bestMatch = i;
					continue;
				}
				// otherwise if this PFD has more bits than our best, use it
				else if ( ( pfds[i].cStencilBits > pfds[bestMatch].cStencilBits ) &&
						  ( pPFD->cStencilBits > 0 ) ) {
					bestMatch = i;
					continue;
				}
			}
		} else
		{
			bestMatch = i;
		}
	}

	if ( !bestMatch ) {
		return 0;
	}

	if ( ( pfds[bestMatch].dwFlags & PFD_GENERIC_FORMAT ) != 0 ) {
		if ( !r_allowSoftwareGL->integer ) {
			ri.Printf( PRINT_ALL, "...no hardware acceleration found\n" );
			return 0;
		} else
		{
			ri.Printf( PRINT_ALL, "...using software emulation\n" );
		}
	} else if ( pfds[bestMatch].dwFlags & PFD_GENERIC_ACCELERATED )   {
		ri.Printf( PRINT_ALL, "...MCD acceleration found\n" );
	} else
	{
		ri.Printf( PRINT_ALL, "...hardware acceleration found\n" );
	}

	*pPFD = pfds[bestMatch];

	return bestMatch;
}

/*
** void GLW_CreatePFD
**
** Helper function zeros out then fills in a PFD
*/
static void GLW_CreatePFD( PIXELFORMATDESCRIPTOR *pPFD, int colorbits, int depthbits, int stencilbits, qboolean stereo ) {
	PIXELFORMATDESCRIPTOR src =
	{
		sizeof( PIXELFORMATDESCRIPTOR ),  // size of this pfd
		1,                              // version number
		PFD_DRAW_TO_WINDOW |            // support window
		PFD_SUPPORT_OPENGL |            // support OpenGL
		PFD_DOUBLEBUFFER,               // double buffered
		PFD_TYPE_RGBA,                  // RGBA type
		24,                             // 24-bit color depth
		0, 0, 0, 0, 0, 0,               // color bits ignored
		0,                              // no alpha buffer
		0,                              // shift bit ignored
		0,                              // no accumulation buffer
		0, 0, 0, 0,                     // accum bits ignored
		24,                             // 24-bit z-buffer
		8,                              // 8-bit stencil buffer
		0,                              // no auxiliary buffer
		PFD_MAIN_PLANE,                 // main layer
		0,                              // reserved
		0, 0, 0                         // layer masks ignored
	};

	src.cColorBits = colorbits;
	src.cDepthBits = depthbits;
	src.cStencilBits = stencilbits;

	if ( stereo ) {
		ri.Printf( PRINT_ALL, "...attempting to use stereo\n" );
		src.dwFlags |= PFD_STEREO;
		glConfig.stereoEnabled = qtrue;
	} else
	{
		glConfig.stereoEnabled = qfalse;
	}

	*pPFD = src;
}

/*
** GLW_MakeContext
*/
static int GLW_MakeContext( PIXELFORMATDESCRIPTOR *pPFD ) {
	int pixelformat;

	//
	// don't putz around with pixelformat if it's already set (e.g. this is a soft
	// reset of the graphics system)
	//
	if ( !pixelFormatSet ) {
		//
		// choose, set, and describe our desired pixel format.  If we're
		// using a minidriver then we need to bypass the GDI functions,
		// otherwise use the GDI functions.
		//
		if ( ( pixelformat = GLW_ChoosePFD( maindc, pPFD ) ) == 0 ) {
			ri.Printf( PRINT_ALL, "...GLW_ChoosePFD failed\n" );
			return TRY_PFD_FAIL_SOFT;
		}
		ri.Printf( PRINT_ALL, "...PIXELFORMAT %d selected\n", pixelformat );

		DescribePixelFormat( maindc, pixelformat, sizeof( *pPFD ), pPFD );

		if ( SetPixelFormat( maindc, pixelformat, pPFD ) == FALSE ) {
			ri.Printf( PRINT_ALL, "...SetPixelFormat failed\n", maindc );
			return TRY_PFD_FAIL_SOFT;
		}

		pixelFormatSet = qtrue;
	}

	//
	// startup the OpenGL subsystem by creating a context and making it current
	//
	if ( !baseRC ) {
		ri.Printf( PRINT_ALL, "...creating GL context: " );
		if ( ( baseRC = qwglCreateContext( maindc ) ) == 0 ) {
			ri.Printf( PRINT_ALL, "failed\n" );

			return TRY_PFD_FAIL_HARD;
		}
		ri.Printf( PRINT_ALL, "succeeded\n" );

		ri.Printf( PRINT_ALL, "...making context current: " );
		if ( !qwglMakeCurrent( maindc, baseRC ) ) {
			qwglDeleteContext( baseRC );
			baseRC = NULL;
			ri.Printf( PRINT_ALL, "failed\n" );
			return TRY_PFD_FAIL_HARD;
		}
		ri.Printf( PRINT_ALL, "succeeded\n" );
	}

	return TRY_PFD_SUCCESS;
}


/*
** GLW_InitDriver
**
** - get a DC if one doesn't exist
** - create an HGLRC if one doesn't exist
*/
static qboolean GLW_InitDriver( int colorbits ) {
	int tpfd;
	int depthbits, stencilbits;
	static PIXELFORMATDESCRIPTOR pfd;       // save between frames since 'tr' gets cleared

	ri.Printf( PRINT_ALL, "Initializing OpenGL driver\n" );

	//
	// get a DC for our window if we don't already have one allocated
	//
	if ( maindc == NULL ) {
		ri.Printf( PRINT_ALL, "...getting DC: " );

		if ( ( maindc = GetDC( GMainWindow ) ) == NULL ) {
			ri.Printf( PRINT_ALL, "failed\n" );
			return qfalse;
		}
		ri.Printf( PRINT_ALL, "succeeded\n" );
	}

	if ( colorbits == 0 ) {
		colorbits = desktopBitsPixel;
	}

	//
	// implicitly assume Z-buffer depth == desktop color depth
	//
	if ( r_depthbits->integer == 0 ) {
		if ( colorbits > 16 ) {
			depthbits = 24;
		} else {
			depthbits = 16;
		}
	} else {
		depthbits = r_depthbits->integer;
	}

	//
	// do not allow stencil if Z-buffer depth likely won't contain it
	//
	stencilbits = r_stencilbits->integer;
	if ( depthbits < 24 ) {
		stencilbits = 0;
	}

	//
	// make two attempts to set the PIXELFORMAT
	//

	//
	// first attempt: r_colorbits, depthbits, and r_stencilbits
	//
	if ( !pixelFormatSet ) {
		GLW_CreatePFD( &pfd, colorbits, depthbits, stencilbits, r_stereo->integer );
		if ( ( tpfd = GLW_MakeContext( &pfd ) ) != TRY_PFD_SUCCESS ) {
			if ( tpfd == TRY_PFD_FAIL_HARD ) {
				ri.Printf( PRINT_WARNING, "...failed hard\n" );
				return qfalse;
			}

			//
			// punt if we've already tried the desktop bit depth and no stencil bits
			//
			if ( ( r_colorbits->integer == desktopBitsPixel ) &&
				 ( stencilbits == 0 ) ) {
				ReleaseDC( GMainWindow, maindc );
				maindc = NULL;

				ri.Printf( PRINT_ALL, "...failed to find an appropriate PIXELFORMAT\n" );

				return qfalse;
			}

			//
			// second attempt: desktop's color bits and no stencil
			//
			if ( colorbits > desktopBitsPixel ) {
				colorbits = desktopBitsPixel;
			}
			GLW_CreatePFD( &pfd, colorbits, depthbits, 0, r_stereo->integer );
			if ( GLW_MakeContext( &pfd ) != TRY_PFD_SUCCESS ) {
				if ( maindc ) {
					ReleaseDC( GMainWindow, maindc );
					maindc = NULL;
				}

				ri.Printf( PRINT_ALL, "...failed to find an appropriate PIXELFORMAT\n" );

				return qfalse;
			}
		}

		/*
		** report if stereo is desired but unavailable
		*/
		if ( !( pfd.dwFlags & PFD_STEREO ) && ( r_stereo->integer != 0 ) ) {
			ri.Printf( PRINT_ALL, "...failed to select stereo pixel format\n" );
			glConfig.stereoEnabled = qfalse;
		}
	}

	/*
	** store PFD specifics
	*/
	glConfig.colorBits = ( int ) pfd.cColorBits;
	glConfig.depthBits = ( int ) pfd.cDepthBits;
	glConfig.stencilBits = ( int ) pfd.cStencilBits;

	return qtrue;
}

/*
** GLW_CreateWindow
**
** Responsible for creating the Win32 window and initializing the OpenGL driver.
*/
#define WINDOW_STYLE    ( WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_VISIBLE )
static qboolean GLW_CreateWindow( int width, int height, int colorbits, qboolean _cdsFullscreen ) {
	RECT r;
	Cvar          *vid_xpos, *vid_ypos;
	int stylebits;
	int x, y, w, h;
	int exstyle;

	//
	// register the window class if necessary
	//
	if ( !s_classRegistered ) {
		WNDCLASS wc;

		memset( &wc, 0, sizeof( wc ) );

		wc.style         = 0;
		wc.lpfnWndProc   = (WNDPROC) glw_state.wndproc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = global_hInstance;
		wc.hIcon         = LoadIcon( global_hInstance, MAKEINTRESOURCE( IDI_ICON1 ) );
		wc.hCursor       = LoadCursor( NULL,IDC_ARROW );
		wc.hbrBackground = (HBRUSH)COLOR_GRAYTEXT;
		wc.lpszMenuName  = 0;
		wc.lpszClassName = WINDOW_CLASS_NAME;

		if ( !RegisterClass( &wc ) ) {
			ri.Error( ERR_FATAL, "GLW_CreateWindow: could not register window class" );
		}
		s_classRegistered = qtrue;
		ri.Printf( PRINT_ALL, "...registered window class\n" );
	}

	//
	// create the HWND if one does not already exist
	//
	if ( !GMainWindow ) {
		//
		// compute width and height
		//
		r.left = 0;
		r.top = 0;
		r.right  = width;
		r.bottom = height;

		if ( _cdsFullscreen ) {
			exstyle = WS_EX_TOPMOST;
			stylebits = WS_POPUP | WS_VISIBLE | WS_SYSMENU;
		} else
		{
			exstyle = 0;
			stylebits = WINDOW_STYLE | WS_SYSMENU;
			AdjustWindowRect( &r, stylebits, FALSE );
		}

		w = r.right - r.left;
		h = r.bottom - r.top;

		if ( _cdsFullscreen ) {
			x = 0;
			y = 0;
		} else
		{
			vid_xpos = ri.Cvar_Get( "vid_xpos", "", 0 );
			vid_ypos = ri.Cvar_Get( "vid_ypos", "", 0 );
			x = vid_xpos->integer;
			y = vid_ypos->integer;

			// adjust window coordinates if necessary
			// so that the window is completely on screen
			if ( x < 0 ) {
				x = 0;
			}
			if ( y < 0 ) {
				y = 0;
			}

			if ( w < desktopWidth &&
				 h < desktopHeight ) {
				if ( x + w > desktopWidth ) {
					x = ( desktopWidth - w );
				}
				if ( y + h > desktopHeight ) {
					y = ( desktopHeight - h );
				}
			}
		}

		GMainWindow = CreateWindowEx(
			exstyle,
			WINDOW_CLASS_NAME,
			"Wolfenstein",
			stylebits,
			x, y, w, h,
			NULL,
			NULL,
			global_hInstance,
			NULL );

		if ( !GMainWindow ) {
			ri.Error( ERR_FATAL, "GLW_CreateWindow() - Couldn't create window" );
		}

		ShowWindow( GMainWindow, SW_SHOW );
		UpdateWindow( GMainWindow );
		ri.Printf( PRINT_ALL, "...created window@%d,%d (%dx%d)\n", x, y, w, h );
	} else
	{
		ri.Printf( PRINT_ALL, "...window already present, CreateWindowEx skipped\n" );
	}

	if ( !GLW_InitDriver( colorbits ) ) {
		ShowWindow( GMainWindow, SW_HIDE );
		DestroyWindow( GMainWindow );
		GMainWindow = NULL;

		return qfalse;
	}

	SetForegroundWindow( GMainWindow );
	SetFocus( GMainWindow );

	return qtrue;
}

static void PrintCDSError( int value ) {
	switch ( value )
	{
	case DISP_CHANGE_RESTART:
		ri.Printf( PRINT_ALL, "restart required\n" );
		break;
	case DISP_CHANGE_BADPARAM:
		ri.Printf( PRINT_ALL, "bad param\n" );
		break;
	case DISP_CHANGE_BADFLAGS:
		ri.Printf( PRINT_ALL, "bad flags\n" );
		break;
	case DISP_CHANGE_FAILED:
		ri.Printf( PRINT_ALL, "DISP_CHANGE_FAILED\n" );
		break;
	case DISP_CHANGE_BADMODE:
		ri.Printf( PRINT_ALL, "bad mode\n" );
		break;
	case DISP_CHANGE_NOTUPDATED:
		ri.Printf( PRINT_ALL, "not updated\n" );
		break;
	default:
		ri.Printf( PRINT_ALL, "unknown error %d\n", value );
		break;
	}
}

/*
** GLW_SetMode
*/
static rserr_t GLW_SetMode( int mode,
							int colorbits,
							qboolean _cdsFullscreen ) {
	HDC hDC;
	const char *win_fs[] = { "W", "FS" };
	int cdsRet;
	DEVMODE dm;

	//
	// print out informational messages
	//
	ri.Printf( PRINT_ALL, "...setting mode %d:", mode );
	if ( !R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode ) ) {
		ri.Printf( PRINT_ALL, " invalid mode\n" );
		return RSERR_INVALID_MODE;
	}
	ri.Printf( PRINT_ALL, " %d %d %s\n", glConfig.vidWidth, glConfig.vidHeight, win_fs[_cdsFullscreen] );

	//
	// check our desktop attributes
	//
	hDC = GetDC( GetDesktopWindow() );
	desktopBitsPixel = GetDeviceCaps( hDC, BITSPIXEL );
	desktopWidth = GetDeviceCaps( hDC, HORZRES );
	desktopHeight = GetDeviceCaps( hDC, VERTRES );
	ReleaseDC( GetDesktopWindow(), hDC );

	//
	// verify desktop bit depth
	//
	if ( desktopBitsPixel < 15 || desktopBitsPixel == 24 ) {
		if ( colorbits == 0 || ( !_cdsFullscreen && colorbits >= 15 ) ) {
			if ( MessageBox( NULL,
								"It is highly unlikely that a correct\n"
								"windowed display can be initialized with\n"
								"the current desktop display depth.  Select\n"
								"'OK' to try anyway.  Press 'Cancel' if you\n"
								"have a 3Dfx Voodoo, Voodoo-2, or Voodoo Rush\n"
								"3D accelerator installed, or if you otherwise\n"
								"wish to quit.",
								"Low Desktop Color Depth",
								MB_OKCANCEL | MB_ICONEXCLAMATION ) != IDOK ) {
				return RSERR_INVALID_MODE;
			}
		}
	}

	// do a CDS if needed
	if ( _cdsFullscreen ) {
		memset( &dm, 0, sizeof( dm ) );

		dm.dmSize = sizeof( dm );

		dm.dmPelsWidth  = glConfig.vidWidth;
		dm.dmPelsHeight = glConfig.vidHeight;
		dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;

		if ( r_displayRefresh->integer != 0 ) {
			dm.dmDisplayFrequency = r_displayRefresh->integer;
			dm.dmFields |= DM_DISPLAYFREQUENCY;
		}

		// try to change color depth if possible
		if ( colorbits != 0 ) {
			if ( glw_state.allowdisplaydepthchange ) {
				dm.dmBitsPerPel = colorbits;
				dm.dmFields |= DM_BITSPERPEL;
				ri.Printf( PRINT_ALL, "...using colorsbits of %d\n", colorbits );
			} else
			{
				ri.Printf( PRINT_ALL, "WARNING:...changing depth not supported on Win95 < pre-OSR 2.x\n" );
			}
		} else
		{
			ri.Printf( PRINT_ALL, "...using desktop display depth of %d\n", desktopBitsPixel );
		}

		//
		// if we're already in fullscreen then just create the window
		//
		if ( cdsFullscreen ) {
			ri.Printf( PRINT_ALL, "...already fullscreen, avoiding redundant CDS\n" );

			if ( !GLW_CreateWindow( glConfig.vidWidth, glConfig.vidHeight, colorbits, qtrue ) ) {
				ri.Printf( PRINT_ALL, "...restoring display settings\n" );
				ChangeDisplaySettings( 0, 0 );
				return RSERR_INVALID_MODE;
			}
		}
		//
		// need to call CDS
		//
		else
		{
			ri.Printf( PRINT_ALL, "...calling CDS: " );

			// try setting the exact mode requested, because some drivers don't report
			// the low res modes in EnumDisplaySettings, but still work
			if ( ( cdsRet = ChangeDisplaySettings( &dm, CDS_FULLSCREEN ) ) == DISP_CHANGE_SUCCESSFUL ) {
				ri.Printf( PRINT_ALL, "ok\n" );

				if ( !GLW_CreateWindow( glConfig.vidWidth, glConfig.vidHeight, colorbits, qtrue ) ) {
					ri.Printf( PRINT_ALL, "...restoring display settings\n" );
					ChangeDisplaySettings( 0, 0 );
					return RSERR_INVALID_MODE;
				}

				cdsFullscreen = qtrue;
			} else
			{
				//
				// the exact mode failed, so scan EnumDisplaySettings for the next largest mode
				//
				DEVMODE devmode;
				int modeNum;

				ri.Printf( PRINT_ALL, "failed, " );

				PrintCDSError( cdsRet );

				ri.Printf( PRINT_ALL, "...trying next higher resolution:" );

				// we could do a better matching job here...
				for ( modeNum = 0 ; ; modeNum++ ) {
					if ( !EnumDisplaySettings( NULL, modeNum, &devmode ) ) {
						modeNum = -1;
						break;
					}
					if ( devmode.dmPelsWidth >= glConfig.vidWidth
						 && devmode.dmPelsHeight >= glConfig.vidHeight
						 && devmode.dmBitsPerPel >= 15 ) {
						break;
					}
				}

				if ( modeNum != -1 && ( cdsRet = ChangeDisplaySettings( &devmode, CDS_FULLSCREEN ) ) == DISP_CHANGE_SUCCESSFUL ) {
					ri.Printf( PRINT_ALL, " ok\n" );
					if ( !GLW_CreateWindow( glConfig.vidWidth, glConfig.vidHeight, colorbits, qtrue ) ) {
						ri.Printf( PRINT_ALL, "...restoring display settings\n" );
						ChangeDisplaySettings( 0, 0 );
						return RSERR_INVALID_MODE;
					}

					cdsFullscreen = qtrue;
				} else
				{
					ri.Printf( PRINT_ALL, " failed, " );

					PrintCDSError( cdsRet );

					ri.Printf( PRINT_ALL, "...restoring display settings\n" );
					ChangeDisplaySettings( 0, 0 );

					cdsFullscreen = qfalse;
					glConfig.isFullscreen = qfalse;
					if ( !GLW_CreateWindow( glConfig.vidWidth, glConfig.vidHeight, colorbits, qfalse ) ) {
						return RSERR_INVALID_MODE;
					}
					return RSERR_INVALID_FULLSCREEN;
				}
			}
		}
	} else
	{
		if ( cdsFullscreen ) {
			ChangeDisplaySettings( 0, 0 );
		}

		cdsFullscreen = qfalse;
		if ( !GLW_CreateWindow( glConfig.vidWidth, glConfig.vidHeight, colorbits, qfalse ) ) {
			return RSERR_INVALID_MODE;
		}
	}

	//
	// success, now check display frequency, although this won't be valid on Voodoo(2)
	//
	memset( &dm, 0, sizeof( dm ) );
	dm.dmSize = sizeof( dm );
	if ( EnumDisplaySettings( NULL, ENUM_CURRENT_SETTINGS, &dm ) ) {
		glConfig.displayFrequency = dm.dmDisplayFrequency;
	}

	// NOTE: this is overridden later on standalone 3Dfx drivers
	glConfig.isFullscreen = _cdsFullscreen;

	return RSERR_OK;
}

/*
** GLW_InitExtensions
*/
static void GLW_InitExtensions( void ) {
	if ( !r_allowExtensions->integer ) {
		ri.Printf( PRINT_ALL, "*** IGNORING OPENGL EXTENSIONS ***\n" );
		return;
	}

	ri.Printf( PRINT_ALL, "Initializing OpenGL extensions\n" );

	// GL_S3_s3tc
	glConfig.textureCompression = TC_NONE;
	// RF, check for GL_EXT_texture_compression_s3tc
	if ( strstr( glConfig.extensions_string, "GL_EXT_texture_compression_s3tc" ) ) {
		if ( r_ext_compressed_textures->integer ) {
			glConfig.textureCompression = TC_EXT_COMP_S3TC;
			ri.Printf( PRINT_ALL, "...using GL_EXT_texture_compression_s3tc\n" );
		} else
		{
			glConfig.textureCompression = TC_NONE;
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_compression_s3tc\n" );
		}
	}
	/* RF, disabled this section, since this method produces very ugly results on nvidia hardware
	else if ( strstr( glConfig.extensions_string, "GL_S3_s3tc" ) )
	{
		if ( r_ext_compressed_textures->integer )
		{
			glConfig.textureCompression = TC_S3TC;
			ri.Printf( PRINT_ALL, "...using GL_S3_s3tc\n" );
		}
		else
		{
			glConfig.textureCompression = TC_NONE;
			ri.Printf( PRINT_ALL, "...ignoring GL_S3_s3tc\n" );
		}
	}
	*/
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_compression_s3tc not found\n" );
	}

	// GL_EXT_texture_env_add
	glConfig.textureEnvAddAvailable = qfalse;
	if ( strstr( glConfig.extensions_string, "EXT_texture_env_add" ) ) {
		if ( r_ext_texture_env_add->integer ) {
			glConfig.textureEnvAddAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_EXT_texture_env_add\n" );
		} else
		{
			glConfig.textureEnvAddAvailable = qfalse;
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_env_add\n" );
		}
	} else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_env_add not found\n" );
	}

	// WGL_EXT_swap_control
	qwglSwapIntervalEXT = ( BOOL ( WINAPI * )(int) )qwglGetProcAddress( "wglSwapIntervalEXT" );
	if ( qwglSwapIntervalEXT ) {
		ri.Printf( PRINT_ALL, "...using WGL_EXT_swap_control\n" );
		r_swapInterval->modified = qtrue;   // force a set next frame
	} else
	{
		ri.Printf( PRINT_ALL, "...WGL_EXT_swap_control not found\n" );
	}

	// GL_ARB_multitexture
	qglMultiTexCoord2fARB = NULL;
	qglActiveTextureARB = NULL;
	qglClientActiveTextureARB = NULL;
	if ( strstr( glConfig.extensions_string, "GL_ARB_multitexture" )  ) {
		if ( r_ext_multitexture->integer ) {
			qglMultiTexCoord2fARB = ( PFNGLMULTITEXCOORD2FARBPROC ) qwglGetProcAddress( "glMultiTexCoord2fARB" );
			qglActiveTextureARB = ( PFNGLACTIVETEXTUREARBPROC ) qwglGetProcAddress( "glActiveTextureARB" );
			qglClientActiveTextureARB = ( PFNGLCLIENTACTIVETEXTUREARBPROC ) qwglGetProcAddress( "glClientActiveTextureARB" );

			if ( qglActiveTextureARB ) {
				qglGetIntegerv( GL_MAX_ACTIVE_TEXTURES_ARB, &glConfig.maxActiveTextures );

				if ( glConfig.maxActiveTextures > 1 ) {
					ri.Printf( PRINT_ALL, "...using GL_ARB_multitexture\n" );
				} else
				{
					qglMultiTexCoord2fARB = NULL;
					qglActiveTextureARB = NULL;
					qglClientActiveTextureARB = NULL;
					ri.Printf( PRINT_ALL, "...not using GL_ARB_multitexture, < 2 texture units\n" );
				}
			}
		} else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_ARB_multitexture\n" );
		}
	} else
	{
		ri.Printf( PRINT_ALL, "...GL_ARB_multitexture not found\n" );
	}

	// GL_EXT_compiled_vertex_array
	qglLockArraysEXT = NULL;
	qglUnlockArraysEXT = NULL;
	if ( strstr( glConfig.extensions_string, "GL_EXT_compiled_vertex_array" ) ) {
		if ( r_ext_compiled_vertex_array->integer ) {
			ri.Printf( PRINT_ALL, "...using GL_EXT_compiled_vertex_array\n" );
			qglLockArraysEXT = ( void ( APIENTRY * )( int, int ) )qwglGetProcAddress( "glLockArraysEXT" );
			qglUnlockArraysEXT = ( void ( APIENTRY * )( void ) )qwglGetProcAddress( "glUnlockArraysEXT" );
			if ( !qglLockArraysEXT || !qglUnlockArraysEXT ) {
				ri.Error( ERR_FATAL, "bad getprocaddress" );
			}
		} else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_compiled_vertex_array\n" );
		}
	} else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_compiled_vertex_array not found\n" );
	}

	// WGL_3DFX_gamma_control
	qwglGetDeviceGammaRamp3DFX = NULL;
	qwglSetDeviceGammaRamp3DFX = NULL;

	if ( strstr( glConfig.extensions_string, "WGL_3DFX_gamma_control" ) ) {
		if ( !r_ignorehwgamma->integer && r_ext_gamma_control->integer ) {
			qwglGetDeviceGammaRamp3DFX = ( BOOL ( WINAPI * )( HDC, LPVOID ) )qwglGetProcAddress( "wglGetDeviceGammaRamp3DFX" );
			qwglSetDeviceGammaRamp3DFX = ( BOOL ( WINAPI * )( HDC, LPVOID ) )qwglGetProcAddress( "wglSetDeviceGammaRamp3DFX" );

			if ( qwglGetDeviceGammaRamp3DFX && qwglSetDeviceGammaRamp3DFX ) {
				ri.Printf( PRINT_ALL, "...using WGL_3DFX_gamma_control\n" );
			} else
			{
				qwglGetDeviceGammaRamp3DFX = NULL;
				qwglSetDeviceGammaRamp3DFX = NULL;
			}
		} else
		{
			ri.Printf( PRINT_ALL, "...ignoring WGL_3DFX_gamma_control\n" );
		}
	} else
	{
		ri.Printf( PRINT_ALL, "...WGL_3DFX_gamma_control not found\n" );
	}

	// GL_NV_fog_distance
	if ( strstr( glConfig.extensions_string, "GL_NV_fog_distance" ) ) {
		if ( r_ext_NV_fog_dist->integer ) {
			glConfig.NVFogAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_NV_fog_distance\n" );
		} else {
			ri.Printf( PRINT_ALL, "...ignoring GL_NV_fog_distance\n" );
			ri.Cvar_Set( "r_ext_NV_fog_dist", "0" );
		}
	} else {
		ri.Printf( PRINT_ALL, "...GL_NV_fog_distance not found\n" );
		ri.Cvar_Set( "r_ext_NV_fog_dist", "0" );
	}
}

/*
** GLW_CheckOSVersion
*/
static qboolean GLW_CheckOSVersion( void ) {
#define OSR2_BUILD_NUMBER 1111

	OSVERSIONINFO vinfo;

	vinfo.dwOSVersionInfoSize = sizeof( vinfo );

	glw_state.allowdisplaydepthchange = qfalse;

	if ( GetVersionEx( &vinfo ) ) {
		if ( vinfo.dwMajorVersion > 4 ) {
			glw_state.allowdisplaydepthchange = qtrue;
		} else if ( vinfo.dwMajorVersion == 4 )   {
			if ( vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT ) {
				glw_state.allowdisplaydepthchange = qtrue;
			} else if ( vinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )   {
				if ( LOWORD( vinfo.dwBuildNumber ) >= OSR2_BUILD_NUMBER ) {
					glw_state.allowdisplaydepthchange = qtrue;
				}
			}
		}
	} else
	{
		ri.Printf( PRINT_ALL, "GLW_CheckOSVersion() - GetVersionEx failed\n" );
		return qfalse;
	}

	return qtrue;
}

/*
** GLW_LoadOpenGL
**
** GLimp_win.c internal function that attempts to load and use
** a specific OpenGL DLL.
*/
static qboolean GLW_LoadOpenGL() {
	qboolean _cdsFullscreen;

	//
	// determine if we're on a standalone driver
	//
	glConfig.driverType = GLDRV_ICD;

	//
	// load the driver and bind our function pointers to it
	//
	if ( QGL_Init() ) {
		{
			_cdsFullscreen = r_fullscreen->integer;
		}

		// create the window and set up the context
		if ( !GLW_StartDriverAndSetMode( r_mode->integer, r_colorbits->integer, _cdsFullscreen ) ) {
			// if we're on a 24/32-bit desktop and we're going fullscreen on an ICD,
			// try it again but with a 16-bit desktop
			if ( r_colorbits->integer != 16 ||
					_cdsFullscreen != qtrue ||
					r_mode->integer != 3 ) {
				if ( !GLW_StartDriverAndSetMode( 3, 16, qtrue ) ) {
					goto fail;
				}
			}
		}

		return qtrue;
	}
fail:

	QGL_Shutdown();

	return qfalse;
}

/*
** GLimp_EndFrame
*/
void GLimp_EndFrame( void ) {
	//
	// swapinterval stuff
	//
	if ( r_swapInterval->modified ) {
		r_swapInterval->modified = qfalse;

		if ( !glConfig.stereoEnabled ) {    // why?
			if ( qwglSwapIntervalEXT ) {
				qwglSwapIntervalEXT( r_swapInterval->integer );
			}
		}
	}


	// don't flip if drawing to front buffer
	if ( String::ICmp( r_drawBuffer->string, "GL_FRONT" ) != 0 ) {
		SwapBuffers( maindc );
	}

	// check logging
	QGL_EnableLogging( r_logFile->integer );
}


static void GLW_StartOpenGL( void ) {
	//
	// load and initialize the specific OpenGL driver
	//
	if ( !GLW_LoadOpenGL() ) {
		ri.Error( ERR_FATAL, "GLW_StartOpenGL() - could not load OpenGL subsystem\n" );
	}
}

/*
** GLimp_Init
**
** This is the platform specific OpenGL initialization function.  It
** is responsible for loading OpenGL, initializing it, setting
** extensions, creating a window of the appropriate size, doing
** fullscreen manipulations, etc.  Its overall responsibility is
** to make sure that a functional OpenGL subsystem is operating
** when it returns to the ref.
*/
void GLimp_Init( void ) {
	char buf[1024];
	Cvar *lastValidRenderer = ri.Cvar_Get( "r_lastValidRenderer", "(uninitialized)", CVAR_ARCHIVE );
	Cvar  *cv;

	ri.Printf( PRINT_ALL, "Initializing OpenGL subsystem\n" );

	//
	// check OS version to see if we can do fullscreen display changes
	//
	if ( !GLW_CheckOSVersion() ) {
		ri.Error( ERR_FATAL, "GLimp_Init() - incorrect operating system\n" );
	}

	// save off hInstance and wndproc
	glw_state.wndproc = MainWndProc;

	// load appropriate DLL and initialize subsystem
	GLW_StartOpenGL();

	// get our config strings
	String::NCpyZ( glConfig.vendor_string, (char*)qglGetString( GL_VENDOR ), sizeof( glConfig.vendor_string ) );
	String::NCpyZ( glConfig.renderer_string, (char*)qglGetString( GL_RENDERER ), sizeof( glConfig.renderer_string ) );
	String::NCpyZ( glConfig.version_string, (char*)qglGetString( GL_VERSION ), sizeof( glConfig.version_string ) );
	String::NCpyZ( glConfig.extensions_string, (char*)qglGetString( GL_EXTENSIONS ), sizeof( glConfig.extensions_string ) );
	// TTimo - safe check
	if ( String::Length( (char*)qglGetString( GL_EXTENSIONS ) ) >= sizeof( glConfig.extensions_string ) ) {
		Com_Printf( S_COLOR_YELLOW "WARNNING: GL extensions string too long (%d), truncated to %d\n", String::Length( (char*)qglGetString( GL_EXTENSIONS ) ), sizeof( glConfig.extensions_string ) );
	}

	//
	// chipset specific configuration
	//
	String::NCpyZ( buf, glConfig.renderer_string, sizeof( buf ) );
	String::ToLower( buf );

	//
	// NOTE: if changing cvars, do it within this block.  This allows them
	// to be overridden when testing driver fixes, etc. but only sets
	// them to their default state when the hardware is first installed/run.
	//
	if ( String::ICmp( lastValidRenderer->string, glConfig.renderer_string ) ) {
		glConfig.hardwareType = GLHW_GENERIC;

		ri.Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST" );

		// VOODOO GRAPHICS w/ 2MB
		if ( strstr( buf, "voodoo graphics/1 tmu/2 mb" ) ) {
			ri.Cvar_Set( "r_picmip", "2" );
			ri.Cvar_Get( "r_picmip", "1", CVAR_ARCHIVE | CVAR_LATCH2 );
		} else
		{

//----(SA)	FIXME: RETURN TO DEFAULT  Another id build change for DK/DM
			ri.Cvar_Set( "r_picmip", "1" );   //----(SA)	was "1" // JPW NERVE back to 1
//----(SA)

			if ( strstr( buf, "rage 128" ) || strstr( buf, "rage128" ) ) {
				ri.Cvar_Set( "r_finish", "0" );
			}
			// Savage3D and Savage4 should always have trilinear enabled
			else if ( strstr( buf, "savage3d" ) || strstr( buf, "s3 savage4" ) ) {
				ri.Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
			}
		}
	}

	ri.Cvar_Set( "r_lastValidRenderer", glConfig.renderer_string );

	GLW_InitExtensions();
	WG_CheckHardwareGamma();
}

/*
** GLimp_Shutdown
**
** This routine does all OS specific shutdown procedures for the OpenGL
** subsystem.
*/
void GLimp_Shutdown( void ) {
//	const char *strings[] = { "soft", "hard" };
	const char *success[] = { "failed", "success" };
	int retVal;

	// FIXME: Brian, we need better fallbacks from partially initialized failures
	if ( !qwglMakeCurrent ) {
		return;
	}

	ri.Printf( PRINT_ALL, "Shutting down OpenGL subsystem\n" );

	// restore gamma.  We do this first because 3Dfx's extension needs a valid OGL subsystem
	WG_RestoreGamma();

	// set current context to NULL
	if ( qwglMakeCurrent ) {
		retVal = qwglMakeCurrent( NULL, NULL ) != 0;

		ri.Printf( PRINT_ALL, "...wglMakeCurrent( NULL, NULL ): %s\n", success[retVal] );
	}

	// delete HGLRC
	if ( baseRC ) {
		retVal = qwglDeleteContext( baseRC ) != 0;
		ri.Printf( PRINT_ALL, "...deleting GL context: %s\n", success[retVal] );
		baseRC = NULL;
	}

	// release DC
	if ( maindc ) {
		retVal = ReleaseDC( GMainWindow, maindc ) != 0;
		ri.Printf( PRINT_ALL, "...releasing DC: %s\n", success[retVal] );
		maindc   = NULL;
	}

	// destroy window
	if ( GMainWindow ) {
		ri.Printf( PRINT_ALL, "...destroying window\n" );
		ShowWindow( GMainWindow, SW_HIDE );
		DestroyWindow( GMainWindow );
		GMainWindow = NULL;
		pixelFormatSet = qfalse;
	}

	// close the r_logFile
	if ( glw_state.log_fp ) {
		fclose( glw_state.log_fp );
		glw_state.log_fp = 0;
	}

	// reset display settings
	if ( cdsFullscreen ) {
		ri.Printf( PRINT_ALL, "...resetting display\n" );
		ChangeDisplaySettings( 0, 0 );
		cdsFullscreen = qfalse;
	}

	// shutdown QGL subsystem
	QGL_Shutdown();

	memset( &glConfig, 0, sizeof( glConfig ) );
	memset( &glState, 0, sizeof( glState ) );
}

/*
** GLimp_LogComment
*/
void GLimp_LogComment( char *comment ) {
	if ( glw_state.log_fp ) {
		fprintf( glw_state.log_fp, "%s", comment );
	}
}


/*
===========================================================

SMP acceleration

===========================================================
*/

extern HANDLE renderCommandsEvent;
extern HANDLE renderCompletedEvent;
extern HANDLE renderActiveEvent;

extern void ( *glimpRenderThread )( void );

void GLimp_RenderThreadWrapper( void ) {
	glimpRenderThread();

	// unbind the context before we die
	qwglMakeCurrent( maindc, NULL );
}

/*
=======================
GLimp_SpawnRenderThread
=======================
*/
extern HANDLE renderThreadHandle;
extern DWORD renderThreadId;
qboolean GLimp_SpawnRenderThread( void ( *function )( void ) ) {

	renderCommandsEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	renderCompletedEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	renderActiveEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

	glimpRenderThread = function;

	renderThreadHandle = CreateThread(
		NULL,   // LPSECURITY_ATTRIBUTES lpsa,
		0,      // DWORD cbStack,
		(LPTHREAD_START_ROUTINE)GLimp_RenderThreadWrapper,  // LPTHREAD_START_ROUTINE lpStartAddr,
		0,          // LPVOID lpvThreadParm,
		0,          //   DWORD fdwCreate,
		&renderThreadId );

	if ( !renderThreadHandle ) {
		return qfalse;
	}

	return qtrue;
}

extern void    *smpData;
extern int wglErrors;

void *GLimp_RendererSleep( void ) {
	void    *data;

	if ( !qwglMakeCurrent( maindc, NULL ) ) {
		wglErrors++;
	}

	ResetEvent( renderActiveEvent );

	// after this, the front end can exit GLimp_FrontEndSleep
	SetEvent( renderCompletedEvent );

	WaitForSingleObject( renderCommandsEvent, INFINITE );

	if ( !qwglMakeCurrent( maindc, baseRC ) ) {
		wglErrors++;
	}

	ResetEvent( renderCompletedEvent );
	ResetEvent( renderCommandsEvent );

	data = smpData;

	// after this, the main thread can exit GLimp_WakeRenderer
	SetEvent( renderActiveEvent );

	return data;
}


void GLimp_FrontEndSleep( void ) {
	WaitForSingleObject( renderCompletedEvent, INFINITE );

	if ( !qwglMakeCurrent( maindc, baseRC ) ) {
		wglErrors++;
	}
}


void GLimp_WakeRenderer( void *data ) {
	smpData = data;

	if ( !qwglMakeCurrent( maindc, NULL ) ) {
		wglErrors++;
	}

	// after this, the renderer can continue through GLimp_RendererSleep
	SetEvent( renderCommandsEvent );

	WaitForSingleObject( renderActiveEvent, INFINITE );
}

