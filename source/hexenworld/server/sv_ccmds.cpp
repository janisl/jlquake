
#include "qwsvdef.h"
#include "../../server/public.h"

/*
===============================================================================

OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

/*
==================
SV_Quit_f
==================
*/
void SV_Quit_f(void)
{
	common->Printf("Shutting down.\n");
	SV_Shutdown("server shutdown\n");
	Com_Shutdown();
	Sys_Quit();
}

/*
==================
SV_InitOperatorCommands
==================
*/
void SV_InitOperatorCommands(void)
{
	Cmd_AddCommand("quit", SV_Quit_f);

	Cvar_Set("cl_warncmd", "1");
}
