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

static sfxHandle_t clh2_fxsfx_bone;
static sfxHandle_t clh2_fxsfx_bonefpow;
static sfxHandle_t clh2_fxsfx_xbowshoot;
static sfxHandle_t clh2_fxsfx_xbowfshoot;
static sfxHandle_t clh2_fxsfx_explode;
static sfxHandle_t clh2_fxsfx_mmfire;
static sfxHandle_t clh2_fxsfx_eidolon;
static sfxHandle_t clh2_fxsfx_scarabwhoosh;
static sfxHandle_t clh2_fxsfx_scarabgrab;
static sfxHandle_t clh2_fxsfx_scarabhome;
static sfxHandle_t clh2_fxsfx_scarabbyebye;
static sfxHandle_t clh2_fxsfx_ravensplit;
static sfxHandle_t clh2_fxsfx_ravenfire;
static sfxHandle_t clh2_fxsfx_ravengo;
static sfxHandle_t clh2_fxsfx_drillashoot;
static sfxHandle_t clh2_fxsfx_drillaspin;
static sfxHandle_t clh2_fxsfx_drillameat;

static sfxHandle_t clh2_fxsfx_arr2flsh;
static sfxHandle_t clh2_fxsfx_arr2wood;
static sfxHandle_t clh2_fxsfx_met2stn;

static sfxHandle_t clh2_fxsfx_ripple;
static sfxHandle_t clh2_fxsfx_splash;

static unsigned int randomseed;

static void setseed(unsigned int seed)
{
	randomseed = seed;
}

//unsigned int seedrand(int max)
static float seedrand()
{
	randomseed = (randomseed * 877 + 573) % 9968;
	return (float)randomseed / 9968;
}

void CLHW_InitEffects()
{
	clh2_fxsfx_bone = S_RegisterSound("necro/bonefnrm.wav");
	clh2_fxsfx_bonefpow = S_RegisterSound("necro/bonefpow.wav");
	clh2_fxsfx_xbowshoot = S_RegisterSound("assassin/firebolt.wav");
	clh2_fxsfx_xbowfshoot = S_RegisterSound("assassin/firefblt.wav");
	clh2_fxsfx_explode = S_RegisterSound("weapons/explode.wav");
	clh2_fxsfx_mmfire = S_RegisterSound("necro/mmfire.wav");
	clh2_fxsfx_eidolon = S_RegisterSound("eidolon/spell.wav");
	clh2_fxsfx_scarabwhoosh = S_RegisterSound("misc/whoosh.wav");
	clh2_fxsfx_scarabgrab = S_RegisterSound("assassin/chn2flsh.wav");
	clh2_fxsfx_scarabhome = S_RegisterSound("assassin/chain.wav");
	clh2_fxsfx_scarabbyebye = S_RegisterSound("items/itmspawn.wav");
	clh2_fxsfx_ravensplit = S_RegisterSound("raven/split.wav");
	clh2_fxsfx_ravenfire = S_RegisterSound("raven/rfire1.wav");
	clh2_fxsfx_ravengo = S_RegisterSound("raven/ravengo.wav");
	clh2_fxsfx_drillashoot = S_RegisterSound("assassin/pincer.wav");
	clh2_fxsfx_drillaspin = S_RegisterSound("assassin/spin.wav");
	clh2_fxsfx_drillameat = S_RegisterSound("assassin/core.wav");

	clh2_fxsfx_arr2flsh = S_RegisterSound("assassin/arr2flsh.wav");
	clh2_fxsfx_arr2wood = S_RegisterSound("assassin/arr2wood.wav");
	clh2_fxsfx_met2stn = S_RegisterSound("weapons/met2stn.wav");

	clh2_fxsfx_ripple = S_RegisterSound("misc/drip.wav");
	clh2_fxsfx_splash = S_RegisterSound("raven/outwater.wav");
}

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
	case H2CE_RAIN:
	case H2CE_SNOW:
	case H2CE_FOUNTAIN:
	case H2CE_QUAKE:
	case H2CE_RIDER_DEATH:
	case H2CE_GRAVITYWELL:
		break;

	case H2CE_WHITE_SMOKE:
	case H2CE_GREEN_SMOKE:
	case H2CE_GREY_SMOKE:
	case H2CE_RED_SMOKE:
	case H2CE_SLOW_WHITE_SMOKE:
	case H2CE_TELESMK1:
	case H2CE_TELESMK2:
	case H2CE_GHOST:
	case H2CE_REDCLOUD:
	case H2CE_ACID_MUZZFL:
	case H2CE_FLAMESTREAM:
	case H2CE_FLAMEWALL:
	case H2CE_FLAMEWALL2:
	case H2CE_ONFIRE:
	// Just go through animation and then remove
	case H2CE_SM_WHITE_FLASH:
	case H2CE_YELLOWRED_FLASH:
	case H2CE_BLUESPARK:
	case H2CE_YELLOWSPARK:
	case H2CE_SM_CIRCLE_EXP:
	case H2CE_BG_CIRCLE_EXP:
	case H2CE_SM_EXPLOSION:
	case H2CE_LG_EXPLOSION:
	case H2CE_FLOOR_EXPLOSION:
	case H2CE_FLOOR_EXPLOSION3:
	case H2CE_BLUE_EXPLOSION:
	case H2CE_REDSPARK:
	case H2CE_GREENSPARK:
	case H2CE_ICEHIT:
	case H2CE_MEDUSA_HIT:
	case H2CE_MEZZO_REFLECT:
	case H2CE_FLOOR_EXPLOSION2:
	case H2CE_XBOW_EXPLOSION:
	case H2CE_NEW_EXPLOSION:
	case H2CE_MAGIC_MISSILE_EXPLOSION:
	case H2CE_BONE_EXPLOSION:
	case H2CE_BLDRN_EXPL:
	case H2CE_BRN_BOUNCE:
	case H2CE_LSHOCK:
	case H2CE_ACID_HIT:
	case H2CE_ACID_SPLAT:
	case H2CE_ACID_EXPL:
	case H2CE_LBALL_EXPL:
	case H2CE_FBOOM:
	case H2CE_BOMB:
	case H2CE_FIREWALL_SMALL:
	case H2CE_FIREWALL_MEDIUM:
	case H2CE_FIREWALL_LARGE:
		CLH2_FreeEffectSmoke(index);
		break;

	// Go forward then backward through animation then remove
	case H2CE_WHITE_FLASH:
	case H2CE_BLUE_FLASH:
	case H2CE_SM_BLUE_FLASH:
	case H2CE_RED_FLASH:
		CLH2_FreeEffectFlash(index);
		break;

	case H2CE_TELEPORTERPUFFS:
		CLH2_FreeEffectTeleporterPuffs(index);
		break;

	case H2CE_TELEPORTERBODY:
		CLH2_FreeEffectTeleporterBody(index);
		break;

	case H2CE_BONESHARD:
	case H2CE_BONESHRAPNEL:
		CLH2_FreeEffectMissile(index);
		break;

	case H2CE_CHUNK:
		CLH2_FreeEffectChunk(index);
		break;
	}
}

static void CLH2_FreeEffectEntityHW(int index)
{
	switch (cl_common->h2_Effects[index].type)
	{
	case HWCE_RAIN:
	case HWCE_FOUNTAIN:
	case HWCE_QUAKE:
	case HWCE_DEATHBUBBLES:
	case HWCE_RIDER_DEATH:
		break;

	case HWCE_TELESMK1:
		CLH2_FreeEffectTeleSmoke1(index);
		break;

	case HWCE_WHITE_SMOKE:
	case HWCE_GREEN_SMOKE:
	case HWCE_GREY_SMOKE:
	case HWCE_RED_SMOKE:
	case HWCE_SLOW_WHITE_SMOKE:
	case HWCE_TELESMK2:
	case HWCE_GHOST:
	case HWCE_REDCLOUD:
	case HWCE_ACID_MUZZFL:
	case HWCE_FLAMESTREAM:
	case HWCE_FLAMEWALL:
	case HWCE_FLAMEWALL2:
	case HWCE_ONFIRE:
	case HWCE_RIPPLE:
	// Just go through animation and then remove
	case HWCE_SM_WHITE_FLASH:
	case HWCE_YELLOWRED_FLASH:
	case HWCE_BLUESPARK:
	case HWCE_YELLOWSPARK:
	case HWCE_SM_CIRCLE_EXP:
	case HWCE_BG_CIRCLE_EXP:
	case HWCE_SM_EXPLOSION:
	case HWCE_SM_EXPLOSION2:
	case HWCE_BG_EXPLOSION:
	case HWCE_FLOOR_EXPLOSION:
	case HWCE_BLUE_EXPLOSION:
	case HWCE_REDSPARK:
	case HWCE_GREENSPARK:
	case HWCE_ICEHIT:
	case HWCE_MEDUSA_HIT:
	case HWCE_MEZZO_REFLECT:
	case HWCE_FLOOR_EXPLOSION2:
	case HWCE_XBOW_EXPLOSION:
	case HWCE_NEW_EXPLOSION:
	case HWCE_MAGIC_MISSILE_EXPLOSION:
	case HWCE_BONE_EXPLOSION:
	case HWCE_BLDRN_EXPL:
	case HWCE_BRN_BOUNCE:
	case HWCE_LSHOCK:
	case HWCE_ACID_HIT:
	case HWCE_ACID_SPLAT:
	case HWCE_ACID_EXPL:
	case HWCE_LBALL_EXPL:
	case HWCE_FBOOM:
	case HWCE_BOMB:
	case HWCE_FIREWALL_SMALL:
	case HWCE_FIREWALL_MEDIUM:
	case HWCE_FIREWALL_LARGE:
		CLH2_FreeEffectSmoke(index);
		break;

	// Go forward then backward through animation then remove
	case HWCE_WHITE_FLASH:
	case HWCE_BLUE_FLASH:
	case HWCE_SM_BLUE_FLASH:
	case HWCE_HWSPLITFLASH:
	case HWCE_RED_FLASH:
		CLH2_FreeEffectFlash(index);
		break;

	case HWCE_TELEPORTERPUFFS:
		CLH2_FreeEffectTeleporterPuffs(index);
		break;

	case HWCE_TELEPORTERBODY:
		CLH2_FreeEffectTeleporterBody(index);
		break;

	case HWCE_HWSHEEPINATOR:
		CLH2_FreeEffectSheepinator(index);
		break;

	case HWCE_HWXBOWSHOOT:
		CLH2_FreeEffectXbow(index);
		break;

	case HWCE_HWDRILLA:
	case HWCE_BONESHARD:
	case HWCE_BONESHRAPNEL:
	case HWCE_HWBONEBALL:
	case HWCE_HWRAVENSTAFF:
	case HWCE_HWRAVENPOWER:
		CLH2_FreeEffectMissile(index);
		break;

	case HWCE_TRIPMINESTILL:
	case HWCE_SCARABCHAIN:
	case HWCE_TRIPMINE:
		CLH2_FreeEffectChain(index);
		break;

	case HWCE_HWMISSILESTAR:
		CLH2_FreeEffectMissileStar(index);
		break;

	case HWCE_HWEIDOLONSTAR:
		CLH2_FreeEffectEidolonStar(index);
		break;

	default:
		break;
	}
}

static void CLH2_FreeEffect(int index)
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

static void CLH2_ParseEffectRain(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].Rain.min_org);
	message.ReadPos(cl_common->h2_Effects[index].Rain.max_org);
	message.ReadPos(cl_common->h2_Effects[index].Rain.e_size);
	message.ReadPos(cl_common->h2_Effects[index].Rain.dir);
	cl_common->h2_Effects[index].Rain.color = message.ReadShort();
	cl_common->h2_Effects[index].Rain.count = message.ReadShort();
	cl_common->h2_Effects[index].Rain.wait = message.ReadFloat();
}

static void CLH2_ParseEffectSnow(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].Snow.min_org);
	message.ReadPos(cl_common->h2_Effects[index].Snow.max_org);
	cl_common->h2_Effects[index].Snow.flags = message.ReadByte();
	message.ReadPos(cl_common->h2_Effects[index].Snow.dir);
	cl_common->h2_Effects[index].Snow.count = message.ReadByte();
}

static void CLH2_ParseEffectFountain(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].Fountain.pos);
	cl_common->h2_Effects[index].Fountain.angle[0] = message.ReadAngle();
	cl_common->h2_Effects[index].Fountain.angle[1] = message.ReadAngle();
	cl_common->h2_Effects[index].Fountain.angle[2] = message.ReadAngle();
	message.ReadPos(cl_common->h2_Effects[index].Fountain.movedir);
	cl_common->h2_Effects[index].Fountain.color = message.ReadShort();
	cl_common->h2_Effects[index].Fountain.cnt = message.ReadByte();

	AngleVectors(cl_common->h2_Effects[index].Fountain.angle, 
		cl_common->h2_Effects[index].Fountain.vforward,
		cl_common->h2_Effects[index].Fountain.vright,
		cl_common->h2_Effects[index].Fountain.vup);
}

static void CLH2_ParseEffectQuake(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].Quake.origin);
	cl_common->h2_Effects[index].Quake.radius = message.ReadFloat();
}

static bool CLH2_ParseEffectSmokeCommon(int index, QMsg& message, const char* modelName, int drawFlags)
{
	message.ReadPos(cl_common->h2_Effects[index].Smoke.origin);
	cl_common->h2_Effects[index].Smoke.velocity[0] = message.ReadFloat();
	cl_common->h2_Effects[index].Smoke.velocity[1] = message.ReadFloat();
	cl_common->h2_Effects[index].Smoke.velocity[2] = message.ReadFloat();
	cl_common->h2_Effects[index].Smoke.framelength = message.ReadFloat();
	if (!(GGameType & GAME_HexenWorld))
	{
		cl_common->h2_Effects[index].Smoke.frame = message.ReadFloat();
	}

	cl_common->h2_Effects[index].Smoke.entity_index = CLH2_NewEffectEntity();
	if (cl_common->h2_Effects[index].Smoke.entity_index == -1)
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];
	VectorCopy(cl_common->h2_Effects[index].Smoke.origin, ent->state.origin);
	ent->model = R_RegisterModel(modelName);
	ent->state.drawflags = drawFlags;
	return true;
}

static bool CLH2_ParseEffectWhiteSmoke(int index, QMsg& message)
{
	return CLH2_ParseEffectSmokeCommon(index, message, "models/whtsmk1.spr", H2DRF_TRANSLUCENT);
}

static bool CLH2_ParseEffectGreenSmoke(int index, QMsg& message)
{
	return CLH2_ParseEffectSmokeCommon(index, message, "models/grnsmk1.spr", H2DRF_TRANSLUCENT);
}

static bool CLH2_ParseEffectGraySmoke(int index, QMsg& message)
{
	return CLH2_ParseEffectSmokeCommon(index, message, "models/grysmk1.spr", H2DRF_TRANSLUCENT);
}

static bool CLH2_ParseEffectRedSmoke(int index, QMsg& message)
{
	return CLH2_ParseEffectSmokeCommon(index, message, "models/redsmk1.spr", H2DRF_TRANSLUCENT);
}

static bool CLH2_ParseEffectTeleportSmoke1(int index, QMsg& message)
{
	if (!CLH2_ParseEffectSmokeCommon(index, message, "models/telesmk1.spr", H2DRF_TRANSLUCENT))
	{
		return false;
	}

	if (GGameType & GAME_HexenWorld)
	{
		S_StartSound(cl_common->h2_Effects[index].Smoke.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_ravenfire, 1, 1);

		if ((cl_common->h2_Effects[index].Smoke.entity_index2 = CLH2_NewEffectEntity()) != -1)
		{
			effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index2];
			VectorCopy(cl_common->h2_Effects[index].Smoke.origin, ent->state.origin);
			ent->model = R_RegisterModel("models/telesmk1.spr");
			ent->state.drawflags = H2DRF_TRANSLUCENT;
		}
	}
	return true;
}

