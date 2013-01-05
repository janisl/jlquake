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

#include "../tech3/local.h"
#include "local.h"
#include "g_public.h"
#include "../botlib/public.h"
#include "../../common/Common.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"
#include "../../common/command_buffer.h"
#include "../../common/precompiler.h"
#include "../../common/system.h"

wmsharedEntity_t* SVWM_GentityNum(int num)
{
	return (wmsharedEntity_t*)((byte*)sv.wm_gentities + sv.q3_gentitySize * num);
}

wmplayerState_t* SVWM_GameClientNum(int num)
{
	return (wmplayerState_t*)((byte*)sv.wm_gameClients + sv.q3_gameClientSize * (num));
}

q3svEntity_t* SVWM_SvEntityForGentity(const wmsharedEntity_t* gEnt)
{
	if (!gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES_Q3)
	{
		common->Error("SVWM_SvEntityForGentity: bad gEnt");
	}
	return &sv.q3_svEntities[gEnt->s.number];
}

static idEntity3* SVWM_EntityForGentity(const wmsharedEntity_t* gEnt)
{
	if (!gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES_Q3)
	{
		common->Error("SVWM_SvEntityForGentity: bad gEnt");
	}
	return sv.q3_entities[gEnt->s.number];
}

void SVWM_ClientThink(client_t* cl, wmusercmd_t* cmd)
{
	cl->wm_lastUsercmd = *cmd;

	if (cl->state != CS_ACTIVE)
	{
		// may have been kicked during the last usercmd
		return;
	}

	VM_Call(gvm, WMGAME_CLIENT_THINK, cl - svs.clients);
}

bool SVWM_BotVisibleFromPos(vec3_t srcpos, int srcnum, vec3_t destpos, int destnum, bool updateVisPos)
{
	return VM_Call(gvm, WMAICAST_VISIBLEFROMPOS, (qintptr)srcpos, srcnum, (qintptr)destpos, destnum, updateVisPos);
}

bool SVWM_BotCheckAttackAtPos(int entnum, int enemy, vec3_t pos, bool ducking, bool allowHitWorld)
{
	return VM_Call(gvm, WMAICAST_CHECKATTACKATPOS, entnum, enemy, (qintptr)pos, ducking, allowHitWorld);
}

void SVWM_BotFrame(int time)
{
	VM_Call(gvm, WMBOTAI_START_FRAME, time);
}

void SVWM_GameClientDisconnect(client_t* drop)
{
	VM_Call(gvm, WMGAME_CLIENT_DISCONNECT, drop - svs.clients);
}

void SVWM_GameInit(int serverTime, int randomSeed, bool restart)
{
	VM_Call(gvm, WMGAME_INIT, serverTime, randomSeed, restart);
}

void SVWM_GameShutdown(bool restart)
{
	VM_Call(gvm, WMGAME_SHUTDOWN, restart);
}

bool SVWM_GameConsoleCommand()
{
	return VM_Call(gvm, WMGAME_CONSOLE_COMMAND);
}

void SVWM_SendMoveSpeedsToGame(int entnum, char* text)
{
	if (!gvm)
	{
		return;
	}
	VM_Call(gvm, WMGAME_RETRIEVE_MOVESPEEDS_FROM_CLIENT, entnum, text);
}

void SVWM_GameClientBegin(int clientNum)
{
	VM_Call(gvm, WMGAME_CLIENT_BEGIN, clientNum);
}

void SVWM_GameClientUserInfoChanged(int clientNum)
{
	VM_Call(gvm, WMGAME_CLIENT_USERINFO_CHANGED, clientNum);
}

const char* SVWM_GameClientConnect(int clientNum, bool firstTime, bool isBot)
{
	qintptr denied = VM_Call(gvm, WMGAME_CLIENT_CONNECT, clientNum, firstTime, isBot);
	if (denied)
	{
		// we can't just use VM_ArgPtr, because that is only valid inside a VM_Call
		return (const char*)VM_ExplicitArgPtr(gvm, denied);
	}
	return NULL;
}

void SVWM_GameClientCommand(int clientNum)
{
	VM_Call(gvm, WMGAME_CLIENT_COMMAND, clientNum);
}

