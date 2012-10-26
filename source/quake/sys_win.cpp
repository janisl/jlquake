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
#include "../client/windows_shared.h"
#include "../client/client.h"

#define PAUSE_SLEEP     50				// sleep time on pause or minimization
#define NOT_FOCUS_SLEEP 20				// sleep time when not focus

#define MAX_NUM_ARGVS   50

static void Sys_Sleep(void)
{
	Sleep(1);
}

/*
==================
WinMain
==================
*/
char* argv[MAX_NUM_ARGVS];
static char* empty_string = "";


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	quakeparms_t parms;
	double time, oldtime, newtime;
	static char cwd[1024];

	/* previous instances do not exist in Win32 */
	if (hPrevInstance)
	{
		return 0;
	}

	global_hInstance = hInstance;

	Sys_CreateConsole("Quake Console");

	if (!GetCurrentDirectory(sizeof(cwd), cwd))
	{
		common->FatalError("Couldn't determine current directory");
	}

	if (cwd[String::Length(cwd) - 1] == '/')
	{
		cwd[String::Length(cwd) - 1] = 0;
	}

	parms.basedir = cwd;

	parms.argc = 1;
	argv[0] = empty_string;

	while (*lpCmdLine && (parms.argc < MAX_NUM_ARGVS))
	{
		while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if (*lpCmdLine)
		{
			argv[parms.argc] = lpCmdLine;
			parms.argc++;

			while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
				lpCmdLine++;

			if (*lpCmdLine)
			{
				*lpCmdLine = 0;
				lpCmdLine++;
			}

		}
	}

	parms.argv = argv;

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
				Sys_Sleep();
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
