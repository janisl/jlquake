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

typedef struct
{
	int maxclients;
	int maxclientslimit;
	struct client_s* clients;		// [maxclients]
	int serverflags;				// episode completion information
	qboolean changelevel_issued;	// cleared when at SV_SpawnServer
} server_static_t;

//=============================================================================

typedef enum {ss_loading, ss_active} server_state_t;

typedef struct
{
	qboolean active;				// false if only a net client

	qboolean paused;
	qboolean loadgame;				// handle connections specially

	double time;

	int lastcheck;					// used by PF_checkclient
	double lastchecktime;

	char name[64];					// map name
	char modelname[64];				// maps/<name>.bsp, for model_precache[0]
	const char* model_precache[MAX_MODELS_Q1];	// NULL terminated
	clipHandle_t models[MAX_MODELS_Q1];
	const char* sound_precache[MAX_SOUNDS_Q1];	// NULL terminated
	const char* lightstyles[MAX_LIGHTSTYLES_Q1];
	int num_edicts;
	int max_edicts;
	qhedict_t* edicts;					// can NOT be array indexed, because
	// qhedict_t is variable sized, but can
	// be used to reference the world ent
	server_state_t state;			// some actions are only valid during load

	QMsg datagram;
	byte datagram_buf[MAX_DATAGRAM_Q1];

	QMsg reliable_datagram;			// copied to all clients at end of frame
	byte reliable_datagram_buf[MAX_DATAGRAM_Q1];

	QMsg signon;
	byte signon_buf[MAX_MSGLEN_Q1];
} server_t;


#define NUM_PING_TIMES      16
#define NUM_SPAWN_PARMS     16

typedef struct client_s
{
	qboolean active;					// false = client is free
	qboolean spawned;					// false = don't send datagrams
	qboolean dropasap;					// has been told to go to another level
	qboolean privileged;				// can execute any host command
	qboolean sendsignon;				// only valid before spawned

	double last_message;				// reliable messages must be sent
										// periodically

	qsocket_t* netconnection;	// communications handle
	netchan_t netchan;

	q1usercmd_t cmd;					// movement
	vec3_t wishdir;						// intended motion calced from cmd

	QMsg message;						// can be added to at any time,
										// copied and clear once per frame
	byte msgbuf[MAX_MSGLEN_Q1];
	qhedict_t* edict;						// EDICT_NUM(clientnum+1)
	char name[32];						// for printing to other people
	int colors;

	float ping_times[NUM_PING_TIMES];
	int num_pings;						// ping_times[num_pings%NUM_PING_TIMES]

// spawn parms are carried from level to level
	float spawn_parms[NUM_SPAWN_PARMS];

// client known data for deltas
	int old_frags;
} client_t;


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
