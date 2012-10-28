// sys_win.h

#include "quakedef.h"
#include "../../client/windows_shared.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	quakeparms_t parms;
	double time, oldtime, newtime;
	int t;

	/* previous instances do not exist in Win32 */
	if (hPrevInstance)
	{
		return 0;
	}

	global_hInstance = hInstance;

	parms.argc = __argc;
	parms.argv = __argv;

	COM_InitArgv2(parms.argc, parms.argv);

	Sys_CreateConsole("HexenWorld Console");

	Sys_Init();

	common->Printf("Host_Init\n");
	Host_Init(&parms);

	oldtime = Sys_DoubleTime();

	/* main window message loop */
	while (1)
	{
		// yield the CPU for a little while when paused, minimized, or not the focus
		if (Minimized || !ActiveApp)
		{
			Sleep(5);
		}

		newtime = Sys_DoubleTime();
		time = newtime - oldtime;

		Host_Frame(time);
		oldtime = newtime;
	}

	/* return success of application */
	return TRUE;
}
