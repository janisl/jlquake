// sys_null.h -- null system driver to aid porting efforts

#include "quakedef.h"
#include "errno.h"


/*
================
filelength
================
*/
int filelength (FILE *f)
{
	int		pos;
	int		end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_Error (char *error, ...)
{
	va_list		argptr;

	printf ("I_Error: ");	
	va_start (argptr,error);
	vprintf (error,argptr);
	va_end (argptr);
	printf ("\n");

	exit (1);
}

void Sys_Printf (char *fmt, ...)
{
	va_list		argptr;
	
	va_start (argptr,fmt);
	vprintf (fmt,argptr);
	va_end (argptr);
}

void Sys_Quit (void)
{
	exit (0);
}

double Sys_FloatTime (void)
{
	static double t;
	
	t += 0.1;
	
	return t;
}

char *Sys_ConsoleInput (void)
{
	return NULL;
}

void Sys_Sleep (void)
{
}

void Sys_SendKeyEvents (void)
{
}

//=============================================================================

void main (int argc, char **argv)
{
	quakeparms_t	parms;

	parms.memsize = 5861376;
	parms.membase = malloc (parms.memsize);
	parms.basedir = ".";

	COM_InitArgv2(argc, argv);

	parms.argc = argc;
	parms.argv = argv;

	printf ("Host_Init\n");
	Host_Init (&parms);
	while (1)
	{
		Host_Frame (0.1);
	}
}


