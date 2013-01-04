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

#include "local.h"
#include "../particles.h"
#include "../dynamic_lights.h"
#include "../../../common/strings.h"

static sfxHandle_t clq2_sfx_ric1;
static sfxHandle_t clq2_sfx_ric2;
static sfxHandle_t clq2_sfx_ric3;
static sfxHandle_t clq2_sfx_lashit;
static sfxHandle_t clq2_sfx_spark5;
static sfxHandle_t clq2_sfx_spark6;
static sfxHandle_t clq2_sfx_spark7;
static sfxHandle_t clq2_sfx_railg;
static sfxHandle_t clq2_sfx_rockexp;
static sfxHandle_t clq2_sfx_grenexp;
static sfxHandle_t clq2_sfx_watrexp;
sfxHandle_t clq2_sfx_footsteps[4];

static qhandle_t clq2_mod_parasite_tip;
qhandle_t clq2_mod_powerscreen;

static sfxHandle_t clq2_sfx_lightning;
static sfxHandle_t clq2_sfx_disrexp;

static byte splash_color[] = { 0x00, 0xe0, 0xb0, 0x50, 0xd0, 0xe0, 0xe8 };

void CLQ2_RegisterTEntSounds()
{
	clq2_sfx_ric1 = S_RegisterSound("world/ric1.wav");
	clq2_sfx_ric2 = S_RegisterSound("world/ric2.wav");
	clq2_sfx_ric3 = S_RegisterSound("world/ric3.wav");
	clq2_sfx_lashit = S_RegisterSound("weapons/lashit.wav");
	clq2_sfx_spark5 = S_RegisterSound("world/spark5.wav");
	clq2_sfx_spark6 = S_RegisterSound("world/spark6.wav");
	clq2_sfx_spark7 = S_RegisterSound("world/spark7.wav");
	clq2_sfx_railg = S_RegisterSound("weapons/railgf1a.wav");
	clq2_sfx_rockexp = S_RegisterSound("weapons/rocklx1a.wav");
	clq2_sfx_grenexp = S_RegisterSound("weapons/grenlx1a.wav");
	clq2_sfx_watrexp = S_RegisterSound("weapons/xpld_wat.wav");
	S_RegisterSound("player/land1.wav");

	S_RegisterSound("player/fall2.wav");
	S_RegisterSound("player/fall1.wav");

	for (int i = 0; i < 4; i++)
	{
		char name[MAX_QPATH];
		String::Sprintf(name, sizeof(name), "player/step%i.wav", i + 1);
		clq2_sfx_footsteps[i] = S_RegisterSound(name);
	}

	clq2_sfx_lightning = S_RegisterSound("weapons/tesla.wav");
	clq2_sfx_disrexp = S_RegisterSound("weapons/disrupthit.wav");
}

void CLQ2_RegisterTEntModels()
{
	CLQ2_RegisterExplosionModels();
	CLQ2_RegisterBeamModels();
	clq2_mod_parasite_tip = R_RegisterModel("models/monsters/parasite/tip/tris.md2");
	clq2_mod_powerscreen = R_RegisterModel("models/items/armor/effect/tris.md2");

	R_RegisterModel("models/objects/laser/tris.md2");
	R_RegisterModel("models/objects/grenade2/tris.md2");
	R_RegisterModel("models/weapons/v_machn/tris.md2");
	R_RegisterModel("models/weapons/v_handgr/tris.md2");
	R_RegisterModel("models/weapons/v_shotg2/tris.md2");
	R_RegisterModel("models/objects/gibs/bone/tris.md2");
	R_RegisterModel("models/objects/gibs/sm_meat/tris.md2");
	R_RegisterModel("models/objects/gibs/bone2/tris.md2");

	R_RegisterPic("w_machinegun");
	R_RegisterPic("a_bullets");
	R_RegisterPic("i_health");
	R_RegisterPic("a_grenades");
}

void CLQ2_ClearTEnts()
{
	CLQ2_ClearBeams();
	CLQ2_ClearExplosions();
	CLQ2_ClearLasers();
	CLQ2_ClearSustains();
}

