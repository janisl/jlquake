/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).

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

// win_main.h

#include "../../common/qcommon.h"
#include "../../client/client.h"
#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "win_local.h"
#include "resource.h"
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>

static char sys_cmdline[MAX_STRING_CHARS];

int totalMsec, countMsec;

/*
==================
WinMain

==================
*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	char cwd[MAX_OSPATH];
	int startTime, endTime;

	// should never get a previous instance in Win32
	if (hPrevInstance)
	{
		return 0;
	}

	global_hInstance = hInstance;
	String::NCpyZ(sys_cmdline, lpCmdLine, sizeof(sys_cmdline));

	// done before Com/Sys_Init since we need this for error output
	Sys_CreateConsole("Wolf Console");

	// no abort/retry/fail errors
	SetErrorMode(SEM_FAILCRITICALERRORS);

	// get the initial time base
	Sys_Milliseconds();

	Com_Init(sys_cmdline);
	NETQ23_Init();

	_getcwd(cwd, sizeof(cwd));
	common->Printf("Working directory: %s\n", cwd);

	// hide the early console since we've reached the point where we
	// have a working graphics subsystems
	if (!com_dedicated->integer && !com_viewlog->integer)
	{
		Sys_ShowConsole(0, false);
	}

#ifndef DEDICATED
	SetFocus(GMainWindow);
#endif

	// main game loop
	while (1)
	{
		// if not running as a game client, sleep a bit
#ifndef DEDICATED
		if (Minimized || (com_dedicated && com_dedicated->integer))
#endif
		{
			Sleep(5);
		}

		// set low precision every frame, because some system calls
		// reset it arbitrarily
//		_controlfp( _PC_24, _MCW_PC );
//    _controlfp( -1, _MCW_EM  ); // no exceptions, even if some crappy
		// syscall turns them back on!

		startTime = Sys_Milliseconds();

		// run the game
		Com_Frame();

		endTime = Sys_Milliseconds();
		totalMsec += endTime - startTime;
		countMsec++;
	}

	// never gets here
}