static bool CLH2_ParseEffectTeleportSmoke2(int index, QMsg& message)
{
	return CLH2_ParseEffectSmokeCommon(index, message, "models/telesmk2.spr", H2DRF_TRANSLUCENT);
}

static bool CLH2_ParseEffectGhost(int index, QMsg& message)
{
	if (!CLH2_ParseEffectSmokeCommon(index, message, "models/ghost.spr", H2DRF_TRANSLUCENT | H2MLS_ABSLIGHT))
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];
	ent->state.abslight = .5;
	if (GGameType & GAME_HexenWorld)
	{
		Log::write("Bad effect type %d\n", (int)cl_common->h2_Effects[index].type);
		return false;
	}
	return true;
}

static bool CLH2_ParseEffectRedCloud(int index, QMsg& message)
{
	return CLH2_ParseEffectSmokeCommon(index, message, "models/rcloud.spr", 0);
}

static bool CLH2_ParseEffectAcidMuzzleFlash(int index, QMsg& message)
{
	if (!CLH2_ParseEffectSmokeCommon(index, message, "models/muzzle1.spr", H2DRF_TRANSLUCENT | H2MLS_ABSLIGHT))
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];
	ent->state.abslight = 0.2;
	return true;
}

static bool CLH2_ParseEffectFlameStream(int index, QMsg& message)
{
	if (!CLH2_ParseEffectSmokeCommon(index, message, "models/flamestr.spr", H2DRF_TRANSLUCENT | H2MLS_ABSLIGHT))
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];
	ent->state.abslight = 1;
	ent->state.frame = cl_common->h2_Effects[index].Smoke.frame;
	return true;
}

static bool CLH2_ParseEffectFlameWall(int index, QMsg& message)
{
	return CLH2_ParseEffectSmokeCommon(index, message, "models/firewal1.spr", 0);
}

static bool CLH2_ParseEffectFlameWall2(int index, QMsg& message)
{
	return CLH2_ParseEffectSmokeCommon(index, message, "models/firewal2.spr", H2DRF_TRANSLUCENT);
}

static const char* CLH2_ChooseOnFireModel()
{
	int rdm = rand() & 3;
	if (rdm < 1)
	{
		return "models/firewal1.spr";
	}
	else if (rdm < 2)
	{
		return "models/firewal2.spr";
	}
	else
	{
		return "models/firewal3.spr";
	}
}

static bool CLH2_ParseEffectOnFire(int index, QMsg& message)
{
	if (!CLH2_ParseEffectSmokeCommon(index, message, CLH2_ChooseOnFireModel(), H2DRF_TRANSLUCENT))
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];
	ent->state.abslight = 1;
	ent->state.frame = cl_common->h2_Effects[index].Smoke.frame;
	return true;
}

static bool CLHW_ParseEffectRipple(int index, QMsg& message)
{
	if (!CLH2_ParseEffectSmokeCommon(index, message, "models/ripple.spr", H2DRF_TRANSLUCENT))
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];
	ent->state.angles[0] = 90;

	if (cl_common->h2_Effects[index].Smoke.framelength == 2)
	{
		CLH2_SplashParticleEffect(cl_common->h2_Effects[index].Smoke.origin, 200, 406 + rand() % 8, pt_h2slowgrav, 40);
		S_StartSound(cl_common->h2_Effects[index].Smoke.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_splash, 1, 1);
	}
	else if (cl_common->h2_Effects[index].Smoke.framelength == 1)
	{
		CLH2_SplashParticleEffect(cl_common->h2_Effects[index].Smoke.origin, 100, 406 + rand() % 8, pt_h2slowgrav, 20);
	}
	else
	{
		S_StartSound(cl_common->h2_Effects[index].Smoke.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_ripple, 1, 1);
	}
	cl_common->h2_Effects[index].Smoke.framelength = 0.05;
	return true;
}

static bool CLH2_ParseEffectExplosionCommon(int index, QMsg& message, const char* modelName)
{
	message.ReadPos(cl_common->h2_Effects[index].Smoke.origin);

	cl_common->h2_Effects[index].Smoke.entity_index = CLH2_NewEffectEntity();
	if (cl_common->h2_Effects[index].Smoke.entity_index == -1)
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];
	VectorCopy(cl_common->h2_Effects[index].Smoke.origin, ent->state.origin);
	ent->model = R_RegisterModel(modelName);
	return true;
}

static bool CLH2_ParseEffectSmallWhiteFlash(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/sm_white.spr");
}

static bool CLH2_ParseEffectYellowRedFlash(int index, QMsg& message)
{
	if (!CLH2_ParseEffectExplosionCommon(index, message, "models/yr_flsh.spr"))
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];
	ent->state.drawflags = H2DRF_TRANSLUCENT;
	return true;
}

static bool CLH2_ParseEffectBlueSpark(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/bspark.spr");
}

static bool CLH2_ParseEffectYellowSpark(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/spark.spr");
}

static bool CLH2_ParseEffectSmallCircleExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/fcircle.spr");
}

static bool CLH2_ParseEffectBigCircleExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/xplod29.spr");
}

static bool CLH2_ParseEffectSmallExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/sm_expld.spr");
}

static bool CLHW_ParseEffectSmallExplosionWithSound(int index, QMsg& message)
{
	if (!CLH2_ParseEffectSmallExplosion(index, message))
	{
		return false;
	}
	S_StartSound(cl_common->h2_Effects[index].Smoke.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_explode, 1, 1);
	return true;
}

static bool CLH2_ParseEffectBigExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/bg_expld.spr");
}

static bool CLHW_ParseEffectBigExplosionWithSound(int index, QMsg& message)
{
	if (!CLH2_ParseEffectBigExplosion(index, message))
	{
		return false;
	}
	S_StartSound(cl_common->h2_Effects[index].Smoke.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_explode, 1, 1);
	return true;
}

static bool CLH2_ParseEffectFloorExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/fl_expld.spr");
}

static bool CLH2_ParseEffectFloorExplosion2(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/flrexpl2.spr");
}

static bool CLH2_ParseEffectFloorExplosion3(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/biggy.spr");
}

static bool CLH2_ParseEffectBlueExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/xpspblue.spr");
}

static bool CLH2_ParseEffectRedSpark(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/rspark.spr");
}

static bool CLH2_ParseEffectGreenSpark(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/gspark.spr");
}

static bool CLH2_ParseEffectIceHit(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/icehit.spr");
}

static bool CLH2_ParseEffectMedusaHit(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/medhit.spr");
}

static bool CLH2_ParseEffectMezzoReflect(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/mezzoref.spr");
}

static bool CLH2_ParseEffectXBowExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/xbowexpl.spr");
}

static bool CLH2_ParseEffectNewExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/gen_expl.spr");
}

static bool CLH2_ParseEffectMagicMissileExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/mm_expld.spr");
}

static bool CLHW_ParseEffectMagicMissileExplosionWithSound(int index, QMsg& message)
{
	if (!CLH2_ParseEffectMagicMissileExplosion(index, message))
	{
		return false;
	}
	S_StartSound(cl_common->h2_Effects[index].Smoke.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_explode, 1, 1);
	return true;
}

static bool CLH2_ParseEffectBoneExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/bonexpld.spr");
}

static bool CLH2_ParseEffectBldrnExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/xplsn_1.spr");
}

static bool CLH2_ParseEffectLShock(int index, QMsg& message)
{
	if (!CLH2_ParseEffectExplosionCommon(index, message, "models/vorpshok.mdl"))
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];
	ent->state.drawflags = H2MLS_TORCH;
	ent->state.angles[2] = 90;
	ent->state.scale = 255;
	return true;
}

static bool CLH2_ParseEffectAcidHit(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/axplsn_2.spr");
}

static bool CLH2_ParseEffectAcidSplat(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/axplsn_1.spr");
}

static bool CLH2_ParseEffectAcidExplosion(int index, QMsg& message)
{
	if (!CLH2_ParseEffectExplosionCommon(index, message, "models/axplsn_5.spr"))
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];
	ent->state.drawflags = H2MLS_ABSLIGHT;
	ent->state.abslight = 1;
	return true;
}

static bool CLH2_ParseEffectLBallExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/Bluexp3.spr");
}

static bool CLH2_ParseEffectFBoom(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/fboom.spr");
}

static bool CLH2_ParseEffectBomb(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/pow.spr");
}

static bool CLH2_ParseEffectFirewallSall(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/firewal1.spr");
}

static bool CLH2_ParseEffectFirewallMedium(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/firewal5.spr");
}

static bool CLH2_ParseEffectFirewallLarge(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/firewal4.spr");
}

static bool CLH2_ParseEffectFlashCommon(int index, QMsg& message, const char* modelName)
{
	message.ReadPos(cl_common->h2_Effects[index].Flash.origin);
	cl_common->h2_Effects[index].Flash.reverse = 0;
	if ((cl_common->h2_Effects[index].Flash.entity_index = CLH2_NewEffectEntity()) == -1)
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Flash.entity_index];
	VectorCopy(cl_common->h2_Effects[index].Flash.origin, ent->state.origin);
	ent->model = R_RegisterModel(modelName);
	ent->state.drawflags = H2DRF_TRANSLUCENT;
	return true;
}

static bool CLH2_ParseEffectWhiteFlash(int index, QMsg& message)
{
	return CLH2_ParseEffectFlashCommon(index, message, "models/gryspt.spr");
}

static bool CLH2_ParseEffectBlueFlash(int index, QMsg& message)
{
	return CLH2_ParseEffectFlashCommon(index, message, "models/bluflash.spr");
}

static bool CLH2_ParseEffectSmallBlueFlash(int index, QMsg& message)
{
	return CLH2_ParseEffectFlashCommon(index, message, "models/sm_blue.spr");
}

static bool CLHW_ParseEffectSplitFlash(int index, QMsg& message)
{
	if (!CLH2_ParseEffectSmallBlueFlash(index, message))
	{
		return false;
	}
	S_StartSound(cl_common->h2_Effects[index].Flash.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_ravensplit, 1, 1);
	return true;
}

static bool CLH2_ParseEffectRedFlash(int index, QMsg& message)
{
	return CLH2_ParseEffectFlashCommon(index, message, "models/redspt.spr");
}

static void CLH2_ParseEffectRiderDeath(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].RD.origin);
}

static void CLH2_ParseEffectGravityWell(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].GravityWell.origin);
	cl_common->h2_Effects[index].GravityWell.color = message.ReadShort();
	cl_common->h2_Effects[index].GravityWell.lifetime = message.ReadFloat();
}

static void CLH2_ParseEffectTeleporterPuffs(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].Teleporter.origin);

	cl_common->h2_Effects[index].Teleporter.framelength = .05;
	int dir = 0;
	for (int i = 0; i < 8; i++)
	{		
		if ((cl_common->h2_Effects[index].Teleporter.entity_index[i] = CLH2_NewEffectEntity()) == -1)
		{
			continue;
		}
		effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Teleporter.entity_index[i]];
		VectorCopy(cl_common->h2_Effects[index].Teleporter.origin, ent->state.origin);

		float angleval = dir * M_PI * 2 / 360;

		float sinval = sin(angleval);
		float cosval = cos(angleval);

		cl_common->h2_Effects[index].Teleporter.velocity[i][0] = 10 * cosval;
		cl_common->h2_Effects[index].Teleporter.velocity[i][1] = 10 * sinval;
		cl_common->h2_Effects[index].Teleporter.velocity[i][2] = 0;
		dir += 45;

		ent->model = R_RegisterModel("models/telesmk2.spr");
		ent->state.drawflags = H2DRF_TRANSLUCENT;
	}
}

static void CLH2_ParseEffectTeleporterBody(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].Teleporter.origin);
	cl_common->h2_Effects[index].Teleporter.velocity[0][0] = message.ReadFloat();
	cl_common->h2_Effects[index].Teleporter.velocity[0][1] = message.ReadFloat();
	cl_common->h2_Effects[index].Teleporter.velocity[0][2] = message.ReadFloat();
	float skinnum = message.ReadFloat();
	
	cl_common->h2_Effects[index].Teleporter.framelength = .05;
	if ((cl_common->h2_Effects[index].Teleporter.entity_index[0] = CLH2_NewEffectEntity()) == -1)
	{
		return;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Teleporter.entity_index[0]];
	VectorCopy(cl_common->h2_Effects[index].Teleporter.origin, ent->state.origin);

	ent->model = R_RegisterModel("models/teleport.mdl");
	ent->state.drawflags = H2SCALE_TYPE_XYONLY | H2DRF_TRANSLUCENT;
	ent->state.scale = 100;
	ent->state.skinnum = skinnum;
}

static bool CLH2_ParseEffectMissileCommon(int index, QMsg& message, const char* modelName)
{
	message.ReadPos(cl_common->h2_Effects[index].Missile.origin);
	cl_common->h2_Effects[index].Missile.velocity[0] = message.ReadFloat();
	cl_common->h2_Effects[index].Missile.velocity[1] = message.ReadFloat();
	cl_common->h2_Effects[index].Missile.velocity[2] = message.ReadFloat();
	cl_common->h2_Effects[index].Missile.angle[0] = message.ReadFloat();
	cl_common->h2_Effects[index].Missile.angle[1] = message.ReadFloat();
	cl_common->h2_Effects[index].Missile.angle[2] = message.ReadFloat();
	cl_common->h2_Effects[index].Missile.avelocity[0] = message.ReadFloat();
	cl_common->h2_Effects[index].Missile.avelocity[1] = message.ReadFloat();
	cl_common->h2_Effects[index].Missile.avelocity[2] = message.ReadFloat();

	if ((cl_common->h2_Effects[index].Missile.entity_index = CLH2_NewEffectEntity()) == -1)
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Missile.entity_index];
	VectorCopy(cl_common->h2_Effects[index].Missile.origin, ent->state.origin);
	if (GGameType & GAME_HexenWorld)
	{
		VectorCopy(cl_common->h2_Effects[index].Missile.angle, ent->state.angles);
	}
	ent->model = R_RegisterModel(modelName);
	return true;
}

static bool CLHW_ParseEffectMissileCommonNoAngles(int index, QMsg& message, const char* modelName)
{
	message.ReadPos(cl_common->h2_Effects[index].Missile.origin);
	cl_common->h2_Effects[index].Missile.velocity[0] = message.ReadFloat();
	cl_common->h2_Effects[index].Missile.velocity[1] = message.ReadFloat();
	cl_common->h2_Effects[index].Missile.velocity[2] = message.ReadFloat();

	VecToAnglesBuggy(cl_common->h2_Effects[index].Missile.velocity, cl_common->h2_Effects[index].Missile.angle);
	if ((cl_common->h2_Effects[index].Missile.entity_index = CLH2_NewEffectEntity()) == -1)
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Missile.entity_index];
	VectorCopy(cl_common->h2_Effects[index].Missile.origin, ent->state.origin);
	VectorCopy(cl_common->h2_Effects[index].Missile.angle, ent->state.angles);
	ent->model = R_RegisterModel(modelName);
	return true;
}

static bool CLH2_ParseEffectBoneShard(int index, QMsg& message)
{
	return CLH2_ParseEffectMissileCommon(index, message, "models/boneshot.mdl");
}

static bool CLHW_ParseEffectBoneShard(int index, QMsg& message)
{
	if (!CLHW_ParseEffectMissileCommonNoAngles(index, message, "models/boneshot.mdl"))
	{
		return false;
	}
	cl_common->h2_Effects[index].Missile.avelocity[0] = (rand() % 1554) - 777;
	S_StartSound(cl_common->h2_Effects[index].Missile.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_bone, 1, 1);
	return true;
}

