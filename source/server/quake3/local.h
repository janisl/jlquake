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

//
//	Game
//
int SVQ3_NumForGentity(const q3sharedEntity_t* ent);
q3sharedEntity_t* SVQ3_GentityNum(int num);
q3playerState_t* SVQ3_GameClientNum(int num);
q3svEntity_t* SVQ3_SvEntityForGentity(const q3sharedEntity_t* gEnt);
q3sharedEntity_t* SVQ3_GEntityForSvEntity(const q3svEntity_t* svEnt);

//
//	World
//
// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself
void SVQ3_UnlinkEntity(q3sharedEntity_t* ent);
// Needs to be called any time an entity changes origin, mins, maxs,
// or solid.  Automatically unlinks if needed.
// sets ent->v.absmin and ent->v.absmax
// sets ent->leafnums[] for pvs determination even if the entity
// is not solid
void SVQ3_LinkEntity(q3sharedEntity_t* ent);
clipHandle_t SVQ3_ClipHandleForEntity(const q3sharedEntity_t* ent);
// clip to a specific entity
void SVQ3_ClipToEntity(q3trace_t* trace, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int entityNum, int contentmask, int capsule);

#endif
