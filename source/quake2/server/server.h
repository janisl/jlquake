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
#include "../../server/quake2/local.h"

#include "../../common/file_formats/bsp38.h"

//define	PARANOID			// speed sapping error checking

#include "../qcommon/qcommon.h"

//=============================================================================

// a client can leave the server in one of four ways:
// dropping properly by quiting or disconnecting
// timing out if no valid messages are received for timeout.value seconds
// getting kicked off by the server operator
// a program error, like an overflowed reliable buffer

extern netadr_t net_from;
extern QMsg net_message;

extern Cvar* sv_paused;
extern Cvar* sv_noreload;					// don't reload level state when reentering
extern Cvar* sv_airaccelerate;				// don't reload level state when reentering
											// development tool

//===========================================================

//
// sv_main.c
//
void SV_FinalMessage(const char* message, qboolean reconnect);

void SV_WriteClientdataToMessage(client_t* client, QMsg* msg);

void SV_InitOperatorCommands(void);

void SV_SendServerinfo(client_t* client);

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
typedef enum {RD_NONE, RD_PACKET} redirect_t;
#define SV_OUTPUTBUF_LENGTH (MAX_MSGLEN_Q2 - 16)

extern char sv_outputbuf[SV_OUTPUTBUF_LENGTH];

void SV_FlushRedirect(int sv_redirected, char* outputbuf);

void SV_DemoCompleted(void);
void SV_SendClientMessages(void);

//
// sv_user.c
//
void SV_ExecuteClientMessage(client_t* cl);

//
// sv_game.c
//
void SV_InitGameProgs(void);
void SV_InitEdict(q2edict_t* e);
