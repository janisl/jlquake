/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// server.h

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../game/g_public.h"
#include "../game/bg_public.h"
#include "../../server/server.h"

//=============================================================================

#define MAX_ENT_CLUSTERS    16

typedef struct svEntity_s
{
	struct worldSector_s* worldSector;
	struct svEntity_s* nextEntityInWorldSector;

	wsentityState_t baseline;			// for delta compression of initial sighting
	int numClusters;				// if -1, use headnode instead
	int clusternums[MAX_ENT_CLUSTERS];
	int lastCluster;				// if all the clusters don't fit in clusternums
	int areanum, areanum2;
	int snapshotCounter;			// used to prevent double adding from portal views
} svEntity_t;

typedef enum {
	SS_DEAD,			// no map loaded
	SS_LOADING,			// spawning level entities
	SS_GAME				// actively running
} serverState_t;

typedef struct
{
	serverState_t state;
	qboolean restarting;				// if true, send configstring changes during SS_LOADING
	int serverId;						// changes each server start
	int restartedServerId;				// serverId before a map_restart
	int checksumFeed;					//
	int snapshotCounter;				// incremented for each snapshot built
	int timeResidual;					// <= 1000 / sv_frame->value
	int nextFrameTime;					// when time > nextFrameTime, process world
	struct cmodel_s* models[MAX_MODELS_Q3];
	char* configstrings[MAX_CONFIGSTRINGS_WS];
	svEntity_t svEntities[MAX_GENTITIES_Q3];

	const char* entityParsePoint;				// used during game VM init

	// the game virtual machine will update these on init and changes
	sharedEntity_t* gentities;
	int gentitySize;
	int num_entities;					// current number, <= MAX_GENTITIES_Q3

	wsplayerState_t* gameClients;
	int gameClientSize;					// will be > sizeof(wsplayerState_t) due to game private data

	int restartTime;
} server_t;





typedef struct
{
	int areabytes;
	byte areabits[MAX_MAP_AREA_BYTES];					// portalarea visibility bits
	wsplayerState_t ps;
	int num_entities;
	int first_entity;					// into the circular sv_packet_entities[]
										// the entities MUST be in increasing state number
										// order, otherwise the delta compression will fail
	int messageSent;					// time the message was transmitted
	int messageAcked;					// time the message was acked
	int messageSize;					// used to rate drop packets
} clientSnapshot_t;

// RF, now using a global string buffer to hold all reliable commands
//#define	RELIABLE_COMMANDS_MULTI		128
//#define	RELIABLE_COMMANDS_SINGLE	256		// need more for loadgame situations

#define RELIABLE_COMMANDS_CHARS     384		// we can scale this down from the max of 1024, since not all commands are going to use that many chars

typedef struct
{
	int bufSize;
	char* buf;					// actual strings
	char** commands;			// pointers to actual strings
	int* commandLengths;		// lengths of actual strings
	//
	char* rover;
} reliableCommands_t;

struct client_t : public client_common_t
{

	//char			reliableCommands[MAX_RELIABLE_COMMANDS_WS][MAX_STRING_CHARS];
	reliableCommands_t reliableCommands;
	int reliableSequence;					// last added reliable message, not necesarily sent or acknowledged yet
	int reliableAcknowledge;				// last acknowledged reliable message
	int reliableSent;						// last sent reliable message, not necesarily acknowledged yet
	int messageAcknowledge;

	int gamestateMessageNum;				// netchan->outgoingSequence of gamestate
	int challenge;

	wsusercmd_t lastUsercmd;
	int lastMessageNum;					// for delta compression
	int lastClientCommand;				// reliable client message sequence
	char lastClientCommandString[MAX_STRING_CHARS];
	sharedEntity_t* gentity;			// SV_GentityNum(clientnum)
	char name[MAX_NAME_LENGTH_WS];						// extracted from userinfo, high bits masked

	// downloading
	char downloadName[MAX_QPATH];			// if not empty string, we are downloading
	fileHandle_t download;				// file being downloaded
	int downloadSize;					// total bytes (can't use EOF because of paks)
	int downloadCount;					// bytes sent
	int downloadClientBlock;				// last block we sent to the client, awaiting ack
	int downloadCurrentBlock;				// current block number
	int downloadXmitBlock;				// last block we xmited
	unsigned char* downloadBlocks[MAX_DOWNLOAD_WINDOW];		// the buffers for the download blocks
	int downloadBlockSize[MAX_DOWNLOAD_WINDOW];
	qboolean downloadEOF;				// We have sent the EOF block
	int downloadSendTime;				// time we last got an ack from the client