static void CLQ2_ParseBlood(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t dir;
	message.ReadDir(dir);

	CLQ2_ParticleEffect(pos, dir, 0xe8, 60);
}

static void CLQ2_ImpactSound(vec3_t pos)
{
	int cnt = rand() & 15;
	if (cnt == 1)
	{
		S_StartSound(pos, 0, 0, clq2_sfx_ric1, 1, ATTN_NORM, 0);
	}
	else if (cnt == 2)
	{
		S_StartSound(pos, 0, 0, clq2_sfx_ric2, 1, ATTN_NORM, 0);
	}
	else if (cnt == 3)
	{
		S_StartSound(pos, 0, 0, clq2_sfx_ric3, 1, ATTN_NORM, 0);
	}
}

static void CLQ2_ParseGunShot(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t dir;
	message.ReadDir(dir);

	CLQ2_ParticleEffect(pos, dir, 0, 40);
	CLQ2_SmokeAndFlash(pos);
	CLQ2_ImpactSound(pos);
}

static void CLQ2_ParseSparks(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t dir;
	message.ReadDir(dir);

	CLQ2_ParticleEffect(pos, dir, 0xe0, 6);
}

static void CLQ2_ParseBulletSparks(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t dir;
	message.ReadDir(dir);

	CLQ2_ParticleEffect(pos, dir, 0xe0, 6);
	CLQ2_SmokeAndFlash(pos);
	CLQ2_ImpactSound(pos);
}

static void CLQ2_ParseScreenSparks(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t dir;
	message.ReadDir(dir);

	CLQ2_ParticleEffect(pos, dir, 0xd0, 40);
	S_StartSound(pos, 0, 0, clq2_sfx_lashit, 1, ATTN_NORM, 0);
}

static void CLQ2_ParseShieldSparks(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t dir;
	message.ReadDir(dir);

	CLQ2_ParticleEffect(pos, dir, 0xb0, 40);
	S_StartSound(pos, 0, 0, clq2_sfx_lashit, 1, ATTN_NORM, 0);
}

static void CLQ2_ParseShotgun(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t dir;
	message.ReadDir(dir);

	CLQ2_ParticleEffect(pos, dir, 0, 20);
	CLQ2_SmokeAndFlash(pos);
}

static void CLQ2_ParseSplash(QMsg& message)
{
	int cnt = message.ReadByte();
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t dir;
	message.ReadDir(dir);
	int r = message.ReadByte();

	int color = r > 6 ? 0x00 : splash_color[r];
	CLQ2_ParticleEffect(pos, dir, color, cnt);
	if (r == Q2SPLASH_SPARKS)
	{
		r = rand() & 3;
		if (r == 0)
		{
			S_StartSound(pos, 0, 0, clq2_sfx_spark5, 1, ATTN_STATIC, 0);
		}
		else if (r == 1)
		{
			S_StartSound(pos, 0, 0, clq2_sfx_spark6, 1, ATTN_STATIC, 0);
		}
		else
		{
			S_StartSound(pos, 0, 0, clq2_sfx_spark7, 1, ATTN_STATIC, 0);
		}
	}
}

static void CLQ2_ParseLaserSparks(QMsg& message)
{
	int cnt = message.ReadByte();
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t dir;
	message.ReadDir(dir);
	int color = message.ReadByte();

	CLQ2_ParticleEffect2(pos, dir, color, cnt);
}

static void CLQ2_ParseBlueHyperBlaster(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t dir;
	message.ReadPos(dir);

	CLQ2_BlasterParticles(pos, dir);
}

static void CLQ2_ParseBlaster(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t dir;
	message.ReadDir(dir);

	CLQ2_BlasterParticles(pos, dir);
	CLQ2_BlasterExplosion(pos, dir);
	S_StartSound(pos, 0, 0, clq2_sfx_lashit, 1, ATTN_NORM, 0);
}

