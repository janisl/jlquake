/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include <sys/types.h>
#include "qwsvdef.h"
#include "../../common/system_unix.h"

#ifdef NeXT
#include <libc.h>
#endif

#if defined(__linux__) || defined(sun)
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#else
#include <sys/dir.h>
#endif
#include <fcntl.h>

Cvar* sys_nostdout;
Cvar* sys_extrasleep;

/*
===============================================================================

                REQUIRED SYS FUNCTIONS

===============================================================================
*/

/*
================
Sys_Error
================
*/
void Sys_Error(const char* error, ...)
{
	va_list argptr;
	char string[1024];

	fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) & ~FNDELAY);
	if (ttycon_on)
	{
		tty_Hide();
	}

	va_start(argptr,error);
	Q_vsnprintf(string, 1024, error, argptr);
	va_end(argptr);
	printf("Fatal error: %s\n",string);

	Sys_Exit(1);
}

/*
================
Sys_Quit
================
*/
void Sys_Quit(void)
{
	fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) & ~FNDELAY);
	Sys_Exit(0);		// appkit isn't running
}

/*
=============
Sys_Init

Quake calls this so the system can register variables before host_hunklevel
is marked
=============
*/
void Sys_Init(void)
{
	sys_nostdout = Cvar_Get("sys_nostdout", "0", 0);
	sys_extrasleep = Cvar_Get("sys_extrasleep","0", 0);
}

/*
=============
main
=============
*/
int main(int argc, char* argv[])
{
	double time, oldtime, newtime;
	quakeparms_t parms;

	Com_Memset(&parms, 0, sizeof(parms));

	COM_InitArgv2(argc, argv);
	parms.argc = argc;
	parms.argv = argv;
	parms.basedir = ".";

	COM_InitServer(&parms);

// run one frame immediately for first heartbeat
	COM_ServerFrame(0.1);

	Sys_ConsoleInputInit();

	fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) | FNDELAY);

//
// main loop
//
	oldtime = Sys_DoubleTime() - 0.1;
	while (1)
	{
		// select on the net socket and stdin
		// the only reason we have a timeout at all is so that if the last
		// connected client times out, the message would not otherwise
		// be printed until the next event.
		if (!SOCK_Sleep(ip_sockets[0], 1000))
		{
			continue;
		}

		// find time passed since last cycle
		newtime = Sys_DoubleTime();
		time = newtime - oldtime;
		oldtime = newtime;

		COM_ServerFrame(time);

		// extrasleep is just a way to generate a fucked up connection on purpose
		if (sys_extrasleep->value)
		{
			usleep(sys_extrasleep->value);
		}
	}
	return 0;
}