	int deltaMessage;					// frame last client usercmd message
	int nextReliableTime;				// svs.time when another reliable command will be allowed
	int lastPacketTime;					// svs.time when packet was last received
	int lastConnectTime;				// svs.time when connection started
	int nextSnapshotTime;				// send another snapshot when svs.time >= nextSnapshotTime
	qboolean rateDelayed;				// true if nextSnapshotTime was set based on rate instead of snapshotMsec
	int timeoutCount;					// must timeout a few frames in a row so debugging doesn't break
	clientSnapshot_t frames[PACKET_BACKUP_Q3];		// updates can be delta'd from here
	int ping;
	int rate;							// bytes / second
	int snapshotMsec;					// requests a snapshot every snapshotMsec unless rate choked
	int pureAuthentic;
	netchan_t netchan;
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
} challenge_t;


#define MAX_MASTERS 8				// max recipients for heartbeat packets


// this structure will be cleared only when the game dll changes
typedef struct
{
	qboolean initialized;					// sv_init has completed

	int time;								// will be strictly increasing across level changes

	int snapFlagServerBit;					// ^= SNAPFLAG_SERVERCOUNT every SV_SpawnServer()

	client_t* clients;						// [sv_maxclients->integer];
	int numSnapshotEntities;				// sv_maxclients->integer*PACKET_BACKUP_Q3*MAX_PACKET_ENTITIES
	int nextSnapshotEntities;				// next snapshotEntities to use
	wsentityState_t* snapshotEntities;			// [numSnapshotEntities]
	int nextHeartbeatTime;
	challenge_t challenges[MAX_CHALLENGES];	// to prevent invalid IPs from connecting
	netadr_t redirectAddress;				// for rcon return messages

	netadr_t authorizeAddress;				// for rcon return messages
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

// Rafael gameskill
extern Cvar* sv_gameskill;
// done

extern Cvar* sv_reloading;		//----(SA)	added

//===========================================================

//
// sv_main.c
//
void SV_FinalMessage(const char* message);
void QDECL SV_SendServerCommand(client_t* cl, const char* fmt, ...);


void SV_AddOperatorCommands(void);
void SV_RemoveOperatorCommands(void);


void SV_MasterHeartbeat(void);
void SV_MasterShutdown(void);




//
// sv_init.c
//
void SV_SetConfigstring(int index, const char* val);
void SV_GetConfigstring(int index, char* buffer, int bufferSize);

void SV_SetUserinfo(int index, const char* val);
void SV_GetUserinfo(int index, char* buffer, int bufferSize);

void SV_ChangeMaxClients(void);
void SV_SpawnServer(char* server, qboolean killBots);

//RF, reliable commands
const char* SV_GetReliableCommand(client_t* cl, int index);
void SV_FreeAcknowledgedReliableCommands(client_t* cl);
qboolean SV_AddReliableCommand(client_t* cl, int index, const char* cmd);
void SV_InitReliableCommandsForClient(client_t* cl, int commands);
void SV_FreeReliableCommandsForClient(client_t* cl);


//
// sv_client.c
//
void SV_GetChallenge(netadr_t from);

void SV_DirectConnect(netadr_t from);

void SV_AuthorizeIpPacket(netadr_t from);

void SV_ExecuteClientMessage(client_t* cl, QMsg* msg);
void SV_UserinfoChanged(client_t* cl);

void SV_ClientEnterWorld(client_t* client, wsusercmd_t* cmd);
void SV_DropClient(client_t* drop, const char* reason);

void SV_ExecuteClientCommand(client_t* cl, const char* s, qboolean clientOK);
void SV_ClientThink(client_t* cl, wsusercmd_t* cmd);

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
int SV_NumForGentity(sharedEntity_t* ent);
sharedEntity_t* SV_GentityNum(int num);
wsplayerState_t* SV_GameClientNum(int num);
svEntity_t* SV_SvEntityForGentity(sharedEntity_t* gEnt);
sharedEntity_t* SV_GEntityForSvEntity(svEntity_t* svEnt);
void        SV_InitGameProgs(void);
void        SV_ShutdownGameProgs(void);
void        SV_RestartGameProgs(void);
qboolean    SV_inPVS(const vec3_t p1, const vec3_t p2);
qboolean SV_GetTag(int clientNum, char* tagname, orientation_t* _1or);

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

void SV_ClearWorld(void);
// called after the world model has been loaded, before linking any entities

void SV_UnlinkEntity(sharedEntity_t* ent);
// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself

void SV_LinkEntity(sharedEntity_t* ent);
// Needs to be called any time an entity changes origin, mins, maxs,
// or solid.  Automatically unlinks if needed.
// sets ent->v.absmin and ent->v.absmax
// sets ent->leafnums[] for pvs determination even if the entity
// is not solid


clipHandle_t SV_ClipHandleForEntity(const sharedEntity_t* ent);


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
void SV_Netchan_Transmit(client_t* client, QMsg* msg);		//int length, const byte *data );
void SV_Netchan_TransmitNextFragment(netchan_t* chan);
qboolean SV_Netchan_Process(client_t* client, QMsg* msg);
