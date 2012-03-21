/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


#include "../client/client.h"
#include "win_local.h"

// Console variables that we need to access from this module
Cvar      *vid_xpos;          // X coordinate of window position
Cvar      *vid_ypos;          // Y coordinate of window position
extern Cvar* r_fullscreen;

#define VID_NUM_MODES ( sizeof( vid_modes ) / sizeof( vid_modes[0] ) )

LRESULT WINAPI MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

static qboolean s_alttab_disabled;

static void WIN_DisableAltTab( void ) {
	if ( s_alttab_disabled ) {
		return;
	}

	if ( !String::ICmp( Cvar_VariableString( "arch" ), "winnt" ) ) {
		RegisterHotKey( 0, 0, MOD_ALT, VK_TAB );
	} else
	{
		BOOL old;

		SystemParametersInfo( SPI_SCREENSAVERRUNNING, 1, &old, 0 );
	}
	s_alttab_disabled = qtrue;
}

static void WIN_EnableAltTab( void ) {
	if ( s_alttab_disabled ) {
		if ( !String::ICmp( Cvar_VariableString( "arch" ), "winnt" ) ) {
			UnregisterHotKey( 0, 0 );
		} else
		{
			BOOL old;

			SystemParametersInfo( SPI_SCREENSAVERRUNNING, 0, &old, 0 );
		}

		s_alttab_disabled = qfalse;
	}
}

/*
==================
VID_AppActivate
==================
*/
static void VID_AppActivate( BOOL fActive, BOOL minimize ) {
	Minimized = minimize;

	Com_DPrintf( "VID_AppActivate: %i\n", fActive );

	Key_ClearStates();  // FIXME!!!

	// we don't want to act like we're active if we're minimized
	if ( fActive && !Minimized ) {
		ActiveApp = qtrue;
	} else
	{
		ActiveApp = qfalse;
	}

	// minimize/restore mouse-capture on demand
	if ( !ActiveApp ) {
		IN_Activate( qfalse );
	} else
	{
		IN_Activate( qtrue );
	}
}

//==========================================================================

/*
====================
MainWndProc

main window procedure
====================
*/
LRESULT WINAPI MainWndProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam ) {

	if (IN_HandleInputMessage(uMsg, wParam, lParam))
	{
		return 0;
	}

	switch ( uMsg )
	{
	case WM_CREATE:

		GMainWindow = hWnd;

		vid_xpos = Cvar_Get( "vid_xpos", "3", CVAR_ARCHIVE );
		vid_ypos = Cvar_Get( "vid_ypos", "22", CVAR_ARCHIVE );
		r_fullscreen = Cvar_Get( "r_fullscreen", "1", CVAR_ARCHIVE | CVAR_LATCH2 );

		if ( r_fullscreen->integer ) {
			WIN_DisableAltTab();
		} else
		{
			WIN_EnableAltTab();
		}

		break;
#if 0
	case WM_DISPLAYCHANGE:
		Com_DPrintf( "WM_DISPLAYCHANGE\n" );
		// we need to force a vid_restart if the user has changed
		// their desktop resolution while the game is running,
		// but don't do anything if the message is a result of
		// our own calling of ChangeDisplaySettings
		if ( com_insideVidInit ) {
			break;      // we did this on purpose
		}
		// something else forced a mode change, so restart all our gl stuff
		Cbuf_AddText( "vid_restart\n" );
		break;
#endif
	case WM_DESTROY:
		// let sound and input know about this?
		GMainWindow = NULL;
		if ( r_fullscreen->integer ) {
			WIN_EnableAltTab();
		}
		break;

	case WM_CLOSE:
		Cbuf_ExecuteText( EXEC_APPEND, "quit" );
		break;

	case WM_ACTIVATE:
	{
		int fActive, fMinimized;

		fActive = LOWORD( wParam );
		fMinimized = (BOOL) HIWORD( wParam );

		VID_AppActivate( fActive != WA_INACTIVE, fMinimized );
		SNDDMA_Activate();
	}
	break;

	case WM_MOVE:
	{
		int xPos, yPos;
		RECT r;
		int style;

		if ( !r_fullscreen->integer ) {
			xPos = (short) LOWORD( lParam );      // horizontal position
			yPos = (short) HIWORD( lParam );      // vertical position

			r.left   = 0;
			r.top    = 0;
			r.right  = 1;
			r.bottom = 1;

			style = GetWindowLong( hWnd, GWL_STYLE );
			AdjustWindowRect( &r, style, FALSE );

			Cvar_SetValue( "vid_xpos", xPos + r.left );
			Cvar_SetValue( "vid_ypos", yPos + r.top );
			vid_xpos->modified = qfalse;
			vid_ypos->modified = qfalse;
			if ( ActiveApp ) {
				IN_Activate( qtrue );
			}
		}
	}
	break;

	case WM_SYSCOMMAND:
		if ( wParam == SC_SCREENSAVE ) {
			return 0;
		}
		break;

	case WM_SYSKEYDOWN:
		if ( wParam == 13 ) {
			if ( r_fullscreen ) {
				Cvar_SetValue( "r_fullscreen", !r_fullscreen->integer );
				Cbuf_AddText( "vid_restart\n" );
			}
			return 0;
		}
		break;
	}

	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}
