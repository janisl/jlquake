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
#include "../../client/client.h"
#include "../qcommon/qcommon.h"
#include "../../client/windows_shared.h"
#include <direct.h>

static char sys_cmdline[MAX_STRING_CHARS];

/*
================
Sys_Init

Called after the common systems (cvars, files, etc)
are initialized
================
*/
void Sys_Init(void)
{
	// make sure the timer is high precision, otherwise
	// NT gets 18ms resolution
	timeBeginPeriod(1);

	Cmd_AddCommand("net_restart", Net_Restart_f);

	OSVERSIONINFO osversion;
	osversion.dwOSVersionInfoSize = sizeof(osversion);

	if (!GetVersionEx(&osversion))
	{
		Sys_Error("Couldn't get OS info");
	}

	if (osversion.dwMajorVersion < 5)
	{
		Sys_Error("Quake3 requires Windows version 5 or greater");
	}

	Cvar_Set("arch", "winnt");

	SysT3_InitCpu();

	Cvar_Set("username", Sys_GetCurrentUser());
}


//=======================================================================

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
