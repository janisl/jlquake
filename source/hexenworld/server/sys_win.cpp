#include <sys/timeb.h>
#include "qwsvdef.h"
#include "../../common/system_windows.h"


/*
================
Sys_Error
================
*/
void Sys_Error(const char* error, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr,error);
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

/*
================
Sys_Quit
================
*/
void Sys_Quit(void)
{
	Sys_DestroyConsole();
	exit(0);
}


/*
=============
Sys_Init

Quake calls this so the system can register variables before host_hunklevel
is marked
=============
*/
void Sys_Init(void)
{
}

/*
==================
main

==================
*/
char* newargv[256];

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	quakeparms_t parms;
	double newtime, time, oldtime;
	static char cwd[1024];

	global_hInstance = hInstance;

	Sys_CreateConsole("HexenWorld Console");

	COM_InitArgv2(__argc, __argv);

	parms.argc = __argc;
	parms.argv = __argv;

	parms.basedir = ".";

	COM_InitServer(&parms);

// run one frame immediately for first heartbeat
	COM_ServerFrame(HX_FRAME_TIME);

//
// main loop
//
	oldtime = Sys_DoubleTime() - HX_FRAME_TIME;
	while (1)
	{
		// select on the net socket and stdin
		// the only reason we have a timeout at all is so that if the last
		// connected client times out, the message would not otherwise
		// be printed until the next event.
		//JL: Originally timeout was 0.1 ms
		if (!SOCK_Sleep(ip_sockets[0], 1))
		{
			continue;
		}

		// find time passed since last cycle
		newtime = Sys_DoubleTime();
		time = newtime - oldtime;
		oldtime = newtime;

		COM_ServerFrame(time);
	}

	return true;
}
