/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
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
#include "winquake.h"

#define MINIMUM_WIN_MEMORY		0x0880000
#define MAXIMUM_WIN_MEMORY		0x1000000

#define PAUSE_SLEEP		50				// sleep time on pause or minimization
#define NOT_FOCUS_SLEEP	20				// sleep time when not focus

#define MAX_NUM_ARGVS	50

int			starttime;
qboolean	ActiveApp, Minimized;
qboolean	WinNT;

static double		pfreq;
static double		curtime = 0.0;
static double		lastcurtime = 0.0;
static int			lowshift;
qboolean			isDedicated;

static HANDLE	tevent;

void Sys_InitFloatTime (void);

volatile int					sys_checksum;


/*
================
Sys_PageIn
================
*/
void Sys_PageIn (void *ptr, int size)
{
	byte	*x;
	int		j, m, n;

// touch all the memory to make sure it's there. The 16-page skip is to
// keep Win 95 from thinking we're trying to page ourselves in (we are
// doing that, of course, but there's no reason we shouldn't)
	x = (byte *)ptr;

	for (n=0 ; n<4 ; n++)
	{
		for (m=0 ; m<(size - 16 * 0x1000) ; m += 4)
		{
			sys_checksum += *(int *)&x[m];
			sys_checksum += *(int *)&x[m + 16 * 0x1000];
		}
	}
}


/*
================
Sys_Init
================
*/
void Sys_Init (void)
{
	LARGE_INTEGER	PerformanceFreq;
	unsigned int	lowpart, highpart;

	if (!QueryPerformanceFrequency (&PerformanceFreq))
		Sys_Error ("No hardware timer available");

// get 32 out of the 64 time bits such that we have around
// 1 microsecond resolution
	lowpart = (unsigned int)PerformanceFreq.LowPart;
	highpart = (unsigned int)PerformanceFreq.HighPart;
	lowshift = 0;

	while (highpart || (lowpart > 2000000.0))
	{
		lowshift++;
		lowpart >>= 1;
		lowpart |= (highpart & 1) << 31;
		highpart >>= 1;
	}

	pfreq = 1.0 / (double)lowpart;

	Sys_InitFloatTime ();
}


void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024];
	static int	in_sys_error0 = 0;
	static int	in_sys_error1 = 0;
	static int	in_sys_error3 = 0;

	if (!in_sys_error3)
	{
		in_sys_error3 = 1;
		VID_ForceUnlockedAndReturnState ();
	}

	va_start (argptr, error);
	Q_vsnprintf(text, 1024, error, argptr);
	va_end (argptr);

	Sys_Print(text);
	Sys_Print("\n");

	Sys_SetErrorText(text);
	Sys_ShowConsole(1, true);

	if (!isDedicated)
	{
	// switch to windowed so the message box is visible, unless we already
	// tried that and failed
		if (!in_sys_error0)
		{
			in_sys_error0 = 1;
			VID_SetDefaultMode ();
		}
	}

	if (!in_sys_error1)
	{
		in_sys_error1 = 1;
		Host_Shutdown ();
	}

	// wait for the user to quit
    MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
      	DispatchMessage(&msg);
	}

	Sys_DestroyConsole();
	exit (1);
}

void Sys_Quit (void)
{

	VID_ForceUnlockedAndReturnState ();

	Host_Shutdown();

	if (tevent)
		CloseHandle (tevent);

	Sys_DestroyConsole();
	exit (0);
}


/*
================
Sys_FloatTime
================
*/
double Sys_FloatTime (void)
{
	static int			sametimecount;
	static unsigned int	oldtime;
	static int			first = 1;
	LARGE_INTEGER		PerformanceCount;
	unsigned int		temp, t2;
	double				time;

	QueryPerformanceCounter (&PerformanceCount);

	temp = ((unsigned int)PerformanceCount.LowPart >> lowshift) |
		   ((unsigned int)PerformanceCount.HighPart << (32 - lowshift));

	if (first)
	{
		oldtime = temp;
		first = 0;
	}
	else
	{
	// check for turnover or backward time
		if ((temp <= oldtime) && ((oldtime - temp) < 0x10000000))
		{
			oldtime = temp;	// so we can't get stuck
		}
		else
		{
			t2 = temp - oldtime;

			time = (double)t2 * pfreq;
			oldtime = temp;

			curtime += time;

			if (curtime == lastcurtime)
			{
				sametimecount++;

				if (sametimecount > 100000)
				{
					curtime += 1.0;
					sametimecount = 0;
				}
			}
			else
			{
				sametimecount = 0;
			}

			lastcurtime = curtime;
		}
	}

    return curtime;
}


