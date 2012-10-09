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

#include "qwsvdef.h"
#include "../../server/server.h"
#include "../../server/quake_hexen/local.h"

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
	Cmd_AddCommand("logfile", SV_Logfile_f);

	Cmd_AddCommand("quit", SV_Quit_f);
	Cmd_AddCommand("gamedir", SV_Gamedir_f);

	Cvar_Set("cl_warncmd", "1");
}
