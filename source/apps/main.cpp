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
#include "../common/hexen2strings.h"
#include "main.h"

//======================= WIN32 DEFINES =================================
#ifdef WIN32

// buildstring will be incorporated into the version string
#ifdef NDEBUG
#ifdef _M_IX86
#define CPUSTRING   "win-x86"
#elif defined _M_X64
#define CPUSTRING   "win-x86_64"
#elif defined _M_ALPHA
#define CPUSTRING   "win-AXP"
#endif
#else
#ifdef _M_IX86
#define CPUSTRING   "win-x86-debug"
#elif defined _M_X64
#define CPUSTRING   "win-x86_64-debug"
#elif defined _M_ALPHA
#define CPUSTRING   "win-AXP-debug"
#endif
#endif

#endif

//======================= MAC OS X DEFINES =====================
#if defined(MACOS_X)

#ifdef __ppc__
#define CPUSTRING   "MacOSX-ppc"
#elif defined __i386__
#define CPUSTRING   "MacOSX-i386"
#elif defined __x86_64__
#define CPUSTRING   "MaxOSX-x86_64"
#else
#define CPUSTRING   "MacOSX-other"
#endif

#endif

//======================= LINUX DEFINES =================================
#ifdef __linux__

#ifdef __i386__
#define CPUSTRING   "linux-i386"
#elif defined __x86_64__
#define CPUSTRING   "linux-x86_64"
#elif defined __axp__
#define CPUSTRING   "linux-alpha"
#else
#define CPUSTRING   "linux-other"
#endif

#endif

//======================= FreeBSD DEFINES =====================
#ifdef __FreeBSD__	// rb010123

#ifdef __i386__
#define CPUSTRING       "freebsd-i386"
#elif defined __axp__
#define CPUSTRING       "freebsd-alpha"
#elif defined __x86_64__
#define CPUSTRING       "freebsd-x86_64"
#else
#define CPUSTRING       "freebsd-other"
#endif

#endif

static Cvar* com_maxfps;
static Cvar* com_fixedtime;
static Cvar* comq3_cameraMode;
static Cvar* com_watchdog;
static Cvar* com_watchdog_cmd;
static Cvar* com_showtrace;

static int com_frameNumber;

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
static void Com_Error_f()
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
static void Com_Freeze_f()
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
static void Com_Crash_f()
{
	*(int*)0 = 0x12345678;
}

static void ComET_GetGameInfo()
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

