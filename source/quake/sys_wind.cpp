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
// sys_null.h -- null system driver to aid porting efforts

#include "quakedef.h"
#include "winquake.h"
#include "errno.h"
#include <sys\types.h>
#include <sys\timeb.h>


void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr,error);
	Q_vsnprintf(text, 1024, error, argptr);
	va_end (argptr);

//    MessageBox(NULL, text, "Error", 0 /* MB_OK */ );
	printf ("ERROR: %s\n", text);

	exit (1);
}

void Sys_Quit (void)
{
	exit (0);
}

double Sys_FloatTime (void)
{
	double t;
    struct _timeb tstruct;
	static int	starttime;

	_ftime( &tstruct );
 
	if (!starttime)
		starttime = tstruct.time;
	t = (tstruct.time-starttime) + tstruct.millitm*0.001;
	
	return t;
}

void Sys_SendKeyEvents (void)
{
}

/*
==================
main

==================
*/
char	*newargv[256];

int main (int argc, char **argv)
{
    MSG        msg;
	quakeparms_t	parms;
	double			time, oldtime;
	static	char	cwd[1024];

	Com_Memset(&parms, 0, sizeof(parms));

	parms.memsize = 16384*1024;
	parms.membase = malloc (parms.memsize);

	_getcwd (cwd, sizeof(cwd));
	if (cwd[QStr::Length(cwd)-1] == '\\')
		cwd[QStr::Length(cwd)-1] = 0;
	parms.basedir = cwd; //"f:/quake";
//	parms.basedir = "f:\\quake";

	COM_InitArgv2(argc, argv);

	// dedicated server ONLY!
	if (!COM_CheckParm ("-dedicated"))
	{
		Com_Memcpy(newargv, argv, argc*4);
		newargv[argc] = "-dedicated";
		argc++;
		argv = newargv;
		COM_InitArgv2(argc, argv);
	}

	parms.argc = argc;
	parms.argv = argv;

	printf ("Host_Init\n");
	Host_Init (&parms);

	oldtime = Sys_FloatTime ();

    /* main window message loop */
	while (1)
	{
		time = Sys_FloatTime();
		if (time - oldtime < sys_ticrate.value )
		{
			Sleep(1);
			continue;
		}

		Host_Frame ( time - oldtime );
		oldtime = time;
	}

    /* return success of application */
    return TRUE;
}

