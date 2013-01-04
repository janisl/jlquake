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

#include "../progsvm/progsvm.h"
#include "../quake_hexen/local.h"
#include "local.h"
#include "../../common/common_defs.h"

Cvar* sv_ce_scale;
Cvar* sv_ce_max_size;

// this will create several effects and store the ids in the array
static float MultiEffectIds[10];
static int MultiEffectIdCount;

static void SVH2_ClearEffects()
{
	Com_Memset(sv.h2_Effects,0,sizeof(sv.h2_Effects));
}

static void SVH2_GetSendEffectTestParams(int index, bool& DoTest, float& TestDistance, vec3_t TestO)
{
	VectorCopy(vec3_origin, TestO);
	TestDistance = 0;

	switch (sv.h2_Effects[index].type)
	{
	case H2CE_RAIN:
	case H2CE_SNOW:
		DoTest = false;
		break;

	case H2CE_FOUNTAIN:
		DoTest = false;
		break;

	case H2CE_QUAKE:
		VectorCopy(sv.h2_Effects[index].Quake.origin, TestO);
		TestDistance = 700;
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
		VectorCopy(sv.h2_Effects[index].Smoke.origin, TestO);
		TestDistance = 250;
		break;

	case H2CE_SM_WHITE_FLASH:
	case H2CE_YELLOWRED_FLASH:
	case H2CE_BLUESPARK:
	case H2CE_YELLOWSPARK:
	case H2CE_SM_CIRCLE_EXP:
	case H2CE_BG_CIRCLE_EXP:
	case H2CE_SM_EXPLOSION:
	case H2CE_LG_EXPLOSION:
	case H2CE_FLOOR_EXPLOSION:
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
	case H2CE_ACID_HIT:
	case H2CE_LBALL_EXPL:
	case H2CE_FIREWALL_SMALL:
	case H2CE_FIREWALL_MEDIUM:
	case H2CE_FIREWALL_LARGE:
	case H2CE_ACID_SPLAT:
	case H2CE_ACID_EXPL:
	case H2CE_FBOOM:
	case H2CE_BRN_BOUNCE:
	case H2CE_LSHOCK:
	case H2CE_BOMB:
	case H2CE_FLOOR_EXPLOSION3:
		VectorCopy(sv.h2_Effects[index].Smoke.origin, TestO);
		TestDistance = 250;
		break;

	case H2CE_WHITE_FLASH:
	case H2CE_BLUE_FLASH:
	case H2CE_SM_BLUE_FLASH:
	case H2CE_RED_FLASH:
		VectorCopy(sv.h2_Effects[index].Smoke.origin, TestO);
		TestDistance = 250;
		break;

	case H2CE_RIDER_DEATH:
		DoTest = false;
		break;

	case H2CE_GRAVITYWELL:
		DoTest = false;
		break;

	case H2CE_TELEPORTERPUFFS:
		VectorCopy(sv.h2_Effects[index].Teleporter.origin, TestO);
		TestDistance = 350;
		break;

	case H2CE_TELEPORTERBODY:
		VectorCopy(sv.h2_Effects[index].Teleporter.origin, TestO);
		TestDistance = 350;
		break;

	case H2CE_BONESHARD:
	case H2CE_BONESHRAPNEL:
		VectorCopy(sv.h2_Effects[index].Missile.origin, TestO);
		TestDistance = 600;
		break;
	case H2CE_CHUNK:
		VectorCopy(sv.h2_Effects[index].Chunk.origin, TestO);
		TestDistance = 600;
		break;

	default:
		PR_RunError("SV_SendEffect: bad type");
		break;
	}
}

static void SVHW_GetSendEffectTestParams(int index, bool& DoTest, vec3_t TestO)
{
	VectorCopy(vec3_origin, TestO);

	switch (sv.h2_Effects[index].type)
	{
	case HWCE_HWSHEEPINATOR:
	case HWCE_HWXBOWSHOOT:
		VectorCopy(sv.h2_Effects[index].Xbow.origin[5], TestO);
		break;
	case HWCE_SCARABCHAIN:
		VectorCopy(sv.h2_Effects[index].Chain.origin, TestO);
		break;

	case HWCE_TRIPMINE:
		VectorCopy(sv.h2_Effects[index].Chain.origin, TestO);
		break;

	//ACHTUNG!!!!!!! setting DoTest to false here does not insure that effect will be sent to everyone!
	case HWCE_TRIPMINESTILL:
		DoTest = false;
		break;

	case HWCE_RAIN:
		DoTest = false;
		break;

	case HWCE_FOUNTAIN:
		DoTest = false;
		break;

	case HWCE_QUAKE:
		VectorCopy(sv.h2_Effects[index].Quake.origin, TestO);
		break;

	case HWCE_WHITE_SMOKE:
	case HWCE_GREEN_SMOKE:
	case HWCE_GREY_SMOKE:
	case HWCE_RED_SMOKE:
	case HWCE_SLOW_WHITE_SMOKE:
	case HWCE_TELESMK1:
	case HWCE_TELESMK2:
	case HWCE_GHOST:
	case HWCE_REDCLOUD:
	case HWCE_FLAMESTREAM:
	case HWCE_ACID_MUZZFL:
	case HWCE_FLAMEWALL:
	case HWCE_FLAMEWALL2:
	case HWCE_ONFIRE:
	case HWCE_RIPPLE:
		VectorCopy(sv.h2_Effects[index].Smoke.origin, TestO);
		break;

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
	case HWCE_ACID_HIT:
	case HWCE_LBALL_EXPL:
	case HWCE_FIREWALL_SMALL:
	case HWCE_FIREWALL_MEDIUM:
	case HWCE_FIREWALL_LARGE:
	case HWCE_ACID_SPLAT:
	case HWCE_ACID_EXPL:
	case HWCE_FBOOM:
	case HWCE_BRN_BOUNCE:
	case HWCE_LSHOCK:
	case HWCE_BOMB:
	case HWCE_FLOOR_EXPLOSION3:
		VectorCopy(sv.h2_Effects[index].Smoke.origin, TestO);
		break;

	case HWCE_WHITE_FLASH:
	case HWCE_BLUE_FLASH:
	case HWCE_SM_BLUE_FLASH:
	case HWCE_HWSPLITFLASH:
	case HWCE_RED_FLASH:
		VectorCopy(sv.h2_Effects[index].Smoke.origin, TestO);
		break;


	case HWCE_RIDER_DEATH:
		DoTest = false;
		break;

	case HWCE_TELEPORTERPUFFS:
		VectorCopy(sv.h2_Effects[index].Teleporter.origin, TestO);
		break;

	case HWCE_TELEPORTERBODY:
		VectorCopy(sv.h2_Effects[index].Teleporter.origin, TestO);
		break;

	case HWCE_DEATHBUBBLES:
		if (sv.h2_Effects[index].Bubble.owner < 0 || sv.h2_Effects[index].Bubble.owner >= sv.qh_num_edicts)
		{
			return;
		}
		VectorCopy(PROG_TO_EDICT(sv.h2_Effects[index].Bubble.owner)->GetOrigin(), TestO);
		break;

	case HWCE_HWDRILLA:
	case HWCE_BONESHARD:
	case HWCE_BONESHRAPNEL:
	case HWCE_HWBONEBALL:
	case HWCE_HWRAVENSTAFF:
	case HWCE_HWRAVENPOWER:
		VectorCopy(sv.h2_Effects[index].Missile.origin, TestO);
		break;

	case HWCE_HWMISSILESTAR:
	case HWCE_HWEIDOLONSTAR:
		VectorCopy(sv.h2_Effects[index].Missile.origin, TestO);
		break;
	default:
		PR_RunError("SV_SendEffect: bad type");
		break;
	}
}