static bool CLH2_ParseEffectBoneShrapnel(int index, QMsg& message)
{
	return CLH2_ParseEffectMissileCommon(index, message, "models/boneshrd.mdl");
}

static void CLH2_ParseEffectChunk(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].Chunk.origin);
	cl_common->h2_Effects[index].Chunk.type = message.ReadByte();
	cl_common->h2_Effects[index].Chunk.srcVel[0] = message.ReadCoord();
	cl_common->h2_Effects[index].Chunk.srcVel[1] = message.ReadCoord();
	cl_common->h2_Effects[index].Chunk.srcVel[2] = message.ReadCoord();
	cl_common->h2_Effects[index].Chunk.numChunks = message.ReadByte();

	CLH2_InitChunkEffect(cl_common->h2_Effects[index]);
}

static bool CLHW_ParseEffectBoneBall(int index, QMsg& message)
{
	if (!CLH2_ParseEffectMissileCommon(index, message, "models/bonelump.mdl"))
	{
		return false;
	}
	S_StartSound(cl_common->h2_Effects[index].Missile.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_bonefpow, 1, 1);
	return true;
}

static bool CLHW_ParseEffectRavenStaff(int index, QMsg& message)
{
	if (!CLHW_ParseEffectMissileCommonNoAngles(index, message, "models/vindsht1.mdl"))
	{
		return false;
	}
	cl_common->h2_Effects[index].Missile.avelocity[2] = 1000;
	return true;
}

static bool CLHW_ParseEffectRavenPower(int index, QMsg& message)
{
	if (!CLHW_ParseEffectMissileCommonNoAngles(index, message, "models/ravproj.mdl"))
	{
		return false;
	}
	S_StartSound(cl_common->h2_Effects[index].Missile.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_ravengo, 1, 1);
	return true;
}

static void CLHW_ParseEffectDeathBubbles(int index, QMsg& message)
{
	cl_common->h2_Effects[index].Bubble.owner = message.ReadShort();
	cl_common->h2_Effects[index].Bubble.offset[0] = message.ReadByte();
	cl_common->h2_Effects[index].Bubble.offset[1] = message.ReadByte();
	cl_common->h2_Effects[index].Bubble.offset[2] = message.ReadByte();
	cl_common->h2_Effects[index].Bubble.count = message.ReadByte();//num of bubbles
	cl_common->h2_Effects[index].Bubble.time_amount = 0;
}

static void CLHW_ParseEffectXBowCommon(int index, QMsg& message, vec3_t origin)
{
	message.ReadPos(origin);
	cl_common->h2_Effects[index].Xbow.angle[0] = message.ReadAngle();
	cl_common->h2_Effects[index].Xbow.angle[1] = message.ReadAngle();
	cl_common->h2_Effects[index].Xbow.angle[2] = 0;
}

static void CLHW_ParseEffectXBowThunderbolt(int index, QMsg& message, int i, vec3_t forward2)
{
	message.ReadPos(cl_common->h2_Effects[index].Xbow.origin[i]);
	vec3_t vtemp;
	vtemp[0] = message.ReadAngle();
	vtemp[1] = message.ReadAngle();
	vtemp[2] = 0;
	vec3_t right2, up2;
	AngleVectors(vtemp, forward2, right2, up2);
}

static void CLHW_InitXBowEffectEntity(int index, int i, const char* modelName)
{
	cl_common->h2_Effects[index].Xbow.ent[i] = CLH2_NewEffectEntity();
	if (cl_common->h2_Effects[index].Xbow.ent[i] == -1)
	{
		return;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Xbow.ent[i]];
	VectorCopy(cl_common->h2_Effects[index].Xbow.origin[i], ent->state.origin);
	VecToAnglesBuggy(cl_common->h2_Effects[index].Xbow.vel[i], ent->state.angles);
	ent->model = R_RegisterModel(modelName);
}

static void CLHW_ParseEffectXBowShoot(int index, QMsg& message)
{
	vec3_t origin;
	CLHW_ParseEffectXBowCommon(index, message, origin);
	cl_common->h2_Effects[index].Xbow.bolts = message.ReadByte();
	cl_common->h2_Effects[index].Xbow.randseed = message.ReadByte();
	cl_common->h2_Effects[index].Xbow.turnedbolts = message.ReadByte();
	cl_common->h2_Effects[index].Xbow.activebolts= message.ReadByte();

	setseed(cl_common->h2_Effects[index].Xbow.randseed);

	vec3_t forward, right, up;
	AngleVectors(cl_common->h2_Effects[index].Xbow.angle, forward, right, up);

	VectorNormalize(forward);
	VectorCopy(forward, cl_common->h2_Effects[index].Xbow.velocity);

	if (cl_common->h2_Effects[index].Xbow.bolts == 3)
	{
		S_StartSound(origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_xbowshoot, 1, 1);
	}
	else if (cl_common->h2_Effects[index].Xbow.bolts == 5)
	{
		S_StartSound(origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_xbowfshoot, 1, 1);
	}

	for (int i = 0; i < cl_common->h2_Effects[index].Xbow.bolts; i++)
	{
		cl_common->h2_Effects[index].Xbow.gonetime[i] = 1 + seedrand() * 2;
		cl_common->h2_Effects[index].Xbow.state[i] = 0;

		if ((1 << i) & cl_common->h2_Effects[index].Xbow.turnedbolts)
		{
			vec3_t forward2;
			CLHW_ParseEffectXBowThunderbolt(index, message, i, forward2);
			VectorScale(forward2, 800 + seedrand() * 500, cl_common->h2_Effects[index].Xbow.vel[i]);
		}
		else
		{
			VectorScale(forward, 800 + seedrand() * 500, cl_common->h2_Effects[index].Xbow.vel[i]);

			vec3_t vtemp;
			VectorScale(right, i * 100 - (cl_common->h2_Effects[index].Xbow.bolts- 1) * 50, vtemp);

			//this should only be done for deathmatch:
			VectorScale(vtemp, 0.333, vtemp);

			VectorAdd(cl_common->h2_Effects[index].Xbow.vel[i], vtemp, cl_common->h2_Effects[index].Xbow.vel[i]);

			//start me off a bit out
			VectorScale(vtemp, 0.05, cl_common->h2_Effects[index].Xbow.origin[i]);
			VectorAdd(origin, cl_common->h2_Effects[index].Xbow.origin[i], cl_common->h2_Effects[index].Xbow.origin[i]);
		}
		if (cl_common->h2_Effects[index].Xbow.bolts == 5)
		{
			CLHW_InitXBowEffectEntity(index, i, "models/flaming.mdl");
		}
		else
		{
			CLHW_InitXBowEffectEntity(index, i, "models/arrow.mdl");
		}
	}
}

static void CLHW_ParseEffectSheepinator(int index, QMsg& message)
{
	vec3_t origin;
	CLHW_ParseEffectXBowCommon(index, message, origin);
	cl_common->h2_Effects[index].Xbow.turnedbolts = message.ReadByte();
	cl_common->h2_Effects[index].Xbow.activebolts= message.ReadByte();
	cl_common->h2_Effects[index].Xbow.bolts = 5;
	cl_common->h2_Effects[index].Xbow.randseed = 0;

	vec3_t forward, right, up;
	AngleVectors(cl_common->h2_Effects[index].Xbow.angle, forward, right, up);

	VectorNormalize(forward);
	VectorCopy(forward, cl_common->h2_Effects[index].Xbow.velocity);

	for (int i = 0; i < cl_common->h2_Effects[index].Xbow.bolts; i++)
	{
		cl_common->h2_Effects[index].Xbow.gonetime[i] = 0;
		cl_common->h2_Effects[index].Xbow.state[i] = 0;

		if ((1 << i) & cl_common->h2_Effects[index].Xbow.turnedbolts)
		{
			vec3_t forward2;
			CLHW_ParseEffectXBowThunderbolt(index, message, i, forward2);
			VectorScale(forward2, 700, cl_common->h2_Effects[index].Xbow.vel[i]);
		}
		else
		{
			VectorCopy(origin, cl_common->h2_Effects[index].Xbow.origin[i]);
			VectorScale(forward, 700, cl_common->h2_Effects[index].Xbow.vel[i]);
			vec3_t vtemp;
			VectorScale(right, i * 75 - 150, vtemp);
			VectorAdd(cl_common->h2_Effects[index].Xbow.vel[i], vtemp, cl_common->h2_Effects[index].Xbow.vel[i]);
		}
		CLHW_InitXBowEffectEntity(index, i, "models/polymrph.spr");
	}
}

static bool CLHW_ParseEffectDrilla(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].Missile.origin);
	cl_common->h2_Effects[index].Missile.angle[0] = message.ReadAngle();
	cl_common->h2_Effects[index].Missile.angle[1] = message.ReadAngle();
	cl_common->h2_Effects[index].Missile.angle[2] = 0;
	cl_common->h2_Effects[index].Missile.speed = message.ReadShort();

	S_StartSound(cl_common->h2_Effects[index].Missile.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_drillashoot, 1, 1);

	vec3_t right, up;
	AngleVectors(cl_common->h2_Effects[index].Missile.angle, cl_common->h2_Effects[index].Missile.velocity, right, up);

	VectorScale(cl_common->h2_Effects[index].Missile.velocity, cl_common->h2_Effects[index].Missile.speed, cl_common->h2_Effects[index].Missile.velocity);

	cl_common->h2_Effects[index].Missile.entity_index = CLH2_NewEffectEntity();
	if (cl_common->h2_Effects[index].Missile.entity_index == -1)
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Missile.entity_index];
	VectorCopy(cl_common->h2_Effects[index].Missile.origin, ent->state.origin);
	VectorCopy(cl_common->h2_Effects[index].Missile.angle, ent->state.angles);
	ent->model = R_RegisterModel("models/scrbstp1.mdl");
	return true;
}

static bool CLHW_ParseEffectScarabChain(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].Chain.origin);
	cl_common->h2_Effects[index].Chain.owner = message.ReadShort();
	cl_common->h2_Effects[index].Chain.tag = message.ReadByte();

	cl_common->h2_Effects[index].Chain.material = cl_common->h2_Effects[index].Chain.owner >> 12;
	cl_common->h2_Effects[index].Chain.owner &= 0x0fff;
	cl_common->h2_Effects[index].Chain.height = 16;

	cl_common->h2_Effects[index].Chain.sound_time = cl_common->serverTime * 0.001;

	cl_common->h2_Effects[index].Chain.state = 0;//state 0: move slowly toward owner

	S_StartSound(cl_common->h2_Effects[index].Chain.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_scarabwhoosh, 1, 1);

	cl_common->h2_Effects[index].Chain.ent1 = CLH2_NewEffectEntity();
	if (cl_common->h2_Effects[index].Chain.ent1 == -1)
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Chain.ent1];
	VectorCopy(cl_common->h2_Effects[index].Chain.origin, ent->state.origin);
	ent->model = R_RegisterModel("models/scrbpbdy.mdl");
	return true;
}

static bool CLHW_ParseEffectTripMineCommon(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].Chain.origin);
	cl_common->h2_Effects[index].Chain.velocity[0] = message.ReadFloat();
	cl_common->h2_Effects[index].Chain.velocity[1] = message.ReadFloat();
	cl_common->h2_Effects[index].Chain.velocity[2] = message.ReadFloat();

	cl_common->h2_Effects[index].Chain.ent1 = CLH2_NewEffectEntity();
	if (cl_common->h2_Effects[index].Chain.ent1 == -1)
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Chain.ent1];
	ent->model = R_RegisterModel("models/twspike.mdl");
	return true;
}

static bool CLHW_ParseEffectTripMine(int index, QMsg& message)
{
	if (!CLHW_ParseEffectTripMineCommon(index, message))
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Chain.ent1];
	VectorCopy(cl_common->h2_Effects[index].Chain.origin, ent->state.origin);
	return true;
}

static bool CLHW_ParseEffectTripMineStill(int index, QMsg& message)
{
	if (!CLHW_ParseEffectTripMineCommon(index, message))
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Chain.ent1];
	VectorCopy(cl_common->h2_Effects[index].Chain.velocity, ent->state.origin);
	return true;
}

static bool CLHW_ParseEffectStarCommon(int index, QMsg& message)
{
	cl_common->h2_Effects[index].Star.origin[0] = message.ReadCoord();
	cl_common->h2_Effects[index].Star.origin[1] = message.ReadCoord();
	cl_common->h2_Effects[index].Star.origin[2] = message.ReadCoord();

	cl_common->h2_Effects[index].Star.velocity[0] = message.ReadFloat();
	cl_common->h2_Effects[index].Star.velocity[1] = message.ReadFloat();
	cl_common->h2_Effects[index].Star.velocity[2] = message.ReadFloat();

	VecToAnglesBuggy(cl_common->h2_Effects[index].Star.velocity, cl_common->h2_Effects[index].Star.angle);
	cl_common->h2_Effects[index].Missile.avelocity[2] = 300 + rand() % 300;

	cl_common->h2_Effects[index].Star.scaleDir = 1;
	cl_common->h2_Effects[index].Star.scale = 0.3;

	cl_common->h2_Effects[index].Star.entity_index = CLH2_NewEffectEntity();
	if (cl_common->h2_Effects[index].Star.entity_index == -1)
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Star.entity_index];
	VectorCopy(cl_common->h2_Effects[index].Star.origin, ent->state.origin);
	VectorCopy(cl_common->h2_Effects[index].Star.angle, ent->state.angles);
	ent->model = R_RegisterModel("models/ball.mdl");
	return true;
}

static bool CLHW_ParseEffectMissileStar(int index, QMsg& message)
{
	bool ImmediateFree = !CLHW_ParseEffectStarCommon(index, message);

	cl_common->h2_Effects[index].Star.ent1 = CLH2_NewEffectEntity();
	if (cl_common->h2_Effects[index].Star.ent1 != -1)
	{
		effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Star.ent1];
		VectorCopy(cl_common->h2_Effects[index].Star.origin, ent->state.origin);
		ent->state.drawflags |= H2MLS_ABSLIGHT;
		ent->state.abslight = 0.5;
		ent->state.angles[2] = 90;
		ent->model = R_RegisterModel("models/star.mdl");
		ent->state.scale = 0.3;
		S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_mmfire, 1, 1);
	}
	cl_common->h2_Effects[index].Star.ent2 = CLH2_NewEffectEntity();
	if (cl_common->h2_Effects[index].Star.ent2 != -1)
	{
		effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Star.ent2];
		VectorCopy(cl_common->h2_Effects[index].Star.origin, ent->state.origin);
		ent->model = R_RegisterModel("models/star.mdl");
		ent->state.drawflags |= H2MLS_ABSLIGHT;
		ent->state.abslight = 0.5;
		ent->state.scale = 0.3;
	}
	return !ImmediateFree;
}

static bool CLHW_ParseEffectEidolonStar(int index, QMsg& message)
{
	bool ImmediateFree = !CLHW_ParseEffectStarCommon(index, message);

	if ((cl_common->h2_Effects[index].Star.ent1 = CLH2_NewEffectEntity()) != -1)
	{
		effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Star.ent1];
		VectorCopy(cl_common->h2_Effects[index].Star.origin, ent->state.origin);
		ent->state.drawflags |= H2MLS_ABSLIGHT;
		ent->state.abslight = 0.5;
		ent->state.angles[2] = 90;
		ent->model = R_RegisterModel("models/glowball.mdl");
		S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_eidolon, 1, 1);
	}
	return !ImmediateFree;
}

