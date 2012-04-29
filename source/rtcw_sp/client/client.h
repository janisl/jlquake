/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// client.h -- primary header for client

#include "../game/q_shared.h"
#include "../../client/client.h"
#include "../qcommon/qcommon.h"
#include "../ui/ui_public.h"
#include "keys.h"
#include "../cgame/cg_public.h"
#include "../game/bg_public.h"

#define RETRANSMIT_TIMEOUT  3000	// time between connection packet retransmits

extern int g_console_field_width;

typedef struct
{
	netadr_t adr;
	int start;
	int time;
	char info[MAX_INFO_STRING_Q3];
} ping_t;

//=============================================================================

extern vm_t* cgvm;				// interface to cgame dll or vm
extern vm_t* uivm;				// interface to ui dll or vm


//
// cvars
//
extern Cvar* cl_nodelta;
extern Cvar* cl_debugMove;
extern Cvar* cl_noprint;
extern Cvar* cl_timegraph;
extern Cvar* cl_maxpackets;
extern Cvar* cl_packetdup;
extern Cvar* cl_shownet;
extern Cvar* cl_showSend;
extern Cvar* cl_timeNudge;
extern Cvar* cl_showTimeDelta;
extern Cvar* cl_freezeDemo;

extern Cvar* cl_timedemo;

extern Cvar* cl_activeAction;

extern Cvar* cl_allowDownload;
extern Cvar* cl_conXOffset;

extern Cvar* cl_missionStats;
extern Cvar* cl_waitForFire;
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
void CL_StartDemoLoop(void);
void CL_NextDemo(void);
void CL_ReadDemoMessage(void);

void CL_InitDownloads(void);
void CL_NextDownload(void);

void CL_GetPing(int n, char* buf, int buflen, int* pingtime);
void CL_GetPingInfo(int n, char* buf, int buflen);
void CL_ClearPing(int n);
int CL_GetPingQueueCount(void);

void CL_ShutdownRef(void);
void CL_InitRef(void);
qboolean CL_CDKeyValidate(const char* key, const char* checksum);
int CL_ServerStatus(char* serverAddress, char* serverStatusString, int maxLen);

void CL_AddToLimboChat(const char* str);					// NERVE - SMF
qboolean CL_GetLimboString(int index, char* buf);			// NERVE - SMF

//
// cl_input
//
void CL_InitInput(void);
void CL_SendCmd(void);
void CL_ClearState(void);
void CL_ReadPackets(void);

void CL_WritePacket(void);
void IN_Notebook(void);
void IN_Help(void);

//----(SA) salute
void IN_Salute(void);
//----(SA)

void CL_VerifyCode(void);

const char* Key_KeynumToString(int keynum, qboolean bTranslate);

//
// cl_parse.c
//
extern int cl_connectedToPureServer;

void CL_SystemInfoChanged(void);
void CL_ParseServerMessage(QMsg* msg);

//====================================================================

void    CL_ServerInfoPacket(netadr_t from, QMsg* msg);
void    CL_LocalServers_f(void);
void    CL_GlobalServers_f(void);
void    CL_FavoriteServers_f(void);
void    CL_Ping_f(void);
qboolean CL_UpdateVisiblePings_f(int source);


//
// console
//
void Con_DrawCharacter(int cx, int line, int num);

void Con_CheckResize(void);
void Con_Init(void);
void Con_Clear_f(void);
void Con_ToggleConsole_f(void);
void Con_DrawNotify(void);
void Con_RunConsole(void);
void Con_DrawConsole(void);
void Con_PageUp(void);
void Con_PageDown(void);
void Con_Top(void);
void Con_Bottom(void);
void Con_Close(void);


//
// cl_scrn.c
//
void    SCR_Init(void);
void    SCR_UpdateScreen(void);

int     SCR_GetBigStringWidth(const char* str);		// returns in virtual 640x480 coordinates

//
// cl_cin.c
//

void CL_PlayCinematic_f(void);
void SCR_DrawCinematic(void);
void SCR_RunCinematic(void);
void SCR_StopCinematic(void);
e_status CIN_StopCinematic(int handle);
void CIN_DrawCinematic(int handle);
void CIN_SetExtents(int handle, int x, int y, int w, int h);
void CIN_CloseAllVideos(void);

//
// cl_cgame.c
//
void CL_InitCGame(void);
void CL_ShutdownCGame(void);
qboolean CL_GameCommand(void);
void CL_CGameRendering(stereoFrame_t stereo);
void CL_SetCGameTime(void);
void CL_FirstSnapshot(void);
void CL_ShaderStateChanged(void);
void CL_UpdateLevelHunkUsage(void);
//
// cl_ui.c
//
void CL_InitUI(void);
void CL_ShutdownUI(void);
int Key_GetCatcher(void);
void Key_SetCatcher(int catcher);
void LAN_LoadCachedServers();
void LAN_SaveServersToCache();


//
// cl_net_chan.c
//
void CL_Netchan_Transmit(netchan_t* chan, QMsg* msg);	//int length, const byte *data );
void CL_Netchan_TransmitNextFragment(netchan_t* chan);
qboolean CL_Netchan_Process(netchan_t* chan, QMsg* msg);
