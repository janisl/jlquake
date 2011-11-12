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

sfxHandle_t clh2_fxsfx_bone;
sfxHandle_t clh2_fxsfx_bonefpow;
sfxHandle_t clh2_fxsfx_xbowshoot;
sfxHandle_t clh2_fxsfx_xbowfshoot;
sfxHandle_t clh2_fxsfx_explode;
sfxHandle_t clh2_fxsfx_mmfire;
sfxHandle_t clh2_fxsfx_eidolon;
sfxHandle_t clh2_fxsfx_scarabwhoosh;
sfxHandle_t clh2_fxsfx_scarabgrab;
sfxHandle_t clh2_fxsfx_scarabhome;
sfxHandle_t clh2_fxsfx_scarabrip;
sfxHandle_t clh2_fxsfx_scarabbyebye;
sfxHandle_t clh2_fxsfx_ravensplit;
static sfxHandle_t clh2_fxsfx_ravenfire;
sfxHandle_t clh2_fxsfx_ravengo;
sfxHandle_t clh2_fxsfx_drillashoot;
sfxHandle_t clh2_fxsfx_drillaspin;
sfxHandle_t clh2_fxsfx_drillameat;

sfxHandle_t clh2_fxsfx_arr2flsh;
sfxHandle_t clh2_fxsfx_arr2wood;
sfxHandle_t clh2_fxsfx_met2stn;

static sfxHandle_t clh2_fxsfx_ripple;
static sfxHandle_t clh2_fxsfx_splash;

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
	clh2_fxsfx_scarabrip = S_RegisterSound("assassin/chntear.wav");
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

void CLH2_ParseEffectRain(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].Rain.min_org);
	message.ReadPos(cl_common->h2_Effects[index].Rain.max_org);
	message.ReadPos(cl_common->h2_Effects[index].Rain.e_size);
	message.ReadPos(cl_common->h2_Effects[index].Rain.dir);
	cl_common->h2_Effects[index].Rain.color = message.ReadShort();
	cl_common->h2_Effects[index].Rain.count = message.ReadShort();
	cl_common->h2_Effects[index].Rain.wait = message.ReadFloat();
}

void CLH2_ParseEffectSnow(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].Snow.min_org);
	message.ReadPos(cl_common->h2_Effects[index].Snow.max_org);
	cl_common->h2_Effects[index].Snow.flags = message.ReadByte();
	message.ReadPos(cl_common->h2_Effects[index].Snow.dir);
	cl_common->h2_Effects[index].Snow.count = message.ReadByte();
}

void CLH2_ParseEffectFountain(int index, QMsg& message)
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

void CLH2_ParseEffectQuake(int index, QMsg& message)
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

bool CLH2_ParseEffectWhiteSmoke(int index, QMsg& message)
{
	return CLH2_ParseEffectSmokeCommon(index, message, "models/whtsmk1.spr", H2DRF_TRANSLUCENT);
}

bool CLH2_ParseEffectGreenSmoke(int index, QMsg& message)
{
	return CLH2_ParseEffectSmokeCommon(index, message, "models/grnsmk1.spr", H2DRF_TRANSLUCENT);
}

bool CLH2_ParseEffectGraySmoke(int index, QMsg& message)
{
	return CLH2_ParseEffectSmokeCommon(index, message, "models/grysmk1.spr", H2DRF_TRANSLUCENT);
}

bool CLH2_ParseEffectRedSmoke(int index, QMsg& message)
{
	return CLH2_ParseEffectSmokeCommon(index, message, "models/redsmk1.spr", H2DRF_TRANSLUCENT);
}

bool CLH2_ParseEffectTeleportSmoke1(int index, QMsg& message)
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

bool CLH2_ParseEffectTeleportSmoke2(int index, QMsg& message)
{
	return CLH2_ParseEffectSmokeCommon(index, message, "models/telesmk2.spr", H2DRF_TRANSLUCENT);
}

bool CLH2_ParseEffectGhost(int index, QMsg& message)
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

bool CLH2_ParseEffectRedCloud(int index, QMsg& message)
{
	return CLH2_ParseEffectSmokeCommon(index, message, "models/rcloud.spr", 0);
}

bool CLH2_ParseEffectAcidMuzzleFlash(int index, QMsg& message)
{
	if (!CLH2_ParseEffectSmokeCommon(index, message, "models/muzzle1.spr", H2DRF_TRANSLUCENT | H2MLS_ABSLIGHT))
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];
	ent->state.abslight = 0.2;
	return true;
}

