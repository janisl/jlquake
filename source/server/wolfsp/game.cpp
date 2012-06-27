//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../server.h"
#include "../tech3/local.h"
#include "local.h"
#include "g_public.h"
#include "../botlib/public.h"

// these functions must be used instead of pointer arithmetic, because
// the game allocates gentities with private information after the server shared part
int SVWS_NumForGentity(const wssharedEntity_t* ent)
{
	return ((byte*)ent - (byte*)sv.ws_gentities) / sv.q3_gentitySize;
}

wssharedEntity_t* SVWS_GentityNum(int num)
{
	return (wssharedEntity_t*)((byte*)sv.ws_gentities + sv.q3_gentitySize * num);
}

wsplayerState_t* SVWS_GameClientNum(int num)
{
	return (wsplayerState_t*)((byte*)sv.ws_gameClients + sv.q3_gameClientSize * (num));
}

q3svEntity_t* SVWS_SvEntityForGentity(const wssharedEntity_t* gEnt)
{
	if (!gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES_Q3)
	{
		common->Error("SVWS_SvEntityForGentity: bad gEnt");
	}
	return &sv.q3_svEntities[gEnt->s.number];
}

idEntity3* SVWS_EntityForGentity(const wssharedEntity_t* gEnt)
{
	if (!gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES_Q3)
	{
		common->Error("SVWS_SvEntityForGentity: bad gEnt");
	}
	return sv.q3_entities[gEnt->s.number];
}

wssharedEntity_t* SVWS_GEntityForSvEntity(const q3svEntity_t* svEnt)
{
	int num = svEnt - sv.q3_svEntities;
	return SVWS_GentityNum(num);
}

void SVWS_UnlinkEntity(wssharedEntity_t* gEnt)
{
	SVT3_UnlinkEntity(SVWS_EntityForGentity(gEnt), SVWS_SvEntityForGentity(gEnt));
}

void SVWS_LinkEntity(wssharedEntity_t* gEnt)
{
	SVT3_LinkEntity(SVWS_EntityForGentity(gEnt), SVWS_SvEntityForGentity(gEnt));
}

//	Returns a headnode that can be used for testing or clipping to a
// given entity.  If the entity is a bsp model, the headnode will
// be returned, otherwise a custom box tree will be constructed.
clipHandle_t SVWS_ClipHandleForEntity(const wssharedEntity_t* gent)
{
	return SVT3_ClipHandleForEntity(SVWS_EntityForGentity(gent));
}

bool SVWS_BotVisibleFromPos(vec3_t srcpos, int srcnum, vec3_t destpos, int destnum, bool updateVisPos)
{
	return VM_Call(gvm, WSAICAST_VISIBLEFROMPOS, (qintptr)srcpos, srcnum, (qintptr)destpos, destnum, updateVisPos);
}

bool SVWS_BotCheckAttackAtPos(int entnum, int enemy, vec3_t pos, bool ducking, bool allowHitWorld)
{
	return VM_Call(gvm, WSAICAST_CHECKATTACKATPOS, entnum, enemy, (qintptr)pos, ducking, allowHitWorld);
}

static void SVWS_LocateGameData(wssharedEntity_t* gEnts, int numGEntities, int sizeofGEntity_t,
	wsplayerState_t* clients, int sizeofGameClient)
{
	sv.ws_gentities = gEnts;
	sv.q3_gentitySize = sizeofGEntity_t;
	sv.q3_num_entities = numGEntities;

	for (int i = 0; i < MAX_GENTITIES_Q3; i++)
	{
		SVT3_EntityNum(i)->SetGEntity(SVWS_GentityNum(i));
	}

	sv.ws_gameClients = clients;
	sv.q3_gameClientSize = sizeofGameClient;
}

static bool SVWS_EntityContact(const vec3_t mins, const vec3_t maxs, const wssharedEntity_t* gEnt, const int capsule)
{
	return SVT3_EntityContact(mins, maxs, SVWS_EntityForGentity(gEnt), capsule);
}