static bool CLH2_ParseEffectType(int index, QMsg& message)
{
	bool ImmediateFree = false;
	switch (cl_common->h2_Effects[index].type)
	{
	case H2CE_RAIN:
		CLH2_ParseEffectRain(index, message);
		break;
	case H2CE_SNOW:
		CLH2_ParseEffectSnow(index, message);
		break;
	case H2CE_FOUNTAIN:
		CLH2_ParseEffectFountain(index, message);
		break;
	case H2CE_QUAKE:
		CLH2_ParseEffectQuake(index, message);
		break;
	case H2CE_WHITE_SMOKE:
	case H2CE_SLOW_WHITE_SMOKE:
		ImmediateFree = CLH2_ParseEffectWhiteSmoke(index, message);
		break;
	case H2CE_GREEN_SMOKE:
		ImmediateFree = !CLH2_ParseEffectGreenSmoke(index, message);
		break;
	case H2CE_GREY_SMOKE:
		ImmediateFree = !CLH2_ParseEffectGraySmoke(index, message);
		break;
	case H2CE_RED_SMOKE:
		ImmediateFree = !CLH2_ParseEffectRedSmoke(index, message);
		break;
	case H2CE_TELESMK1:
		ImmediateFree = !CLH2_ParseEffectTeleportSmoke1(index, message);
		break;
	case H2CE_TELESMK2:
		ImmediateFree = !CLH2_ParseEffectTeleportSmoke2(index, message);
		break;
	case H2CE_GHOST:
		ImmediateFree = !CLH2_ParseEffectGhost(index, message);
		break;
	case H2CE_REDCLOUD:
		ImmediateFree = !CLH2_ParseEffectRedCloud(index, message);
		break;
	case H2CE_ACID_MUZZFL:
		ImmediateFree = !CLH2_ParseEffectAcidMuzzleFlash(index, message);
		break;
	case H2CE_FLAMESTREAM:
		ImmediateFree = !CLH2_ParseEffectFlameStream(index, message);
		break;
	case H2CE_FLAMEWALL:
		ImmediateFree = !CLH2_ParseEffectFlameWall(index, message);
		break;
	case H2CE_FLAMEWALL2:
		ImmediateFree = !CLH2_ParseEffectFlameWall2(index, message);
		break;
	case H2CE_ONFIRE:
		ImmediateFree = !CLH2_ParseEffectOnFire(index, message);
		break;
	case H2CE_SM_WHITE_FLASH:
		ImmediateFree = !CLH2_ParseEffectSmallWhiteFlash(index, message);
		break;
	case H2CE_YELLOWRED_FLASH:
		ImmediateFree = !CLH2_ParseEffectYellowRedFlash(index, message);
		break;
	case H2CE_BLUESPARK:
		ImmediateFree = !CLH2_ParseEffectBlueSpark(index, message);
		break;
	case H2CE_YELLOWSPARK:
	case H2CE_BRN_BOUNCE:
		ImmediateFree = !CLH2_ParseEffectYellowSpark(index, message);
		break;
	case H2CE_SM_CIRCLE_EXP:
		ImmediateFree = !CLH2_ParseEffectSmallCircleExplosion(index, message);
		break;
	case H2CE_BG_CIRCLE_EXP:
		ImmediateFree = !CLH2_ParseEffectBigCircleExplosion(index, message);
		break;
	case H2CE_SM_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectSmallExplosion(index, message);
		break;
	case H2CE_LG_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectBigExplosion(index, message);
		break;
	case H2CE_FLOOR_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectFloorExplosion(index, message);
		break;
	case H2CE_FLOOR_EXPLOSION3:
		ImmediateFree = !CLH2_ParseEffectFloorExplosion3(index, message);
		break;
	case H2CE_BLUE_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectBlueExplosion(index, message);
		break;
	case H2CE_REDSPARK:
		ImmediateFree = !CLH2_ParseEffectRedSpark(index, message);
		break;
	case H2CE_GREENSPARK:
		ImmediateFree = !CLH2_ParseEffectGreenSpark(index, message);
		break;
	case H2CE_ICEHIT:
		ImmediateFree = !CLH2_ParseEffectIceHit(index, message);
		break;
	case H2CE_MEDUSA_HIT:
		ImmediateFree = !CLH2_ParseEffectMedusaHit(index, message);
		break;
	case H2CE_MEZZO_REFLECT:
		ImmediateFree = !CLH2_ParseEffectMezzoReflect(index, message);
		break;
	case H2CE_FLOOR_EXPLOSION2:
		ImmediateFree = !CLH2_ParseEffectFloorExplosion2(index, message);
		break;
	case H2CE_XBOW_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectXBowExplosion(index, message);
		break;
	case H2CE_NEW_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectNewExplosion(index, message);
		break;
	case H2CE_MAGIC_MISSILE_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectMagicMissileExplosion(index, message);
		break;
	case H2CE_BONE_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectBoneExplosion(index, message);
		break;
	case H2CE_BLDRN_EXPL:
		ImmediateFree = !CLH2_ParseEffectBldrnExplosion(index, message);
		break;
	case H2CE_LSHOCK:
		ImmediateFree = !CLH2_ParseEffectLShock(index, message);
		break;
	case H2CE_ACID_HIT:
		ImmediateFree = !CLH2_ParseEffectAcidHit(index, message);
		break;
	case H2CE_ACID_SPLAT:
		ImmediateFree = !CLH2_ParseEffectAcidSplat(index, message);
		break;
	case H2CE_ACID_EXPL:
		ImmediateFree = !CLH2_ParseEffectAcidExplosion(index, message);
		break;
	case H2CE_LBALL_EXPL:
		ImmediateFree = !CLH2_ParseEffectLBallExplosion(index, message);
		break;
	case H2CE_FBOOM:
		ImmediateFree = !CLH2_ParseEffectFBoom(index, message);
		break;
	case H2CE_BOMB:
		ImmediateFree = !CLH2_ParseEffectBomb(index, message);
		break;
	case H2CE_FIREWALL_SMALL:
		ImmediateFree = !CLH2_ParseEffectFirewallSall(index, message);
		break;
	case H2CE_FIREWALL_MEDIUM:
		ImmediateFree = !CLH2_ParseEffectFirewallMedium(index, message);
		break;
	case H2CE_FIREWALL_LARGE:
		ImmediateFree = !CLH2_ParseEffectFirewallLarge(index, message);
		break;
	case H2CE_WHITE_FLASH:
		ImmediateFree = !CLH2_ParseEffectWhiteFlash(index, message);
		break;
	case H2CE_BLUE_FLASH:
		ImmediateFree = !CLH2_ParseEffectBlueFlash(index, message);
		break;
	case H2CE_SM_BLUE_FLASH:
		ImmediateFree = !CLH2_ParseEffectSmallBlueFlash(index, message);
		break;
	case H2CE_RED_FLASH:
		ImmediateFree = !CLH2_ParseEffectRedFlash(index, message);
		break;
	case H2CE_RIDER_DEATH:
		CLH2_ParseEffectRiderDeath(index, message);
		break;
	case H2CE_GRAVITYWELL:
		CLH2_ParseEffectGravityWell(index, message);
		break;
	case H2CE_TELEPORTERPUFFS:
		CLH2_ParseEffectTeleporterPuffs(index, message);
		break;
	case H2CE_TELEPORTERBODY:
		CLH2_ParseEffectTeleporterBody(index, message);
		break;
	case H2CE_BONESHARD:
		ImmediateFree = !CLH2_ParseEffectBoneShard(index, message);
		break;
	case H2CE_BONESHRAPNEL:
		ImmediateFree = !CLH2_ParseEffectBoneShrapnel(index, message);
		break;
	case H2CE_CHUNK:
		CLH2_ParseEffectChunk(index, message);
		break;
	default:
		throw Exception("CL_ParseEffect: bad type");
	}
	return ImmediateFree;
}

static bool CLHW_ParseEffectType(int index, QMsg& message)
{
	bool ImmediateFree = false;
	switch (cl_common->h2_Effects[index].type)
	{
	case HWCE_RAIN:
		CLH2_ParseEffectRain(index, message);
		break;
	case HWCE_FOUNTAIN:
		CLH2_ParseEffectFountain(index, message);
		break;
	case HWCE_QUAKE:
		CLH2_ParseEffectQuake(index, message);
		break;
	case HWCE_WHITE_SMOKE:
	case HWCE_SLOW_WHITE_SMOKE:
		ImmediateFree = !CLH2_ParseEffectWhiteSmoke(index, message);
		break;
	case HWCE_GREEN_SMOKE:
		ImmediateFree = !CLH2_ParseEffectGreenSmoke(index, message);
		break;
	case HWCE_GREY_SMOKE:
		ImmediateFree = !CLH2_ParseEffectGraySmoke(index, message);
		break;
	case HWCE_RED_SMOKE:
		ImmediateFree = !CLH2_ParseEffectRedSmoke(index, message);
		break;
	case HWCE_TELESMK1:
		ImmediateFree = !CLH2_ParseEffectTeleportSmoke1(index, message);
		break;
	case HWCE_TELESMK2:
		ImmediateFree = !CLH2_ParseEffectTeleportSmoke2(index, message);
		break;
	case HWCE_GHOST:
		ImmediateFree = !CLH2_ParseEffectGhost(index, message);
		break;
	case HWCE_REDCLOUD:
		ImmediateFree = !CLH2_ParseEffectRedCloud(index, message);
		break;
	case HWCE_ACID_MUZZFL:
		ImmediateFree = !CLH2_ParseEffectAcidMuzzleFlash(index, message);
		break;
	case HWCE_FLAMESTREAM:
		ImmediateFree = !CLH2_ParseEffectFlameStream(index, message);
		break;
	case HWCE_FLAMEWALL:
		ImmediateFree = !CLH2_ParseEffectFlameWall(index, message);
		break;
	case HWCE_FLAMEWALL2:
		ImmediateFree = !CLH2_ParseEffectFlameWall2(index, message);
		break;
	case HWCE_ONFIRE:
		ImmediateFree = !CLH2_ParseEffectOnFire(index, message);
		break;
	case HWCE_RIPPLE:
		ImmediateFree = !CLHW_ParseEffectRipple(index, message);
		break;
	case HWCE_SM_WHITE_FLASH:
		ImmediateFree = !CLH2_ParseEffectSmallWhiteFlash(index, message);
		break;
	case HWCE_YELLOWRED_FLASH:
		ImmediateFree = !CLH2_ParseEffectYellowRedFlash(index, message);
		break;
	case HWCE_BLUESPARK:
		ImmediateFree = !CLH2_ParseEffectBlueSpark(index, message);
		break;
	case HWCE_YELLOWSPARK:
	case HWCE_BRN_BOUNCE:
		ImmediateFree = !CLH2_ParseEffectYellowSpark(index, message);
		break;
	case HWCE_SM_CIRCLE_EXP:
		ImmediateFree = !CLH2_ParseEffectSmallCircleExplosion(index, message);
		break;
	case HWCE_BG_CIRCLE_EXP:
		ImmediateFree = !CLH2_ParseEffectBigCircleExplosion(index, message);
		break;
	case HWCE_SM_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectSmallExplosion(index, message);
		break;
	case HWCE_SM_EXPLOSION2:
		ImmediateFree = !CLHW_ParseEffectSmallExplosionWithSound(index, message);
		break;
	case HWCE_BG_EXPLOSION:
		ImmediateFree = !CLHW_ParseEffectBigExplosionWithSound(index, message);
		break;
	case HWCE_FLOOR_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectFloorExplosion(index, message);
		break;
	case HWCE_BLUE_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectBlueExplosion(index, message);
		break;
	case HWCE_REDSPARK:
		ImmediateFree = !CLH2_ParseEffectRedSpark(index, message);
		break;
	case HWCE_GREENSPARK:
		ImmediateFree = !CLH2_ParseEffectGreenSpark(index, message);
		break;
	case HWCE_ICEHIT:
		ImmediateFree = !CLH2_ParseEffectIceHit(index, message);
		break;
	case HWCE_MEDUSA_HIT:
		ImmediateFree = !CLH2_ParseEffectMedusaHit(index, message);
		break;
	case HWCE_MEZZO_REFLECT:
		ImmediateFree = !CLH2_ParseEffectMezzoReflect(index, message);
		break;
	case HWCE_FLOOR_EXPLOSION2:
		ImmediateFree = !CLH2_ParseEffectFloorExplosion2(index, message);
		break;
	case HWCE_XBOW_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectXBowExplosion(index, message);
		break;
	case HWCE_NEW_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectNewExplosion(index, message);
		break;
	case HWCE_MAGIC_MISSILE_EXPLOSION:
		ImmediateFree = !CLHW_ParseEffectMagicMissileExplosionWithSound(index, message);
		break;
	case HWCE_BONE_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectBoneExplosion(index, message);
		break;
	case HWCE_BLDRN_EXPL:
		ImmediateFree = !CLH2_ParseEffectBldrnExplosion(index, message);
		break;
	case HWCE_LSHOCK:
		ImmediateFree = !CLH2_ParseEffectLShock(index, message);
		break;
	case HWCE_ACID_HIT:
		ImmediateFree = !CLH2_ParseEffectAcidHit(index, message);
		break;
	case HWCE_ACID_SPLAT:
		ImmediateFree = !CLH2_ParseEffectAcidSplat(index, message);
		break;
	case HWCE_ACID_EXPL:
		ImmediateFree = !CLH2_ParseEffectAcidExplosion(index, message);
		break;
	case HWCE_LBALL_EXPL:
		ImmediateFree = !CLH2_ParseEffectLBallExplosion(index, message);
		break;
	case HWCE_FBOOM:
		ImmediateFree = !CLH2_ParseEffectFBoom(index, message);
		break;
	case HWCE_BOMB:
		ImmediateFree = !CLH2_ParseEffectBomb(index, message);
		break;
	case HWCE_FIREWALL_SMALL:
		ImmediateFree = !CLH2_ParseEffectFirewallSall(index, message);
		break;
	case HWCE_FIREWALL_MEDIUM:
		ImmediateFree = !CLH2_ParseEffectFirewallMedium(index, message);
		break;
	case HWCE_FIREWALL_LARGE:
		ImmediateFree = !CLH2_ParseEffectFirewallLarge(index, message);
		break;
	case HWCE_WHITE_FLASH:
		ImmediateFree = !CLH2_ParseEffectWhiteFlash(index, message);
		break;
	case HWCE_BLUE_FLASH:
		ImmediateFree = !CLH2_ParseEffectBlueFlash(index, message);
		break;
	case HWCE_SM_BLUE_FLASH:
		ImmediateFree = !CLH2_ParseEffectSmallBlueFlash(index, message);
		break;
	case HWCE_HWSPLITFLASH:
		ImmediateFree = !CLHW_ParseEffectSplitFlash(index, message);
		break;
	case HWCE_RED_FLASH:
		ImmediateFree = !CLH2_ParseEffectRedFlash(index, message);
		break;
	case HWCE_RIDER_DEATH:
		CLH2_ParseEffectRiderDeath(index, message);
		break;
	case HWCE_TELEPORTERPUFFS:
		CLH2_ParseEffectTeleporterPuffs(index, message);
		break;
	case HWCE_TELEPORTERBODY:
		CLH2_ParseEffectTeleporterBody(index, message);
		break;
	case HWCE_BONESHRAPNEL:
		ImmediateFree = !CLH2_ParseEffectBoneShrapnel(index, message);
		break;
	case HWCE_HWBONEBALL:
		ImmediateFree = !CLHW_ParseEffectBoneBall(index, message);
		break;
	case HWCE_BONESHARD:
		ImmediateFree = !CLHW_ParseEffectBoneShard(index, message);
		break;
	case HWCE_HWRAVENSTAFF:
		ImmediateFree = !CLHW_ParseEffectRavenStaff(index, message);
		break;
	case HWCE_HWRAVENPOWER:
		ImmediateFree = !CLHW_ParseEffectRavenPower(index, message);
		break;
	case HWCE_DEATHBUBBLES:
		CLHW_ParseEffectDeathBubbles(index, message);
		break;
	case HWCE_HWXBOWSHOOT:
		CLHW_ParseEffectXBowShoot(index, message);
		break;
	case HWCE_HWSHEEPINATOR:
		CLHW_ParseEffectSheepinator(index, message);
		break;
	case HWCE_HWDRILLA:
		ImmediateFree = !CLHW_ParseEffectDrilla(index, message);
		break;
	case HWCE_SCARABCHAIN:
		ImmediateFree = !CLHW_ParseEffectScarabChain(index, message);
		break;
	case HWCE_TRIPMINE:
		ImmediateFree = !CLHW_ParseEffectTripMine(index, message);
		break;
	case HWCE_TRIPMINESTILL:
		ImmediateFree = !CLHW_ParseEffectTripMineStill(index, message);
		break;
	case HWCE_HWMISSILESTAR:
		ImmediateFree = !CLHW_ParseEffectMissileStar(index, message);
		break;
	case HWCE_HWEIDOLONSTAR:
		ImmediateFree = !CLHW_ParseEffectEidolonStar(index, message);
		break;
	default:
		throw Exception("CL_ParseEffect: bad type");
	}
	return ImmediateFree;
}

