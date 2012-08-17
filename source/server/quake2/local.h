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

#ifndef _SERVER_QUAKE2_LOCAL_H
#define _SERVER_QUAKE2_LOCAL_H

#include "game.h"

#define Q2_EDICT_NUM(n) ((q2edict_t*)((byte*)ge->edicts + ge->edict_size * (n)))
#define Q2_NUM_FOR_EDICT(e) (((byte*)(e) - (byte*)ge->edicts) / ge->edict_size)

//
//	CCmds
//
client_t* SVQ2_SetPlayer();
void SVQ2_WipeSavegame(const char* savename);
void SVQ2_CopySaveGame(const char* src, const char* dst);
void SVQ2_WriteLevelFile();
void SVQ2_ReadLevelFile();
void SVQ2_WriteServerFile(bool autosave);
void SVQ2_Kick_f();
void SVQ2_Status_f();
void SVQ2_ConSay_f();
void SVQ2_Heartbeat_f();
void SVQ2_Serverinfo_f();
void SVQ2_DumpUser_f();
void SVQ2_ServerRecord_f();
void SVQ2_ServerStop_f();
void SVQ2_ServerCommand_f();

//
//	Entities
//
void SVQ2_WriteFrameToClient(client_t* client, QMsg* msg);
void SVQ2_BuildClientFrame(client_t* client);
void SVQ2_RecordDemoMessage();

//
//	Game
//
extern q2game_export_t* ge;

void SVQ2_ShutdownGameProgs();
void SVQ2_InitGameProgs();

//
//	Init
//
int SVQ2_ModelIndex(const char* name);
int SVQ2_SoundIndex(const char* name);
int SVQ2_ImageIndex(const char* name);
void SVQ2_InitGame();
void SVQ2_Map(bool attractloop, const char* levelstring, bool loadgame);

//
//	Main
//
extern Cvar* allow_download;
extern Cvar* allow_download_players;
extern Cvar* allow_download_models;
extern Cvar* allow_download_sounds;
extern Cvar* allow_download_maps;
extern Cvar* svq2_enforcetime;
extern Cvar* svq2_noreload;					// don't reload level state when reentering
extern Cvar* svq2_airaccelerate;
extern Cvar* svq2_reconnect_limit;		// minimum seconds between connect messages
extern Cvar* q2rcon_password;			// password for remote server commands
extern Cvar* q2public_server;			// should heartbeats be sent

void SVQ2_ConnectionlessPacket(const netadr_t& from, QMsg& message);
void SVQ2_MasterHeartbeat();
void SVQ2_MasterShutdown();
void SVQ2_Shutdown(const char* finalmsg, bool reconnect);

//
//	Send
//
void SVQ2_ClientPrintf(client_t* cl, int level, const char* fmt, ...) id_attribute((format(printf, 3, 4)));
void SVQ2_BroadcastPrintf(int level, const char* fmt, ...) id_attribute((format(printf, 2, 3)));
void SVQ2_Multicast(const vec3_t origin, q2multicast_t to);
void SVQ2_BroadcastCommand(const char* fmt, ...) id_attribute((format(printf, 1, 2)));
void SVQ2_StartSound(const vec3_t origin, q2edict_t* entity, int channel,
	int soundindex, float volume, float attenuation, float timeofs);
void SVQ2_SendClientMessages();

//
//	User
//
extern ucmd_t q2_ucmds[];

void SVQ2_Nextserver();
void SVQ2_UserinfoChanged(client_t* cl);
void SVQ2_DropClient(client_t* drop);
void SVQ2_ExecuteClientMessage(client_t* cl, QMsg& message);

//
//	World
//
// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself
void SVQ2_UnlinkEdict(q2edict_t* ent);
// Needs to be called any time an entity changes origin, mins, maxs,
// or solid.  Automatically unlinks if needed.
// sets ent->v.absmin and ent->v.absmax
// sets ent->leafnums[] for pvs determination even if the entity
// is not solid
void SVQ2_LinkEdict(q2edict_t* ent);
// fills in a table of edict pointers with edicts that have bounding boxes
// that intersect the given area.  It is possible for a non-axial bmodel
// to be returned that doesn't actually intersect the area on an exact
// test.
// returns the number of pointers filled in
// ??? does this always return the world?
int SVQ2_AreaEdicts(vec3_t mins, vec3_t maxs, q2edict_t** list, int maxcount, int areatype);
// returns the CONTENTS_* value from the world at the given point.
// Quake 2 extends this to also check entities, to allow moving liquids
int SVQ2_PointContents(vec3_t p);
//	mins and maxs are relative
//	if the entire move stays in a solid volume, trace.allsolid will be set,
// trace.startsolid will be set, and trace.fraction will be 0
//	if the starting point is in a solid, it will be allowed to move out
// to an open area
//	passedict is explicitly excluded from clipping checks (normally NULL)
q2trace_t SVQ2_Trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, q2edict_t* passedict, int contentmask);

#endif
