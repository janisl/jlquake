/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

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

// =======================================================================
// General routines
// =======================================================================

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
	vsprintf(string,error,argptr);
	va_end(argptr);
	fprintf(stderr, "Sys_Error: %s\n", string);

	Sys_Exit(1);	// bk010104 - use single exit point.
}

void Sys_Warn(char* warning, ...)
{
	va_list argptr;
	char string[1024];

	va_start(argptr,warning);
	vsprintf(string,warning,argptr);
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
		common->Printf("stat('%s')  failed: errno %d\n", file, errno);
		return;
	}
	perm = s_buf.st_mode | mode;
	if (chmod(file, perm) != 0)
	{
		common->Printf("chmod('%s', %d) failed: errno %d\n", file, perm, errno);
	}
	common->DPrintf("chmod +%d '%s'\n", mode, file);
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