static void CLQ2_ParseRailTrail(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t pos2;
	message.ReadPos(pos2);

	CLQ2_RailTrail(pos, pos2);
	S_StartSound(pos2, 0, 0, clq2_sfx_railg, 1, ATTN_NORM, 0);
}

static void CLQ2_ParseGrenadeExplosion(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	CLQ2_GrenadeExplosion(pos);
	CLQ2_ExplosionParticles(pos);
	S_StartSound(pos, 0, 0, clq2_sfx_grenexp, 1, ATTN_NORM, 0);
}

static void CLQ2_ParseGrenadeExplosionWater(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	CLQ2_GrenadeExplosion(pos);
	CLQ2_ExplosionParticles(pos);
	S_StartSound(pos, 0, 0, clq2_sfx_watrexp, 1, ATTN_NORM, 0);
}

static void CLQ2_ParsePlasmaExplosion(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	CLQ2_RocketExplosion(pos);
	CLQ2_ExplosionParticles(pos);
	S_StartSound(pos, 0, 0, clq2_sfx_rockexp, 1, ATTN_NORM, 0);
}

static void CLQ2_ParseRocketExplosion(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	CLQ2_RocketExplosion(pos);
	CLQ2_ExplosionParticles(pos);
	S_StartSound(pos, 0, 0, clq2_sfx_rockexp, 1, ATTN_NORM, 0);
}

static void CLQ2_ParseExplosion1Big(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	CLQ2_BigExplosion(pos);
	S_StartSound(pos, 0, 0, clq2_sfx_rockexp, 1, ATTN_NORM, 0);
}

static void CLQ2_ParseRocketExplosionWater(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	CLQ2_RocketExplosion(pos);
	CLQ2_ExplosionParticles(pos);
	S_StartSound(pos, 0, 0, clq2_sfx_watrexp, 1, ATTN_NORM, 0);
}

static void CLQ2_ParsePlainExplosion(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	CLQ2_RocketExplosion(pos);
	S_StartSound(pos, 0, 0, clq2_sfx_rockexp, 1, ATTN_NORM, 0);
}

static void CLQ2_ParseBfgExplosion(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	CLQ2_BfgExplosion(pos);
}

static void CLQ2_ParseBfgBigExplosion(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	CLQ2_BFGExplosionParticles(pos);
}

static void CLQ2_ParseBfgLaser(QMsg& message)
{
	vec3_t start;
	message.ReadPos(start);
	vec3_t end;
	message.ReadPos(end);

	CLQ2_NewLaser(start, end, 0xd0d1d2d3);
}

static void CLQ2_ParseBubleTrail(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t pos2;
	message.ReadPos(pos2);

	CLQ2_BubbleTrail(pos, pos2);
}

static void CLQ2_ParseParasiteAttack(QMsg& message)
{
	int ent = message.ReadShort();
	vec3_t start;
	message.ReadPos(start);
	vec3_t end;
	message.ReadPos(end);

	CLQ2_ParasiteBeam(ent, start, end);
}

static void CLQ2_ParseBossTeleport(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	CLQ2_BigTeleportParticles(pos);
	S_StartSound(pos, 0, 0, S_RegisterSound("misc/bigtele.wav"), 1, ATTN_NONE, 0);
}

static void CLQ2_ParseGrappleCable(QMsg& message)
{
	int ent = message.ReadShort();
	vec3_t start;
	message.ReadPos(start);
	vec3_t end;
	message.ReadPos(end);
	vec3_t offset;
	message.ReadPos(offset);

	CLQ2_GrappleCableBeam(ent, start, end, offset);
}

static void CLQ2_ParseWeldingSparks(QMsg& message)
{
	int cnt = message.ReadByte();
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t dir;
	message.ReadDir(dir);
	int color = message.ReadByte();

	CLQ2_ParticleEffect2(pos, dir, color, cnt);
	CLQ2_WeldingSparks(pos);
}

static void CLQ2_ParseGreenBlood(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t dir;
	message.ReadDir(dir);

	CLQ2_ParticleEffect2(pos, dir, 0xdf, 30);
}

