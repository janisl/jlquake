/*
Copyright (C) 1996-1997 Id Software, Inc.

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

#include "../../common/qcommon.h"
#include "../../server/public.h"
#include "../../client/public.h"
#include "../../apps/main.h"

#define VERSION     2.40

void Com_SharedInit(int argc, char* argv[], char* cmdline)
{
	GGameType = GAME_Quake | GAME_QuakeWorld;
	Sys_SetHomePathSuffix("jlquake");

	COM_InitArgv2(argc, argv);
	COM_AddParm("-game");
	COM_AddParm("qw");

	Cbuf_Init();
	Cmd_Init();
	Cvar_Init();

	com_dedicated = Cvar_Get("dedicated", "1", CVAR_ROM);
	com_maxfps = Cvar_Get("com_maxfps", "0", CVAR_ARCHIVE);
	com_fixedtime = Cvar_Get("fixedtime", "0", CVAR_CHEAT);
	com_showtrace = Cvar_Get("com_showtrace", "0", CVAR_CHEAT);

	com_watchdog = Cvar_Get("com_watchdog", "60", CVAR_ARCHIVE);
	com_watchdog_cmd = Cvar_Get("com_watchdog_cmd", "", CVAR_ARCHIVE);

	Com_InitByteOrder();

	COM_InitCommonCvars();

	qh_registered = Cvar_Get("registered", "0", 0);

	FS_InitFilesystem();
	COMQH_CheckRegistered();

	COM_InitCommonCommands();

	Cvar_Set("cl_warncmd", "1");

	SV_Init();
	Sys_Init();
	PMQH_Init();

	Cbuf_InsertText("exec server.cfg\n");

	com_fullyInitialized = true;

	common->Printf("Exe: "__TIME__ " "__DATE__ "\n");

	common->Printf("\nServer Version %4.2f\n\n", VERSION);

	common->Printf("======== QuakeWorld Initialized ========\n");

// process command line arguments
	Cbuf_AddLateCommands();
	Cbuf_Execute();

// if a map wasn't specified on the command line, spawn start.map
	if (!SV_IsServerActive())
	{
		Cmd_ExecuteString("map start");
	}
	if (!SV_IsServerActive())
	{
		common->Error("Couldn't spawn a server");
	}
}