void CLH2_ParseEffect(QMsg& message)
{
	int index = message.ReadByte();
	if (cl_common->h2_Effects[index].type)
	{
		CLH2_FreeEffect(index);
	}

	Com_Memset(&cl_common->h2_Effects[index], 0, sizeof(h2EffectT));

	cl_common->h2_Effects[index].type = message.ReadByte();

	bool ImmediateFree;
	if (!(GGameType & GAME_HexenWorld))
	{
		ImmediateFree = CLH2_ParseEffectType(index, message);
	}
	else
	{
		ImmediateFree = CLHW_ParseEffectType(index, message);
	}

	if (ImmediateFree)
	{
		cl_common->h2_Effects[index].type = HWCE_NONE;
	}
}

static void CLHW_ParseMultiEffectRavenPower(QMsg& message)
{
	vec3_t orig, vel;
	message.ReadPos(orig);
	message.ReadPos(vel);

	for (int count = 0; count < 3; count++)
	{
		int index = message.ReadByte();
		// create the effect
		cl_common->h2_Effects[index].type = HWCE_HWRAVENPOWER;
		VectorCopy(orig, cl_common->h2_Effects[index].Missile.origin);
		VectorCopy(vel, cl_common->h2_Effects[index].Missile.velocity);
		VecToAnglesBuggy(cl_common->h2_Effects[index].Missile.velocity, cl_common->h2_Effects[index].Missile.angle);
		if ((cl_common->h2_Effects[index].Missile.entity_index = CLH2_NewEffectEntity()) != -1)
		{
			effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Missile.entity_index];
			VectorCopy(cl_common->h2_Effects[index].Missile.origin, ent->state.origin);
			VectorCopy(cl_common->h2_Effects[index].Missile.angle, ent->state.angles);
			ent->model = R_RegisterModel("models/ravproj.mdl");
			S_StartSound(cl_common->h2_Effects[index].Missile.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_ravengo, 1, 1);
		}
	}
	CLHW_CreateRavenExplosions(orig);
}

// this creates multi effects from one packet
void CLHW_ParseMultiEffect(QMsg& message)
{
	int type = message.ReadByte();
	switch (type)
	{
	case HWCE_HWRAVENPOWER:
		CLHW_ParseMultiEffectRavenPower(message);
		break;
	default:
		throw Exception("CLHW_ParseMultiEffect: bad type");
	}
}

static void CLHW_XbowImpactPuff(const vec3_t origin, int material)//hopefully can use this with xbow & chain both
{
	int part_color;
	switch (material)
	{
	case H2XBOW_IMPACT_REDFLESH:
		part_color = 256 + 8 * 16 + rand() % 9;				//Blood red
		break;
	case H2XBOW_IMPACT_STONE:
		part_color = 256 + 20 + rand() % 8;			// Gray
		break;
	case H2XBOW_IMPACT_METAL:
		part_color = 256 + (8 * 15);			// Sparks
		break;
	case H2XBOW_IMPACT_WOOD:
		part_color = 256 + (5 * 16) + rand() % 8;			// Wood chunks
		break;
	case H2XBOW_IMPACT_ICE:
		part_color = 406 + rand() % 8;				// Ice particles
		break;
	case H2XBOW_IMPACT_GREENFLESH:
		part_color = 256 + 183 + rand() % 8;		// Spider's have green blood
		break;
	default:
		part_color = 256 + (3 * 16) + 4;		// Dust Brown
		break;
	}

	CLH2_RunParticleEffect4(origin, 24, part_color, pt_h2fastgrav, 20);
}

static void CLHW_ParseReviseEffectScarabChain(QMsg& message, int index)
{
	//attach to new guy or retract if new guy is world
	int curEnt = message.ReadShort();

	cl_common->h2_Effects[index].Chain.material = curEnt >> 12;
	curEnt &= 0x0fff;

	if (curEnt)
	{
		cl_common->h2_Effects[index].Chain.state = 1;
		cl_common->h2_Effects[index].Chain.owner = curEnt;
		h2entity_state_t* es = CLH2_FindState(cl_common->h2_Effects[index].Chain.owner);
		if (es)
		{
			effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Chain.ent1];
			CLHW_XbowImpactPuff(ent->state.origin, cl_common->h2_Effects[index].Chain.material);
		}
	}
	else
	{
		cl_common->h2_Effects[index].Chain.state = 2;
	}
}

static void CLHW_ParseDirectionChangeAngles(QMsg& message, vec3_t angles)
{
	angles[0] = message.ReadAngle();
	if (angles[0] < 0)
	{
		angles[0] += 360;
	}
	angles[0] *= -1;
	angles[1] = message.ReadAngle();
	if (angles[1] < 0)
	{
		angles[1] += 360;
	}
	angles[2] = 0;
}

static void CLHW_UpdateDirectionChangeVelocity(const vec3_t angles, vec3_t velocity)
{
	vec3_t forward, right, up;
	AngleVectors(angles, forward, right, up);
	float speed = VectorLength(velocity);
	VectorScale(forward, speed, velocity);
}

static void CLHW_ParseReviseEffectXBowDirectionChange(QMsg& message, int index, int revisionCode, int curEnt)
{
	vec3_t angles;
	CLHW_ParseDirectionChangeAngles(message, angles);

	if (revisionCode & 128)//new origin
	{
		message.ReadPos(cl_common->h2_Effects[index].Xbow.origin[curEnt]);
	}

	CLHW_UpdateDirectionChangeVelocity(angles, cl_common->h2_Effects[index].Xbow.vel[curEnt]);

	if (cl_common->h2_Effects[index].Xbow.ent[curEnt] != -1)
	{
		effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Xbow.ent[curEnt]];
		VectorCopy(angles, ent->state.angles);
		VectorCopy(cl_common->h2_Effects[index].Xbow.origin[curEnt], ent->state.origin);
	}
}

static void CLHW_ParseReviseEffectXBowShoot(QMsg& message, int index)
{
	int revisionCode = message.ReadByte();
	//this is one packed byte!
	//highest bit: for impact revision, indicates whether damage is done
	//				for redirect revision, indicates whether new origin was sent
	//next 3 high bits: for all revisions, indicates which bolt is to be revised
	//highest 3 of the low 4 bits: for impact revision, indicates the material that was hit
	//lowest bit: indicates whether revision is of impact or redirect variety


	int curEnt = (revisionCode >> 4) & 7;
	if (revisionCode & 1)//impact effect: 
	{
		cl_common->h2_Effects[index].Xbow.activebolts &= ~(1 << curEnt);
		float dist = message.ReadCoord();
		if (cl_common->h2_Effects[index].Xbow.ent[curEnt]!= -1)
		{
			effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Xbow.ent[curEnt]];

			//make sure bolt is in correct position
			vec3_t forward;
			VectorCopy(cl_common->h2_Effects[index].Xbow.vel[curEnt], forward);
			VectorNormalize(forward);
			VectorScale(forward, dist, forward);
			VectorAdd(cl_common->h2_Effects[index].Xbow.origin[curEnt], forward, ent->state.origin);

			int material = (revisionCode & 14);
			int takedamage = (revisionCode & 128);

			if (takedamage)
			{
				cl_common->h2_Effects[index].Xbow.gonetime[curEnt] = cl_common->serverTime / 1000.0;
			}
			else
			{
				cl_common->h2_Effects[index].Xbow.gonetime[curEnt] += cl_common->serverTime / 1000.0;
			}
			
			VectorCopy(cl_common->h2_Effects[index].Xbow.vel[curEnt], forward);
			VectorNormalize(forward);
			VectorScale(forward, 8, forward);

			// do a particle effect here, with the color depending on chType
			CLHW_XbowImpactPuff(ent->state.origin, material);

			// impact sound:
			switch (material)
			{
			case H2XBOW_IMPACT_GREENFLESH:
			case H2XBOW_IMPACT_REDFLESH:
			case H2XBOW_IMPACT_MUMMY:
				S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 0, clh2_fxsfx_arr2flsh, 1, 1);
				break;
			case H2XBOW_IMPACT_WOOD:
				S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 0, clh2_fxsfx_arr2wood, 1, 1);
				break;
			default:
				S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 0, clh2_fxsfx_met2stn, 1, 1);
				break;
			}

			CLHW_XbowImpact(ent->state.origin, forward, material, takedamage, cl_common->h2_Effects[index].Xbow.bolts);
		}
	}
	else
	{
		CLHW_ParseReviseEffectXBowDirectionChange(message, index, revisionCode, curEnt);
	}
}

static void CLHW_ParseReviseEffectSheepinator(QMsg& message, int index)
{
	int revisionCode = message.ReadByte();
	int curEnt = (revisionCode >> 4) & 7;
	if (revisionCode & 1)//impact
	{
		float dist = message.ReadCoord();
		cl_common->h2_Effects[index].Xbow.activebolts &= ~(1<<curEnt);
		if (cl_common->h2_Effects[index].Xbow.ent[curEnt] != -1)
		{
			effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Xbow.ent[curEnt]];

			//make sure bolt is in correct position
			vec3_t forward;
			VectorCopy(cl_common->h2_Effects[index].Xbow.vel[curEnt],forward);
			VectorNormalize(forward);
			VectorScale(forward,dist,forward);
			VectorAdd(cl_common->h2_Effects[index].Xbow.origin[curEnt],forward,ent->state.origin);
			CLH2_ColouredParticleExplosion(ent->state.origin,(rand()%16)+144/*(144,159)*/,20,30);
		}
	}
	else
	{
		CLHW_ParseReviseEffectXBowDirectionChange(message, index, revisionCode, curEnt);
	}
}

static void CLHW_ParseReviseEffectDrilla(QMsg& message, int index)
{
	int revisionCode = message.ReadByte();
	if (revisionCode == 0)//impact
	{
		vec3_t pos;
		message.ReadPos(pos);
		int material = message.ReadByte();

		//throw lil bits of victim at entry
		CLHW_XbowImpactPuff(pos, material);

		if ((material == H2XBOW_IMPACT_GREENFLESH) || (material == H2XBOW_IMPACT_GREENFLESH))
		{
			//meaty sound and some chunks too
			S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_fxsfx_drillameat, 1, 1);
			
			//todo: the chunks
		}

		//lil bits at exit
		vec3_t forward;
		VectorCopy(cl_common->h2_Effects[index].Missile.velocity, forward);
		VectorNormalize(forward);
		VectorScale(forward, 36, forward);
		VectorAdd(forward, pos, pos);
		CLHW_XbowImpactPuff(pos, material);
	}
	else//turn
	{
		vec3_t angles;
		CLHW_ParseDirectionChangeAngles(message, angles);

		message.ReadPos(cl_common->h2_Effects[index].Missile.origin);

		CLHW_UpdateDirectionChangeVelocity(angles, cl_common->h2_Effects[index].Missile.velocity);

		if (cl_common->h2_Effects[index].Missile.entity_index != -1)
		{
			effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Missile.entity_index];
			VectorCopy(angles, ent->state.angles);
			VectorCopy(cl_common->h2_Effects[index].Missile.origin, ent->state.origin);
		}
	}
}

static void CLHW_ParseReviseEffectType(QMsg& message, int index)
{
	switch (cl_common->h2_Effects[index].type)
	{
	case HWCE_SCARABCHAIN:
		CLHW_ParseReviseEffectScarabChain(message, index);
		break;
	case HWCE_HWXBOWSHOOT:
		CLHW_ParseReviseEffectXBowShoot(message, index);
		break;
	case HWCE_HWSHEEPINATOR:
		CLHW_ParseReviseEffectSheepinator(message, index);
		break;
	case HWCE_HWDRILLA:
		CLHW_ParseReviseEffectDrilla(message, index);
		break;
	}
}

static void CLHW_SkipReviseEffectScarabChain(QMsg& message)
{
	message.ReadShort();
}

static void CLHW_SkipReviseEffectXBowShoot(QMsg& message)
{
	int revisionCode = message.ReadByte();
	if (revisionCode & 1)//impact effect: 
	{
		message.ReadCoord();
	}
	else
	{
		message.ReadAngle();
		message.ReadAngle();
		if (revisionCode & 128)//new origin
		{
			message.ReadCoord();
			message.ReadCoord();
			message.ReadCoord();
		}
	}
}

static void CLHW_SkipReviseEffectSheepinator(QMsg& message)
{
	int revisionCode = message.ReadByte();
	if (revisionCode & 1)//impact
	{
		message.ReadCoord();
	}
	else//direction change
	{
		message.ReadAngle();
		message.ReadAngle();
		if (revisionCode & 128)//new origin
		{
			message.ReadCoord();
			message.ReadCoord();
			message.ReadCoord();
		}
	}
}

static void CLHW_SkipReviseEffectDrilla(QMsg& message)
{
	int revisionCode = message.ReadByte();
	if (revisionCode == 0)//impact
	{
		message.ReadCoord();
		message.ReadCoord();
		message.ReadCoord();
		message.ReadByte();
	}
	else//turn
	{
		message.ReadAngle();
		message.ReadAngle();
		message.ReadCoord();
		message.ReadCoord();
		message.ReadCoord();
	}
}

static void CLHW_SkipReviseEffectType(QMsg& message, int type)
{
	switch (type)
	{
	case HWCE_SCARABCHAIN:
		CLHW_SkipReviseEffectScarabChain(message);
		break;
	case HWCE_HWXBOWSHOOT:
		CLHW_SkipReviseEffectXBowShoot(message);
		break;
	case HWCE_HWSHEEPINATOR:
		CLHW_SkipReviseEffectSheepinator(message);
		break;
	case HWCE_HWDRILLA:
		CLHW_SkipReviseEffectDrilla(message);
		break;
	}
}

// be sure to read, in the switch statement, everything
// in this message, even if the effect is not the right kind or invalid,
// or else client is sure to crash.
void CLHW_ParseReviseEffect(QMsg& message)
{
	int index = message.ReadByte();
	int type = message.ReadByte();

	if (cl_common->h2_Effects[index].type == type)
	{
		CLHW_ParseReviseEffectType(message, index);
	}
	else
	{
		CLHW_SkipReviseEffectType(message, type);
	}
}

static void CLHW_TurnEffectMissile(int index, float time, vec3_t position, vec3_t velocity)
{
	if (cl_common->h2_Effects[index].Missile.entity_index == -1)
	{
		return;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Missile.entity_index];
	VectorCopy(position, ent->state.origin);
	VectorCopy(velocity, cl_common->h2_Effects[index].Missile.velocity);
	VecToAnglesBuggy(cl_common->h2_Effects[index].Missile.velocity, cl_common->h2_Effects[index].Missile.angle);
}

