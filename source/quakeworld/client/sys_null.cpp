// sys_null.h -- null system driver to aid porting efforts

#include "quakedef.h"
#include "errno.h"

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

void Sys_Quit (void)
{
	exit (0);
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