/*
================
Sys_InitFloatTime
================
*/
void Sys_InitFloatTime (void)
{
	int		j;

	Sys_FloatTime ();

	j = COM_CheckParm("-starttime");

	if (j)
	{
		curtime = (double) (QStr::Atof(COM_Argv(j+1)));
	}
	else
	{
		curtime = 0.0;
	}

	lastcurtime = curtime;
}

static void Sys_Sleep (void)
{
	Sleep (1);
}


void Sys_SendKeyEvents (void)
{
    MSG        msg;

	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
	{
	// we always update if there are any event, even if we're paused
		scr_skipupdate = 0;

		if (!GetMessage (&msg, NULL, 0, 0))
			Sys_Quit ();

		// save the msg time, because wndprocs don't have access to the timestamp
		sysMsgTime = msg.time;

      	TranslateMessage (&msg);
      	DispatchMessage (&msg);
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
void SleepUntilInput (int time)
{

	MsgWaitForMultipleObjects(1, &tevent, FALSE, time, QS_ALLINPUT);
}


/*
==================
WinMain
==================
*/
int			global_nCmdShow;
char		*argv[MAX_NUM_ARGVS];
static char	*empty_string = "";


int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG				msg;
	quakeparms_t	parms;
	double			time, oldtime, newtime;
	MEMORYSTATUS	lpBuffer;
	static	char	cwd[1024];
	int				t;

    /* previous instances do not exist in Win32 */
    if (hPrevInstance)
        return 0;

	global_hInstance = hInstance;
	global_nCmdShow = nCmdShow;

	Sys_CreateConsole("Quake Console");

	lpBuffer.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus (&lpBuffer);

	if (!GetCurrentDirectory (sizeof(cwd), cwd))
		Sys_Error ("Couldn't determine current directory");

	if (cwd[QStr::Length(cwd)-1] == '/')
		cwd[QStr::Length(cwd)-1] = 0;

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

	isDedicated = (COM_CheckParm ("-dedicated") != 0);

// take the greater of all the available memory or half the total memory,
// but at least 8 Mb and no more than 16 Mb, unless they explicitly
// request otherwise
	parms.memsize = lpBuffer.dwAvailPhys;

	if (parms.memsize < MINIMUM_WIN_MEMORY)
		parms.memsize = MINIMUM_WIN_MEMORY;

	if (parms.memsize < (lpBuffer.dwTotalPhys >> 1))
		parms.memsize = lpBuffer.dwTotalPhys >> 1;

	if (parms.memsize > MAXIMUM_WIN_MEMORY)
		parms.memsize = MAXIMUM_WIN_MEMORY;

	if (COM_CheckParm ("-heapsize"))
	{
		t = COM_CheckParm("-heapsize") + 1;

		if (t < COM_Argc())
			parms.memsize = QStr::Atoi(COM_Argv(t)) * 1024;
	}

	parms.membase = malloc (parms.memsize);

	if (!parms.membase)
		Sys_Error ("Not enough memory free; check disk space\n");

	Sys_PageIn (parms.membase, parms.memsize);

	tevent = CreateEvent(NULL, FALSE, FALSE, NULL);

	if (!tevent)
		Sys_Error ("Couldn't create event");

	Sys_Init ();

	Con_Printf("Host_Init\n");
	Host_Init (&parms);

	oldtime = Sys_FloatTime ();

    /* main window message loop */
	while (1)
	{
		if (isDedicated)
		{
			newtime = Sys_FloatTime ();
			time = newtime - oldtime;

			while (time < sys_ticrate->value )
			{
				Sys_Sleep();
				newtime = Sys_FloatTime ();
				time = newtime - oldtime;
			}
		}
		else
		{
		// yield the CPU for a little while when paused, minimized, or not the focus
			if ((cl.paused && !ActiveApp) || Minimized || block_drawing)
			{
				SleepUntilInput (PAUSE_SLEEP);
				scr_skipupdate = 1;		// no point in bothering to draw
			}
			else if (!ActiveApp)
			{
				SleepUntilInput (NOT_FOCUS_SLEEP);
			}

			newtime = Sys_FloatTime ();
			time = newtime - oldtime;
		}

		Host_Frame (time);
		oldtime = newtime;
	}

    /* return success of application */
    return TRUE;
}
