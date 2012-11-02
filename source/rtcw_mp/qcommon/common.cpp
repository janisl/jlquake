/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// common.c -- misc functions used in client and server

#include "../../common/qcommon.h"
#include "../../client/public.h"
#include "../../server/public.h"
#include "../../apps/main.h"

#define Q3_VERSION      "Wolf 1.41b-MP"
// 1.41b-MP: fix autodl sploit
// 1.4-MP : (== 1.34)
// 1.3-MP : final for release
// 1.1b - TTimo SP linux release (+ MP updates)
// 1.1b5 - Mac update merge in

FILE* debuglogfile;

Cvar* com_fixedtime;
Cvar* com_timedemo;
Cvar* com_showtrace;
Cvar* com_blood;
Cvar* com_introPlayed;
Cvar* com_cameraMode;
Cvar* com_recommendedSet;

// Rafael Notebook
Cvar* cl_notebook;

Cvar* com_hunkused;			// Ridah

int com_frameMsec;
int com_frameNumber;

/*
=============
Com_Error_f

Just throw a fatal error to
test error shutdown procedures
=============
*/
static void Com_Error_f(void)
{
	if (Cmd_Argc() > 1)
	{
		common->Error("Testing drop error");
	}
	else
	{
		common->FatalError("Testing fatal error");
	}
}


/*
=============
Com_Freeze_f

Just freeze in place for a given number of seconds to test
error recovery
=============
*/
static void Com_Freeze_f(void)
{
	float s;
	int start, now;

	if (Cmd_Argc() != 2)
	{
		common->Printf("freeze <seconds>\n");
		return;
	}
	s = String::Atof(Cmd_Argv(1));

	start = Com_Milliseconds();

	while (1)
	{
		now = Com_Milliseconds();
		if ((now - start) * 0.001 > s)
		{
			break;
		}
	}
}

/*
=================
Com_Crash_f

A way to force a bus error for development reasons
=================
*/
static void Com_Crash_f(void)
{
	*(int*)0 = 0x12345678;
}

