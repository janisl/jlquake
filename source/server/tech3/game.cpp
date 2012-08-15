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
#include "local.h"
#include "../quake3/local.h"
#include "../wolfsp/local.h"
#include "../wolfmp/local.h"
#include "../et/local.h"

vm_t* gvm = NULL;							// game virtual machine

idEntity3* SVT3_EntityNum(int number)
{
	qassert(number >= 0);
	qassert(number < MAX_GENTITIES_Q3);
	qassert(sv.q3_entities[number]);
	return sv.q3_entities[number];
}

idEntity3* SVT3_EntityForSvEntity(const q3svEntity_t* svEnt)
{
	int num = svEnt - sv.q3_svEntities;
	return SVT3_EntityNum(num);
}

idPlayerState3* SVT3_GameClientNum(int num)
{
	qassert(num >= 0);
	qassert(num < sv_maxclients->integer);
	return sv.q3_gamePlayerStates[num];
}

//	Also checks portalareas so that doors block sight
bool SVT3_inPVS(const vec3_t p1, const vec3_t p2)
{
	int leafnum = CM_PointLeafnum(p1);
	int cluster = CM_LeafCluster(leafnum);
	int area1 = CM_LeafArea(leafnum);
	byte* mask = CM_ClusterPVS(cluster);

	leafnum = CM_PointLeafnum(p2);
	cluster = CM_LeafCluster(leafnum);
	int area2 = CM_LeafArea(leafnum);

	if (mask && (!(mask[cluster >> 3] & (1 << (cluster & 7)))))
	{
		return false;
	}
	if (!CM_AreasConnected(area1, area2))
	{
		return false;		// a door blocks sight
	}
	return true;
}

bool SVT3_inPVSIgnorePortals(const vec3_t p1, const vec3_t p2)
{
	int leafnum = CM_PointLeafnum(p1);
	int cluster = CM_LeafCluster(leafnum);
	byte* mask = CM_ClusterPVS(cluster);

	leafnum = CM_PointLeafnum(p2);
	cluster = CM_LeafCluster(leafnum);

	if (mask && (!(mask[cluster >> 3] & (1 << (cluster & 7)))))
	{
		return false;
	}
	return true;
}

bool SVT3_BotVisibleFromPos(vec3_t srcpos, int srcnum, vec3_t destpos, int destnum, bool updateVisPos)
{
	if (GGameType & GAME_WolfSP)
	{
		return SVWS_BotVisibleFromPos(srcpos, srcnum, destpos, destnum, updateVisPos);
	}
	if (GGameType & GAME_WolfMP)
	{
		return SVWM_BotVisibleFromPos(srcpos, srcnum, destpos, destnum, updateVisPos);
	}
	if (GGameType & GAME_ET)
	{
		return SVET_BotVisibleFromPos(srcpos, srcnum, destpos, destnum, updateVisPos);
	}
	return false;
}

bool SVT3_BotCheckAttackAtPos(int entnum, int enemy, vec3_t pos, bool ducking, bool allowHitWorld)
{
	if (GGameType & GAME_WolfSP)
	{
		return SVWS_BotCheckAttackAtPos(entnum, enemy, pos, ducking, allowHitWorld);
	}
	if (GGameType & GAME_WolfMP)
	{
		return SVWM_BotCheckAttackAtPos(entnum, enemy, pos, ducking, allowHitWorld);
	}
	if (GGameType & GAME_ET)
	{
		return SVET_BotCheckAttackAtPos(entnum, enemy, pos, ducking, allowHitWorld);
	}
	return false;
}

bool SVT3_EntityContact(const vec3_t mins, const vec3_t maxs, const idEntity3* ent, int capsule)
{
	// check for exact collision
	const float* origin = ent->GetCurrentOrigin();
	const float* angles = ent->GetCurrentAngles();

	clipHandle_t ch = SVT3_ClipHandleForEntity(ent);
	q3trace_t trace;
	CM_TransformedBoxTraceQ3(&trace, vec3_origin, vec3_origin, mins, maxs,
		ch, -1, origin, angles, capsule);

	return trace.startsolid;
}

//	sets mins and maxs for inline bmodels
void SVT3_SetBrushModel(idEntity3* ent, q3svEntity_t* svEnt, const char* name)
{
	if (!name)
	{
		common->Error("SV_SetBrushModel: NULL");
	}

	if (name[0] != '*')
	{
		common->Error("SV_SetBrushModel: %s isn't a brush model", name);
	}


	ent->SetModelIndex(String::Atoi(name + 1));

	clipHandle_t h = CM_InlineModel(ent->GetModelIndex());
	vec3_t mins, maxs;
	CM_ModelBounds(h, mins, maxs);
	ent->SetMins(mins);
	ent->SetMaxs(maxs);
	ent->SetBModel(true);

	// we don't know exactly what is in the brushes
	ent->SetContents(-1);

	SVT3_LinkEntity(ent, svEnt);
}

