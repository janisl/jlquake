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

#define QW_SERVER

#define MAX_MASTERS 8				// max recipients for heartbeat packets

// a client can leave the server in one of four ways:
// dropping properly by quiting or disconnecting
// timing out if no valid messages are received for timeout.value seconds
// getting kicked off by the server operator
// a program error, like an overflowed reliable buffer

//=============================================================================


#define STATFRAMES  100

// edict->deadflag values
#define DEAD_NO                 0
#define DEAD_DYING              1
#define DEAD_DEAD               2

#define DAMAGE_NO               0
#define DAMAGE_YES              1
#define DAMAGE_AIM              2

#define SPAWNFLAG_NOT_EASY          256
#define SPAWNFLAG_NOT_MEDIUM        512
#define SPAWNFLAG_NOT_HARD          1024
#define SPAWNFLAG_NOT_DEATHMATCH    2048

#define MULTICAST_ALL           0
#define MULTICAST_PHS           1
#define MULTICAST_PVS           2

#define MULTICAST_ALL_R         3
#define MULTICAST_PHS_R         4
#define MULTICAST_PVS_R         5

//============================================================================

extern Cvar* sv_mintic;
extern Cvar* sv_maxtic;
extern Cvar* sv_maxspeed;
extern Cvar* sv_highchars;

extern netadr_t master_adr[MAX_MASTERS];		// address of the master server

extern Cvar* spawn;
extern Cvar* teamplay;
extern Cvar* deathmatch;
extern Cvar* fraglimit;
extern Cvar* timelimit;

extern client_t* host_client;

extern qhedict_t* sv_player;

extern char localmodels[MAX_MODELS_Q1][5];			// inline model names for precache

extern char localinfo[MAX_LOCALINFO_STRING + 1];

extern int host_hunklevel;
extern fileHandle_t sv_logfile;
extern fileHandle_t sv_fraglogfile;

extern int sv_net_port;

//===========================================================

//
// sv_main.c
//
void SV_Shutdown(void);
void SV_Frame(float time);
void SV_FinalMessage(const char* message);
void SV_DropClient(client_t* drop);

int SV_CalcPing(client_t* cl);
void SV_FullClientUpdate(client_t* client, QMsg* buf);

int SV_ModelIndex(const char* name);

void SV_WriteClientdataToMessage(client_t* client, QMsg* msg);

void SV_SaveSpawnparms(void);

void SV_Physics_Client(qhedict_t* ent);

void SV_InitOperatorCommands(void);

void SV_SendServerinfo(client_t* client);
void SV_ExtractFromUserinfo(client_t* cl);


void Master_Heartbeat(void);
void Master_Packet(void);

//
// sv_init.c
//
void SV_SpawnServer(char* server);
void SV_FlushSignon(void);


//
// sv_phys.c
//
void SV_ProgStartFrame(void);
void SV_Physics(void);
void SV_CheckVelocity(qhedict_t* ent);
void SV_AddGravity(qhedict_t* ent, float scale);
qboolean SV_RunThink(qhedict_t* ent);
void SV_Physics_Toss(qhedict_t* ent);
void SV_RunNewmis(void);
void SV_Impact(qhedict_t* e1, qhedict_t* e2);
void SV_SetMoveVars(void);

//
// sv_send.c
//
void SV_SendClientMessages(void);

void SV_Multicast(vec3_t origin, int to);
void SV_StartSound(qhedict_t* entity, int channel, const char* sample, int volume,
	float attenuation);
void SV_ClientPrintf(client_t* cl, int level, const char* fmt, ...);
void SV_BroadcastPrintf(int level, const char* fmt, ...);
void SV_BroadcastCommand(const char* fmt, ...);
void SV_SendMessagesToAll(void);
void SV_FindModelNumbers(void);

//
// sv_user.c
//
void SV_ExecuteClientMessage(client_t* cl);
void SV_UserInit(void);
void SV_TogglePause(const char* msg);


//
// svonly.c
//
typedef enum {RD_NONE, RD_CLIENT, RD_PACKET} redirect_t;
void SV_BeginRedirect(redirect_t rd);
void SV_EndRedirect(void);

//
// sv_ccmds.c
//
void SV_Status_f(void);

//
// sv_ents.c
//
void SV_WriteEntitiesToClient(client_t* client, QMsg* msg);

//
// sv_nchan.c
//

void ClientReliableCheckBlock(client_t* cl, int maxsize);
void ClientReliable_FinishWrite(client_t* cl);
void ClientReliableWrite_Begin(client_t* cl, int c, int maxsize);
void ClientReliableWrite_Angle(client_t* cl, float f);
void ClientReliableWrite_Angle16(client_t* cl, float f);
void ClientReliableWrite_Byte(client_t* cl, int c);
void ClientReliableWrite_Char(client_t* cl, int c);
void ClientReliableWrite_Float(client_t* cl, float f);
void ClientReliableWrite_Coord(client_t* cl, float f);
void ClientReliableWrite_Long(client_t* cl, int c);
void ClientReliableWrite_Short(client_t* cl, int c);
void ClientReliableWrite_String(client_t* cl, const char* s);
void ClientReliableWrite_SZ(client_t* cl, void* data, int len);

void SV_FullClientUpdateToClient(client_t* client, client_t* cl);