/*
=================
Com_Init
=================
*/
void Com_SharedInit(int argc, char* argv[], char* commandLine)
{
	// get the initial time base
	Sys_Milliseconds();

	char* s;
	// TTimo gcc warning: variable `safeMode' might be clobbered by `longjmp' or `vfork'
	volatile qboolean safeMode = true;

	common->Printf("%s %s %s\n", Q3_VERSION, CPUSTRING, __DATE__);

	if (setjmp(abortframe))
	{
		Sys_Error("Error during initialization");
	}

	GGameType = GAME_WolfMP;
	Sys_SetHomePathSuffix("jlwolf");

	// bk001129 - do this before anything else decides to push events
	Com_InitEventQueue();

	Cvar_Init();

	// prepare enough of the subsystems to handle
	// cvar and command buffer management
	Com_ParseCommandLine(commandLine);

	Com_InitByteOrder();
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
	Cbuf_AddText("exec language.cfg\n");	// NERVE - SMF

	// skip the q3config.cfg if "safe" is on the command line
	if (!Com_SafeMode())
	{
		safeMode = false;
		Cbuf_AddText("exec wolfconfig_mp.cfg\n");
	}

	Cbuf_AddText("exec autoexec.cfg\n");

	Cbuf_Execute();

	// override anything from the config files with command line args
	Com_StartupVariable(NULL);

	// get dedicated here for proper hunk megs initialization
#if DEDICATED
	// TTimo: default to internet dedicated, not LAN dedicated
	com_dedicated = Cvar_Get("dedicated", "2", CVAR_ROM);
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
	com_maxfps = Cvar_Get("com_maxfps", "85", CVAR_ARCHIVE | CVAR_LATCH2);
	com_blood = Cvar_Get("com_blood", "1", CVAR_ARCHIVE);

	com_fixedtime = Cvar_Get("fixedtime", "0", CVAR_CHEAT);
	com_showtrace = Cvar_Get("com_showtrace", "0", CVAR_CHEAT);
	com_dropsim = Cvar_Get("com_dropsim", "0", CVAR_CHEAT);
	com_speeds = Cvar_Get("com_speeds", "0", 0);
	com_timedemo = Cvar_Get("timedemo", "0", CVAR_CHEAT);
	com_cameraMode = Cvar_Get("com_cameraMode", "0", CVAR_CHEAT);

	cl_paused = Cvar_Get("cl_paused", "0", CVAR_ROM);
	sv_paused = Cvar_Get("sv_paused", "0", CVAR_ROM);
	com_sv_running = Cvar_Get("sv_running", "0", CVAR_ROM);
	com_cl_running = Cvar_Get("cl_running", "0", CVAR_ROM);

	com_introPlayed = Cvar_Get("com_introplayed", "0", CVAR_ARCHIVE);
	com_recommendedSet = Cvar_Get("com_recommendedSet", "0", CVAR_ARCHIVE);

	com_hunkused = Cvar_Get("com_hunkused", "0", 0);

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
	}

	// start in full screen ui mode
	Cvar_Set("r_uiFullScreen", "1");

	CL_StartHunkUsers();

	// delay this so potential wicked3d dll can find a wolf window
	if (!com_dedicated->integer)
	{
		Sys_ShowConsole(com_viewlog->integer, false);
	}

	// NERVE - SMF - force recommendedSet and don't do vid_restart if in safe mode
	if (!com_recommendedSet->integer && !safeMode)
	{
		Com_SetRecommended(true);
	}
	Cvar_Set("com_recommendedSet", "1");

	if (!com_dedicated->integer)
	{
		Cbuf_AddText("cinematic gmlogo.RoQ\n");
		if (!com_introPlayed->integer)
		{
			Cvar_Set(com_introPlayed->name, "1");
			Cvar_Set("nextmap", "cinematic wolfintro.RoQ");
		}
	}

	NETQ23_Init();

	common->Printf("Working directory: %s\n", Sys_Cwd());

	com_fullyInitialized = true;
	fs_ProtectKeyFile = true;
	common->Printf("--- Common Initialization Complete ---\n");
}

/*
================
Com_ModifyMsec
================
*/
int Com_ModifyMsec(int msec)
{
	int clampTime;

	//
	// modify time for debugging values
	//
	if (com_fixedtime->integer)
	{
		msec = com_fixedtime->integer;
	}
	else if (com_timescale->value)
	{
		msec *= com_timescale->value;
//	} else if (com_cameraMode->integer) {
//		msec *= com_timescale->value;
	}

	// don't let it scale below 1 msec
	if (msec < 1 && com_timescale->value)
	{
		msec = 1;
	}

	if (com_dedicated->integer)
	{
		// dedicated servers don't want to clamp for a much longer
		// period, because it would mess up all the client's views
		// of time.
		if (msec > 500 && msec < 500000)
		{
			common->Printf("Hitch warning: %i msec frame time\n", msec);
		}
		clampTime = 5000;
	}
	else
	if (!com_sv_running->integer)
	{
		// clients of remote servers do not want to clamp time, because
		// it would skew their view of the server's time temporarily
		clampTime = 5000;
	}
	else
	{
		// for local single player gaming
		// we may want to clamp the time to prevent players from
		// flying off edges when something hitches.
		clampTime = 200;
	}

	if (msec > clampTime)
	{
		msec = clampTime;
	}

	return msec;
}

