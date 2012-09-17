/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// cmd.c -- Quake script command processing module

#include "../game/q_shared.h"
#include "qcommon.h"
#include "../../client/public.h"

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
