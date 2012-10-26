// sys_win.c -- Win32 system interface code

/*
 * $Header: /H2 Mission Pack/SYS_WIN.C 8     4/13/98 1:01p Jmonroe $
 */

#include "quakedef.h"
#include "../client/windows_shared.h"
#include "../server/public.h"
#include "../client/client.h"

#define PAUSE_SLEEP     50				// sleep time on pause or minimization
#define NOT_FOCUS_SLEEP 20				// sleep time when not focus

#define MAX_NUM_ARGVS   50

static HANDLE tevent;

Cvar* sys_delay;

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
	timeBeginPeriod(1);
}


void Sys_Error(const char* error, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, 1024, error, argptr);
	va_end(argptr);

	Sys_Print(text);
	Sys_Print("\n");

	Sys_SetErrorText(text);
	Sys_ShowConsole(1, true);

	ComQH_HostShutdown();

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
	if (tevent)
	{
		CloseHandle(tevent);
	}

	Sys_DestroyConsole();
	exit(0);
}

static void Sys_Sleep(void)
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
void SleepUntilInput(int time)
{

	MsgWaitForMultipleObjects(1, &tevent, FALSE, time, QS_ALLINPUT);
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

	SVH2_RemoveGIPFiles(NULL);

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

	Sys_CreateConsole("Hexen II Console");

	tevent = CreateEvent(NULL, FALSE, FALSE, NULL);

	if (!tevent)
	{
		common->FatalError("Couldn't create event");
	}

	Sys_Init();

	common->Printf("Host_Init\n");
	Host_Init(&parms);

	oldtime = Sys_DoubleTime();

	sys_delay = Cvar_Get("sys_delay", "0", CVAR_ARCHIVE);

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
			if ((cl.qh_paused && !ActiveApp) || Minimized)
			{
				SleepUntilInput(PAUSE_SLEEP);
			}
			else if (!ActiveApp)
			{
				SleepUntilInput(NOT_FOCUS_SLEEP);
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
	return TRUE;
}
