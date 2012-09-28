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
// cmd.c -- Quake script command processing module

#include "quakedef.h"

/*
=============================================================================

                    COMMAND EXECUTION

=============================================================================
*/

#ifndef DEDICATED
void Cmd_ForwardToServer_f(void)
{
	if (cls.state != CA_ACTIVE)
	{
		common->Printf("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}

	if (clc.demoplaying)
	{
		return;		// not really connected

	}

	if (Cmd_Argc() > 1)
	{
		clc.netchan.message.WriteByte(q1clc_stringcmd);
		clc.netchan.message.Print(Cmd_ArgsUnmodified());
	}
}
#endif

/*
============
Cmd_Init
============
*/
void Cmd_Init(void)
{
	Cmd_SharedInit();
#ifndef DEDICATED
	Cmd_AddCommand("cmd", Cmd_ForwardToServer);
#endif
}

bool Cmd_HandleNullCommand(const char* text)
{
	common->FatalError("NULL command");
	return false;
}

void Cmd_HandleUnknownCommand()
{
	common->Printf("Unknown command \"%s\"\n", Cmd_Argv(0));
}

/*
===================
Cmd_ForwardToServer

Sends the entire command line over to the server
===================
*/
void Cmd_ForwardToServer(void)
{
#ifndef DEDICATED
	if (cls.state != CA_ACTIVE)
	{
		common->Printf("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}

	if (clc.demoplaying)
	{
		return;		// not really connected

	}

	clc.netchan.message.WriteByte(q1clc_stringcmd);
	clc.netchan.message.Print(Cmd_Cmd());
#endif
}