static void SVH2_WriteEffectRain(int index, QMsg& message)
{
	message.WriteCoord(sv.h2_Effects[index].Rain.min_org[0]);
	message.WriteCoord(sv.h2_Effects[index].Rain.min_org[1]);
	message.WriteCoord(sv.h2_Effects[index].Rain.min_org[2]);
	message.WriteCoord(sv.h2_Effects[index].Rain.max_org[0]);
	message.WriteCoord(sv.h2_Effects[index].Rain.max_org[1]);
	message.WriteCoord(sv.h2_Effects[index].Rain.max_org[2]);
	message.WriteCoord(sv.h2_Effects[index].Rain.e_size[0]);
	message.WriteCoord(sv.h2_Effects[index].Rain.e_size[1]);
	message.WriteCoord(sv.h2_Effects[index].Rain.e_size[2]);
	message.WriteCoord(sv.h2_Effects[index].Rain.dir[0]);
	message.WriteCoord(sv.h2_Effects[index].Rain.dir[1]);
	message.WriteCoord(sv.h2_Effects[index].Rain.dir[2]);
	message.WriteShort(sv.h2_Effects[index].Rain.color);
	message.WriteShort(sv.h2_Effects[index].Rain.count);
	message.WriteFloat(sv.h2_Effects[index].Rain.wait);
}

static void SVH2_WriteEffectSnow(int index, QMsg& message)
{
	message.WriteCoord(sv.h2_Effects[index].Snow.min_org[0]);
	message.WriteCoord(sv.h2_Effects[index].Snow.min_org[1]);
	message.WriteCoord(sv.h2_Effects[index].Snow.min_org[2]);
	message.WriteCoord(sv.h2_Effects[index].Snow.max_org[0]);
	message.WriteCoord(sv.h2_Effects[index].Snow.max_org[1]);
	message.WriteCoord(sv.h2_Effects[index].Snow.max_org[2]);
	message.WriteByte(sv.h2_Effects[index].Snow.flags);
	message.WriteCoord(sv.h2_Effects[index].Snow.dir[0]);
	message.WriteCoord(sv.h2_Effects[index].Snow.dir[1]);
	message.WriteCoord(sv.h2_Effects[index].Snow.dir[2]);
	message.WriteByte(sv.h2_Effects[index].Snow.count);
}

static void SVH2_WriteEffectFountain(int index, QMsg& message)
{
	message.WriteCoord(sv.h2_Effects[index].Fountain.pos[0]);
	message.WriteCoord(sv.h2_Effects[index].Fountain.pos[1]);
	message.WriteCoord(sv.h2_Effects[index].Fountain.pos[2]);
	message.WriteAngle(sv.h2_Effects[index].Fountain.angle[0]);
	message.WriteAngle(sv.h2_Effects[index].Fountain.angle[1]);
	message.WriteAngle(sv.h2_Effects[index].Fountain.angle[2]);
	message.WriteCoord(sv.h2_Effects[index].Fountain.movedir[0]);
	message.WriteCoord(sv.h2_Effects[index].Fountain.movedir[1]);
	message.WriteCoord(sv.h2_Effects[index].Fountain.movedir[2]);
	message.WriteShort(sv.h2_Effects[index].Fountain.color);
	message.WriteByte(sv.h2_Effects[index].Fountain.cnt);
}

static void SVH2_WriteEffectQuake(int index, QMsg& message)
{
	message.WriteCoord(sv.h2_Effects[index].Quake.origin[0]);
	message.WriteCoord(sv.h2_Effects[index].Quake.origin[1]);
	message.WriteCoord(sv.h2_Effects[index].Quake.origin[2]);
	message.WriteFloat(sv.h2_Effects[index].Quake.radius);
}

static void SVH2_WriteEffectSmoke(int index, QMsg& message)
{
	message.WriteCoord(sv.h2_Effects[index].Smoke.origin[0]);
	message.WriteCoord(sv.h2_Effects[index].Smoke.origin[1]);
	message.WriteCoord(sv.h2_Effects[index].Smoke.origin[2]);
	message.WriteFloat(sv.h2_Effects[index].Smoke.velocity[0]);
	message.WriteFloat(sv.h2_Effects[index].Smoke.velocity[1]);
	message.WriteFloat(sv.h2_Effects[index].Smoke.velocity[2]);
	message.WriteFloat(sv.h2_Effects[index].Smoke.framelength);
	if (!(GGameType & GAME_HexenWorld))
	{
		message.WriteFloat(sv.h2_Effects[index].Smoke.frame);
	}
}

static void SVH2_WriteEffectFlash(int index, QMsg& message)
{
	message.WriteCoord(sv.h2_Effects[index].Smoke.origin[0]);
	message.WriteCoord(sv.h2_Effects[index].Smoke.origin[1]);
	message.WriteCoord(sv.h2_Effects[index].Smoke.origin[2]);
}

static void SVH2_WriteEffectRiderDeath(int index, QMsg& message)
{
	message.WriteCoord(sv.h2_Effects[index].RD.origin[0]);
	message.WriteCoord(sv.h2_Effects[index].RD.origin[1]);
	message.WriteCoord(sv.h2_Effects[index].RD.origin[2]);
}

static void SVH2_WriteEffectTeleporterPuffs(int index, QMsg& message)
{
	message.WriteCoord(sv.h2_Effects[index].Teleporter.origin[0]);
	message.WriteCoord(sv.h2_Effects[index].Teleporter.origin[1]);
	message.WriteCoord(sv.h2_Effects[index].Teleporter.origin[2]);
}

static void SVH2_WriteEffectTeleporterBody(int index, QMsg& message)
{
	message.WriteCoord(sv.h2_Effects[index].Teleporter.origin[0]);
	message.WriteCoord(sv.h2_Effects[index].Teleporter.origin[1]);
	message.WriteCoord(sv.h2_Effects[index].Teleporter.origin[2]);
	message.WriteFloat(sv.h2_Effects[index].Teleporter.velocity[0][0]);
	message.WriteFloat(sv.h2_Effects[index].Teleporter.velocity[0][1]);
	message.WriteFloat(sv.h2_Effects[index].Teleporter.velocity[0][2]);
	message.WriteFloat(sv.h2_Effects[index].Teleporter.skinnum);
}

static void SVH2_WriteEffectBoneShrapnel(int index, QMsg& message)
{
	message.WriteCoord(sv.h2_Effects[index].Missile.origin[0]);
	message.WriteCoord(sv.h2_Effects[index].Missile.origin[1]);
	message.WriteCoord(sv.h2_Effects[index].Missile.origin[2]);
	message.WriteFloat(sv.h2_Effects[index].Missile.velocity[0]);
	message.WriteFloat(sv.h2_Effects[index].Missile.velocity[1]);
	message.WriteFloat(sv.h2_Effects[index].Missile.velocity[2]);
	message.WriteFloat(sv.h2_Effects[index].Missile.angle[0]);
	message.WriteFloat(sv.h2_Effects[index].Missile.angle[1]);
	message.WriteFloat(sv.h2_Effects[index].Missile.angle[2]);
	message.WriteFloat(sv.h2_Effects[index].Missile.avelocity[0]);
	message.WriteFloat(sv.h2_Effects[index].Missile.avelocity[1]);
	message.WriteFloat(sv.h2_Effects[index].Missile.avelocity[2]);
}

static void SVH2_WriteEffectGravityWell(int index, QMsg& message)
{
	message.WriteCoord(sv.h2_Effects[index].GravityWell.origin[0]);
	message.WriteCoord(sv.h2_Effects[index].GravityWell.origin[1]);
	message.WriteCoord(sv.h2_Effects[index].GravityWell.origin[2]);
	message.WriteShort(sv.h2_Effects[index].GravityWell.color);
	message.WriteFloat(sv.h2_Effects[index].GravityWell.lifetime);
}

static void SVH2_WriteEffectChunk(int index, QMsg& message)
{
	message.WriteCoord(sv.h2_Effects[index].Chunk.origin[0]);
	message.WriteCoord(sv.h2_Effects[index].Chunk.origin[1]);
	message.WriteCoord(sv.h2_Effects[index].Chunk.origin[2]);
	message.WriteByte(sv.h2_Effects[index].Chunk.type);
	message.WriteCoord(sv.h2_Effects[index].Chunk.srcVel[0]);
	message.WriteCoord(sv.h2_Effects[index].Chunk.srcVel[1]);
	message.WriteCoord(sv.h2_Effects[index].Chunk.srcVel[2]);
	message.WriteByte(sv.h2_Effects[index].Chunk.numChunks);
}

