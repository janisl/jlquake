/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// win_main.c

#include "../../common/qcommon.h"
#include "../qcommon/qcommon.h"
#include "../../client/windows_shared.h"
#include <direct.h>

static char sys_cmdline[MAX_STRING_CHARS];

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	char cwd[MAX_OSPATH];

	// should never get a previous instance in Win32
	if (hPrevInstance)
	{
		return 0;
	}

	global_hInstance = hInstance;
	String::NCpyZ(sys_cmdline, lpCmdLine, sizeof(sys_cmdline));

	// done before Com/Sys_Init since we need this for error output
	Sys_CreateConsole("Quake 3 Console");

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

		// run the game
		Com_Frame();
	}

	// never gets here
}
