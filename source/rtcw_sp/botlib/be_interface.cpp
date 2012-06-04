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
 * name:		be_interface.c
 *
 * desc:		bot library interface
 *
 *
 *****************************************************************************/

#include "../game/q_shared.h"
#include "../game/botlib.h"
#include "be_interface.h"
#include "../game/be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"

#include "../game/be_ea.h"
#include "../../server/botlib/ai_weight.h"
#include "../game/be_ai_goal.h"
#include "../game/be_ai_move.h"
#include "../game/be_ai_chat.h"

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
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_NearestHideArea(int srcnum, vec3_t origin, int areanum, int enemynum, vec3_t enemyorigin, int enemyareanum, int travelflags);

int AAS_FindAttackSpotWithinRange(int srcnum, int rangenum, int enemynum, float rangedist, int travelflags, float* outpos);

qboolean AAS_GetRouteFirstVisPos(vec3_t srcpos, vec3_t destpos, int travelflags, vec3_t retpos);

/*
============
Init_AAS_Export
============
*/
static void Init_AAS_Export(aas_export_t* aas)
{
	//--------------------------------------------
	// be_aas_move.c
	//--------------------------------------------
	aas->AAS_Swimming = AAS_Swimming;
	aas->AAS_PredictClientMovement = AAS_PredictClientMovement;

	// Ridah, route-tables
	//--------------------------------------------
	// be_aas_routetable.c
	//--------------------------------------------
	aas->AAS_RT_GetHidePos = AAS_RT_GetHidePos;
	aas->AAS_FindAttackSpotWithinRange = AAS_FindAttackSpotWithinRange;
	aas->AAS_GetRouteFirstVisPos = AAS_GetRouteFirstVisPos;
}


/*
============
Init_EA_Export
============
*/
static void Init_EA_Export(ea_export_t* ea)
{
	//ClientCommand elementary actions
	ea->EA_Say = EA_Say;
	ea->EA_SayTeam = EA_SayTeam;
	ea->EA_UseItem = EA_UseItem;
	ea->EA_DropItem = EA_DropItem;
	ea->EA_UseInv = EA_UseInv;
	ea->EA_DropInv = EA_DropInv;
	ea->EA_Command = EA_Command;
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
	botimport = *import;

	memset(&be_botlib_export, 0, sizeof(be_botlib_export));

	if (apiVersion != BOTLIB_API_VERSION)
	{
		BotImport_Print(PRT_ERROR, "Mismatched BOTLIB_API_VERSION: expected %i, got %i\n", BOTLIB_API_VERSION, apiVersion);
		return NULL;
	}

	Init_AAS_Export(&be_botlib_export.aas);
	Init_EA_Export(&be_botlib_export.ea);
	Init_AI_Export(&be_botlib_export.ai);

	be_botlib_export.BotLibStartFrame = Export_BotLibStartFrame;
	be_botlib_export.BotLibLoadMap = Export_BotLibLoadMap;

	return &be_botlib_export;
}
