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

//============================================================================

extern Cvar* spawn;

extern client_t* host_client;

extern char localmodels[MAX_MODELS_Q1][5];			// inline model names for precache

extern int host_hunklevel;
extern fileHandle_t sv_logfile;

extern int sv_net_port;

//===========================================================

//
// sv_main.c
//
void SV_Shutdown(void);
void SV_Frame(float time);
void SV_FinalMessage(const char* message);

void SV_SaveSpawnparms(void);

void SV_InitOperatorCommands(void);

void SV_SendServerinfo(client_t* client);


void Master_Heartbeat(void);
void Master_Packet(void);

//
// sv_init.c
//
void SV_SpawnServer(char* server);

//
// sv_send.c
//
void SV_SendClientMessages(void);

void SV_SendMessagesToAll(void);
void SV_FindModelNumbers(void);

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
