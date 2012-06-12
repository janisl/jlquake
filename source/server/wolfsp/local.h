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

//
//	Game
//
int SVWS_NumForGentity(const wssharedEntity_t* ent);
wssharedEntity_t* SVWS_GentityNum(int num);
wsplayerState_t* SVWS_GameClientNum(int num);
q3svEntity_t* SVWS_SvEntityForGentity(const wssharedEntity_t* gEnt);
wssharedEntity_t* SVWS_GEntityForSvEntity(const q3svEntity_t* svEnt);
idEntity3* SVWS_EntityForGentity(const wssharedEntity_t* gEnt);

bool SVWS_BotVisibleFromPos(vec3_t srcpos, int srcnum, vec3_t destpos, int destnum, bool updateVisPos);
bool SVWS_BotCheckAttackAtPos(int entnum, int enemy, vec3_t pos, bool ducking, bool allowHitWorld);

//
//	World
//
// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself
void SVWS_UnlinkEntity(wssharedEntity_t* ent);
// Needs to be called any time an entity changes origin, mins, maxs,
// or solid.  Automatically unlinks if needed.
// sets ent->v.absmin and ent->v.absmax
// sets ent->leafnums[] for pvs determination even if the entity
// is not solid
void SVWS_LinkEntity(wssharedEntity_t* ent);
clipHandle_t SVWS_ClipHandleForEntity(const wssharedEntity_t* ent);

#endif
