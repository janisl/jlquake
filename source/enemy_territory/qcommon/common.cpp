/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// common.c -- misc functions used in client and server

#include "../../common/qcommon.h"
#include "../../client/public.h"
#include "../../server/public.h"
#include "../../apps/main.h"

#define Q3_VERSION      "ET 2.60d"
// 2.60d: Mac OSX universal binaries
// 2.60c: Mac OSX universal binaries
// 2.60b: CVE-2006-2082 fix
// 2.6x: Enemy Territory - ETPro team maintenance release
// 2.5x: Enemy Territory FINAL
// 2.4x: Enemy Territory RC's
// 2.3x: Enemy Territory TEST
// 2.2+: post SP removal
// 2.1+: post Enemy Territory moved standalone
// 2.x: post Enemy Territory
// 1.x: pre Enemy Territory
////
// 1.3-MP : final for release
// 1.1b - TTimo SP linux release (+ MP updates)
// 1.1b5 - Mac update merge in

FILE* debuglogfile;

//Cvar	*com_blood;
Cvar* com_introPlayed;
Cvar* com_logosPlaying;
Cvar* com_recommendedSet;

// Rafael Notebook
Cvar* cl_notebook;

Cvar* com_hunkused;			// Ridah

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

void Com_GetGameInfo()
{
	char* f;
	const char* buf;
	char* token;

	memset(&comet_gameInfo, 0, sizeof(comet_gameInfo));

	if (FS_ReadFile("gameinfo.dat", (void**)&f) > 0)
	{

		buf = f;

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
					common->FatalError("Com_GetGameInfo: bad syntax.");
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
					common->FatalError("Com_GetGameInfo: bad syntax.");
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
					common->FatalError("Com_GetGameInfo: bad syntax.");
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
					common->FatalError("Com_GetGameInfo: bad syntax.");
				}
			}
			else
			{
				FS_FreeFile(f);
				common->FatalError("Com_GetGameInfo: bad syntax.");
			}
		}

		// all is good
		FS_FreeFile(f);
	}
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
	int pid;
	// TTimo gcc warning: variable `safeMode' might be clobbered by `longjmp' or `vfork'
	volatile qboolean safeMode = true;

	common->Printf("%s %s %s\n", Q3_VERSION, CPUSTRING, __DATE__);

	if (setjmp(abortframe))
	{
		Sys_Error("Error during initialization");
	}

	GGameType = GAME_ET;
	Sys_SetHomePathSuffix("jlet");

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

	// bani: init this early
	Com_StartupVariable("com_ignorecrash");
	com_ignorecrash = Cvar_Get("com_ignorecrash", "0", 0);

	// ydnar: init crashed variable as early as possible
	com_crashed = Cvar_Get("com_crashed", "0", CVAR_TEMP);

	// bani: init pid
	pid = Sys_GetCurrentProcessId();
	s = va("%d", pid);
	com_pid = Cvar_Get("com_pid", s, CVAR_ROM);

	// done early so bind command exists
	CL_InitKeyCommands();

	FS_InitFilesystem();

	Com_InitJournaling();

	Com_GetGameInfo();

	Cbuf_AddText("exec default.cfg\n");
	Cbuf_AddText("exec language.cfg\n");	// NERVE - SMF

	// skip the q3config.cfg if "safe" is on the command line
	if (!Com_SafeMode())
	{
		const char* cl_profileStr = Cvar_VariableString("cl_profile");

		safeMode = false;
		if (comet_gameInfo.usesProfiles)
		{
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

	Cbuf_AddText("exec autoexec.cfg\n");

	// ydnar: reset crashed state
	Cbuf_AddText("set com_crashed 0\n");

	// execute the queued commands
	Cbuf_Execute();

	// override anything from the config files with command line args
	Com_StartupVariable(NULL);

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
	// Gordon: no need to latch this in ET, our recoil is framerate independant
	com_maxfps = Cvar_Get("com_maxfps", "85", CVAR_ARCHIVE /*|CVAR_LATCH2*/);
//	com_blood = Cvar_Get ("com_blood", "1", CVAR_ARCHIVE); // Gordon: no longer used?

	com_fixedtime = Cvar_Get("fixedtime", "0", CVAR_CHEAT);
	com_showtrace = Cvar_Get("com_showtrace", "0", CVAR_CHEAT);
	com_dropsim = Cvar_Get("com_dropsim", "0", CVAR_CHEAT);
	com_speeds = Cvar_Get("com_speeds", "0", 0);
	comt3_timedemo = Cvar_Get("timedemo", "0", CVAR_CHEAT);

	com_watchdog = Cvar_Get("com_watchdog", "60", CVAR_ARCHIVE);
	com_watchdog_cmd = Cvar_Get("com_watchdog_cmd", "", CVAR_ARCHIVE);

	cl_paused = Cvar_Get("cl_paused", "0", CVAR_ROM);
	sv_paused = Cvar_Get("sv_paused", "0", CVAR_ROM);
	com_sv_running = Cvar_Get("sv_running", "0", CVAR_ROM);
	com_cl_running = Cvar_Get("cl_running", "0", CVAR_ROM);

	com_introPlayed = Cvar_Get("com_introplayed", "0", CVAR_ARCHIVE);
	com_logosPlaying = Cvar_Get("com_logosPlaying", "0", CVAR_ROM);
	com_recommendedSet = Cvar_Get("com_recommendedSet", "0", CVAR_ARCHIVE);

	Cvar_Get("savegame_loading", "0", CVAR_ROM);

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
		//Cvar_Set( "com_logosPlaying", "1" );
		Cbuf_AddText("cinematic etintro.roq\n");
		/*Cvar_Set( "nextmap", "cinematic avlogo.roq" );
		if( !com_introPlayed->integer ) {
			Cvar_Set( com_introPlayed->name, "1" );
			//Cvar_Set( "nextmap", "cinematic avlogo.roq" );
		}*/
	}

	NETQ23_Init();

	common->Printf("Working directory: %s\n", Sys_Cwd());

	com_fullyInitialized = true;
	fs_ProtectKeyFile = true;
	common->Printf("--- Common Initialization Complete ---\n");
}
