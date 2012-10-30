// common.c -- misc functions used in client and server

/*
 * $Header: /H2 Mission Pack/COMMON.C 15    3/19/98 4:28p Jmonroe $
 */

#include "quakedef.h"
#include "../client/public.h"
#include "../apps/main.h"

/*
================
COM_Init
================
*/
void COM_Init()
{
	Com_InitByteOrder();

	qh_registered = Cvar_Get("registered", "0", 0);

	FS_InitFilesystem();
	COMQH_CheckRegistered();
}

#ifndef _WIn32
static double oldtime;

void Com_SharedInit(int argc, char* argv[], char* cmdline)
{
	quakeparms_t parms;
	Com_Memset(&parms, 0, sizeof(parms));

	COM_InitArgv2(argc, argv);
	parms.argc = argc;
	parms.argv = argv;

	Host_Init(&parms);

	oldtime = Sys_DoubleTime() - 0.1;
}

void Com_SharedFrame()
{
	// find time spent rendering last frame
	double newtime = Sys_DoubleTime();
	double time = newtime - oldtime;

	if (com_dedicated->integer)
	{	// play vcrfiles at max speed
		if (time < sys_ticrate->value)
		{
			Sys_Sleep(1);
			return;		// not time to run a server only tic yet
		}
		time = sys_ticrate->value;
	}

	if (time > sys_ticrate->value * 2)
	{
		oldtime = newtime;
	}
	else
	{
		oldtime += time;
	}

	Host_Frame(time);
}
#endif
