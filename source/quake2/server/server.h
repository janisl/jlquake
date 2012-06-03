/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// server.h

#include "../../server/server.h"

#include "../../common/file_formats/bsp38.h"

//define	PARANOID			// speed sapping error checking

#include "../qcommon/qcommon.h"
#include "game.h"

//=============================================================================

#define MAX_MASTERS 8				// max recipients for heartbeat packets

#define EDICT_NUM(n) ((q2edict_t*)((byte*)ge->edicts + ge->edict_size * (n)))
#define NUM_FOR_EDICT(e) (((byte*)(e) - (byte*)ge->edicts) / ge->edict_size)

// a client can leave the server in one of four ways:
// dropping properly by quiting or disconnecting
// timing out if no valid messages are received for timeout.value seconds
// getting kicked off by the server operator
// a program error, like an overflowed reliable buffer

//=============================================================================

// MAX_CHALLENGES is made large to prevent a denial
// of service attack that could cycle all of them
// out before legitimate users connected
#define MAX_CHALLENGES  1024

typedef struct
{
	netadr_t adr;
	int challenge;
	int time;
} challenge_t;


typedef struct
{
	qboolean initialized;					// sv_init has completed
	int realtime;							// always increasing, no clamping, etc

	char mapcmd[MAX_TOKEN_CHARS_Q2];		// ie: *intro.cin+base

	int spawncount;							// incremented each server start
											// used to check late spawns

	client_t* clients;						// [maxclients->value];
	int num_client_entities;				// maxclients->value*UPDATE_BACKUP_Q2*MAX_PACKET_ENTITIES
	int next_client_entities;				// next client_entity to use
	q2entity_state_t* client_entities;			// [num_client_entities]

	int last_heartbeat;

	challenge_t challenges[MAX_CHALLENGES];	// to prevent invalid IPs from connecting

	// serverrecord values
	fileHandle_t demofile;
	QMsg demo_multicast;
	byte demo_multicast_buf[MAX_MSGLEN_Q2];
} serverStatic_t;

//=============================================================================

extern netadr_t net_from;
extern QMsg net_message;

extern netadr_t master_adr[MAX_MASTERS];		// address of the master server

extern serverStatic_t svs;					// persistant server info
extern server_t sv;							// local server

extern Cvar* sv_paused;
extern Cvar* maxclients;
extern Cvar* sv_noreload;					// don't reload level state when reentering
extern Cvar* sv_airaccelerate;				// don't reload level state when reentering
											// development tool
extern Cvar* sv_enforcetime;

extern client_t* sv_client;
extern q2edict_t* sv_player;

//===========================================================

//
// sv_main.c
//
void SV_FinalMessage(const char* message, qboolean reconnect);
void SV_DropClient(client_t* drop);

int SV_ModelIndex(char* name);
int SV_SoundIndex(char* name);
int SV_ImageIndex(char* name);

void SV_WriteClientdataToMessage(client_t* client, QMsg* msg);

void SV_ExecuteUserCommand(char* s);
void SV_InitOperatorCommands(void);

void SV_SendServerinfo(client_t* client);
void SV_UserinfoChanged(client_t* cl);


void Master_Heartbeat(void);
void Master_Packet(void);

//
// sv_init.c
//
void SV_InitGame(void);
void SV_Map(qboolean attractloop, char* levelstring, qboolean loadgame);


//
// sv_phys.c
//
void SV_PrepWorldFrame(void);

//
// sv_send.c
//
typedef enum {RD_NONE, RD_CLIENT, RD_PACKET} redirect_t;
#define SV_OUTPUTBUF_LENGTH (MAX_MSGLEN_Q2 - 16)

extern char sv_outputbuf[SV_OUTPUTBUF_LENGTH];

void SV_FlushRedirect(int sv_redirected, char* outputbuf);

void SV_DemoCompleted(void);
void SV_SendClientMessages(void);

void SV_Multicast(vec3_t origin, multicast_t to);
void SV_StartSound(vec3_t origin, q2edict_t* entity, int channel,
	int soundindex, float volume,
	float attenuation, float timeofs);
void SV_ClientPrintf(client_t* cl, int level, const char* fmt, ...);
void SV_BroadcastPrintf(int level, const char* fmt, ...);
void SV_BroadcastCommand(const char* fmt, ...);

//
// sv_user.c
//
void SV_Nextserver(void);
void SV_ExecuteClientMessage(client_t* cl);

//
// sv_ccmds.c
//
void SV_ReadLevelFile(void);
void SV_Status_f(void);

//
// sv_ents.c
//
void SV_WriteFrameToClient(client_t* client, QMsg* msg);
void SV_RecordDemoMessage(void);
void SV_BuildClientFrame(client_t* client);


//
// sv_game.c
//
extern game_export_t* ge;

void SV_InitGameProgs(void);
void SV_ShutdownGameProgs(void);
void SV_InitEdict(q2edict_t* e);



//============================================================

//
// high level object sorting to reduce interaction tests
//

void SV_ClearWorld(void);
// called after the world model has been loaded, before linking any entities

void SV_UnlinkEdict(q2edict_t* ent);
// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself

void SV_LinkEdict(q2edict_t* ent);
// Needs to be called any time an entity changes origin, mins, maxs,
// or solid.  Automatically unlinks if needed.
// sets ent->v.absmin and ent->v.absmax
// sets ent->leafnums[] for pvs determination even if the entity
// is not solid

int SV_AreaEdicts(vec3_t mins, vec3_t maxs, q2edict_t** list, int maxcount, int areatype);
// fills in a table of edict pointers with edicts that have bounding boxes
// that intersect the given area.  It is possible for a non-axial bmodel
// to be returned that doesn't actually intersect the area on an exact
// test.
// returns the number of pointers filled in
// ??? does this always return the world?

//===================================================================

//
// functions that interact with everything apropriate
//
int SV_PointContents(vec3_t p);
// returns the CONTENTS_* value from the world at the given point.
// Quake 2 extends this to also check entities, to allow moving liquids


q2trace_t SV_Trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, q2edict_t* passedict, int contentmask);
// mins and maxs are relative

// if the entire move stays in a solid volume, trace.allsolid will be set,
// trace.startsolid will be set, and trace.fraction will be 0

// if the starting point is in a solid, it will be allowed to move out
// to an open area

// passedict is explicitly excluded from clipping checks (normally NULL)