static void SVHW_WriteEffectRavenStaff(int index, QMsg& message)
{
	message.WriteCoord(sv.h2_Effects[index].Missile.origin[0]);
	message.WriteCoord(sv.h2_Effects[index].Missile.origin[1]);
	message.WriteCoord(sv.h2_Effects[index].Missile.origin[2]);
	message.WriteFloat(sv.h2_Effects[index].Missile.velocity[0]);
	message.WriteFloat(sv.h2_Effects[index].Missile.velocity[1]);
	message.WriteFloat(sv.h2_Effects[index].Missile.velocity[2]);
}

static void SVHW_WriteEffectDrilla(int index, QMsg& message)
{
	message.WriteCoord(sv.h2_Effects[index].Missile.origin[0]);
	message.WriteCoord(sv.h2_Effects[index].Missile.origin[1]);
	message.WriteCoord(sv.h2_Effects[index].Missile.origin[2]);
	message.WriteAngle(sv.h2_Effects[index].Missile.angle[0]);
	message.WriteAngle(sv.h2_Effects[index].Missile.angle[1]);
	message.WriteShort(sv.h2_Effects[index].Missile.speed);
}

static void SVHW_WriteEffectDeathBubbles(int index, QMsg& message)
{
	message.WriteShort(sv.h2_Effects[index].Bubble.owner);
	message.WriteByte(sv.h2_Effects[index].Bubble.offset[0]);
	message.WriteByte(sv.h2_Effects[index].Bubble.offset[1]);
	message.WriteByte(sv.h2_Effects[index].Bubble.offset[2]);
	message.WriteByte(sv.h2_Effects[index].Bubble.count);
}

static void SVHW_WriteEffectScarabChain(int index, QMsg& message)
{
	message.WriteCoord(sv.h2_Effects[index].Chain.origin[0]);
	message.WriteCoord(sv.h2_Effects[index].Chain.origin[1]);
	message.WriteCoord(sv.h2_Effects[index].Chain.origin[2]);
	message.WriteShort(sv.h2_Effects[index].Chain.owner + sv.h2_Effects[index].Chain.material);
	message.WriteByte(sv.h2_Effects[index].Chain.tag);
}

static void SVHW_WriteEffectTripMine(int index, QMsg& message)
{
	message.WriteCoord(sv.h2_Effects[index].Chain.origin[0]);
	message.WriteCoord(sv.h2_Effects[index].Chain.origin[1]);
	message.WriteCoord(sv.h2_Effects[index].Chain.origin[2]);
	message.WriteFloat(sv.h2_Effects[index].Chain.velocity[0]);
	message.WriteFloat(sv.h2_Effects[index].Chain.velocity[1]);
	message.WriteFloat(sv.h2_Effects[index].Chain.velocity[2]);
}

static void SVHW_WriteEffectSheepinator(int index, QMsg& message)
{
	message.WriteCoord(sv.h2_Effects[index].Xbow.origin[5][0]);
	message.WriteCoord(sv.h2_Effects[index].Xbow.origin[5][1]);
	message.WriteCoord(sv.h2_Effects[index].Xbow.origin[5][2]);
	message.WriteAngle(sv.h2_Effects[index].Xbow.angle[0]);
	message.WriteAngle(sv.h2_Effects[index].Xbow.angle[1]);

	//now send the guys that have turned
	message.WriteByte(sv.h2_Effects[index].Xbow.turnedbolts);
	message.WriteByte(sv.h2_Effects[index].Xbow.activebolts);
	for (int i = 0; i < 5; i++)
	{
		if ((1 << i) & sv.h2_Effects[index].Xbow.turnedbolts)
		{
			message.WriteCoord(sv.h2_Effects[index].Xbow.origin[i][0]);
			message.WriteCoord(sv.h2_Effects[index].Xbow.origin[i][1]);
			message.WriteCoord(sv.h2_Effects[index].Xbow.origin[i][2]);
			message.WriteAngle(sv.h2_Effects[index].Xbow.vel[i][0]);
			message.WriteAngle(sv.h2_Effects[index].Xbow.vel[i][1]);
		}
	}
}

static void SVHW_WriteEffectXBowShoot(int index, QMsg& message)
{
	message.WriteCoord(sv.h2_Effects[index].Xbow.origin[5][0]);
	message.WriteCoord(sv.h2_Effects[index].Xbow.origin[5][1]);
	message.WriteCoord(sv.h2_Effects[index].Xbow.origin[5][2]);
	message.WriteAngle(sv.h2_Effects[index].Xbow.angle[0]);
	message.WriteAngle(sv.h2_Effects[index].Xbow.angle[1]);
	message.WriteByte(sv.h2_Effects[index].Xbow.bolts);
	message.WriteByte(sv.h2_Effects[index].Xbow.randseed);

	//now send the guys that have turned
	message.WriteByte(sv.h2_Effects[index].Xbow.turnedbolts);
	message.WriteByte(sv.h2_Effects[index].Xbow.activebolts);
	for (int i = 0; i < 5; i++)
	{
		if ((1 << i) & sv.h2_Effects[index].Xbow.turnedbolts)
		{
			message.WriteCoord(sv.h2_Effects[index].Xbow.origin[i][0]);
			message.WriteCoord(sv.h2_Effects[index].Xbow.origin[i][1]);
			message.WriteCoord(sv.h2_Effects[index].Xbow.origin[i][2]);
			message.WriteAngle(sv.h2_Effects[index].Xbow.vel[i][0]);
			message.WriteAngle(sv.h2_Effects[index].Xbow.vel[i][1]);
		}
	}
}

static void SVH2_WriteEffect(int index, QMsg& message)
{
	message.WriteByte(h2svc_start_effect);
	message.WriteByte(index);
	message.WriteByte(sv.h2_Effects[index].type);

	switch (sv.h2_Effects[index].type)
	{
	case H2CE_RAIN:
		SVH2_WriteEffectRain(index, message);
		break;
	case H2CE_SNOW:
		SVH2_WriteEffectSnow(index, message);
		break;
	case H2CE_FOUNTAIN:
		SVH2_WriteEffectFountain(index, message);
		break;
	case H2CE_QUAKE:
		SVH2_WriteEffectQuake(index, message);
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
		SVH2_WriteEffectSmoke(index, message);
		break;
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
	case H2CE_ACID_HIT:
	case H2CE_ACID_SPLAT:
	case H2CE_ACID_EXPL:
	case H2CE_LBALL_EXPL:
	case H2CE_FIREWALL_SMALL:
	case H2CE_FIREWALL_MEDIUM:
	case H2CE_FIREWALL_LARGE:
	case H2CE_FBOOM:
	case H2CE_BOMB:
	case H2CE_BRN_BOUNCE:
	case H2CE_LSHOCK:
	case H2CE_WHITE_FLASH:
	case H2CE_BLUE_FLASH:
	case H2CE_SM_BLUE_FLASH:
	case H2CE_RED_FLASH:
		SVH2_WriteEffectFlash(index, message);
		break;
	case H2CE_RIDER_DEATH:
		SVH2_WriteEffectRiderDeath(index, message);
		break;
	case H2CE_TELEPORTERPUFFS:
		SVH2_WriteEffectTeleporterPuffs(index, message);
		break;
	case H2CE_TELEPORTERBODY:
		SVH2_WriteEffectTeleporterBody(index, message);
		break;
	case H2CE_BONESHARD:
	case H2CE_BONESHRAPNEL:
		SVH2_WriteEffectBoneShrapnel(index, message);
		break;
	case H2CE_GRAVITYWELL:
		SVH2_WriteEffectGravityWell(index, message);
		break;
	case H2CE_CHUNK:
		SVH2_WriteEffectChunk(index, message);
		break;
	default:
		PR_RunError("SV_SendEffect: bad type");
		break;
	}
}

