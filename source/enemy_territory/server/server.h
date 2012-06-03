/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// server.h

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../game/g_public.h"
#include "../game/bg_public.h"
#include "../../common/file_formats/md3.h"
#include "../qcommon/qfiles.h"
#include "../../server/server.h"

//=============================================================================

#define MAX_ENT_CLUSTERS_Q3    16

#define MAX_BPS_WINDOW      20			// NERVE - SMF - net debugging

typedef struct svEntity_s
{
	struct worldSector_s* worldSector;
	struct svEntity_s* nextEntityInWorldSector;

	etentityState_t baseline;			// for delta compression of initial sighting
	int numClusters;				// if -1, use headnode instead
	int clusternums[MAX_ENT_CLUSTERS_Q3];
	int lastCluster;				// if all the clusters don't fit in clusternums
	int areanum, areanum2;
	int snapshotCounter;			// used to prevent double adding from portal views
	int originCluster;				// Gordon: calced upon linking, for origin only bmodel vis checks
} svEntity_t;

struct server_t : server_common_t
{
	qboolean restarting;				// if true, send configstring changes during SS_LOADING
	int serverId;						// changes each server start
	int restartedServerId;				// serverId before a map_restart
	int checksumFeed;					// the feed key that we use to compute the pure checksum strings
	// show_bug.cgi?id=475
	// the serverId associated with the current checksumFeed (always <= serverId)
	int checksumFeedServerId;
	int snapshotCounter;				// incremented for each snapshot built
	int timeResidual;					// <= 1000 / sv_frame->value
	int nextFrameTime;					// when time > nextFrameTime, process world
	struct cmodel_s* models[MAX_MODELS_Q3];
	char* configstrings[MAX_CONFIGSTRINGS_ET];
	qboolean configstringsmodified[MAX_CONFIGSTRINGS_ET];
	svEntity_t svEntities[MAX_GENTITIES_Q3];

	const char* entityParsePoint;				// used during game VM init

	// the game virtual machine will update these on init and changes
	etsharedEntity_t* gentities;
	int gentitySize;
	int num_entities;					// current number, <= MAX_GENTITIES_Q3

	etplayerState_t* gameClients;
	int gameClientSize;					// will be > sizeof(etplayerState_t) due to game private data

	int restartTime;

	// NERVE - SMF - net debugging
	int bpsWindow[MAX_BPS_WINDOW];
	int bpsWindowSteps;
	int bpsTotalBytes;
	int bpsMaxBytes;

	int ubpsWindow[MAX_BPS_WINDOW];
	int ubpsTotalBytes;
	int ubpsMaxBytes;

	float ucompAve;
	int ucompNum;
	// -NERVE - SMF

	md3Tag_t tags[MAX_SERVER_TAGS];
	tagHeaderExt_t tagHeadersExt[MAX_TAG_FILES];

	int num_tagheaders;
	int num_tags;
};

//=============================================================================


// MAX_CHALLENGES is made large to prevent a denial
// of service attack that could cycle all of them
// out before legitimate users connected
#define MAX_CHALLENGES  1024

#define AUTHORIZE_TIMEOUT   5000

typedef struct
{
	netadr_t adr;
	int challenge;
	int time;						// time the last packet was sent to the autherize server
	int pingTime;					// time the challenge response was sent to client
	int firstTime;					// time the adr was first used, for authorize timeout checks
	int firstPing;					// Used for min and max ping checks
	qboolean connected;
} challenge_t;

typedef struct tempBan_s
{
	netadr_t adr;
	int endtime;
} tempBan_t;


#define MAX_MASTERS                         8				// max recipients for heartbeat packets
#define MAX_TEMPBAN_ADDRESSES               MAX_CLIENTS_ET

#define SERVER_PERFORMANCECOUNTER_FRAMES    600
#define SERVER_PERFORMANCECOUNTER_SAMPLES   6

// this structure will be cleared only when the game dll changes
typedef struct
{
	qboolean initialized;					// sv_init has completed

	int time;								// will be strictly increasing across level changes

	int snapFlagServerBit;					// ^= SNAPFLAG_SERVERCOUNT every SV_SpawnServer()

	client_t* clients;						// [sv_maxclients->integer];
	int numSnapshotEntities;				// sv_maxclients->integer*PACKET_BACKUP_Q3*MAX_PACKET_ENTITIES
	int nextSnapshotEntities;				// next snapshotEntities to use
	etentityState_t* snapshotEntities;			// [numSnapshotEntities]
	int nextHeartbeatTime;
	challenge_t challenges[MAX_CHALLENGES];	// to prevent invalid IPs from connecting
	netadr_t redirectAddress;				// for rcon return messages
	tempBan_t tempBanAddresses[MAX_TEMPBAN_ADDRESSES];

#ifdef AUTHORIZE_SUPPORT
	netadr_t authorizeAddress;
#endif	// AUTHORIZE_SUPPORT

	int sampleTimes[SERVER_PERFORMANCECOUNTER_SAMPLES];
	int currentSampleIndex;
	int totalFrameTime;
	int currentFrameIndex;
	int serverLoad;
} serverStatic_t;


//=============================================================================

extern serverStatic_t svs;					// persistant server info across maps
extern server_t sv;							// cleared each map
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
extern Cvar* sv_maxclients;
extern Cvar* sv_needpass;

extern Cvar* sv_privateClients;
extern Cvar* sv_hostname;
extern Cvar* sv_master[MAX_MASTER_SERVERS];
extern Cvar* sv_reconnectlimit;
extern Cvar* sv_tempbanmessage;
extern Cvar* sv_showloss;
extern Cvar* sv_padPackets;
extern Cvar* sv_killserver;
extern Cvar* sv_mapname;
extern Cvar* sv_mapChecksum;
extern Cvar* sv_serverid;
extern Cvar* sv_maxRate;
extern Cvar* sv_minPing;
extern Cvar* sv_maxPing;
//extern	Cvar	*sv_gametype;
extern Cvar* sv_pure;
extern Cvar* sv_floodProtect;
extern Cvar* sv_allowAnonymous;
extern Cvar* sv_lanForceRate;
extern Cvar* sv_onlyVisibleClients;

