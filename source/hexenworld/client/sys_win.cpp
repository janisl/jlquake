// sys_win.h

#include "quakedef.h"
#include "../../client/windows_shared.h"

#define MINIMUM_WIN_MEMORY      0x1000000
#define MAXIMUM_WIN_MEMORY      0x1600000

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


void Sys_SendKeyEvents(void)
{
	MSG msg;

	while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!GetMessage(&msg, NULL, 0, 0))
		{
			Sys_Quit();
		}
		// save the msg time, because wndprocs don't have access to the timestamp
		sysMsgTime = msg.time;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
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
	MEMORYSTATUS lpBuffer;
	static char cwd[1024];
	int t;

	/* previous instances do not exist in Win32 */
	if (hPrevInstance)
	{
		return 0;
	}

	global_hInstance = hInstance;

	lpBuffer.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&lpBuffer);

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

// take the greater of all the available memory or half the total memory,
// but at least 8 Mb and no more than 16 Mb, unless they explicitly
// request otherwise
	parms.memsize = lpBuffer.dwAvailPhys;

	if (parms.memsize < MINIMUM_WIN_MEMORY)
	{
		parms.memsize = MINIMUM_WIN_MEMORY;
	}

	if (parms.memsize < (lpBuffer.dwTotalPhys >> 1))
	{
		parms.memsize = lpBuffer.dwTotalPhys >> 1;
	}

	if (parms.memsize > MAXIMUM_WIN_MEMORY)
	{
		parms.memsize = MAXIMUM_WIN_MEMORY;
	}

	if (COM_CheckParm("-heapsize"))
	{
		t = COM_CheckParm("-heapsize") + 1;

		if (t < COM_Argc())
		{
			parms.memsize = String::Atoi(COM_Argv(t)) * 1024;
		}
	}

	parms.membase = malloc(parms.memsize);

	if (!parms.membase)
	{
		common->FatalError("Not enough memory free; check disk space\n");
	}

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