void SVWM_GameRunFrame(int time)
{
	VM_Call(gvm, WMGAME_RUN_FRAME, time);
}

static void SVWM_LocateGameData(wmsharedEntity_t* gEnts, int numGEntities, int sizeofGEntity_t,
	wmplayerState_t* clients, int sizeofGameClient)
{
	sv.wm_gentities = gEnts;
	sv.q3_gentitySize = sizeofGEntity_t;
	sv.q3_num_entities = numGEntities;

	for (int i = 0; i < MAX_GENTITIES_Q3; i++)
	{
		SVT3_EntityNum(i)->SetGEntity(SVWM_GentityNum(i));
	}

	sv.wm_gameClients = clients;
	sv.q3_gameClientSize = sizeofGameClient;

	for (int i = 0; i < sv_maxclients->integer; i++)
	{
		SVT3_GameClientNum(i)->SetGEntity(SVWM_GameClientNum(i));
	}
}

static void SVWM_UnlinkEntity(wmsharedEntity_t* gEnt)
{
	SVT3_UnlinkEntity(SVWM_EntityForGentity(gEnt), SVWM_SvEntityForGentity(gEnt));
}

static void SVWM_LinkEntity(wmsharedEntity_t* gEnt)
{
	SVT3_LinkEntity(SVWM_EntityForGentity(gEnt), SVWM_SvEntityForGentity(gEnt));
}

static bool SVWM_EntityContact(const vec3_t mins, const vec3_t maxs, const wmsharedEntity_t* gEnt, const int capsule)
{
	return SVT3_EntityContact(mins, maxs, SVWM_EntityForGentity(gEnt), capsule);
}

static void SVWM_SetBrushModel(wmsharedEntity_t* ent, const char* name)
{
	SVT3_SetBrushModel(SVWM_EntityForGentity(ent), SVWM_SvEntityForGentity(ent), name);
}

static void SVWM_AdjustAreaPortalState(wmsharedEntity_t* ent, qboolean open)
{
	SVT3_AdjustAreaPortalState(SVWM_SvEntityForGentity(ent), open);
}

static void SVWM_GetUsercmd(int clientNum, wmusercmd_t* cmd)
{
	if (clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		common->Error("SVWM_GetUsercmd: bad clientNum:%i", clientNum);
	}
	*cmd = svs.clients[clientNum].wm_lastUsercmd;
}

