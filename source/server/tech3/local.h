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

#ifndef _TECH3_LOCAL_H
#define _TECH3_LOCAL_H

//
//	Bot
//
extern int bot_enable;

int SVT3_BotAllocateClient(int clientNum);
void SVT3_BotFreeClient(int clientNum);
int SVT3_BotLibSetup();
void SVT3_BotInitCvars();
void SVT3_BotInitBotLib();
bool SVT3_BotGetConsoleMessage(int client, char* buf, int size);
int SVT3_BotGetSnapshotEntity(int client, int ent);
void SVT3_BotFrame(int time);

//
//	CCmds
//
void SVT3_Heartbeat_f();
void SVET_TempBanNetAddress(netadr_t address, int length);

//
//	Client
//
void SVT3_CloseDownload(client_t* cl);
void SVT3_DropClient(client_t* drop, const char* reason);

//
//	Game
//
extern vm_t* gvm;							// game virtual machine

idEntity3* SVT3_EntityNum(int number);
idEntity3* SVT3_EntityForSvEntity(const q3svEntity_t* svEnt);
bool SVT3_inPVS(const vec3_t p1, const vec3_t p2);
bool SVT3_inPVSIgnorePortals(const vec3_t p1, const vec3_t p2);

bool SVT3_BotVisibleFromPos(vec3_t srcpos, int srcnum, vec3_t destpos, int destnum, bool updateVisPos);
bool SVT3_BotCheckAttackAtPos(int entnum, int enemy, vec3_t pos, bool ducking, bool allowHitWorld);
bool SVT3_EntityContact(const vec3_t mins, const vec3_t maxs, const idEntity3* ent, int capsule);
void SVT3_SetBrushModel(idEntity3* ent, q3svEntity_t* svEnt, const char* name);
void SVT3_GetServerinfo(char* buffer, int bufferSize);
void SVT3_AdjustAreaPortalState(q3svEntity_t* svEnt, bool open);
bool SVT3_GetEntityToken(char* buffer, int length);
void SVT3_GameSendServerCommand(int clientNum, const char* text);
void SVT3_GameDropClient(int clientNum, const char* reason, int length);
bool SVT3_GetTag(int clientNum, int tagFileNumber, const char* tagname, orientation_t* _or);
void SVT3_InitGameProgs();
void SVT3_RestartGameProgs();
void SVT3_ShutdownGameProgs();

//
//	Init
//
bool SVT3_AddReliableCommand(client_t* cl, int index, const char* cmd);
const char* SVT3_GetReliableCommand(client_t* cl, int index);
void SVT3_SetConfigstring(int index, const char* val);
void SVT3_SetConfigstringNoUpdate(int index, const char* val);
void SVET_UpdateConfigStrings();
void SVT3_GetConfigstring(int index, char* buffer, int bufferSize);
void SVT3_SetUserinfo(int index, const char* val);
void SVT3_GetUserinfo(int index, char* buffer, int bufferSize);
void SVT3_CreateBaseline();
void SVWS_InitReliableCommandsForClient(client_t* cl, int commands);
void SVWS_InitReliableCommands(client_t* clients);
void SVWS_FreeReliableCommandsForClient(client_t* cl);
void SVWS_FreeAcknowledgedReliableCommands(client_t* cl);
void SVT3_Startup();
void SVT3_ChangeMaxClients();
void SVT3_ClearServer();
void SVT3_TouchCGame();
void SVT3_TouchCGameDLL();

//
//	Main
//
extern Cvar* svt3_gametype;
extern Cvar* svt3_pure;
extern Cvar* svt3_padPackets;
extern Cvar* svt3_maxRate;
extern Cvar* svt3_dl_maxRate;

void SVT3_AddServerCommand(client_t* client, const char* cmd);
void SVT3_SendServerCommand(client_t* cl, const char* fmt, ...) id_attribute((format(printf, 2, 3)));
int SVET_LoadTag(const char* mod_name);

//
//	Snapshot
//
void SVT3_WriteSnapshotToClient(client_t* client, QMsg* msg);
void SVT3_UpdateServerCommandsToClient(client_t* client, QMsg* msg);
void SVT3_BuildClientSnapshot(client_t* client);
int SVT3_RateMsec(client_t* client, int messageSize);

//
//	World
//
void SVT3_SectorList_f();
// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself
void SVT3_UnlinkEntity(idEntity3* ent, q3svEntity_t* svent);
// Needs to be called any time an entity changes origin, mins, maxs,
// or solid.  Automatically unlinks if needed.
// sets ent->v.absmin and ent->v.absmax
// sets ent->leafnums[] for pvs determination even if the entity
// is not solid
void SVT3_LinkEntity(idEntity3* ent, q3svEntity_t* svent);
// fills in a table of entity numbers with entities that have bounding boxes
// that intersect the given area.  It is possible for a non-axial bmodel
// to be returned that doesn't actually intersect the area on an exact
// test.
// returns the number of pointers filled in
// The world entity is never returned in this list.
int SVT3_AreaEntities(const vec3_t mins, const vec3_t maxs, int* entityList, int maxcount);
clipHandle_t SVT3_ClipHandleForEntity(const idEntity3* ent);
// clip to a specific entity
void SVT3_ClipToEntity(q3trace_t* trace, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int entityNum, int contentmask, int capsule);
//	mins and maxs are relative
//	if the entire move stays in a solid volume, trace.allsolid will be set,
// trace.startsolid will be set, and trace.fraction will be 0
//	if the starting point is in a solid, it will be allowed to move out
// to an open area
//	passEntityNum is explicitly excluded from clipping checks (normally Q3ENTITYNUM_NONE)
void SVT3_Trace(q3trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask, int capsule);
// returns the CONTENTS_* value from the world and all entities at the given point.
int SVT3_PointContents(const vec3_t p, int passEntityNum);


bool CL_GetTag(int clientNum, const char* tagname, orientation_t* _or);

#endif