void SVT3_GetServerinfo(char* buffer, int bufferSize)
{
	if (bufferSize < 1)
	{
		common->Error("SVT3_GetServerinfo: bufferSize == %i", bufferSize);
	}
	String::NCpyZ(buffer, Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE, MAX_INFO_STRING_Q3), bufferSize);
}

void SVT3_AdjustAreaPortalState(q3svEntity_t* svEnt, bool open)
{
	if (svEnt->areanum2 == -1)
	{
		return;
	}
	CM_AdjustAreaPortalState(svEnt->areanum, svEnt->areanum2, open);
}

bool SVT3_GetEntityToken(char* buffer, int length)
{
	const char* s = String::Parse3(&sv.q3_entityParsePoint);
	String::NCpyZ(buffer, s, length);
	if (!sv.q3_entityParsePoint && !s[0])
	{
		return false;
	}
	else
	{
		return true;
	}
}

//	Sends a command string to a client
void SVT3_GameSendServerCommand(int clientNum, const char* text)
{
	if (clientNum == -1)
	{
		SVT3_SendServerCommand(NULL, "%s", text);
	}
	else
	{
		if (clientNum < 0 || clientNum >= sv_maxclients->integer)
		{
			return;
		}
		SVT3_SendServerCommand(svs.clients + clientNum, "%s", text);
	}
}

//	Disconnects the client with a message
void SVT3_GameDropClient(int clientNum, const char* reason, int length)
{
	if (clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		return;
	}
	SVT3_DropClient(svs.clients + clientNum, reason);
	if (length)
	{
		SVET_TempBanNetAddress(svs.clients[clientNum].netchan.remoteAddress, length);
	}
}

//	return false if unable to retrieve tag information for this client
bool SVT3_GetTag(int clientNum, int tagFileNumber, const char* tagname, orientation_t* _or)
{
	if (tagFileNumber > 0 && tagFileNumber <= sv.et_num_tagheaders)
	{
		for (int i = sv.et_tagHeadersExt[tagFileNumber - 1].start;
			i < sv.et_tagHeadersExt[tagFileNumber - 1].start + sv.et_tagHeadersExt[tagFileNumber - 1].count; i++)
		{
			if (!String::ICmp(sv.et_tags[i].name, tagname))
			{
				VectorCopy(sv.et_tags[i].origin, _or->origin);
				VectorCopy(sv.et_tags[i].axis[0], _or->axis[0]);
				VectorCopy(sv.et_tags[i].axis[1], _or->axis[1]);
				VectorCopy(sv.et_tags[i].axis[2], _or->axis[2]);
				return true;
			}
		}
	}

	// Gordon: lets try and remove the inconsitancy between ded/non-ded servers...
	// Gordon: bleh, some code in clientthink_real really relies on this working on player models...
	if (com_dedicated->integer)
	{
		return false;
	}

	return CL_GetTag(clientNum, tagname, _or);
}

//	Called for both a full init and a restart
static void SVT3_InitGameVM(bool restart)
{
	// start the entity parsing at the beginning
	sv.q3_entityParsePoint = CM_EntityString();

	// clear all gentity pointers that might still be set from
	// a previous level
	for (int i = 0; i < sv_maxclients->integer; i++)
	{
		svs.clients[i].q3_gentity = NULL;
		svs.clients[i].ws_gentity = NULL;
		svs.clients[i].wm_gentity = NULL;
		svs.clients[i].et_gentity = NULL;
		svs.clients[i].q3_entity = NULL;
	}

	// use the current msec count for a random seed
	// init for this gamestate
	if (GGameType & GAME_WolfSP)
	{
		SVWS_GameInit(svs.q3_time, Com_Milliseconds(), restart);
	}
	else if (GGameType & GAME_WolfMP)
	{
		SVWM_GameInit(svs.q3_time, Com_Milliseconds(), restart);
	}
	else if (GGameType & GAME_ET)
	{
		SVET_GameInit(svs.q3_time, Com_Milliseconds(), restart);
	}
	else
	{
		SVQ3_GameInit(svs.q3_time, Com_Milliseconds(), restart);
	}
}

