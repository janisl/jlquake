//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../../client.h"
#include "local.h"

effect_entity_t EffectEntities[MAX_EFFECT_ENTITIES_H2];
bool EntityUsed[MAX_EFFECT_ENTITIES_H2];
int EffectEntityCount;

void CLH2_ClearEffects()
{
	Com_Memset(cl_common->h2_Effects, 0, sizeof(cl_common->h2_Effects));
	Com_Memset(EntityUsed, 0, sizeof(EntityUsed));
	EffectEntityCount = 0;
}

int CLH2_NewEffectEntity()
{
	if (EffectEntityCount == MAX_EFFECT_ENTITIES_H2)
	{
		return -1;
	}

	int counter;
	for (counter = 0; counter < MAX_EFFECT_ENTITIES_H2; counter++)
	{
		if (!EntityUsed[counter]) 
		{
			break;
		}
	}

	EntityUsed[counter] = true;
	EffectEntityCount++;
	effect_entity_t* ent = &EffectEntities[counter];
	Com_Memset(ent, 0, sizeof(*ent));

	return counter;
}

void CLH2_FreeEffectEntity(int index)
{
	if (index == -1)
	{
		return;
	}
	EntityUsed[index] = false;
	EffectEntityCount--;
}
