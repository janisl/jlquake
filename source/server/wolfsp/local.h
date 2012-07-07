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

#ifndef _WOLFSP_LOCAL_H
#define _WOLFSP_LOCAL_H

#include "WolfSPEntity.h"
#include "WolfSPPlayerState.h"

//
//	Game
//
wssharedEntity_t* SVWS_GentityNum(int num);
wsplayerState_t* SVWS_GameClientNum(int num);
q3svEntity_t* SVWS_SvEntityForGentity(const wssharedEntity_t* gEnt);
void SVWS_ClientThink(client_t* cl, wsusercmd_t* cmd);
bool SVWS_BotVisibleFromPos(vec3_t srcpos, int srcnum, vec3_t destpos, int destnum, bool updateVisPos);
bool SVWS_BotCheckAttackAtPos(int entnum, int enemy, vec3_t pos, bool ducking, bool allowHitWorld);
void SVWS_BotFrame(int time);
qintptr SVWS_GameSystemCalls(qintptr* args);
void SVWS_GameClientDisconnect(client_t* drop);
void SVWS_GameInit(int serverTime, int randomSeed, bool restart);
void SVWS_GameShutdown(bool restart);
bool SVWS_GameConsoleCommand();

#endif
