//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../common/qcommon.h"
#include "../server/public.h"
#include "../client/public.h"
#include "main.h"

Cvar* com_maxfps;
Cvar* com_fixedtime;
Cvar* comq3_cameraMode;
Cvar* com_watchdog;
Cvar* com_watchdog_cmd;
Cvar* com_showtrace;

int com_frameNumber;

static int Com_ModifyMsec(int msec)
{
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
	}
	else if (GGameType & GAME_Quake3 && comq3_cameraMode->integer)
	{
		//JL timescale will be 0 in this case.
		msec *= com_timescale->value;
	}

	// don't let it scale below 1 msec
	if (msec < 1 && com_timescale->value)
	{
		msec = 1;
	}

	int clampTime;
	if (com_dedicated->integer)
	{
		// dedicated servers don't want to clamp for a much longer
		// period, because it would mess up all the client's views
		// of time.
		if (msec > 500)
		{
			common->Printf("Hitch warning: %i msec frame time\n", msec);
		}
		clampTime = 5000;
	}
	else if (GGameType & GAME_Tech3 ? !com_sv_running->integer : !SV_IsServerActive())
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
		clampTime = GGameType & GAME_Hexen2 && !(GGameType & GAME_HexenWorld) ? 50 : 200;
	}

	if (msec > clampTime)
	{
		msec = clampTime;
	}

	return msec;
}

void Com_Frame()
{
	static int lastTime;

	if (setjmp(abortframe))
	{
		return;		// an ERR_DROP was thrown
	}

	// make sure mouse and joystick are only called once a frame
	IN_Frame();

	if (GGameType & GAME_Tech3)
	{
		// write config file if anything changed
		Com_WriteConfiguration();
	}

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
	int timeBeforeFirstEvents = 0;
	if (com_speeds->integer)
	{
		timeBeforeFirstEvents = Sys_Milliseconds();
	}

	// we may want to spin here if things are going too fast
	int minMsec;
	if (!com_dedicated->integer && com_maxfps->integer > 0 && !CL_IsTimeDemo())
	{
		minMsec = 1000 / com_maxfps->integer;
	}
	else
	{
		minMsec = 1;
	}
	int msec;
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
	msec = Com_ModifyMsec(msec);

	//
	// server side
	//
	int timeBeforeServer = 0;
	if (com_speeds->integer)
	{
		timeBeforeServer = Sys_Milliseconds();
	}

	if (GGameType & GAME_QuakeHexen)
	{
		// keep the random time dependent
		rand();
		if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)))
		{
			SVQH_SetRealTime(com_frameTime);
		}
	}

	SV_Frame(msec);

	// if "dedicated" has been modified, start up
	// or shut down the client system.
	// Do this after the server may have started,
	// but before the client tries to auto-connect
	if (GGameType & GAME_Tech3 && com_dedicated->modified)
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
	int timeBeforeEvents = 0;
	int timeBeforeClient = 0;
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
	}

	int timeAfter = 0;
	if (com_speeds->integer)
	{
		timeAfter = Sys_Milliseconds();
	}

	//
	// watchdog
	//
	if (com_dedicated->integer && !com_sv_running->integer && com_watchdog->integer)
	{
		static int watchdogTime = 0;
		static bool watchWarn = false;
		if (watchdogTime == 0)
		{
			watchdogTime = Sys_Milliseconds();
		}
		else
		{
			if (!watchWarn && Sys_Milliseconds() - watchdogTime > (com_watchdog->integer - 4) * 1000)
			{
				common->Printf("WARNING: watchdog will trigger in 4 seconds\n");
				watchWarn = true;
			}
			else if (Sys_Milliseconds() - watchdogTime > com_watchdog->integer * 1000)
			{
				common->Printf("Idle Server with no map - triggering watchdog\n");
				watchdogTime = 0;
				watchWarn = false;
				if (com_watchdog_cmd->string[0] == '\0')
				{
					Cbuf_AddText("quit\n");
				}
				else
				{
					Cbuf_AddText(va("%s\n", com_watchdog_cmd->string));
				}
			}
		}
	}

	//
	// report timing information
	//
	if (com_speeds->integer)
	{
		if (GGameType & GAME_QuakeHexen)
		{
			static int time3 = 0;
			int pass1 = time_before_ref - time3;
			time3 = Sys_Milliseconds();
			int pass2 = time_after_ref - time_before_ref;
			int pass3 = time3 - time_after_ref;
			common->Printf("%3i tot %3i server %3i gfx %3i snd\n",
				pass1 + pass2 + pass3, pass1, pass2, pass3);
		}
		else if (GGameType & GAME_Quake2)
		{
			int all, sv, gm, cl, rf;

			all = timeAfter - timeBeforeServer;
			sv = timeBeforeEvents - timeBeforeServer;
			cl = timeAfter - timeBeforeEvents;
			gm = time_after_game - time_before_game;
			rf = time_after_ref - time_before_ref;
			sv -= gm;
			cl -= rf;
			common->Printf("all:%3i sv:%3i gm:%3i cl:%3i rf:%3i\n",
				all, sv, gm, cl, rf);
		}
		else
		{
			int all = timeAfter - timeBeforeServer;
			int sv = timeBeforeEvents - timeBeforeServer;
			int sev = timeBeforeServer - timeBeforeFirstEvents;
			int cev = timeBeforeClient - timeBeforeEvents;
			int cl = timeAfter - timeBeforeClient;
			sv -= t3time_game;
			cl -= time_frontend + time_backend;

			common->Printf("frame:%i all:%3i sv:%3i sev:%3i cev:%3i cl:%3i gm:%3i rf:%3i bk:%3i\n",
				com_frameNumber, all, sv, sev, cev, cl, t3time_game, time_frontend, time_backend);
		}
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

	com_frameNumber++;
}

