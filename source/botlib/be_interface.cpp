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
 * name:		be_interface.c // bk010221 - FIXME - DEAD code elimination
 *
 * desc:		bot library interface
 *
 * $Archive: /MissionPack/code/botlib/be_interface.c $
 *
 *****************************************************************************/

#include "../common/qcommon.h"
#include <time.h>
#include "botlib.h"
#include "../server/server.h"
#include "../server/botlib/local.h"

#include "be_ea.h"
#include "../server/botlib/ai_weight.h"
#include "be_ai_move.h"
#include "be_ai_chat.h"

botlib_export_t be_botlib_export;

/*
============
Init_EA_Export
============
*/
static void Init_EA_Export(ea_export_t* ea)
{
	//ClientCommand elementary actions
	ea->EA_Command = EA_Command;
	ea->EA_Say = EA_Say;
	ea->EA_SayTeam = EA_SayTeam;
}


/*
============
Init_AI_Export
============
*/
static void Init_AI_Export(ai_export_t* ai)
{
	//-----------------------------------
	// be_ai_chat.h
	//-----------------------------------
	ai->BotEnterChat = BotEnterChat;
	//-----------------------------------
	// be_ai_move.h
	//-----------------------------------
	ai->BotMoveToGoal = BotMoveToGoal;
}


/*
============
GetBotLibAPI
============
*/
botlib_export_t* GetBotLibAPI(int apiVersion)
{
	Com_Memset(&be_botlib_export, 0, sizeof(be_botlib_export));

	if (apiVersion != BOTLIB_API_VERSION)
	{
		BotImport_Print(PRT_ERROR, "Mismatched BOTLIB_API_VERSION: expected %i, got %i\n", BOTLIB_API_VERSION, apiVersion);
		return NULL;
	}

	Init_EA_Export(&be_botlib_export.ea);
	Init_AI_Export(&be_botlib_export.ai);

	return &be_botlib_export;
}
