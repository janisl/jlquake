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

//
//	Game
//
int SVWM_NumForGentity(const wmsharedEntity_t* ent);
wmsharedEntity_t* SVWM_GentityNum(int num);
wmplayerState_t* SVWM_GameClientNum(int num);
q3svEntity_t* SVWM_SvEntityForGentity(const wmsharedEntity_t* gEnt);
wmsharedEntity_t* SVWM_GEntityForSvEntity(const q3svEntity_t* svEnt);
idEntity3* SVWM_EntityForGentity(const wmsharedEntity_t* gEnt);

bool SVWM_BotVisibleFromPos(vec3_t srcpos, int srcnum, vec3_t destpos, int destnum, bool updateVisPos);
bool SVWM_BotCheckAttackAtPos(int entnum, int enemy, vec3_t pos, bool ducking, bool allowHitWorld);
qintptr SVWM_GameSystemCalls(qintptr* args);

//
//	World
//
// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself
void SVWM_UnlinkEntity(wmsharedEntity_t* ent);
// Needs to be called any time an entity changes origin, mins, maxs,
// or solid.  Automatically unlinks if needed.
// sets ent->v.absmin and ent->v.absmax
// sets ent->leafnums[] for pvs determination even if the entity
// is not solid
void SVWM_LinkEntity(wmsharedEntity_t* ent);
clipHandle_t SVWM_ClipHandleForEntity(const wmsharedEntity_t* ent);

#endif
