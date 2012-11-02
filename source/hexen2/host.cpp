// host.c -- coordinates spawning and killing of local servers

/*
 * $Header: /H2 Mission Pack/HOST.C 6     3/12/98 6:31p Mgummelt $
 */

#include "../common/qcommon.h"
#include "../common/hexen2strings.h"
#include "../server/public.h"
#include "../client/public.h"
#include "../apps/main.h"

void Host_InitLocal()
{
	COM_InitCommonCvars();

	com_speeds = Cvar_Get("host_speeds", "0", 0);			// set for running times
	com_maxfps = Cvar_Get("com_maxfps", "72", CVAR_ARCHIVE);

	if (COM_CheckParm("-dedicated"))
	{
		com_dedicated = Cvar_Get("dedicated", "1", CVAR_ROM);
	}
	else
	{
#ifdef DEDICATED
		com_dedicated = Cvar_Get("dedicated", "1", CVAR_ROM);
#else
		com_dedicated = Cvar_Get("dedicated", "0", CVAR_ROM);
#endif
	}

	COM_InitCommonCommands();
}

void Com_SharedInit(int argc, char* argv[], char* cmdline)
{
	COM_InitArgv2(argc, argv);

	Sys_Init();

	GGameType = GAME_Hexen2;
	if (COM_CheckParm("-portals"))
	{
		GGameType |= GAME_H2Portals;
	}
	Sys_SetHomePathSuffix("jlhexen2");

	Cbuf_Init();
	Cmd_Init();
	Cvar_Init();
	Com_InitByteOrder();

	qh_registered = Cvar_Get("registered", "0", 0);
	com_fixedtime = Cvar_Get("fixedtime", "0", CVAR_CHEAT);
	com_showtrace = Cvar_Get("com_showtrace", "0", CVAR_CHEAT);

	com_watchdog = Cvar_Get("com_watchdog", "60", CVAR_ARCHIVE);
	com_watchdog_cmd = Cvar_Get("com_watchdog_cmd", "", CVAR_ARCHIVE);

	FS_InitFilesystem();
	COMQH_CheckRegistered();
	Host_InitLocal();
	SVH2_RemoveGIPFiles(NULL);
	CL_InitKeyCommands();
	ComH2_LoadStrings();
	SV_Init();

	common->Printf("Exe: "__TIME__ " "__DATE__ "\n");

	if (!com_dedicated->integer)
	{
		CL_Init();
	}

	Cbuf_InsertText("exec hexen.rc\n");
	Cbuf_Execute();

	NETQH_Init();

	if (!com_dedicated->integer)
	{
		CL_StartHunkUsers();
		Sys_ShowConsole(0, false);
	}

	com_fullyInitialized = true;

	common->Printf("======== Hexen II Initialized =========\n");
}
