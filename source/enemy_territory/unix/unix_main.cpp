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

#include <signal.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <errno.h>
#ifdef __linux__// rb010123
  #include <mntent.h>
#endif

#ifdef __linux__
  #include <fpu_control.h>	// bk001213 - force dumps on divide by zero
#endif

// FIXME TTimo should we gard this? most *nix system should comply?
#include <termios.h>

#include "../client/client.h"
#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../../common/system_unix.h"

#include "linux_local.h"// bk001204

unsigned sys_frame_time;

uid_t saved_euid;

void Sys_BeginProfiling(void)
{
}

/*
=================
Sys_In_Restart_f

Restart the input subsystem
=================
*/
#ifndef DEDICATED
void Sys_In_Restart_f(void)
{
	IN_Shutdown();
	IN_Init();
}
#endif

// =============================================================
// general sys routines
// =============================================================

#define MAX_CMD 1024
static char exit_cmdline[MAX_CMD] = "";
void Sys_DoStartProcess(const char* cmdline);

// single exit point (regular exit or in case of signal fault)
void Sys_Exit(int ex)
{
	Sys_ConsoleInputShutdown();

	// we may be exiting to spawn another process
	if (exit_cmdline[0] != '\0')
	{
		// possible race conditions?
		// buggy kernels / buggy GL driver, I don't know for sure
		// but it's safer to wait an eternity before and after the fork
		sleep(1);
		Sys_DoStartProcess(exit_cmdline);
		sleep(1);
	}

	exit(ex);
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
#elif defined __x86_64__
	Cvar_Set("arch", "linux x86_64");
#elif defined __alpha__
	Cvar_Set("arch", "linux alpha");
#elif defined __sparc__
	Cvar_Set("arch", "linux sparc");
#elif defined __FreeBSD__

#if defined __i386__// FreeBSD
	Cvar_Set("arch", "freebsd i386");
#elif defined __alpha__
	Cvar_Set("arch", "freebsd alpha");
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

void Sys_Error(const char* error, ...)
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
	Q_vsnprintf(string, sizeof(string), error, argptr);
	va_end(argptr);
	fprintf(stderr, "Sys_Error: %s\n", string);

	Sys_Exit(1);	// bk010104 - use single exit point.
}

void Sys_Warn(char* warning, ...)
{
	va_list argptr;
	char string[1024];

	va_start(argptr,warning);
	Q_vsnprintf(string, sizeof(string), warning, argptr);
	va_end(argptr);

	if (ttycon_on)
	{
		tty_Hide();
	}

	fprintf(stderr, "Warning: %s", string);

	if (ttycon_on)
	{
		tty_Show();
	}
}

/*
============
Sys_FileTime

returns -1 if not present
============
*/
int Sys_FileTime(char* path)
{
	struct  stat buf;

	if (stat(path,&buf) == -1)
	{
		return -1;
	}

	return buf.st_mtime;
}

// NOTE TTimo: huh?
void floating_point_exception_handler(int whatever)
{
	signal(SIGFPE, floating_point_exception_handler);
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
	// in vga this calls KBD_Update, under X, it calls GetEvent
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
	if (Sys_GetPacket(&adr, &netmsg))
	{
		netadr_t* buf;
		int len;

		// copy out to a seperate buffer for qeueing
		len = sizeof(netadr_t) + netmsg.cursize;
		buf = (netadr_t*)Mem_Alloc(len);
		*buf = adr;
		memcpy(buf + 1, netmsg._data, netmsg.cursize);
		Sys_QueEvent(0, SE_PACKET, 0, 0, len, buf);
	}

	return Sys_SharedGetEvent();
}

/*****************************************************************************/

qboolean Sys_CheckCD(void)
{
	return qtrue;
}

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
		Com_Printf("FPU Control 0x%x (was 0x%x)\n", fpu_word, current);
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
	fprintf(stdout, "%s %s %s\n", Q3_VERSION, CPUSTRING, __DATE__);
#ifdef DEDICATED
	fprintf(stdout, "Dedicated Server\n");
#endif
	fprintf(stdout, "\n");
}

/*
==================
chmod OR on a file
==================
*/
void Sys_Chmod(char* file, int mode)
{
	struct stat s_buf;
	int perm;
	if (stat(file, &s_buf) != 0)
	{
		Com_Printf("stat('%s')  failed: errno %d\n", file, errno);
		return;
	}
	perm = s_buf.st_mode | mode;
	if (chmod(file, perm) != 0)
	{
		Com_Printf("chmod('%s', %d) failed: errno %d\n", file, perm, errno);
	}
	Com_DPrintf("chmod +%d '%s'\n", mode, file);
}

