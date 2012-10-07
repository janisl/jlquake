/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// client.h -- primary header for client

#include "../game/q_shared.h"
#include "../../client/client.h"
#include "../qcommon/qcommon.h"
#include "keys.h"
#include "../../client/game/et/local.h"

//=============================================================================

//
// cvars
//
extern Cvar* cl_nodelta;
extern Cvar* cl_timegraph;
extern Cvar* cl_maxpackets;
extern Cvar* cl_packetdup;
extern Cvar* cl_shownuments;				// DHM - Nerve
extern Cvar* cl_visibleClients;				// DHM - Nerve
extern Cvar* cl_showSend;
extern Cvar* cl_timeNudge;
extern Cvar* cl_freezeDemo;

extern Cvar* cl_autorecord;

extern Cvar* cl_allowDownload;

extern Cvar* cl_missionStats;
extern Cvar* cl_waitForFire;

extern Cvar* cl_defaultProfile;

//=================================================

//
// cl_main
//

void CL_Init(void);
void CL_FlushMemory(void);
void CL_ShutdownAll(void);
void CL_AddReliableCommand(const char* cmd);

void CL_StartHunkUsers(void);

void CL_Disconnect_f(void);
void CL_GetChallengePacket(void);
void CL_Vid_Restart_f(void);
void CL_Snd_Restart_f(void);
void CL_NextDemo(void);
void CL_ReadDemoMessage(void);

void CL_InitDownloads(void);
void CL_NextDownload(void);

void CL_ShutdownRef(void);
void CL_InitRef(void);

void CL_Record(const char* name);

//
// cl_input
//
void CL_InitInput(void);
void CL_SendCmd(void);
void CL_ReadPackets(void);

void CL_WritePacket(void);
void IN_Notebook(void);

//----(SA) salute
void IN_Salute(void);
//----(SA)

void CL_VerifyCode(void);

//
// cl_parse.c
//
extern int cl_connectedToPureServer;

void CL_ParseServerMessage(QMsg* msg);

//
// cl_cgame.c
//
void CL_SetCGameTime(void);
void CL_ShaderStateChanged(void);

//
// cl_net_chan.c
//
void CL_Netchan_Transmit(netchan_t* chan, QMsg* msg);	//int length, const byte *data );
void CL_Netchan_TransmitNextFragment(netchan_t* chan);
qboolean CL_Netchan_Process(netchan_t* chan, QMsg* msg);
