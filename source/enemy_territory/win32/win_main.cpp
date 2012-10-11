/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

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

static char sys_cmdline[MAX_STRING_CHARS];

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
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	Sys_Print(text);
	Sys_Print("\n");

	Sys_SetErrorText(text);
	Sys_ShowConsole(1, true);

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
	netmsg.Init(sys_packetReceived, sizeof(sys_packetReceived));
	if (NET_GetUdpPacket(NS_SERVER, &adr, &netmsg))
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
	Cmd_AddCommand("clearviewlog", Sys_ClearViewlog_f);

	OSVERSIONINFO osversion;
	osversion.dwOSVersionInfoSize = sizeof(osversion);

	if (!GetVersionEx(&osversion))
	{
		Sys_Error("Couldn't get OS info");
	}

	if (osversion.dwMajorVersion < 5)
	{
		Sys_Error("JLQuake requires Windows version 4 or greater");
	}
	if (osversion.dwPlatformId == VER_PLATFORM_WIN32s)
	{
		Sys_Error("JLQuake doesn't run on Win32s");
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
		common->Printf("...detecting CPU, found ");

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
			common->FatalError("Unknown cpu type %d\n", cpuid);
			break;
		}
	}
	else
	{
		common->Printf("...forcing CPU type to ");
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
			common->Printf("WARNING: unknown sys_cpustring '%s'\n", Cvar_VariableString("sys_cpustring"));
			cpuid = CPUID_GENERIC;
		}
	}
	Cvar_SetValue("sys_cpuid", cpuid);
	common->Printf("%s\n", Cvar_VariableString("sys_cpustring"));

	Cvar_Set("username", Sys_GetCurrentUser());

//	IN_Init();		// FIXME: not in dedicated?
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
	Sys_CreateConsole("ET Console");

	// no abort/retry/fail errors
	SetErrorMode(SEM_FAILCRITICALERRORS);

	// get the initial time base
	Sys_Milliseconds();

	Com_Init(sys_cmdline);
	NETQ23_Init();

#ifndef DEDICATED
	IN_Init();	// fretn - directinput must be inited after video etc
#endif

	_getcwd(cwd, sizeof(cwd));
	common->Printf("Working directory: %s\n", cwd);

	// hide the early console since we've reached the point where we
	// have a working graphics subsystems
	if (!com_dedicated->integer && !com_viewlog->integer)
	{
		Sys_ShowConsole(0, false);
	}

#ifndef DEDICATED
	SetFocus(GMainWindow);
#endif

	// main game loop
	while (1)
	{
		// if not running as a game client, sleep a bit
#ifndef DEDICATED
		if (Minimized || (com_dedicated && com_dedicated->integer))
#endif
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

		Com_Frame();

		endTime = Sys_Milliseconds();
		totalMsec += endTime - startTime;
		countMsec++;
	}

	// never gets here
}