void Com_Init(int argc, char* argv[], char* commandLine)
{
	if (setjmp(abortframe))
	{
		Sys_Error("Error during initialization");
	}

	common->Printf("JLQuake " JLQUAKE_VERSION_STRING " " CPUSTRING " " __DATE__ "\n");

	// get the initial time base
	Sys_Milliseconds();

	Com_InitByteOrder();

	// bk001129 - do this before anything else decides to push events
	Com_InitEventQueue();

	// prepare enough of the subsystems to handle
	// cvar and command buffer management
	COM_InitArgv2(argc, argv);
	Com_ParseCommandLine(commandLine);

#ifdef DEDICATED
	FS_InitGame(true);
#else
	FS_InitGame(false);
#endif
	Com_SharedInit();

	Cvar_Init();

	Cbuf_Init();

	Cmd_Init();

	if (GGameType & GAME_Quake2)
	{
		// we need to add the early commands twice, because
		// a basedir or cddir needs to be set before execing
		// config files, but we want other parms to override
		// the settings of the config files
		Cbuf_AddEarlyCommands(false);
		Cbuf_Execute();
	}

	if (GGameType & GAME_Tech3)
	{
		// override anything from the config files with command line args
		Com_StartupVariable(NULL);

		// get the developer cvar set as early as possible
		Com_StartupVariable("developer");
	}

	if (GGameType & GAME_ET)
	{
		// bani: init this early
		Com_StartupVariable("com_ignorecrash");
		com_ignorecrash = Cvar_Get("com_ignorecrash", "0", 0);

		// ydnar: init crashed variable as early as possible
		com_crashed = Cvar_Get("com_crashed", "0", CVAR_TEMP);

		// bani: init pid
		int pid = Sys_GetCurrentProcessId();
		char* s = va("%d", pid);
		com_pid = Cvar_Get("com_pid", s, CVAR_ROM);
	}

	FS_InitFilesystem();

	// done early so bind command exists
	CL_InitKeyCommands();

	if (GGameType & GAME_QuakeHexen)
	{
		COMQH_CheckRegistered();
	}
	if (GGameType & GAME_Hexen2 && !(GGameType & GAME_HexenWorld))
	{
		SVH2_RemoveGIPFiles(NULL);
	}

	if (GGameType & GAME_Quake2)
	{
		Cbuf_AddText("exec default.cfg\n");
		Cbuf_AddText("exec config.cfg\n");

		Cbuf_AddEarlyCommands(true);
		Cbuf_Execute();
	}

	if (GGameType & GAME_ET)
	{
		ComET_GetGameInfo();
	}

	bool safeMode = false;
	if (GGameType & GAME_Tech3)
	{
		Com_InitJournaling();

		Cbuf_AddText("exec default.cfg\n");
		if (GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET))
		{
			Cbuf_AddText("exec language.cfg\n");
		}
		
		// skip the q3config.cfg if "safe" is on the command line
		safeMode = Com_SafeMode();
		if (!safeMode)
		{
			if (GGameType & GAME_Quake3)
			{
				Cbuf_AddText("exec q3config.cfg\n");
			}
			else if (GGameType & GAME_WolfSP)
			{
				Cbuf_AddText("exec wolfconfig.cfg\n");
			}
			else if (GGameType & GAME_WolfMP)
			{
				Cbuf_AddText("exec wolfconfig_mp.cfg\n");
			}
			else if (GGameType & GAME_ET)
			{
				if (comet_gameInfo.usesProfiles)
				{
					const char* cl_profileStr = Cvar_VariableString("cl_profile");
					if (!cl_profileStr[0])
					{
						char* defaultProfile = NULL;

						FS_ReadFile("profiles/defaultprofile.dat", (void**)&defaultProfile);

						if (defaultProfile)
						{
							const char* text_p = defaultProfile;
							char* token = String::Parse3(&text_p);

							if (token && *token)
							{
								Cvar_Set("cl_defaultProfile", token);
								Cvar_Set("cl_profile", token);
							}

							FS_FreeFile(defaultProfile);

							cl_profileStr = Cvar_VariableString("cl_defaultProfile");
						}
					}

					if (cl_profileStr[0])
					{
						// bani - check existing pid file and make sure it's ok
						if (!ComET_CheckProfile(va("profiles/%s/profile.pid", cl_profileStr)))
						{
#ifndef _DEBUG
							common->Printf("^3WARNING: profile.pid found for profile '%s' - system settings will revert to defaults\n", cl_profileStr);
							// ydnar: set crashed state
							Cbuf_AddText("set com_crashed 1\n");
#endif
						}

						// bani - write a new one
						if (!ComET_WriteProfile(va("profiles/%s/profile.pid", cl_profileStr)))
						{
							common->Printf("^3WARNING: couldn't write profiles/%s/profile.pid\n", cl_profileStr);
						}

						// exec the config
						Cbuf_AddText(va("exec profiles/%s/%s\n", cl_profileStr, ETCONFIG_NAME));
					}
				}
				else
				{
					Cbuf_AddText(va("exec %s\n", ETCONFIG_NAME));
				}
			}
		}

		Cbuf_AddText("exec autoexec.cfg\n");

		if (GGameType & GAME_ET)
		{
			// ydnar: reset crashed state
			Cbuf_AddText("set com_crashed 0\n");
		}

		Cbuf_Execute();

		// override anything from the config files with command line args
		Com_StartupVariable(NULL);
	}

#ifdef DEDICATED
	// TTimo: default to internet dedicated, not LAN dedicated
	com_dedicated = Cvar_Get("dedicated", "2", CVAR_ROM);
#else
	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld | GAME_Quake2))
	{
		com_dedicated = Cvar_Get("dedicated", "0", CVAR_ROM);
	}
	else if (GGameType & GAME_Tech3)
	{
		com_dedicated = Cvar_Get("dedicated", "0", CVAR_LATCH2);
	}
	else if (COM_CheckParm("-dedicated"))
	{
		com_dedicated = Cvar_Get("dedicated", "1", CVAR_ROM);
	}
	else
	{
		com_dedicated = Cvar_Get("dedicated", "0", CVAR_ROM);
	}