static void CLHW_TurnEffectStar(int index, float time, vec3_t position, vec3_t velocity)
{
	if (cl_common->h2_Effects[index].Star.entity_index == -1)
	{
		return;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Star.entity_index];
	VectorCopy(position, ent->state.origin);
	VectorCopy(velocity, cl_common->h2_Effects[index].Star.velocity);
}

void CLHW_ParseTurnEffect(QMsg& message)
{
	int index = message.ReadByte();
	float time = message.ReadFloat();
	vec3_t pos, vel;
	message.ReadPos(pos);
	message.ReadPos(vel);

	switch (cl_common->h2_Effects[index].type)
	{
	case HWCE_HWRAVENSTAFF:
	case HWCE_HWRAVENPOWER:
	case HWCE_BONESHARD:
	case HWCE_BONESHRAPNEL:
	case HWCE_HWBONEBALL:
		CLHW_TurnEffectMissile(index, time, pos, vel);
		break;
	case HWCE_HWMISSILESTAR:
	case HWCE_HWEIDOLONSTAR:
		CLHW_TurnEffectStar(index, time, pos, vel);
		break;
	case 0:
		break;
	default:
		Log::write("CLHW_ParseTurnEffect: bad type %d\n", cl_common->h2_Effects[index].type);
		break;
	}
}

static void CLHW_EndEffectRavenPower(int index)
{
	if (cl_common->h2_Effects[index].Missile.entity_index == -1)
	{
		return;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Missile.entity_index];
	CLHW_CreateRavenDeath(ent->state.origin);
}

static void CLHW_EndEffectRavenStaff(int index)
{
	if (cl_common->h2_Effects[index].Missile.entity_index == -1)
	{
		return;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Missile.entity_index];
	CLHW_CreateExplosionWithSound(ent->state.origin);
}

void CLH2_ParseEndEffect(QMsg& message)
{
	int index = message.ReadByte();

	if (GGameType & GAME_HexenWorld)
	{
		switch (cl_common->h2_Effects[index].type)
		{
		case HWCE_HWRAVENPOWER:
			CLHW_EndEffectRavenPower(index);
			break;
		case HWCE_HWRAVENSTAFF:
			CLHW_EndEffectRavenStaff(index);
			break;
		}
	}
	CLH2_FreeEffect(index);
}

static void CLH2_LinkEffectEntity(effect_entity_t* entity)
{
	refEntity_t refEntity;
	Com_Memset(&refEntity, 0, sizeof(refEntity));
	refEntity.reType = RT_MODEL;
	VectorCopy(entity->state.origin, refEntity.origin);
	refEntity.hModel = entity->model;
	refEntity.frame = entity->state.frame;
	refEntity.skinNum = entity->state.skinnum;
	CLH2_SetRefEntAxis(&refEntity, entity->state.angles, vec3_origin,
		entity->state.scale, 0, entity->state.abslight, entity->state.drawflags);
	CLH2_HandleCustomSkin(&refEntity, -1);
	R_AddRefEntityToScene(&refEntity);
}

static void CLH2_UpdateEffectRain(int index, float frametime)
{
	cl_common->h2_Effects[index].Rain.next_time += frametime;
	if (cl_common->h2_Effects[index].Rain.next_time < cl_common->h2_Effects[index].Rain.wait)
	{
		return;
	}

	CLH2_RainEffect(cl_common->h2_Effects[index].Rain.min_org, cl_common->h2_Effects[index].Rain.e_size,
		cl_common->h2_Effects[index].Rain.dir[0], cl_common->h2_Effects[index].Rain.dir[1],
		cl_common->h2_Effects[index].Rain.color, cl_common->h2_Effects[index].Rain.count);
	cl_common->h2_Effects[index].Rain.next_time = 0;
}

static void CLH2_UpdateEffectSnow(int index, float frametime)
{
	cl_common->h2_Effects[index].Snow.next_time += frametime;
	if (cl_common->h2_Effects[index].Snow.next_time < 0.10)
	{
		return;
	}

	vec3_t org, org2, alldir;
	VectorCopy(cl_common->h2_Effects[index].Snow.min_org, org);
	VectorCopy(cl_common->h2_Effects[index].Snow.max_org, org2);
	VectorCopy(cl_common->h2_Effects[index].Snow.dir, alldir);

	vec3_t snow_org;
	VectorAdd(org, org2, snow_org);

	snow_org[0] *= 0.5;
	snow_org[1] *= 0.5;
	snow_org[2] *= 0.5;

	snow_org[2] = cl_common->refdef.vieworg[2];

	VectorSubtract(snow_org, cl_common->refdef.vieworg, snow_org);

	float distance = VectorNormalize(snow_org);
	if (distance >= 1024)
	{
		return;
	}
	CLH2_SnowEffect(org, org2, cl_common->h2_Effects[index].Snow.flags, alldir,
		cl_common->h2_Effects[index].Snow.count);

	cl_common->h2_Effects[index].Snow.next_time = 0;
}

static void CLH2_UpdateEffectFountain(int index)
{
	vec3_t mymin;
	mymin[0] = (-3 * cl_common->h2_Effects[index].Fountain.vright[0] * cl_common->h2_Effects[index].Fountain.movedir[0]) +
				(-3 * cl_common->h2_Effects[index].Fountain.vforward[0] * cl_common->h2_Effects[index].Fountain.movedir[1]) +
				(2 * cl_common->h2_Effects[index].Fountain.vup[0] * cl_common->h2_Effects[index].Fountain.movedir[2]);
	mymin[1] = (-3 * cl_common->h2_Effects[index].Fountain.vright[1] * cl_common->h2_Effects[index].Fountain.movedir[0]) +
				(-3 * cl_common->h2_Effects[index].Fountain.vforward[1] * cl_common->h2_Effects[index].Fountain.movedir[1]) +
				(2 * cl_common->h2_Effects[index].Fountain.vup[1] * cl_common->h2_Effects[index].Fountain.movedir[2]);
	mymin[2] = (-3 * cl_common->h2_Effects[index].Fountain.vright[2] * cl_common->h2_Effects[index].Fountain.movedir[0]) +
				(-3 * cl_common->h2_Effects[index].Fountain.vforward[2] * cl_common->h2_Effects[index].Fountain.movedir[1]) +
				(2 * cl_common->h2_Effects[index].Fountain.vup[2] * cl_common->h2_Effects[index].Fountain.movedir[2]);
	mymin[0] *= 15;
	mymin[1] *= 15;
	mymin[2] *= 15;

	vec3_t mymax;
	mymax[0] = (3 * cl_common->h2_Effects[index].Fountain.vright[0] * cl_common->h2_Effects[index].Fountain.movedir[0]) +
				(3 * cl_common->h2_Effects[index].Fountain.vforward[0] * cl_common->h2_Effects[index].Fountain.movedir[1]) +
				(10 * cl_common->h2_Effects[index].Fountain.vup[0] * cl_common->h2_Effects[index].Fountain.movedir[2]);
	mymax[1] = (3 * cl_common->h2_Effects[index].Fountain.vright[1] * cl_common->h2_Effects[index].Fountain.movedir[0]) +
				(3 * cl_common->h2_Effects[index].Fountain.vforward[1] * cl_common->h2_Effects[index].Fountain.movedir[1]) +
				(10 * cl_common->h2_Effects[index].Fountain.vup[1] * cl_common->h2_Effects[index].Fountain.movedir[2]);
	mymax[2] = (3 * cl_common->h2_Effects[index].Fountain.vright[2] * cl_common->h2_Effects[index].Fountain.movedir[0]) +
				(3 * cl_common->h2_Effects[index].Fountain.vforward[2] * cl_common->h2_Effects[index].Fountain.movedir[1]) +
				(10 * cl_common->h2_Effects[index].Fountain.vup[2] * cl_common->h2_Effects[index].Fountain.movedir[2]);
	mymax[0] *= 15;
	mymax[1] *= 15;
	mymax[2] *= 15;

	CLH2_RunParticleEffect2(cl_common->h2_Effects[index].Fountain.pos, mymin, mymax,
		cl_common->h2_Effects[index].Fountain.color, pt_h2fastgrav, cl_common->h2_Effects[index].Fountain.cnt);
}

static void CLH2_UpdateEffectQuake(int index)
{
	CLH2_RunQuakeEffect(cl_common->h2_Effects[index].Quake.origin, cl_common->h2_Effects[index].Quake.radius);
}

static float CLH2_UpdateEffectSmokeTimeAmount(int index)
{
	float smoketime = cl_common->h2_Effects[index].Smoke.framelength;
	if (!smoketime)
	{
		smoketime = HX_FRAME_TIME;
	}
	return smoketime;
}

static int CLH2_UpdateEffectSmokeTimeAmount(int index, float frametime, float smoketime)
{
	cl_common->h2_Effects[index].Smoke.time_amount += frametime;

	int i = 0;
	while (cl_common->h2_Effects[index].Smoke.time_amount >= smoketime)
	{
		i++;
		cl_common->h2_Effects[index].Smoke.time_amount -= smoketime;
	}
	return i;
}

static bool CLH2_UpdateEffectSmokeEntity(int index, int entity_index, float frametime, float smoketime, int numFramesPassed)
{
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];
	ent->state.origin[0] += (frametime / smoketime) * cl_common->h2_Effects[index].Smoke.velocity[0];
	ent->state.origin[1] += (frametime / smoketime) * cl_common->h2_Effects[index].Smoke.velocity[1];
	ent->state.origin[2] += (frametime / smoketime) * cl_common->h2_Effects[index].Smoke.velocity[2];

	ent->state.frame += numFramesPassed;
	if (ent->state.frame >= R_ModelNumFrames(ent->model))
	{
		return false;
	}
	CLH2_LinkEffectEntity(ent);
	return true;
}

static int CLH2_UpdateEffectSmokeCommon(int index, float frametime, float& smoketime)
{
	smoketime = CLH2_UpdateEffectSmokeTimeAmount(index);
	int i = CLH2_UpdateEffectSmokeTimeAmount(index, frametime, smoketime);
	if (!CLH2_UpdateEffectSmokeEntity(index, cl_common->h2_Effects[index].Smoke.entity_index, frametime, smoketime, i))
	{
		CLH2_FreeEffect(index);
	}
	return i;
}

static void CLH2_UpdateEffectSmoke(int index, float frametime)
{
	float smoketime;
	CLH2_UpdateEffectSmokeCommon(index, frametime, smoketime);
}

static void CLHW_UpdateEffectTeleportSmoke1(int index, float frametime)
{
	float smoketime;
	int i = CLH2_UpdateEffectSmokeCommon(index, frametime, smoketime);
	CLH2_UpdateEffectSmokeEntity(index, cl_common->h2_Effects[index].Smoke.entity_index2, frametime, smoketime, i);
}

static void CLHW_UpdateEffectRipple(int index, float frametime)
{
	cl_common->h2_Effects[index].Smoke.time_amount += frametime;
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];

	float smoketime = cl_common->h2_Effects[index].Smoke.framelength;
	if (!smoketime)
	{
		smoketime = HX_FRAME_TIME * 2;
	}

	while (cl_common->h2_Effects[index].Smoke.time_amount >= smoketime && ent->state.scale < 250)
	{
		ent->state.frame++;
		ent->state.angles[1] += 1;
		cl_common->h2_Effects[index].Smoke.time_amount -= smoketime;
	}

	if (ent->state.frame >= 10)
	{
		CLH2_FreeEffect(index);
	}
	else
	{
		CLH2_LinkEffectEntity(ent);
	}
}

static void CLH2_UpdateEffectExplosionCommon(int index, float frametime, float frameDuration)
{
	// Just go through animation and then remove
	cl_common->h2_Effects[index].Smoke.time_amount += frametime;
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];

	while (cl_common->h2_Effects[index].Smoke.time_amount >= frameDuration)
	{
		ent->state.frame++;
		cl_common->h2_Effects[index].Smoke.time_amount -= frameDuration;
	}

	if (ent->state.frame >= R_ModelNumFrames(ent->model))
	{
		CLH2_FreeEffect(index);
	}
	else
	{
		CLH2_LinkEffectEntity(ent);
	}
}

static void CLH2_UpdateEffectExplosion(int index, float frametime)
{
	CLH2_UpdateEffectExplosionCommon(index, frametime, HX_FRAME_TIME);
}

static void CLH2_UpdateEffectBigCircleExplosion(int index, float frametime)
{
	CLH2_UpdateEffectExplosionCommon(index, frametime, HX_FRAME_TIME * 2);
}

static void CLH2_UpdateEffectLShock(int index)
{
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];
	if (ent->state.skinnum == 0)
	{
		ent->state.skinnum = 1;
	}
	else if (ent->state.skinnum == 1)
	{
		ent->state.skinnum = 0;
	}
	ent->state.scale -= 10;
	if (ent->state.scale <= 10)
	{
		CLH2_FreeEffect(index);
	}
	else
	{
		CLH2_LinkEffectEntity(ent);
	}
}

static void CLH2_UpdateEffectFlash(int index, float frametime)
{
	// Go forward then backward through animation then remove
	cl_common->h2_Effects[index].Flash.time_amount += frametime;
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Flash.entity_index];

	while (cl_common->h2_Effects[index].Flash.time_amount >= HX_FRAME_TIME)
	{
		if (!cl_common->h2_Effects[index].Flash.reverse)
		{
			if (ent->state.frame >= R_ModelNumFrames(ent->model) - 1)  // Ran through forward animation
			{
				cl_common->h2_Effects[index].Flash.reverse = 1;
				ent->state.frame--;
			}
			else
			{
				ent->state.frame++;
			}
		}	
		else
		{
			ent->state.frame--;
		}
		cl_common->h2_Effects[index].Flash.time_amount -= HX_FRAME_TIME;
	}

	if (ent->state.frame <= 0 && cl_common->h2_Effects[index].Flash.reverse)
	{
		CLH2_FreeEffect(index);
	}
	else
	{
		CLH2_LinkEffectEntity(ent);
	}
}

static void CLH2_UpdateEffectGravityWell(int index, float frametime)
{
	cl_common->h2_Effects[index].GravityWell.time_amount += frametime * 2;
	if (cl_common->h2_Effects[index].GravityWell.time_amount >= 1)
	{
		cl_common->h2_Effects[index].GravityWell.time_amount -= 1;
	}

	vec3_t org;
	VectorCopy(cl_common->h2_Effects[index].GravityWell.origin, org);
	org[0] += sin(cl_common->h2_Effects[index].GravityWell.time_amount * 2 * M_PI) * 30;
	org[1] += cos(cl_common->h2_Effects[index].GravityWell.time_amount * 2 * M_PI) * 30;

	if (cl_common->h2_Effects[index].GravityWell.lifetime < cl_common->serverTime * 0.001)
	{
		CLH2_FreeEffect(index);
	}
	else
	{
		CLH2_GravityWellParticle(rand() % 8, org, cl_common->h2_Effects[index].GravityWell.color);
	}
}

static void CLH2_UpdateEffectTeleporterPuffs(int index, float frametime)
{
	cl_common->h2_Effects[index].Teleporter.time_amount += frametime;
	float smoketime = cl_common->h2_Effects[index].Teleporter.framelength;

	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Teleporter.entity_index[0]];
	while (cl_common->h2_Effects[index].Teleporter.time_amount >= HX_FRAME_TIME)
	{
		ent->state.frame++;
		cl_common->h2_Effects[index].Teleporter.time_amount -= HX_FRAME_TIME;
	}
	int cur_frame = ent->state.frame;

	if (cur_frame >= R_ModelNumFrames(ent->model))
	{
		CLH2_FreeEffect(index);
		return;
	}

	for (int i = 0; i < 8; i++)
	{
		ent = &EffectEntities[cl_common->h2_Effects[index].Teleporter.entity_index[i]];

		ent->state.origin[0] += (frametime / smoketime) * cl_common->h2_Effects[index].Teleporter.velocity[i][0];
		ent->state.origin[1] += (frametime / smoketime) * cl_common->h2_Effects[index].Teleporter.velocity[i][1];
		ent->state.origin[2] += (frametime / smoketime) * cl_common->h2_Effects[index].Teleporter.velocity[i][2];
		ent->state.frame = cur_frame;

		CLH2_LinkEffectEntity(ent);
	}
}

