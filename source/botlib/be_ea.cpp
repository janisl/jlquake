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
#include "l_memory.h"
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
void EA_Tell(int client, int clientto, char* str)
{
	botimport.BotClientCommand(client, va("tell %d, %s", clientto, str));
}	//end of the function EA_SayTeam
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_UseItem(int client, char* it)
{
	botimport.BotClientCommand(client, va("use %s", it));
}	//end of the function EA_UseItem
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_DropItem(int client, char* it)
{
	botimport.BotClientCommand(client, va("drop %s", it));
}	//end of the function EA_DropItem
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_UseInv(int client, char* inv)
{
	botimport.BotClientCommand(client, va("invuse %s", inv));
}	//end of the function EA_UseInv
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_DropInv(int client, char* inv)
{
	botimport.BotClientCommand(client, va("invdrop %s", inv));
}	//end of the function EA_DropInv
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
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_EndRegular(int client, float thinktime)
{
/*
    bot_input_t *bi;
    int jumped = qfalse;

    bi = &botinputs[client];

    bi->actionflags &= ~ACTION_JUMPEDLASTFRAME;

    bi->thinktime = thinktime;
    botimport.BotInput(client, bi);

    bi->thinktime = 0;
    VectorClear(bi->dir);
    bi->speed = 0;
    jumped = bi->actionflags & Q3ACTION_JUMP;
    bi->actionflags = 0;
    if (jumped) bi->actionflags |= ACTION_JUMPEDLASTFRAME;
*/
}	//end of the function EA_EndRegular
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_GetInput(int client, float thinktime, bot_input_t* input)
{
	bot_input_t* bi;
//	int jumped = qfalse;

	bi = &botinputs[client];

//	bi->actionflags &= ~ACTION_JUMPEDLASTFRAME;

	bi->thinktime = thinktime;
	Com_Memcpy(input, bi, sizeof(bot_input_t));

	/*
	bi->thinktime = 0;
	VectorClear(bi->dir);
	bi->speed = 0;
	jumped = bi->actionflags & Q3ACTION_JUMP;
	bi->actionflags = 0;
	if (jumped) bi->actionflags |= ACTION_JUMPEDLASTFRAME;
	*/
}	//end of the function EA_GetInput
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_ResetInput(int client)
{
	bot_input_t* bi;
	int jumped = false;

	bi = &botinputs[client];
	bi->actionflags &= ~ACTION_JUMPEDLASTFRAME;

	bi->thinktime = 0;
	VectorClear(bi->dir);
	bi->speed = 0;
	jumped = bi->actionflags & Q3ACTION_JUMP;
	bi->actionflags = 0;
	if (jumped)
	{
		bi->actionflags |= ACTION_JUMPEDLASTFRAME;
	}
}	//end of the function EA_ResetInput
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int EA_Setup(void)
{
	//initialize the bot inputs
	botinputs = (bot_input_t*)GetClearedHunkMemory(
		botlibglobals.maxclients * sizeof(bot_input_t));
	return BLERR_NOERROR;
}	//end of the function EA_Setup
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Shutdown(void)
{
	FreeMemory(botinputs);
	botinputs = NULL;
}	//end of the function EA_Shutdown
