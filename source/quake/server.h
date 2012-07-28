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

// edict->deadflag values
#define DEAD_NO                 0
#define DEAD_DYING              1
#define DEAD_DEAD               2

//============================================================================

extern Cvar* skill;

extern client_t* host_client;

extern jmp_buf host_abortserver;

extern double host_time;

extern qhedict_t* sv_player;

//===========================================================

void SV_Init(void);

void SV_DropClient(qboolean crash);

void SV_SendClientMessages(void);
void SV_ClearDatagram(void);

void SV_SetIdealPitch(void);

void SV_AddUpdates(void);

void SV_ClientThink(void);
void SV_AddClientToServer(qsocket_t* ret);

void SV_WriteClientdataToMessage(qhedict_t* ent, QMsg* msg);

void SV_CheckForNewClients(void);
void SV_RunClients(void);
void SV_SaveSpawnparms();
void SV_SpawnServer(char* server);