static void SVWS_SetBrushModel(wssharedEntity_t* ent, const char* name)
{
	SVT3_SetBrushModel(SVWS_EntityForGentity(ent), SVWS_SvEntityForGentity(ent), name);
}

static void SVWS_AdjustAreaPortalState(wssharedEntity_t* ent, qboolean open)
{
	SVT3_AdjustAreaPortalState(SVWS_SvEntityForGentity(ent), open);
}

static void SVWS_GetUsercmd(int clientNum, wsusercmd_t* cmd)
{
	if (clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		common->Error("SVWS_GetUsercmd: bad clientNum:%i", clientNum);
	}
	*cmd = svs.clients[clientNum].ws_lastUsercmd;
}

//	The module is making a system call
qintptr SVWS_GameSystemCalls(qintptr* args)
{
	switch (args[0])
	{
	case WSG_PRINT:
		common->Printf("%s", (char*)VMA(1));
		return 0;
	case WSG_ERROR:
		common->Error("%s", (char*)VMA(1));
		return 0;
//-----------
	case WSG_MILLISECONDS:
		return Sys_Milliseconds();
	case WSG_CVAR_REGISTER:
		Cvar_Register((vmCvar_t*)VMA(1), (char*)VMA(2), (char*)VMA(3), args[4]);
		return 0;
	case WSG_CVAR_UPDATE:
		Cvar_Update((vmCvar_t*)VMA(1));
		return 0;
	case WSG_CVAR_SET:
		Cvar_Set((const char*)VMA(1), (const char*)VMA(2));
		return 0;
	case WSG_CVAR_VARIABLE_INTEGER_VALUE:
		return Cvar_VariableIntegerValue((const char*)VMA(1));
	case WSG_CVAR_VARIABLE_STRING_BUFFER:
		Cvar_VariableStringBuffer((char*)VMA(1), (char*)VMA(2), args[3]);
		return 0;
	case WSG_ARGC:
		return Cmd_Argc();
	case WSG_ARGV:
		Cmd_ArgvBuffer(args[1], (char*)VMA(2), args[3]);
		return 0;
	case WSG_SEND_CONSOLE_COMMAND:
		Cbuf_ExecuteText(args[1], (char*)VMA(2));
		return 0;

	case WSG_FS_FOPEN_FILE:
		return FS_FOpenFileByMode((char*)VMA(1), (fileHandle_t*)VMA(2), (fsMode_t)args[3]);
	case WSG_FS_READ:
		FS_Read(VMA(1), args[2], args[3]);
		return 0;
	case WSG_FS_WRITE:
		return FS_Write(VMA(1), args[2], args[3]);
	case WSG_FS_RENAME:
		FS_Rename((char*)VMA(1), (char*)VMA(2));
		return 0;
	case WSG_FS_FCLOSE_FILE:
		FS_FCloseFile(args[1]);
		return 0;
//-----------
	case WSG_FS_GETFILELIST:
		return FS_GetFileList((char*)VMA(1), (char*)VMA(2), (char*)VMA(3), args[4]);

	case WSG_LOCATE_GAME_DATA:
		SVWS_LocateGameData((wssharedEntity_t*)VMA(1), args[2], args[3], (wsplayerState_t*)VMA(4), args[5]);
		return 0;
//-----------
	case WSG_LINKENTITY:
		SVWS_LinkEntity((wssharedEntity_t*)VMA(1));
		return 0;
	case WSG_UNLINKENTITY:
		SVWS_UnlinkEntity((wssharedEntity_t*)VMA(1));
		return 0;
	case WSG_ENTITIES_IN_BOX:
		return SVT3_AreaEntities((float*)VMA(1), (float*)VMA(2), (int*)VMA(3), args[4]);
	case WSG_ENTITY_CONTACT:
		return SVWS_EntityContact((float*)VMA(1), (float*)VMA(2), (wssharedEntity_t*)VMA(3), false);
	case WSG_ENTITY_CONTACTCAPSULE:
		return SVWS_EntityContact((float*)VMA(1), (float*)VMA(2), (wssharedEntity_t*)VMA(3), true);
	case WSG_TRACE:
		SVT3_Trace((q3trace_t*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4), (float*)VMA(5), args[6], args[7], false);
		return 0;
	case WSG_TRACECAPSULE:
		SVT3_Trace((q3trace_t*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4), (float*)VMA(5), args[6], args[7], true);
		return 0;
	case WSG_POINT_CONTENTS:
		return SVT3_PointContents((float*)VMA(1), args[2]);
	case WSG_SET_BRUSH_MODEL:
		SVWS_SetBrushModel((wssharedEntity_t*)VMA(1), (char*)VMA(2));
		return 0;
	case WSG_IN_PVS:
		return SVT3_inPVS((float*)VMA(1), (float*)VMA(2));
	case WSG_IN_PVS_IGNORE_PORTALS:
		return SVT3_inPVSIgnorePortals((float*)VMA(1), (float*)VMA(2));

//-----------
	case WSG_GET_SERVERINFO:
		SVT3_GetServerinfo((char*)VMA(1), args[2]);
		return 0;
	case WSG_ADJUST_AREA_PORTAL_STATE:
		SVWS_AdjustAreaPortalState((wssharedEntity_t*)VMA(1), args[2]);
		return 0;
	case WSG_AREAS_CONNECTED:
		return CM_AreasConnected(args[1], args[2]);

//-----------

	case WSG_GET_USERCMD:
		SVWS_GetUsercmd(args[1], (wsusercmd_t*)VMA(2));
		return 0;
	case WSG_GET_ENTITY_TOKEN:
		return SVT3_GetEntityToken((char*)VMA(1), args[2]);

	case WSG_DEBUG_POLYGON_CREATE:
		return BotImport_DebugPolygonCreate(args[1], args[2], (vec3_t*)VMA(3));
	case WSG_DEBUG_POLYGON_DELETE:
		BotImport_DebugPolygonDelete(args[1]);
		return 0;
//-----------
	case WSBOTLIB_LIBVAR_SET:
		return BotLibVarSet((char*)VMA(1), (char*)VMA(2));
	case WSBOTLIB_LIBVAR_GET:
		return BotLibVarGet((char*)VMA(1), (char*)VMA(2), args[3]);

	case WSBOTLIB_PC_ADD_GLOBAL_DEFINE:
		return PC_AddGlobalDefine((char*)VMA(1));
	case WSBOTLIB_PC_LOAD_SOURCE:
		return PC_LoadSourceHandle((char*)VMA(1));
	case WSBOTLIB_PC_FREE_SOURCE:
		return PC_FreeSourceHandle(args[1]);
	case WSBOTLIB_PC_READ_TOKEN:
		return PC_ReadTokenHandleQ3(args[1], (q3pc_token_t*)VMA(2));
	case WSBOTLIB_PC_SOURCE_FILE_AND_LINE:
		return PC_SourceFileAndLine(args[1], (char*)VMA(2), (int*)VMA(3));

	case WSBOTLIB_START_FRAME:
		return BotLibStartFrame(VMF(1));
	case WSBOTLIB_LOAD_MAP:
		return BotLibLoadMap((char*)VMA(1));
	case WSBOTLIB_UPDATENTITY:
		return BotLibUpdateEntity(args[1], (bot_entitystate_t*)VMA(2));
	case WSBOTLIB_TEST:
		return 0;

//-----------

	case WSBOTLIB_AAS_ENTITY_INFO:
		AAS_EntityInfo(args[1], (aas_entityinfo_t*)VMA(2));
		return 0;

	case WSBOTLIB_AAS_INITIALIZED:
		return AAS_Initialized();
	case WSBOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX:
		AAS_PresenceTypeBoundingBox(args[1], (float*)VMA(2), (float*)VMA(3));
		return 0;
	case WSBOTLIB_AAS_TIME:
		return FloatAsInt(AAS_Time());

	// Ridah
	case WSBOTLIB_AAS_SETCURRENTWORLD:
		AAS_SetCurrentWorld(args[1]);
		return 0;
	// done.

	case WSBOTLIB_AAS_POINT_AREA_NUM:
		return AAS_PointAreaNum((float*)VMA(1));
	case WSBOTLIB_AAS_TRACE_AREAS:
		return AAS_TraceAreas((float*)VMA(1), (float*)VMA(2), (int*)VMA(3), (vec3_t*)VMA(4), args[5]);

	case WSBOTLIB_AAS_POINT_CONTENTS:
		return AAS_PointContents((float*)VMA(1));
	case WSBOTLIB_AAS_NEXT_BSP_ENTITY:
		return AAS_NextBSPEntity(args[1]);
	case WSBOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY:
		return AAS_ValueForBSPEpairKey(args[1], (char*)VMA(2), (char*)VMA(3), args[4]);
	case WSBOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY:
		return AAS_VectorForBSPEpairKey(args[1], (char*)VMA(2), (float*)VMA(3));
	case WSBOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY:
		return AAS_FloatForBSPEpairKey(args[1], (char*)VMA(2), (float*)VMA(3));
	case WSBOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY:
		return AAS_IntForBSPEpairKey(args[1], (char*)VMA(2), (int*)VMA(3));

	case WSBOTLIB_AAS_AREA_REACHABILITY:
		return AAS_AreaReachability(args[1]);

	case WSBOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA:
		return AAS_AreaTravelTimeToGoalArea(args[1], (float*)VMA(2), args[3], args[4]);

	case WSBOTLIB_AAS_SWIMMING:
		return AAS_Swimming((float*)VMA(1));
	case WSBOTLIB_AAS_PREDICT_CLIENT_MOVEMENT:
		return AAS_PredictClientMovementWolf((struct aas_clientmove_rtcw_t*)VMA(1), args[2], (float*)VMA(3), args[4], args[5],
			(float*)VMA(6), (float*)VMA(7), args[8], args[9], VMF(10), args[11], args[12], args[13]);

	// Ridah, route-tables
	case WSBOTLIB_AAS_RT_SHOWROUTE:
		return 0;

	case WSBOTLIB_AAS_RT_GETHIDEPOS:
		return AAS_RT_GetHidePos((float*)VMA(1), args[2], args[3], (float*)VMA(4), args[5], args[6], (float*)VMA(7));

	case WSBOTLIB_AAS_FINDATTACKSPOTWITHINRANGE:
		return AAS_FindAttackSpotWithinRange(args[1], args[2], args[3], VMF(4), args[5], (float*)VMA(6));

	case WSBOTLIB_AAS_GETROUTEFIRSTVISPOS:
		return AAS_GetRouteFirstVisPos((float*)VMA(1), (float*)VMA(2), args[3], (float*)VMA(4));

	case WSBOTLIB_AAS_SETAASBLOCKINGENTITY:
		AAS_SetAASBlockingEntity((float*)VMA(1), (float*)VMA(2), args[3]);
		return 0;
	// done.

	case WSBOTLIB_EA_SAY:
		EA_Say(args[1], (char*)VMA(2));
		return 0;
	case WSBOTLIB_EA_SAY_TEAM:
		EA_SayTeam(args[1], (char*)VMA(2));
		return 0;
	case WSBOTLIB_EA_USE_ITEM:
		EA_UseItem(args[1], (char*)VMA(2));
		return 0;
	case WSBOTLIB_EA_DROP_ITEM:
		EA_DropItem(args[1], (char*)VMA(2));
		return 0;
	case WSBOTLIB_EA_USE_INV:
		EA_UseInv(args[1], (char*)VMA(2));
		return 0;
	case WSBOTLIB_EA_DROP_INV:
		EA_DropInv(args[1], (char*)VMA(2));
		return 0;
	case WSBOTLIB_EA_GESTURE:
		EA_Gesture(args[1]);
		return 0;
	case WSBOTLIB_EA_COMMAND:
		EA_Command(args[1], (char*)VMA(2));
		return 0;

	case WSBOTLIB_EA_SELECT_WEAPON:
		EA_SelectWeapon(args[1], args[2]);
		return 0;
	case WSBOTLIB_EA_TALK:
		EA_Talk(args[1]);
		return 0;
	case WSBOTLIB_EA_ATTACK:
		EA_Attack(args[1]);
		return 0;
	case WSBOTLIB_EA_RELOAD:
		EA_Reload(args[1]);
		return 0;
	case WSBOTLIB_EA_USE:
		EA_Use(args[1]);
		return 0;
	case WSBOTLIB_EA_RESPAWN:
		EA_Respawn(args[1]);
		return 0;
	case WSBOTLIB_EA_JUMP:
		EA_Jump(args[1]);
		return 0;
	case WSBOTLIB_EA_DELAYED_JUMP:
		EA_DelayedJump(args[1]);
		return 0;
	case WSBOTLIB_EA_CROUCH:
		EA_Crouch(args[1]);
		return 0;
	case WSBOTLIB_EA_MOVE_UP:
		EA_MoveUp(args[1]);
		return 0;
	case WSBOTLIB_EA_MOVE_DOWN:
		EA_MoveDown(args[1]);
		return 0;
	case WSBOTLIB_EA_MOVE_FORWARD:
		EA_MoveForward(args[1]);
		return 0;
	case WSBOTLIB_EA_MOVE_BACK:
		EA_MoveBack(args[1]);
		return 0;
	case WSBOTLIB_EA_MOVE_LEFT:
		EA_MoveLeft(args[1]);
		return 0;
	case WSBOTLIB_EA_MOVE_RIGHT:
		EA_MoveRight(args[1]);
		return 0;
	case WSBOTLIB_EA_MOVE:
		EA_Move(args[1], (float*)VMA(2), VMF(3));
		return 0;
	case WSBOTLIB_EA_VIEW:
		EA_View(args[1], (float*)VMA(2));
		return 0;

	case WSBOTLIB_EA_END_REGULAR:
		return 0;
	case WSBOTLIB_EA_GET_INPUT:
		EA_GetInput(args[1], VMF(2), (bot_input_t*)VMA(3));
		return 0;
	case WSBOTLIB_EA_RESET_INPUT:
		EA_ResetInputWolf(args[1], (bot_input_t*)VMA(2));
		return 0;

	case WSBOTLIB_AI_LOAD_CHARACTER:
		return BotLoadCharacter((char*)VMA(1), args[2]);
	case WSBOTLIB_AI_FREE_CHARACTER:
		BotFreeCharacter(args[1]);
		return 0;
	case WSBOTLIB_AI_CHARACTERISTIC_FLOAT:
		return FloatAsInt(Characteristic_Float(args[1], args[2]));
	case WSBOTLIB_AI_CHARACTERISTIC_BFLOAT:
		return FloatAsInt(Characteristic_BFloat(args[1], args[2], VMF(3), VMF(4)));
	case WSBOTLIB_AI_CHARACTERISTIC_INTEGER:
		return Characteristic_Integer(args[1], args[2]);
	case WSBOTLIB_AI_CHARACTERISTIC_BINTEGER:
		return Characteristic_BInteger(args[1], args[2], args[3], args[4]);
	case WSBOTLIB_AI_CHARACTERISTIC_STRING:
		Characteristic_String(args[1], args[2], (char*)VMA(3), args[4]);
		return 0;

	case WSBOTLIB_AI_ALLOC_CHAT_STATE:
		return BotAllocChatState();
	case WSBOTLIB_AI_FREE_CHAT_STATE:
		BotFreeChatState(args[1]);
		return 0;
	case WSBOTLIB_AI_QUEUE_CONSOLE_MESSAGE:
		BotQueueConsoleMessage(args[1], args[2], (char*)VMA(3));
		return 0;
	case WSBOTLIB_AI_REMOVE_CONSOLE_MESSAGE:
		BotRemoveConsoleMessage(args[1], args[2]);
		return 0;
	case WSBOTLIB_AI_NEXT_CONSOLE_MESSAGE:
		return BotNextConsoleMessageWolf(args[1], (struct bot_consolemessage_wolf_t*)VMA(2));
	case WSBOTLIB_AI_NUM_CONSOLE_MESSAGE:
		return BotNumConsoleMessages(args[1]);
	case WSBOTLIB_AI_INITIAL_CHAT:
		BotInitialChat(args[1], (char*)VMA(2), args[3], (char*)VMA(4), (char*)VMA(5), (char*)VMA(6), (char*)VMA(7), (char*)VMA(8), (char*)VMA(9), (char*)VMA(10), (char*)VMA(11));
		return 0;
	case WSBOTLIB_AI_NUM_INITIAL_CHATS:
		return BotNumInitialChats(args[1], (char*)VMA(2));
	case WSBOTLIB_AI_REPLY_CHAT:
		return BotReplyChat(args[1], (char*)VMA(2), args[3], args[4], (char*)VMA(5), (char*)VMA(6), (char*)VMA(7), (char*)VMA(8), (char*)VMA(9), (char*)VMA(10), (char*)VMA(11), (char*)VMA(12));
	case WSBOTLIB_AI_CHAT_LENGTH:
		return BotChatLength(args[1]);
	case WSBOTLIB_AI_ENTER_CHAT:
		BotEnterChatWolf(args[1], args[2], args[3]);
		return 0;
	case WSBOTLIB_AI_GET_CHAT_MESSAGE:
		BotGetChatMessage(args[1], (char*)VMA(2), args[3]);
		return 0;
	case WSBOTLIB_AI_STRING_CONTAINS:
		return StringContains((char*)VMA(1), (char*)VMA(2), args[3]);
	case WSBOTLIB_AI_FIND_MATCH:
		return BotFindMatchWolf((char*)VMA(1), (bot_match_wolf_t*)VMA(2), args[3]);
	case WSBOTLIB_AI_MATCH_VARIABLE:
		BotMatchVariableWolf((bot_match_wolf_t*)VMA(1), args[2], (char*)VMA(3), args[4]);
		return 0;
	case WSBOTLIB_AI_UNIFY_WHITE_SPACES:
		UnifyWhiteSpaces((char*)VMA(1));
		return 0;
	case WSBOTLIB_AI_REPLACE_SYNONYMS:
		BotReplaceSynonyms((char*)VMA(1), args[2]);
		return 0;
	case WSBOTLIB_AI_LOAD_CHAT_FILE:
		return BotLoadChatFile(args[1], (char*)VMA(2), (char*)VMA(3));
	case WSBOTLIB_AI_SET_CHAT_GENDER:
		BotSetChatGender(args[1], args[2]);
		return 0;
	case WSBOTLIB_AI_SET_CHAT_NAME:
		BotSetChatName(args[1], (char*)VMA(2), 0);
		return 0;

	case WSBOTLIB_AI_RESET_GOAL_STATE:
		BotResetGoalState(args[1]);
		return 0;
	case WSBOTLIB_AI_RESET_AVOID_GOALS:
		BotResetAvoidGoals(args[1]);
		return 0;
	case WSBOTLIB_AI_REMOVE_FROM_AVOID_GOALS:
		BotRemoveFromAvoidGoals(args[1], args[2]);
		return 0;
	case WSBOTLIB_AI_PUSH_GOAL:
		BotPushGoalQ3(args[1], (struct bot_goal_q3_t*)VMA(2));
		return 0;
	case WSBOTLIB_AI_POP_GOAL:
		BotPopGoal(args[1]);
		return 0;
	case WSBOTLIB_AI_EMPTY_GOAL_STACK:
		BotEmptyGoalStack(args[1]);
		return 0;
	case WSBOTLIB_AI_DUMP_AVOID_GOALS:
		BotDumpAvoidGoals(args[1]);
		return 0;
	case WSBOTLIB_AI_DUMP_GOAL_STACK:
		BotDumpGoalStack(args[1]);
		return 0;
	case WSBOTLIB_AI_GOAL_NAME:
		BotGoalName(args[1], (char*)VMA(2), args[3]);
		return 0;
	case WSBOTLIB_AI_GET_TOP_GOAL:
		return BotGetTopGoalQ3(args[1], (struct bot_goal_q3_t*)VMA(2));
	case WSBOTLIB_AI_GET_SECOND_GOAL:
		return BotGetSecondGoalQ3(args[1], (struct bot_goal_q3_t*)VMA(2));
	case WSBOTLIB_AI_CHOOSE_LTG_ITEM:
		return BotChooseLTGItem(args[1], (float*)VMA(2), (int*)VMA(3), args[4]);
	case WSBOTLIB_AI_CHOOSE_NBG_ITEM:
		return BotChooseNBGItemQ3(args[1], (float*)VMA(2), (int*)VMA(3), args[4], (struct bot_goal_q3_t*)VMA(5), VMF(6));
	case WSBOTLIB_AI_TOUCHING_GOAL:
		return BotTouchingGoalQ3((float*)VMA(1), (struct bot_goal_q3_t*)VMA(2));
	case WSBOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE:
		return BotItemGoalInVisButNotVisibleQ3(args[1], (float*)VMA(2), (float*)VMA(3), (struct bot_goal_q3_t*)VMA(4));
	case WSBOTLIB_AI_GET_LEVEL_ITEM_GOAL:
		return BotGetLevelItemGoalQ3(args[1], (char*)VMA(2), (struct bot_goal_q3_t*)VMA(3));
	case WSBOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL:
		return BotGetNextCampSpotGoalQ3(args[1], (struct bot_goal_q3_t*)VMA(2));
	case WSBOTLIB_AI_GET_MAP_LOCATION_GOAL:
		return BotGetMapLocationGoalQ3((char*)VMA(1), (struct bot_goal_q3_t*)VMA(2));
	case WSBOTLIB_AI_AVOID_GOAL_TIME:
		return FloatAsInt(BotAvoidGoalTime(args[1], args[2]));
	case WSBOTLIB_AI_INIT_LEVEL_ITEMS:
		BotInitLevelItems();
		return 0;
	case WSBOTLIB_AI_UPDATE_ENTITY_ITEMS:
		BotUpdateEntityItems();
		return 0;
	case WSBOTLIB_AI_LOAD_ITEM_WEIGHTS:
		return BotLoadItemWeights(args[1], (char*)VMA(2));
	case WSBOTLIB_AI_FREE_ITEM_WEIGHTS:
		BotFreeItemWeights(args[1]);
		return 0;
	case WSBOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC:
		BotInterbreedGoalFuzzyLogic(args[1], args[2], args[3]);
		return 0;
	case WSBOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC:
		return 0;
	case WSBOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC:
		BotMutateGoalFuzzyLogic(args[1], VMF(2));
		return 0;
	case WSBOTLIB_AI_ALLOC_GOAL_STATE:
		return BotAllocGoalState(args[1]);
	case WSBOTLIB_AI_FREE_GOAL_STATE:
		BotFreeGoalState(args[1]);
		return 0;

	case WSBOTLIB_AI_RESET_MOVE_STATE:
		BotResetMoveState(args[1]);
		return 0;
	case WSBOTLIB_AI_MOVE_TO_GOAL:
		BotMoveToGoalQ3((bot_moveresult_t*)VMA(1), args[2], (bot_goal_q3_t*)VMA(3), args[4]);
		return 0;
	case WSBOTLIB_AI_MOVE_IN_DIRECTION:
		return BotMoveInDirection(args[1], (float*)VMA(2), VMF(3), args[4]);
	case WSBOTLIB_AI_RESET_AVOID_REACH:
		BotResetAvoidReachAndMove(args[1]);
		return 0;
	case WSBOTLIB_AI_RESET_LAST_AVOID_REACH:
		BotResetLastAvoidReach(args[1]);
		return 0;
	case WSBOTLIB_AI_REACHABILITY_AREA:
		return BotReachabilityArea((float*)VMA(1), args[2]);
	case WSBOTLIB_AI_MOVEMENT_VIEW_TARGET:
		return BotMovementViewTargetQ3(args[1], (struct bot_goal_q3_t*)VMA(2), args[3], VMF(4), (float*)VMA(5));
	case WSBOTLIB_AI_PREDICT_VISIBLE_POSITION:
		return BotPredictVisiblePositionQ3((float*)VMA(1), args[2], (struct bot_goal_q3_t*)VMA(3), args[4], (float*)VMA(5));
	case WSBOTLIB_AI_ALLOC_MOVE_STATE:
		return BotAllocMoveState();
	case WSBOTLIB_AI_FREE_MOVE_STATE:
		BotFreeMoveState(args[1]);
		return 0;
	case WSBOTLIB_AI_INIT_MOVE_STATE:
		BotInitMoveStateQ3(args[1], (struct bot_initmove_q3_t*)VMA(2));
		return 0;
	// Ridah
	case WSBOTLIB_AI_INIT_AVOID_REACH:
		BotResetAvoidReach(args[1]);
		return 0;
	// done.

	case WSBOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON:
		return BotChooseBestFightWeapon(args[1], (int*)VMA(2));
	case WSBOTLIB_AI_GET_WEAPON_INFO:
		BotGetWeaponInfo(args[1], args[2], (weaponinfo_t*)VMA(3));
		return 0;
	case WSBOTLIB_AI_LOAD_WEAPON_WEIGHTS:
		return BotLoadWeaponWeights(args[1], (char*)VMA(2));
	case WSBOTLIB_AI_ALLOC_WEAPON_STATE:
		return BotAllocWeaponState();
	case WSBOTLIB_AI_FREE_WEAPON_STATE:
		BotFreeWeaponState(args[1]);
		return 0;
	case WSBOTLIB_AI_RESET_WEAPON_STATE:
		BotResetWeaponState(args[1]);
		return 0;

	case WSBOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION:
		return GeneticParentsAndChildSelection(args[1], (float*)VMA(2), (int*)VMA(3), (int*)VMA(4), (int*)VMA(5));

	case TRAP_MEMSET:
		memset(VMA(1), args[2], args[3]);
		return 0;

	case TRAP_MEMCPY:
		memcpy(VMA(1), VMA(2), args[3]);
		return 0;

	case TRAP_STRNCPY:
		String::NCpy((char*)VMA(1), (char*)VMA(2), args[3]);
		return args[1];

	case TRAP_SIN:
		return FloatAsInt(sin(VMF(1)));

	case TRAP_COS:
		return FloatAsInt(cos(VMF(1)));

	case TRAP_ATAN2:
		return FloatAsInt(atan2(VMF(1), VMF(2)));

	case TRAP_SQRT:
		return FloatAsInt(sqrt(VMF(1)));

	case TRAP_MATRIXMULTIPLY:
		MatrixMultiply((vec3_t*)VMA(1), (vec3_t*)VMA(2), (vec3_t*)VMA(3));
		return 0;

	case TRAP_ANGLEVECTORS:
		AngleVectors((float*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4));
		return 0;

	case TRAP_PERPENDICULARVECTOR:
		PerpendicularVector((float*)VMA(1), (float*)VMA(2));
		return 0;

	case TRAP_FLOOR:
		return FloatAsInt(floor(VMF(1)));

	case TRAP_CEIL:
		return FloatAsInt(ceil(VMF(1)));

	default:
		common->Error("Bad game system trap: %i", (int)args[0]);
	}
	return -1;
}
