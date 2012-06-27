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
// sv_game.c -- interface to the game dll

#include "server.h"

void SV_GameError(const char* string)
{
	Com_Error(ERR_DROP, "%s", string);
}

void SV_GamePrint(const char* string)
{
	Com_Printf("%s", string);
}

/*
===============
SV_GameSendServerCommand

Sends a command string to a client
===============
*/
void SV_GameSendServerCommand(int clientNum, const char* text)
{
	if (clientNum == -1)
	{
		SV_SendServerCommand(NULL, "%s", text);
	}
	else
	{
		if (clientNum < 0 || clientNum >= sv_maxclients->integer)
		{
			return;
		}
		SV_SendServerCommand(svs.clients + clientNum, "%s", text);
	}
}


/*
===============
SV_GameDropClient

Disconnects the client with a message
===============
*/
void SV_GameDropClient(int clientNum, const char* reason)
{
	if (clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		return;
	}
	SV_DropClient(svs.clients + clientNum, reason);
}


/*
=================
SV_SetBrushModel

sets mins and maxs for inline bmodels
=================
*/
void SV_SetBrushModel(q3sharedEntity_t* ent, const char* name)
{
	clipHandle_t h;
	vec3_t mins, maxs;

	if (!name)
	{
		Com_Error(ERR_DROP, "SV_SetBrushModel: NULL");
	}

	if (name[0] != '*')
	{
		Com_Error(ERR_DROP, "SV_SetBrushModel: %s isn't a brush model", name);
	}


	ent->s.modelindex = String::Atoi(name + 1);

	h = CM_InlineModel(ent->s.modelindex);
	CM_ModelBounds(h, mins, maxs);
	VectorCopy(mins, ent->r.mins);
	VectorCopy(maxs, ent->r.maxs);
	ent->r.bmodel = qtrue;

	ent->r.contents = -1;		// we don't know exactly what is in the brushes

	SVQ3_LinkEntity(ent);			// FIXME: remove
}



/*
========================
SV_AdjustAreaPortalState
========================
*/
void SV_AdjustAreaPortalState(q3sharedEntity_t* ent, qboolean open)
{
	q3svEntity_t* svEnt;

	svEnt = SVQ3_SvEntityForGentity(ent);
	if (svEnt->areanum2 == -1)
	{
		return;
	}
	CM_AdjustAreaPortalState(svEnt->areanum, svEnt->areanum2, open);
}


/*
==================
SV_GameAreaEntities
==================
*/
qboolean    SV_EntityContact(vec3_t mins, vec3_t maxs, const q3sharedEntity_t* gEnt, int capsule)
{
	const float* origin, * angles;
	clipHandle_t ch;
	q3trace_t trace;

	// check for exact collision
	origin = gEnt->r.currentOrigin;
	angles = gEnt->r.currentAngles;

	ch = SVQ3_ClipHandleForEntity(gEnt);
	CM_TransformedBoxTraceQ3(&trace, vec3_origin, vec3_origin, mins, maxs,
		ch, -1, origin, angles, capsule);

	return trace.startsolid;
}


/*
===============
SV_GetServerinfo

===============
*/
void SV_GetServerinfo(char* buffer, int bufferSize)
{
	if (bufferSize < 1)
	{
		Com_Error(ERR_DROP, "SV_GetServerinfo: bufferSize == %i", bufferSize);
	}
	String::NCpyZ(buffer, Cvar_InfoString(CVAR_SERVERINFO, MAX_INFO_STRING_Q3), bufferSize);
}

/*
===============
SV_LocateGameData

===============
*/
void SV_LocateGameData(q3sharedEntity_t* gEnts, int numGEntities, int sizeofGEntity_t,
	q3playerState_t* clients, int sizeofGameClient)
{
	sv.q3_gentities = gEnts;
	sv.q3_gentitySize = sizeofGEntity_t;
	sv.q3_num_entities = numGEntities;

	for (int i = 0; i < MAX_GENTITIES_Q3; i++)
	{
		SVT3_EntityNum(i)->SetGEntity(SVQ3_GentityNum(i));
	}

	sv.q3_gameClients = clients;
	sv.q3_gameClientSize = sizeofGameClient;
}


/*
===============
SV_GetUsercmd

===============
*/
void SV_GetUsercmd(int clientNum, q3usercmd_t* cmd)
{
	if (clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "SV_GetUsercmd: bad clientNum:%i", clientNum);
	}
	*cmd = svs.clients[clientNum].q3_lastUsercmd;
}

//==============================================

