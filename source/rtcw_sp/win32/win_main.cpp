/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// win_main.h

#include "../client/client.h"
#include "../qcommon/qcommon.h"
#include "win_local.h"
#include "resource.h"
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>

//#define	CD_BASEDIR	"wolf"
#define CD_BASEDIR  ""
//#define	CD_EXE		"wolf.exe"
#define CD_EXE      "setup\\setup.exe"
//#define	CD_BASEDIR_LINUX	"bin\\x86\\glibc-2.1"
#define CD_BASEDIR_LINUX    "bin\\x86\\glibc-2.1"
//#define	CD_EXE_LINUX "wolf"
#define CD_EXE_LINUX "setup\\setup"

static char sys_cmdline[MAX_STRING_CHARS];

//NOTE TTimo: heavily NON PORTABLE, PLZ DON'T USE
//  show_bug.cgi?id=447
#if 0
//----(SA) added
/*
==============
Sys_ShellExecute

-	Windows only

    Performs an operation on a specified file.

    See info on ShellExecute() for details

==============
*/
int Sys_ShellExecute(char* op, char* file, qboolean doexit, char* params, char* dir)
{
	unsigned int retval;
	char* se_op;

	// set default operation to "open"
	if (op)
	{
		se_op = op;
	}
	else
	{
		se_op = "open";
	}


	// probably need to protect this some in the future so people have
	// less chance of system invasion with this powerful interface
	// (okay, not so invasive, but could be annoying/rude)


	retval = (UINT)ShellExecute(NULL, se_op, file, params, dir, SW_NORMAL);		// only option forced by game is 'sw_normal'

	if (retval <= 32)		// ERROR
	{
		Com_DPrintf("Sys_ShellExecuteERROR: %d\n", retval);
		return retval;
	}

	if (doexit)
	{
		// (SA) this works better for exiting cleanly...
		Cbuf_ExecuteText(EXEC_APPEND, "quit");
	}

	return 999;	// success
}
//----(SA) end
#endif

