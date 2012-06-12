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

#include "../server.h"
#include "../tech3/local.h"
#include "local.h"
#include "g_public.h"

// these functions must be used instead of pointer arithmetic, because
// the game allocates gentities with private information after the server shared part
int SVET_NumForGentity(const etsharedEntity_t* ent)
{
	return ((byte*)ent - (byte*)sv.et_gentities) / sv.q3_gentitySize;
}

etsharedEntity_t* SVET_GentityNum(int num)
{
	return (etsharedEntity_t*)((byte*)sv.et_gentities + sv.q3_gentitySize * num);
}

etplayerState_t* SVET_GameClientNum(int num)
{
	return (etplayerState_t*)((byte*)sv.et_gameClients + sv.q3_gameClientSize * num);
}

q3svEntity_t* SVET_SvEntityForGentity(const etsharedEntity_t* gEnt)
{
	if (!gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES_Q3)
	{
		common->Error("SVET_SvEntityForGentity: bad gEnt");
	}
	return &sv.q3_svEntities[gEnt->s.number];
}

idEntity3* SVET_EntityForGentity(const etsharedEntity_t* gEnt)
{
	if (!gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES_Q3)
	{
		common->Error("SVET_SvEntityForGentity: bad gEnt");
	}
	return sv.q3_entities[gEnt->s.number];
}

etsharedEntity_t* SVET_GEntityForSvEntity(const q3svEntity_t* svEnt)
{
	int num = svEnt - sv.q3_svEntities;
	return SVET_GentityNum(num);
}

void SVET_UnlinkEntity(etsharedEntity_t* gEnt)
{
	SVT3_UnlinkEntity(SVET_EntityForGentity(gEnt), SVET_SvEntityForGentity(gEnt));
}

void SVET_LinkEntity(etsharedEntity_t* gEnt)
{
	SVT3_LinkEntity(SVET_EntityForGentity(gEnt), SVET_SvEntityForGentity(gEnt));
}

//	Returns a headnode that can be used for testing or clipping to a
// given entity.  If the entity is a bsp model, the headnode will
// be returned, otherwise a custom box tree will be constructed.
clipHandle_t SVET_ClipHandleForEntity(const etsharedEntity_t* gent)
{
	return SVT3_ClipHandleForEntity(SVET_EntityForGentity(gent));
}

bool SVET_BotVisibleFromPos(vec3_t srcorigin, int srcnum, vec3_t destorigin, int destent, bool dummy)
{
	return VM_Call(gvm, ETBOT_VISIBLEFROMPOS, srcorigin, srcnum, destorigin, destent, dummy);
}

bool SVET_BotCheckAttackAtPos(int entnum, int enemy, vec3_t pos, bool ducking, bool allowHitWorld)
{
	return VM_Call(gvm, ETBOT_CHECKATTACKATPOS, entnum, enemy, pos, ducking, allowHitWorld);
}
