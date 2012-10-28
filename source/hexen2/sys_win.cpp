// sys_win.c -- Win32 system interface code

/*
 * $Header: /H2 Mission Pack/SYS_WIN.C 8     4/13/98 1:01p Jmonroe $
 */

#include "quakedef.h"
#include "../client/windows_shared.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	/* previous instances do not exist in Win32 */
	if (hPrevInstance)
	{
		return 0;
	}

	global_hInstance = hInstance;

	quakeparms_t parms;
	parms.argc = __argc;
	parms.argv = __argv;

	COM_InitArgv2(parms.argc, parms.argv);

	Sys_CreateConsole("Hexen II Console");

	Sys_Init();

	common->Printf("Host_Init\n");
	Host_Init(&parms);

	double oldtime = Sys_DoubleTime();

	Cvar* sys_delay = Cvar_Get("sys_delay", "0", CVAR_ARCHIVE);

	/* main window message loop */
	while (1)
	{
		double newtime;
		double time;
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
			if (Minimized || !ActiveApp)
			{
				Sleep(5);
			}

			newtime = Sys_DoubleTime();
			time = newtime - oldtime;
		}

		if (sys_delay->value)
		{
			Sleep(sys_delay->value);
		}

		Host_Frame(time);
		oldtime = newtime;

	}

	/* return success of application */
	return 0;
}
