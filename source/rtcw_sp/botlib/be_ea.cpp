/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


/*****************************************************************************
 * name:		be_ea.c
 *
 * desc:		elementary actions
 *
 *
 *****************************************************************************/

#include "../game/q_shared.h"
#include "l_memory.h"
#include "../game/botlib.h"
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
void EA_Command(int client, const char* command)
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
    jumped = bi->actionflags & WOLFACTION_JUMP;
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
	memcpy(input, bi, sizeof(bot_input_t));

	/*
	bi->thinktime = 0;
	VectorClear(bi->dir);
	bi->speed = 0;
	jumped = bi->actionflags & WOLFACTION_JUMP;
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
void EA_ResetInput(int client, bot_input_t* init)
{
	bot_input_t* bi;
	int jumped = qfalse;

	bi = &botinputs[client];
	bi->actionflags &= ~ACTION_JUMPEDLASTFRAME;

	bi->thinktime = 0;
	VectorClear(bi->dir);
	bi->speed = 0;
	jumped = bi->actionflags & WOLFACTION_JUMP;
	bi->actionflags = 0;
	if (jumped)
	{
		bi->actionflags |= ACTION_JUMPEDLASTFRAME;
	}

	if (init)
	{
		memcpy(bi, init, sizeof(bot_input_t));
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
