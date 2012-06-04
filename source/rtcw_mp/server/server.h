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

// server.h

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../game/g_public.h"
#include "../game/bg_public.h"
#include "../../server/server.h"
#include "../../server/tech3/local.h"
#include "../../server/wolfmp/local.h"

//=============================================================================

#define AUTHORIZE_TIMEOUT   5000

#define MAX_MASTERS 8				// max recipients for heartbeat packets


//================
// DHM - Nerve
#ifdef UPDATE_SERVER

typedef struct
{
	char version[MAX_QPATH];
	char platform[MAX_QPATH];
	char installer[MAX_QPATH];
} versionMapping_t;


#define MAX_UPDATE_VERSIONS 128
extern versionMapping_t versionMap[MAX_UPDATE_VERSIONS];
extern int numVersions;
// Maps client version to appropriate installer

#endif
// DHM - Nerve

//=============================================================================

extern vm_t* gvm;							// game virtual machine


#define MAX_MASTER_SERVERS  5

extern Cvar* sv_fps;
extern Cvar* sv_timeout;
extern Cvar* sv_zombietime;
extern Cvar* sv_rconPassword;
extern Cvar* sv_privatePassword;
extern Cvar* sv_allowDownload;
extern Cvar* sv_friendlyFire;			// NERVE - SMF
extern Cvar* sv_maxlives;				// NERVE - SMF
extern Cvar* sv_tourney;				// NERVE - SMF
extern Cvar* sv_maxclients;

extern Cvar* sv_privateClients;
extern Cvar* sv_hostname;
extern Cvar* sv_master[MAX_MASTER_SERVERS];
extern Cvar* sv_reconnectlimit;
extern Cvar* sv_showloss;
extern Cvar* sv_padPackets;
extern Cvar* sv_killserver;
extern Cvar* sv_mapname;
extern Cvar* sv_mapChecksum;
extern Cvar* sv_serverid;
extern Cvar* sv_maxRate;
extern Cvar* sv_minPing;
extern Cvar* sv_maxPing;
extern Cvar* sv_gametype;
extern Cvar* sv_pure;
extern Cvar* sv_floodProtect;
extern Cvar* sv_allowAnonymous;
extern Cvar* sv_lanForceRate;
extern Cvar* sv_onlyVisibleClients;

extern Cvar* sv_showAverageBPS;				// NERVE - SMF - net debugging

// Rafael gameskill
extern Cvar* sv_gameskill;
// done

// TTimo - autodl
extern Cvar* sv_dl_maxRate;


//===========================================================

//
// sv_main.c
//
void SV_FinalMessage(const char* message);
void QDECL SV_SendServerCommand(client_t* cl, const char* fmt, ...);


void SV_AddOperatorCommands(void);
void SV_RemoveOperatorCommands(void);


void SV_MasterHeartbeat(const char* hbname);
void SV_MasterShutdown(void);

void SV_MasterGameCompleteStatus();		// NERVE - SMF



//
// sv_init.c
//
void SV_SetConfigstring(int index, const char* val);
void SV_GetConfigstring(int index, char* buffer, int bufferSize);

void SV_SetUserinfo(int index, const char* val);
void SV_GetUserinfo(int index, char* buffer, int bufferSize);

void SV_ChangeMaxClients(void);
void SV_SpawnServer(char* server, qboolean killBots);



//
// sv_client.c
//
void SV_GetChallenge(netadr_t from);

void SV_DirectConnect(netadr_t from);

void SV_AuthorizeIpPacket(netadr_t from);

void SV_ExecuteClientMessage(client_t* cl, QMsg* msg);
void SV_UserinfoChanged(client_t* cl);

void SV_ClientEnterWorld(client_t* client, wmusercmd_t* cmd);
void SV_DropClient(client_t* drop, const char* reason);

void SV_ExecuteClientCommand(client_t* cl, const char* s, qboolean clientOK);
void SV_ClientThink(client_t* cl, wmusercmd_t* cmd);

void SV_WriteDownloadToClient(client_t* cl, QMsg* msg);

//
// sv_ccmds.c
//
void SV_Heartbeat_f(void);

//
// sv_snapshot.c
//
void SV_AddServerCommand(client_t* client, const char* cmd);
void SV_UpdateServerCommandsToClient(client_t* client, QMsg* msg);
void SV_WriteFrameToClient(client_t* client, QMsg* msg);
void SV_SendMessageToClient(QMsg* msg, client_t* client);
void SV_SendClientMessages(void);
void SV_SendClientSnapshot(client_t* client);

//
// sv_game.c
//
wmplayerState_t* SV_GameClientNum(int num);
q3svEntity_t* SV_SvEntityForGentity(wmsharedEntity_t* gEnt);
wmsharedEntity_t* SV_GEntityForSvEntity(q3svEntity_t* svEnt);
void        SV_InitGameProgs(void);
void        SV_ShutdownGameProgs(void);
void        SV_RestartGameProgs(void);
qboolean    SV_inPVS(const vec3_t p1, const vec3_t p2);
qboolean SV_GetTag(int clientNum, char* tagname, orientation_t* _or);

//
// sv_bot.c
//
void        SV_BotFrame(int time);
int         SV_BotAllocateClient(void);
void        SV_BotFreeClient(int clientNum);

void        SV_BotInitCvars(void);
int         SV_BotLibSetup(void);
int         SV_BotLibShutdown(void);
int         SV_BotGetSnapshotEntity(int client, int ent);
int         SV_BotGetConsoleMessage(int client, char* buf, int size);

//============================================================
//
// high level object sorting to reduce interaction tests
//

void SV_UnlinkEntity(wmsharedEntity_t* ent);
// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself

void SV_LinkEntity(wmsharedEntity_t* ent);
// Needs to be called any time an entity changes origin, mins, maxs,
// or solid.  Automatically unlinks if needed.
// sets ent->v.absmin and ent->v.absmax
// sets ent->leafnums[] for pvs determination even if the entity
// is not solid


clipHandle_t SV_ClipHandleForEntity(const wmsharedEntity_t* ent);


void SV_SectorList_f(void);


int SV_AreaEntities(const vec3_t mins, const vec3_t maxs, int* entityList, int maxcount);
// fills in a table of entity numbers with entities that have bounding boxes
// that intersect the given area.  It is possible for a non-axial bmodel
// to be returned that doesn't actually intersect the area on an exact
// test.
// returns the number of pointers filled in
// The world entity is never returned in this list.


int SV_PointContents(const vec3_t p, int passEntityNum);
// returns the CONTENTS_* value from the world and all entities at the given point.


void SV_Trace(q3trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask, int capsule);
// mins and maxs are relative

// if the entire move stays in a solid volume, trace.allsolid will be set,
// trace.startsolid will be set, and trace.fraction will be 0

// if the starting point is in a solid, it will be allowed to move out
// to an open area

// passEntityNum is explicitly excluded from clipping checks (normally Q3ENTITYNUM_NONE)


void SV_ClipToEntity(q3trace_t* trace, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int entityNum, int contentmask, int capsule);
// clip to a specific entity

//
// sv_net_chan.c
//
void SV_Netchan_Transmit(client_t* client, QMsg* msg);
void SV_Netchan_TransmitNextFragment(client_t* client);
qboolean SV_Netchan_Process(client_t* client, QMsg* msg);
