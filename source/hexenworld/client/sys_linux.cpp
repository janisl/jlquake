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
#include <signal.h>
#include <sys/time.h>
#include <fcntl.h>

#include "quakedef.h"
#include "../../core/system_unix.h"

int noconinput = 0;
int nostdout = 0;

const char *basedir = ".";

// =======================================================================
// General routines
// =======================================================================

void Sys_Quit (void)
{
	Sys_ConsoleInputShutdown();
	Host_Shutdown();
    fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
	exit(0);
}

void Sys_Init(void)
{
}

void Sys_Error (const char *error, ...)
{ 
    va_list     argptr;
    char        string[1024];

// change stdin to non blocking
    fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);

	if (ttycon_on)
	{
		tty_Hide();
	}

    va_start (argptr,error);
    Q_vsnprintf(string, 1024, error, argptr);
    va_end (argptr);
	fprintf(stderr, "Error: %s\n", string);

	Sys_ConsoleInputShutdown();
	Host_Shutdown ();
	exit (1);

} 

void signal_handler(int sig)
{
	printf("Received signal %d, exiting...\n", sig);
	Sys_Quit();
	exit(0);
}

void InitSig(void)
{
	signal(SIGHUP, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGILL, signal_handler);
	signal(SIGTRAP, signal_handler);
	signal(SIGIOT, signal_handler);
	signal(SIGBUS, signal_handler);
	signal(SIGFPE, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGTERM, signal_handler);
}

int main (int c, char **v)
{

	double		time, oldtime, newtime;
	quakeparms_t parms;
	int j;

	InitSig(); // trap evil signals

	Com_Memset(&parms, 0, sizeof(parms));

	COM_InitArgv2(c, v);
	parms.argc = c;
	parms.argv = v;

	parms.memsize = 16*1024*1024;

	j = COM_CheckParm("-mem");
	if (j)
		parms.memsize = (int) (String::Atof(COM_Argv(j+1)) * 1024 * 1024);
	parms.membase = malloc (parms.memsize);

	parms.basedir = basedir;

	noconinput = COM_CheckParm("-noconinput");
	if (!noconinput)
		fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);

	if (COM_CheckParm("-nostdout"))
		nostdout = 1;

	Sys_Init();

    Host_Init(&parms);

	Sys_ConsoleInputInit();

    oldtime = Sys_DoubleTime ();
    while (1)
    {
// find time spent rendering last frame
        newtime = Sys_DoubleTime ();
        time = newtime - oldtime;

		Host_Frame(time);
		oldtime = newtime;
    }

}
