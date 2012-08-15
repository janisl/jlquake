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

#ifndef _QUAKE3_LOCAL_H
#define _QUAKE3_LOCAL_H

#include "Quake3Entity.h"
#include "Quake3PlayerState.h"

//
//	Game
//
q3sharedEntity_t* SVQ3_GentityNum(int num);
q3playerState_t* SVQ3_GameClientNum(int num);
q3svEntity_t* SVQ3_SvEntityForGentity(const q3sharedEntity_t* gEnt);
qintptr SVQ3_GameSystemCalls(qintptr* args);
void SVQ3_ClientThink(client_t* cl, q3usercmd_t* cmd);
void SVQ3_BotFrame(int time);
void SVQ3_GameClientDisconnect(client_t* drop);
void SVQ3_GameInit(int serverTime, int randomSeed, bool restart);
void SVQ3_GameShutdown(bool restart);
bool SVQ3_GameConsoleCommand();
void SVQ3_GameClientBegin(int clientNum);
void SVQ3_GameClientUserInfoChanged(int clientNum);
const char* SVQ3_GameClientConnect(int clientNum, bool firstTime, bool isBot);
void SVQ3_GameClientCommand(int clientNum);
void SVQ3_GameRunFrame(int time);

#endif
