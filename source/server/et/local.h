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

#ifndef _ET_LOCAL_H
#define _ET_LOCAL_H

#include "ETEntity.h"
#include "ETPlayerState.h"

//
//	Game
//
etsharedEntity_t* SVET_GentityNum(int num);
etplayerState_t* SVET_GameClientNum(int num);
q3svEntity_t* SVET_SvEntityForGentity(const etsharedEntity_t* gEnt);
bool SVET_GameIsSinglePlayer();
bool SVET_GameIsCoop();
void SVET_ClientThink(client_t* cl, etusercmd_t* cmd);
bool SVET_BotVisibleFromPos(vec3_t srcorigin, int srcnum, vec3_t destorigin, int destent, bool dummy);
bool SVET_BotCheckAttackAtPos(int entnum, int enemy, vec3_t pos, bool ducking, bool allowHitWorld);
void SVET_BotFrame(int time);
qintptr SVET_GameSystemCalls(qintptr* args);
void SVET_GameClientDisconnect(client_t* drop);
void SVET_GameInit(int serverTime, int randomSeed, bool restart);
void SVET_GameShutdown(bool restart);
bool SVET_GameConsoleCommand();
void SVET_GameBinaryMessageReceived(int cno, const char* buf, int buflen, int commandTime);
bool SVET_GameSnapshotCallback(int entityNumber, int clientNumber);

#endif
