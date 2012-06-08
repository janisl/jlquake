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
// clip to a specific entity
void SVWM_ClipToEntity(q3trace_t* trace, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int entityNum, int contentmask, int capsule);
void SVWM_ClipMoveToEntities(q3moveclip_t* clip);

#endif