static void CLQ2_ParseTunnelSparks(QMsg& message)
{
	int cnt = message.ReadByte();
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t dir;
	message.ReadDir(dir);
	int color = message.ReadByte();

	CLQ2_ParticleEffect3(pos, dir, color, cnt);
}

static void CLQ2_ParseBlaster2(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t dir;
	message.ReadDir(dir);

	CLQ2_BlasterParticles2(pos, dir, 0xd0);
	CLQ2_Blaster2Explosion(pos, dir);
	S_StartSound(pos, 0, 0, clq2_sfx_lashit, 1, ATTN_NORM, 0);
}

static void CLQ2_ParseFlechette(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t dir;
	message.ReadDir(dir);

	CLQ2_BlasterParticles2(pos, dir, 0x6f);	// 75
	CLQ2_FlechetteExplosion(pos, dir);
	S_StartSound(pos, 0, 0, clq2_sfx_lashit, 1, ATTN_NORM, 0);
}

static void CLQ2_ParseLightning(QMsg& message)
{
	int srcEnt = message.ReadShort();
	int destEnt = message.ReadShort();
	vec3_t start;
	message.ReadPos(start);
	vec3_t end;
	message.ReadPos(end);

	CLQ2_LightningBeam(srcEnt, destEnt, start, end);
	S_StartSound(NULL, srcEnt, Q2CHAN_WEAPON, clq2_sfx_lightning, 1, ATTN_NORM, 0);
}

static void CLQ2_ParseDebugTrail(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t pos2;
	message.ReadPos(pos2);

	CLQ2_DebugTrail(pos, pos2);
}

static void CLQ2_ParseFlashLight(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	int ent = message.ReadShort();

	CLQ2_Flashlight(ent, pos);
}

static void CLQ2_ParseForceWall(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t pos2;
	message.ReadPos(pos2);
	int color = message.ReadByte();

	CLQ2_ForceWall(pos, pos2, color);
}

static void CLQ2_ParseHeatBeam(QMsg& message)
{
	int ent = message.ReadShort();
	vec3_t start;
	message.ReadPos(start);
	vec3_t end;
	message.ReadPos(end);

	CLQ2_HeatBeam(ent, start, end);
}

static void CLQ2_ParseMonsterHeatBeam(QMsg& message)
{
	int ent = message.ReadShort();
	vec3_t start;
	message.ReadPos(start);
	vec3_t end;
	message.ReadPos(end);

	CLQ2_MonsterHeatBeam(ent, start, end);
}

static void CLQ2_ParseHeatBeamSparks(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t dir;
	message.ReadDir(dir);

	CLQ2_ParticleSteamEffect(pos, dir, 8, 50, 60);
	S_StartSound(pos, 0, 0, clq2_sfx_lashit, 1, ATTN_NORM, 0);
}

static void CLQ2_ParseHeatBeamSteam(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t dir;
	message.ReadDir(dir);

	CLQ2_ParticleSteamEffect(pos, dir, 0xe0, 20, 60);
	S_StartSound(pos, 0, 0, clq2_sfx_lashit, 1, ATTN_NORM, 0);
}

static void CLQ2_ParseSteam(QMsg& message)
{
	int id = message.ReadShort();		// an id of -1 is an instant effect
	int cnt = message.ReadByte();
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t dir;
	message.ReadDir(dir);
	int r = message.ReadByte();
	int magnitude = message.ReadShort();

	if (id != -1)	// sustains
	{
		int interval = message.ReadLong();
		CLQ2_SustainParticleStream(id, cnt, pos, dir, r, magnitude, interval);
	}
	else// instant
	{
		int color = r & 0xff;
		CLQ2_ParticleSteamEffect(pos, dir, color, cnt, magnitude);
	}
}

static void CLQ2_ParseBubleTrail2(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t pos2;
	message.ReadPos(pos2);

	CLQ2_BubbleTrail2(pos, pos2, 8);
	S_StartSound(pos, 0, 0, clq2_sfx_lashit, 1, ATTN_NORM, 0);
}