static void CLH2_UpdateEffectTeleporterBody(int index, float frametime)
{
	cl_common->h2_Effects[index].Teleporter.time_amount += frametime;

	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Teleporter.entity_index[0]];
	while (cl_common->h2_Effects[index].Teleporter.time_amount >= HX_FRAME_TIME)
	{
		ent->state.scale -= 15;
		cl_common->h2_Effects[index].Teleporter.time_amount -= HX_FRAME_TIME;
	}

	ent = &EffectEntities[cl_common->h2_Effects[index].Teleporter.entity_index[0]];
	ent->state.angles[1] += 45;

	if (ent->state.scale <= 10)
	{
		CLH2_FreeEffect(index);
	}
	else
	{
		CLH2_LinkEffectEntity(ent);
	}
}

static effect_entity_t* CLH2_UpdateEffectMissileCommon(int index, float frametime)
{
	cl_common->h2_Effects[index].Missile.time_amount += frametime;
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Missile.entity_index];

	ent->state.angles[0] += frametime * cl_common->h2_Effects[index].Missile.avelocity[0];
	ent->state.angles[1] += frametime * cl_common->h2_Effects[index].Missile.avelocity[1];
	ent->state.angles[2] += frametime * cl_common->h2_Effects[index].Missile.avelocity[2];

	ent->state.origin[0] += frametime * cl_common->h2_Effects[index].Missile.velocity[0];
	ent->state.origin[1] += frametime * cl_common->h2_Effects[index].Missile.velocity[1];
	ent->state.origin[2] += frametime * cl_common->h2_Effects[index].Missile.velocity[2];
	return ent;
}

static void CLH2_UpdateEffectMissile(int index, float frametime)
{
	effect_entity_t* ent = CLH2_UpdateEffectMissileCommon(index, frametime);
	CLH2_LinkEffectEntity(ent);
}

static void CLHW_UpdateEffectBoneBall(int index, float frametime)
{
	effect_entity_t* ent = CLH2_UpdateEffectMissileCommon(index, frametime);
	CLH2_LinkEffectEntity(ent);
	CLH2_RunParticleEffect4(ent->state.origin, 10, 368 + rand() % 16, pt_h2slowgrav, 3);
}

static void CLHW_UpdateEffectRavenPower(int index, float frametime)
{
	effect_entity_t* ent = CLH2_UpdateEffectMissileCommon(index, frametime);
	while (cl_common->h2_Effects[index].Missile.time_amount >= HX_FRAME_TIME)
	{
		ent->state.frame++;
		cl_common->h2_Effects[index].Missile.time_amount -= HX_FRAME_TIME;
	}

	if (ent->state.frame > 7)
	{
		ent->state.frame = 0;
		return;
	}
	CLH2_LinkEffectEntity(ent);
}

static void CLHW_UpdateEffectDrilla(int index, float frametime)
{
	cl_common->h2_Effects[index].Missile.time_amount += frametime;
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Missile.entity_index];

	S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_drillaspin, 1, 1);

	ent->state.angles[0] += frametime * cl_common->h2_Effects[index].Missile.avelocity[0];
	ent->state.angles[1] += frametime * cl_common->h2_Effects[index].Missile.avelocity[1];
	ent->state.angles[2] += frametime * cl_common->h2_Effects[index].Missile.avelocity[2];

	vec3_t old_origin;
	VectorCopy(ent->state.origin, old_origin);

	ent->state.origin[0] += frametime * cl_common->h2_Effects[index].Missile.velocity[0];
	ent->state.origin[1] += frametime * cl_common->h2_Effects[index].Missile.velocity[1];
	ent->state.origin[2] += frametime * cl_common->h2_Effects[index].Missile.velocity[2];

	CLH2_TrailParticles(old_origin, ent->state.origin, rt_setstaff);

	CLH2_LinkEffectEntity(ent);
}

static void CLHW_UpdateEffectXBowShot(int index, float frametime)
{
	cl_common->h2_Effects[index].Xbow.time_amount += frametime;
	for (int i = 0; i < cl_common->h2_Effects[index].Xbow.bolts; i++)
	{
		if (cl_common->h2_Effects[index].Xbow.ent[i] == -1)//only update valid effect ents
		{
			continue;
		}
		if (cl_common->h2_Effects[index].Xbow.activebolts & (1 << i))//bolt in air, simply update position
		{
			effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Xbow.ent[i]];

			ent->state.origin[0] += frametime * cl_common->h2_Effects[index].Xbow.vel[i][0];
			ent->state.origin[1] += frametime * cl_common->h2_Effects[index].Xbow.vel[i][1];
			ent->state.origin[2] += frametime * cl_common->h2_Effects[index].Xbow.vel[i][2];

			CLH2_LinkEffectEntity(ent);
		}
		else if (cl_common->h2_Effects[index].Xbow.bolts == 5)//fiery bolts don't just go away
		{
			if (cl_common->h2_Effects[index].Xbow.state[i] == 0)//waiting to explode state
			{
				if (cl_common->h2_Effects[index].Xbow.gonetime[i] > cl_common->serverTime * 0.001)//fiery bolts stick around for a while
				{
					effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Xbow.ent[i]];
					CLH2_LinkEffectEntity(ent);
				}
				else//when time's up on fiery guys, they explode
				{
					//set state to exploding
					cl_common->h2_Effects[index].Xbow.state[i] = 1;

					effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Xbow.ent[i]];

					//move bolt back a little to make explosion look better
					VectorNormalize(cl_common->h2_Effects[index].Xbow.vel[i]);
					VectorScale(cl_common->h2_Effects[index].Xbow.vel[i],-8,cl_common->h2_Effects[index].Xbow.vel[i]);
					VectorAdd(ent->state.origin,cl_common->h2_Effects[index].Xbow.vel[i],ent->state.origin);

					//turn bolt entity into an explosion
					ent->model = R_RegisterModel("models/xbowexpl.spr");
					ent->state.frame = 0;

					//set frame change counter
					cl_common->h2_Effects[index].Xbow.gonetime[i] = cl_common->serverTime * 0.001 + HX_FRAME_TIME * 2;

					//play explosion sound
					S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_explode, 1, 1);

					CLH2_LinkEffectEntity(ent);
				}
			}
			else if (cl_common->h2_Effects[index].Xbow.state[i] == 1)//fiery bolt exploding state
			{
				effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Xbow.ent[i]];

				//increment frame if it's time
				while (cl_common->h2_Effects[index].Xbow.gonetime[i] <= cl_common->serverTime * 0.001)
				{
					ent->state.frame++;
					cl_common->h2_Effects[index].Xbow.gonetime[i] += HX_FRAME_TIME * 0.75;
				}


				if (ent->state.frame >= R_ModelNumFrames(ent->model))
				{
					cl_common->h2_Effects[index].Xbow.state[i] = 2;//if anim is over, set me to inactive state
				}
				else
				{
					CLH2_LinkEffectEntity(ent);
				}
			}
		}
	}
}

static void CLHW_UpdateEffectSheepinator(int index, float frametime)
{
	cl_common->h2_Effects[index].Xbow.time_amount += frametime;
	for (int i = 0; i < cl_common->h2_Effects[index].Xbow.bolts; i++)
	{
		if (cl_common->h2_Effects[index].Xbow.ent[i] == -1)//only update valid effect ents
		{
			continue;
		}
		if (cl_common->h2_Effects[index].Xbow.activebolts & (1 << i))//bolt in air, simply update position
		{
			effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Xbow.ent[i]];

			ent->state.origin[0] += frametime * cl_common->h2_Effects[index].Xbow.vel[i][0];
			ent->state.origin[1] += frametime * cl_common->h2_Effects[index].Xbow.vel[i][1];
			ent->state.origin[2] += frametime * cl_common->h2_Effects[index].Xbow.vel[i][2];

			CLH2_RunParticleEffect4(ent->state.origin, 7, (rand() % 15) + 144, pt_h2explode2, (rand() %5) + 1);

			CLH2_LinkEffectEntity(ent);
		}
	}
}

static void CLHW_UpdateEffectDeathBubbles(int index, float frametime)
{
	cl_common->h2_Effects[index].Bubble.time_amount += frametime;
	if (cl_common->h2_Effects[index].Bubble.time_amount > 0.1)//10 bubbles a sec
	{
		cl_common->h2_Effects[index].Bubble.time_amount = 0;
		cl_common->h2_Effects[index].Bubble.count--;
		h2entity_state_t* es = CLH2_FindState(cl_common->h2_Effects[index].Bubble.owner);
		if (es)
		{
			vec3_t org;
			VectorCopy(es->origin, org);
			VectorAdd(org, cl_common->h2_Effects[index].Bubble.offset, org);

			if (CM_PointContentsQ1(org, 0) != BSP29CONTENTS_WATER) 
			{
				//not in water anymore
				CLH2_FreeEffect(index);
				return;
			}
			else
			{
				CLHW_SpawnDeathBubble(org);
			}
		}
	}
	if (cl_common->h2_Effects[index].Bubble.count <= 0)
	{
		CLH2_FreeEffect(index);
	}
}

static void CLHW_UpdateEffectScarabChain(int index, float frametime)
{
	cl_common->h2_Effects[index].Chain.time_amount += frametime;
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Chain.ent1];

	h2entity_state_t* es;
	switch (cl_common->h2_Effects[index].Chain.state)
	{
	case 0://zooming in toward owner
		es = CLH2_FindState(cl_common->h2_Effects[index].Chain.owner);
		if (cl_common->h2_Effects[index].Chain.sound_time * 1000 <= cl_common->serverTime)
		{
			cl_common->h2_Effects[index].Chain.sound_time = cl_common->serverTime * 0.001 + 0.5;
			S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_scarabhome, 1, 1);
		}
		if (es)
		{
			vec3_t org;
			VectorCopy(es->origin,org);
			org[2] += cl_common->h2_Effects[index].Chain.height;
			VectorSubtract(org, ent->state.origin, org);
			if (fabs(VectorNormalize(org)) < 500 * frametime)
			{
				S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_scarabgrab, 1, 1);
				cl_common->h2_Effects[index].Chain.state = 1;
				VectorCopy(es->origin, ent->state.origin);
				ent->state.origin[2] += cl_common->h2_Effects[index].Chain.height;
				CLHW_XbowImpactPuff(ent->state.origin, cl_common->h2_Effects[index].Chain.material);
			}
			else
			{
				VectorScale(org, 500 * frametime, org);
				VectorAdd(ent->state.origin, org, ent->state.origin);
			}
		}
		break;
	case 1://attached--snap to owner's pos
		es = CLH2_FindState(cl_common->h2_Effects[index].Chain.owner);
		if (es)
		{
			VectorCopy(es->origin, ent->state.origin);
			ent->state.origin[2] += cl_common->h2_Effects[index].Chain.height;
		}
		break;
	case 2://unattaching, server needs to set this state
		{
		vec3_t org;
		VectorCopy(ent->state.origin,org);
		VectorSubtract(cl_common->h2_Effects[index].Chain.origin, org, org);
		if (fabs(VectorNormalize(org)) > 350 * frametime)//closer than 30 is too close?
		{
			VectorScale(org, 350 * frametime, org);
			VectorAdd(ent->state.origin, org, ent->state.origin);
		}
		else//done--flash & git outa here (change type to redflash)
		{
			S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_scarabbyebye, 1, 1);
			cl_common->h2_Effects[index].Flash.entity_index = cl_common->h2_Effects[index].Chain.ent1;
			cl_common->h2_Effects[index].type = HWCE_RED_FLASH;
			VectorCopy(ent->state.origin, cl_common->h2_Effects[index].Flash.origin);
			cl_common->h2_Effects[index].Flash.reverse = 0;
			ent->model = R_RegisterModel("models/redspt.spr");
			ent->state.frame = 0;
			ent->state.drawflags = H2DRF_TRANSLUCENT;
		}
		}
		break;
	}

	CLH2_LinkEffectEntity(ent);

	CLH2_CreateStreamChain(cl_common->h2_Effects[index].Chain.ent1, cl_common->h2_Effects[index].Chain.tag,
		1, 0, 100, cl_common->h2_Effects[index].Chain.origin, ent->state.origin);
}

static void CLHW_UpdateEffectTripMineStill(int index, float frametime)
{
	cl_common->h2_Effects[index].Chain.time_amount += frametime;
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Chain.ent1];

	CLH2_LinkEffectEntity(ent);

	CLH2_CreateStreamChain(cl_common->h2_Effects[index].Chain.ent1, 1, 1, 0, 100,
		cl_common->h2_Effects[index].Chain.origin, ent->state.origin);
}

static void CLHW_UpdateEffectTripMine(int index, float frametime)
{
	cl_common->h2_Effects[index].Chain.time_amount += frametime;
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Chain.ent1];

	ent->state.origin[0] += frametime * cl_common->h2_Effects[index].Chain.velocity[0];
	ent->state.origin[1] += frametime * cl_common->h2_Effects[index].Chain.velocity[1];
	ent->state.origin[2] += frametime * cl_common->h2_Effects[index].Chain.velocity[2];

	CLH2_LinkEffectEntity(ent);

	CLH2_CreateStreamChain(cl_common->h2_Effects[index].Chain.ent1, 1, 1, 0, 100,
		cl_common->h2_Effects[index].Chain.origin, ent->state.origin);
}

static void CLHW_UpdateEffectEidolonStar(int index, float frametime)
{
	// update scale
	if (cl_common->h2_Effects[index].Star.scaleDir)
	{
		cl_common->h2_Effects[index].Star.scale += 0.05;
		if (cl_common->h2_Effects[index].Star.scale >= 1)
		{
			cl_common->h2_Effects[index].Star.scaleDir = 0;
		}
	}
	else
	{
		cl_common->h2_Effects[index].Star.scale -= 0.05;
		if (cl_common->h2_Effects[index].Star.scale <= 0.01)
		{
			cl_common->h2_Effects[index].Star.scaleDir = 1;
		}
	}
	
	cl_common->h2_Effects[index].Star.time_amount += frametime;
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Star.entity_index];

	ent->state.angles[0] += frametime * cl_common->h2_Effects[index].Star.avelocity[0];
	ent->state.angles[1] += frametime * cl_common->h2_Effects[index].Star.avelocity[1];
	ent->state.angles[2] += frametime * cl_common->h2_Effects[index].Star.avelocity[2];

	ent->state.origin[0] += frametime * cl_common->h2_Effects[index].Star.velocity[0];
	ent->state.origin[1] += frametime * cl_common->h2_Effects[index].Star.velocity[1];
	ent->state.origin[2] += frametime * cl_common->h2_Effects[index].Star.velocity[2];

	CLH2_LinkEffectEntity(ent);
	
	if (cl_common->h2_Effects[index].Star.ent1 != -1)
	{
		effect_entity_t* ent2 = &EffectEntities[cl_common->h2_Effects[index].Star.ent1];
		VectorCopy(ent->state.origin, ent2->state.origin);
		ent2->state.scale = cl_common->h2_Effects[index].Star.scale;
		ent2->state.angles[1] += frametime * 300;
		ent2->state.angles[2] += frametime * 400;
		CLH2_LinkEffectEntity(ent2);
	}
	if (rand() % 10 < 3)		
	{
		CLH2_RunParticleEffect4(ent->state.origin, 7, 148 + rand() % 11, pt_h2grav, 10 + rand() % 10);
	}
}

