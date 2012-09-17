/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// cmd.c -- Quake script command processing module

#include "../game/q_shared.h"
#include "qcommon.h"
#include "../../client/public.h"

/*
=============================================================================

                    COMMAND EXECUTION

=============================================================================
*/

bool Cmd_HandleNullCommand(const char* text)
{
	// let the cgame or game handle it
	return false;
}

void Cmd_HandleUnknownCommand()
{
	// check client game commands
	if (com_cl_running && com_cl_running->integer && CLT3_GameCommand())
	{
		return;
	}

	// check server game commands
	if (com_sv_running && com_sv_running->integer && SVT3_GameCommand())
	{
		return;
	}

	// check ui commands
	if (com_cl_running && com_cl_running->integer && UIT3_GameCommand())
	{
		return;
	}

	// send it as a server command if we are connected
	// this will usually result in a chat message
	CL_ForwardCommandToServer(Cmd_Cmd());
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
