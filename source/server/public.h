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

#ifndef _SERVER_PUBLIC_H
#define _SERVER_PUBLIC_H

extern int svh2_kingofhill;
extern Cvar* svqh_coop;
extern Cvar* svqh_teamplay;
extern Cvar* qh_skill;
extern Cvar* qh_timelimit;
extern Cvar* qh_fraglimit;
extern Cvar* h2_randomclass;
extern Cvar* allow_download;
extern Cvar* allow_download_players;
extern Cvar* allow_download_models;
extern Cvar* allow_download_sounds;
extern Cvar* allow_download_maps;

bool SV_IsServerActive();
void SV_CvarChanged(Cvar* var);
int SVQH_GetMaxClients();
int SVQH_GetMaxClientsLimit();
void SV_Init();
void SV_Shutdown(const char* finalMessage);
void SV_Frame(int msec);
int SVQH_GetNumConnectedClients();
void SVQH_SetRealTime(int time);

void SVQH_ShutdownNetwork();
void SVH2_RemoveGIPFiles(const char* path);

void PR_SetPlayerClassGlobal(float newClass);

bool SVT3_GameCommand();
void SVT3_PacketEvent(netadr_t from, QMsg* msg);

// RF, this is only used when running a local server
void SVWS_SendMoveSpeedsToGame(int entnum, char* text);
int SVWS_GetModelInfo(int clientNum, char* modelName, animModelInfo_t** modelInfo);
void SVWM_SendMoveSpeedsToGame(int entnum, char* text);

bool SVET_GameIsSinglePlayer();

#endif
