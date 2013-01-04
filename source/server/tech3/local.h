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

#include "../global.h"

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
bool SVET_TempBanIsBanned(netadr_t address);
void SVT3_AddOperatorCommands();

//
//	Client
//
extern ucmd_t q3_ucmds[];
extern ucmd_t et_ucmds[];

void SVT3_DropClient(client_t* drop, const char* reason);
void SVQ3_ClientEnterWorld(client_t* client, q3usercmd_t* cmd);
void SVWS_ClientEnterWorld(client_t* client, wsusercmd_t* cmd);
void SVWM_ClientEnterWorld(client_t* client, wmusercmd_t* cmd);
void SVET_ClientEnterWorld(client_t* client, etusercmd_t* cmd);
void SVT3_WriteDownloadToClient(client_t* cl, QMsg* msg);
void SVT3_GetChallenge(netadr_t from);
void SVT3_AuthorizeIpPacket(netadr_t from);
void SVT3_DirectConnect(netadr_t from);
void SVT3_ExecuteClientMessage(client_t* cl, QMsg* msg);

//
//	Game
//
extern vm_t* gvm;							// game virtual machine

idEntity3* SVT3_EntityNum(int number);
idEntity3* SVT3_EntityForSvEntity(const q3svEntity_t* svEnt);
idPlayerState3* SVT3_GameClientNum(int num);
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
const char* SVT3_GameClientConnect(int clientNum, bool firstTime, bool isBot);
void SVT3_GameClientCommand(int clientNum);
void SVT3_GameRunFrame(int time);
void SVT3_GameClientBegin(int clientNum);

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
void SVWS_InitReliableCommandsForClient(client_t* cl, int commands);
void SVWS_FreeReliableCommandsForClient(client_t* cl);
void SVWS_FreeAcknowledgedReliableCommands(client_t* cl);
void SVT3_ClearServer();
void SVT3_FinalCommand(const char* cmd, bool disconnect);
void SVT3_SpawnServer(const char* server, bool killBots);
void SVT3_Shutdown(const char* finalmsg);
void SVT3_Init();

//
//	Main
//
extern Cvar* svt3_gametype;
extern Cvar* svt3_pure;
extern Cvar* svt3_padPackets;
extern Cvar* svt3_maxRate;
extern Cvar* svt3_dl_maxRate;
extern Cvar* svt3_mapname;
extern Cvar* svt3_timeout;
extern Cvar* svt3_zombietime;
extern Cvar* svt3_allowDownload;
extern Cvar* svet_wwwFallbackURL;
extern Cvar* svet_wwwDownload;// general flag to enable/disable www download redirects
extern Cvar* svet_wwwBaseURL;	// the base URL of all the files
// tell clients to perform their downloads while disconnected from the server
// this gets you a better throughput, but you loose the ability to control the download usage
extern Cvar* svet_wwwDlDisconnected;
extern Cvar* svt3_lanForceRate;
extern Cvar* svwm_showAverageBPS;				// NERVE - SMF - net debugging
extern Cvar* svet_tempbanmessage;
extern Cvar* svwm_onlyVisibleClients;
extern Cvar* svq3_strictAuth;
extern Cvar* svet_fullmsg;
extern Cvar* svt3_reconnectlimit;
extern Cvar* svt3_minPing;
extern Cvar* svt3_maxPing;
extern Cvar* svt3_privatePassword;
extern Cvar* svt3_privateClients;
extern Cvar* svt3_floodProtect;
extern Cvar* svt3_reloading;
extern Cvar* svt3_master[Q3MAX_MASTER_SERVERS];
extern Cvar* svt3_gameskill;
extern Cvar* svt3_allowAnonymous;
extern Cvar* svt3_friendlyFire;			// NERVE - SMF
extern Cvar* svt3_maxlives;				// NERVE - SMF
extern Cvar* svwm_tourney;				// NERVE - SMF
extern Cvar* svet_needpass;
extern Cvar* svt3_rconPassword;
extern Cvar* svt3_fps;
extern Cvar* svt3_killserver;

void SVT3_AddServerCommand(client_t* client, const char* cmd);
void SVT3_SendServerCommand(client_t* cl, const char* fmt, ...) id_attribute((format(printf, 2, 3)));
int SVET_LoadTag(const char* mod_name);
void SVT3_MasterShutdown();
void SVT3_MasterGameCompleteStatus();
void SVT3_Frame(int msec);

//
//	NetChan
//
void SVT3_Netchan_TransmitNextFragment(client_t* client);
void SVT3_Netchan_Transmit(client_t* client, QMsg* msg);
bool SVT3_Netchan_Process(client_t* client, QMsg* msg);

//
//	Snapshot
//
void SVT3_UpdateServerCommandsToClient(client_t* client, QMsg* msg);
void SVT3_SendMessageToClient(QMsg* msg, client_t* client);
void SVT3_SendClientSnapshot(client_t* client);
void SVT3_SendClientMessages();

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

#endif
