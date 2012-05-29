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

/*****************************************************************************
 * name:		be_ea.c
 *
 * desc:		elementary actions
 *
 * $Archive: /MissionPack/code/botlib/be_ea.c $
 *
 *****************************************************************************/

#include "../common/qcommon.h"
#include "botlib.h"
#include "be_interface.h"

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_Say(int client, char* str)
{
	botimport.BotClientCommand(client, va("say %s", str));
}	//end of the function EA_Say
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_SayTeam(int client, char* str)
{
	botimport.BotClientCommand(client, va("say_team %s", str));
}	//end of the function EA_SayTeam
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_Command(int client, char* command)
{
	botimport.BotClientCommand(client, command);
}	//end of the function EA_Command
