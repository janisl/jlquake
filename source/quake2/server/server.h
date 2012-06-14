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

#define Q2_EDICT_NUM(n) ((q2edict_t*)((byte*)ge->edicts + ge->edict_size * (n)))
#define Q2_NUM_FOR_EDICT(e) (((byte*)(e) - (byte*)ge->edicts) / ge->edict_size)

// a client can leave the server in one of four ways:
// dropping properly by quiting or disconnecting
// timing out if no valid messages are received for timeout.value seconds
// getting kicked off by the server operator
// a program error, like an overflowed reliable buffer

extern netadr_t net_from;
extern QMsg net_message;

extern netadr_t master_adr[MAX_MASTERS];		// address of the master server

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
