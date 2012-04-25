/*
Copyright (C) 1997-2001 Id Software, Inc.

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

#include "qcommon.h"

void Cmd_ForwardToServer(void);

/*
=============================================================================

                    COMMAND EXECUTION

=============================================================================
*/

bool Cmd_HandleNullCommand(const char* text)
{
	// forward to server command
	Cmd_ExecuteString(va("cmd %s", text));
	return true;
}

void Cmd_HandleUnknownCommand()
{
	// send it as a server command if we are connected
	Cmd_ForwardToServer();
}

/*
============
Cmd_Init
============
*/
void Cmd_Init(void)
{
	Cmd_SharedInit();
}