static void SVHW_WriteEffect(int index, QMsg& message)
{
	message.WriteByte(hwsvc_start_effect);
	message.WriteByte(index);
	message.WriteByte(sv.h2_Effects[index].type);

	switch (sv.h2_Effects[index].type)
	{
	case HWCE_RAIN:
		SVH2_WriteEffectRain(index, message);
		break;
	case HWCE_FOUNTAIN:
		SVH2_WriteEffectFountain(index, message);
		break;
	case HWCE_QUAKE:
		SVH2_WriteEffectQuake(index, message);
		break;
	case HWCE_WHITE_SMOKE:
	case HWCE_GREEN_SMOKE:
	case HWCE_GREY_SMOKE:
	case HWCE_RED_SMOKE:
	case HWCE_SLOW_WHITE_SMOKE:
	case HWCE_TELESMK1:
	case HWCE_TELESMK2:
	case HWCE_GHOST:
	case HWCE_REDCLOUD:
	case HWCE_FLAMESTREAM:
	case HWCE_ACID_MUZZFL:
	case HWCE_FLAMEWALL:
	case HWCE_FLAMEWALL2:
	case HWCE_ONFIRE:
	case HWCE_RIPPLE:
		SVH2_WriteEffectSmoke(index, message);
		break;
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
	case HWCE_ACID_HIT:
	case HWCE_ACID_SPLAT:
	case HWCE_ACID_EXPL:
	case HWCE_LBALL_EXPL:
	case HWCE_FIREWALL_SMALL:
	case HWCE_FIREWALL_MEDIUM:
	case HWCE_FIREWALL_LARGE:
	case HWCE_FBOOM:
	case HWCE_BOMB:
	case HWCE_BRN_BOUNCE:
	case HWCE_LSHOCK:
	case HWCE_WHITE_FLASH:
	case HWCE_BLUE_FLASH:
	case HWCE_SM_BLUE_FLASH:
	case HWCE_HWSPLITFLASH:
	case HWCE_RED_FLASH:
		SVH2_WriteEffectFlash(index, message);
		break;
	case HWCE_RIDER_DEATH:
		SVH2_WriteEffectRiderDeath(index, message);
		break;
	case HWCE_TELEPORTERPUFFS:
		SVH2_WriteEffectTeleporterPuffs(index, message);
		break;
	case HWCE_TELEPORTERBODY:
		SVH2_WriteEffectTeleporterBody(index, message);
		break;
	case HWCE_BONESHRAPNEL:
	case HWCE_HWBONEBALL:
		SVH2_WriteEffectBoneShrapnel(index, message);
		break;
	case HWCE_BONESHARD:
	case HWCE_HWRAVENSTAFF:
	case HWCE_HWMISSILESTAR:
	case HWCE_HWEIDOLONSTAR:
	case HWCE_HWRAVENPOWER:
		SVHW_WriteEffectRavenStaff(index, message);
		break;
	case HWCE_HWDRILLA:
		SVHW_WriteEffectDrilla(index, message);
		break;
	case HWCE_DEATHBUBBLES:
		SVHW_WriteEffectDeathBubbles(index, message);
		break;
	case HWCE_SCARABCHAIN:
		SVHW_WriteEffectScarabChain(index, message);
		break;
	case HWCE_TRIPMINESTILL:
	case HWCE_TRIPMINE:
		SVHW_WriteEffectTripMine(index, message);
		break;
	case HWCE_HWSHEEPINATOR:
		SVHW_WriteEffectSheepinator(index, message);
		break;
	case HWCE_HWXBOWSHOOT:
		SVHW_WriteEffectXBowShoot(index, message);
		break;
	default:
		PR_RunError("SV_SendEffect: bad type");
		break;
	}
}

static void SVH2_SendEffect(QMsg* sb, int index)
{
	bool DoTest;
	if (sb == &sv.qh_reliable_datagram && sv_ce_scale->value > 0)
	{
		DoTest = true;
	}
	else
	{
		DoTest = false;
	}

	vec3_t TestO;
	float TestDistance;
	SVH2_GetSendEffectTestParams(index, DoTest, TestDistance, TestO);

	int count;
	if (!DoTest)
	{
		count = 1;
	}
	else
	{
		count = svs.qh_maxclients;
		TestDistance = (float)TestDistance * sv_ce_scale->value;
		TestDistance *= TestDistance;
	}

	for (int i = 0; i < count; i++)
	{
		if (DoTest)
		{
			if (svs.clients[i].state >= CS_CONNECTED)
			{
				sb = &svs.clients[i].datagram;
				vec3_t Diff;
				VectorSubtract(svs.clients[i].qh_edict->GetOrigin(), TestO, Diff);
				float Size = (Diff[0] * Diff[0]) + (Diff[1] * Diff[1]) + (Diff[2] * Diff[2]);

				if (Size > TestDistance)
				{
					continue;
				}

				if (sv_ce_max_size->value > 0 && sb->cursize > sv_ce_max_size->value)
				{
					continue;
				}
			}
			else
			{
				continue;
			}
		}

		SVH2_WriteEffect(index, *sb);
	}
}

void SVHW_SendEffect(QMsg* sb, int index)
{
	bool DoTest;
	if ((GGameType & GAME_HexenWorld || sb == &sv.qh_reliable_datagram) && sv_ce_scale->value > 0)
	{
		DoTest = true;
	}
	else
	{
		DoTest = false;
	}

	vec3_t TestO;
	SVHW_GetSendEffectTestParams(index, DoTest, TestO);

	SVHW_WriteEffect(index, sv.multicast);

	if (sb)
	{
		sb->WriteData(sv.multicast._data, sv.multicast.cursize);
		sv.multicast.Clear();
	}
	else
	{
		if (DoTest)
		{
			SVQH_Multicast(TestO, MULTICAST_PVS_R);
		}
		else
		{
			SVQH_Multicast(TestO, MULTICAST_ALL_R);
		}
		sv.h2_Effects[index].client_list = clients_multicast;
	}
}

void SVH2_UpdateEffects(QMsg* sb)
{
	for (int index = 0; index < MAX_EFFECTS_H2; index++)
	{
		if (sv.h2_Effects[index].type)
		{
			SVH2_SendEffect(sb,index);
		}
	}
}

static void SVH2_ParseEffectRain(int index)
{
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Rain.min_org);
	VectorCopy(G_VECTOR(OFS_PARM2), sv.h2_Effects[index].Rain.max_org);
	VectorCopy(G_VECTOR(OFS_PARM3), sv.h2_Effects[index].Rain.e_size);
	VectorCopy(G_VECTOR(OFS_PARM4), sv.h2_Effects[index].Rain.dir);
	sv.h2_Effects[index].Rain.color = G_FLOAT(OFS_PARM5);
	sv.h2_Effects[index].Rain.count = G_FLOAT(OFS_PARM6);
	sv.h2_Effects[index].Rain.wait = G_FLOAT(OFS_PARM7);

	sv.h2_Effects[index].Rain.next_time = 0;
}

static void SVH2_ParseEffectSnow(int index)
{
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Snow.min_org);
	VectorCopy(G_VECTOR(OFS_PARM2), sv.h2_Effects[index].Snow.max_org);
	sv.h2_Effects[index].Snow.flags = G_FLOAT(OFS_PARM3);
	VectorCopy(G_VECTOR(OFS_PARM4), sv.h2_Effects[index].Snow.dir);
	sv.h2_Effects[index].Snow.count = G_FLOAT(OFS_PARM5);

	sv.h2_Effects[index].Snow.next_time = 0;
}

static void SVH2_ParseEffectFountain(int index)
{
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Fountain.pos);
	VectorCopy(G_VECTOR(OFS_PARM2), sv.h2_Effects[index].Fountain.angle);
	VectorCopy(G_VECTOR(OFS_PARM3), sv.h2_Effects[index].Fountain.movedir);
	sv.h2_Effects[index].Fountain.color = G_FLOAT(OFS_PARM4);
	sv.h2_Effects[index].Fountain.cnt = G_FLOAT(OFS_PARM5);
}