/*
====================
SV_GameSystemCalls

The module is making a system call
====================
*/
qintptr SV_GameSystemCalls(qintptr* args)
{
	switch (args[0])
	{
//----
	case Q3G_LOCATE_GAME_DATA:
		SV_LocateGameData((q3sharedEntity_t*)VMA(1), args[2], args[3], (q3playerState_t*)VMA(4), args[5]);
		return 0;
	case Q3G_DROP_CLIENT:
		SV_GameDropClient(args[1], (char*)VMA(2));
		return 0;
	case Q3G_SEND_SERVER_COMMAND:
		SV_GameSendServerCommand(args[1], (char*)VMA(2));
		return 0;
//----
	case Q3G_ENTITY_CONTACT:
		return SV_EntityContact((float*)VMA(1), (float*)VMA(2), (q3sharedEntity_t*)VMA(3), /*int capsule*/ qfalse);
	case Q3G_ENTITY_CONTACTCAPSULE:
		return SV_EntityContact((float*)VMA(1), (float*)VMA(2), (q3sharedEntity_t*)VMA(3), /*int capsule*/ qtrue);
//----
	case Q3G_SET_BRUSH_MODEL:
		SV_SetBrushModel((q3sharedEntity_t*)VMA(1), (char*)VMA(2));
		return 0;
//----
	case Q3G_SET_CONFIGSTRING:
		SV_SetConfigstring(args[1], (char*)VMA(2));
		return 0;
	case Q3G_GET_CONFIGSTRING:
		SV_GetConfigstring(args[1], (char*)VMA(2), args[3]);
		return 0;
	case Q3G_SET_USERINFO:
		SV_SetUserinfo(args[1], (char*)VMA(2));
		return 0;
	case Q3G_GET_USERINFO:
		SV_GetUserinfo(args[1], (char*)VMA(2), args[3]);
		return 0;
	case Q3G_GET_SERVERINFO:
		SV_GetServerinfo((char*)VMA(1), args[2]);
		return 0;
	case Q3G_ADJUST_AREA_PORTAL_STATE:
		SV_AdjustAreaPortalState((q3sharedEntity_t*)VMA(1), args[2]);
		return 0;
//----
	case Q3G_BOT_ALLOCATE_CLIENT:
		return SV_BotAllocateClient();
	case Q3G_BOT_FREE_CLIENT:
		SV_BotFreeClient(args[1]);
		return 0;

	case Q3G_GET_USERCMD:
		SV_GetUsercmd(args[1], (q3usercmd_t*)VMA(2));
		return 0;
	case Q3G_GET_ENTITY_TOKEN:
	{
		const char* s;

		s = String::Parse3(&sv.q3_entityParsePoint);
		String::NCpyZ((char*)VMA(1), s, args[2]);
		if (!sv.q3_entityParsePoint && !s[0])
		{
			return qfalse;
		}
		else
		{
			return qtrue;
		}
	}
//----
	case Q3G_REAL_TIME:
		return Com_RealTime((qtime_t*)VMA(1));
	case Q3G_SNAPVECTOR:
		Sys_SnapVector((float*)VMA(1));
		return 0;

	//====================================

	case Q3BOTLIB_SETUP:
		return SV_BotLibSetup();
	case Q3BOTLIB_SHUTDOWN:
		return SV_BotLibShutdown();
//----
	case Q3BOTLIB_GET_SNAPSHOT_ENTITY:
		return SV_BotGetSnapshotEntity(args[1], args[2]);
	case Q3BOTLIB_GET_CONSOLE_MESSAGE:
		return SV_BotGetConsoleMessage(args[1], (char*)VMA(2), args[3]);
	case Q3BOTLIB_USER_COMMAND:
		SV_ClientThink(&svs.clients[args[1]], (q3usercmd_t*)VMA(2));
		return 0;
//----
	default:
		return SVQ3_GameSystemCalls(args);
	}
	return -1;
}

/*
===============
SV_ShutdownGameProgs

Called every time a map changes
===============
*/
void SV_ShutdownGameProgs(void)
{
	if (!gvm)
	{
		return;
	}
	VM_Call(gvm, Q3GAME_SHUTDOWN, qfalse);
	VM_Free(gvm);
	gvm = NULL;
}

/*
==================
SV_InitGameVM

Called for both a full init and a restart
==================
*/
static void SV_InitGameVM(qboolean restart)
{
	int i;

	// start the entity parsing at the beginning
	sv.q3_entityParsePoint = CM_EntityString();

	// clear all gentity pointers that might still be set from
	// a previous level
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=522
	//   now done before GAME_INIT call
	for (i = 0; i < sv_maxclients->integer; i++)
	{
		svs.clients[i].q3_gentity = NULL;
	}

	// use the current msec count for a random seed
	// init for this gamestate
	VM_Call(gvm, Q3GAME_INIT, svs.q3_time, Com_Milliseconds(), restart);
}



/*
===================
SV_RestartGameProgs

Called on a map_restart, but not on a normal map change
===================
*/
void SV_RestartGameProgs(void)
{
	if (!gvm)
	{
		return;
	}
	VM_Call(gvm, Q3GAME_SHUTDOWN, qtrue);

	// do a restart instead of a free
	gvm = VM_Restart(gvm);
	if (!gvm)		// bk001212 - as done below
	{
		Com_Error(ERR_FATAL, "VM_Restart on game failed");
	}

	SV_InitGameVM(qtrue);
}


/*
===============
SV_InitGameProgs

Called on a normal map change, not on a map_restart
===============
*/
void SV_InitGameProgs(void)
{
	Cvar* var;
	//FIXME these are temp while I make bots run in vm
	extern int bot_enable;

	var = Cvar_Get("bot_enable", "1", CVAR_LATCH2);
	if (var)
	{
		bot_enable = var->integer;
	}
	else
	{
		bot_enable = 0;
	}

	// load the dll or bytecode
	gvm = VM_Create("qagame", SV_GameSystemCalls, (vmInterpret_t)(int)Cvar_VariableValue("vm_game"));
	if (!gvm)
	{
		Com_Error(ERR_FATAL, "VM_Create on game failed");
	}

	SV_InitGameVM(qfalse);
}


/*
====================
SV_GameCommand

See if the current console command is claimed by the game
====================
*/
qboolean SV_GameCommand(void)
{
	if (sv.state != SS_GAME)
	{
		return qfalse;
	}

	return VM_Call(gvm, Q3GAME_CONSOLE_COMMAND);
}
