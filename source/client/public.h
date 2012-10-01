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

void Key_WriteBindings(fileHandle_t f);
void CL_InitKeyCommands();
// the keyboard binding interface must be setup before execing
// config files, but the rest of client startup will happen later

bool CLT3_GameCommand();
bool CL_GetTag(int clientNum, const char* tagname, orientation_t* _or);

bool UIT3_GameCommand();
void CLT3_ReadCDKey(const char* gameName);
void CLT3_AppendCDKey(const char* gameName);
void CLT3_WriteCDKey();

void Con_ConsolePrint(const char* txt);

#endif
