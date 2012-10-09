
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
	if (sv_logfile)
	{
		FS_FCloseFile(sv_logfile);
		sv_logfile = 0;
	}
	Sys_Quit();
}

/*
============
SV_Logfile_f
============
*/
void SV_Logfile_f(void)
{
	char name[MAX_OSPATH];

	if (sv_logfile)
	{
		common->Printf("File logging off.\n");
		FS_FCloseFile(sv_logfile);
		sv_logfile = 0;
		return;
	}

	sprintf(name, "qconsole.log");
	common->Printf("Logging text to %s.\n", name);
	sv_logfile = FS_FOpenFileWrite(name);
	if (!sv_logfile)
	{
		common->Printf("failed.\n");
	}
}

/*
==================
SV_InitOperatorCommands
==================
*/
void SV_InitOperatorCommands(void)
{
	Cmd_AddCommand("logfile", SV_Logfile_f);

	Cmd_AddCommand("quit", SV_Quit_f);

	Cvar_Set("cl_warncmd", "1");
}