static void SVH2_ParseEffectQuake(int index)
{
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Quake.origin);
	sv.h2_Effects[index].Quake.radius = G_FLOAT(OFS_PARM2);
}

static void SVH2_ParseEffectSmoke(int index)
{
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Smoke.origin);
	VectorCopy(G_VECTOR(OFS_PARM2), sv.h2_Effects[index].Smoke.velocity);
	sv.h2_Effects[index].Smoke.framelength = G_FLOAT(OFS_PARM3);
	if (!(GGameType & GAME_HexenWorld))
	{
		sv.h2_Effects[index].Smoke.frame = 0;
	}
	sv.h2_Effects[index].expire_time = sv.qh_time + 1000;
}

static void SVH2_ParseEffectFlameWall(int index)
{
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Smoke.origin);
	VectorCopy(G_VECTOR(OFS_PARM2), sv.h2_Effects[index].Smoke.velocity);
	sv.h2_Effects[index].Smoke.framelength = 0.05;
	sv.h2_Effects[index].Smoke.frame = G_FLOAT(OFS_PARM3);
	sv.h2_Effects[index].expire_time = sv.qh_time + 1000;
}

static void SVH2_ParseEffectSmokeFlash(int index)
{
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Smoke.origin);
	sv.h2_Effects[index].expire_time = sv.qh_time + 1000;
}

static void SVH2_ParseEffectFlash(int index)
{
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Flash.origin);
	sv.h2_Effects[index].expire_time = sv.qh_time + 1000;
}

static void SVH2_ParseEffectRiderDeath(int index)
{
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].RD.origin);
}

static void SVH2_ParseEffectTeleporterPuffs(int index)
{
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Teleporter.origin);
	sv.h2_Effects[index].expire_time = sv.qh_time + 1000;
}

static void SVH2_ParseEffectTeleporterBody(int index)
{
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Teleporter.origin);
	VectorCopy(G_VECTOR(OFS_PARM2), sv.h2_Effects[index].Teleporter.velocity[0]);
	sv.h2_Effects[index].Teleporter.skinnum = G_FLOAT(OFS_PARM3);
	sv.h2_Effects[index].expire_time = sv.qh_time + 1000;
}

static void SVH2_ParseEffectBoneShrapnel(int index)
{
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Missile.origin);
	VectorCopy(G_VECTOR(OFS_PARM2), sv.h2_Effects[index].Missile.velocity);
	VectorCopy(G_VECTOR(OFS_PARM3), sv.h2_Effects[index].Missile.angle);
	VectorCopy(G_VECTOR(OFS_PARM2), sv.h2_Effects[index].Missile.avelocity);

	sv.h2_Effects[index].expire_time = sv.qh_time + 10000;
}

static void SVH2_ParseEffectGravityWell(int index)
{
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].GravityWell.origin);
	sv.h2_Effects[index].GravityWell.color = G_FLOAT(OFS_PARM2);
	sv.h2_Effects[index].GravityWell.lifetime = G_FLOAT(OFS_PARM3);
}

static void SVH2_ParseEffectChunk(int index)
{
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Chunk.origin);
	sv.h2_Effects[index].Chunk.type = G_FLOAT(OFS_PARM2);
	VectorCopy(G_VECTOR(OFS_PARM3), sv.h2_Effects[index].Chunk.srcVel);
	sv.h2_Effects[index].Chunk.numChunks = G_FLOAT(OFS_PARM4);

	sv.h2_Effects[index].expire_time = sv.qh_time + 3000;
}

static void SVHW_ParseEffectRavenStaff(int index)
{
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Missile.origin);
	VectorCopy(G_VECTOR(OFS_PARM2), sv.h2_Effects[index].Missile.velocity);
	sv.h2_Effects[index].expire_time = sv.qh_time + 10000;
}

static void SVHW_ParseEffectDeathBubbles(int index)
{
	VectorCopy(G_VECTOR(OFS_PARM2), sv.h2_Effects[index].Bubble.offset);
	sv.h2_Effects[index].Bubble.owner = G_EDICTNUM(OFS_PARM1);
	sv.h2_Effects[index].Bubble.count = G_FLOAT(OFS_PARM3);
	sv.h2_Effects[index].expire_time = sv.qh_time + 30000;
}

static void SVHW_ParseEffectDrilla(int index)
{
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Missile.origin);
	VectorCopy(G_VECTOR(OFS_PARM2), sv.h2_Effects[index].Missile.angle);
	sv.h2_Effects[index].Missile.speed = G_FLOAT(OFS_PARM3);
	sv.h2_Effects[index].expire_time = sv.qh_time + 10000;
}

static void SVHW_ParseEffectTripMineStill(int index)
{
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Chain.origin);
	VectorCopy(G_VECTOR(OFS_PARM2), sv.h2_Effects[index].Chain.velocity);
	sv.h2_Effects[index].expire_time = sv.qh_time + 70000;
}

static void SVHW_ParseEffectTripMine(int index)
{
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Chain.origin);
	VectorCopy(G_VECTOR(OFS_PARM2), sv.h2_Effects[index].Chain.velocity);
	sv.h2_Effects[index].expire_time = sv.qh_time + 10000;
}

static void SVHW_ParseEffectScarabChain(int index)
{
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Chain.origin);
	sv.h2_Effects[index].Chain.owner = G_EDICTNUM(OFS_PARM2);
	sv.h2_Effects[index].Chain.material = G_INT(OFS_PARM3);
	sv.h2_Effects[index].Chain.tag = G_INT(OFS_PARM4);
	sv.h2_Effects[index].Chain.state = 0;
	sv.h2_Effects[index].expire_time = sv.qh_time + 15000;
}

static void SVHW_ParseEffectSheepinator(int index)
{
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Xbow.origin[0]);
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Xbow.origin[1]);
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Xbow.origin[2]);
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Xbow.origin[3]);
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Xbow.origin[4]);
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Xbow.origin[5]);
	VectorCopy(G_VECTOR(OFS_PARM2), sv.h2_Effects[index].Xbow.angle);
	sv.h2_Effects[index].Xbow.bolts = 5;
	sv.h2_Effects[index].Xbow.activebolts = 31;
	sv.h2_Effects[index].Xbow.randseed = 0;
	sv.h2_Effects[index].Xbow.turnedbolts = 0;
	sv.h2_Effects[index].expire_time = sv.qh_time + 7000;
}

static void SVHW_ParseEffectXBowShoot(int index)
{
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Xbow.origin[0]);
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Xbow.origin[1]);
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Xbow.origin[2]);
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Xbow.origin[3]);
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Xbow.origin[4]);
	VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Xbow.origin[5]);
	VectorCopy(G_VECTOR(OFS_PARM2), sv.h2_Effects[index].Xbow.angle);
	sv.h2_Effects[index].Xbow.bolts = G_FLOAT(OFS_PARM3);
	sv.h2_Effects[index].Xbow.randseed = G_FLOAT(OFS_PARM4);
	sv.h2_Effects[index].Xbow.turnedbolts = 0;
	if (sv.h2_Effects[index].Xbow.bolts == 3)
	{
		sv.h2_Effects[index].Xbow.activebolts = 7;
	}
	else
	{
		sv.h2_Effects[index].Xbow.activebolts = 31;
	}
	sv.h2_Effects[index].expire_time = sv.qh_time + 15000;
}

