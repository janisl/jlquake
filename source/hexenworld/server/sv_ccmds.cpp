
#include "qwsvdef.h"

extern Cvar* cl_warncmd;
extern char* rd_buffer;

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
	SVQHW_FinalMessage("server shutdown\n");
	common->Printf("Shutting down.\n");
	SVQHW_Shutdown();
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
================
SV_Gamedir_f

Sets the gamedir and path to a different directory.
================
*/
void SV_Gamedir_f(void)
{
	char* dir;

	if (Cmd_Argc() == 1)
	{
		common->Printf("Current gamedir: %s\n", fs_gamedir);
		return;
	}

	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: gamedir <newdir>\n");
		return;
	}

	dir = Cmd_Argv(1);

	if (strstr(dir, "..") || strstr(dir, "/") ||
		strstr(dir, "\\") || strstr(dir, ":"))
	{
		common->Printf("Gamedir should be a single filename, not a path\n");
		return;
	}

	COM_Gamedir(dir);
	Info_SetValueForKey(svs.qh_info, "*gamedir", dir, MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value);
}


/*
==================
SV_InitOperatorCommands
==================
*/
void SV_InitOperatorCommands(void)
{
	SVQHW_InitOperatorCommands();
	Cmd_AddCommand("logfile", SV_Logfile_f);

	Cmd_AddCommand("quit", SV_Quit_f);
	Cmd_AddCommand("gamedir", SV_Gamedir_f);

	cl_warncmd->value = 1;
}
