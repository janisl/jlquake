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
// client.h -- primary header for client

#include "../../client/client.h"
#include "../../client/game/quake2/local.h"
#include "../../client/game/quake2/menu.h"

//define	PARANOID			// speed sapping error checking

#include "../qcommon/qcommon.h"

#include "keys.h"

//=============================================================================

extern char cl_weaponmodels[MAX_CLIENTWEAPONMODELS_Q2][MAX_QPATH];
extern int num_cl_weaponmodels;

//=============================================================================

//
// cvars
//
extern Cvar* cl_noskins;
extern Cvar* cl_autoskins;

extern Cvar* cl_showmiss;

//=============================================================================

extern netadr_t net_from;
extern QMsg net_message;

qboolean    CL_CheckOrDownloadFile(char* filename);

//=================================================

#define BLASTER_PARTICLE_COLOR      0xe0
// ========

int CL_ParseEntityBits(unsigned* bits);
void CL_ParseDelta(q2entity_state_t* from, q2entity_state_t* to, int number, int bits);
void CL_ParseFrame(void);

void CL_ParseConfigString(void);

//=================================================

void CL_PrepRefresh(void);
void CL_RegisterSounds(void);

void IN_Accumulate(void);

void CL_ParseLayout(void);

//
// cl_main
//
void CL_Init(void);

void CL_FixUpGender(void);
void CL_Disconnect(void);
void CL_Disconnect_f(void);
void CL_GetChallengePacket(void);
void CL_Snd_Restart_f(void);
void CL_RequestNextDownload(void);

//
// cl_input
//
void CL_InitInput(void);
void CL_SendCmd(void);
void CL_SendMove(q2usercmd_t* cmd);

void CL_ReadPackets(void);

int  CL_ReadFromServer(void);
void CL_WriteToServer(q2usercmd_t* cmd);
void CL_MouseEvent(int mx, int my);

//
// cl_demo.c
//
void CL_WriteDemoMessage(void);
void CL_Stop_f(void);
void CL_Record_f(void);

//
// cl_parse.c
//
extern const char* svc_strings[256];

void CL_ParseServerMessage(void);
void CL_LoadClientinfo(q2clientinfo_t* ci, const char* s);
void SHOWNET(const char* s);
void CL_ParseClientinfo(int player);
void CL_Download_f(void);

// the sound code makes callbacks to the client for entitiy position
// information, so entities can be dynamically re-spatialized
void CL_GetEntitySoundOrigin(int ent, vec3_t org);


//
// cl_pred.c
//
void CL_CheckPredictionError(void);

//
// cl_pred.c
//
void CL_PredictMovement(void);

#if id386
void x86_TimerStart(void);
void x86_TimerStop(void);
void x86_TimerInit(unsigned long smallest, unsigned longest);
unsigned long* x86_TimerGetHistogram(void);
#endif