static void CLQ2_ParseMoreBlood(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t dir;
	message.ReadDir(dir);

	CLQ2_ParticleEffect(pos, dir, 0xe8, 250);
}

static void CLQ2_ParseChainfistSmoke(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	vec3_t dir;
	dir[0] = 0;
	dir[1] = 0;
	dir[2] = 1;
	CLQ2_ParticleSmokeEffect(pos, dir, 0, 20, 20);
}

static void CLQ2_ParseElectricSparks(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	vec3_t dir;
	message.ReadDir(dir);

	CLQ2_ParticleEffect(pos, dir, 0x75, 40);
	S_StartSound(pos, 0, 0, clq2_sfx_lashit, 1, ATTN_NORM, 0);
}

static void CLQ2_ParseTrackerExplosion(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	CLQ2_ColorFlash(0, pos, 150, -1, -1, -1);
	CLQ2_ColorExplosionParticles(pos, 0, 1);
	S_StartSound(pos, 0, 0, clq2_sfx_disrexp, 1, ATTN_NORM, 0);
}

static void CLQ2_ParseTeleportEffect(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	CLQ2_TeleportParticles(pos);
}

static void CLQ2_ParseWidowBeamOut(QMsg& message)
{
	int id = message.ReadShort();
	vec3_t pos;
	message.ReadPos(pos);

	CLQ2_SustainWindow(id, pos);
}

static void CLQ2_ParseNukeBlast(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	CLQ2_SustainNuke(pos);
}

static void CLQ2_ParseWidowSplash(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	CLQ2_WidowSplash(pos);
}