//	Called on a normal map change, not on a map_restart
void SVT3_InitGameProgs()
{
	if (GGameType & (GAME_Quake3 | GAME_WolfSP))
	{
		Cvar* var = Cvar_Get("bot_enable", "1", CVAR_LATCH2);
		if (var)
		{
			bot_enable = var->integer;
		}
		else
		{
			bot_enable = 0;
		}
	}

	sv.et_num_tagheaders = 0;
	sv.et_num_tags = 0;

	// load the dll or bytecode
	if (GGameType & GAME_WolfSP)
	{
		gvm = VM_Create("qagame", SVWS_GameSystemCalls, VMI_NATIVE);
	}
	else if (GGameType & GAME_WolfMP)
	{
		gvm = VM_Create("qagame", SVWM_GameSystemCalls, VMI_NATIVE);
	}
	else if (GGameType & GAME_ET)
	{
		gvm = VM_Create("qagame", SVET_GameSystemCalls, VMI_NATIVE);
	}
	else
	{
		gvm = VM_Create("qagame", SVQ3_GameSystemCalls, (vmInterpret_t)(int)Cvar_VariableValue("vm_game"));
	}
	if (!gvm)
	{
		common->FatalError("VM_Create on game failed");
	}

	SVT3_InitGameVM(false);
}

//	Called on a map_restart, but not on a normal map change
void SVT3_RestartGameProgs()
{
	if (!gvm)
	{
		return;
	}
	if (GGameType & GAME_WolfSP)
	{
		SVWS_GameShutdown(true);
	}
	else if (GGameType & GAME_WolfMP)
	{
		SVWM_GameShutdown(true);
	}
	else if (GGameType & GAME_ET)
	{
		SVET_GameShutdown(true);
	}
	else
	{
		SVQ3_GameShutdown(true);
	}

	// do a restart instead of a free
	gvm = VM_Restart(gvm);
	if (!gvm)		// bk001212 - as done below
	{
		common->FatalError("VM_Restart on game failed");
	}

	SVT3_InitGameVM(true);
}

//	Called every time a map changes
void SVT3_ShutdownGameProgs()
{
	if (!gvm)
	{
		return;
	}
	if (GGameType & GAME_WolfSP)
	{
		SVWS_GameShutdown(false);
	}
	else if (GGameType & GAME_WolfMP)
	{
		SVWM_GameShutdown(false);
	}
	else if (GGameType & GAME_ET)
	{
		SVET_GameShutdown(false);
	}
	else
	{
		SVQ3_GameShutdown(false);
	}
	VM_Free(gvm);
	gvm = NULL;
}

//	See if the current console command is claimed by the game
bool SVT3_GameCommand()
{
	if (sv.state != SS_GAME)
	{
		return false;
	}

	if (GGameType & GAME_WolfSP)
	{
		return SVWS_GameConsoleCommand();
	}
	if (GGameType & GAME_WolfMP)
	{
		return SVWM_GameConsoleCommand();
	}
	if (GGameType & GAME_ET)
	{
		return SVET_GameConsoleCommand();
	}
	return SVQ3_GameConsoleCommand();
}

const char* SVT3_GameClientConnect(int clientNum, bool firstTime, bool isBot)
{
	if (GGameType & GAME_WolfSP)
	{
		return SVWS_GameClientConnect(clientNum, firstTime, isBot);
	}
	if (GGameType & GAME_WolfMP)
	{
		return SVWM_GameClientConnect(clientNum, firstTime, isBot);
	}
	if (GGameType & GAME_ET)
	{
		return SVET_GameClientConnect(clientNum, firstTime, isBot);
	}
	return SVQ3_GameClientConnect(clientNum, firstTime, isBot);
}

void SVT3_GameClientCommand(int clientNum)
{
	if (GGameType & GAME_WolfSP)
	{
		SVWS_GameClientCommand(clientNum);
	}
	else if (GGameType & GAME_WolfMP)
	{
		SVWM_GameClientCommand(clientNum);
	}
	else if (GGameType & GAME_ET)
	{
		SVET_GameClientCommand(clientNum);
	}
	else
	{
		SVQ3_GameClientCommand(clientNum);
	}
}

void SVT3_GameRunFrame(int time)
{
	if (GGameType & GAME_WolfSP)
	{
		SVWS_GameRunFrame(time);
	}
	else if (GGameType & GAME_WolfMP)
	{
		SVWM_GameRunFrame(time);
	}
	else if (GGameType & GAME_ET)
	{
		SVET_GameRunFrame(time);
	}
	else
	{
		SVQ3_GameRunFrame(time);
	}
}

void SVT3_GameClientBegin(int clientNum)
{
	if (GGameType & GAME_WolfSP)
	{
		SVWS_GameClientBegin(clientNum);
	}
	else if (GGameType & GAME_WolfMP)
	{
		SVWM_GameClientBegin(clientNum);
	}
	else if (GGameType & GAME_ET)
	{
		SVET_GameClientBegin(clientNum);
	}
	else
	{
		SVQ3_GameClientBegin(clientNum);
	}
}
