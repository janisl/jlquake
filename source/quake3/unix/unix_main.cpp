/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#ifdef __linux__// rb010123
  #include <mntent.h>
#endif

#ifdef __linux__
  #include <fpu_control.h>	// bk001213 - force dumps on divide by zero
#endif

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#ifndef DEDICATED
#include "../../client/client.h"
#endif

#include "linux_local.h"// bk001204
#include "../../common/system_unix.h"

// =======================================================================
// General routines
// =======================================================================

void Sys_BeginProfiling(void)
{
}

#ifndef DEDICATED
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
#endif

// =============================================================
// general sys routines
// =============================================================

// single exit point (regular exit or in case of signal fault)
void Sys_Exit(int ex)
{
	Sys_ConsoleInputShutdown();

#ifdef NDEBUG	// regular behavior

	// We can't do this
	//  as long as GL DLL's keep installing with atexit...
	//exit(ex);
	_exit(ex);
#else

	// Give me a backtrace on error exits.
	assert(ex == 0);
	exit(ex);
#endif
}


void Sys_Quit(void)
{
	CL_Shutdown();
	fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) & ~FNDELAY);
	Sys_Exit(0);
}

void Sys_Init(void)
{
#ifndef DEDICATED
	Cmd_AddCommand("in_restart", Sys_In_Restart_f);
#endif

#if defined __linux__
#if defined __i386__
	Cvar_Set("arch", "linux i386");
#elif defined __alpha__
	Cvar_Set("arch", "linux alpha");
#elif defined __sparc__
	Cvar_Set("arch", "linux sparc");
#elif defined __x86_64__
	Cvar_Set("arch", "linux x86_86");
#elif defined __FreeBSD__

#if defined __i386__// FreeBSD
	Cvar_Set("arch", "freebsd i386");
#elif defined __alpha__
	Cvar_Set("arch", "freebsd alpha");
#elif defined __x86_64__
	Cvar_Set("arch", "freebsd x86_86");
#else
	Cvar_Set("arch", "freebsd unknown");
#endif	// FreeBSD

#else
	Cvar_Set("arch", "linux unknown");
#endif
#elif defined __sun__
#if defined __i386__
	Cvar_Set("arch", "solaris x86");
#elif defined __sparc__
	Cvar_Set("arch", "solaris sparc");
#elif defined __x86_64__
	Cvar_Set("arch", "solaris x86_86");
#else
	Cvar_Set("arch", "solaris unknown");
#endif
#elif defined __sgi__
#if defined __mips__
	Cvar_Set("arch", "sgi mips");
#else
	Cvar_Set("arch", "sgi unknown");
#endif
#else
	Cvar_Set("arch", "unknown");
#endif

	Cvar_Set("username", Sys_GetCurrentUser());

#ifndef DEDICATED
	IN_Init();
#endif
}

void  Sys_Error(const char* error, ...)
{
	va_list argptr;
	char string[1024];

	// change stdin to non blocking
	// NOTE TTimo not sure how well that goes with tty console mode
	fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) & ~FNDELAY);

	// don't bother do a show on this one heh
	if (ttycon_on)
	{
		tty_Hide();
	}

	CL_Shutdown();

	va_start(argptr,error);
	Q_vsnprintf(string, 1024, error, argptr);
	va_end(argptr);
	fprintf(stderr, "Sys_Error: %s\n", string);

	Sys_Exit(1);// bk010104 - use single exit point.
}

/*
========================================================================

EVENT LOOP

========================================================================
*/

byte sys_packetReceived[MAX_MSGLEN_Q3];