void CLQ2_ParseTEnt(QMsg& message)
{
	int type = message.ReadByte();
	switch (type)
	{
	case Q2TE_BLOOD:			// bullet hitting flesh
		CLQ2_ParseBlood(message);
		break;

	case Q2TE_GUNSHOT:			// bullet hitting wall
		CLQ2_ParseGunShot(message);
		break;

	case Q2TE_SPARKS:
		CLQ2_ParseSparks(message);
		break;

	case Q2TE_BULLET_SPARKS:
		CLQ2_ParseBulletSparks(message);
		break;

	case Q2TE_SCREEN_SPARKS:
		CLQ2_ParseScreenSparks(message);
		break;

	case Q2TE_SHIELD_SPARKS:
		CLQ2_ParseShieldSparks(message);
		break;

	case Q2TE_SHOTGUN:			// bullet hitting wall
		CLQ2_ParseShotgun(message);
		break;

	case Q2TE_SPLASH:			// bullet hitting water
		CLQ2_ParseSplash(message);
		break;

	case Q2TE_LASER_SPARKS:
		CLQ2_ParseLaserSparks(message);
		break;

	// RAFAEL
	case Q2TE_BLUEHYPERBLASTER:
		CLQ2_ParseBlueHyperBlaster(message);
		break;

	case Q2TE_BLASTER:			// blaster hitting wall
		CLQ2_ParseBlaster(message);
		break;

	case Q2TE_RAILTRAIL:			// railgun effect
		CLQ2_ParseRailTrail(message);
		break;

	case Q2TE_EXPLOSION2:
	case Q2TE_GRENADE_EXPLOSION:
		CLQ2_ParseGrenadeExplosion(message);
		break;

	case Q2TE_GRENADE_EXPLOSION_WATER:
		CLQ2_ParseGrenadeExplosionWater(message);
		break;

	// RAFAEL
	case Q2TE_PLASMA_EXPLOSION:
		CLQ2_ParsePlasmaExplosion(message);
		break;

	case Q2TE_EXPLOSION1:
	case Q2TE_ROCKET_EXPLOSION:
		CLQ2_ParseRocketExplosion(message);
		break;

	case Q2TE_EXPLOSION1_BIG:
		CLQ2_ParseExplosion1Big(message);
		break;

	case Q2TE_ROCKET_EXPLOSION_WATER:
		CLQ2_ParseRocketExplosionWater(message);
		break;

	case Q2TE_EXPLOSION1_NP:
	case Q2TE_PLAIN_EXPLOSION:
		CLQ2_ParsePlainExplosion(message);
		break;

	case Q2TE_BFG_EXPLOSION:
		CLQ2_ParseBfgExplosion(message);
		break;

	case Q2TE_BFG_BIGEXPLOSION:
		CLQ2_ParseBfgBigExplosion(message);
		break;

	case Q2TE_BFG_LASER:
		CLQ2_ParseBfgLaser(message);
		break;

	case Q2TE_BUBBLETRAIL:
		CLQ2_ParseBubleTrail(message);
		break;

	case Q2TE_PARASITE_ATTACK:
	case Q2TE_MEDIC_CABLE_ATTACK:
		CLQ2_ParseParasiteAttack(message);
		break;

	case Q2TE_BOSSTPORT:			// boss teleporting to station
		CLQ2_ParseBossTeleport(message);
		break;

	case Q2TE_GRAPPLE_CABLE:
		CLQ2_ParseGrappleCable(message);
		break;

	case Q2TE_WELDING_SPARKS:
		CLQ2_ParseWeldingSparks(message);
		break;

	case Q2TE_GREENBLOOD:
		CLQ2_ParseGreenBlood(message);
		break;

	case Q2TE_TUNNEL_SPARKS:
		CLQ2_ParseTunnelSparks(message);
		break;

	case Q2TE_BLASTER2:			// green blaster hitting wall
		CLQ2_ParseBlaster2(message);
		break;

	case Q2TE_FLECHETTE:			// flechette
		CLQ2_ParseFlechette(message);
		break;

	case Q2TE_LIGHTNING:
		CLQ2_ParseLightning(message);
		break;

	case Q2TE_DEBUGTRAIL:
		CLQ2_ParseDebugTrail(message);
		break;

	case Q2TE_FLASHLIGHT:
		CLQ2_ParseFlashLight(message);
		break;

	case Q2TE_FORCEWALL:
		CLQ2_ParseForceWall(message);
		break;

	case Q2TE_HEATBEAM:
		CLQ2_ParseHeatBeam(message);
		break;

	case Q2TE_MONSTER_HEATBEAM:
		CLQ2_ParseMonsterHeatBeam(message);
		break;

	case Q2TE_HEATBEAM_SPARKS:
		CLQ2_ParseHeatBeamSparks(message);
		break;

	case Q2TE_HEATBEAM_STEAM:
		CLQ2_ParseHeatBeamSteam(message);
		break;

	case Q2TE_STEAM:
		CLQ2_ParseSteam(message);
		break;

	case Q2TE_BUBBLETRAIL2:
		CLQ2_ParseBubleTrail2(message);
		break;

	case Q2TE_MOREBLOOD:
		CLQ2_ParseMoreBlood(message);
		break;

	case Q2TE_CHAINFIST_SMOKE:
		CLQ2_ParseChainfistSmoke(message);
		break;

	case Q2TE_ELECTRIC_SPARKS:
		CLQ2_ParseElectricSparks(message);
		break;

	case Q2TE_TRACKER_EXPLOSION:
		CLQ2_ParseTrackerExplosion(message);
		break;

	case Q2TE_TELEPORT_EFFECT:
	case Q2TE_DBALL_GOAL:
		CLQ2_ParseTeleportEffect(message);
		break;

	case Q2TE_WIDOWBEAMOUT:
		CLQ2_ParseWidowBeamOut(message);
		break;

	case Q2TE_NUKEBLAST:
		CLQ2_ParseNukeBlast(message);
		break;

	case Q2TE_WIDOWSPLASH:
		CLQ2_ParseWidowSplash(message);
		break;

	default:
		common->Error("CLQ2_ParseTEnt: bad type");
	}
}

void CLQ2_AddTEnts()
{
	CLQ2_AddBeams();
	CLQ2_AddPlayerBeams();
	CLQ2_AddExplosions();
	CLQ2_AddLasers();
	CLQ2_ProcessSustain();
}
