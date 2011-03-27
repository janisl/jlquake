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
#include <sys/types.h>
#include "qwsvdef.h"

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

QCvar*	sys_nostdout;
QCvar*	sys_extrasleep;

qboolean	stdin_ready;

/*
===============================================================================

				REQUIRED SYS FUNCTIONS

===============================================================================
*/

/*
================
Sys_DoubleTime
================
*/
double Sys_DoubleTime (void)
{
	struct timeval tp;
	struct timezone tzp;
	static int		secbase;

	gettimeofday(&tp, &tzp);
	
	if (!secbase)
	{
		secbase = tp.tv_sec;
		return tp.tv_usec/1000000.0;
	}
	
	return (tp.tv_sec - secbase) + tp.tv_usec/1000000.0;
}

/*
================
Sys_Error
================
*/
void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		string[1024];
	
	va_start (argptr,error);
	Q_vsnprintf(string, 1024, error, argptr);
	va_end (argptr);
	printf ("Fatal error: %s\n",string);
	
	exit (1);
}

/*
================
Sys_Printf
================
*/
void Sys_Printf (char *fmt, ...)
{
	va_list		argptr;
	static char		text[2048];
	unsigned char		*p;

	va_start (argptr,fmt);
	Q_vsnprintf(text, 2048, fmt, argptr);
	va_end (argptr);

	if (QStr::Length(text) > sizeof(text))
		Sys_Error("memory overwrite in Sys_Printf");

	if (sys_nostdout && sys_nostdout->value)
		return;

	for (p = (unsigned char *)text; *p; p++) {
		*p &= 0x7f;
		if ((*p > 128 || *p < 32) && *p != 10 && *p != 13 && *p != 9)
			printf("[%02x]", *p);
		else
			putc(*p, stdout);
	}
	fflush(stdout);
}


/*
================
Sys_Quit
================
*/
void Sys_Quit (void)
{
	exit (0);		// appkit isn't running
}

static int do_stdin = 1;

/*
================
Sys_ConsoleInput

Checks for a complete line of text typed in at the console, then forwards
it to the host command processor
================
*/
char *Sys_ConsoleInput (void)
{
	static char	text[256];
	int		len;

	if (!stdin_ready || !do_stdin)
		return NULL;		// the select didn't say it was ready
	stdin_ready = false;

	len = read (0, text, sizeof(text));
	if (len == 0) {
		// end of file
		do_stdin = 0;
		return NULL;
	}
	if (len < 1)
		return NULL;
	text[len-1] = 0;	// rip off the /n and terminate
	
	return text;
}

/*
=============
Sys_Init

Quake calls this so the system can register variables before host_hunklevel
is marked
=============
*/
void Sys_Init (void)
{
	sys_nostdout = Cvar_Get("sys_nostdout", "0", 0);
	sys_extrasleep = Cvar_Get("sys_extrasleep","0", 0);
}

/*
=============
main
=============
*/
int main(int argc, char *argv[])
{
	double			time, oldtime, newtime;
	quakeparms_t	parms;
	fd_set	fdset;
	extern	int		net_socket;
    struct timeval timeout;
	int j;

	Com_Memset(&parms, 0, sizeof(parms));

	COM_InitArgv2(argc, argv);	
	parms.argc = argc;
	parms.argv = argv;

	parms.memsize = 16*1024*1024;

	j = COM_CheckParm("-mem");
	if (j)
		parms.memsize = (int) (QStr::Atof(COM_Argv(j+1)) * 1024 * 1024);
	if ((parms.membase = malloc (parms.memsize)) == NULL)
		Sys_Error("Can't allocate %ld\n", parms.memsize);

	parms.basedir = ".";

	SV_Init (&parms);

// run one frame immediately for first heartbeat
	SV_Frame (0.1);		

//
// main loop
//
	oldtime = Sys_DoubleTime () - 0.1;
	while (1)
	{
	// select on the net socket and stdin
	// the only reason we have a timeout at all is so that if the last
	// connected client times out, the message would not otherwise
	// be printed until the next event.
		FD_ZERO(&fdset);
		if (do_stdin)
			FD_SET(0, &fdset);
		FD_SET(net_socket, &fdset);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if (select (net_socket+1, &fdset, NULL, NULL, &timeout) == -1)
			continue;
		stdin_ready = FD_ISSET(0, &fdset);

	// find time passed since last cycle
		newtime = Sys_DoubleTime ();
		time = newtime - oldtime;
		oldtime = newtime;
		
		SV_Frame (time);		
		
	// extrasleep is just a way to generate a fucked up connection on purpose
		if (sys_extrasleep->value)
			usleep (sys_extrasleep->value);
	}
	return 0;
}