/*
==================
Sys_DoStartProcess
actually forks and starts a process

UGLY HACK:
  Sys_StartProcess works with a command line only
  if this command line is actually a binary with command line parameters,
  the only way to do this on unix is to use a system() call
  but system doesn't replace the current process, which leads to a situation like:
  wolf.x86--spawned_process.x86
  in the case of auto-update for instance, this will cause write access denied on wolf.x86:
  wolf-beta/2002-March/000079.html
  we hack the implementation here, if there are no space in the command line, assume it's a straight process and use execl
  otherwise, use system ..
  The clean solution would be Sys_StartProcess and Sys_StartProcess_Args..
==================
*/
void Sys_DoStartProcess(const char* cmdline)
{
	switch (fork())
	{
	case -1:
		// main thread
		break;
	case 0:
		if (strchr(cmdline, ' '))
		{
			system(cmdline);
		}
		else
		{
			execl(cmdline, cmdline, NULL);
			printf("execl failed: %s\n", strerror(errno));
		}
		_exit(0);
		break;
	}
}

/*
==================
Sys_StartProcess
if !doexit, start the process asap
otherwise, push it for execution at exit
(i.e. let complete shutdown of the game and freeing of resources happen)
NOTE: might even want to add a small delay?
==================
*/
void Sys_StartProcess(const char* cmdline, qboolean doexit)
{

	if (doexit)
	{
		Com_DPrintf("Sys_StartProcess %s (delaying to final exit)\n", cmdline);
		String::NCpyZ(exit_cmdline, cmdline, MAX_CMD);
		Cbuf_ExecuteText(EXEC_APPEND, "quit\n");
		return;
	}

	Com_DPrintf("Sys_StartProcess %s\n", cmdline);
	Sys_DoStartProcess(cmdline);
}

/*
=================
Sys_OpenURL
=================
*/
void Sys_OpenURL(const char* url, qboolean doexit)
{
	const char* basepath, * homepath, * pwdpath;
	char fname[20];
	char fn[MAX_OSPATH];
	char cmdline[MAX_CMD];

	static qboolean doexit_spamguard = qfalse;

	if (doexit_spamguard)
	{
		Com_DPrintf("Sys_OpenURL: already in a doexit sequence, ignoring %s\n", url);
		return;
	}

	Com_Printf("Open URL: %s\n", url);
	// opening an URL on *nix can mean a lot of things ..
	// just spawn a script instead of deciding for the user :-)

	// do the setup before we fork
	// search for an openurl.sh script
	// search procedure taken from Sys_VM_LoadDll
	String::NCpyZ(fname, "openurl.sh", 20);

	pwdpath = Sys_Cwd();
	String::Sprintf(fn, MAX_OSPATH, "%s/%s", pwdpath, fname);
	if (access(fn, X_OK) == -1)
	{
		Com_DPrintf("%s not found\n", fn);
		// try in home path
		homepath = Cvar_VariableString("fs_homepath");
		String::Sprintf(fn, MAX_OSPATH, "%s/%s", homepath, fname);
		if (access(fn, X_OK) == -1)
		{
			Com_DPrintf("%s not found\n", fn);
			// basepath, last resort
			basepath = Cvar_VariableString("fs_basepath");
			String::Sprintf(fn, MAX_OSPATH, "%s/%s", basepath, fname);
			if (access(fn, X_OK) == -1)
			{
				Com_DPrintf("%s not found\n", fn);
				Com_Printf("Can't find script '%s' to open requested URL (use +set developer 1 for more verbosity)\n", fname);
				// we won't quit
				return;
			}
		}
	}

	// show_bug.cgi?id=612
	if (doexit)
	{
		doexit_spamguard = qtrue;
	}

	Com_DPrintf("URL script: %s\n", fn);

	// build the command line
	String::Sprintf(cmdline, MAX_CMD, "%s '%s' &", fn, url);

	Sys_StartProcess(cmdline, doexit);

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

	// go back to real user for config loads
	saved_euid = geteuid();
	seteuid(getuid());

	InitSig();

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
			strcat(cmdline, " ");
		}
		strcat(cmdline, argv[i]);
	}

	// bk000306 - clear queues
	memset(&eventQue[0], 0, MAX_QUED_EVENTS * sizeof(sysEvent_t));
	memset(&sys_packetReceived[0], 0, MAX_MSGLEN_WOLF * sizeof(byte));

	Com_Init(cmdline);
	NET_Init();

	Sys_ConsoleInputInit();

	fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) | FNDELAY);

	while (1)
	{
#ifdef __linux__
		Sys_ConfigureFPU();
#endif
		Com_Frame();

		if (com_dedicated && com_dedicated->integer && !com_sv_running->integer)
		{
			usleep(25000);
		}

	}
}

qboolean Sys_IsNumLockDown(void)
{
	// Gordon: FIXME for timothee
	return qfalse;
}
