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
#include "local.h"

//global bsp
bsp_t bspworld;

int AAS_NextBSPEntity(int ent)
{
	ent++;
	if (ent >= 1 && ent < bspworld.numentities)
	{
		return ent;
	}
	return 0;
}

static int AAS_BSPEntityInRange(int ent)
{
	if (ent <= 0 || ent >= bspworld.numentities)
	{
		BotImport_Print(PRT_MESSAGE, "bsp entity out of range\n");
		return false;
	}
	return true;
}

bool AAS_ValueForBSPEpairKey(int ent, const char* key, char* value, int size)
{
	value[0] = '\0';
	if (!AAS_BSPEntityInRange(ent))
	{
		return false;
	}
	for (bsp_epair_t* epair = bspworld.entities[ent].epairs; epair; epair = epair->next)
	{
		if (!String::Cmp(epair->key, key))
		{
			String::NCpyZ(value, epair->value, size);
			return true;
		}
	}
	return false;
}

bool AAS_VectorForBSPEpairKey(int ent, const char* key, vec3_t v)
{
	VectorClear(v);
	char buf[MAX_EPAIRKEY];
	if (!AAS_ValueForBSPEpairKey(ent, key, buf, MAX_EPAIRKEY))
	{
		return false;
	}
	//scanf into doubles, then assign, so it is vec_t size independent
	double v1, v2, v3;
	v1 = v2 = v3 = 0;
	sscanf(buf, "%lf %lf %lf", &v1, &v2, &v3);
	v[0] = v1;
	v[1] = v2;
	v[2] = v3;
	return true;
}

bool AAS_FloatForBSPEpairKey(int ent, const char* key, float* value)
{
	*value = 0;
	char buf[MAX_EPAIRKEY];
	if (!AAS_ValueForBSPEpairKey(ent, key, buf, MAX_EPAIRKEY))
	{
		return false;
	}
	*value = String::Atof(buf);
	return true;
}

bool AAS_IntForBSPEpairKey(int ent, const char* key, int* value)
{
	*value = 0;
	char buf[MAX_EPAIRKEY];
	if (!AAS_ValueForBSPEpairKey(ent, key, buf, MAX_EPAIRKEY))
	{
		return false;
	}
	*value = String::Atoi(buf);
	return true;
}
