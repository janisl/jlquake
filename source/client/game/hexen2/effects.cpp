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
static bool EntityUsed[MAX_EFFECT_ENTITIES_H2];
static int EffectEntityCount;

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

static void CLH2_FreeEffectEntity(int index)
{
	if (index == -1)
	{
		return;
	}
	EntityUsed[index] = false;
	EffectEntityCount--;
}

static void CLH2_FreeEffectSmoke(int index)
{
	CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Smoke.entity_index);
}

static void CLH2_FreeEffectTeleSmoke1(int index)
{
	CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Smoke.entity_index2);
	CLH2_FreeEffectSmoke(index);
}

static void CLH2_FreeEffectFlash(int index)
{
	CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Flash.entity_index);
}

static void CLH2_FreeEffectTeleporterPuffs(int index)
{
	for (int i = 0; i < 8; i++)
	{
		CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Teleporter.entity_index[i]);
	}
}

static void CLH2_FreeEffectTeleporterBody(int index)
{
	CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Teleporter.entity_index[0]);
}

static void CLH2_FreeEffectMissile(int index)
{
	CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Missile.entity_index);
}

static void CLH2_FreeEffectChunk(int index)
{
	for (int i = 0; i < cl_common->h2_Effects[index].Chunk.numChunks; i++)
	{
		if (cl_common->h2_Effects[index].Chunk.entity_index[i] != -1)
		{
			CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Chunk.entity_index[i]);
		}
	}
}

static void CLH2_FreeEffectSheepinator(int index)
{
	for (int i = 0; i < 5; i++)
	{
		CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Xbow.ent[i]);
	}
}

static void CLH2_FreeEffectXbow(int index)
{
	for (int i = 0; i < cl_common->h2_Effects[index].Xbow.bolts; i++)
	{
		CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Xbow.ent[i]);
	}
}

static void CLH2_FreeEffectChain(int index)
{
	CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Chain.ent1);
}

static void CLH2_FreeEffectEidolonStar(int index)
{
	CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Star.ent1);
	CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Star.entity_index);
}

static void CLH2_FreeEffectMissileStar(int index)
{
	CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Star.ent2);
	CLH2_FreeEffectEidolonStar(index);
}

static void CLH2_FreeEffectEntityH2(int index)
{
	switch (cl_common->h2_Effects[index].type)
	{
	case CEH2_RAIN:
	case CEH2_SNOW:
	case CEH2_FOUNTAIN:
	case CEH2_QUAKE:
	case CEH2_RIDER_DEATH:
	case CEH2_GRAVITYWELL:
		break;

	case CEH2_WHITE_SMOKE:
	case CEH2_GREEN_SMOKE:
	case CEH2_GREY_SMOKE:
	case CEH2_RED_SMOKE:
	case CEH2_SLOW_WHITE_SMOKE:
	case CEH2_TELESMK1:
	case CEH2_TELESMK2:
	case CEH2_GHOST:
	case CEH2_REDCLOUD:
	case CEH2_ACID_MUZZFL:
	case CEH2_FLAMESTREAM:
	case CEH2_FLAMEWALL:
	case CEH2_FLAMEWALL2:
	case CEH2_ONFIRE:
	// Just go through animation and then remove
	case CEH2_SM_WHITE_FLASH:
	case CEH2_YELLOWRED_FLASH:
	case CEH2_BLUESPARK:
	case CEH2_YELLOWSPARK:
	case CEH2_SM_CIRCLE_EXP:
	case CEH2_BG_CIRCLE_EXP:
	case CEH2_SM_EXPLOSION:
	case CEH2_LG_EXPLOSION:
	case CEH2_FLOOR_EXPLOSION:
	case CEH2_FLOOR_EXPLOSION3:
	case CEH2_BLUE_EXPLOSION:
	case CEH2_REDSPARK:
	case CEH2_GREENSPARK:
	case CEH2_ICEHIT:
	case CEH2_MEDUSA_HIT:
	case CEH2_MEZZO_REFLECT:
	case CEH2_FLOOR_EXPLOSION2:
	case CEH2_XBOW_EXPLOSION:
	case CEH2_NEW_EXPLOSION:
	case CEH2_MAGIC_MISSILE_EXPLOSION:
	case CEH2_BONE_EXPLOSION:
	case CEH2_BLDRN_EXPL:
	case CEH2_BRN_BOUNCE:
	case CEH2_LSHOCK:
	case CEH2_ACID_HIT:
	case CEH2_ACID_SPLAT:
	case CEH2_ACID_EXPL:
	case CEH2_LBALL_EXPL:
	case CEH2_FBOOM:
	case CEH2_BOMB:
	case CEH2_FIREWALL_SMALL:
	case CEH2_FIREWALL_MEDIUM:
	case CEH2_FIREWALL_LARGE:
		CLH2_FreeEffectSmoke(index);
		break;

	// Go forward then backward through animation then remove
	case CEH2_WHITE_FLASH:
	case CEH2_BLUE_FLASH:
	case CEH2_SM_BLUE_FLASH:
	case CEH2_RED_FLASH:
		CLH2_FreeEffectFlash(index);
		break;

	case CEH2_TELEPORTERPUFFS:
		CLH2_FreeEffectTeleporterPuffs(index);
		break;

	case CEH2_TELEPORTERBODY:
		CLH2_FreeEffectTeleporterBody(index);
		break;

	case CEH2_BONESHARD:
	case CEH2_BONESHRAPNEL:
		CLH2_FreeEffectMissile(index);
		break;

	case CEH2_CHUNK:
		CLH2_FreeEffectChunk(index);
		break;
	}
}

