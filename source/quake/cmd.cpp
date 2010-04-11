/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cmd.c -- Quake script command processing module

#include "quakedef.h"

/*
==============================================================================

						SCRIPT COMMANDS

==============================================================================
*/


/*
===============
Cmd_Exec_f
===============
*/
void Cmd_Exec_f (void)
{
	char	*f;
	int		mark;

	if (Cmd_Argc () != 2)
	{
		Con_Printf ("exec <filename> : execute a script file\n");
		return;
	}

	mark = Hunk_LowMark ();
	f = (char *)COM_LoadHunkFile (Cmd_Argv(1));
	if (!f)
	{
		Con_Printf ("couldn't exec %s\n",Cmd_Argv(1));
		return;
	}
	Con_Printf ("execing %s\n",Cmd_Argv(1));
	
	Cbuf_InsertText (f);
	Hunk_FreeToLowMark (mark);
}


/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/


/*
============
Cmd_Init
============
*/
void Cmd_Init (void)
{
	Cmd_SharedInit(false);
	Cmd_AddCommand ("exec",Cmd_Exec_f);
	Cmd_AddCommand ("cmd", Cmd_ForwardToServer);
}

bool Cmd_HandleNullCommand(const char* text)
{
    throw QException("NULL command");
}

void Cmd_HandleUnknownCommand()
{
	Con_Printf ("Unknown command \"%s\"\n", Cmd_Argv(0));
}

/*
===================
Cmd_ForwardToServer

Sends the entire command line over to the server
===================
*/
void Cmd_ForwardToServer (void)
{
	if (cls.state != ca_connected)
	{
		Con_Printf ("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}
	
	if (cls.demoplayback)
		return;		// not really connected

	cls.message.WriteByte(clc_stringcmd);
	if (QStr::ICmp(Cmd_Argv(0), "cmd") != 0)
	{
		cls.message.Print(Cmd_Argv(0));
		cls.message.Print(" ");
	}
	if (Cmd_Argc() > 1)
		cls.message.Print(Cmd_Args());
	else
		cls.message.Print("\n");
}
