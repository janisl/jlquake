// sys_win.c -- Win32 system interface code

/*
 * $Header: /H2 Mission Pack/SYS_WIN.C 8     4/13/98 1:01p Jmonroe $
 */

#include "quakedef.h"
#include <windows.h>
#include "winquake.h"
#include "errno.h"
#include "resource.h"
#include <direct.h>
#include "conproc.h"

#define CRC_A 59461 // "Who's Ridin' With Chaos?"
#define CRC_B 54866 // "Santa needs a new sled!"

#define MINIMUM_WIN_MEMORY		0x1000000
#define MAXIMUM_WIN_MEMORY		0x1800000

#define CONSOLE_ERROR_TIMEOUT	60.0	// # of seconds to wait on Sys_Error running
										//  dedicated before exiting
#define PAUSE_SLEEP		50				// sleep time on pause or minimization
#define NOT_FOCUS_SLEEP	20				// sleep time when not focus

#define MAX_NUM_ARGVS	50

int			starttime;
qboolean	ActiveApp, Minimized;
qboolean	Win32AtLeastV4, WinNT;

static double		pfreq;
static double		curtime = 0.0;
static double		lastcurtime = 0.0;
static int			lowshift;
qboolean			isDedicated;
static qboolean		sc_return_on_enter = false;
HANDLE				hinput, houtput;

static char			*tracking_tag = "Sticky Buns";

static HANDLE	tevent;
static HANDLE	hFile;
static HANDLE	heventParent;
static HANDLE	heventChild;

extern "C"
{
void Sys_InitFloatTime (void);
}

QCvar*		sys_delay;

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
===============================================================================

FILE IO

===============================================================================
*/

#define	MAX_HANDLES		10
FILE	*sys_handles[MAX_HANDLES];

int		findhandle (void)
{
	int		i;
	
	for (i=1 ; i<MAX_HANDLES ; i++)
		if (!sys_handles[i])
			return i;
	Sys_Error ("out of handles");
	return -1;
}

/*
================
filelength
================
*/
int filelength (FILE *f)
{
	int		pos;
	int		end;
	int		t;

	t = VID_ForceUnlockedAndReturnState ();

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	VID_ForceLockState (t);

	return end;
}

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
void Sys_Init (void)
{
	LARGE_INTEGER	PerformanceFreq;
	unsigned int	lowpart, highpart;
	OSVERSIONINFO	vinfo;
static	char temp[MAX_PATH+1];
	int value,i;
	HKEY hKey;
    DWORD dwSize,dwType;
	unsigned short crc;

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

	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	if (!GetVersionEx (&vinfo))
		Sys_Error ("Couldn't get OS info");

	if (vinfo.dwMajorVersion < 4)
		Win32AtLeastV4 = false;
	else
		Win32AtLeastV4 = true;

	if (vinfo.dwPlatformId == VER_PLATFORM_WIN32s)
		Sys_Error ("Hexen2 requires at least Win95 or NT 4.0");
	
	if (vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
		WinNT = true;
	else
		WinNT = false;
}


void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024], text2[1024];
	char		*text3 = "Press Enter to exit\n";
	char		*text4 = "***********************************\n";
	char		*text5 = "\n";
	DWORD		dummy;
	double		starttime;

	VID_ForceUnlockedAndReturnState ();

	va_start (argptr, error);
	Q_vsnprintf(text, 1024, error, argptr);
	va_end (argptr);

	if (isDedicated)
	{
		va_start (argptr, error);
		Q_vsnprintf(text, 1024, error, argptr);
		va_end (argptr);

		sprintf (text2, "ERROR: %s\n", text);
		WriteFile (houtput, text5, QStr::Length(text5), &dummy, NULL);
		WriteFile (houtput, text4, QStr::Length(text4), &dummy, NULL);
		WriteFile (houtput, text2, QStr::Length(text2), &dummy, NULL);
		WriteFile (houtput, text3, QStr::Length(text3), &dummy, NULL);
		WriteFile (houtput, text4, QStr::Length(text4), &dummy, NULL);


		starttime = Sys_FloatTime ();
		sc_return_on_enter = true;	// so Enter will get us out of here

		while (!Sys_ConsoleInput () &&
				((Sys_FloatTime () - starttime) < CONSOLE_ERROR_TIMEOUT))
		{
		}
	}
	else
	{
	// switch to windowed so the message box is visible
		VID_SetDefaultMode ();
		MessageBox(NULL, text, "Hexen II Error", MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
	}

	Host_Shutdown ();

// shut down QHOST hooks if necessary
	DeinitConProc ();

	exit (1);
}

void Sys_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];
	DWORD		dummy;
	
	if (isDedicated)
	{
		va_start (argptr,fmt);
		Q_vsnprintf(text, 1024, fmt, argptr);
		va_end (argptr);

		WriteFile(houtput, text, QStr::Length(text), &dummy, NULL);	
	}
}