static void CLH2_FreeEffectEntityHW(int index)
{
	switch (cl_common->h2_Effects[index].type)
	{
	case CEHW_RAIN:
	case CEHW_FOUNTAIN:
	case CEHW_QUAKE:
	case CEHW_DEATHBUBBLES:
	case CEHW_RIDER_DEATH:
		break;

	case CEHW_TELESMK1:
		CLH2_FreeEffectTeleSmoke1(index);
		break;

	case CEHW_WHITE_SMOKE:
	case CEHW_GREEN_SMOKE:
	case CEHW_GREY_SMOKE:
	case CEHW_RED_SMOKE:
	case CEHW_SLOW_WHITE_SMOKE:
	case CEHW_TELESMK2:
	case CEHW_GHOST:
	case CEHW_REDCLOUD:
	case CEHW_ACID_MUZZFL:
	case CEHW_FLAMESTREAM:
	case CEHW_FLAMEWALL:
	case CEHW_FLAMEWALL2:
	case CEHW_ONFIRE:
	case CEHW_RIPPLE:
	// Just go through animation and then remove
	case CEHW_SM_WHITE_FLASH:
	case CEHW_YELLOWRED_FLASH:
	case CEHW_BLUESPARK:
	case CEHW_YELLOWSPARK:
	case CEHW_SM_CIRCLE_EXP:
	case CEHW_BG_CIRCLE_EXP:
	case CEHW_SM_EXPLOSION:
	case CEHW_SM_EXPLOSION2:
	case CEHW_BG_EXPLOSION:
	case CEHW_FLOOR_EXPLOSION:
	case CEHW_BLUE_EXPLOSION:
	case CEHW_REDSPARK:
	case CEHW_GREENSPARK:
	case CEHW_ICEHIT:
	case CEHW_MEDUSA_HIT:
	case CEHW_MEZZO_REFLECT:
	case CEHW_FLOOR_EXPLOSION2:
	case CEHW_XBOW_EXPLOSION:
	case CEHW_NEW_EXPLOSION:
	case CEHW_MAGIC_MISSILE_EXPLOSION:
	case CEHW_BONE_EXPLOSION:
	case CEHW_BLDRN_EXPL:
	case CEHW_BRN_BOUNCE:
	case CEHW_LSHOCK:
	case CEHW_ACID_HIT:
	case CEHW_ACID_SPLAT:
	case CEHW_ACID_EXPL:
	case CEHW_LBALL_EXPL:
	case CEHW_FBOOM:
	case CEHW_BOMB:
	case CEHW_FIREWALL_SMALL:
	case CEHW_FIREWALL_MEDIUM:
	case CEHW_FIREWALL_LARGE:
		CLH2_FreeEffectSmoke(index);
		break;

	// Go forward then backward through animation then remove
	case CEHW_WHITE_FLASH:
	case CEHW_BLUE_FLASH:
	case CEHW_SM_BLUE_FLASH:
	case CEHW_HWSPLITFLASH:
	case CEHW_RED_FLASH:
		CLH2_FreeEffectFlash(index);
		break;

	case CEHW_TELEPORTERPUFFS:
		CLH2_FreeEffectTeleporterPuffs(index);
		break;

	case CEHW_TELEPORTERBODY:
		CLH2_FreeEffectTeleporterBody(index);
		break;

	case CEHW_HWSHEEPINATOR:
		CLH2_FreeEffectSheepinator(index);
		break;

	case CEHW_HWXBOWSHOOT:
		CLH2_FreeEffectXbow(index);
		break;

	case CEHW_HWDRILLA:
	case CEHW_BONESHARD:
	case CEHW_BONESHRAPNEL:
	case CEHW_HWBONEBALL:
	case CEHW_HWRAVENSTAFF:
	case CEHW_HWRAVENPOWER:
		CLH2_FreeEffectMissile(index);
		break;

	case CEHW_TRIPMINESTILL:
	case CEHW_SCARABCHAIN:
	case CEHW_TRIPMINE:
		CLH2_FreeEffectChain(index);
		break;

	case CEHW_HWMISSILESTAR:
		CLH2_FreeEffectMissileStar(index);
		break;

	case CEHW_HWEIDOLONSTAR:
		CLH2_FreeEffectEidolonStar(index);
		break;

	default:
		break;
	}
}

void CLH2_FreeEffect(int index)
{
	if (GGameType & GAME_HexenWorld)
	{
		CLH2_FreeEffectEntityHW(index);
	}
	else
	{
		CLH2_FreeEffectEntityH2(index);
	}

	Com_Memset(&cl_common->h2_Effects[index], 0, sizeof(h2EffectT));
}
