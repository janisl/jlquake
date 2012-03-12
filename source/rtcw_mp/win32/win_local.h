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

// win_local.h: Win32-specific Quake3 header file

#if defined ( _MSC_VER ) && ( _MSC_VER >= 1200 )
#pragma warning(disable : 4201)
#pragma warning( push )
#endif
#include "../../wolfclient/windows_shared.h"
#if defined ( _MSC_VER ) && ( _MSC_VER >= 1200 )
#pragma warning( pop )
#endif

#include <dinput.h>
#include <dsound.h>

#include <winsock.h>
#include <wsipx.h>

void    IN_MouseEvent( int mstate );

// Input subsystem

void    IN_Init( void );
void    IN_Shutdown( void );
void    IN_JoystickCommands( void );

void    IN_Move( wmusercmd_t *cmd );
// add additional non keyboard / non mouse movement on top of the keyboard move cmd

void    IN_DeactivateWin32Mouse( void );

void    IN_Activate( qboolean active );
void    IN_Frame( void );

// window procedure
LRESULT WINAPI MainWndProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam );

void SNDDMA_Activate( void );
int  SNDDMA_InitDS();
