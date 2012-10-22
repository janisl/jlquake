/*
 * $Header: /H2 Mission Pack/Host_cmd.c 25    4/01/98 4:53p Jmonroe $
 */

#include "quakedef.h"
#include "../server/public.h"

void Com_Quit_f()
{
	CL_Disconnect(true);
	SV_Shutdown("");

	Sys_Quit();
}

void Host_Version_f()
{
	common->Printf("Version %4.2f\n", HEXEN2_VERSION);
	common->Printf("Exe: "__TIME__ " "__DATE__ "\n");
}

void Host_InitCommands()
{
	Cmd_AddCommand("quit", Com_Quit_f);
	Cmd_AddCommand("version", Host_Version_f);
}
