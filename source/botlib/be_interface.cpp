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
#include "be_interface.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"

#include "be_ea.h"
#include "../server/botlib/ai_weight.h"
#include "be_ai_goal.h"
#include "be_ai_move.h"
#include "be_ai_chat.h"

botlib_export_t be_botlib_export;
botlib_import_t botimport;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibStartFrame(float time)
{
	if (!IsBotLibSetup("BotStartFrame"))
	{
		return BLERR_LIBRARYNOTSETUP;
	}
	return AAS_StartFrame(time);
}	//end of the function Export_BotLibStartFrame
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibLoadMap(const char* mapname)
{
#ifdef DEBUG
	int starttime = Sys_Milliseconds();
#endif
	int errnum;

	if (!IsBotLibSetup("BotLoadMap"))
	{
		return BLERR_LIBRARYNOTSETUP;
	}
	//
	BotImport_Print(PRT_MESSAGE, "------------ Map Loading ------------\n");
	//startup AAS for the current map, model and sound index
	errnum = AAS_LoadMap(mapname);
	if (errnum != BLERR_NOERROR)
	{
		return errnum;
	}
	//initialize the items in the level
	BotInitLevelItems();		//be_ai_goal.h
	BotSetBrushModelTypes();	//be_ai_move.h
	//
	BotImport_Print(PRT_MESSAGE, "-------------------------------------\n");
#ifdef DEBUG
	BotImport_Print(PRT_MESSAGE, "map loaded in %d msec\n", Sys_Milliseconds() - starttime);
#endif
	//
	return BLERR_NOERROR;
}	//end of the function Export_BotLibLoadMap

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
	// be_ai_goal.h
	//-----------------------------------
	ai->BotChooseLTGItem = BotChooseLTGItem;
	ai->BotChooseNBGItem = BotChooseNBGItem;
	ai->BotItemGoalInVisButNotVisible = BotItemGoalInVisButNotVisible;
	ai->BotInitLevelItems = BotInitLevelItems;
	ai->BotUpdateEntityItems = BotUpdateEntityItems;
	//-----------------------------------
	// be_ai_move.h
	//-----------------------------------
	ai->BotMoveToGoal = BotMoveToGoal;
	ai->BotMoveInDirection = BotMoveInDirection;
	ai->BotReachabilityArea = BotReachabilityArea;
	ai->BotPredictVisiblePosition = BotPredictVisiblePosition;
}


/*
============
GetBotLibAPI
============
*/
botlib_export_t* GetBotLibAPI(int apiVersion, botlib_import_t* import)
{
	qassert(import);	// bk001129 - this wasn't set for baseq3/
	botimport = *import;

	Com_Memset(&be_botlib_export, 0, sizeof(be_botlib_export));

	if (apiVersion != BOTLIB_API_VERSION)
	{
		BotImport_Print(PRT_ERROR, "Mismatched BOTLIB_API_VERSION: expected %i, got %i\n", BOTLIB_API_VERSION, apiVersion);
		return NULL;
	}

	Init_EA_Export(&be_botlib_export.ea);
	Init_AI_Export(&be_botlib_export.ai);

	be_botlib_export.BotLibStartFrame = Export_BotLibStartFrame;
	be_botlib_export.BotLibLoadMap = Export_BotLibLoadMap;

	return &be_botlib_export;
}