/*
================
Sys_GetEvent

================
*/
sysEvent_t Sys_GetEvent(void)
{
	char* s;
	QMsg netmsg;
	netadr_t adr;

	// return if we have data
	if (eventHead > eventTail)
	{
		eventTail++;
		return eventQue[(eventTail - 1) & MASK_QUED_EVENTS];
	}

#ifndef DEDICATED
	// pump the message loop
	Sys_SendKeyEvents();
#endif

	// check for console commands
	s = Sys_ConsoleInput();
	if (s)
	{
		char* b;
		int len;

		len = String::Length(s) + 1;
		b = (char*)Mem_Alloc(len);
		String::Cpy(b, s);
		Sys_QueEvent(0, SE_CONSOLE, 0, 0, len, b);
	}

#ifndef DEDICATED
	// check for other input devices
	IN_Frame();
#endif

	// check for network packets
	netmsg.Init(sys_packetReceived, sizeof(sys_packetReceived));
	if (NET_GetUdpPacket(NS_SERVER, &adr, &netmsg))
	{
		netadr_t* buf;
		int len;

		// copy out to a seperate buffer for qeueing
		len = sizeof(netadr_t) + netmsg.cursize;
		buf = (netadr_t*)Mem_Alloc(len);
		*buf = adr;
		Com_Memcpy(buf + 1, netmsg._data, netmsg.cursize);
		Sys_QueEvent(0, SE_PACKET, 0, 0, len, buf);
	}

	return Sys_SharedGetEvent();
}

/*****************************************************************************/

void Sys_AppActivate(void)
{
}

void    Sys_ConfigureFPU()	// bk001213 - divide by zero
{
#ifdef __linux__
#ifdef __i386
#ifndef NDEBUG

	// bk0101022 - enable FPE's in debug mode
	static int fpu_word = _FPU_DEFAULT & ~(_FPU_MASK_ZM | _FPU_MASK_IM);
	int current = 0;
	_FPU_GETCW(current);
	if (current != fpu_word)
	{
#if 0
		common->Printf("FPU Control 0x%x (was 0x%x)\n", fpu_word, current);
		_FPU_SETCW(fpu_word);
		_FPU_GETCW(current);
		assert(fpu_word == current);
#endif
	}
#else	// NDEBUG
	static int fpu_word = _FPU_DEFAULT;
	_FPU_SETCW(fpu_word);
#endif	// NDEBUG
#endif	// __i386
#endif	// __linux
}

void Sys_PrintBinVersion(const char* name)
{
	const char* date = __DATE__;
	const char* time = __TIME__;
	const char* sep = "==============================================================";
	fprintf(stdout, "\n\n%s\n", sep);
#ifdef DEDICATED
	fprintf(stdout, "Linux Quake3 Dedicated Server [%s %s]\n", date, time);
#else
	fprintf(stdout, "Linux Quake3 Full Executable  [%s %s]\n", date, time);
#endif
	fprintf(stdout, " local install: %s\n", name);
	fprintf(stdout, "%s\n\n", sep);
}

void Sys_ParseArgs(int argc, char* argv[])
{

	if (argc == 2)
	{
		if ((!String::Cmp(argv[1], "--version")) ||
			(!String::Cmp(argv[1], "-v")))
		{
			Sys_PrintBinVersion(argv[0]);
			Sys_Exit(0);
		}
	}
}

int main(int argc, char* argv[])
{
	// int  oldtime, newtime; // bk001204 - unused
	int len, i;
	char* cmdline;

	InitSig();

	// go back to real user for config loads
	seteuid(getuid());

	Sys_ParseArgs(argc, argv);	// bk010104 - added this for support

	// merge the command line, this is kinda silly
	for (len = 1, i = 1; i < argc; i++)
		len += String::Length(argv[i]) + 1;
	cmdline = (char*)malloc(len);
	*cmdline = 0;
	for (i = 1; i < argc; i++)
	{
		if (i > 1)
		{
			String::Cat(cmdline, len, " ");
		}
		String::Cat(cmdline, len, argv[i]);
	}

	// bk000306 - clear queues
	Com_Memset(&eventQue[0], 0, MAX_QUED_EVENTS * sizeof(sysEvent_t));
	Com_Memset(&sys_packetReceived[0], 0, MAX_MSGLEN_Q3 * sizeof(byte));

	Com_Init(cmdline);
	NETQ23_Init();

	Sys_ConsoleInputInit();

	fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) | FNDELAY);

	while (1)
	{
#ifdef __linux__
		Sys_ConfigureFPU();
#endif
		Com_Frame();
	}
}
