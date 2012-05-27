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


/*****************************************************************************
 * name:		be_interface.c
 *
 * desc:		bot library interface
 *
 *
 *****************************************************************************/

#include "../game/q_shared.h"
#include "l_memory.h"
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

//library globals in a structure
botlib_globals_t botlibglobals;

botlib_export_t be_botlib_export;
botlib_import_t botimport;
//qtrue if the library is setup
int botlibsetup = qfalse;

//===========================================================================
//
// several functions used by the exported functions
//
//===========================================================================

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean ValidClientNumber(int num, char* str)
{
	if (num < 0 || num > botlibglobals.maxclients)
	{
		//weird: the disabled stuff results in a crash
		BotImport_Print(PRT_ERROR, "%s: invalid client number %d, [0, %d]\n",
			str, num, botlibglobals.maxclients);
		return qfalse;
	}	//end if
	return qtrue;
}	//end of the function BotValidateClientNumber
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean ValidEntityNumber(int num, const char* str)
{
	if (num < 0 || num > botlibglobals.maxentities)
	{
		BotImport_Print(PRT_ERROR, "%s: invalid entity number %d, [0, %d]\n",
			str, num, botlibglobals.maxentities);
		return qfalse;
	}	//end if
	return qtrue;
}	//end of the function BotValidateClientNumber
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean BotLibSetup(const char* str)
{
//	return qtrue;

	if (!botlibglobals.botlibsetup)
	{
		BotImport_Print(PRT_ERROR, "%s: bot library used before being setup\n", str);
		return qfalse;
	}	//end if
	return qtrue;
}	//end of the function BotLibSetup
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibSetup(void)
{
	int errnum;

	bot_developer = LibVarGetValue("bot_developer");
	Log_Open("botlib.log");
	//
	BotImport_Print(PRT_MESSAGE, "------- BotLib Initialization -------\n");
	//
	botlibglobals.maxclients = (int)LibVarValue("maxclients", "128");
	botlibglobals.maxentities = (int)LibVarValue("maxentities", "1024");

	errnum = AAS_Setup();			//be_aas_main.c
	if (errnum != BLERR_NOERROR)
	{
		return errnum;
	}
	errnum = EA_Setup();			//be_ea.c
	if (errnum != BLERR_NOERROR)
	{
		return errnum;
	}
//	errnum = BotSetupChatAI();		//be_ai_chat.c
//	if (errnum != BLERR_NOERROR) return errnum;
//	errnum = BotSetupMoveAI();		//be_ai_move.c
//	if (errnum != BLERR_NOERROR) return errnum;

	botlibsetup = qtrue;
	botlibglobals.botlibsetup = qtrue;

	return BLERR_NOERROR;
}	//end of the function Export_BotLibSetup
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibShutdown(void)
{
	static int recursive = 0;

	if (!BotLibSetup("BotLibShutdown"))
	{
		return BLERR_LIBRARYNOTSETUP;
	}
	//
	if (recursive)
	{
		return BLERR_NOERROR;
	}
	recursive = 1;
	// shutdown all AI subsystems
	BotShutdownChatAI();		//be_ai_chat.c
	BotShutdownMoveAI();		//be_ai_move.c
	BotShutdownGoalAI();		//be_ai_goal.c
	BotShutdownWeaponAI();		//be_ai_weap.c
	BotShutdownWeights();		//be_ai_weight.c
	BotShutdownCharacters();	//be_ai_char.c
	// shutdown AAS
	AAS_Shutdown();
	// shutdown bot elemantary actions
	EA_Shutdown();
	// free all libvars
	LibVarDeAllocAll();
	// remove all global defines from the pre compiler
	PC_RemoveAllGlobalDefines();
	// shut down library log file
	Log_Shutdown();
	//
	botlibsetup = qfalse;
	botlibglobals.botlibsetup = qfalse;
	recursive = 0;
	// print any files still open
	PC_CheckOpenSourceHandles();
	return BLERR_NOERROR;
}	//end of the function Export_BotLibShutdown
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibStartFrame(float time)
{
	if (!BotLibSetup("BotStartFrame"))
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

	if (!BotLibSetup("BotLoadMap"))
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
int Export_BotLibUpdateEntity(int ent, bot_entitystate_t* state)
{
	if (!BotLibSetup("BotUpdateEntity"))
	{
		return BLERR_LIBRARYNOTSETUP;
	}
	if (!ValidEntityNumber(ent, "BotUpdateEntity"))
	{
		return WOLFBLERR_INVALIDENTITYNUMBER;
	}

	return AAS_UpdateEntity(ent, state);
}	//end of the function Export_BotLibUpdateEntity
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_NearestHideArea(int srcnum, vec3_t origin, int areanum, int enemynum, vec3_t enemyorigin, int enemyareanum, int travelflags);

int AAS_FindAttackSpotWithinRange(int srcnum, int rangenum, int enemynum, float rangedist, int travelflags, float* outpos);

void AAS_SetAASBlockingEntity(vec3_t absmin, vec3_t absmax, qboolean blocking);

/*
============
Init_AAS_Export
============
*/
static void Init_AAS_Export(aas_export_t* aas)
{
	//--------------------------------------------
	// be_aas_main.c
	//--------------------------------------------
	aas->AAS_PresenceTypeBoundingBox = AAS_PresenceTypeBoundingBox;
	//--------------------------------------------
	// be_aas_sample.c
	//--------------------------------------------
	aas->AAS_PointAreaNum = AAS_PointAreaNum;
	aas->AAS_TraceAreas = AAS_TraceAreas;
	//--------------------------------------------
	// be_aas_bspq3.c
	//--------------------------------------------
	aas->AAS_PointContents = AAS_PointContents;
	//--------------------------------------------
	// be_aas_route.c
	//--------------------------------------------
	aas->AAS_AreaTravelTimeToGoalArea = AAS_AreaTravelTimeToGoalArea;
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
	aas->AAS_SetAASBlockingEntity = AAS_SetAASBlockingEntity;
	// done.
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
	ea->EA_Gesture = EA_Gesture;
	ea->EA_Command = EA_Command;
	ea->EA_SelectWeapon = EA_SelectWeapon;
	ea->EA_Talk = EA_Talk;
	ea->EA_Attack = EA_Attack;
	ea->EA_Reload = EA_Reload;
	ea->EA_Use = EA_Use;
	ea->EA_Respawn = EA_Respawn;
	ea->EA_Jump = EA_Jump;
	ea->EA_DelayedJump = EA_DelayedJump;
	ea->EA_Crouch = EA_Crouch;
	ea->EA_MoveUp = EA_MoveUp;
	ea->EA_MoveDown = EA_MoveDown;
	ea->EA_MoveForward = EA_MoveForward;
	ea->EA_MoveBack = EA_MoveBack;
	ea->EA_MoveLeft = EA_MoveLeft;
	ea->EA_MoveRight = EA_MoveRight;
	ea->EA_Move = EA_Move;
	ea->EA_View = EA_View;
	ea->EA_GetInput = EA_GetInput;
	ea->EA_EndRegular = EA_EndRegular;
	ea->EA_ResetInput = EA_ResetInput;
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
	ai->BotRemoveFromAvoidGoals = BotRemoveFromAvoidGoals;
	ai->BotDumpAvoidGoals = BotDumpAvoidGoals;
	ai->BotChooseLTGItem = BotChooseLTGItem;
	ai->BotChooseNBGItem = BotChooseNBGItem;
	ai->BotTouchingGoal = BotTouchingGoal;
	ai->BotItemGoalInVisButNotVisible = BotItemGoalInVisButNotVisible;
	ai->BotAvoidGoalTime = BotAvoidGoalTime;
	ai->BotInitLevelItems = BotInitLevelItems;
	ai->BotUpdateEntityItems = BotUpdateEntityItems;
	//-----------------------------------
	// be_ai_move.h
	//-----------------------------------
	ai->BotMoveToGoal = BotMoveToGoal;
	ai->BotMoveInDirection = BotMoveInDirection;
	ai->BotReachabilityArea = BotReachabilityArea;
	ai->BotMovementViewTarget = BotMovementViewTarget;
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

	be_botlib_export.BotLibSetup = Export_BotLibSetup;
	be_botlib_export.BotLibShutdown = Export_BotLibShutdown;

	be_botlib_export.BotLibStartFrame = Export_BotLibStartFrame;
	be_botlib_export.BotLibLoadMap = Export_BotLibLoadMap;
	be_botlib_export.BotLibUpdateEntity = Export_BotLibUpdateEntity;

	return &be_botlib_export;
}