static void SVH2_ParseEffectDetails(int index)
{
	switch (sv.h2_Effects[index].type)
	{
	case H2CE_RAIN:
		SVH2_ParseEffectRain(index);
		break;
	case H2CE_SNOW:
		SVH2_ParseEffectSnow(index);
		break;
	case H2CE_FOUNTAIN:
		SVH2_ParseEffectFountain(index);
		break;
	case H2CE_QUAKE:
		SVH2_ParseEffectQuake(index);
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
		SVH2_ParseEffectSmoke(index);
		break;
	case H2CE_ACID_MUZZFL:
	case H2CE_FLAMESTREAM:
	case H2CE_FLAMEWALL:
	case H2CE_FLAMEWALL2:
	case H2CE_ONFIRE:
		SVH2_ParseEffectFlameWall(index);
		break;
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
	case H2CE_ACID_HIT:
	case H2CE_ACID_SPLAT:
	case H2CE_ACID_EXPL:
	case H2CE_LBALL_EXPL:
	case H2CE_FIREWALL_SMALL:
	case H2CE_FIREWALL_MEDIUM:
	case H2CE_FIREWALL_LARGE:
	case H2CE_FBOOM:
	case H2CE_BOMB:
	case H2CE_BRN_BOUNCE:
	case H2CE_LSHOCK:
		SVH2_ParseEffectSmokeFlash(index);
		break;
	case H2CE_WHITE_FLASH:
	case H2CE_BLUE_FLASH:
	case H2CE_SM_BLUE_FLASH:
	case H2CE_RED_FLASH:
		SVH2_ParseEffectFlash(index);
		break;
	case H2CE_RIDER_DEATH:
		SVH2_ParseEffectRiderDeath(index);
		break;
	case H2CE_TELEPORTERPUFFS:
		SVH2_ParseEffectTeleporterPuffs(index);
		break;
	case H2CE_TELEPORTERBODY:
		SVH2_ParseEffectTeleporterBody(index);
		break;
	case H2CE_BONESHARD:
	case H2CE_BONESHRAPNEL:
		SVH2_ParseEffectBoneShrapnel(index);
		break;
	case H2CE_GRAVITYWELL:
		SVH2_ParseEffectGravityWell(index);
		break;
	case H2CE_CHUNK:
		SVH2_ParseEffectChunk(index);
		break;
	default:
		PR_RunError("SVH2_SendEffect: bad type");
	}
}

static void SVHW_ParseEffectDetails(int index)
{
	switch (sv.h2_Effects[index].type)
	{
	case HWCE_RAIN:
		SVH2_ParseEffectRain(index);
		break;
	case HWCE_FOUNTAIN:
		SVH2_ParseEffectFountain(index);
		break;
	case HWCE_QUAKE:
		SVH2_ParseEffectQuake(index);
		break;
	case HWCE_WHITE_SMOKE:
	case HWCE_GREEN_SMOKE:
	case HWCE_GREY_SMOKE:
	case HWCE_RED_SMOKE:
	case HWCE_SLOW_WHITE_SMOKE:
	case HWCE_TELESMK1:
	case HWCE_TELESMK2:
	case HWCE_GHOST:
	case HWCE_REDCLOUD:
	case HWCE_RIPPLE:
		SVH2_ParseEffectSmoke(index);
		break;
	case HWCE_ACID_MUZZFL:
	case HWCE_FLAMESTREAM:
	case HWCE_FLAMEWALL:
	case HWCE_FLAMEWALL2:
	case HWCE_ONFIRE:
		SVH2_ParseEffectFlameWall(index);
		break;
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
	case HWCE_ACID_HIT:
	case HWCE_ACID_SPLAT:
	case HWCE_ACID_EXPL:
	case HWCE_LBALL_EXPL:
	case HWCE_FIREWALL_SMALL:
	case HWCE_FIREWALL_MEDIUM:
	case HWCE_FIREWALL_LARGE:
	case HWCE_FBOOM:
	case HWCE_BOMB:
	case HWCE_BRN_BOUNCE:
	case HWCE_LSHOCK:
		SVH2_ParseEffectSmokeFlash(index);
		break;
	case HWCE_WHITE_FLASH:
	case HWCE_BLUE_FLASH:
	case HWCE_SM_BLUE_FLASH:
	case HWCE_HWSPLITFLASH:
	case HWCE_RED_FLASH:
		SVH2_ParseEffectFlash(index);
		break;
	case HWCE_RIDER_DEATH:
		SVH2_ParseEffectRiderDeath(index);
		break;
	case HWCE_TELEPORTERPUFFS:
		SVH2_ParseEffectTeleporterPuffs(index);
		break;
	case HWCE_TELEPORTERBODY:
		SVH2_ParseEffectTeleporterBody(index);
		break;
	case HWCE_BONESHRAPNEL:
	case HWCE_HWBONEBALL:
		SVH2_ParseEffectBoneShrapnel(index);
		break;
	case HWCE_BONESHARD:
	case HWCE_HWRAVENSTAFF:
	case HWCE_HWMISSILESTAR:
	case HWCE_HWEIDOLONSTAR:
	case HWCE_HWRAVENPOWER:
		SVHW_ParseEffectRavenStaff(index);
		break;
	case HWCE_DEATHBUBBLES:
		SVHW_ParseEffectDeathBubbles(index);
		break;
	case HWCE_HWDRILLA:
		SVHW_ParseEffectDrilla(index);
		break;
	case HWCE_TRIPMINESTILL:
		SVHW_ParseEffectTripMineStill(index);
		break;
	case HWCE_TRIPMINE:
		SVHW_ParseEffectTripMine(index);
		break;
	case HWCE_SCARABCHAIN:
		SVHW_ParseEffectScarabChain(index);
		break;
	case HWCE_HWSHEEPINATOR:
		SVHW_ParseEffectSheepinator(index);
		break;
	case HWCE_HWXBOWSHOOT:
		SVHW_ParseEffectXBowShoot(index);
		break;
	default:
		PR_RunError("SV_SendEffect: bad type");
	}
}

void SVH2_ParseEffect(QMsg* sb)
{
	byte effect = G_FLOAT(OFS_PARM0);

	int index;
	for (index = 0; index < MAX_EFFECTS_H2; index++)
	{
		if (!sv.h2_Effects[index].type ||
			(sv.h2_Effects[index].expire_time && sv.h2_Effects[index].expire_time <= sv.qh_time))
		{
			break;
		}
	}

	if (index >= MAX_EFFECTS_H2)
	{
		PR_RunError("MAX_EFFECTS_H2 reached");
		return;
	}

	Com_Memset(&sv.h2_Effects[index], 0, sizeof(struct h2EffectT));

	sv.h2_Effects[index].type = effect;
	G_FLOAT(OFS_RETURN) = index;

	if (GGameType & GAME_HexenWorld)
	{
		SVHW_ParseEffectDetails(index);
		SVHW_SendEffect(sb,index);
	}
	else
	{
		SVH2_ParseEffectDetails(index);
		SVH2_SendEffect(sb,index);
	}
}

static void SVHW_ParseMultiEffectRavenPower(QMsg* sb, byte effect)
{
	// need to set aside 3 effect ids

	sb->WriteByte(hwsvc_multieffect);
	sb->WriteByte(effect);

	vec3_t orig;
	VectorCopy(G_VECTOR(OFS_PARM1), orig);
	sb->WriteCoord(orig[0]);
	sb->WriteCoord(orig[1]);
	sb->WriteCoord(orig[2]);
	vec3_t vel;
	VectorCopy(G_VECTOR(OFS_PARM2), vel);
	sb->WriteCoord(vel[0]);
	sb->WriteCoord(vel[1]);
	sb->WriteCoord(vel[2]);
	for (int count = 0; count < 3; count++)
	{
		int index;
		for (index = 0; index < MAX_EFFECTS_H2; index++)
		{
			if (!sv.h2_Effects[index].type ||
				(sv.h2_Effects[index].expire_time && sv.h2_Effects[index].expire_time <= sv.qh_time))
			{
				break;
			}
		}
		if (index >= MAX_EFFECTS_H2)
		{
			PR_RunError("MAX_EFFECTS_H2 reached");
			return;
		}
		sb->WriteByte(index);
		sv.h2_Effects[index].type = HWCE_HWRAVENPOWER;
		VectorCopy(orig, sv.h2_Effects[index].Missile.origin);
		VectorCopy(vel, sv.h2_Effects[index].Missile.velocity);
		sv.h2_Effects[index].expire_time = sv.qh_time + 10000;
		MultiEffectIds[count] = index;
	}
}

void SVHW_ParseMultiEffect(QMsg* sb)
{
	MultiEffectIdCount = 0;
	byte effect = G_FLOAT(OFS_PARM0);
	switch (effect)
	{
	case HWCE_HWRAVENPOWER:
		SVHW_ParseMultiEffectRavenPower(sb, effect);
		break;
	default:
		PR_RunError("SVHW_ParseMultiEffect: bad type");
	}
}

