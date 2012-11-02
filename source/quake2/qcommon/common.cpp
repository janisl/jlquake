/*
Copyright (C) 1997-2001 Id Software, Inc.

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
// common.c -- misc functions used in client and server
#include "../../common/qcommon.h"
#include "../../client/public.h"
#include "../../server/public.h"
#include "../../apps/main.h"

#define VERSION     3.19

static int oldtime;

Cvar* timescale;
Cvar* fixedtime;
Cvar* showtrace;

/*
=============
Com_Error_f

Just throw a fatal error to
test error shutdown procedures
=============
*/
void Com_Error_f(void)
{
	common->FatalError("%s", Cmd_Argv(1));
}

void Com_SharedInit(int argc, char* argv[], char* cmdline)
{
	char* s;

	if (setjmp(abortframe))
	{
		Sys_Error("Error during initialization");
	}

	GGameType = GAME_Quake2;
	Sys_SetHomePathSuffix("jlquake2");

	// prepare enough of the subsystems to handle
	// cvar and command buffer management
	COM_InitArgv(argc, argv);

	Com_InitByteOrder();
	Cbuf_Init();

	Cmd_Init();
	Cvar_Init();

	CL_InitKeyCommands();

	// we need to add the early commands twice, because
	// a basedir or cddir needs to be set before execing
	// config files, but we want other parms to override
	// the settings of the config files
	Cbuf_AddEarlyCommands(false);
	Cbuf_Execute();

	FS_InitFilesystem();

	Cbuf_AddText("exec default.cfg\n");
	Cbuf_AddText("exec config.cfg\n");

	Cbuf_AddEarlyCommands(true);
	Cbuf_Execute();

	//
	// init commands and vars
	//
	Cmd_AddCommand("error", Com_Error_f);

	COM_InitCommonCvars();

	com_speeds = Cvar_Get("host_speeds", "0", 0);
	timescale = Cvar_Get("timescale", "1", 0);
	fixedtime = Cvar_Get("fixedtime", "0", 0);
	showtrace = Cvar_Get("showtrace", "0", 0);
#ifdef DEDICATED
	com_dedicated = Cvar_Get("dedicated", "1", CVAR_INIT);
#else
	com_dedicated = Cvar_Get("dedicated", "0", CVAR_INIT);
#endif
	com_maxfps = Cvar_Get("com_maxfps", "0", CVAR_ARCHIVE);

	s = va("%4.2f %s %s", VERSION, CPUSTRING, __DATE__);
	Cvar_Get("version", s, CVAR_SERVERINFO | CVAR_INIT);


	COM_InitCommonCommands();

	Sys_Init();

	NETQ23_Init();
	// pick a port value that should be nice and random
	Netchan_Init(Sys_Milliseconds());

	SV_Init();
	if (!com_dedicated->value)
	{
		CL_Init();
		CL_StartHunkUsers();
	}

	// add + commands from command line
	if (!Cbuf_AddLateCommands())
	{	// if the user didn't give any commands, run default action
		if (!com_dedicated->value)
		{
			Cbuf_AddText("d1\n");
		}
		else
		{
			Cbuf_AddText("dedicated_start\n");
		}
		Cbuf_Execute();
	}
	else
	{	// the user asked for something explicit
		// so drop the loading plaque
		SCR_EndLoadingPlaque();
	}

	oldtime = Sys_Milliseconds();

	com_fullyInitialized = true;
	common->Printf("====== Quake2 Initialized ======\n\n");
}

void Com_SharedFrame()
{
	// find time spent rendering last frame
	int newtime;
	int msec;
	do
	{
		newtime = Sys_Milliseconds();
		msec = newtime - oldtime;
	}
	while (msec < 1);

	int time_before, time_between, time_after;

	if (setjmp(abortframe))
	{
		return;		// an ERR_DROP was thrown
	}

	if (fixedtime->value)
	{
		msec = fixedtime->value;
	}
	else if (timescale->value)
	{
		msec *= timescale->value;
		if (msec < 1)
		{
			msec = 1;
		}
	}

	if (showtrace->value)
	{
		common->Printf("%4i traces  %4i points\n", c_traces, c_pointcontents);
		c_traces = 0;
		c_brush_traces = 0;
		c_pointcontents = 0;
	}

	Com_EventLoop();
	Cbuf_Execute();

	if (com_speeds->value)
	{
		time_before = Sys_Milliseconds();
	}

	SV_Frame(msec);

	if (com_speeds->value)
	{
		time_between = Sys_Milliseconds();
	}

	// let the mouse activate or deactivate
	IN_Frame();

	// get new key events
	Com_EventLoop();

	// process console commands
	Cbuf_Execute();

	CL_Frame(msec);

	if (com_speeds->value)
	{
		time_after = Sys_Milliseconds();
	}


	if (com_speeds->value)
	{
		int all, sv, gm, cl, rf;

		all = time_after - time_before;
		sv = time_between - time_before;
		cl = time_after - time_between;
		gm = time_after_game - time_before_game;
		rf = time_after_ref - time_before_ref;
		sv -= gm;
		cl -= rf;
		common->Printf("all:%3i sv:%3i gm:%3i cl:%3i rf:%3i\n",
			all, sv, gm, cl, rf);
	}

	oldtime = newtime;
}