//----(SA)	from NERVE MP codebase (10/15/01)  (checkins at time of this file should be related)
/*
==================
Sys_StartProcess
==================
*/
void Sys_StartProcess(const char* exeName, qboolean doexit)					// NERVE - SMF
{
	TCHAR szPathOrig[_MAX_PATH];
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	GetCurrentDirectory(_MAX_PATH, szPathOrig);
	if (!CreateProcess(NULL, va("%s\\%s", szPathOrig, exeName), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		// couldn't start it, popup error box
		Com_Error(ERR_DROP, "Could not start process: '%s\\%s' ", szPathOrig, exeName);
		return;
	}

	// TTimo: similar way of exiting as used in Sys_OpenURL below
	if (doexit)
	{
		Cbuf_ExecuteText(EXEC_APPEND, "quit");
	}
}

/*
==================
Sys_OpenURL
==================
*/
void Sys_OpenURL(char* url, qboolean doexit)					// NERVE - SMF
{
	HWND wnd;

	if (!ShellExecute(NULL, "open", url, NULL, NULL, SW_RESTORE))
	{
		// couldn't start it, popup error box
		Com_Error(ERR_DROP, "Could not open url: '%s' ", url);
		return;
	}

	wnd = GetForegroundWindow();

	if (wnd)
	{
		ShowWindow(wnd, SW_MAXIMIZE);
	}

	if (doexit)
	{
		Cbuf_ExecuteText(EXEC_APPEND, "quit");
	}
}
//----(SA)	end

/*
==================
Sys_BeginProfiling
==================
*/
void Sys_BeginProfiling(void)
{
	// this is just used on the mac build
}

/*
=============
Sys_Error

Show the early console as an error dialog
=============
*/
void QDECL Sys_Error(const char* error, ...)
{
	va_list argptr;
	char text[4096];
	MSG msg;

	va_start(argptr, error);
	vsprintf(text, error, argptr);
	va_end(argptr);

	Sys_Print(text);
	Sys_Print("\n");

	Sys_SetErrorText(text);
	Sys_ShowConsole(1, qtrue);

	timeEndPeriod(1);

	IN_Shutdown();

	// wait for the user to quit
	while (1)
	{
		if (!GetMessage(&msg, NULL, 0, 0))
		{
			Com_Quit_f();
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	Sys_DestroyConsole();

	exit(1);
}

/*
==============
Sys_Quit
==============
*/
void Sys_Quit(void)
{
	timeEndPeriod(1);
	IN_Shutdown();
	Sys_DestroyConsole();

	exit(0);
}

//========================================================


/*
================
Sys_ScanForCD

Search all the drives to see if there is a valid CD to grab
the cddir from
================
*/
qboolean Sys_ScanForCD(void)
{
	static char cddir[MAX_OSPATH];
	char drive[4];
	FILE* f;
	char test[MAX_OSPATH];
#if 0
	// don't override a cdpath on the command line
	if (strstr(sys_cmdline, "cdpath"))
	{
		return;
	}
#endif

	drive[0] = 'c';
	drive[1] = ':';
	drive[2] = '\\';
	drive[3] = 0;

	// scan the drives
	for (drive[0] = 'c'; drive[0] <= 'z'; drive[0]++)
	{
		if (GetDriveType(drive) != DRIVE_CDROM)
		{
			continue;
		}

		sprintf(cddir, "%s%s", drive, CD_BASEDIR);
		sprintf(test, "%s\\%s", cddir, CD_EXE);
		f = fopen(test, "r");
		if (f)
		{
			fclose(f);
			return qtrue;

		}
		else
		{
			sprintf(cddir, "%s%s", drive, CD_BASEDIR_LINUX);
			sprintf(test, "%s\\%s", cddir, CD_EXE_LINUX);
			f = fopen(test, "r");
			if (f)
			{
				fclose(f);
				return qtrue;
			}
		}
	}

	return qfalse;
}

/*
================
Sys_CheckCD

Return true if the proper CD is in the drive
================
*/
qboolean    Sys_CheckCD(void)
{
	// FIXME: mission pack
	return qtrue;
//	return Sys_ScanForCD();
}


/*
========================================================================

LOAD/UNLOAD DLL

========================================================================
*/

/*
=================
Sys_UnloadDll

=================
*/
void Sys_UnloadDll(void* dllHandle)
{
	if (!dllHandle)
	{
		return;
	}
	if (!FreeLibrary((HMODULE)dllHandle))
	{
		Com_Error(ERR_FATAL, "Sys_UnloadDll FreeLibrary failed");
	}
}

/*
=================
Sys_LoadDll

Used to load a development dll instead of a virtual machine
=================
*/
extern char* FS_BuildOSPath(const char* base, const char* game, const char* qpath);

void* QDECL Sys_LoadDll(const char* name, qintptr(QDECL * *entryPoint) (int, ...),
	qintptr (QDECL* systemcalls)(int, ...))
{
	static int lastWarning = 0;
	HINSTANCE libHandle;
	void (QDECL* dllEntry)(qintptr (QDECL* syscallptr)(int, ...));
	const char* basepath;
	const char* cdpath;
	const char* gamedir;
	char* fn;
#ifdef NDEBUG
	int timestamp;
	int ret;
#endif
	char filename[MAX_QPATH];

#ifdef _WIN64
#ifdef WOLF_SP_DEMO
	String::Sprintf(filename, sizeof(filename), "%sx86_64_d.dll", name);
#else
	String::Sprintf(filename, sizeof(filename), "%sx86_64.dll", name);
#endif
#else
#ifdef WOLF_SP_DEMO
	String::Sprintf(filename, sizeof(filename), "%sx86_d.dll", name);
#else
	String::Sprintf(filename, sizeof(filename), "%sx86.dll", name);
#endif
#endif


#ifdef NDEBUG
	timestamp = Sys_Milliseconds();
	if (((timestamp - lastWarning) > (5 * 60000)) && !Cvar_VariableIntegerValue("dedicated") &&
		!Cvar_VariableIntegerValue("com_blindlyLoadDLLs"))
	{
		if (FS_FileExists(filename))
		{
			lastWarning = timestamp;
			ret = MessageBoxEx(NULL, "You are about to load a .DLL executable that\n"
									 "has not been verified for use with Wolfenstein.\n"
									 "This type of file can compromise the security of\n"
									 "your computer.\n\n"
									 "Select 'OK' if you choose to load it anyway.",
				"Security Warning", MB_OKCANCEL | MB_ICONEXCLAMATION | MB_DEFBUTTON2 | MB_TOPMOST | MB_SETFOREGROUND,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT));
			if (ret != IDOK)
			{
				return NULL;
			}
		}
	}
#endif

	// check current folder only if we are a developer
	if (1)		//----(SA)	always dll
	{	//	if (Cvar_VariableIntegerValue( "devdll" )) {
		libHandle = LoadLibrary(filename);
		if (libHandle)
		{
			goto found_dll;
		}
	}

	basepath = Cvar_VariableString("fs_basepath");
	cdpath = Cvar_VariableString("fs_cdpath");
	gamedir = Cvar_VariableString("fs_game");

	fn = FS_BuildOSPath(basepath, gamedir, filename);
	libHandle = LoadLibrary(fn);

	if (!libHandle)
	{
		if (cdpath[0])
		{
			fn = FS_BuildOSPath(cdpath, gamedir, filename);
			libHandle = LoadLibrary(fn);
		}

		if (!libHandle)
		{
			return NULL;
		}
	}

found_dll:

	dllEntry = (void (QDECL*)(qintptr (QDECL*)(int, ...)))GetProcAddress(libHandle, "dllEntry");
	*entryPoint = (qintptr (QDECL*)(int,...))GetProcAddress(libHandle, "vmMain");
	if (!*entryPoint || !dllEntry)
	{
		FreeLibrary(libHandle);
		return NULL;
	}
	dllEntry(systemcalls);

	return libHandle;
}

/*
========================================================================

EVENT LOOP

========================================================================
*/

byte sys_packetReceived[MAX_MSGLEN_WOLF];

/*
================
Sys_GetEvent

================
*/
sysEvent_t Sys_GetEvent(void)
{
	MSG msg;
	sysEvent_t ev;
	char* s;
	QMsg netmsg;
	netadr_t adr;

	// return if we have data
	if (eventHead > eventTail)
	{
		eventTail++;
		return eventQue[(eventTail - 1) & MASK_QUED_EVENTS];
	}

	// pump the message loop
	while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!GetMessage(&msg, NULL, 0, 0))
		{
			Com_Quit_f();
		}

		// save the msg time, because wndprocs don't have access to the timestamp
		sysMsgTime = msg.time;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// check for console commands
	s = Sys_ConsoleInput();
	if (s)
	{
		char* b;
		int len;

		len = String::Length(s) + 1;
		b = (char*)Mem_Alloc(len);
		String::NCpyZ(b, s, len - 1);
		Sys_QueEvent(0, SE_CONSOLE, 0, 0, len, b);
	}

	// check for network packets
	MSG_Init(&netmsg, sys_packetReceived, sizeof(sys_packetReceived));
	if (Sys_GetPacket(&adr, &netmsg))
	{
		netadr_t* buf;
		int len;

		// copy out to a seperate buffer for qeueing
		// the readcount stepahead is for SOCKS support
		len = sizeof(netadr_t) + netmsg.cursize - netmsg.readcount;
		buf = (netadr_t*)Mem_Alloc(len);
		*buf = adr;
		memcpy(buf + 1, &netmsg._data[netmsg.readcount], netmsg.cursize - netmsg.readcount);
		Sys_QueEvent(0, SE_PACKET, 0, 0, len, buf);
	}

	return Sys_SharedGetEvent();
}

//================================================================

/*
=================
Sys_In_Restart_f

Restart the input subsystem
=================
*/
void Sys_In_Restart_f(void)
{
	IN_Shutdown();
	IN_Init();
}


/*
=================
Sys_Net_Restart_f

Restart the network subsystem
=================
*/
void Sys_Net_Restart_f(void)
{
	NET_Restart();
}


/*
================
Sys_Init

Called after the common systems (cvars, files, etc)
are initialized
================
*/
#define OSR2_BUILD_NUMBER 1111
#define WIN98_BUILD_NUMBER 1998

void Sys_Init(void)
{
	int cpuid;

	// make sure the timer is high precision, otherwise
	// NT gets 18ms resolution
	timeBeginPeriod(1);

	Cmd_AddCommand("in_restart", Sys_In_Restart_f);
	Cmd_AddCommand("net_restart", Sys_Net_Restart_f);

	OSVERSIONINFO osversion;
	osversion.dwOSVersionInfoSize = sizeof(osversion);

	if (!GetVersionEx(&osversion))
	{
		Sys_Error("Couldn't get OS info");
	}

	if (osversion.dwMajorVersion < 5)
	{
		Sys_Error("Wolf requires Windows version 5 or greater");
	}
	if (osversion.dwPlatformId == VER_PLATFORM_WIN32s)
	{
		Sys_Error("Wolf doesn't run on Win32s");
	}

	if (osversion.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		Cvar_Set("arch", "winnt");
	}
	else
	{
		Cvar_Set("arch", "unknown Windows variant");
	}

	//
	// figure out our CPU
	//
	Cvar_Get("sys_cpustring", "detect", 0);
	if (!String::ICmp(Cvar_VariableString("sys_cpustring"), "detect"))
	{
		Com_Printf("...detecting CPU, found ");

		cpuid = Sys_GetProcessorId();

		switch (cpuid)
		{
		case CPUID_GENERIC:
			Cvar_Set("sys_cpustring", "generic");
			break;
		case CPUID_INTEL_UNSUPPORTED:
			Cvar_Set("sys_cpustring", "x86 (pre-Pentium)");
			break;
		case CPUID_INTEL_PENTIUM:
			Cvar_Set("sys_cpustring", "x86 (P5/PPro, non-MMX)");
			break;
		case CPUID_INTEL_MMX:
			Cvar_Set("sys_cpustring", "x86 (P5/Pentium2, MMX)");
			break;
		case CPUID_INTEL_KATMAI:
			Cvar_Set("sys_cpustring", "Intel Pentium III");
			break;
		case CPUID_AMD_3DNOW:
			Cvar_Set("sys_cpustring", "AMD w/ 3DNow!");
			break;
		case CPUID_AXP:
			Cvar_Set("sys_cpustring", "Alpha AXP");
			break;
		default:
			Com_Error(ERR_FATAL, "Unknown cpu type %d\n", cpuid);
			break;
		}
	}
	else
	{
		Com_Printf("...forcing CPU type to ");
		if (!String::ICmp(Cvar_VariableString("sys_cpustring"), "generic"))
		{
			cpuid = CPUID_GENERIC;
		}
		else if (!String::ICmp(Cvar_VariableString("sys_cpustring"), "x87"))
		{
			cpuid = CPUID_INTEL_PENTIUM;
		}
		else if (!String::ICmp(Cvar_VariableString("sys_cpustring"), "mmx"))
		{
			cpuid = CPUID_INTEL_MMX;
		}
		else if (!String::ICmp(Cvar_VariableString("sys_cpustring"), "3dnow"))
		{
			cpuid = CPUID_AMD_3DNOW;
		}
		else if (!String::ICmp(Cvar_VariableString("sys_cpustring"), "PentiumIII"))
		{
			cpuid = CPUID_INTEL_KATMAI;
		}
		else if (!String::ICmp(Cvar_VariableString("sys_cpustring"), "axp"))
		{
			cpuid = CPUID_AXP;
		}
		else
		{
			Com_Printf("WARNING: unknown sys_cpustring '%s'\n", Cvar_VariableString("sys_cpustring"));
			cpuid = CPUID_GENERIC;
		}
	}
	Cvar_SetValue("sys_cpuid", cpuid);
	Com_Printf("%s\n", Cvar_VariableString("sys_cpustring"));

	Cvar_Set("username", Sys_GetCurrentUser());

	IN_Init();		// FIXME: not in dedicated?
}


//=======================================================================

int totalMsec, countMsec;

/*
==================
WinMain

==================
*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	char cwd[MAX_OSPATH];
	int startTime, endTime;

	// should never get a previous instance in Win32
	if (hPrevInstance)
	{
		return 0;
	}

	global_hInstance = hInstance;
	String::NCpyZ(sys_cmdline, lpCmdLine, sizeof(sys_cmdline));

	// done before Com/Sys_Init since we need this for error output
	Sys_CreateConsole("Wolf Console");

	// no abort/retry/fail errors
	SetErrorMode(SEM_FAILCRITICALERRORS);

	// get the initial time base
	Sys_Milliseconds();

// re-enabled CD checking for proper 'setup.exe' file on game cd
// (SA) enable to do cd check for setup\setup.exe
//#if 1
#if 0
	// if we find the CD, add a +set cddir xxx command line
	if (!Sys_ScanForCD())
	{
		Sys_Error("Game CD not in drive");
	}

#endif

	Com_Init(sys_cmdline);
	NET_Init();

	_getcwd(cwd, sizeof(cwd));
	Com_Printf("Working directory: %s\n", cwd);

	// hide the early console since we've reached the point where we
	// have a working graphics subsystems
	if (!com_dedicated->integer && !com_viewlog->integer)
	{
		Sys_ShowConsole(0, qfalse);
	}

	SetFocus(GMainWindow);

	// main game loop
	while (1)
	{
		// if not running as a game client, sleep a bit
		if (Minimized || (com_dedicated && com_dedicated->integer))
		{
			Sleep(5);
		}

		// set low precision every frame, because some system calls
		// reset it arbitrarily
//		_controlfp( _PC_24, _MCW_PC );
//    _controlfp( -1, _MCW_EM  ); // no exceptions, even if some crappy
		// syscall turns them back on!

		startTime = Sys_Milliseconds();

		// make sure mouse and joystick are only called once a frame
		IN_Frame();

		// run the game
		Com_Frame();

		endTime = Sys_Milliseconds();
		totalMsec += endTime - startTime;
		countMsec++;
	}

	// never gets here
}
