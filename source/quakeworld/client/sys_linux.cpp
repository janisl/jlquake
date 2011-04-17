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
#include <signal.h>
#include <sys/time.h>
#include <fcntl.h>

#include "quakedef.h"
#include "../../core/system_unix.h"

int noconinput = 0;
int nostdout = 0;

char *basedir = ".";

// =======================================================================
// General routines
// =======================================================================

void Sys_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		text[4 * 2048];
	unsigned char		*p;

	va_start (argptr,fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end (argptr);

    if (nostdout)
        return;

	if (ttycon_on)
	{
		tty_Hide();
	}
	for (p = (unsigned char *)text; *p; p++)
		if ((*p > 128 || *p < 32) && *p != 10 && *p != 13 && *p != 9)
			printf("[%02x]", *p);
		else
			putc(*p, stdout);
	if (ttycon_on)
	{
		tty_Show();
	}
}

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

void Sys_Error (char *error, ...)
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

double Sys_DoubleTime (void)
{
    struct timeval tp;
    struct timezone tzp; 
    static int      secbase; 
    
    gettimeofday(&tp, &tzp);  

    if (!secbase)
    {
        secbase = tp.tv_sec;
        return tp.tv_usec/1000000.0;
    }

    return (tp.tv_sec - secbase) + tp.tv_usec/1000000.0;
}

int main (int c, char **v)
{

	double		time, oldtime, newtime;
	quakeparms_t parms;
	int j;

	signal(SIGFPE, SIG_IGN);

	Com_Memset(&parms, 0, sizeof(parms));

	COM_InitArgv2(c, v);
	parms.argc = c;
	parms.argv = v;

	parms.memsize = 16*1024*1024;

	j = COM_CheckParm("-mem");
	if (j)
		parms.memsize = (int) (QStr::Atof(COM_Argv(j+1)) * 1024 * 1024);
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
