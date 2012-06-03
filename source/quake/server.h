/*
Copyright (C) 1996-1997 Id Software, Inc.

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

#include "../server/server.h"

typedef struct
{
	int maxclients;
	int maxclientslimit;
	struct client_t* clients;		// [maxclients]
	int serverflags;				// episode completion information
	qboolean changelevel_issued;	// cleared when at SV_SpawnServer
} server_static_t;

//=============================================================================

// edict->solid values
#define SOLID_NOT               0		// no interaction with other objects
#define SOLID_TRIGGER           1		// touch on edge, but not blocking
#define SOLID_BBOX              2		// touch on edge, block
#define SOLID_SLIDEBOX          3		// touch on edge, but not an onground
#define SOLID_BSP               4		// bsp clip, touch on edge, block

// edict->deadflag values
#define DEAD_NO                 0
#define DEAD_DYING              1
#define DEAD_DEAD               2

#define DAMAGE_NO               0
#define DAMAGE_YES              1
#define DAMAGE_AIM              2

// edict->flags
#define FL_FLY                  1
#define FL_SWIM                 2
//#define	FL_GLIMPSE				4
#define FL_CONVEYOR             4
#define FL_CLIENT               8
#define FL_INWATER              16
#define FL_MONSTER              32
#define FL_GODMODE              64
#define FL_NOTARGET             128
#define FL_ITEM                 256
#define FL_ONGROUND             512
#define FL_PARTIALGROUND        1024	// not all corners are valid
#define FL_WATERJUMP            2048	// player jumping out of water
#define FL_JUMPRELEASED         4096	// for jump debouncing

#define SPAWNFLAG_NOT_EASY          256
#define SPAWNFLAG_NOT_MEDIUM        512
#define SPAWNFLAG_NOT_HARD          1024
#define SPAWNFLAG_NOT_DEATHMATCH    2048

//============================================================================

extern Cvar* teamplay;
extern Cvar* skill;
extern Cvar* deathmatch;
extern Cvar* coop;
extern Cvar* fraglimit;
extern Cvar* timelimit;

extern server_static_t svs;					// persistant server info
extern server_t sv;							// local server

extern client_t* host_client;

extern jmp_buf host_abortserver;

extern double host_time;

extern qhedict_t* sv_player;

//===========================================================

void SV_Init(void);

void SV_StartParticle(vec3_t org, vec3_t dir, int color, int count);
void SV_StartSound(qhedict_t* entity, int channel, const char* sample, int volume,
	float attenuation);

void SV_DropClient(qboolean crash);

void SV_SendClientMessages(void);
void SV_ClearDatagram(void);

int SV_ModelIndex(const char* name);

void SV_SetIdealPitch(void);

void SV_AddUpdates(void);

void SV_ClientThink(void);
void SV_AddClientToServer(qsocket_t* ret);

void SV_ClientPrintf(const char* fmt, ...);
void SV_BroadcastPrintf(const char* fmt, ...);

void SV_Physics(void);

qboolean SV_CheckBottom(qhedict_t* ent);
qboolean SV_movestep(qhedict_t* ent, vec3_t move, qboolean relink);

void SV_WriteClientdataToMessage(qhedict_t* ent, QMsg* msg);

void SV_MoveToGoal(void);

void SV_CheckForNewClients(void);
void SV_RunClients(void);
void SV_SaveSpawnparms();
void SV_SpawnServer(char* server);
