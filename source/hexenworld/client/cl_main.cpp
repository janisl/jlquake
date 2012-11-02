// cl_main.c  -- client main loop

#include "../../common/qcommon.h"
#include "../../client/public.h"
#include "../../server/public.h"
#include "../../common/hexen2strings.h"
#include "../../apps/main.h"

void aaa()
{
	SV_IsServerActive();
}

/*
=================
CL_InitLocal
=================
*/
void CL_InitLocal(void)
{
	CL_Init();
	CL_StartHunkUsers();

//
// register our commands
//
	com_speeds = Cvar_Get("host_speeds", "0", 0);			// set for running times
	com_maxfps = Cvar_Get("com_maxfps", "72", CVAR_ARCHIVE);

	COM_InitCommonCommands();
}

void Com_SharedInit(int argc, char* argv[], char* cmdline)
{
	Sys_Init();

	GGameType = GAME_Hexen2 | GAME_HexenWorld;
	Sys_SetHomePathSuffix("jlhexen2");

	COM_InitArgv2(argc, argv);

	Cbuf_Init();
	Cmd_Init();
	Cvar_Init();

	com_dedicated = Cvar_Get("dedicated", "0", CVAR_ROM);

	Com_InitByteOrder();

	COM_InitCommonCvars();

	qh_registered = Cvar_Get("registered", "0", 0);
	com_fixedtime = Cvar_Get("fixedtime", "0", CVAR_CHEAT);
	com_showtrace = Cvar_Get("com_showtrace", "0", CVAR_CHEAT);

	com_watchdog = Cvar_Get("com_watchdog", "60", CVAR_ARCHIVE);
	com_watchdog_cmd = Cvar_Get("com_watchdog_cmd", "", CVAR_ARCHIVE);

	FS_InitFilesystem();
	COMQH_CheckRegistered();

	NETQHW_Init(HWPORT_CLIENT);

	Netchan_Init(0);

	CL_InitKeyCommands();
	ComH2_LoadStrings();

	Cbuf_InsertText("exec hexen.rc\n");
	Cbuf_AddText("cl_warncmd 1\n");
	Cbuf_Execute();

	CL_InitLocal();
	Sys_ShowConsole(0, false);

	com_fullyInitialized = true;

	common->Printf("������� HexenWorld Initialized �������\n");
}