/*
=================
Com_Frame
=================
*/
void Com_SharedFrame()
{
	int msec, minMsec;
	static int lastTime;
	int key;

	int timeBeforeFirstEvents;
	int timeBeforeServer;
	int timeBeforeEvents;
	int timeBeforeClient;
	int timeAfter;

	if (setjmp(abortframe))
	{
		return;		// an ERR_DROP was thrown
	}

	// bk001204 - init to zero.
	//  also:  might be clobbered by `longjmp' or `vfork'
	timeBeforeFirstEvents = 0;
	timeBeforeServer = 0;
	timeBeforeEvents = 0;
	timeBeforeClient = 0;
	timeAfter = 0;


	// old net chan encryption key
	key = 0x87243987;

	// make sure mouse and joystick are only called once a frame
	IN_Frame();

	// write config file if anything changed
	Com_WriteConfiguration();

	// if "viewlog" has been modified, show or hide the log console
	if (com_viewlog->modified)
	{
		if (!com_dedicated->value)
		{
			Sys_ShowConsole(com_viewlog->integer, false);
		}
		com_viewlog->modified = false;
	}

	//
	// main event loop
	//
	if (com_speeds->integer)
	{
		timeBeforeFirstEvents = Sys_Milliseconds();
	}

	// we may want to spin here if things are going too fast
	if (!com_dedicated->integer && com_maxfps->integer > 0 && !com_timedemo->integer)
	{
		minMsec = 1000 / com_maxfps->integer;
	}
	else
	{
		minMsec = 1;
	}
	do
	{
		com_frameTime = Com_EventLoop();
		if (lastTime > com_frameTime)
		{
			lastTime = com_frameTime;	// possible on first frame
		}
		msec = com_frameTime - lastTime;
	}
	while (msec < minMsec);
	Cbuf_Execute();

	lastTime = com_frameTime;

	// mess with msec if needed
	com_frameMsec = msec;
	msec = Com_ModifyMsec(msec);

	//
	// server side
	//
	if (com_speeds->integer)
	{
		timeBeforeServer = Sys_Milliseconds();
	}

	SV_Frame(msec);

	// if "dedicated" has been modified, start up
	// or shut down the client system.
	// Do this after the server may have started,
	// but before the client tries to auto-connect
	if (com_dedicated->modified)
	{
		// get the latched value
		Cvar_Get("dedicated", "0", 0);
		com_dedicated->modified = false;
		if (!com_dedicated->integer)
		{
			CL_Init();
			Sys_ShowConsole(com_viewlog->integer, false);
		}
		else
		{
			CL_Shutdown();
			Sys_ShowConsole(1, true);
		}
	}

	//
	// client system
	//
	if (!com_dedicated->integer)
	{
		//
		// run event loop a second time to get server to client packets
		// without a frame of latency
		//
		if (com_speeds->integer)
		{
			timeBeforeEvents = Sys_Milliseconds();
		}
		Com_EventLoop();
		Cbuf_Execute();

		//
		// client side
		//
		if (com_speeds->integer)
		{
			timeBeforeClient = Sys_Milliseconds();
		}

		CL_Frame(msec);

		if (com_speeds->integer)
		{
			timeAfter = Sys_Milliseconds();
		}
	}

	//
	// report timing information
	//
	if (com_speeds->integer)
	{
		int all, sv, ev, cl;

		all = timeAfter - timeBeforeServer;
		sv = timeBeforeEvents - timeBeforeServer;
		ev = timeBeforeServer - timeBeforeFirstEvents + timeBeforeClient - timeBeforeEvents;
		cl = timeAfter - timeBeforeClient;
		sv -= t3time_game;
		cl -= time_frontend + time_backend;

		common->Printf("frame:%i all:%3i sv:%3i ev:%3i cl:%3i gm:%3i rf:%3i bk:%3i\n",
			com_frameNumber, all, sv, ev, cl, t3time_game, time_frontend, time_backend);
	}

	//
	// trace optimization tracking
	//
	if (com_showtrace->integer)
	{

		common->Printf("%4i traces  (%ib %ip) %4i points\n", c_traces,
			c_brush_traces, c_patch_traces, c_pointcontents);
		c_traces = 0;
		c_brush_traces = 0;
		c_patch_traces = 0;
		c_pointcontents = 0;
	}

	// old net chan encryption key
	key = lastTime * 0x87243987;

	com_frameNumber++;
}
