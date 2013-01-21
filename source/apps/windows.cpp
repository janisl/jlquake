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

#include "../common/qcommon.h"
#include "../common/system_windows.h"
#include "../common/common_defs.h"
#include "../common/strings.h"
#include "../common/system.h"
#include "main.h"

static char sys_cmdline[MAX_STRING_CHARS];

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// should never get a previous instance in Win32
	if (hPrevInstance)
	{
		return 0;
	}

	global_hInstance = hInstance;
	String::NCpyZ(sys_cmdline, lpCmdLine, sizeof(sys_cmdline));

	// done before Com/Sys_Init since we need this for error output
	Sys_CreateConsole("JLQuake Console");

	// no abort/retry/fail errors
	SetErrorMode(SEM_FAILCRITICALERRORS);

	Com_Init(__argc, __argv, sys_cmdline);

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
		if (Minimized || (com_dedicated && com_dedicated->integer))
		{
			Sleep(5);
		}

		// run the game
		Com_Frame();
	}

	// never gets here
	return 0;
}