static void CLHW_UpdateEffectMissileStar(int index, float frametime)
{
	CLHW_UpdateEffectEidolonStar(index, frametime);
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Star.entity_index];
	if (cl_common->h2_Effects[index].Star.ent2 != -1)
	{
		effect_entity_t* ent2 = &EffectEntities[cl_common->h2_Effects[index].Star.ent2];
		VectorCopy(ent->state.origin, ent2->state.origin);
		ent2->state.scale = cl_common->h2_Effects[index].Star.scale;
		ent2->state.angles[1] += frametime * -300;
		ent2->state.angles[2] += frametime * -400;
		CLH2_LinkEffectEntity(ent2);
	}
}

static void CLH2_UpdateEffectChunk(int index, float frametime)
{
	cl_common->h2_Effects[index].Chunk.time_amount -= frametime;
	if (cl_common->h2_Effects[index].Chunk.time_amount < 0)
	{
		CLH2_FreeEffect(index);
		return;
	}
	for (int i = 0; i < cl_common->h2_Effects[index].Chunk.numChunks; i++)
	{
		effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Chunk.entity_index[i]];

		vec3_t oldorg;
		VectorCopy(ent->state.origin, oldorg);

		ent->state.origin[0] += frametime * cl_common->h2_Effects[index].Chunk.velocity[i][0];
		ent->state.origin[1] += frametime * cl_common->h2_Effects[index].Chunk.velocity[i][1];
		ent->state.origin[2] += frametime * cl_common->h2_Effects[index].Chunk.velocity[i][2];

		bool moving = true;
		if (CM_PointContentsQ1(ent->state.origin, 0) != BSP29CONTENTS_EMPTY) //||in_solid==true
		{
			// bouncing prolly won't work...
			VectorCopy(oldorg, ent->state.origin);

			cl_common->h2_Effects[index].Chunk.velocity[i][0] = 0;
			cl_common->h2_Effects[index].Chunk.velocity[i][1] = 0;
			cl_common->h2_Effects[index].Chunk.velocity[i][2] = 0;

			moving = false;
		}
		else
		{
			ent->state.angles[0] += frametime * cl_common->h2_Effects[index].Chunk.avel[i % 3][0];
			ent->state.angles[1] += frametime * cl_common->h2_Effects[index].Chunk.avel[i % 3][1];
			ent->state.angles[2] += frametime * cl_common->h2_Effects[index].Chunk.avel[i % 3][2];
		}

		if (cl_common->h2_Effects[index].Chunk.time_amount < frametime * 3)
		{
			// chunk leaves in 3 frames
			ent->state.scale *= .7;
		}

		CLH2_LinkEffectEntity(ent);

		cl_common->h2_Effects[index].Chunk.velocity[i][2] -= frametime * 500; // apply gravity

		switch (cl_common->h2_Effects[index].Chunk.type)
		{
		case H2THINGTYPE_GREYSTONE:
			break;
		case H2THINGTYPE_WOOD:
			break;
		case H2THINGTYPE_METAL:
			break;
		case H2THINGTYPE_FLESH:
			if (moving)
			{
				CLH2_TrailParticles(oldorg, ent->state.origin, rt_bloodshot);
			}
			break;
		case H2THINGTYPE_FIRE:
			break;
		case H2THINGTYPE_CLAY:
		case H2THINGTYPE_BONE:
			break;
		case H2THINGTYPE_LEAVES:
			break;
		case H2THINGTYPE_HAY:
			break;
		case H2THINGTYPE_BROWNSTONE:
			break;
		case H2THINGTYPE_CLOTH:
			break;
		case H2THINGTYPE_WOOD_LEAF:
			break;
		case H2THINGTYPE_WOOD_METAL:
			break;
		case H2THINGTYPE_WOOD_STONE:
			break;
		case H2THINGTYPE_METAL_STONE:
			break;
		case H2THINGTYPE_METAL_CLOTH:
			break;
		case H2THINGTYPE_WEBS:
			break;
		case H2THINGTYPE_GLASS:
			break;
		case H2THINGTYPE_ICE:
			if (moving)
			{
				CLH2_TrailParticles(oldorg, ent->state.origin, rt_ice);
			}
			break;
		case H2THINGTYPE_CLEARGLASS:
			break;
		case H2THINGTYPE_REDGLASS:
			break;
		case H2THINGTYPE_ACID:
			if (moving)
			{
				CLH2_TrailParticles(oldorg, ent->state.origin, rt_acidball);
			}
			break;
		case H2THINGTYPE_METEOR:
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_smoke);
			break;
		case H2THINGTYPE_GREENFLESH:
			if (moving)
			{
				CLH2_TrailParticles(oldorg, ent->state.origin, rt_acidball);
			}
			break;
		}
	}
}

static void CLH2_UpdateEffectRiderDeath(int index, float frametime)
{
	cl_common->h2_Effects[index].RD.time_amount += frametime;
	if (cl_common->h2_Effects[index].RD.time_amount >= 1)
	{
		cl_common->h2_Effects[index].RD.stage++;
		cl_common->h2_Effects[index].RD.time_amount -= 1;
	}

	vec3_t org;
	VectorCopy(cl_common->h2_Effects[index].RD.origin, org);
	org[0] += sin(cl_common->h2_Effects[index].RD.time_amount * 2 * M_PI) * 30;
	org[1] += cos(cl_common->h2_Effects[index].RD.time_amount * 2 * M_PI) * 30;

	if (cl_common->h2_Effects[index].RD.stage <= 6)
	{
		CLH2_RiderParticles(cl_common->h2_Effects[index].RD.stage + 1, org);
	}
	else
	{
		// To set the rider's origin point for the particles
		CLH2_RiderParticles(0, org);
		if (cl_common->h2_Effects[index].RD.stage == 7) 
		{
			cl_common->qh_cshifts[CSHIFT_BONUS].destcolor[0] = 255;
			cl_common->qh_cshifts[CSHIFT_BONUS].destcolor[1] = 255;
			cl_common->qh_cshifts[CSHIFT_BONUS].destcolor[2] = 255;
			cl_common->qh_cshifts[CSHIFT_BONUS].percent = 256;
		}
		else if (cl_common->h2_Effects[index].RD.stage > 13) 
		{
			CLH2_FreeEffect(index);
		}
	}
}

static void CLH2_UpdateEffect(int index, float frametime)
{
	switch (cl_common->h2_Effects[index].type)
	{
	case H2CE_RAIN:
		CLH2_UpdateEffectRain(index, frametime);
		break;
	case H2CE_SNOW:
		CLH2_UpdateEffectSnow(index, frametime);
		break;
	case H2CE_FOUNTAIN:
		CLH2_UpdateEffectFountain(index);
		break;
	case H2CE_QUAKE:
		CLH2_UpdateEffectQuake(index);
		break;
	case H2CE_WHITE_SMOKE:
	case H2CE_GREEN_SMOKE:
	case H2CE_GREY_SMOKE:
	case H2CE_RED_SMOKE:
	case H2CE_SLOW_WHITE_SMOKE:
	case H2CE_TELESMK1:
	case H2CE_TELESMK2:
	case H2CE_GHOST:
	case H2CE_REDCLOUD:
	case H2CE_FLAMESTREAM:
	case H2CE_ACID_MUZZFL:
	case H2CE_FLAMEWALL:
	case H2CE_FLAMEWALL2:
	case H2CE_ONFIRE:
		CLH2_UpdateEffectSmoke(index, frametime);
		break;
	case H2CE_SM_WHITE_FLASH:
	case H2CE_YELLOWRED_FLASH:
	case H2CE_BLUESPARK:
	case H2CE_YELLOWSPARK:
	case H2CE_SM_CIRCLE_EXP:
	case H2CE_SM_EXPLOSION:
	case H2CE_LG_EXPLOSION:
	case H2CE_FLOOR_EXPLOSION:
	case H2CE_FLOOR_EXPLOSION3:
	case H2CE_BLUE_EXPLOSION:
	case H2CE_REDSPARK:
	case H2CE_GREENSPARK:
	case H2CE_ICEHIT:
	case H2CE_MEDUSA_HIT:
	case H2CE_MEZZO_REFLECT:
	case H2CE_FLOOR_EXPLOSION2:
	case H2CE_XBOW_EXPLOSION:
	case H2CE_NEW_EXPLOSION:
	case H2CE_MAGIC_MISSILE_EXPLOSION:
	case H2CE_BONE_EXPLOSION:
	case H2CE_BLDRN_EXPL:
	case H2CE_BRN_BOUNCE:
	case H2CE_ACID_HIT:
	case H2CE_ACID_SPLAT:
	case H2CE_ACID_EXPL:
	case H2CE_LBALL_EXPL:
	case H2CE_FBOOM:
	case H2CE_BOMB:
	case H2CE_FIREWALL_SMALL:
	case H2CE_FIREWALL_MEDIUM:
	case H2CE_FIREWALL_LARGE:
		CLH2_UpdateEffectExplosion(index, frametime);
		break;
	case H2CE_BG_CIRCLE_EXP:
		CLH2_UpdateEffectBigCircleExplosion(index, frametime);
		break;
	case H2CE_LSHOCK:
		CLH2_UpdateEffectLShock(index);
		break;
	case H2CE_WHITE_FLASH:
	case H2CE_BLUE_FLASH:
	case H2CE_SM_BLUE_FLASH:
	case H2CE_RED_FLASH:
		CLH2_UpdateEffectFlash(index, frametime);
		break;
	case H2CE_RIDER_DEATH:
		CLH2_UpdateEffectRiderDeath(index, frametime);
		break;
	case H2CE_GRAVITYWELL:
		CLH2_UpdateEffectGravityWell(index, frametime);
		break;
	case H2CE_TELEPORTERPUFFS:
		CLH2_UpdateEffectTeleporterPuffs(index, frametime);
		break;
	case H2CE_TELEPORTERBODY:
		CLH2_UpdateEffectTeleporterBody(index, frametime);
		break;
	case H2CE_BONESHARD:
	case H2CE_BONESHRAPNEL:
		CLH2_UpdateEffectMissile(index, frametime);
		break;
	case H2CE_CHUNK:
		CLH2_UpdateEffectChunk(index, frametime);
		break;
	}
}

static void CLHW_UpdateEffect(int index, float frametime)
{
	switch (cl_common->h2_Effects[index].type)
	{
	case HWCE_RAIN:
		CLH2_UpdateEffectRain(index, frametime);
		break;
	case HWCE_FOUNTAIN:
		CLH2_UpdateEffectFountain(index);
		break;
	case HWCE_QUAKE:
		CLH2_UpdateEffectQuake(index);
		break;
	case HWCE_RIPPLE:
		CLHW_UpdateEffectRipple(index, frametime);
		break;
	case HWCE_WHITE_SMOKE:
	case HWCE_GREEN_SMOKE:
	case HWCE_GREY_SMOKE:
	case HWCE_RED_SMOKE:
	case HWCE_SLOW_WHITE_SMOKE:
	case HWCE_TELESMK2:
	case HWCE_GHOST:
	case HWCE_REDCLOUD:
	case HWCE_FLAMESTREAM:
	case HWCE_ACID_MUZZFL:
	case HWCE_FLAMEWALL:
	case HWCE_FLAMEWALL2:
	case HWCE_ONFIRE:
		CLH2_UpdateEffectSmoke(index, frametime);
		break;
	case HWCE_TELESMK1:
		CLHW_UpdateEffectTeleportSmoke1(index, frametime);
		break;
	case HWCE_SM_WHITE_FLASH:
	case HWCE_YELLOWRED_FLASH:
	case HWCE_BLUESPARK:
	case HWCE_YELLOWSPARK:
	case HWCE_SM_CIRCLE_EXP:
	case HWCE_SM_EXPLOSION:
	case HWCE_SM_EXPLOSION2:
	case HWCE_BG_EXPLOSION:
	case HWCE_FLOOR_EXPLOSION:
	case HWCE_BLUE_EXPLOSION:
	case HWCE_REDSPARK:
	case HWCE_GREENSPARK:
	case HWCE_ICEHIT:
	case HWCE_MEDUSA_HIT:
	case HWCE_MEZZO_REFLECT:
	case HWCE_FLOOR_EXPLOSION2:
	case HWCE_XBOW_EXPLOSION:
	case HWCE_NEW_EXPLOSION:
	case HWCE_MAGIC_MISSILE_EXPLOSION:
	case HWCE_BONE_EXPLOSION:
	case HWCE_BLDRN_EXPL:
	case HWCE_BRN_BOUNCE:
	case HWCE_ACID_HIT:
	case HWCE_ACID_SPLAT:
	case HWCE_ACID_EXPL:
	case HWCE_LBALL_EXPL:
	case HWCE_FBOOM:
	case HWCE_BOMB:
	case HWCE_FIREWALL_SMALL:
	case HWCE_FIREWALL_MEDIUM:
	case HWCE_FIREWALL_LARGE:
		CLH2_UpdateEffectExplosion(index, frametime);
		break;
	case HWCE_BG_CIRCLE_EXP:
		CLH2_UpdateEffectBigCircleExplosion(index, frametime);
		break;
	case HWCE_WHITE_FLASH:
	case HWCE_BLUE_FLASH:
	case HWCE_SM_BLUE_FLASH:
	case HWCE_HWSPLITFLASH:
	case HWCE_RED_FLASH:
		CLH2_UpdateEffectFlash(index, frametime);
		break;
	case HWCE_RIDER_DEATH:
		CLH2_UpdateEffectRiderDeath(index, frametime);
		break;
	case HWCE_TELEPORTERPUFFS:
		CLH2_UpdateEffectTeleporterPuffs(index, frametime);
		break;
	case HWCE_TELEPORTERBODY:
		CLH2_UpdateEffectTeleporterBody(index, frametime);
		break;
	case HWCE_HWDRILLA:
		CLHW_UpdateEffectDrilla(index, frametime);
		break;
	case HWCE_HWXBOWSHOOT:
		CLHW_UpdateEffectXBowShot(index, frametime);
		break;
	case HWCE_HWSHEEPINATOR:
		CLHW_UpdateEffectSheepinator(index, frametime);
		break;
	case HWCE_DEATHBUBBLES:
		CLHW_UpdateEffectDeathBubbles(index, frametime);
		break;
	case HWCE_SCARABCHAIN:
		CLHW_UpdateEffectScarabChain(index, frametime);
		break;
	case HWCE_TRIPMINESTILL:
		CLHW_UpdateEffectTripMineStill(index, frametime);
		break;
	case HWCE_TRIPMINE:
		CLHW_UpdateEffectTripMine(index, frametime);
		break;
	case HWCE_BONESHARD:
	case HWCE_BONESHRAPNEL:
	case HWCE_HWRAVENSTAFF:
		CLH2_UpdateEffectMissile(index, frametime);
		break;
	case HWCE_HWBONEBALL:
		CLHW_UpdateEffectBoneBall(index, frametime);
		break;
	case HWCE_HWRAVENPOWER:
		CLHW_UpdateEffectRavenPower(index, frametime);
		break;
	case HWCE_HWMISSILESTAR:
		CLHW_UpdateEffectMissileStar(index, frametime);
		break;
	case HWCE_HWEIDOLONSTAR:
		CLHW_UpdateEffectEidolonStar(index, frametime);
		break;
	}
}

void CLH2_UpdateEffects()
{
	float frametime = cls_common->frametime * 0.001;
	if (!frametime)
	{
		return;
	}

	for (int index = 0; index < MAX_EFFECTS_H2; index++)
	{
		if (!cl_common->h2_Effects[index].type)
		{
			continue;
		}
		if (GGameType & GAME_HexenWorld)
		{
			CLHW_UpdateEffect(index, frametime);
		}
		else
		{
			CLH2_UpdateEffect(index, frametime);
		}
	}
}
