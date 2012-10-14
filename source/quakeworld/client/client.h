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
// client.h

extern qboolean nomaster;

//=============================================================================


//
// cl_main
//
void CL_Init(void);
void Host_WriteConfiguration(void);

void CL_Disconnect(void);
void CL_Disconnect_f(void);
void CL_NextDemo(void);
qboolean CL_DemoBehind(void);

void CL_BeginServerConnect(void);

//
// cl_input
//
void CL_SendCmd(void);
void CL_SendMove(qwusercmd_t* cmd);

void CL_ReadPackets(void);

int  CL_ReadFromServer(void);
void CL_WriteToServer(qwusercmd_t* cmd);

//
// cl_demo.c
//
void CL_StopPlayback(void);
qboolean CL_GetMessage(void);
void CL_WriteDemoCmd(qwusercmd_t* pcmd);

void CL_Stop_f(void);
void CL_Record_f(void);
void CL_ReRecord_f(void);
void CL_PlayDemo_f(void);
void CL_TimeDemo_f(void);

//
// cl_parse.c
//
void CL_ParseServerMessage(void);