float SVHW_GetMultiEffectId()
{
	MultiEffectIdCount++;
	return MultiEffectIds[MultiEffectIdCount - 1];
}

void SVH2_SaveEffects(fileHandle_t FH)
{
	int count = 0;
	for (int index = 0; index < MAX_EFFECTS_H2; index++)
	{
		if (sv.h2_Effects[index].type)
		{
			count++;
		}
	}

	FS_Printf(FH,"Effects: %d\n", count);

	for (int index = 0; index < MAX_EFFECTS_H2; index++)
	{
		if (sv.h2_Effects[index].type)
		{
			FS_Printf(FH,"Effect: %d %d %f: ", index, sv.h2_Effects[index].type, sv.h2_Effects[index].expire_time * 0.001f);

			switch (sv.h2_Effects[index].type)
			{
			case H2CE_RAIN:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Rain.min_org[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Rain.min_org[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Rain.min_org[2]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Rain.max_org[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Rain.max_org[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Rain.max_org[2]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Rain.e_size[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Rain.e_size[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Rain.e_size[2]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Rain.dir[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Rain.dir[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Rain.dir[2]);
				FS_Printf(FH, "%d ", sv.h2_Effects[index].Rain.color);
				FS_Printf(FH, "%d ", sv.h2_Effects[index].Rain.count);
				FS_Printf(FH, "%f\n", sv.h2_Effects[index].Rain.wait);
				break;

			case H2CE_SNOW:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Snow.min_org[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Snow.min_org[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Snow.min_org[2]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Snow.max_org[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Snow.max_org[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Snow.max_org[2]);
				FS_Printf(FH, "%d ", sv.h2_Effects[index].Snow.flags);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Snow.dir[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Snow.dir[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Snow.dir[2]);
				FS_Printf(FH, "%d ", sv.h2_Effects[index].Snow.count);
				break;

			case H2CE_FOUNTAIN:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Fountain.pos[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Fountain.pos[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Fountain.pos[2]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Fountain.angle[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Fountain.angle[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Fountain.angle[2]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Fountain.movedir[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Fountain.movedir[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Fountain.movedir[2]);
				FS_Printf(FH, "%d ", sv.h2_Effects[index].Fountain.color);
				FS_Printf(FH, "%d\n", sv.h2_Effects[index].Fountain.cnt);
				break;

			case H2CE_QUAKE:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Quake.origin[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Quake.origin[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Quake.origin[2]);
				FS_Printf(FH, "%f\n", sv.h2_Effects[index].Quake.radius);
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
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Smoke.origin[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Smoke.origin[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Smoke.origin[2]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Smoke.velocity[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Smoke.velocity[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Smoke.velocity[2]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Smoke.framelength);
				FS_Printf(FH, "%f\n", sv.h2_Effects[index].Smoke.frame);
				break;

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
			case H2CE_FIREWALL_SMALL:
			case H2CE_FIREWALL_MEDIUM:
			case H2CE_FIREWALL_LARGE:
			case H2CE_FBOOM:
			case H2CE_BOMB:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Smoke.origin[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Smoke.origin[1]);
				FS_Printf(FH, "%f\n", sv.h2_Effects[index].Smoke.origin[2]);
				break;

			case H2CE_WHITE_FLASH:
			case H2CE_BLUE_FLASH:
			case H2CE_SM_BLUE_FLASH:
			case H2CE_RED_FLASH:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Flash.origin[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Flash.origin[1]);
				FS_Printf(FH, "%f\n", sv.h2_Effects[index].Flash.origin[2]);
				break;

			case H2CE_RIDER_DEATH:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].RD.origin[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].RD.origin[1]);
				FS_Printf(FH, "%f\n", sv.h2_Effects[index].RD.origin[2]);
				break;

			case H2CE_GRAVITYWELL:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].GravityWell.origin[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].GravityWell.origin[1]);
				FS_Printf(FH, "%f", sv.h2_Effects[index].GravityWell.origin[2]);
				FS_Printf(FH, "%d", sv.h2_Effects[index].GravityWell.color);
				FS_Printf(FH, "%f\n", sv.h2_Effects[index].GravityWell.lifetime);
				break;
			case H2CE_TELEPORTERPUFFS:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Teleporter.origin[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Teleporter.origin[1]);
				FS_Printf(FH, "%f\n", sv.h2_Effects[index].Teleporter.origin[2]);
				break;

			case H2CE_TELEPORTERBODY:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Teleporter.origin[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Teleporter.origin[1]);
				FS_Printf(FH, "%f\n", sv.h2_Effects[index].Teleporter.origin[2]);
				break;

			case H2CE_BONESHARD:
			case H2CE_BONESHRAPNEL:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Missile.origin[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Missile.origin[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Missile.origin[2]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Missile.velocity[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Missile.velocity[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Missile.velocity[2]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Missile.angle[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Missile.angle[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Missile.angle[2]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Missile.avelocity[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Missile.avelocity[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Missile.avelocity[2]);
				break;

			case H2CE_CHUNK:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Chunk.origin[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Chunk.origin[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Chunk.origin[2]);
				FS_Printf(FH, "%d ", sv.h2_Effects[index].Chunk.type);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Chunk.srcVel[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Chunk.srcVel[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Chunk.srcVel[2]);
				FS_Printf(FH, "%d ", sv.h2_Effects[index].Chunk.numChunks);
				break;

			default:
				PR_RunError("SV_SaveEffect: bad type");
				break;
			}
		}
	}
}

static int GetInt(const char*& Data)
{
	//	Skip whitespace.
	while (*Data && *Data <= ' ')
	{
		Data++;
	}
	char Tmp[32];
	int Len = 0;
	while ((*Data >= '0' && *Data <= '9') || *Data == '-')
	{
		if (Len >= (int)sizeof(Tmp) - 1)
		{
			Tmp[31] = 0;
			common->FatalError("Number too long %s", Tmp);
		}
		Tmp[Len] = *Data++;
		Len++;
	}
	Tmp[Len] = 0;
	return String::Atoi(Tmp);
}

static float GetFloat(const char*& Data)
{
	//	Skip whitespace.
	while (*Data && *Data <= ' ')
	{
		Data++;
	}
	char Tmp[32];
	int Len = 0;
	while ((*Data >= '0' && *Data <= '9') || *Data == '-' || *Data == '.')
	{
		if (Len >= (int)sizeof(Tmp) - 1)
		{
			Tmp[31] = 0;
			common->FatalError("Number too long %s", Tmp);
		}
		Tmp[Len] = *Data++;
		Len++;
	}
	Tmp[Len] = 0;
	return String::Atof(Tmp);
}

const char* SVH2_LoadEffects(const char* Data)
{
	// Since the map is freshly loaded, clear out any effects as a result of
	// the loading
	SVH2_ClearEffects();

	if (String::NCmp(Data, "Effects: ", 9))
	{
		common->Error("Effects expected");
	}
	Data += 9;
	int Total = GetInt(Data);

	for (int count = 0; count < Total; count++)
	{
		//	Skip whitespace.
		while (*Data && *Data <= ' ')
			Data++;
		if (String::NCmp(Data, "Effect: ", 8))
		{
			common->FatalError("Effect expected");
		}
		Data += 8;
		int index = GetInt(Data);
		sv.h2_Effects[index].type = GetInt(Data);
		sv.h2_Effects[index].expire_time = GetFloat(Data) * 1000;
		if (*Data != ':')
		{
			common->FatalError("Colon expected");
		}
		Data++;

		switch (sv.h2_Effects[index].type)
		{
		case H2CE_RAIN:
			sv.h2_Effects[index].Rain.min_org[0] = GetFloat(Data);
			sv.h2_Effects[index].Rain.min_org[1] = GetFloat(Data);
			sv.h2_Effects[index].Rain.min_org[2] = GetFloat(Data);
			sv.h2_Effects[index].Rain.max_org[0] = GetFloat(Data);
			sv.h2_Effects[index].Rain.max_org[1] = GetFloat(Data);
			sv.h2_Effects[index].Rain.max_org[2] = GetFloat(Data);
			sv.h2_Effects[index].Rain.e_size[0] = GetFloat(Data);
			sv.h2_Effects[index].Rain.e_size[1] = GetFloat(Data);
			sv.h2_Effects[index].Rain.e_size[2] = GetFloat(Data);
			sv.h2_Effects[index].Rain.dir[0] = GetFloat(Data);
			sv.h2_Effects[index].Rain.dir[1] = GetFloat(Data);
			sv.h2_Effects[index].Rain.dir[2] = GetFloat(Data);
			sv.h2_Effects[index].Rain.color = GetInt(Data);
			sv.h2_Effects[index].Rain.count = GetInt(Data);
			sv.h2_Effects[index].Rain.wait = GetFloat(Data);
			break;

		case H2CE_SNOW:
			sv.h2_Effects[index].Snow.min_org[0] = GetFloat(Data);
			sv.h2_Effects[index].Snow.min_org[1] = GetFloat(Data);
			sv.h2_Effects[index].Snow.min_org[2] = GetFloat(Data);
			sv.h2_Effects[index].Snow.max_org[0] = GetFloat(Data);
			sv.h2_Effects[index].Snow.max_org[1] = GetFloat(Data);
			sv.h2_Effects[index].Snow.max_org[2] = GetFloat(Data);
			sv.h2_Effects[index].Snow.flags = GetInt(Data);
			sv.h2_Effects[index].Snow.dir[0] = GetFloat(Data);
			sv.h2_Effects[index].Snow.dir[1] = GetFloat(Data);
			sv.h2_Effects[index].Snow.dir[2] = GetFloat(Data);
			sv.h2_Effects[index].Snow.count = GetInt(Data);
			break;

		case H2CE_FOUNTAIN:
			sv.h2_Effects[index].Fountain.pos[0] = GetFloat(Data);
			sv.h2_Effects[index].Fountain.pos[1] = GetFloat(Data);
			sv.h2_Effects[index].Fountain.pos[2] = GetFloat(Data);
			sv.h2_Effects[index].Fountain.angle[0] = GetFloat(Data);
			sv.h2_Effects[index].Fountain.angle[1] = GetFloat(Data);
			sv.h2_Effects[index].Fountain.angle[2] = GetFloat(Data);
			sv.h2_Effects[index].Fountain.movedir[0] = GetFloat(Data);
			sv.h2_Effects[index].Fountain.movedir[1] = GetFloat(Data);
			sv.h2_Effects[index].Fountain.movedir[2] = GetFloat(Data);
			sv.h2_Effects[index].Fountain.color = GetInt(Data);
			sv.h2_Effects[index].Fountain.cnt = GetInt(Data);
			break;

		case H2CE_QUAKE:
			sv.h2_Effects[index].Quake.origin[0] = GetFloat(Data);
			sv.h2_Effects[index].Quake.origin[1] = GetFloat(Data);
			sv.h2_Effects[index].Quake.origin[2] = GetFloat(Data);
			sv.h2_Effects[index].Quake.radius = GetFloat(Data);
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
			sv.h2_Effects[index].Smoke.origin[0] = GetFloat(Data);
			sv.h2_Effects[index].Smoke.origin[1] = GetFloat(Data);
			sv.h2_Effects[index].Smoke.origin[2] = GetFloat(Data);
			sv.h2_Effects[index].Smoke.velocity[0] = GetFloat(Data);
			sv.h2_Effects[index].Smoke.velocity[1] = GetFloat(Data);
			sv.h2_Effects[index].Smoke.velocity[2] = GetFloat(Data);
			sv.h2_Effects[index].Smoke.framelength = GetFloat(Data);
			sv.h2_Effects[index].Smoke.frame = GetFloat(Data);
			break;

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
		case H2CE_FIREWALL_SMALL:
		case H2CE_FIREWALL_MEDIUM:
		case H2CE_FIREWALL_LARGE:
		case H2CE_BOMB:
			sv.h2_Effects[index].Smoke.origin[0] = GetFloat(Data);
			sv.h2_Effects[index].Smoke.origin[1] = GetFloat(Data);
			sv.h2_Effects[index].Smoke.origin[2] = GetFloat(Data);
			break;

		case H2CE_WHITE_FLASH:
		case H2CE_BLUE_FLASH:
		case H2CE_SM_BLUE_FLASH:
		case H2CE_RED_FLASH:
			sv.h2_Effects[index].Flash.origin[0] = GetFloat(Data);
			sv.h2_Effects[index].Flash.origin[1] = GetFloat(Data);
			sv.h2_Effects[index].Flash.origin[2] = GetFloat(Data);
			break;

		case H2CE_RIDER_DEATH:
			sv.h2_Effects[index].RD.origin[0] = GetFloat(Data);
			sv.h2_Effects[index].RD.origin[1] = GetFloat(Data);
			sv.h2_Effects[index].RD.origin[2] = GetFloat(Data);
			break;

		case H2CE_GRAVITYWELL:
			sv.h2_Effects[index].GravityWell.origin[0] = GetFloat(Data);
			sv.h2_Effects[index].GravityWell.origin[1] = GetFloat(Data);
			sv.h2_Effects[index].GravityWell.origin[2] = GetFloat(Data);
			sv.h2_Effects[index].GravityWell.color = GetInt(Data);
			sv.h2_Effects[index].GravityWell.lifetime = GetFloat(Data);
			break;

		case H2CE_TELEPORTERPUFFS:
			sv.h2_Effects[index].Teleporter.origin[0] = GetFloat(Data);
			sv.h2_Effects[index].Teleporter.origin[1] = GetFloat(Data);
			sv.h2_Effects[index].Teleporter.origin[2] = GetFloat(Data);
			break;

		case H2CE_TELEPORTERBODY:
			sv.h2_Effects[index].Teleporter.origin[0] = GetFloat(Data);
			sv.h2_Effects[index].Teleporter.origin[1] = GetFloat(Data);
			sv.h2_Effects[index].Teleporter.origin[2] = GetFloat(Data);
			break;

		case H2CE_BONESHARD:
		case H2CE_BONESHRAPNEL:
			sv.h2_Effects[index].Missile.origin[0] = GetFloat(Data);
			sv.h2_Effects[index].Missile.origin[1] = GetFloat(Data);
			sv.h2_Effects[index].Missile.origin[2] = GetFloat(Data);
			sv.h2_Effects[index].Missile.velocity[0] = GetFloat(Data);
			sv.h2_Effects[index].Missile.velocity[1] = GetFloat(Data);
			sv.h2_Effects[index].Missile.velocity[2] = GetFloat(Data);
			sv.h2_Effects[index].Missile.angle[0] = GetFloat(Data);
			sv.h2_Effects[index].Missile.angle[1] = GetFloat(Data);
			sv.h2_Effects[index].Missile.angle[2] = GetFloat(Data);
			sv.h2_Effects[index].Missile.avelocity[0] = GetFloat(Data);
			sv.h2_Effects[index].Missile.avelocity[1] = GetFloat(Data);
			sv.h2_Effects[index].Missile.avelocity[2] = GetFloat(Data);
			break;

		case H2CE_CHUNK:
			sv.h2_Effects[index].Chunk.origin[0] = GetFloat(Data);
			sv.h2_Effects[index].Chunk.origin[1] = GetFloat(Data);
			sv.h2_Effects[index].Chunk.origin[2] = GetFloat(Data);
			sv.h2_Effects[index].Chunk.type = GetInt(Data);
			sv.h2_Effects[index].Chunk.srcVel[0] = GetFloat(Data);
			sv.h2_Effects[index].Chunk.srcVel[1] = GetFloat(Data);
			sv.h2_Effects[index].Chunk.srcVel[2] = GetFloat(Data);
			sv.h2_Effects[index].Chunk.numChunks = GetInt(Data);
			break;

		default:
			PR_RunError("SV_SaveEffect: bad type");
			break;
		}
	}
	return Data;
}
