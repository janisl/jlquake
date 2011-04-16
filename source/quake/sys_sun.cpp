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
// sys_sun.h -- Sun system driver

#include "quakedef.h"
#include "errno.h"
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>

qboolean			isDedicated;

void Sys_Error (char *error, ...)
{
    va_list         argptr;

    printf ("Sys_Error: ");   
    va_start (argptr,error);
    vprintf (error,argptr);
    va_end (argptr);
    printf ("\n");
    Host_Shutdown();
    exit (1);
}

void Sys_Printf (char *fmt, ...)
{
    va_list         argptr;
    
    va_start (argptr,fmt);
    vprintf (fmt,argptr);
    va_end (argptr);
}

void Sys_Quit (void)
{
    Host_Shutdown();
    exit (0);
}

double Sys_FloatTime (void)
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

char *Sys_ConsoleInput (void)
{
	return Sys_CommonConsoleInput();
}

void Sys_Init(void)
{
}

//=============================================================================

int main (int argc, char **argv)
{
    static quakeparms_t    parms;
    float time, oldtime, newtime;
    
    parms.memsize = 16*1024*1024;
    parms.membase = malloc (parms.memsize);
    parms.basedir = ".";

    COM_InitArgv2(argc, argv);

    parms.argc = c;
    parms.argv = v;

    printf ("Host_Init\n");
    Host_Init (&parms);

	Sys_Init();

    // unroll the simulation loop to give the video side a chance to see _vid_default_mode
    Host_Frame( 0.1 );
    VID_SetDefaultMode();

    oldtime = Sys_FloatTime();
    while (1)
    {
		newtime = Sys_FloatTime();
		Host_Frame (newtime - oldtime);
		oldtime = newtime;
    }
	return 0;
}




