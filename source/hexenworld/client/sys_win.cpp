// sys_win.h

#include "quakedef.h"
#include "../../client/windows_shared.h"

#define PAUSE_SLEEP     50				// sleep time on pause or minimization
#define NOT_FOCUS_SLEEP 20				// sleep time when not focus

#define MAX_NUM_ARGVS   50

HANDLE qwclsemaphore;

/*
===============================================================================

SYSTEM IO

===============================================================================
*/

/*
================
Sys_Init
================
*/
void Sys_Init(void)
{
#ifndef SERVERONLY
	// allocate a named semaphore on the client so the
	// front end can tell if it is alive

	// mutex will fail if semephore allready exists
	qwclsemaphore = CreateMutex(
		NULL,			/* Security attributes */
		0,				/* owner       */
		"hwcl");/* Semaphore name      */
	if (!qwclsemaphore)
	{
		common->FatalError("HWCL is already running on this system");
	}
	CloseHandle(qwclsemaphore);

	qwclsemaphore = CreateSemaphore(
		NULL,			/* Security attributes */
		0,				/* Initial count       */
		1,				/* Maximum count       */
		"hwcl");/* Semaphore name      */
#endif

	timeBeginPeriod(1);
}


void Sys_Error(const char* error, ...)
{
	va_list argptr;
	char text[1024], text2[1024];
	DWORD dummy;

	Host_Shutdown();

	va_start(argptr, error);
	Q_vsnprintf(text, 1024, error, argptr);
	va_end(argptr);

	Sys_Print(text);
	Sys_Print("\n");

	Sys_SetErrorText(text);
	Sys_ShowConsole(1, true);

#ifndef SERVERONLY
	CloseHandle(qwclsemaphore);
#endif

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
	Host_Shutdown();
#ifndef SERVERONLY
	if (qwclsemaphore)
	{
		CloseHandle(qwclsemaphore);
	}
#endif

	Sys_DestroyConsole();
	exit(0);
}

void Sys_Sleep(void)
{
	Sleep(1);
}

/*
==============================================================================

 WINDOWS CRAP

==============================================================================
*/

/*
==================
WinMain
==================
*/
char* argv[MAX_NUM_ARGVS];
static char* empty_string = "";


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	quakeparms_t parms;
	double time, oldtime, newtime;
	static char cwd[1024];
	int t;

	/* previous instances do not exist in Win32 */
	if (hPrevInstance)
	{
		return 0;
	}

	global_hInstance = hInstance;

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

	Sys_CreateConsole("HexenWorld Console");

	Sys_Init();

	common->Printf("Host_Init\n");
	Host_Init(&parms);

	oldtime = Sys_DoubleTime();

	/* main window message loop */
	while (1)
	{
		// yield the CPU for a little while when paused, minimized, or not the focus
		if ((cl.qh_paused && !ActiveApp) || Minimized)
		{
			Sleep(PAUSE_SLEEP);
		}
		else if (!ActiveApp)
		{
			Sleep(NOT_FOCUS_SLEEP);
		}

		newtime = Sys_DoubleTime();
		time = newtime - oldtime;

		Host_Frame(time);
		oldtime = newtime;
	}

	/* return success of application */
	return TRUE;
}
