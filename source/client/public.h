//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#ifndef _CLIENT_PUBLIC_H
#define _CLIENT_PUBLIC_H

void S_ClearSoundBuffer(bool killStreaming);
int CLH2_GetLightStyleValue(int style);
void CL_ClearDrift();
int CL_GetKeyCatchers();
void CLQH_StopDemoLoop();
void CL_ClearKeyCatchers();
void CLQH_GetSpawnParams();
bool CL_IsDemoPlaying();
int CLQH_GetIntermission();
void SCR_DebugGraph(float value, int color);
void CL_CvarChanged(Cvar* var);
const char* CL_TranslateStringBuf(const char* string);
void CL_ForwardKnownCommandToServer();
void CL_ForwardCommandToServer();
void SCRQH_BeginLoadingPlaque();
void SCRQ2_BeginLoadingPlaque(bool Clear);
void SCR_EndLoadingPlaque();
void CL_Disconnect(bool showMainMenu);
void CL_Init();
void CL_Shutdown();
void CL_FrameCommon();

void Key_WriteBindings(fileHandle_t f);
// the keyboard binding interface must be setup before execing
// config files, but the rest of client startup will happen later
void CL_InitKeyCommands();
void CL_KeyEvent(int key, bool down, unsigned time);
// char events are for field typing, not game control
void CL_CharEvent(int key);
void CL_MouseEvent(int dx, int dy);
void CL_JoystickEvent(int axis, int value);
void Sys_SendKeyEvents();
void IN_Frame();

void CLQH_EstablishConnection(const char* name);

void CLQ2_Drop();

// start all the client stuff using the hunk
void CL_StartHunkUsers();
// shutdown all the client stuff
void CLT3_ShutdownAll();
// dump all memory on an error
void CLT3_FlushMemory();
// do a screen update before starting to load a map
// when the server is going to load a new map, the entire hunk
// will be cleared, so the client must shutdown cgame, ui, and
// the renderer
void CLT3_MapLoading();
void CLT3_PacketEvent(netadr_t from, QMsg* msg);

bool CLT3_GameCommand();
bool CL_GetTag(int clientNum, const char* tagname, orientation_t* _or);

bool UIT3_GameCommand();
void CLT3_ReadCDKey(const char* gameName);
void CLT3_AppendCDKey(const char* gameName);
void CLT3_WriteCDKey();

void Con_ConsolePrint(const char* txt);

#endif
