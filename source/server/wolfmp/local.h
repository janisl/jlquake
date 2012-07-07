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

#ifndef _WOLFMP_LOCAL_H
#define _WOLFMP_LOCAL_H

#include "WolfMPEntity.h"
#include "WolfMPPlayerState.h"

//
//	Game
//
wmsharedEntity_t* SVWM_GentityNum(int num);
wmplayerState_t* SVWM_GameClientNum(int num);
q3svEntity_t* SVWM_SvEntityForGentity(const wmsharedEntity_t* gEnt);
void SVWM_ClientThink(client_t* cl, wmusercmd_t* cmd);
bool SVWM_BotVisibleFromPos(vec3_t srcpos, int srcnum, vec3_t destpos, int destnum, bool updateVisPos);
bool SVWM_BotCheckAttackAtPos(int entnum, int enemy, vec3_t pos, bool ducking, bool allowHitWorld);
void SVWM_BotFrame(int time);
qintptr SVWM_GameSystemCalls(qintptr* args);
void SVWM_GameClientDisconnect(client_t* drop);
void SVWM_GameInit(int serverTime, int randomSeed, bool restart);
void SVWM_GameShutdown(bool restart);
bool SVWM_GameConsoleCommand();

#endif