//	Just throw a fatal error to test error shutdown procedures
void Com_Error_f()
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

//	Just freeze in place for a given number of seconds to test
// error recovery
void Com_Freeze_f()
{
	if (Cmd_Argc() != 2)
	{
		common->Printf("freeze <seconds>\n");
		return;
	}
	float s = String::Atof(Cmd_Argv(1));

	int start = Com_Milliseconds();

	while (1)
	{
		int now = Com_Milliseconds();
		if ((now - start) * 0.001 > s)
		{
			break;
		}
	}
}

//	A way to force a bus error for development reasons
void Com_Crash_f()
{
	*(int*)0 = 0x12345678;
}

void ComET_GetGameInfo()
{
	Com_Memset(&comet_gameInfo, 0, sizeof(comet_gameInfo));

	char* f;
	if (FS_ReadFile("gameinfo.dat", (void**)&f) <= 0)
	{
		return;
	}

	const char* buf = f;
	char* token;
	while ((token = String::Parse3(&buf)) != NULL && token[0])
	{
		if (!String::ICmp(token, "spEnabled"))
		{
			comet_gameInfo.spEnabled = true;
		}
		else if (!String::ICmp(token, "spGameTypes"))
		{
			while ((token = String::ParseExt(&buf, false)) != NULL && token[0])
			{
				comet_gameInfo.spGameTypes |= (1 << String::Atoi(token));
			}
		}
		else if (!String::ICmp(token, "defaultSPGameType"))
		{
			if ((token = String::ParseExt(&buf, false)) != NULL && token[0])
			{
				comet_gameInfo.defaultSPGameType = String::Atoi(token);
			}
			else
			{
				FS_FreeFile(f);
				common->FatalError("ComET_GetGameInfo: bad syntax.");
			}
		}
		else if (!String::ICmp(token, "coopGameTypes"))
		{
			while ((token = String::ParseExt(&buf, false)) != NULL && token[0])
			{
				comet_gameInfo.coopGameTypes |= (1 << String::Atoi(token));
			}
		}
		else if (!String::ICmp(token, "defaultCoopGameType"))
		{
			if ((token = String::ParseExt(&buf, false)) != NULL && token[0])
			{
				comet_gameInfo.defaultCoopGameType = String::Atoi(token);
			}
			else
			{
				FS_FreeFile(f);
				common->FatalError("ComET_GetGameInfo: bad syntax.");
			}
		}
		else if (!String::ICmp(token, "defaultGameType"))
		{
			if ((token = String::ParseExt(&buf, false)) != NULL && token[0])
			{
				comet_gameInfo.defaultGameType = String::Atoi(token);
			}
			else
			{
				FS_FreeFile(f);
				common->FatalError("ComET_GetGameInfo: bad syntax.");
			}
		}
		else if (!String::ICmp(token, "usesProfiles"))
		{
			if ((token = String::ParseExt(&buf, false)) != NULL && token[0])
			{
				comet_gameInfo.usesProfiles = String::Atoi(token);
			}
			else
			{
				FS_FreeFile(f);
				common->FatalError("ComET_GetGameInfo: bad syntax.");
			}
		}
		else
		{
			FS_FreeFile(f);
			common->FatalError("ComET_GetGameInfo: bad syntax.");
		}
	}

	// all is good
	FS_FreeFile(f);
}

void Com_Init(int argc, char* argv[], char* cmdline)
{
	Com_SharedInit(argc, argv, cmdline);
}
