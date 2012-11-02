/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// common.c -- misc functions used in client and server

#include "../../common/qcommon.h"
#include "../../client/public.h"
#include "../../server/public.h"
#include "../../apps/main.h"

#define Q3_VERSION      "Q3 1.32b"
// 1.32 released 7-10-2002

void Com_SharedInit(int argc, char* argv[], char* commandLine)
{
	// get the initial time base
	Sys_Milliseconds();

	char* s;

	common->Printf("%s %s %s\n", Q3_VERSION, CPUSTRING, __DATE__);

	if (setjmp(abortframe))
	{
		Sys_Error("Error during initialization");
	}

	GGameType = GAME_Quake3;
	Sys_SetHomePathSuffix("jlquake3");

	Com_InitByteOrder();

	// bk001129 - do this before anything else decides to push events
	Com_InitEventQueue();

	Cvar_Init();

	// prepare enough of the subsystems to handle
	// cvar and command buffer management
	Com_ParseCommandLine(commandLine);

	Cbuf_Init();

	Cmd_Init();

	// override anything from the config files with command line args
	Com_StartupVariable(NULL);

	// get the developer cvar set as early as possible
	Com_StartupVariable("developer");

	// done early so bind command exists
	CL_InitKeyCommands();

	FS_InitFilesystem();

	Com_InitJournaling();

	Cbuf_AddText("exec default.cfg\n");

	// skip the q3config.cfg if "safe" is on the command line
	if (!Com_SafeMode())
	{
		Cbuf_AddText("exec q3config.cfg\n");
	}

	Cbuf_AddText("exec autoexec.cfg\n");

	Cbuf_Execute();

	// override anything from the config files with command line args
	Com_StartupVariable(NULL);

	// get dedicated here for proper hunk megs initialization
#ifdef DEDICATED
	com_dedicated = Cvar_Get("dedicated", "1", CVAR_ROM);
#else
	com_dedicated = Cvar_Get("dedicated", "0", CVAR_LATCH2);
#endif

	// if any archived cvars are modified after this, we will trigger a writing
	// of the config file
	cvar_modifiedFlags &= ~CVAR_ARCHIVE;

	//
	// init commands and vars
	//
	COM_InitCommonCvars();

	com_maxfps = Cvar_Get("com_maxfps", "85", CVAR_ARCHIVE);

	com_fixedtime = Cvar_Get("fixedtime", "0", CVAR_CHEAT);
	com_showtrace = Cvar_Get("com_showtrace", "0", CVAR_CHEAT);
	com_dropsim = Cvar_Get("com_dropsim", "0", CVAR_CHEAT);
	com_speeds = Cvar_Get("com_speeds", "0", 0);
	comt3_timedemo = Cvar_Get("timedemo", "0", CVAR_CHEAT);
	comq3_cameraMode = Cvar_Get("com_cameraMode", "0", CVAR_CHEAT);

	cl_paused = Cvar_Get("cl_paused", "0", CVAR_ROM);
	sv_paused = Cvar_Get("sv_paused", "0", CVAR_ROM);
	com_sv_running = Cvar_Get("sv_running", "0", CVAR_ROM);
	com_cl_running = Cvar_Get("cl_running", "0", CVAR_ROM);

	com_watchdog = Cvar_Get("com_watchdog", "60", CVAR_ARCHIVE);
	com_watchdog_cmd = Cvar_Get("com_watchdog_cmd", "", CVAR_ARCHIVE);

	if (com_dedicated->integer)
	{
		if (!com_viewlog->integer)
		{
			Cvar_Set("viewlog", "1");
		}
	}

	if (com_developer && com_developer->integer)
	{
		Cmd_AddCommand("error", Com_Error_f);
		Cmd_AddCommand("crash", Com_Crash_f);
		Cmd_AddCommand("freeze", Com_Freeze_f);
	}
	COM_InitCommonCommands();

	s = va("%s %s %s", Q3_VERSION, CPUSTRING, __DATE__);
	comt3_version = Cvar_Get("version", s, CVAR_ROM | CVAR_SERVERINFO);

	Sys_Init();
	Netchan_Init(Com_Milliseconds() & 0xffff);	// pick a port value that should be nice and random
	VM_Init();
	SV_Init();

	com_dedicated->modified = false;
	if (!com_dedicated->integer)
	{
		CL_Init();
		Sys_ShowConsole(com_viewlog->integer, false);
	}

	// set com_frameTime so that if a map is started on the
	// command line it will still be able to count on com_frameTime
	// being random enough for a serverid
	com_frameTime = Com_Milliseconds();

	// add + commands from command line
	if (!Com_AddStartupCommands())
	{
		// if the user didn't give any commands, run default action
		if (!com_dedicated->integer)
		{
			Cbuf_AddText("cinematic idlogo.RoQ\n");
			Cvar* com_introPlayed = Cvar_Get("com_introplayed", "0", CVAR_ARCHIVE);
			if (!com_introPlayed->integer)
			{
				Cvar_Set(com_introPlayed->name, "1");
				Cvar_Set("nextmap", "cinematic intro.RoQ");
			}
		}
	}

	// start in full screen ui mode
	Cvar_Set("r_uiFullScreen", "1");

	CL_StartHunkUsers();

	// make sure single player is off by default
	Cvar_Set("ui_singlePlayerActive", "0");

	NETQ23_Init();

	common->Printf("Working directory: %s\n", Sys_Cwd());

	fs_ProtectKeyFile = true;
	com_fullyInitialized = true;
	common->Printf("--- Common Initialization Complete ---\n");
}