//	The module is making a system call
qintptr SVWM_GameSystemCalls(qintptr* args)
{
	switch (args[0])
	{
	case WMG_PRINT:
		common->Printf("%s", (char*)VMA(1));
		return 0;
	case WMG_ERROR:
		common->Error("%s", (char*)VMA(1));
		return 0;
	case WMG_MILLISECONDS:
		return Sys_Milliseconds();
	case WMG_CVAR_REGISTER:
		Cvar_Register((vmCvar_t*)VMA(1), (char*)VMA(2), (char*)VMA(3), args[4]);
		return 0;
	case WMG_CVAR_UPDATE:
		Cvar_Update((vmCvar_t*)VMA(1));
		return 0;
	case WMG_CVAR_SET:
		Cvar_Set((const char*)VMA(1), (const char*)VMA(2));
		return 0;
	case WMG_CVAR_VARIABLE_INTEGER_VALUE:
		return Cvar_VariableIntegerValue((const char*)VMA(1));
	case WMG_CVAR_VARIABLE_STRING_BUFFER:
		Cvar_VariableStringBuffer((char*)VMA(1), (char*)VMA(2), args[3]);
		return 0;
	case WMG_ARGC:
		return Cmd_Argc();
	case WMG_ARGV:
		Cmd_ArgvBuffer(args[1], (char*)VMA(2), args[3]);
		return 0;
	case WMG_SEND_CONSOLE_COMMAND:
		Cbuf_ExecuteText(args[1], (char*)VMA(2));
		return 0;

	case WMG_FS_FOPEN_FILE:
		return FS_FOpenFileByMode((char*)VMA(1), (fileHandle_t*)VMA(2), (fsMode_t)args[3]);
	case WMG_FS_READ:
		FS_Read(VMA(1), args[2], args[3]);
		return 0;
	case WMG_FS_WRITE:
		return FS_Write(VMA(1), args[2], args[3]);
	case WMG_FS_RENAME:
		FS_Rename((char*)VMA(1), (char*)VMA(2));
		return 0;
	case WMG_FS_FCLOSE_FILE:
		FS_FCloseFile(args[1]);
		return 0;
	case WMG_FS_GETFILELIST:
		return FS_GetFileList((char*)VMA(1), (char*)VMA(2), (char*)VMA(3), args[4]);

	case WMG_LOCATE_GAME_DATA:
		SVWM_LocateGameData((wmsharedEntity_t*)VMA(1), args[2], args[3], (wmplayerState_t*)VMA(4), args[5]);
		return 0;
	case WMG_DROP_CLIENT:
		SVT3_GameDropClient(args[1], (char*)VMA(2), 0);
		return 0;
	case WMG_SEND_SERVER_COMMAND:
		SVT3_GameSendServerCommand(args[1], (char*)VMA(2));
		return 0;
	case WMG_LINKENTITY:
		SVWM_LinkEntity((wmsharedEntity_t*)VMA(1));
		return 0;
	case WMG_UNLINKENTITY:
		SVWM_UnlinkEntity((wmsharedEntity_t*)VMA(1));
		return 0;
	case WMG_ENTITIES_IN_BOX:
		return SVT3_AreaEntities((float*)VMA(1), (float*)VMA(2), (int*)VMA(3), args[4]);
	case WMG_ENTITY_CONTACT:
		return SVWM_EntityContact((float*)VMA(1), (float*)VMA(2), (wmsharedEntity_t*)VMA(3), false);
	case WMG_ENTITY_CONTACTCAPSULE:
		return SVWM_EntityContact((float*)VMA(1), (float*)VMA(2), (wmsharedEntity_t*)VMA(3), true);
	case WMG_TRACE:
		SVT3_Trace((q3trace_t*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4), (float*)VMA(5), args[6], args[7], false);
		return 0;
	case WMG_TRACECAPSULE:
		SVT3_Trace((q3trace_t*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4), (float*)VMA(5), args[6], args[7], true);
		return 0;
	case WMG_POINT_CONTENTS:
		return SVT3_PointContents((float*)VMA(1), args[2]);
	case WMG_SET_BRUSH_MODEL:
		SVWM_SetBrushModel((wmsharedEntity_t*)VMA(1), (char*)VMA(2));
		return 0;
	case WMG_IN_PVS:
		return SVT3_inPVS((float*)VMA(1), (float*)VMA(2));
	case WMG_IN_PVS_IGNORE_PORTALS:
		return SVT3_inPVSIgnorePortals((float*)VMA(1), (float*)VMA(2));

	case WMG_SET_CONFIGSTRING:
		SVT3_SetConfigstring(args[1], (char*)VMA(2));
		return 0;
	case WMG_GET_CONFIGSTRING:
		SVT3_GetConfigstring(args[1], (char*)VMA(2), args[3]);
		return 0;
	case WMG_SET_USERINFO:
		SVT3_SetUserinfo(args[1], (char*)VMA(2));
		return 0;
	case WMG_GET_USERINFO:
		SVT3_GetUserinfo(args[1], (char*)VMA(2), args[3]);
		return 0;
	case WMG_GET_SERVERINFO:
		SVT3_GetServerinfo((char*)VMA(1), args[2]);
		return 0;
	case WMG_ADJUST_AREA_PORTAL_STATE:
		SVWM_AdjustAreaPortalState((wmsharedEntity_t*)VMA(1), args[2]);
		return 0;
	case WMG_AREAS_CONNECTED:
		return CM_AreasConnected(args[1], args[2]);

	case WMG_BOT_ALLOCATE_CLIENT:
		return SVT3_BotAllocateClient(-1);
	case WMG_BOT_FREE_CLIENT:
		SVT3_BotFreeClient(args[1]);
		return 0;

	case WMG_GET_USERCMD:
		SVWM_GetUsercmd(args[1], (wmusercmd_t*)VMA(2));
		return 0;
	case WMG_GET_ENTITY_TOKEN:
		return SVT3_GetEntityToken((char*)VMA(1), args[2]);

	case WMG_DEBUG_POLYGON_CREATE:
		return BotImport_DebugPolygonCreate(args[1], args[2], (vec3_t*)VMA(3));
	case WMG_DEBUG_POLYGON_DELETE:
		BotImport_DebugPolygonDelete(args[1]);
		return 0;
	case WMG_REAL_TIME:
		return Com_RealTime((qtime_t*)VMA(1));
	case WMG_SNAPVECTOR:
		Sys_SnapVector((float*)VMA(1));
		return 0;
	case WMG_GETTAG:
		return SVT3_GetTag(args[1], 0, (char*)VMA(2), (orientation_t*)VMA(3));

	case WMBOTLIB_SETUP:
		return SVT3_BotLibSetup();
	case WMBOTLIB_SHUTDOWN:
		return BotLibShutdown();
	case WMBOTLIB_LIBVAR_SET:
		return BotLibVarSet((char*)VMA(1), (char*)VMA(2));
	case WMBOTLIB_LIBVAR_GET:
		return BotLibVarGet((char*)VMA(1), (char*)VMA(2), args[3]);

	case WMBOTLIB_PC_ADD_GLOBAL_DEFINE:
		return PC_AddGlobalDefine((char*)VMA(1));
	case WMBOTLIB_PC_LOAD_SOURCE:
		return PC_LoadSourceHandle((char*)VMA(1));
	case WMBOTLIB_PC_FREE_SOURCE:
		return PC_FreeSourceHandle(args[1]);
	case WMBOTLIB_PC_READ_TOKEN:
		return PC_ReadTokenHandleQ3(args[1], (q3pc_token_t*)VMA(2));
	case WMBOTLIB_PC_SOURCE_FILE_AND_LINE:
		return PC_SourceFileAndLine(args[1], (char*)VMA(2), (int*)VMA(3));

	case WMBOTLIB_START_FRAME:
		return BotLibStartFrame(VMF(1));
	case WMBOTLIB_LOAD_MAP:
		return BotLibLoadMap((char*)VMA(1));
	case WMBOTLIB_UPDATENTITY:
		return BotLibUpdateEntity(args[1], (bot_entitystate_t*)VMA(2));
	case WMBOTLIB_TEST:
		return 0;

	case WMBOTLIB_GET_SNAPSHOT_ENTITY:
		return SVT3_BotGetSnapshotEntity(args[1], args[2]);
	case WMBOTLIB_GET_CONSOLE_MESSAGE:
		return SVT3_BotGetConsoleMessage(args[1], (char*)VMA(2), args[3]);
	case WMBOTLIB_USER_COMMAND:
		SVWM_ClientThink(&svs.clients[args[1]], (wmusercmd_t*)VMA(2));
		return 0;

	case WMBOTLIB_AAS_ENTITY_INFO:
		AAS_EntityInfo(args[1], (aas_entityinfo_t*)VMA(2));
		return 0;

	case WMBOTLIB_AAS_INITIALIZED:
		return AAS_Initialized();
	case WMBOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX:
		AAS_PresenceTypeBoundingBox(args[1], (float*)VMA(2), (float*)VMA(3));
		return 0;
	case WMBOTLIB_AAS_TIME:
		return FloatAsInt(AAS_Time());

	// Ridah
	case WMBOTLIB_AAS_SETCURRENTWORLD:
		AAS_SetCurrentWorld(args[1]);
		return 0;
	// done.

	case WMBOTLIB_AAS_POINT_AREA_NUM:
		return AAS_PointAreaNum((float*)VMA(1));
	case WMBOTLIB_AAS_TRACE_AREAS:
		return AAS_TraceAreas((float*)VMA(1), (float*)VMA(2), (int*)VMA(3), (vec3_t*)VMA(4), args[5]);

	case WMBOTLIB_AAS_POINT_CONTENTS:
		return AAS_PointContents((float*)VMA(1));
	case WMBOTLIB_AAS_NEXT_BSP_ENTITY:
		return AAS_NextBSPEntity(args[1]);
	case WMBOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY:
		return AAS_ValueForBSPEpairKey(args[1], (char*)VMA(2), (char*)VMA(3), args[4]);
	case WMBOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY:
		return AAS_VectorForBSPEpairKey(args[1], (char*)VMA(2), (float*)VMA(3));
	case WMBOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY:
		return AAS_FloatForBSPEpairKey(args[1], (char*)VMA(2), (float*)VMA(3));
	case WMBOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY:
		return AAS_IntForBSPEpairKey(args[1], (char*)VMA(2), (int*)VMA(3));

	case WMBOTLIB_AAS_AREA_REACHABILITY:
		return AAS_AreaReachability(args[1]);

	case WMBOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA:
		return AAS_AreaTravelTimeToGoalArea(args[1], (float*)VMA(2), args[3], args[4]);

	case WMBOTLIB_AAS_SWIMMING:
		return AAS_Swimming((float*)VMA(1));
	case WMBOTLIB_AAS_PREDICT_CLIENT_MOVEMENT:
		return AAS_PredictClientMovementWolf((struct aas_clientmove_rtcw_t*)VMA(1), args[2], (float*)VMA(3), args[4], args[5],
			(float*)VMA(6), (float*)VMA(7), args[8], args[9], VMF(10), args[11], args[12], args[13]);

	// Ridah, route-tables
	case WMBOTLIB_AAS_RT_SHOWROUTE:
		return 0;

	case WMBOTLIB_AAS_RT_GETHIDEPOS:
		return AAS_RT_GetHidePos((float*)VMA(1), args[2], args[3], (float*)VMA(4), args[5], args[6], (float*)VMA(7));

	case WMBOTLIB_AAS_FINDATTACKSPOTWITHINRANGE:
		return AAS_FindAttackSpotWithinRange(args[1], args[2], args[3], VMF(4), args[5], (float*)VMA(6));

	case WMBOTLIB_AAS_SETAASBLOCKINGENTITY:
		AAS_SetAASBlockingEntity((float*)VMA(1), (float*)VMA(2), args[3]);
		return 0;
	// done.

	case WMBOTLIB_EA_SAY:
		EA_Say(args[1], (char*)VMA(2));
		return 0;
	case WMBOTLIB_EA_SAY_TEAM:
		EA_SayTeam(args[1], (char*)VMA(2));
		return 0;
	case WMBOTLIB_EA_USE_ITEM:
		EA_UseItem(args[1], (char*)VMA(2));
		return 0;
	case WMBOTLIB_EA_DROP_ITEM:
		EA_DropItem(args[1], (char*)VMA(2));
		return 0;
	case WMBOTLIB_EA_USE_INV:
		EA_UseInv(args[1], (char*)VMA(2));
		return 0;
	case WMBOTLIB_EA_DROP_INV:
		EA_DropInv(args[1], (char*)VMA(2));
		return 0;
	case WMBOTLIB_EA_GESTURE:
		EA_Gesture(args[1]);
		return 0;
	case WMBOTLIB_EA_COMMAND:
		EA_Command(args[1], (char*)VMA(2));
		return 0;

	case WMBOTLIB_EA_SELECT_WEAPON:
		EA_SelectWeapon(args[1], args[2]);
		return 0;
	case WMBOTLIB_EA_TALK:
		EA_Talk(args[1]);
		return 0;
	case WMBOTLIB_EA_ATTACK:
		EA_Attack(args[1]);
		return 0;
	case WMBOTLIB_EA_RELOAD:
		EA_Reload(args[1]);
		return 0;
	case WMBOTLIB_EA_USE:
		EA_Use(args[1]);
		return 0;
	case WMBOTLIB_EA_RESPAWN:
		EA_Respawn(args[1]);
		return 0;
	case WMBOTLIB_EA_JUMP:
		EA_Jump(args[1]);
		return 0;
	case WMBOTLIB_EA_DELAYED_JUMP:
		EA_DelayedJump(args[1]);
		return 0;
	case WMBOTLIB_EA_CROUCH:
		EA_Crouch(args[1]);
		return 0;
	case WMBOTLIB_EA_MOVE_UP:
		EA_MoveUp(args[1]);
		return 0;
	case WMBOTLIB_EA_MOVE_DOWN:
		EA_MoveDown(args[1]);
		return 0;
	case WMBOTLIB_EA_MOVE_FORWARD:
		EA_MoveForward(args[1]);
		return 0;
	case WMBOTLIB_EA_MOVE_BACK:
		EA_MoveBack(args[1]);
		return 0;
	case WMBOTLIB_EA_MOVE_LEFT:
		EA_MoveLeft(args[1]);
		return 0;
	case WMBOTLIB_EA_MOVE_RIGHT:
		EA_MoveRight(args[1]);
		return 0;
	case WMBOTLIB_EA_MOVE:
		EA_Move(args[1], (float*)VMA(2), VMF(3));
		return 0;
	case WMBOTLIB_EA_VIEW:
		EA_View(args[1], (float*)VMA(2));
		return 0;

	case WMBOTLIB_EA_END_REGULAR:
		return 0;
	case WMBOTLIB_EA_GET_INPUT:
		EA_GetInput(args[1], VMF(2), (bot_input_t*)VMA(3));
		return 0;
	case WMBOTLIB_EA_RESET_INPUT:
		EA_ResetInputWolf(args[1], (bot_input_t*)VMA(2));
		return 0;

	case WMBOTLIB_AI_LOAD_CHARACTER:
		return BotLoadCharacter((char*)VMA(1), args[2]);
	case WMBOTLIB_AI_FREE_CHARACTER:
		BotFreeCharacter(args[1]);
		return 0;
	case WMBOTLIB_AI_CHARACTERISTIC_FLOAT:
		return FloatAsInt(Characteristic_Float(args[1], args[2]));
	case WMBOTLIB_AI_CHARACTERISTIC_BFLOAT:
		return FloatAsInt(Characteristic_BFloat(args[1], args[2], VMF(3), VMF(4)));
	case WMBOTLIB_AI_CHARACTERISTIC_INTEGER:
		return Characteristic_Integer(args[1], args[2]);
	case WMBOTLIB_AI_CHARACTERISTIC_BINTEGER:
		return Characteristic_BInteger(args[1], args[2], args[3], args[4]);
	case WMBOTLIB_AI_CHARACTERISTIC_STRING:
		Characteristic_String(args[1], args[2], (char*)VMA(3), args[4]);
		return 0;

	case WMBOTLIB_AI_ALLOC_CHAT_STATE:
		return BotAllocChatState();
	case WMBOTLIB_AI_FREE_CHAT_STATE:
		BotFreeChatState(args[1]);
		return 0;
	case WMBOTLIB_AI_QUEUE_CONSOLE_MESSAGE:
		BotQueueConsoleMessage(args[1], args[2], (char*)VMA(3));
		return 0;
	case WMBOTLIB_AI_REMOVE_CONSOLE_MESSAGE:
		BotRemoveConsoleMessage(args[1], args[2]);
		return 0;
	case WMBOTLIB_AI_NEXT_CONSOLE_MESSAGE:
		return BotNextConsoleMessageWolf(args[1], (struct bot_consolemessage_wolf_t*)VMA(2));
	case WMBOTLIB_AI_NUM_CONSOLE_MESSAGE:
		return BotNumConsoleMessages(args[1]);
	case WMBOTLIB_AI_INITIAL_CHAT:
		BotInitialChat(args[1], (char*)VMA(2), args[3], (char*)VMA(4), (char*)VMA(5), (char*)VMA(6), (char*)VMA(7), (char*)VMA(8), (char*)VMA(9), (char*)VMA(10), (char*)VMA(11));
		return 0;
	case WMBOTLIB_AI_NUM_INITIAL_CHATS:
		return BotNumInitialChats(args[1], (char*)VMA(2));
	case WMBOTLIB_AI_REPLY_CHAT:
		return BotReplyChat(args[1], (char*)VMA(2), args[3], args[4], (char*)VMA(5), (char*)VMA(6), (char*)VMA(7), (char*)VMA(8), (char*)VMA(9), (char*)VMA(10), (char*)VMA(11), (char*)VMA(12));
	case WMBOTLIB_AI_CHAT_LENGTH:
		return BotChatLength(args[1]);
	case WMBOTLIB_AI_ENTER_CHAT:
		BotEnterChatWolf(args[1], args[2], args[3]);
		return 0;
	case WMBOTLIB_AI_GET_CHAT_MESSAGE:
		BotGetChatMessage(args[1], (char*)VMA(2), args[3]);
		return 0;
	case WMBOTLIB_AI_STRING_CONTAINS:
		return StringContains((char*)VMA(1), (char*)VMA(2), args[3]);
	case WMBOTLIB_AI_FIND_MATCH:
		return BotFindMatchWolf((char*)VMA(1), (bot_match_wolf_t*)VMA(2), args[3]);
	case WMBOTLIB_AI_MATCH_VARIABLE:
		BotMatchVariableWolf((bot_match_wolf_t*)VMA(1), args[2], (char*)VMA(3), args[4]);
		return 0;
	case WMBOTLIB_AI_UNIFY_WHITE_SPACES:
		UnifyWhiteSpaces((char*)VMA(1));
		return 0;
	case WMBOTLIB_AI_REPLACE_SYNONYMS:
		BotReplaceSynonyms((char*)VMA(1), args[2]);
		return 0;
	case WMBOTLIB_AI_LOAD_CHAT_FILE:
		return BotLoadChatFile(args[1], (char*)VMA(2), (char*)VMA(3));
	case WMBOTLIB_AI_SET_CHAT_GENDER:
		BotSetChatGender(args[1], args[2]);
		return 0;
	case WMBOTLIB_AI_SET_CHAT_NAME:
		BotSetChatName(args[1], (char*)VMA(2), 0);
		return 0;

	case WMBOTLIB_AI_RESET_GOAL_STATE:
		BotResetGoalState(args[1]);
		return 0;
	case WMBOTLIB_AI_RESET_AVOID_GOALS:
		BotResetAvoidGoals(args[1]);
		return 0;
	case WMBOTLIB_AI_REMOVE_FROM_AVOID_GOALS:
		BotRemoveFromAvoidGoals(args[1], args[2]);
		return 0;
	case WMBOTLIB_AI_PUSH_GOAL:
		BotPushGoalQ3(args[1], (struct bot_goal_q3_t*)VMA(2));
		return 0;
	case WMBOTLIB_AI_POP_GOAL:
		BotPopGoal(args[1]);
		return 0;
	case WMBOTLIB_AI_EMPTY_GOAL_STACK:
		BotEmptyGoalStack(args[1]);
		return 0;
	case WMBOTLIB_AI_DUMP_AVOID_GOALS:
		BotDumpAvoidGoals(args[1]);
		return 0;
	case WMBOTLIB_AI_DUMP_GOAL_STACK:
		BotDumpGoalStack(args[1]);
		return 0;
	case WMBOTLIB_AI_GOAL_NAME:
		BotGoalName(args[1], (char*)VMA(2), args[3]);
		return 0;
	case WMBOTLIB_AI_GET_TOP_GOAL:
		return BotGetTopGoalQ3(args[1], (struct bot_goal_q3_t*)VMA(2));
	case WMBOTLIB_AI_GET_SECOND_GOAL:
		return BotGetSecondGoalQ3(args[1], (struct bot_goal_q3_t*)VMA(2));
	case WMBOTLIB_AI_CHOOSE_LTG_ITEM:
		return BotChooseLTGItem(args[1], (float*)VMA(2), (int*)VMA(3), args[4]);
	case WMBOTLIB_AI_CHOOSE_NBG_ITEM:
		return BotChooseNBGItemQ3(args[1], (float*)VMA(2), (int*)VMA(3), args[4], (struct bot_goal_q3_t*)VMA(5), VMF(6));
	case WMBOTLIB_AI_TOUCHING_GOAL:
		return BotTouchingGoalQ3((float*)VMA(1), (struct bot_goal_q3_t*)VMA(2));
	case WMBOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE:
		return BotItemGoalInVisButNotVisibleQ3(args[1], (float*)VMA(2), (float*)VMA(3), (struct bot_goal_q3_t*)VMA(4));
	case WMBOTLIB_AI_GET_LEVEL_ITEM_GOAL:
		return BotGetLevelItemGoalQ3(args[1], (char*)VMA(2), (struct bot_goal_q3_t*)VMA(3));
	case WMBOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL:
		return BotGetNextCampSpotGoalQ3(args[1], (struct bot_goal_q3_t*)VMA(2));
	case WMBOTLIB_AI_GET_MAP_LOCATION_GOAL:
		return BotGetMapLocationGoalQ3((char*)VMA(1), (struct bot_goal_q3_t*)VMA(2));
	case WMBOTLIB_AI_AVOID_GOAL_TIME:
		return FloatAsInt(BotAvoidGoalTime(args[1], args[2]));
	case WMBOTLIB_AI_INIT_LEVEL_ITEMS:
		BotInitLevelItems();
		return 0;
	case WMBOTLIB_AI_UPDATE_ENTITY_ITEMS:
		BotUpdateEntityItems();
		return 0;
	case WMBOTLIB_AI_LOAD_ITEM_WEIGHTS:
		return BotLoadItemWeights(args[1], (char*)VMA(2));
	case WMBOTLIB_AI_FREE_ITEM_WEIGHTS:
		BotFreeItemWeights(args[1]);
		return 0;
	case WMBOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC:
		BotInterbreedGoalFuzzyLogic(args[1], args[2], args[3]);
		return 0;
	case WMBOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC:
		return 0;
	case WMBOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC:
		BotMutateGoalFuzzyLogic(args[1], VMF(2));
		return 0;
	case WMBOTLIB_AI_ALLOC_GOAL_STATE:
		return BotAllocGoalState(args[1]);
	case WMBOTLIB_AI_FREE_GOAL_STATE:
		BotFreeGoalState(args[1]);
		return 0;

	case WMBOTLIB_AI_RESET_MOVE_STATE:
		BotResetMoveState(args[1]);
		return 0;
	case WMBOTLIB_AI_MOVE_TO_GOAL:
		BotMoveToGoalQ3((bot_moveresult_t*)VMA(1), args[2], (bot_goal_q3_t*)VMA(3), args[4]);
		return 0;
	case WMBOTLIB_AI_MOVE_IN_DIRECTION:
		return BotMoveInDirection(args[1], (float*)VMA(2), VMF(3), args[4]);
	case WMBOTLIB_AI_RESET_AVOID_REACH:
		BotResetAvoidReachAndMove(args[1]);
		return 0;
	case WMBOTLIB_AI_RESET_LAST_AVOID_REACH:
		BotResetLastAvoidReach(args[1]);
		return 0;
	case WMBOTLIB_AI_REACHABILITY_AREA:
		return BotReachabilityArea((float*)VMA(1), args[2]);
	case WMBOTLIB_AI_MOVEMENT_VIEW_TARGET:
		return BotMovementViewTargetQ3(args[1], (struct bot_goal_q3_t*)VMA(2), args[3], VMF(4), (float*)VMA(5));
	case WMBOTLIB_AI_PREDICT_VISIBLE_POSITION:
		return BotPredictVisiblePositionQ3((float*)VMA(1), args[2], (struct bot_goal_q3_t*)VMA(3), args[4], (float*)VMA(5));
	case WMBOTLIB_AI_ALLOC_MOVE_STATE:
		return BotAllocMoveState();
	case WMBOTLIB_AI_FREE_MOVE_STATE:
		BotFreeMoveState(args[1]);
		return 0;
	case WMBOTLIB_AI_INIT_MOVE_STATE:
		BotInitMoveStateQ3(args[1], (struct bot_initmove_q3_t*)VMA(2));
		return 0;
	// Ridah
	case WMBOTLIB_AI_INIT_AVOID_REACH:
		BotResetAvoidReach(args[1]);
		return 0;
	// done.

	case WMBOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON:
		return BotChooseBestFightWeapon(args[1], (int*)VMA(2));
	case WMBOTLIB_AI_GET_WEAPON_INFO:
		BotGetWeaponInfo(args[1], args[2], (weaponinfo_t*)VMA(3));
		return 0;
	case WMBOTLIB_AI_LOAD_WEAPON_WEIGHTS:
		return BotLoadWeaponWeights(args[1], (char*)VMA(2));
	case WMBOTLIB_AI_ALLOC_WEAPON_STATE:
		return BotAllocWeaponState();
	case WMBOTLIB_AI_FREE_WEAPON_STATE:
		BotFreeWeaponState(args[1]);
		return 0;
	case WMBOTLIB_AI_RESET_WEAPON_STATE:
		BotResetWeaponState(args[1]);
		return 0;

	case WMBOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION:
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