void Sys_Quit (void)
{

	VID_ForceUnlockedAndReturnState ();

	Host_Shutdown();

	if (tevent)
		CloseHandle (tevent);

	if (isDedicated)
		FreeConsole ();

// shut down QHOST hooks if necessary
	DeinitConProc ();

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


char *Sys_ConsoleInput (void)
{
	static char	text[256];
	static int		len;
	INPUT_RECORD	recs[1024];
	int		count;
	int		i;
	DWORD	dummy;
	DWORD	ch, numread, numevents;

	if (!isDedicated)
		return NULL;


	for ( ;; )
	{
		if (!GetNumberOfConsoleInputEvents (hinput, &numevents))
			Sys_Error ("Error getting # of console events");

		if (numevents <= 0)
			break;

		if (!ReadConsoleInput(hinput, recs, 1, &numread))
			Sys_Error ("Error reading console input");

		if (numread != 1)
			Sys_Error ("Couldn't read console input");

		if (recs[0].EventType == KEY_EVENT)
		{
			if (!recs[0].Event.KeyEvent.bKeyDown)
			{
				ch = recs[0].Event.KeyEvent.uChar.AsciiChar;

				switch (ch)
				{
					case '\r':
						WriteFile(houtput, "\r\n", 2, &dummy, NULL);	

						if (len)
						{
							text[len] = 0;
							len = 0;
							return text;
						}
						else if (sc_return_on_enter)
						{
						// special case to allow exiting from the error handler on Enter
							text[0] = '\r';
							len = 0;
							return text;
						}

						break;

					case '\b':
						WriteFile(houtput, "\b \b", 3, &dummy, NULL);	
						if (len)
						{
							len--;
						}
						break;

					default:
						if (ch >= ' ')
						{
							WriteFile(houtput, &ch, 1, &dummy, NULL);	
							text[len] = ch;
							len = (len + 1) & 0xff;
						}

						break;

				}
			}
		}
	}

	return NULL;
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
HWND		hwnd_dialog;


#define H2_PARAM_KEY      "Software\\Hexen2"
#define H2_FLAG_VALUE     "Flag"

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

	CL_RemoveGIPFiles(NULL);

	global_hInstance = hInstance;
	global_nCmdShow = nCmdShow;

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

	if (!isDedicated)
	{
		hwnd_dialog = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, NULL);

		if (hwnd_dialog)
		{
			ShowWindow (hwnd_dialog, SW_SHOWDEFAULT);
			UpdateWindow (hwnd_dialog);
			SetForegroundWindow (hwnd_dialog);
		}
	}

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
			parms.memsize = QStr::Atoi (COM_Argv(t)) * 1024;
	}

	parms.membase = malloc (parms.memsize);

	if (!parms.membase)
		Sys_Error ("Not enough memory free; check disk space\n");

	if(COM_CheckParm("-nopagein") == 0)
	{
		Sys_PageIn (parms.membase, parms.memsize);
	}

	tevent = CreateEvent(NULL, FALSE, FALSE, NULL);

	if (!tevent)
		Sys_Error ("Couldn't create event");

	if (isDedicated)
	{
		if (!AllocConsole ())
		{
			Sys_Error ("Couldn't create dedicated server console");
		}

		hinput = GetStdHandle (STD_INPUT_HANDLE);
		houtput = GetStdHandle (STD_OUTPUT_HANDLE);

	// give QHOST a chance to hook into the console
		if ((t = COM_CheckParm ("-HFILE")) > 0)
		{
			if (t < COM_Argc())
				hFile = (HANDLE)QStr::Atoi (COM_Argv(t+1));
		}
			
		if ((t = COM_CheckParm ("-HPARENT")) > 0)
		{
			if (t < COM_Argc())
				heventParent = (HANDLE)QStr::Atoi (COM_Argv(t+1));
		}
			
		if ((t = COM_CheckParm ("-HCHILD")) > 0)
		{
			if (t < COM_Argc())
				heventChild = (HANDLE)QStr::Atoi (COM_Argv(t+1));
		}

		InitConProc (hFile, heventParent, heventChild);
	}

	Sys_Init ();

	Sys_Printf ("Host_Init\n");
	Host_Init (&parms);

	oldtime = Sys_FloatTime ();

	sys_delay = Cvar_Get("sys_delay", "0", CVAR_ARCHIVE);

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
			if ((cl.paused && !ActiveApp) || Minimized)
			{
				SleepUntilInput (PAUSE_SLEEP);
				scr_skipupdate = 1;		// no point in bothering to draw
			}
			else if (!ActiveApp)
			{
				SleepUntilInput (NOT_FOCUS_SLEEP);
				scr_skipupdate = 1;		// no point in bothering to draw
			}

			newtime = Sys_FloatTime ();
			time = newtime - oldtime;
		}

		if (sys_delay->value) 
			Sleep(sys_delay->value);

		Host_Frame (time);
		oldtime = newtime;

	}

    /* return success of application */
    return TRUE;
}