#endif

	// if any archived cvars are modified after this, we will trigger a writing
	// of the config file
	cvar_modifiedFlags &= ~CVAR_ARCHIVE;

	//
	// init commands and vars
	//
	COM_InitCommonCvars();

	com_maxfps = Cvar_Get("com_maxfps", GGameType & GAME_Tech3 ? "85" :
		GGameType & GAME_Quake2 || com_dedicated->integer ? "0" : "72", CVAR_ARCHIVE);

	com_fixedtime = Cvar_Get("fixedtime", "0", CVAR_CHEAT);
	com_showtrace = Cvar_Get("com_showtrace", "0", CVAR_CHEAT);

	com_watchdog = Cvar_Get("com_watchdog", "60", CVAR_ARCHIVE);
	com_watchdog_cmd = Cvar_Get("com_watchdog_cmd", "", CVAR_ARCHIVE);

	if (GGameType & GAME_Quake3)
	{
		comq3_cameraMode = Cvar_Get("com_cameraMode", "0", CVAR_CHEAT);
	}

	if (GGameType & GAME_Tech3)
	{
		com_dropsim = Cvar_Get("com_dropsim", "0", CVAR_CHEAT);
		comt3_timedemo = Cvar_Get("timedemo", "0", CVAR_CHEAT);

		cl_paused = Cvar_Get("cl_paused", "0", CVAR_ROM);
		sv_paused = Cvar_Get("sv_paused", "0", CVAR_ROM);
		com_sv_running = Cvar_Get("sv_running", "0", CVAR_ROM);
		com_cl_running = Cvar_Get("cl_running", "0", CVAR_ROM);
	}

	if (GGameType & (GAME_WolfSP | GAME_ET))
	{
		Cvar_Get("savegame_loading", "0", CVAR_ROM);
	}

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

	if (GGameType & (GAME_Quake2 | GAME_Tech3))
	{
		com_version = Cvar_Get("version", "JLQuake " JLQUAKE_VERSION_STRING " " CPUSTRING " " __DATE__,
			CVAR_ROM | CVAR_SERVERINFO);
	}

	Sys_Init();

	if (GGameType & GAME_Hexen2)
	{
		ComH2_LoadStrings();
	}

	if (GGameType & GAME_QuakeWorld && !com_dedicated->integer)
	{
		NETQHW_Init(QWPORT_CLIENT);
	}
	if (GGameType & GAME_HexenWorld && !com_dedicated->integer)
	{
		NETQHW_Init(HWPORT_CLIENT);
	}

	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		if (com_dedicated->integer)
		{
			Cvar_Set("cl_warncmd", "1");

			Cbuf_InsertText("exec server.cfg\n");
		}
		else
		{
			Cbuf_InsertText(GGameType & GAME_Hexen2 ? "exec hexen.rc\n" : "exec quake.rc\n");
			Cbuf_AddText("cl_warncmd 1\n");
			Cbuf_Execute();
		}
	}

	if (GGameType & GAME_Quake2)
	{
		NETQ23_Init();
	}

	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld | GAME_Quake2 | GAME_Tech3))
	{
		// pick a port value that should be nice and random
		Netchan_Init(Com_Milliseconds() & 0xffff);
	}

	if (GGameType & GAME_Tech3)
	{
		VM_Init();
	}
	if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)) || com_dedicated->integer)
	{
		SV_Init();
	}

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

	if (GGameType & GAME_QuakeHexen && !(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)))
	{
		Cbuf_InsertText(GGameType & GAME_Hexen2 ? "exec hexen.rc\n" : "exec quake.rc\n");
		Cbuf_Execute();

		NETQH_Init();
	}

	if (GGameType & GAME_Tech3)
	{
		// add + commands from command line
		if (!Com_AddStartupCommands())
		{
			// if the user didn't give any commands, run default action
			if (GGameType & GAME_Quake3 && !com_dedicated->integer)
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
	}

	CL_StartHunkUsers();

	if (GGameType & GAME_Quake2)
	{
		// add + commands from command line
		if (!Cbuf_AddLateCommands())
		{
			// if the user didn't give any commands, run default action
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
		{
			// the user asked for something explicit
			// so drop the loading plaque
			SCR_EndLoadingPlaque();
		}
	}

	if (GGameType & GAME_Quake3)
	{
		// make sure single player is off by default
		Cvar_Set("ui_singlePlayerActive", "0");
	}

	if (GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET))
	{
		Cvar* com_recommendedSet = Cvar_Get("com_recommendedSet", "0", CVAR_ARCHIVE);
		if (!com_recommendedSet->integer && !safeMode)
		{
			Com_SetRecommended(true);
		}
		Cvar_Set("com_recommendedSet", "1");
	}

	if (!com_dedicated->integer)
	{
		if (GGameType & GAME_WolfSP)
		{
			Cvar* com_introPlayed = Cvar_Get("com_introplayed", "0", CVAR_ARCHIVE);
			if (!com_introPlayed->integer)
			{
				//----(SA)	force this to get played every time (but leave cvar for override)
				Cbuf_AddText("cinematic wolfintro.RoQ 3\n");
			}
		}
		else if (GGameType & GAME_WolfMP)
		{
			Cbuf_AddText("cinematic gmlogo.RoQ\n");
			Cvar* com_introPlayed = Cvar_Get("com_introplayed", "0", CVAR_ARCHIVE);
			if (!com_introPlayed->integer)
			{
				Cvar_Set(com_introPlayed->name, "1");
				Cvar_Set("nextmap", "cinematic wolfintro.RoQ");
			}
		}
		else if (GGameType & GAME_ET)
		{
			Cbuf_AddText("cinematic etintro.roq\n");
		}
	}

	if (GGameType & GAME_Tech3)
	{
		NETQ23_Init();
	}

	common->Printf("Working directory: %s\n", Sys_Cwd());

	fs_ProtectKeyFile = true;
	com_fullyInitialized = true;

	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld) && com_dedicated->integer)
	{
		// process command line arguments
		Cbuf_AddLateCommands();
		Cbuf_Execute();

		// if a map wasn't specified on the command line, spawn start.map
		if (!SV_IsServerActive())
		{
			Cmd_ExecuteString(GGameType & GAME_HexenWorld ? "map demo1" : "map start");
		}
		if (!SV_IsServerActive())
		{
			common->Error("Couldn't spawn a server");
		}
	}

	common->Printf("--- Common Initialization Complete ---\n");
}
