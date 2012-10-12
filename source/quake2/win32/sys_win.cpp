/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// sys_win.h

#include "../qcommon/qcommon.h"
#include "../../common/system_windows.h"
#ifndef DEDICATED_ONLY
#include "../../client/windows_shared.h"
#endif
#include <direct.h>
#include <dsound.h>

#define MAX_NUM_ARGVS   128
int argc;
char* argv[MAX_NUM_ARGVS];


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_Error(const char* error, ...)
{
	va_list argptr;
	char text[1024];

	CL_Shutdown();
	Qcommon_Shutdown();

	va_start(argptr, error);
	Q_vsnprintf(text, 1024, error, argptr);
	va_end(argptr);

	Sys_Print(text);
	Sys_Print("\n");

	Sys_SetErrorText(text);
	Sys_ShowConsole(1, true);

	// wait for the user to quit
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	Sys_DestroyConsole();
	exit(1);
}

void Sys_Quit(void)
{
	timeEndPeriod(1);

	CL_Shutdown();
	Qcommon_Shutdown();

	Sys_DestroyConsole();
	exit(0);
}

//================================================================

/*
================
Sys_Init
================
*/
void Sys_Init(void)
{
	timeBeginPeriod(1);
}

/*
==============================================================================

 WINDOWS CRAP

==============================================================================
*/

/*
=================
Sys_AppActivate
=================
*/
void Sys_AppActivate(void)
{
#ifndef DEDICATED_ONLY
	ShowWindow(GMainWindow, SW_RESTORE);
	SetForegroundWindow(GMainWindow);
#endif
}

/*
==================
ParseCommandLine

==================
*/
void ParseCommandLine(LPSTR lpCmdLine)
{
	argc = 1;
	argv[0] = "exe";

	while (*lpCmdLine && (argc < MAX_NUM_ARGVS))
	{
		while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if (*lpCmdLine)
		{
			argv[argc] = lpCmdLine;
			argc++;

			while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
				lpCmdLine++;

			if (*lpCmdLine)
			{
				*lpCmdLine = 0;
				lpCmdLine++;
			}

		}
	}

}

/*
==================
WinMain

==================
*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int time, oldtime, newtime;

	/* previous instances do not exist in Win32 */
	if (hPrevInstance)
	{
		return 0;
	}

	global_hInstance = hInstance;

	Sys_CreateConsole("Quake 2 Console");

	ParseCommandLine(lpCmdLine);

	Qcommon_Init(argc, argv);
	oldtime = Sys_Milliseconds_();

	/* main window message loop */
	while (1)
	{
#ifndef DEDICATED_ONLY
		if (Minimized || (com_dedicated && com_dedicated->value))
#endif
		{
			Sleep(1);
		}

		do
		{
			newtime = Sys_Milliseconds_();
			time = newtime - oldtime;
		}
		while (time < 1);
//			common->Printf ("time:%5.2f - %5.2f = %5.2f\n", newtime, oldtime, time);

		//	_controlfp( ~( _EM_ZERODIVIDE /*| _EM_INVALID*/ ), _MCW_EM );
		//_controlfp( _PC_24, _MCW_PC );
		Qcommon_Frame(time);

		oldtime = newtime;
	}

	// never gets here
	return TRUE;
}