bool CLH2_ParseEffectFlameStream(int index, QMsg& message)
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

bool CLH2_ParseEffectFlameWall(int index, QMsg& message)
{
	return CLH2_ParseEffectSmokeCommon(index, message, "models/firewal1.spr", 0);
}

bool CLH2_ParseEffectFlameWall2(int index, QMsg& message)
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

bool CLH2_ParseEffectOnFire(int index, QMsg& message)
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

bool CLHW_ParseEffectRipple(int index, QMsg& message)
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

bool CLH2_ParseEffectSmallWhiteFlash(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/sm_white.spr");
}

bool CLH2_ParseEffectYellowRedFlash(int index, QMsg& message)
{
	if (!CLH2_ParseEffectExplosionCommon(index, message, "models/yr_flsh.spr"))
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];
	ent->state.drawflags = H2DRF_TRANSLUCENT;
	return true;
}

bool CLH2_ParseEffectBlueSpark(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/bspark.spr");
}

bool CLH2_ParseEffectYellowSpark(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/spark.spr");
}

bool CLH2_ParseEffectSmallCircleExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/fcircle.spr");
}

bool CLH2_ParseEffectBigCircleExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/xplod29.spr");
}

bool CLH2_ParseEffectSmallExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/sm_expld.spr");
}

bool CLH2_ParseEffectSmallExplosionWithSound(int index, QMsg& message)
{
	if (!CLH2_ParseEffectSmallExplosion(index, message))
	{
		return false;
	}
	S_StartSound(cl_common->h2_Effects[index].Smoke.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_explode, 1, 1);
	return true;
}

bool CLH2_ParseEffectBigExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/bg_expld.spr");
}

bool CLH2_ParseEffectBigExplosionWithSound(int index, QMsg& message)
{
	if (!CLH2_ParseEffectBigExplosion(index, message))
	{
		return false;
	}
	S_StartSound(cl_common->h2_Effects[index].Smoke.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_explode, 1, 1);
	return true;
}

bool CLH2_ParseEffectFloorExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/fl_expld.spr");
}

bool CLH2_ParseEffectFloorExplosion2(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/flrexpl2.spr");
}

bool CLH2_ParseEffectFloorExplosion3(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/biggy.spr");
}

bool CLH2_ParseEffectBlueExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/xpspblue.spr");
}

bool CLH2_ParseEffectRedSpark(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/rspark.spr");
}

bool CLH2_ParseEffectGreenSpark(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/gspark.spr");
}

bool CLH2_ParseEffectIceHit(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/icehit.spr");
}

bool CLH2_ParseEffectMedusaHit(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/medhit.spr");
}

bool CLH2_ParseEffectMezzoReflect(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/mezzoref.spr");
}

bool CLH2_ParseEffectXBowExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/xbowexpl.spr");
}

bool CLH2_ParseEffectNewExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/gen_expl.spr");
}

bool CLH2_ParseEffectMagicMissileExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/mm_expld.spr");
}

bool CLH2_ParseEffectMagicMissileExplosionWithSound(int index, QMsg& message)
{
	if (!CLH2_ParseEffectMagicMissileExplosion(index, message))
	{
		return false;
	}
	S_StartSound(cl_common->h2_Effects[index].Smoke.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_explode, 1, 1);
	return true;
}

bool CLH2_ParseEffectBoneExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/bonexpld.spr");
}

bool CLH2_ParseEffectBldrnExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/xplsn_1.spr");
}

bool CLH2_ParseEffectLShock(int index, QMsg& message)
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

bool CLH2_ParseEffectAcidHit(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/axplsn_2.spr");
}

bool CLH2_ParseEffectAcidSplat(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/axplsn_1.spr");
}

bool CLH2_ParseEffectAcidExplosion(int index, QMsg& message)
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

bool CLH2_ParseEffectLBallExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/Bluexp3.spr");
}

bool CLH2_ParseEffectFBoom(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/fboom.spr");
}

bool CLH2_ParseEffectBomb(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/pow.spr");
}

bool CLH2_ParseEffectFirewallSall(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/firewal1.spr");
}

bool CLH2_ParseEffectFirewallMedium(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/firewal5.spr");
}

bool CLH2_ParseEffectFirewallLarge(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/firewal4.spr");
}
