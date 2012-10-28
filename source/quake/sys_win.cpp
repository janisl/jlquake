/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// sys_win.c -- Win32 system interface code

#include "quakedef.h"
#include "../common/system_windows.h"
#include "../client/windows_shared.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	quakeparms_t parms;
	double time, oldtime, newtime;

	/* previous instances do not exist in Win32 */
	if (hPrevInstance)
	{
		return 0;
	}

	global_hInstance = hInstance;

	Sys_CreateConsole("Quake Console");

	parms.argc = __argc;
	parms.argv = __argv;

	COM_InitArgv2(parms.argc, parms.argv);

	Sys_Init();

	common->Printf("Host_Init\n");
	Host_Init(&parms);

	oldtime = Sys_DoubleTime();

	/* main window message loop */
	while (1)
	{
		if (com_dedicated->integer)
		{
			newtime = Sys_DoubleTime();
			time = newtime - oldtime;

			while (time < sys_ticrate->value)
			{
				Sleep(1);
				newtime = Sys_DoubleTime();
				time = newtime - oldtime;
			}
		}
		else
		{
			// yield the CPU for a little while when paused, minimized, or not the focus
			if (!ActiveApp || Minimized)
			{
				Sleep(5);
			}

			newtime = Sys_DoubleTime();
			time = newtime - oldtime;
		}

		Host_Frame(time);
		oldtime = newtime;
	}

	/* return success of application */
	return TRUE;
}