extern Cvar* sv_showAverageBPS;				// NERVE - SMF - net debugging

extern Cvar* g_gameType;

// Rafael gameskill
//extern	Cvar	*sv_gameskill;
// done

extern Cvar* sv_reloading;

// TTimo - autodl
extern Cvar* sv_dl_maxRate;

// TTimo
extern Cvar* sv_wwwDownload;// general flag to enable/disable www download redirects
extern Cvar* sv_wwwBaseURL;	// the base URL of all the files
// tell clients to perform their downloads while disconnected from the server
// this gets you a better throughput, but you loose the ability to control the download usage
extern Cvar* sv_wwwDlDisconnected;
extern Cvar* sv_wwwFallbackURL;

//bani
extern Cvar* sv_cheats;
extern Cvar* sv_packetloss;
extern Cvar* sv_packetdelay;

//fretn
extern Cvar* sv_fullmsg;

//===========================================================

//
// sv_main.c
//
void SV_FinalCommand(const char* cmd, qboolean disconnect);		// ydnar: added disconnect flag so map changes can use this function as well
void QDECL SV_SendServerCommand(client_t* cl, const char* fmt, ...);


void SV_AddOperatorCommands(void);
void SV_RemoveOperatorCommands(void);


void SV_MasterHeartbeat(const char* hbname);
void SV_MasterShutdown(void);

void SV_MasterGameCompleteStatus();		// NERVE - SMF
//bani - bugtraq 12534
qboolean SV_VerifyChallenge(char* challenge);


//
// sv_init.c
//
void SV_SetConfigstringNoUpdate(int index, const char* val);
void SV_SetConfigstring(int index, const char* val);
void SV_UpdateConfigStrings(void);
void SV_GetConfigstring(int index, char* buffer, int bufferSize);

void SV_SetUserinfo(int index, const char* val);
void SV_GetUserinfo(int index, char* buffer, int bufferSize);

void SV_CreateBaseline(void);

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

void SV_ClientEnterWorld(client_t* client, etusercmd_t* cmd);
void SV_FreeClientNetChan(client_t* client);
void SV_DropClient(client_t* drop, const char* reason);

void SV_ExecuteClientCommand(client_t* cl, const char* s, qboolean clientOK, qboolean premaprestart);
void SV_ClientThink(client_t* cl, etusercmd_t* cmd);

void SV_WriteDownloadToClient(client_t* cl, QMsg* msg);

//
// sv_ccmds.c
//
void SV_Heartbeat_f(void);

qboolean SV_TempBanIsBanned(netadr_t address);
void SV_TempBanNetAddress(netadr_t address, int length);

//
// sv_snapshot.c
//
void SV_AddServerCommand(client_t* client, const char* cmd);
void SV_UpdateServerCommandsToClient(client_t* client, QMsg* msg);
void SV_WriteFrameToClient(client_t* client, QMsg* msg);
void SV_SendMessageToClient(QMsg* msg, client_t* client);
void SV_SendClientMessages(void);
void SV_SendClientSnapshot(client_t* client);
//bani
void SV_SendClientIdle(client_t* client);

//
// sv_game.c
//
int SV_NumForGentity(etsharedEntity_t* ent);

//#define SV_GentityNum( num ) ((etsharedEntity_t *)((byte *)sv.gentities + sv.gentitySize*(num)))
//#define SV_GameClientNum( num ) ((etplayerState_t *)((byte *)sv.gameClients + sv.gameClientSize*(num)))

etsharedEntity_t* SV_GentityNum(int num);
etplayerState_t* SV_GameClientNum(int num);

svEntity_t* SV_SvEntityForGentity(etsharedEntity_t* gEnt);
etsharedEntity_t* SV_GEntityForSvEntity(svEntity_t* svEnt);
void        SV_InitGameProgs(void);
void        SV_ShutdownGameProgs(void);
void        SV_RestartGameProgs(void);
qboolean    SV_inPVS(const vec3_t p1, const vec3_t p2);
qboolean SV_GetTag(int clientNum, int tagFileNumber, char* tagname, orientation_t* _or);
int SV_LoadTag(const char* mod_name);
qboolean    SV_GameIsSinglePlayer(void);
qboolean    SV_GameIsCoop(void);
void        SV_GameBinaryMessageReceived(int cno, const char* buf, int buflen, int commandTime);

//
// sv_bot.c
//
void        SV_BotFrame(int time);
int         SV_BotAllocateClient(int clientNum);
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

void SV_ClearWorld(void);
// called after the world model has been loaded, before linking any entities

void SV_UnlinkEntity(etsharedEntity_t* ent);
// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself

void SV_LinkEntity(etsharedEntity_t* ent);
// Needs to be called any time an entity changes origin, mins, maxs,
// or solid.  Automatically unlinks if needed.
// sets ent->v.absmin and ent->v.absmax
// sets ent->leafnums[] for pvs determination even if the entity
// is not solid


clipHandle_t SV_ClipHandleForEntity(const etsharedEntity_t* ent);


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

//bani - cl->downloadnotify
#define DLNOTIFY_REDIRECT   0x00000001	// "Redirecting client ..."
#define DLNOTIFY_BEGIN      0x00000002	// "clientDownload: 4 : beginning ..."
#define DLNOTIFY_ALL        (DLNOTIFY_REDIRECT | DLNOTIFY_BEGIN)
