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
#include "../../client_main.h"
#include "../particles.h"
#include "../dynamic_lights.h"
#include "../../../common/player_move.h"
#include "../../../common/content_types.h"

#define FRANDOM() (rand() * (1.0 / RAND_MAX))

enum exflags_t
{
	EXFLAG_ROTATE = 1,
	EXFLAG_COLLIDE = 2,
	EXFLAG_STILL_FRAME = 4
};

#define H2MAX_EXPLOSIONS            128

struct h2explosion_t
{
	vec3_t origin;
	vec3_t oldorg;	// holds position from last frame
	int startTime;
	int endTime;
	vec3_t velocity;
	vec3_t accel;
	vec3_t angles;
	vec3_t avel;	// angular velocity
	int flags;
	int abslight;
	int exflags;
	int skin;
	int scale;
	qhandle_t model;
	void (* frameFunc)(h2explosion_t* ex);
	void (* removeFunc)(h2explosion_t* ex);
	float data;	//for easy transition of script code that relied on counters of some sort
};

static h2explosion_t clh2_explosions[H2MAX_EXPLOSIONS];

static sfxHandle_t clh2_sfx_explode;
static sfxHandle_t clh2_sfx_bonephit;
static sfxHandle_t clh2_sfx_bonehit;
static sfxHandle_t clh2_sfx_bonewal;
static sfxHandle_t clh2_sfx_ravendie;
static sfxHandle_t clh2_sfx_ravengo;
static sfxHandle_t clh2_sfx_dropfizzle;
static sfxHandle_t clh2_sfx_axeExplode;
static sfxHandle_t clh2_sfx_icewall;
static sfxHandle_t clh2_sfx_iceshatter;
static sfxHandle_t clh2_sfx_iceflesh;
static sfxHandle_t clh2_sfx_big_gib;
static sfxHandle_t clh2_sfx_gib1;
static sfxHandle_t clh2_sfx_gib2;
static sfxHandle_t clh2_sfx_telefrag;
static sfxHandle_t clh2_sfx_acidhit;
static sfxHandle_t clh2_sfx_teleport[5];
static sfxHandle_t clh2_sfx_swordExplode;
static sfxHandle_t clh2_sfx_axeBounce;
static sfxHandle_t clh2_sfx_fireBall;
static sfxHandle_t clh2_sfx_purify2;
static sfxHandle_t clh2_sfx_purify1_fire;
static sfxHandle_t clh2_sfx_purify1_hit;
static sfxHandle_t clh2_sfx_flameend;

static int MultiGrenadeCurrentChannel;

static float clh2_targetDistance = 0.0;
static float clh2_targetAngle;
static float clh2_targetPitch;

void CLHW_InitExplosionSounds()
{
	clh2_sfx_explode = S_RegisterSound("weapons/explode.wav");
	clh2_sfx_bonephit = S_RegisterSound("necro/bonephit.wav");
	clh2_sfx_bonehit = S_RegisterSound("necro/bonenhit.wav");
	clh2_sfx_bonewal = S_RegisterSound("necro/bonenwal.wav");
	clh2_sfx_ravendie = S_RegisterSound("raven/death.wav");
	clh2_sfx_ravengo = S_RegisterSound("raven/ravengo.wav");
	clh2_sfx_dropfizzle = S_RegisterSound("succubus/dropfizz.wav");
	clh2_sfx_axeExplode = S_RegisterSound("weapons/explode.wav");
	clh2_sfx_icewall = S_RegisterSound("crusader/icewall.wav");
	clh2_sfx_iceshatter = S_RegisterSound("misc/icestatx.wav");
	clh2_sfx_iceflesh = S_RegisterSound("crusader/icehit.wav");
	clh2_sfx_big_gib = S_RegisterSound("player/megagib.wav");
	clh2_sfx_gib1 = S_RegisterSound("player/gib1.wav");
	clh2_sfx_gib2 = S_RegisterSound("player/gib2.wav");
	clh2_sfx_telefrag = S_RegisterSound("player/telefrag.wav");
	clh2_sfx_acidhit = S_RegisterSound("succubus/acidhit.wav");
	clh2_sfx_teleport[0] = S_RegisterSound("misc/teleprt1.wav");
	clh2_sfx_teleport[1] = S_RegisterSound("misc/teleprt2.wav");
	clh2_sfx_teleport[2] = S_RegisterSound("misc/teleprt3.wav");
	clh2_sfx_teleport[3] = S_RegisterSound("misc/teleprt4.wav");
	clh2_sfx_teleport[4] = S_RegisterSound("misc/teleprt5.wav");
	clh2_sfx_swordExplode = S_RegisterSound("weapons/explode.wav");
	clh2_sfx_axeBounce = S_RegisterSound("paladin/axric1.wav");
	clh2_sfx_fireBall = S_RegisterSound("weapons/fbfire.wav");
	clh2_sfx_purify2 = S_RegisterSound("weapons/exphuge.wav");
	clh2_sfx_purify1_fire = S_RegisterSound("paladin/purfire.wav");
	clh2_sfx_purify1_hit = S_RegisterSound("weapons/expsmall.wav");
	clh2_sfx_flameend = S_RegisterSound("succubus/flamend.wav");
}

void CLH2_ClearExplosions()
{
	Com_Memset(clh2_explosions, 0, sizeof(clh2_explosions));
}

//**** CAREFUL!!! This may overwrite an explosion!!!!!
static h2explosion_t* CLH2_AllocExplosion()
{
	int index = 0;
	bool freeSlot = false;

	for (int i = 0; i < H2MAX_EXPLOSIONS; i++)
	{
		if (!clh2_explosions[i].model)
		{
			index = i;
			freeSlot = true;
			break;
		}
	}


	// find the oldest explosion
	if (!freeSlot)
	{
		int time = cl.serverTime;

		for (int i = 0; i < H2MAX_EXPLOSIONS; i++)
		{
			if (clh2_explosions[i].startTime < time)
			{
				time = clh2_explosions[i].startTime;
				index = i;
			}
		}
	}

	//zero out velocity and acceleration, funcs
	Com_Memset(&clh2_explosions[index], 0, sizeof(h2explosion_t));

	return &clh2_explosions[index];
}

static void BubbleThink(h2explosion_t* ex)
{
	if (CM_PointContentsQ1(ex->origin, 0) == BSP29CONTENTS_WATER)
	{
		//still in water
		if (ex->data < cl.serverTime * 0.001)	//change course
		{
			ex->velocity[0] += rand() % 20 - 10;
			ex->velocity[1] += rand() % 20 - 10;
			ex->velocity[2] += rand() % 10 + 10;

			if (ex->velocity[0] > 10)
			{
				ex->velocity[0] = 5;
			}
			if (ex->velocity[0] < -10)
			{
				ex->velocity[0] = -5;
			}

			if (ex->velocity[1] > 10)
			{
				ex->velocity[1] = 5;
			}
			if (ex->velocity[1] < -10)
			{
				ex->velocity[1] = -5;
			}

			if (ex->velocity[2] < 10)
			{
				ex->velocity[2] = 15;
			}
			if (ex->velocity[2] > 30)
			{
				ex->velocity[2] = 25;
			}

			ex->data += 0.5;
		}
	}
	else
	{
		ex->endTime = cl.serverTime;
	}
}

void CLHW_SpawnDeathBubble(const vec3_t pos)
{
	//generic spinny impact image
	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	VectorSet(ex->velocity, 0, 0, 17);
	ex->data = cl.serverTime * 0.001;
	ex->scale = 128;
	ex->frameFunc = BubbleThink;
	ex->startTime = cl.serverTime;
	ex->endTime = cl.serverTime + 15000;
	ex->model = R_RegisterModel("models/s_bubble.spr");
	ex->flags = H2DRF_TRANSLUCENT | H2MLS_ABSLIGHT;
	ex->abslight = 175;
}

static void MissileFlashThink(h2explosion_t* ex)
{
	ex->abslight -= (0.05 * 256);
	ex->scale += 5;
	if (ex->abslight < (0.05 * 256))
	{
		ex->endTime = ex->startTime;	//remove
	}
}

void CLHW_ParseMissileFlash(QMsg& message)
{
	vec3_t pos, vel;
	message.ReadPos(pos);
	vel[0] = message.ReadCoord();	//angles
	vel[1] = message.ReadCoord();
	vel[2] = rand() % 360;

	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	VectorCopy(vel, ex->angles);
	ex->frameFunc = MissileFlashThink;
	ex->model = R_RegisterModel("models/handfx.mdl");
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + 2000;
	ex->exflags = EXFLAG_ROTATE;
	ex->avel[2] = rand() * 360 + 360;
	ex->flags = H2MLS_ABSLIGHT | H2DRF_TRANSLUCENT;
	ex->abslight = 128;
	ex->scale = 80 + rand() % 40;
}

void CLHW_ParseDrillaExplode(QMsg& message)
{
	// particles
	vec3_t pos;
	message.ReadPos(pos);
	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos,ex->origin);

	ex->model = R_RegisterModel("models/gen_expl.spr");

	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 100;

	// sound
	S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_explode, 1, 1);
}

static void MultiGrenadeExplodeSound(h2explosion_t* ex)
{
	//plug up all of -1's channels w/ grenade sounds
	if (!(rand() & 7))
	{
		if (MultiGrenadeCurrentChannel >= 8 || MultiGrenadeCurrentChannel < 0)
		{
			MultiGrenadeCurrentChannel = 0;
		}

		S_StartSound(ex->origin, CLH2_TempSoundChannel(), MultiGrenadeCurrentChannel++, clh2_sfx_explode, 1, 1);
	}

	ex->frameFunc = NULL;
}

static void MultiGrenadePieceThink(h2explosion_t* ex)
{
	//FIXME: too messy
	VectorSet(ex->velocity, 0, 0, 0);
	VectorSet(ex->accel, 0, 0, 0);

	ex->frameFunc = MultiGrenadeExplodeSound;

	if (ex->data < 70)
	{
		ex->data = 70;
	}

	float ftemp = (rand() / RAND_MAX * 0.2) - 0.1;
	ex->startTime = cl.serverTime + ((1 - (ex->data - 69) / 180.0) + ftemp - 0.3) * 1250;
	if (ex->startTime < cl.serverTime)
	{
		ex->startTime = cl.serverTime;
	}
	ftemp = (rand() / RAND_MAX * 0.4) - 0.4;
	ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 100 + ftemp * 1000;

	if (R_ModelNumFrames(ex->model) > 14)
	{
		ex->startTime -= 400;
		ex->endTime -= 400;
	}

	if (ex->data <= 71)
	{
		return;
	}

	int attack_counter = 0;
	int number_explosions = (rand() & 3) + 2;

	while (attack_counter < number_explosions)
	{
		attack_counter += 1;
		h2explosion_t* missile = CLH2_AllocExplosion();

		missile->frameFunc = MultiGrenadePieceThink;

		VectorCopy(ex->origin,missile->origin);

		missile->origin[0] += (rand() % 100) - 50;
		missile->origin[1] += (rand() % 100) - 50;
		missile->origin[2] += (rand() % 10) - 4;

		ftemp = rand() / RAND_MAX * 0.2;

		missile->data = ex->data * (0.7  +  ftemp);

		if (rand() & 7)
		{
			missile->model = R_RegisterModel("models/bg_expld.spr");
		}
		else
		{
			missile->model = R_RegisterModel("models/fl_expld.spr");
		}

		ftemp = rand() / RAND_MAX * 0.5;

		missile->startTime = cl.serverTime + 10;
		missile->endTime = missile->startTime + R_ModelNumFrames(missile->model) * 100;
	}
}

static void MultiGrenadePiece2Think(h2explosion_t* ex)
{
	//FIXME: too messy
	VectorSet(ex->velocity, 0, 0, 0);
	VectorSet(ex->accel, 0, 0, 0);

	ex->frameFunc = MultiGrenadeExplodeSound;

	if (ex->data < 70)
	{
		ex->data = 70;
	}

	ex->startTime = cl.serverTime + 1300 - (ex->data - 69) * 15 / 2;
	float ftemp = (rand() / RAND_MAX * 0.4) - 0.2;
	ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 100 + ftemp * 1000;

	if (ex->data <= 71)
	{
		return;
	}

	int attack_counter = 0;
	int number_explosions = (rand() % 3) + 2;

	while (attack_counter < number_explosions)
	{
		attack_counter += 1;
		h2explosion_t* missile = CLH2_AllocExplosion();

		missile->frameFunc = MultiGrenadePiece2Think;

		VectorCopy(ex->origin, missile->origin);

		missile->origin[0] += (rand() % 30) - 15;
		missile->origin[1] += (rand() % 30) - 15;

		ftemp = rand() / RAND_MAX;

		missile->origin[2] += (ftemp * 7) + 30;
		missile->data = ex->data - ftemp * 10 - 20;

		if (rand() & 1)
		{
			missile->model = R_RegisterModel("models/gen_expl.spr");
		}
		else
		{
			missile->model = R_RegisterModel("models/bg_expld.spr");
		}

		ftemp = rand() / RAND_MAX * 0.5;

		missile->startTime = cl.serverTime + 10;
		missile->endTime = missile->startTime + R_ModelNumFrames(missile->model) * 100;
	}
}

static void MultiGrenadeThink(h2explosion_t* ex)
{
	//FIXME: too messy
	VectorSet(ex->velocity, 0, 0, 0);
	VectorSet(ex->accel, 0, 0, 0);

	ex->frameFunc = MultiGrenadeExplodeSound;

	int attack_counter = 0;
	int number_explosions = (rand() & 2) + 6;

	while (attack_counter < number_explosions)
	{
		attack_counter += 1;
		h2explosion_t* missile = CLH2_AllocExplosion();

		missile->frameFunc = MultiGrenadePieceThink;

		VectorCopy(ex->origin,missile->origin);

		if (rand() & 1)
		{
			missile->model = R_RegisterModel("models/gen_expl.spr");
		}
		else
		{
			missile->model = R_RegisterModel("models/bg_expld.spr");
		}

		float ftemp;
		switch (attack_counter % 3)
		{
		case 2:
			missile->frameFunc = MultiGrenadePieceThink;
			missile->velocity[0] = (rand() % 600) - 300;
			missile->velocity[1] = (rand() % 600) - 300;
			missile->velocity[2] = 0;

			missile->startTime = cl.serverTime;
			ftemp = (rand() / RAND_MAX * 0.3) - 0.15;
			missile->endTime = missile->startTime + R_ModelNumFrames(missile->model) * 100 + ftemp * 1000;
			break;
		case 1:
			missile->frameFunc = MultiGrenadePiece2Think;
			missile->velocity[0] = (rand() % 80) - 40;
			missile->velocity[1] = (rand() % 80) - 40;
			missile->velocity[2] = (rand() % 150) + 150;

			missile->startTime = cl.serverTime;
			ftemp = (rand() / RAND_MAX  * 0.3) - 0.15;
			missile->endTime = missile->startTime + R_ModelNumFrames(missile->model) * 100 + ftemp * 1000;
			break;
		default://some extra explosions for at first
			missile->frameFunc = NULL;
			missile->origin[0] += (rand() % 50) - 25;
			missile->origin[1] += (rand() % 50) - 25;
			missile->origin[2] += (rand() % 50) - 25;

			ftemp = rand() / RAND_MAX * 0.2;
			missile->startTime = cl.serverTime + ftemp * 1000;
			ftemp = (rand() / RAND_MAX * 0.2) - 0.1;
			missile->endTime = missile->startTime + R_ModelNumFrames(missile->model) * 100 + ftemp * 1000;
			break;
		}

		ftemp = rand() / RAND_MAX * 0.2;

		missile->data = ex->data * (0.7  +  ftemp);
	}
}

void CLHW_ParseBigGrenade(QMsg& message)
{
	vec3_t pos;
	pos[0] = message.ReadCoord();
	pos[1] = message.ReadCoord();
	pos[2] = message.ReadCoord();

	CLH2_ParticleExplosion(pos);

	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->frameFunc = MultiGrenadeThink;
	ex->data = 250;
	ex->model = R_RegisterModel("models/sm_expld.spr");
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 100;
}

static void CLHW_XBowHit(const vec3_t pos, const vec3_t vel, int damage, int absoluteLight, void (* frameFunc)(h2explosion_t* ex))
{
	//	generic spinny impact image
	h2explosion_t* ex = CLH2_AllocExplosion();
	ex->frameFunc = frameFunc;
	ex->origin[0] = pos[0] - vel[0];
	ex->origin[1] = pos[1] - vel[1];
	ex->origin[2] = pos[2] - vel[2];
	VecToAnglesBuggy(vel, ex->angles);
	ex->avel[2] = (rand() % 500) + 200;
	ex->scale = 10;
	ex->startTime = cl.serverTime;
	ex->endTime = cl.serverTime + 300;
	ex->model = R_RegisterModel("models/arrowhit.mdl");
	ex->exflags = EXFLAG_ROTATE;
	ex->flags = H2DRF_TRANSLUCENT | H2MLS_ABSLIGHT;
	ex->abslight = absoluteLight;

	//	white smoke if invulnerable impact
	if (!damage)
	{
		ex = CLH2_AllocExplosion();
		ex->origin[0] = pos[0] - vel[0] * 2;
		ex->origin[1] = pos[1] - vel[1] * 2;
		ex->origin[2] = pos[2] - vel[2] * 2;
		ex->velocity[0] = 0.0;
		ex->velocity[1] = 0.0;
		ex->velocity[2] = 80.0;
		VecToAnglesBuggy(vel, ex->angles);
		ex->startTime = cl.serverTime;
		ex->endTime = cl.serverTime + 350;
		ex->model = R_RegisterModel("models/whtsmk1.spr");
		ex->flags = H2DRF_TRANSLUCENT;
	}
}

void CLHW_ParseXBowHit(QMsg& message)
{
	vec3_t pos, vel;
	message.ReadPos(pos);
	message.ReadPos(vel);
	message.ReadByte();	//	Chunk type, unused
	int damage = message.ReadByte();

	CLHW_XBowHit(pos, vel, damage, 128, NULL);
}

// remove tent if not in open air
static void CheckSpaceThink(h2explosion_t* ex)
{
	if (CM_PointContentsQ1(ex->origin, 0) != BSP29CONTENTS_EMPTY)
	{
		ex->endTime = ex->startTime;
	}
}

void CLHW_ParseBonePower(QMsg& message)
{
	int cnt = message.ReadByte();
	vec3_t pos, movedir;
	message.ReadPos(pos);
	message.ReadPos(movedir);

	CLH2_RunParticleEffect4(pos, 50, 368 + rand() % 16, pt_h2grav, 10);

	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	VectorMA(ex->origin, -6, movedir, ex->origin);
	ex->data = 250;
	ex->model = R_RegisterModel("models/sm_expld.spr");
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 100;
	for (int cnt2 = 0; cnt2 < cnt; cnt2++)
	{
		vec3_t offset;
		offset[0] = rand() % 40 - 20;
		offset[1] = rand() % 40 - 20;
		offset[2] = rand() % 40 - 20;
		ex = CLH2_AllocExplosion();
		VectorAdd(pos, offset, ex->origin);
		VectorMA(ex->origin, -8, movedir, ex->origin);
		VectorCopy(offset, ex->velocity);
		ex->velocity[2] += 30;
		ex->data = 250;
		ex->model = R_RegisterModel("models/ghost.spr");
		ex->abslight = 128;
		ex->flags = H2DRF_TRANSLUCENT | H2MLS_ABSLIGHT;
		ex->startTime = cl.serverTime;
		ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 100;
	}
	for (int cnt2 = 0; cnt2 < 20; cnt2++)
	{
		// want faster velocity to hide the fact that these
		// aren't in the real location
		vec3_t offset;
		offset[0] = rand() % 400 + 300;
		if (rand() % 2)
		{
			offset[0] = -offset[0];
		}
		offset[1] = rand() % 400 + 300;
		if (rand() % 2)
		{
			offset[1] = -offset[1];
		}
		offset[2] = rand() % 400 + 300;
		if (rand() % 2)
		{
			offset[2] = -offset[2];
		}
		ex = CLH2_AllocExplosion();
		VectorMA(pos, 1 / 700, offset, ex->origin);
		VectorCopy(offset, ex->velocity);
		ex->data = 250;
		ex->model = R_RegisterModel("models/boneshrd.mdl");
		ex->startTime = cl.serverTime;
		ex->endTime = ex->startTime + rand() * 500;
		ex->flags |= EXFLAG_ROTATE | EXFLAG_COLLIDE;
		ex->angles[0] = rand() % 700;
		ex->angles[1] = rand() % 700;
		ex->angles[2] = rand() % 700;
		ex->avel[0] = rand() % 700;
		ex->avel[1] = rand() % 700;
		ex->avel[2] = rand() % 700;
		ex->frameFunc = CheckSpaceThink;
	}
	S_StartSound(pos, CLH2_TempSoundChannel(), 1, clh2_sfx_bonephit, 1, 1);
}

void CLHW_ParseBonePower2(QMsg& message)
{
	int cnt2 = message.ReadByte();		//did it hit? changes sound
	vec3_t pos;
	message.ReadPos(pos);

	// white smoke
	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->model = R_RegisterModel("models/whtsmk1.spr");
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 100;

	//sound
	if (cnt2)
	{
		S_StartSound(pos, CLH2_TempSoundChannel(), 1, clh2_sfx_bonehit, 1, 1);
	}
	else
	{
		S_StartSound(pos, CLH2_TempSoundChannel(), 1, clh2_sfx_bonewal, 1, 1);
	}

	CLH2_RunParticleEffect4(pos, 3, 368 + rand() % 16, pt_h2grav, 7);
}

static void NewRavenDieExplosion(const vec3_t pos, float velocityY, const char* modelName)
{
	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->velocity[1] = velocityY;
	ex->velocity[2] = -10;
	ex->model = R_RegisterModel(modelName);
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + HX_FRAME_MSTIME * 10;
}

void CLHW_CreateRavenDeath(const vec3_t pos)
{
	NewRavenDieExplosion(pos, 8, "models/whtsmk1.spr");
	NewRavenDieExplosion(pos, 0, "models/redsmk1.spr");
	NewRavenDieExplosion(pos, -8, "models/whtsmk1.spr");
	S_StartSound(pos, CLH2_TempSoundChannel(), 1, clh2_sfx_ravendie, 1, 1);
}

void CLHW_ParseRavenDie(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	CLHW_CreateRavenDeath(pos);
}

static void NewRavenExplodeExplosion(const vec3_t pos, float originAdjust)
{
	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->origin[2] -= originAdjust;
	ex->velocity[2] = 8;
	ex->model = R_RegisterModel("models/whtsmk1.spr");
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + HX_FRAME_MSTIME * 10;
}

void CLHW_CreateRavenExplosions(const vec3_t pos)
{
	NewRavenExplodeExplosion(pos, 0);
	NewRavenExplodeExplosion(pos, 5);
	NewRavenExplodeExplosion(pos, 10);
}

void CLHW_ParseRavenExplode(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	CLHW_CreateRavenExplosions(pos);
	S_StartSound(pos, CLH2_TempSoundChannel(), 1, clh2_sfx_ravengo, 1, 1);
}

static void ChunkThink(h2explosion_t* ex)
{
	bool moving = true;
	if (CM_PointContentsQ1(ex->origin, 0) != BSP29CONTENTS_EMPTY)
	{
		//collided with world
		VectorCopy(ex->oldorg, ex->origin);

		if ((int)ex->data == H2THINGTYPE_FLESH)
		{
			if (VectorNormalize(ex->velocity) > 100.0)
			{
				// hit, now make a splash of blood
				vec3_t dmin = {-40, -40, 10};
				vec3_t dmax = {40, 40, 40};
				CLH2_RunParticleEffect2(ex->origin, dmin, dmax, 136 + (rand() % 5), pt_h2darken, 20);
			}
		}
		else if ((int)ex->data == H2THINGTYPE_ACID)
		{
			if (VectorNormalize(ex->velocity) > 100.0)
			{
				// hit, now make a splash of acid
				//vec3_t	dmin = {-40, -40, 10};
				//vec3_t	dmax = {40, 40, 40};
				if (!(rand() % 3))
				{
					S_StartSound(ex->origin, CLH2_TempSoundChannel(), 0, clh2_sfx_dropfizzle, 1, 1);
				}
			}
		}

		ex->velocity[0] = 0;
		ex->velocity[1] = 0;
		ex->velocity[2] = 0;
		ex->avel[0] = 0;
		ex->avel[1] = 0;
		ex->avel[2] = 0;

		moving = false;
	}

	if (cl.serverTime + cls.frametime * 5 > ex->endTime)
	{
		// chunk leaves in 5 frames about
		switch ((int)ex->data)
		{
		case H2THINGTYPE_METEOR:
			if (cl.serverTime + cls.frametime * 4 < ex->endTime)
			{
				// just crossed the threshold
				ex->abslight = 200;
			}
			else
			{
				ex->abslight -= 35;
			}
			ex->flags |= H2DRF_TRANSLUCENT | H2MLS_ABSLIGHT;
			ex->scale *= 1.4;
			ex->angles[0] += 20;
			break;
		default:
			ex->scale *= .8;
			break;
		}
	}

	ex->velocity[2] -= cls.frametime * 0.001 * movevars.gravity;// this is gravity

	vec3_t oldorg;
	switch ((int)ex->data)
	{
	case H2THINGTYPE_FLESH:
		if (moving)
		{
			CLH2_TrailParticles(ex->oldorg, ex->origin, rt_blood);
		}
		break;
	case H2THINGTYPE_ICE:
		ex->velocity[2] += cls.frametime * 0.001 * movevars.gravity * 0.5;	// lower gravity for ice chunks
		if (moving)
		{
			CLH2_TrailParticles(ex->oldorg, ex->origin, rt_ice);
		}
		break;
	case H2THINGTYPE_ACID:
		if (moving)
		{
			CLH2_TrailParticles(ex->oldorg, ex->origin, rt_grensmoke);
		}
		break;
	case H2THINGTYPE_METEOR:
		VectorCopy(ex->oldorg, oldorg);
		if (!moving)
		{
			// resting meteors still give off smoke
			oldorg[0] += rand() % 7 - 3;
			oldorg[1] += rand() % 7 - 3;
			oldorg[2] += 16;
		}
		CLH2_TrailParticles(oldorg, ex->origin, rt_smoke);
		break;
	case H2THINGTYPE_GREENFLESH:
		if (moving)
		{
			CLH2_TrailParticles(ex->oldorg, ex->origin, rt_grensmoke);
		}
		break;
	}
}

static void CLHW_InitChunkExplosionCommon(h2explosion_t* ex, int chType)
{
	ex->frameFunc = ChunkThink;
	CLH2_InitChunkAngles(ex->angles);
	ex->data = chType;

	int tmpFrame;
	int tmpAbsoluteLight;
	CLH2_InitChunkModel(chType, &ex->model, &ex->skin, &ex->flags, &tmpFrame, &tmpAbsoluteLight);

	if (chType == H2THINGTYPE_METEOR)
	{
		VectorScale(ex->avel, 4.0, ex->avel);
	}

	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + 4000;
}

void CLHW_ParseChunk(QMsg& message)
{
	//directed chunks
	vec3_t pos, vel;
	message.ReadPos(pos);
	message.ReadPos(vel);
	int chType = message.ReadByte();
	int cnt = message.ReadByte();

	for (int i = 0; i < cnt; i++)
	{
		h2explosion_t* ex = CLH2_AllocExplosion();
		VectorCopy(pos,ex->origin);
		CLH2_InitChunkVelocity(vel, ex->velocity);

		ex->exflags = EXFLAG_ROTATE;

		CLH2_InitChunkAngleVelocity(ex->avel);

		ex->scale = 30 + 100 * (cnt / 40.0) + rand() % 40;
		CLHW_InitChunkExplosionCommon(ex, chType);
	}
}

void CLHW_ParseChunk2(QMsg& message)
{
	//volume based chunks
	vec3_t pos, vel;
	message.ReadPos(pos);
	message.ReadPos(vel);	//vel for CHUNK, volume for CHUNK2
	int chType = message.ReadByte();

	float volume = vel[0] * vel[1] * vel[2];
	int cnt = volume / 8192;
	float scale;
	if (volume < 5000)
	{
		scale = .20;
		cnt *= 3;	// Because so few pieces come out of a small object
	}
	else if (volume < 50000)
	{
		scale = .45;
		cnt *= 3;	// Because so few pieces come out of a small object
	}
	else if (volume < 500000)
	{
		scale = .50;
	}
	else if (volume < 1000000)
	{
		scale = .75;
	}
	else
	{
		scale = 1;
	}
	if (cnt > 30)
	{
		cnt = 30;
	}

	for (int i = 0; i < cnt; i++)
	{
		h2explosion_t* ex = CLH2_AllocExplosion();
		// set origin (origin is really absmin here)
		VectorCopy(pos, ex->origin);
		ex->origin[0] += FRANDOM() * vel[0];
		ex->origin[1] += FRANDOM() * vel[1];
		ex->origin[2] += FRANDOM() * vel[2];
		// set velocity
		ex->velocity[0] = -210 + FRANDOM() * 420;
		ex->velocity[1] = -210 + FRANDOM() * 420;
		ex->velocity[2] = -210 + FRANDOM() * 490;
		// set scale
		ex->scale = scale * 100;

		ex->avel[0] = rand() % 1200;
		ex->avel[1] = rand() % 1200;
		ex->avel[2] = rand() % 1200;
		CLHW_InitChunkExplosionCommon(ex, chType);
	}
}

void CLHW_ParseMeteorHit(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	// always make 8 meteors
	int i = (cls.frametime < 70) ? 0 : 4;	// based on framerate
	for (; i < 8; i++)
	{
		h2explosion_t* ex = CLH2_AllocExplosion();
		VectorCopy(pos,ex->origin);
		ex->frameFunc = ChunkThink;

		// temp modify them...
		ex->velocity[0] += (rand() % 400) - 200;
		ex->velocity[1] += (rand() % 400) - 200;
		ex->velocity[2] += (rand() % 200) + 150;

		// are these in degrees or radians?
		ex->angles[0] = rand() % 360;
		ex->angles[1] = rand() % 360;
		ex->angles[2] = rand() % 360;
		ex->exflags = EXFLAG_ROTATE;

		ex->avel[0] = rand() % 850 - 425;
		ex->avel[1] = rand() % 850 - 425;
		ex->avel[2] = rand() % 850 - 425;

		ex->scale = 45 + rand() % 10;
		ex->data = H2THINGTYPE_METEOR;

		ex->model = R_RegisterModel("models/tempmetr.mdl");
		ex->skin = 0;
		VectorScale(ex->avel, 4.0, ex->avel);

		ex->startTime = cl.serverTime;
		ex->endTime = ex->startTime + 4000;
	}

	// make the actual explosion
	i = (cls.frametime < 70) ? 0 : 8;	// based on framerate
	for (; i < 11; i++)
	{
		h2explosion_t* ex = CLH2_AllocExplosion();
		VectorCopy(pos, ex->origin);
		ex->origin[0] += (rand() % 10) - 5;
		ex->origin[1] += (rand() % 10) - 5;
		ex->origin[2] += (rand() % 10) - 5;

		ex->velocity[0] = (ex->origin[0] - pos[0]) * 12;
		ex->velocity[1] = (ex->origin[1] - pos[1]) * 12;
		ex->velocity[2] = (ex->origin[2] - pos[2]) * 12;

		switch (rand() % 4)
		{
		case 0:
		case 1:
			ex->model = R_RegisterModel("models/sm_expld.spr");
			break;
		case 2:
			ex->model = R_RegisterModel("models/bg_expld.spr");
			break;
		case 3:
			ex->model = R_RegisterModel("models/gen_expl.spr");
			break;
		}
		if (cls.frametime * 0.001 < .07)
		{
			ex->flags |= H2MLS_ABSLIGHT | H2DRF_TRANSLUCENT;
		}
		ex->abslight = 160 + rand() % 64;
		ex->skin = 0;
		ex->scale = 80 + rand() % 40;
		ex->startTime = cl.serverTime + 5 * (rand() % 50);
		ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 40;
	}
	S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_axeExplode, 1, 1);
}

void CLHW_ParseIceHit(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	int cnt2 = message.ReadByte();	// 0 for person, 1 for wall

	if (cnt2)
	{
		int i = (cls.frametime < 70) ? 0 : 5;	// based on framerate
		for (; i < 9; i++)
		{
			h2explosion_t* ex = CLH2_AllocExplosion();
			VectorCopy(pos,ex->origin);
			ex->frameFunc = ChunkThink;

			ex->velocity[0] += (rand() % 1000) - 500;
			ex->velocity[1] += (rand() % 1000) - 500;
			ex->velocity[2] += (rand() % 200) - 50;

			// are these in degrees or radians?
			ex->angles[0] = rand() % 360;
			ex->angles[1] = rand() % 360;
			ex->angles[2] = rand() % 360;
			ex->exflags = EXFLAG_ROTATE;

			ex->avel[0] = rand() % 850 - 425;
			ex->avel[1] = rand() % 850 - 425;
			ex->avel[2] = rand() % 850 - 425;

			if (cnt2 == 2)
			{
				ex->scale = 65 + rand() % 10;
			}
			else
			{
				ex->scale = 35 + rand() % 10;
			}
			ex->data = H2THINGTYPE_ICE;

			ex->model = R_RegisterModel("models/shard.mdl");
			ex->skin = 0;
			//ent->frame = rand()%2;
			ex->flags |= H2DRF_TRANSLUCENT | H2MLS_ABSLIGHT;
			ex->abslight = 128;

			ex->startTime = cl.serverTime;
			ex->endTime = ex->startTime + 2000;
			if (cnt2 == 2)
			{
				ex->endTime += 3000;
			}
		}
	}
	else
	{
		vec3_t dmin = {-10, -10, -10};
		vec3_t dmax = {10, 10, 10};
		CLH2_ColouredParticleExplosion(pos,14,10,10);
		CLH2_RunParticleEffect2(pos, dmin, dmax, 145, pt_h2explode, 14);
	}
	// make the actual explosion
	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->model = R_RegisterModel("models/icehit.spr");
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 100;

	// Add in the sound
	if (cnt2 == 1)
	{	// hit a wall
		S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_icewall, 1, 1);
	}
	else if (cnt2 == 2)
	{
		S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_iceshatter, 1, 1);
	}
	else
	{	// hit a person
		S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_iceflesh, 1, 1);
	}
}

void CLHW_ParsePlayerDeath(QMsg& message)
{
	float throwPower, curAng, curPitch;

	vec3_t pos;
	message.ReadPos(pos);
	int angle = message.ReadByte();	// from 0 to 256
	int pitch = message.ReadByte();	// from 0 to 256
	int force = message.ReadByte();
	int style = message.ReadByte();


	int i = (cls.frametime < 70) ? 0 : 8;
	for (; i < 12; i++)
	{
		h2explosion_t* ex = CLH2_AllocExplosion();
		VectorCopy(pos, ex->origin);
		ex->origin[0] += (rand() % 40) - 20;
		ex->origin[1] += (rand() % 40) - 20;
		ex->origin[2] += rand() % 40;
		ex->frameFunc = ChunkThink;

		throwPower = 3.5 + ((rand() % 100) / 100.0);
		curAng = angle * idMath::TWO_PI / 256.0 + ((rand() % 100) / 50.0) - 1.0;
		curPitch = pitch * idMath::TWO_PI / 256.0 + ((rand() % 100) / 100.0) - .5;

		ex->velocity[0] = force * throwPower * cos(curAng) * cos(curPitch);
		ex->velocity[1] = force * throwPower * sin(curAng) * cos(curPitch);
		ex->velocity[2] = force * throwPower * sin(curPitch);

		// are these in degrees or radians?
		ex->angles[0] = rand() % 360;
		ex->angles[1] = rand() % 360;
		ex->angles[2] = rand() % 360;
		ex->exflags = EXFLAG_ROTATE;

		ex->avel[0] = rand() % 850 - 425;
		ex->avel[1] = rand() % 850 - 425;
		ex->avel[2] = rand() % 850 - 425;

		ex->scale = 80 + rand() % 40;
		ex->data = H2THINGTYPE_FLESH;

		switch (rand() % 3)
		{
		case 0:
			ex->model = R_RegisterModel("models/flesh1.mdl");
			break;
		case 1:
			ex->model = R_RegisterModel("models/flesh2.mdl");
			break;
		case 2:
			ex->model = R_RegisterModel("models/flesh3.mdl");
			break;
		}

		ex->skin = 0;

		ex->startTime = cl.serverTime;
		ex->endTime = ex->startTime + 4000;
	}

	switch (style)
	{
	case 0:
		S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_big_gib, 1, 1);
		break;
	case 1:
		if (rand() % 2)
		{
			S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_gib1, 1, 1);
		}
		else
		{
			S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_gib2, 1, 1);
		}
		break;
	case 2:
		S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_telefrag, 1, 1);
		break;
	}
}

void CLHW_ParseAcidBlob(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	int i = (cls.frametime < 70) ? 0 : 7;
	for (; i < 12; i++)
	{
		h2explosion_t* ex = CLH2_AllocExplosion();
		VectorCopy(pos, ex->origin);
		ex->origin[0] += rand() % 6 - 3;
		ex->origin[1] += rand() % 6 - 3;
		ex->origin[2] += rand() % 6 - 3;

		ex->velocity[0] = (ex->origin[0] - pos[0]) * 25;
		ex->velocity[1] = (ex->origin[1] - pos[1]) * 25;
		ex->velocity[2] = (ex->origin[2] - pos[2]) * 25;

		switch (rand() % 4)
		{
		case 0:
			ex->model = R_RegisterModel("models/axplsn_2.spr");
			break;
		case 1:
			ex->model = R_RegisterModel("models/axplsn_1.spr");
			break;
		case 2:
		case 3:
			ex->model = R_RegisterModel("models/axplsn_5.spr");
			break;
		}
		if (cls.frametime < 70)
		{
			ex->flags |= H2MLS_ABSLIGHT | H2DRF_TRANSLUCENT;
		}
		ex->abslight = 1;
		ex->skin = 0;
		ex->scale = 80 + rand() % 40;
		ex->startTime = cl.serverTime + 5 * (rand() % 50);
		ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 50;
	}

	// always make 8 meteors
	i = (cls.frametime < 70) ? 0 : 4;
	for (; i < 8; i++)
	{
		h2explosion_t* ex = CLH2_AllocExplosion();
		VectorCopy(pos,ex->origin);
		ex->frameFunc = ChunkThink;

		// temp modify them...
		ex->velocity[0] = (rand() % 500) - 250;
		ex->velocity[1] = (rand() % 500) - 250;
		ex->velocity[2] = (rand() % 200) + 200;

		// are these in degrees or radians?
		ex->angles[0] = rand() % 360;
		ex->angles[1] = rand() % 360;
		ex->angles[2] = rand() % 360;
		ex->exflags = EXFLAG_ROTATE;

		ex->avel[0] = rand() % 850 - 425;
		ex->avel[1] = rand() % 850 - 425;
		ex->avel[2] = rand() % 850 - 425;

		ex->scale = 45 + rand() % 10;
		ex->data = H2THINGTYPE_ACID;

		ex->model = R_RegisterModel("models/sucwp2p.mdl");
		ex->skin = 0;
		VectorScale(ex->avel, 4.0, ex->avel);

		ex->startTime = cl.serverTime;
		ex->endTime = ex->startTime + 4000;
	}

	S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_acidhit, 1, 1);
}

void CLHW_XbowImpact(const vec3_t pos, const vec3_t vel, int chType, int damage, int arrowType)	//arrowType is total # of arrows in effect
{
	CLHW_XBowHit(pos, vel, damage, 175, MissileFlashThink);

	if (!damage)
	{
		if (arrowType == 3)	//little arrows go away
		{
			if (rand() & 3)	//chunky go
			{
				int cnt = rand() % 2 + 1;
				for (int i = 0; i < cnt; i++)
				{
					h2explosion_t* ex = CLH2_AllocExplosion();
					ex->frameFunc = ChunkThink;

					VectorSubtract(pos,vel,ex->origin);
					// temp modify them...
					ex->velocity[0] = (rand() % 140) - 70;
					ex->velocity[1] = (rand() % 140) - 70;
					ex->velocity[2] = (rand() % 140) - 70;

					// are these in degrees or radians?
					ex->angles[0] = rand() % 360;
					ex->angles[1] = rand() % 360;
					ex->angles[2] = rand() % 360;
					ex->exflags = EXFLAG_ROTATE;

					ex->avel[0] = rand() % 850 - 425;
					ex->avel[1] = rand() % 850 - 425;
					ex->avel[2] = rand() % 850 - 425;

					ex->scale = 30 + 100 * (cnt / 40.0) + rand() % 40;

					ex->data = H2THINGTYPE_WOOD;

					int tmpFrame;
					int tmpAbsoluteLight;
					CLH2_InitChunkModel(H2THINGTYPE_WOOD, &ex->model, &ex->skin, &ex->flags, &tmpFrame, &tmpAbsoluteLight);

					ex->startTime = cl.serverTime;
					ex->endTime = ex->startTime + 4000;
				}
			}
			else if (rand() & 1)//whole go
			{
				h2explosion_t* ex = CLH2_AllocExplosion();
				ex->frameFunc = ChunkThink;

				VectorSubtract(pos, vel, ex->origin);
				// temp modify them...
				ex->velocity[0] = (rand() % 140) - 70;
				ex->velocity[1] = (rand() % 140) - 70;
				ex->velocity[2] = (rand() % 140) - 70;

				// are these in degrees or radians?
				ex->angles[0] = rand() % 360;
				ex->angles[1] = rand() % 360;
				ex->angles[2] = rand() % 360;
				ex->exflags = EXFLAG_ROTATE;

				ex->avel[0] = rand() % 850 - 425;
				ex->avel[1] = rand() % 850 - 425;
				ex->avel[2] = rand() % 850 - 425;

				ex->scale = 128;

				ex->data = H2THINGTYPE_WOOD;

				ex->model = R_RegisterModel("models/arrow.mdl");

				ex->startTime = cl.serverTime;
				ex->endTime = ex->startTime + 4000;
			}
		}
	}
}

static void TeleportFlashThink(h2explosion_t* ex)
{
	ex->scale -= 15;
	if (ex->scale < 10)
	{
		ex->endTime = ex->startTime;
	}
}

void CLHW_ParseTeleport(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	int cnt = message.ReadShort();	//skin#

	S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_teleport[rand() % 5], 1, 1);

	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->frameFunc = TeleportFlashThink;
	ex->model = R_RegisterModel("models/teleport.mdl");
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + 2000;
	ex->avel[2] = rand() * 360 + 360;
	ex->flags = H2SCALE_TYPE_XYONLY | H2DRF_TRANSLUCENT;
	ex->skin = cnt;
	ex->scale = 100;

	for (int dir = 0; dir < 360; dir += 45)
	{
		float cosval = 10 * cos(DEG2RAD(dir));
		float sinval = 10 * sin(DEG2RAD(dir));
		ex = CLH2_AllocExplosion();
		VectorCopy(pos, ex->origin);
		ex->model = R_RegisterModel("models/telesmk2.spr");
		ex->startTime = cl.serverTime;
		ex->endTime = ex->startTime + 500;
		ex->velocity[0] = cosval;
		ex->velocity[1] = sinval;
		ex->flags = H2DRF_TRANSLUCENT;

		ex = CLH2_AllocExplosion();
		VectorCopy(pos, ex->origin);
		ex->origin[2] += 64;
		ex->model = R_RegisterModel("models/telesmk2.spr");
		ex->startTime = cl.serverTime;
		ex->endTime = ex->startTime + 500;
		ex->velocity[0] = cosval;
		ex->velocity[1] = sinval;
		ex->flags = H2DRF_TRANSLUCENT;
	}
}

static void SwordFrameFunc(h2explosion_t* ex)
{
	ex->scale = (ex->endTime - cl.serverTime) * 0.15 + 1;
	if ((cl.serverTime / 50) % 2)
	{
		ex->skin = 0;
	}
	else
	{
		ex->skin = 1;
	}

}

void CLHW_SwordExplosion(const vec3_t pos)
{
	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->model = R_RegisterModel("models/vorpshok.mdl");
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + 1000;
	ex->flags |= H2MLS_ABSLIGHT;
	ex->abslight = 128;
	ex->skin = 0;
	ex->scale = 100;
	ex->frameFunc = SwordFrameFunc;

	S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_swordExplode, 1, 1);
}

void CLHW_ParseAxeBounce(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->model = R_RegisterModel("models/spark.spr");
	ex->startTime = cl.serverTime;
	ex->flags |= H2MLS_ABSLIGHT;
	ex->abslight = 128;
	ex->skin = 0;
	ex->scale = 100;
	ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 50;

	S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_axeBounce, 1, 1);
}

void CLHW_ParseAxeExplode(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	int i = (cls.frametime < 70) ? 0 : 3;	// based on framerate
	for (; i < 5; i++)
	{
		h2explosion_t* ex = CLH2_AllocExplosion();
		VectorCopy(pos, ex->origin);
		ex->origin[0] += rand() % 6 - 3;
		ex->origin[1] += rand() % 6 - 3;
		ex->origin[2] += rand() % 6 - 3;

		ex->velocity[0] = (ex->origin[0] - pos[0]) * 15;
		ex->velocity[1] = (ex->origin[1] - pos[1]) * 15;
		ex->velocity[2] = (ex->origin[2] - pos[2]) * 15;

		switch (rand() % 6)
		{
		case 0:
		case 1:
			ex->model = R_RegisterModel("models/xpspblue.spr");
			break;
		case 2:
		case 3:
			ex->model = R_RegisterModel("models/xpspblue.spr");
			ex->flags |= H2MLS_ABSLIGHT | H2DRF_TRANSLUCENT;
			break;
		case 4:
		case 5:
			ex->model = R_RegisterModel("models/spark0.spr");
			ex->flags |= H2MLS_ABSLIGHT | H2DRF_TRANSLUCENT;
			break;
		}
		ex->flags |= H2MLS_ABSLIGHT;
		ex->abslight = 160 + rand() % 24;
		ex->skin = 0;
		ex->scale = 80 + rand() % 40;
		ex->startTime = cl.serverTime + 5 * (rand() % 50);
		ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 50;
	}


	S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_axeExplode, 1, 1);
}

void CLHW_ParseTimeBomb(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	int i = (cls.frametime < 70) ? 0 : 14;	// based on framerate
	for (; i < 20; i++)
	{
		h2explosion_t* ex = CLH2_AllocExplosion();
		VectorCopy(pos, ex->origin);
		ex->origin[0] += rand() % 6 - 3;
		ex->origin[1] += rand() % 6 - 3;
		ex->origin[2] += rand() % 6 - 3;

		ex->velocity[0] = (ex->origin[0] - pos[0]) * 40;
		ex->velocity[1] = (ex->origin[1] - pos[1]) * 40;
		ex->velocity[2] = (ex->origin[2] - pos[2]) * 40;

		switch (rand() % 4)
		{
		case 0:
			ex->model = R_RegisterModel("models/sm_expld.spr");
			break;
		case 2:
			ex->model = R_RegisterModel("models/bg_expld.spr");
			break;
		case 1:
		case 3:
			ex->model = R_RegisterModel("models/gen_expl.spr");
			break;
		}
		ex->flags |= H2MLS_ABSLIGHT | H2DRF_TRANSLUCENT;
		ex->abslight = 160 + rand() % 24;
		ex->skin = 0;
		ex->scale = 80 + rand() % 40;
		ex->startTime = cl.serverTime + 5 * (rand() % 50);
		ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 50;
	}

	S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_axeExplode, 1, 1);
}

static void fireBallUpdate(h2explosion_t* ex)
{
	ex->scale = ((cl.serverTime - ex->startTime) / 4) + 1;
}

void CLHW_ParseFireBall(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->model = R_RegisterModel("models/blast.mdl");
	ex->flags |= H2MLS_ABSLIGHT | H2SCALE_TYPE_UNIFORM | H2SCALE_ORIGIN_CENTER;
	ex->abslight = 128;
	ex->skin = 0;
	ex->scale = 1;
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + 1000;
	ex->frameFunc = fireBallUpdate;

	ex->exflags = EXFLAG_ROTATE;

	ex->avel[0] = 50;
	ex->avel[1] = 50;
	ex->avel[2] = 50;

	S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_fireBall, 1, 1);
}

static void sunBallUpdate(h2explosion_t* ex)
{
	ex->scale = 121 - ((cl.serverTime - ex->startTime) * 3 / 20);
}

void CLHW_SunStaffExplosions(const vec3_t pos)
{
	for (int i = 0; i < 2; i++)
	{
		h2explosion_t* ex = CLH2_AllocExplosion();
		VectorCopy(pos, ex->origin);
		if (i)
		{
			ex->model = R_RegisterModel("models/stsunsf3.mdl");
			ex->scale = 200;
		}
		else
		{
			ex->model = R_RegisterModel("models/blast.mdl");
			ex->flags |= H2DRF_TRANSLUCENT;
			ex->frameFunc = sunBallUpdate;
			ex->scale = 120;
		}
		ex->flags |= H2MLS_ABSLIGHT | H2SCALE_TYPE_UNIFORM | H2SCALE_ORIGIN_CENTER;
		ex->abslight = 128;
		ex->skin = 0;
		ex->scale = 200;
		ex->startTime = cl.serverTime;
		ex->endTime = ex->startTime + 800;

		ex->exflags = EXFLAG_ROTATE;

		ex->avel[0] = 50;
		ex->avel[1] = 50;
		ex->avel[2] = 50;
	}

	S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_fireBall, 1, 1);
}

void CLHW_ParsePurify2Explode(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->model = R_RegisterModel("models/xplod29.spr");
	ex->startTime = cl.serverTime;
	ex->flags |= H2MLS_ABSLIGHT;
	ex->abslight = 128;
	ex->skin = 0;
	ex->scale = 100;
	ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 50;

	for (int i = 0; i < 12; i++)
	{
		ex = CLH2_AllocExplosion();
		VectorCopy(pos, ex->origin);
		ex->origin[0] += (rand() % 20) - 10;
		ex->origin[1] += (rand() % 20) - 10;
		ex->origin[2] += (rand() % 20) - 10;

		ex->velocity[0] = (ex->origin[0] - pos[0]) * 12;
		ex->velocity[1] = (ex->origin[1] - pos[1]) * 12;
		ex->velocity[2] = (ex->origin[2] - pos[2]) * 12;

		switch (rand() % 4)
		{
		case 0:
		case 1:
			ex->model = R_RegisterModel("models/sm_expld.spr");
			break;
		case 2:
			ex->model = R_RegisterModel("models/bg_expld.spr");
			break;
		case 3:
			ex->model = R_RegisterModel("models/gen_expl.spr");
			break;
		}
		if (cls.frametime < 70)
		{
			ex->flags |= H2MLS_ABSLIGHT | H2DRF_TRANSLUCENT;
		}
		ex->abslight = 160 + rand() % 64;
		ex->skin = 0;
		ex->scale = 80 + rand() % 40;
		ex->startTime = cl.serverTime + 5 * (rand() % 50);
		ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 40;
	}

	S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_purify2, 1, 1);
}

void CLHW_ParsePurify1Effect(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	float angle = message.ReadByte() * idMath::TWO_PI / 256.0;
	float pitch = message.ReadByte() * idMath::TWO_PI / 256.0;
	float dist = message.ReadShort();

	vec3_t endPos;
	endPos[0] = pos[0] + dist* cos(angle) * cos(pitch);
	endPos[1] = pos[1] + dist* sin(angle) * cos(pitch);
	endPos[2] = pos[2] + dist* sin(pitch);

	//CLH2_TrailParticles (pos, endPos, rt_purify);
	CLH2_TrailParticles(pos, endPos, rt_purify);

	S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_purify1_fire, 1, 1);
	S_StartSound(endPos, CLH2_TempSoundChannel(), 0, clh2_sfx_purify1_hit, 1, 1);

	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(endPos, ex->origin);
	ex->model = R_RegisterModel("models/fcircle.spr");
	ex->startTime = cl.serverTime;
	ex->flags |= H2MLS_ABSLIGHT;
	ex->abslight = 128;
	ex->skin = 0;
	ex->scale = 100;
	ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 50;
}

static void telPuffMove(h2explosion_t* ex)
{
	vec3_t tvec, tvec2;
	VectorSubtract(ex->angles, ex->origin, tvec);
	VectorCopy(tvec, tvec2);
	VectorNormalize(tvec);
	VectorScale(tvec, 320, tvec);

	ex->velocity[0] = tvec[1];
	ex->velocity[1] = -tvec[0];
	ex->velocity[2] += FRANDOM();

	if (ex->velocity[2] > 40)
	{
		ex->velocity[2] = 30;
	}

	//keep it from getting too spread out:
	ex->velocity[0] += tvec2[0];
	ex->velocity[1] += tvec2[1];

	CLH2_TrailParticles(ex->oldorg, ex->origin, rt_magicmissile);
}

static void telEffectUpdate(h2explosion_t* ex)
{
	if (ex->endTime - cl.serverTime <= 1200)
	{
		ex->frameFunc = NULL;
	}

	int testVal = cl.serverTime / 100;
	int testVal2 = (cl.serverTime - cls.frametime) / 100;

	if (testVal != testVal2)
	{
		if (!(testVal % 3))
		{
			h2explosion_t* ex2 = CLH2_AllocExplosion();
			float angle = FRANDOM() * 3.14159;

			VectorCopy(ex->origin, ex2->origin);
			VectorCopy(ex->origin, ex2->angles);
			ex2->origin[0] += cos(angle) * 10;
			ex2->origin[1] += sin(angle) * 10;

			vec3_t tvec;
			VectorSubtract(ex->origin, ex2->origin, tvec);
			VectorScale(tvec, 20, tvec);
			ex2->model = R_RegisterModel("models/sm_blue.spr");
			ex2->startTime = cl.serverTime;
			ex2->endTime = ex2->startTime + 3200;

			ex2->scale = 100;

			ex2->data = 0;
			ex2->skin = 0;
			ex2->frameFunc = telPuffMove;

			ex2->velocity[0] = tvec[1];
			ex2->velocity[1] = -tvec[0];
			ex2->velocity[2] = 20.0;

			ex2->flags |= H2MLS_ABSLIGHT;
			ex2->abslight = 128;
		}
	}

	if (ex->endTime > cl.serverTime + 10)
	{
		ex->startTime = cl.serverTime + 10;
	}
}

void CLHW_ParseTeleportLinger(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	float duration = message.ReadCoord();

	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->model = R_RegisterModel("models/bspark.spr");
	ex->startTime = cl.serverTime;
	ex->flags |= H2MLS_ABSLIGHT;
	ex->abslight = 128;
	ex->frameFunc = telEffectUpdate;
	ex->skin = 0;
	ex->scale = 0;
	ex->endTime = ex->startTime + duration * 1000;
}

void CLHW_ParseLineExplosion(QMsg& message)
{
	vec3_t curPos;
	float ratio;

	vec3_t pos, endPos;
	message.ReadPos(pos);
	message.ReadPos(endPos);

	vec3_t midPos;
	VectorAdd(pos, endPos, midPos);
	VectorScale(midPos, 0.5, midPos);

	vec3_t distVec;
	VectorSubtract(midPos, pos, distVec);
	int distance = (int)(VectorNormalize(distVec) * 0.025);
	if (distance > 0)
	{
		VectorScale(distVec, 40, distVec);

		VectorCopy(midPos, curPos);

		for (int i = 0; i < distance; i++)
		{
			h2explosion_t* ex = CLH2_AllocExplosion();
			VectorCopy(curPos, ex->origin);
			switch (rand() % 3)
			{
			case 0:
				ex->model = R_RegisterModel("models/gen_expl.spr");
				break;
			case 1:
				ex->model = R_RegisterModel("models/bg_expld.spr");
				break;
			case 2:
				ex->model = R_RegisterModel("models/sm_expld.spr");
				break;
			}
			ex->flags |= H2MLS_ABSLIGHT;
			ex->abslight = 128;
			ex->skin = 0;
			ratio = (float)i / (float)distance;
			ex->scale = 200 - (int)(150.0 * ratio);
			ex->startTime = cl.serverTime + ratio * 750;
			ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * (25 + FRANDOM() * 10);

			VectorAdd(curPos, distVec, curPos);
		}

		VectorScale(distVec, -1, distVec);
		VectorCopy(midPos, curPos);

		for (int i = 0; i < distance; i++)
		{
			h2explosion_t* ex = CLH2_AllocExplosion();
			VectorCopy(curPos, ex->origin);
			switch (rand() % 3)
			{
			case 0:
				ex->model = R_RegisterModel("models/gen_expl.spr");
				break;
			case 1:
				ex->model = R_RegisterModel("models/bg_expld.spr");
				break;
			case 2:
				ex->model = R_RegisterModel("models/sm_expld.spr");
				break;
			}
			ex->flags |= H2MLS_ABSLIGHT;
			ex->abslight = 128;
			ex->skin = 0;
			ratio = (float)i / (float)distance;
			ex->scale = 200 - (int)(150.0 * ratio);
			ex->startTime = cl.serverTime + ratio * 750;
			ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * (25 + FRANDOM() * 10);

			VectorAdd(curPos, distVec, curPos);
		}
	}
}

static void MeteorBlastThink(h2explosion_t* ex)
{
	CLH2_TrailParticles(ex->oldorg, ex->origin, rt_fireball);

	ex->data -= 1.6 * cls.frametime;// decrease distance, roughly...

	bool hitWall = false;
	vec3_t oldPos;
	if (ex->data <= 0)
	{
		// ran out of juice
		VectorCopy(ex->origin, oldPos);
		hitWall = true;
	}
	else
	{
		vec3_t tempVect;
		VectorCopy(ex->oldorg, tempVect);

		for (int i = 0; (i < 10) && !hitWall; i++)
		{
			VectorCopy(tempVect, oldPos);
			VectorScale(ex->origin, .1 * (i + 1), tempVect);
			VectorMA(tempVect, 1.0 - (.1 * (i + 1)), ex->oldorg, tempVect);
			if (CM_PointContentsQ1(tempVect, 0) != BSP29CONTENTS_EMPTY)
			{
				hitWall = true;
			}
		}
	}

	if (hitWall)
	{
		//collided with world
		VectorCopy(oldPos, ex->origin);

		int maxI = (cls.frametime <= 50) ? 12 : 5;
		for (int i = 0; i < maxI; i++)
		{
			h2explosion_t* ex2 = CLH2_AllocExplosion();
			VectorCopy(ex->origin, ex2->origin);
			ex2->origin[0] += (rand() % 10) - 5;
			ex2->origin[1] += (rand() % 10) - 5;
			ex2->origin[2] += (rand() % 10);

			ex2->velocity[0] = (ex2->origin[0] - ex->origin[0]) * 12;
			ex2->velocity[1] = (ex2->origin[1] - ex->origin[1]) * 12;
			ex2->velocity[2] = (ex2->origin[2] - ex->origin[2]) * 12;

			switch (rand() % 4)
			{
			case 0:
			case 1:
				ex2->model = R_RegisterModel("models/sm_expld.spr");
				break;
			case 2:
				ex2->model = R_RegisterModel("models/bg_expld.spr");
				break;
			case 3:
				ex2->model = R_RegisterModel("models/gen_expl.spr");
				break;
			}
			ex2->flags |= H2MLS_ABSLIGHT | H2DRF_TRANSLUCENT;
			ex2->abslight = 160 + rand() % 64;
			ex2->skin = 0;
			ex2->scale = 80 + rand() % 40;
			ex2->startTime = cl.serverTime + 5 * (rand() % 30);
			ex2->endTime = ex2->startTime + R_ModelNumFrames(ex2->model) * 30;
		}
		if (rand() & 1)
		{
			S_StartSound(ex->origin, CLH2_TempSoundChannel(), 0, clh2_sfx_axeExplode, 1, 1);
		}

		ex->model = 0;
		ex->endTime = cl.serverTime;
	}
}

static void MeteorCrushSpawnThink(h2explosion_t* ex)
{
	float chance;
	if (cls.frametime <= 50)
	{
		chance = (cls.frametime / 25.0);
	}
	else
	{
		chance = (cls.frametime / 50.0);
	}

	while (chance > 0)
	{
		if (chance < 1.0)
		{
			// take care of fractional numbers of meteors...
			if ((rand() % 100) > chance * 100)
			{
				return;
			}
		}

		h2explosion_t* ex2 = CLH2_AllocExplosion();
		VectorCopy(ex->origin, ex2->origin);
		ex2->origin[0] += (rand() % 160) - 80;
		ex2->origin[1] += (rand() % 160) - 80;
		ex2->model = R_RegisterModel("models/tempmetr.mdl");
		ex2->startTime = cl.serverTime;
		ex2->endTime = ex2->startTime + 2000;
		ex2->frameFunc = MeteorBlastThink;

		ex2->exflags = EXFLAG_ROTATE;

		ex2->avel[0] = 800;
		ex2->avel[1] = 800;
		ex2->avel[2] = 800;

		ex2->velocity[0] = rand() % 180 - 90;
		ex2->velocity[1] = rand() % 180 - 90;
		ex2->velocity[2] = -1600 - (rand() % 200);
		ex2->data = ex->data;	// stores how far the thingy can travel max

		ex2->scale = 200 + (rand() % 120);

		chance -= 1.0;
	}
}

void CLHW_ParseMeteorCrush(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	float maxDist = message.ReadLong();

	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->model = R_RegisterModel("models/null.spr");
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + 400;
	ex->frameFunc = MeteorCrushSpawnThink;
	ex->data = maxDist;

	S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_axeExplode, 1, 1);
}

void CLHW_ParseAcidBall(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);

	int i = (cls.frametime < 70) ? 0 : 2;
	for (; i < 5; i++)
	{
		h2explosion_t* ex = CLH2_AllocExplosion();
		VectorCopy(pos, ex->origin);
		ex->origin[0] += rand() % 6 - 3;
		ex->origin[1] += rand() % 6 - 3;
		ex->origin[2] += rand() % 6 - 3;

		ex->velocity[0] = (ex->origin[0] - pos[0]) * 6;
		ex->velocity[1] = (ex->origin[1] - pos[1]) * 6;
		ex->velocity[2] = (ex->origin[2] - pos[2]) * 6;

		ex->model = R_RegisterModel("models/axplsn_2.spr");
		if (cls.frametime < 70)
		{
			ex->flags |= H2MLS_ABSLIGHT | H2DRF_TRANSLUCENT;
		}
		ex->abslight = 160 + rand() % 24;
		ex->skin = 0;
		ex->scale = 80 + rand() % 40;
		ex->startTime = cl.serverTime + 5 * (rand() % 50);
		ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 50;
	}

	S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_acidhit, 1, 1);
}

void CLHW_ParseFireWall(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	float travelAng = message.ReadByte() * idMath::TWO_PI / 256.0;
	float travelPitch = message.ReadByte() * idMath::TWO_PI / 256.0;
	int fireCounts = message.ReadByte();

	CLH2_DimLight(0, pos);

	vec3_t endPos;
	VectorCopy(pos, endPos);
	endPos[0] += cos(travelAng) * cos(travelPitch) * 450;
	endPos[1] += sin(travelAng) * cos(travelPitch) * 450;
	endPos[2] += sin(travelPitch) * 450;

	vec3_t curPos;
	VectorCopy(pos, curPos);
	vec3_t posAdd;
	VectorSubtract(endPos, pos, posAdd);
	VectorScale(posAdd, .125, posAdd);

	for (int i = 0; i < fireCounts; i++)
	{
		h2explosion_t* ex = CLH2_AllocExplosion();
		VectorCopy(curPos, ex->origin);
		switch (rand() % 3)
		{
		case 0:
			ex->model = R_RegisterModel("models/firewal1.spr");
			break;
		case 1:
			ex->model = R_RegisterModel("models/firewal5.spr");
			break;
		case 2:
			ex->model = R_RegisterModel("models/firewal4.spr");
			break;
		}
		ex->startTime = cl.serverTime + 300 * i / 8;
		ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 50;

		int PtContents;
		do
		{	// I dunno how expensive this is, but it kind of sucks anyway around it...
			PtContents = CM_PointContentsQ1(ex->origin, 0);

			if (PtContents == BSP29CONTENTS_EMPTY)
			{
				ex->origin[2] -= 16;
			}
			else
			{
				ex->origin[2] += 16;
			}
		}
		while (PtContents == BSP29CONTENTS_EMPTY);
		ex->origin[0] += (rand() % 8) - 4;
		ex->origin[1] += (rand() % 8) - 4;
		ex->origin[2] += (rand() % 6) + 21;

		ex = CLH2_AllocExplosion();
		VectorCopy(curPos, ex->origin);
		ex->origin[0] += (rand() % 8) - 4;
		ex->origin[1] += (rand() % 8) - 4;
		ex->origin[2] += (rand() % 6) - 3;
		ex->model = R_RegisterModel("models/flamestr.spr");
		ex->startTime = cl.serverTime + 300 * i / 8;
		ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 50;
		ex->flags |= H2DRF_TRANSLUCENT;

		VectorAdd(curPos, posAdd, curPos);
	}
}

void CLHW_ParseFireWallImpact(QMsg& message)
{
	// Add in the actual explosion
	vec3_t pos;
	message.ReadPos(pos);

	int i = (cls.frametime < 70) ? 0 : 8;
	for (; i < 12; i++)
	{
		h2explosion_t* ex = CLH2_AllocExplosion();
		VectorCopy(pos, ex->origin);
		ex->origin[0] += (rand() % 32) - 16;
		ex->origin[1] += (rand() % 32) - 16;
		ex->origin[2] += (rand() % 32) - 16;
		ex->model = R_RegisterModel("models/fboom.spr");
		ex->startTime = cl.serverTime + 5 * (rand() % 150);
		ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 50;
	}

	S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_flameend, 1, 1);
}

void CLHW_ParsePowerFlame(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	float travelAng = message.ReadByte() * idMath::TWO_PI / 256.0;
	float travelPitch = message.ReadByte() * idMath::TWO_PI / 256.0;
	int fireCounts = message.ReadByte();
	float svTime = message.ReadLong();

	vec3_t angles;
	angles[0] = RAD2DEG(travelPitch);
	angles[1] = RAD2DEG(travelAng);
	angles[2] = 0;

	vec3_t forward, right, up;
	AngleVectors(angles, forward, right, up);

	CLH2_DimLight(0, pos);

	vec3_t endPos;
	VectorCopy(pos, endPos);
	endPos[0] += cos(travelAng) * cos(travelPitch) * 375;
	endPos[1] += sin(travelAng) * cos(travelPitch) * 375;
	endPos[2] += sin(travelPitch) * 375;

	vec3_t curPos, posAdd;
	VectorCopy(pos, curPos);
	VectorSubtract(endPos, pos, posAdd);
	VectorScale(posAdd, .125, posAdd);

	for (int i = 0; i < fireCounts; i++)
	{
		float cVal = cos((svTime + (i * .3 / 8.0)) * 8) * 10;
		float sVal = sin((svTime + (i * .3 / 8.0)) * 8) * 10;

		h2explosion_t* ex = CLH2_AllocExplosion();
		VectorCopy(curPos, ex->origin);
		VectorMA(ex->origin, cVal, right, ex->origin);
		VectorMA(ex->origin, sVal, up, ex->origin);
		ex->origin[0] += (rand() % 8) - 4;
		ex->origin[1] += (rand() % 8) - 4;
		ex->origin[2] += (rand() % 6) - 3;
		ex->model = R_RegisterModel("models/flamestr.spr");
		ex->startTime = cl.serverTime + 300 * i / 8;
		ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 50;
		ex->flags |= H2DRF_TRANSLUCENT;

		ex->velocity[0] = 0;
		ex->velocity[1] = 0;
		ex->velocity[2] = 0;
		VectorMA(ex->velocity, cVal * 4.0, right, ex->velocity);
		VectorMA(ex->velocity, sVal * 4.0, up, ex->velocity);

		ex = CLH2_AllocExplosion();
		VectorCopy(curPos, ex->origin);
		VectorMA(ex->origin, -cVal, right, ex->origin);
		VectorMA(ex->origin, -sVal, up, ex->origin);
		ex->origin[0] += (rand() % 8) - 4;
		ex->origin[1] += (rand() % 8) - 4;
		ex->origin[2] += (rand() % 6) - 3;
		ex->model = R_RegisterModel("models/flamestr.spr");
		ex->startTime = cl.serverTime + 300 * i / 8;
		ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 50;
		ex->flags |= H2DRF_TRANSLUCENT;

		ex->velocity[0] = 0;
		ex->velocity[1] = 0;
		ex->velocity[2] = 0;
		VectorMA(ex->velocity, -cVal * 4.0, right, ex->velocity);
		VectorMA(ex->velocity, -sVal * 4.0, up, ex->velocity);

		VectorAdd(curPos, posAdd, curPos);
	}
}

static void updateBloodRain(h2explosion_t* ex)
{
	CLH2_TrailParticles(ex->oldorg, ex->origin, rt_blood);

	ex->scale -= cls.frametime / 5.0;
	if (ex->scale <= 0)
	{
		ex->endTime = cl.serverTime - 1000;
	}
}

void CLHW_ParseBloodRain(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	float travelAng = message.ReadByte() * idMath::TWO_PI / 256.0;
	float travelPitch = message.ReadByte() * idMath::TWO_PI / 256.0;
	float trailLen = message.ReadByte();
	byte health = message.ReadByte();

	vec3_t vel;
	vel[0] = cos(travelAng) * cos(travelPitch) * 800;
	vel[1] = sin(travelAng) * cos(travelPitch) * 800;
	vel[2] = sin(travelPitch) * 800;

	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->model = R_RegisterModel("models/sucwp1p.mdl");
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + trailLen * 10 / 8;
	ex->angles[0] = RAD2DEG(travelPitch);
	ex->angles[1] = RAD2DEG(travelAng);
	ex->scale = health;
	VectorCopy(vel, ex->velocity);
	ex->frameFunc = updateBloodRain;
	ex->exflags |= EXFLAG_COLLIDE;

	if (health > 90)
	{
		vec3_t angles;
		angles[0] = RAD2DEG(travelPitch);
		angles[1] = RAD2DEG(travelAng);
		angles[2] = 0;

		vec3_t forward, right, up;
		AngleVectors(angles, forward, right, up);

		ex = CLH2_AllocExplosion();
		VectorCopy(pos, ex->origin);
		VectorMA(ex->origin, 7, right, ex->origin);
		ex->model = R_RegisterModel("models/sucwp1p.mdl");
		ex->startTime = cl.serverTime;
		ex->endTime = ex->startTime + trailLen * 10 / 8;
		ex->angles[0] = RAD2DEG(travelPitch);
		ex->angles[1] = RAD2DEG(travelAng);
		ex->scale = health - 90;
		VectorCopy(vel, ex->velocity);
		ex->frameFunc = updateBloodRain;

		ex = CLH2_AllocExplosion();
		VectorCopy(pos, ex->origin);
		VectorMA(ex->origin, -7, right, ex->origin);
		ex->model = R_RegisterModel("models/sucwp1p.mdl");
		ex->startTime = cl.serverTime;
		ex->endTime = ex->startTime + trailLen * 10 / 8;
		ex->angles[0] = RAD2DEG(travelPitch);
		ex->angles[1] = RAD2DEG(travelAng);
		ex->scale = health - 90;
		VectorCopy(vel, ex->velocity);
		ex->frameFunc = updateBloodRain;
		ex->exflags |= EXFLAG_COLLIDE;
	}
}

void CLHW_ParseAxe(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	float travelAng = message.ReadByte() * idMath::TWO_PI / 256.0;
	float travelPitch = message.ReadByte() * idMath::TWO_PI / 256.0;
	float trailLen = message.ReadByte() * .01;

	vec3_t vel;
	vel[0] = cos(travelAng) * cos(travelPitch) * 1100;
	vel[1] = sin(travelAng) * cos(travelPitch) * 1100;
	vel[2] = sin(travelPitch) * 1100;

	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->model = R_RegisterModel("models/axblade.mdl");
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + trailLen * 300;
	ex->angles[0] = RAD2DEG(travelPitch);
	ex->angles[1] = RAD2DEG(travelAng);
	VectorCopy(vel, ex->velocity);
	ex->exflags |= EXFLAG_COLLIDE;

	ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->model = R_RegisterModel("models/axtail.mdl");
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + trailLen * 300;
	ex->angles[0] = RAD2DEG(travelPitch);
	ex->angles[1] = RAD2DEG(travelAng);
	VectorCopy(vel, ex->velocity);
	ex->exflags |= EXFLAG_COLLIDE;
	ex->exflags |= EXFLAG_STILL_FRAME;
	ex->data = 0;
	ex->flags |= H2MLS_ABSLIGHT | H2DRF_TRANSLUCENT;
	ex->abslight = 128;
	ex->skin = 0;
}

static void SmokeRingFrameFunc(h2explosion_t* ex)
{
	if (cl.serverTime - ex->startTime < 300)
	{
		ex->skin = 0;
	}
	else if (cl.serverTime - ex->startTime < 600)
	{
		ex->skin = 1;
	}
	else if (cl.serverTime - ex->startTime < 900)
	{
		ex->skin = 2;
	}
	else
	{
		ex->skin = 3;
	}
}

static void updatePurify2(h2explosion_t* ex)
{
	CLH2_TrailParticles(ex->oldorg, ex->origin, rt_purify);

	int numSprites;
	if (cls.frametime <= 50)
	{
		numSprites = 20;
	}
	else
	{
		numSprites = 8;
	}

	if ((rand() % 100) < numSprites * cls.frametime / 10)
	{
		h2explosion_t* ex2 = CLH2_AllocExplosion();
		VectorCopy(ex->origin, ex2->origin);
		ex2->model = R_RegisterModel("models/ring.mdl");
		ex2->startTime = cl.serverTime;
		ex2->endTime = ex2->startTime + 1200;

		ex2->scale = 150;

		VectorCopy(ex->angles, ex2->angles);
		ex2->angles[2] += 90;
		ex2->exflags |= EXFLAG_STILL_FRAME;
		ex2->data = 0;
		ex2->skin = 0;
		ex2->frameFunc = SmokeRingFrameFunc;

		ex2->velocity[2] = 15.0;

		ex2->flags |= H2DRF_TRANSLUCENT | H2SCALE_ORIGIN_CENTER;
	}
}

void CLHW_ParsePurify2Missile(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	float travelAng = message.ReadByte() * idMath::TWO_PI / 256.0;
	float travelPitch = message.ReadByte() * idMath::TWO_PI / 256.0;
	float trailLen = message.ReadByte() * .01;

	vec3_t vel;
	vel[0] = cos(travelAng) * cos(travelPitch) * 1000;
	vel[1] = sin(travelAng) * cos(travelPitch) * 1000;
	vel[2] = sin(travelPitch) * 1000;

	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->model = R_RegisterModel("models/drgnball.mdl");
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + trailLen * 300;
	ex->angles[0] = RAD2DEG(travelPitch);
	ex->angles[1] = RAD2DEG(travelAng);
	VectorCopy(vel, ex->velocity);
	ex->exflags |= EXFLAG_COLLIDE;
	ex->frameFunc = updatePurify2;
	ex->scale = 150;
}

static void updateSwordShot(h2explosion_t* ex)
{
	CLH2_TrailParticles(ex->oldorg, ex->origin, rt_vorpal);

	ex->data = 16 + ((cl.serverTime / 50) % 13);

	ex->flags |= H2MLS_ABSLIGHT;
	ex->abslight = 128;

	int testVal = cl.serverTime / 50;
	if (testVal % 2)
	{
		ex->skin = 0;
	}
	else
	{
		ex->skin = 1;
	}

}

void CLHW_ParseSwordShot(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	float travelAng = message.ReadByte() * idMath::TWO_PI / 256.0;
	float travelPitch = message.ReadByte() * idMath::TWO_PI / 256.0;
	float trailLen = message.ReadByte() * .01;

	vec3_t vel;
	vel[0] = cos(travelAng) * cos(travelPitch) * 1200;
	vel[1] = sin(travelAng) * cos(travelPitch) * 1200;
	vel[2] = sin(travelPitch) * 1200;

	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->model = R_RegisterModel("models/vorpshot.mdl");
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + trailLen * 300;
	ex->angles[0] = RAD2DEG(travelPitch);
	ex->angles[1] = RAD2DEG(travelAng);
	VectorCopy(vel, ex->velocity);
	ex->exflags |= EXFLAG_COLLIDE;
	ex->frameFunc = updateSwordShot;
	ex->scale = 100;
	ex->exflags |= EXFLAG_STILL_FRAME;
	ex->data = 16 + ((int)(cl.serverTime * 0.001 * 20.0) % 13);
}

static void updateIceShot(h2explosion_t* ex)
{
	CLH2_TrailParticles(ex->oldorg, ex->origin, rt_ice);
}

void CLHW_ParseIceShot(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	float travelAng = message.ReadByte() * idMath::TWO_PI / 256.0;
	float travelPitch = message.ReadByte() * idMath::TWO_PI / 256.0;
	float trailLen = message.ReadByte() * .01;

	vec3_t vel;
	vel[0] = cos(travelAng) * cos(travelPitch) * 1200;
	vel[1] = sin(travelAng) * cos(travelPitch) * 1200;
	vel[2] = sin(travelPitch) * 1200;

	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->model = R_RegisterModel("models/iceshot1.mdl");
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + trailLen * 300;
	ex->angles[0] = RAD2DEG(travelPitch);
	ex->angles[1] = RAD2DEG(travelAng);
	VectorCopy(vel, ex->velocity);
	ex->exflags |= EXFLAG_COLLIDE;
	ex->frameFunc = updateIceShot;
	ex->scale = 100;

	ex->exflags = EXFLAG_ROTATE;

	ex->avel[0] = 425;
	ex->avel[1] = 425;
	ex->avel[2] = 425;

	h2explosion_t* ex2 = ex;

	ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->model = R_RegisterModel("models/iceshot2.mdl");
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + trailLen * 300;
	ex->angles[0] = RAD2DEG(travelPitch);
	ex->angles[1] = RAD2DEG(travelAng);
	VectorCopy(vel, ex->velocity);
	ex->exflags |= EXFLAG_COLLIDE;
	ex->scale = 200;
	ex->flags |= H2MLS_ABSLIGHT | H2DRF_TRANSLUCENT;
	ex->abslight = 128;
	ex->exflags = EXFLAG_ROTATE;

	VectorCopy(ex2->avel, ex->avel);
}

static void updateMeteor(h2explosion_t* ex)
{
	CLH2_TrailParticles(ex->oldorg, ex->origin, rt_smoke);
}

void CLHW_ParseMeteor(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	float travelAng = message.ReadByte() * idMath::TWO_PI / 256.0;
	float travelPitch = message.ReadByte() * idMath::TWO_PI / 256.0;
	float trailLen = message.ReadByte() * .01;

	vec3_t vel;
	vel[0] = cos(travelAng) * cos(travelPitch) * 1000;
	vel[1] = sin(travelAng) * cos(travelPitch) * 1000;
	vel[2] = sin(travelPitch) * 1000;

	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->model = R_RegisterModel("models/tempmetr.mdl");
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + trailLen * 300;
	ex->angles[0] = RAD2DEG(travelPitch);
	ex->angles[1] = RAD2DEG(travelAng);
	VectorCopy(vel, ex->velocity);
	ex->exflags |= EXFLAG_COLLIDE;
	ex->frameFunc = updateMeteor;
	ex->scale = 100;

	ex->exflags = EXFLAG_ROTATE;

	ex->avel[0] = 200;
	ex->avel[1] = 200;
	ex->avel[2] = 200;
}

void CLHW_ParseMegaMeteor(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	float travelAng = message.ReadByte() * idMath::TWO_PI / 256.0;
	float travelPitch = message.ReadByte() * idMath::TWO_PI / 256.0;
	float trailLen = message.ReadByte() * .01;

	vec3_t vel;
	vel[0] = cos(travelAng) * cos(travelPitch) * 1600;
	vel[1] = sin(travelAng) * cos(travelPitch) * 1600;
	vel[2] = sin(travelPitch) * 1600;

	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->model = R_RegisterModel("models/tempmetr.mdl");
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + trailLen * 300;
	ex->angles[0] = RAD2DEG(travelPitch);
	ex->angles[1] = RAD2DEG(travelAng);
	VectorCopy(vel, ex->velocity);
	ex->exflags |= EXFLAG_COLLIDE;
	ex->frameFunc = updateMeteor;
	ex->scale = 230;
	ex->flags |= H2DRF_TRANSLUCENT;

	ex->exflags = EXFLAG_ROTATE;

	ex->avel[0] = 200;
	ex->avel[1] = 200;
	ex->avel[2] = 200;
}

void CLHW_ParseLightningBall(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	float travelAng = message.ReadByte() * idMath::TWO_PI / 256.0;
	float travelPitch = message.ReadByte() * idMath::TWO_PI / 256.0;
	float speed = message.ReadShort();
	float trailLen = message.ReadByte();

	// light
	CLH2_ExplosionLight(pos);

	vec3_t vel;
	vel[0] = cos(travelAng) * cos(travelPitch) * speed;
	vel[1] = sin(travelAng) * cos(travelPitch) * speed;
	vel[2] = sin(travelPitch) * speed;

	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->model = R_RegisterModel("models/lball.mdl");
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + trailLen * 2;
	ex->angles[0] = RAD2DEG(travelPitch);
	ex->angles[1] = RAD2DEG(travelAng);
	VectorCopy(vel, ex->velocity);
	ex->exflags |= EXFLAG_COLLIDE;
}

static void updateAcidBall(h2explosion_t* ex)
{
	CLH2_TrailParticles(ex->oldorg, ex->origin, rt_acidball);
}

void CLHW_ParseAcidBallFly(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	float travelAng = message.ReadByte() * idMath::TWO_PI / 256.0;
	float travelPitch = message.ReadByte() * idMath::TWO_PI / 256.0;
	float trailLen = message.ReadByte() * .01;

	vec3_t vel;
	vel[0] = cos(travelAng) * cos(travelPitch) * 850;
	vel[1] = sin(travelAng) * cos(travelPitch) * 850;
	vel[2] = sin(travelPitch) * 850;

	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->model = R_RegisterModel("models/sucwp2p.mdl");
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + trailLen * 300;
	ex->angles[0] = RAD2DEG(travelPitch);
	ex->angles[1] = RAD2DEG(travelAng);
	VectorCopy(vel, ex->velocity);
	ex->exflags |= EXFLAG_COLLIDE;
	ex->frameFunc = updateAcidBall;
}

static void updateAcidBlob(h2explosion_t* ex)
{
	CLH2_TrailParticles(ex->oldorg, ex->origin, rt_acidball);

	int testVal = cl.serverTime / 100;
	int testVal2 = (cl.serverTime - cls.frametime) / 100;

	if (testVal != testVal2)
	{
		if (!(testVal % 2))
		{
			h2explosion_t* ex2 = CLH2_AllocExplosion();
			VectorCopy(ex->origin, ex2->origin);
			ex2->model = R_RegisterModel("models/muzzle1.spr");
			ex2->startTime = cl.serverTime;
			ex2->endTime = ex2->startTime + 400;

			VectorCopy(ex->angles, ex2->angles);
			ex2->angles[2] += 90;
			ex2->skin = 0;

			ex2->velocity[0] = 0;
			ex2->velocity[1] = 0;
			ex2->velocity[2] = 0;

			ex2->flags |= H2DRF_TRANSLUCENT | H2SCALE_ORIGIN_CENTER;
		}
	}
}

void CLHW_ParseAcidBlobFly(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	float travelAng = message.ReadByte() * idMath::TWO_PI / 256.0;
	float travelPitch = message.ReadByte() * idMath::TWO_PI / 256.0;
	float trailLen = message.ReadByte();

	vec3_t vel;
	vel[0] = cos(travelAng) * cos(travelPitch) * 1000;
	vel[1] = sin(travelAng) * cos(travelPitch) * 1000;
	vel[2] = sin(travelPitch) * 1000;

	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->model = R_RegisterModel("models/sucwp2p.mdl");
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + trailLen * 3;
	ex->angles[0] = RAD2DEG(travelPitch);
	ex->angles[1] = RAD2DEG(travelAng);
	VectorCopy(vel, ex->velocity);
	ex->exflags |= EXFLAG_COLLIDE;
	ex->frameFunc = updateAcidBlob;
	ex->scale = 230;
}

static void zapFrameFunc(h2explosion_t* ex)
{
	ex->scale = (ex->endTime - cl.serverTime) / 2 + 1;
	if ((cl.serverTime / 50) % 2)
	{
		ex->skin = 0;
	}
	else
	{
		ex->skin = 1;
	}
}

void CLHW_ChainLightningExplosion(const vec3_t pos)
{
	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->model = R_RegisterModel("models/vorpshok.mdl");
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + 300;
	ex->flags |= H2MLS_ABSLIGHT;
	ex->abslight = 224;
	ex->skin = rand() & 1;
	ex->scale = 150;
	ex->frameFunc = zapFrameFunc;
}

void CLHW_CreateExplosionWithSound(const vec3_t pos)
{
	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + HX_FRAME_MSTIME * 10;
	ex->model = R_RegisterModel("models/sm_expld.spr");

	S_StartSound(pos, CLH2_TempSoundChannel(), 1, clh2_sfx_explode, 1, 1);
}

void CLHW_UpdatePoisonGas(const vec3_t pos, const vec3_t angles)
{
	float smokeCount;
	if (cls.frametime <= 50)
	{
		smokeCount = 32 * cls.frametime * 0.001;
	}
	else
	{
		smokeCount = 16 * cls.frametime * 0.001;
	}

	while (smokeCount > 0)
	{
		if (smokeCount < 1.0)
		{
			if ((rand() % 100) / 100 > smokeCount)
			{
				// account for fractional chance of more smoke...
				smokeCount = 0;
				continue;
			}
		}

		h2explosion_t* ex = CLH2_AllocExplosion();
		VectorCopy(pos, ex->origin);
		ex->model = R_RegisterModel("models/grnsmk1.spr");
		ex->startTime = cl.serverTime;
		ex->endTime = ex->startTime + 700 + (rand() % 200);

		ex->scale = 100;

		VectorCopy(angles, ex->angles);
		ex->angles[2] += 90;
		ex->skin = 0;

		ex->velocity[0] = (rand() % 100) - 50;
		ex->velocity[1] = (rand() % 100) - 50;
		ex->velocity[2] = 150.0;

		ex->flags |= H2DRF_TRANSLUCENT | H2SCALE_ORIGIN_CENTER;

		smokeCount -= 1.0;
	}
}

void CLHW_UpdateAcidBlob(const vec3_t pos, const vec3_t angles)
{
	int testVal = cl.serverTime / 100;
	int testVal2 = (cl.serverTime - cls.frametime) / 100;

	if (testVal != testVal2)
	{
		if (!(testVal % 2))
		{
			h2explosion_t* ex = CLH2_AllocExplosion();
			VectorCopy(pos, ex->origin);
			ex->model = R_RegisterModel("models/muzzle1.spr");
			ex->startTime = cl.serverTime;
			ex->endTime = ex->startTime + 400;

			ex->scale = 100;

			VectorCopy(angles, ex->angles);
			ex->angles[2] += 90;
			ex->skin = 0;

			ex->velocity[0] = 0;
			ex->velocity[1] = 0;
			ex->velocity[2] = 0;

			ex->flags |= H2DRF_TRANSLUCENT | H2SCALE_ORIGIN_CENTER;
		}
	}
}

void CLHW_UpdateOnFire(refEntity_t* ent, const vec3_t angles, int edict_num)
{
	if (rand() % 100 < cls.frametime / 2)
	{
		h2explosion_t* ex = CLH2_AllocExplosion();
		VectorCopy(ent->origin, ex->origin);

		//raise and randomize origin some
		ex->origin[0] += (rand() % 10) - 5;
		ex->origin[1] += (rand() % 10) - 5;
		ex->origin[2] += rand() % 20 + 10;	//at least 10 up from origin, sprite's origin is in center!

		switch (rand() % 3)
		{
		case 0:
			ex->model = R_RegisterModel("models/firewal1.spr");
			break;
		case 1:
			ex->model = R_RegisterModel("models/firewal2.spr");
			break;
		case 2:
			ex->model = R_RegisterModel("models/firewal3.spr");
			break;
		}

		ex->startTime = cl.serverTime;
		ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 50;

		ex->scale = 100;

		VectorCopy(angles, ex->angles);
		ex->angles[2] += 90;

		ex->velocity[0] = (rand() % 40) - 20;
		ex->velocity[1] = (rand() % 40) - 20;
		ex->velocity[2] = 50 + (rand() % 100);

		if (rand() % 5)	//translucent 80% of the time
		{
			ex->flags |= H2DRF_TRANSLUCENT;
		}
	}
}

static void PowerFlameBurnRemove(h2explosion_t* ex)
{
	h2explosion_t* ex2 = CLH2_AllocExplosion();
	VectorCopy(ex->origin, ex2->origin);
	switch (rand() % 3)
	{
	case 0:
		ex2->model = R_RegisterModel("models/sm_expld.spr");
		break;
	case 1:
		ex2->model = R_RegisterModel("models/fboom.spr");
		break;
	case 2:
		ex2->model = R_RegisterModel("models/pow.spr");
		break;
	}
	ex2->startTime = cl.serverTime;
	ex2->endTime = ex2->startTime + R_ModelNumFrames(ex2->model) * 50;

	ex2->scale = 100;

	ex2->flags |= H2MLS_ABSLIGHT | H2DRF_TRANSLUCENT;
	ex2->abslight = 128;


	if (rand() & 1)
	{
		S_StartSound(ex2->origin, CLH2_TempSoundChannel(), 1, clh2_sfx_flameend, 1, 1);
	}
}

void CLHW_UpdatePowerFlameBurn(refEntity_t* ent, int edict_num)
{
	if (rand() % 100 < cls.frametime)
	{
		h2explosion_t* ex = CLH2_AllocExplosion();
		VectorCopy(ent->origin, ex->origin);
		ex->origin[0] += (rand() % 120) - 60;
		ex->origin[1] += (rand() % 120) - 60;
		ex->origin[2] += (rand() % 120) - 60 + 120;
		ex->model = R_RegisterModel("models/sucwp1p.mdl");
		ex->startTime = cl.serverTime;
		ex->endTime = ex->startTime + 250;
		ex->removeFunc = PowerFlameBurnRemove;

		ex->scale = 100;

		vec3_t srcVec;
		VectorSubtract(ent->origin, ex->origin, srcVec);
		VectorCopy(srcVec, ex->velocity);
		ex->velocity[2] += 24;
		VectorScale(ex->velocity, 1.0 / .25, ex->velocity);
		VectorNormalize(srcVec);
		VecToAnglesBuggy(srcVec, ex->angles);

		ex->flags |= H2MLS_ABSLIGHT;
		ex->abslight = 128;

		// I'm not seeing this right now... (?)
		h2explosion_t* ex2 = CLH2_AllocExplosion();
		VectorCopy(ex->origin, ex2->origin);
		ex2->model = R_RegisterModel("models/flamestr.spr");
		ex2->startTime = cl.serverTime;
		ex2->endTime = ex2->startTime + R_ModelNumFrames(ex2->model) * 50;
		ex2->flags |= H2DRF_TRANSLUCENT;
	}
}

static void CLHW_CreateIceChunk(const vec3_t origin)
{
	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(origin, ex->origin);
	ex->origin[0] += rand() % 32 - 16;
	ex->origin[1] += rand() % 32 - 16;
	ex->origin[2] += 48 + rand() % 32;
	ex->frameFunc = ChunkThink;

	ex->velocity[0] += (rand() % 300) - 150;
	ex->velocity[1] += (rand() % 300) - 150;
	ex->velocity[2] += (rand() % 200) - 50;

	// are these in degrees or radians?
	ex->angles[0] = rand() % 360;
	ex->angles[1] = rand() % 360;
	ex->angles[2] = rand() % 360;
	ex->exflags = EXFLAG_ROTATE;

	ex->avel[0] = rand() % 850 - 425;
	ex->avel[1] = rand() % 850 - 425;
	ex->avel[2] = rand() % 850 - 425;

	ex->scale = 65 + rand() % 10;

	ex->data = H2THINGTYPE_ICE;

	ex->model = R_RegisterModel("models/shard.mdl");
	ex->skin = 0;
	ex->flags |= H2DRF_TRANSLUCENT | H2MLS_ABSLIGHT;
	ex->abslight = 128;

	ex->startTime = cl.serverTime;
	ex->endTime = ex->startTime + 2000;
}

void CLHW_UpdateIceStorm(refEntity_t* ent, int edict_num)
{
	static int playIceSound = 600;

	h2entity_state_t* state = CLH2_FindState(edict_num);
	if (state)
	{
		vec3_t center;
		VectorCopy(state->origin, center);

		// handle the particles
		vec3_t side1;
		VectorCopy(center, side1);
		side1[0] -= 80;
		side1[1] -= 80;
		side1[2] += 104;
		vec3_t side2 = { 160, 160, 128 };
		CLH2_RainEffect2(side1, side2, rand() % 400 - 200, rand() % 400 - 200, rand() % 15 + 9 * 16,
			cls.frametime * 3 / 5);

		playIceSound += cls.frametime;
		if (playIceSound >= 600)
		{
			S_StartSound(center, CLH2_TempSoundChannel(), 0, clh2_sfx_icestorm, 1, 1);
			playIceSound -= 600;
		}
	}

	// toss little ice chunks
	if (rand() % 100 < cls.frametime * 3 / 10)
	{
		CLHW_CreateIceChunk(ent->origin);
	}
}

void CLH2_UpdateExplosions()
{
	h2explosion_t* ex = clh2_explosions;
	for (int i = 0; i < H2MAX_EXPLOSIONS; i++, ex++)
	{
		if (!ex->model)
		{
			continue;
		}

		if (ex->exflags & EXFLAG_COLLIDE)
		{
			if (CM_PointContentsQ1(ex->origin, 0) != BSP29CONTENTS_EMPTY)
			{
				if (ex->removeFunc)
				{
					ex->removeFunc(ex);
				}
				ex->model = 0;
				continue;
			}
		}

		// if we hit endTime, get rid of explosion (i assume endTime is greater than startTime, etc)
		if (ex->endTime <= cl.serverTime)
		{
			if (ex->removeFunc)
			{
				ex->removeFunc(ex);
			}
			ex->model = 0;
			continue;
		}

		VectorCopy(ex->origin, ex->oldorg);

		// set the current frame so i finish anim at endTime
		int f;
		if (ex->exflags & EXFLAG_STILL_FRAME)
		{
			// if it's a still frame, use the data field
			f = (int)ex->data;
		}
		else
		{
			f = (R_ModelNumFrames(ex->model) - 1) * (cl.serverTime - ex->startTime) / (ex->endTime - ex->startTime);
		}

		// apply velocity
		ex->origin[0] += cls.frametime * 0.001 * ex->velocity[0];
		ex->origin[1] += cls.frametime * 0.001 * ex->velocity[1];
		ex->origin[2] += cls.frametime * 0.001 * ex->velocity[2];

		// apply acceleration
		ex->velocity[0] += cls.frametime * 0.001 * ex->accel[0];
		ex->velocity[1] += cls.frametime * 0.001 * ex->accel[1];
		ex->velocity[2] += cls.frametime * 0.001 * ex->accel[2];

		// add in angular velocity
		if (ex->exflags & EXFLAG_ROTATE)
		{
			VectorMA(ex->angles, cls.frametime * 0.001, ex->avel, ex->angles);
		}
		// you can set startTime to some point in the future to delay the explosion showing up or thinking; it'll still move, though
		if (ex->startTime > cl.serverTime)
		{
			continue;
		}

		if (ex->frameFunc)
		{
			ex->frameFunc(ex);
		}

		// allow for the possibility for the frame func to reset startTime
		if (ex->startTime > cl.serverTime)
		{
			continue;
		}

		// just incase the frameFunc eliminates the thingy here.
		if (ex->model == 0)
		{
			continue;
		}

		refEntity_t ent;
		Com_Memset(&ent, 0, sizeof(ent));
		ent.reType = RT_MODEL;
		VectorCopy(ex->origin, ent.origin);
		ent.hModel = ex->model;
		ent.frame = f;
		ent.skinNum = ex->skin;
		CLH2_SetRefEntAxis(&ent, ex->angles, vec3_origin, ex->scale, 0, ex->abslight, ex->flags);
		CLH2_HandleCustomSkin(&ent, -1);
		R_AddRefEntityToScene(&ent);
	}
}

void CLHW_ClearTarget()
{
	clh2_targetDistance = 0;
}

void CLHW_ParseTarget(QMsg& message)
{
	clh2_targetAngle = message.ReadByte();
	clh2_targetPitch = message.ReadByte();
	clh2_targetDistance = message.ReadByte();
}

void CLHW_UpdateTargetBall()
{
	if (clh2_targetDistance < 24)
	{
		// either there is no ball, or it's too close to be needed...
		return;
	}

	// search for the two thingies.  If they don't exist, make new ones and set v_oldTargOrg

	qhandle_t iceMod = R_RegisterModel("models/iceshot2.mdl");

	h2explosion_t* ex1 = NULL;
	h2explosion_t* ex2 = NULL;
	for (int i = 0; i < H2MAX_EXPLOSIONS; i++)
	{
		if (clh2_explosions[i].endTime > cl.serverTime)
		{
			// make certain it's an active one
			if (clh2_explosions[i].model == iceMod)
			{
				if (clh2_explosions[i].flags & H2DRF_TRANSLUCENT)
				{
					ex1 = &clh2_explosions[i];
				}
				else
				{
					ex2 = &clh2_explosions[i];
				}
			}
		}
	}

	vec3_t newOrg;
	VectorCopy(cl.qh_simorg, newOrg);
	newOrg[0] += cos(clh2_targetAngle * idMath::PI * 2 / 256.0) * 50 * cos(clh2_targetPitch * idMath::PI * 2 / 256.0);
	newOrg[1] += sin(clh2_targetAngle * idMath::PI * 2 / 256.0) * 50 * cos(clh2_targetPitch * idMath::PI * 2 / 256.0);
	newOrg[2] += 44 + sin(clh2_targetPitch * idMath::PI * 2 / 256.0) * 50 + cos(cl.serverTime * 0.001 * 2) * 5;
	float newScale;
	if (clh2_targetDistance < 60)
	{
		// make it scale back down up close...
		newScale = 172 - (172 * (1.0 - (clh2_targetDistance - 24.0) / 36.0));
	}
	else
	{
		newScale = 80 + (120 * ((256.0 - clh2_targetDistance) / 256.0));
	}
	if (ex1 == NULL)
	{
		ex1 = CLH2_AllocExplosion();
		ex1->model = iceMod;
		ex1->exflags |= EXFLAG_STILL_FRAME;
		ex1->data = 0;

		ex1->flags |= H2MLS_ABSLIGHT | H2DRF_TRANSLUCENT;
		ex1->skin = 0;
		VectorCopy(newOrg, ex1->origin);
		ex1->scale = newScale;
	}

	VectorScale(ex1->origin, (.75 - cls.frametime * 0.001 * 1.5), ex1->origin);	// FIXME this should be affected by frametime...
	VectorMA(ex1->origin, (.25 + cls.frametime * 0.001 * 1.5), newOrg, ex1->origin);
	ex1->startTime = cl.serverTime;
	ex1->endTime = ex1->startTime + cls.frametime + 200;
	ex1->scale = (ex1->scale * (.75 - cls.frametime * 0.001 * 1.5) + newScale * (.25 + cls.frametime * 0.001 * 1.5));
	ex1->angles[0] = clh2_targetPitch * 360 / 256.0;
	ex1->angles[1] = clh2_targetAngle * 360 / 256.0;
	ex1->angles[2] = cl.serverTime * 0.001 * 240;
	ex1->abslight = 96 + (32 * cos(cl.serverTime * 0.001 * 6.5)) + (64 * ((256.0 - clh2_targetDistance) / 256.0));

	if (clh2_targetDistance < 60)
	{
		// make it scale back down up close...
		newScale = 76 - (76 * (1.0 - (clh2_targetDistance - 24.0) / 36.0));
	}
	else
	{
		newScale = 30 + (60 * ((256.0 - clh2_targetDistance) / 256.0));
	}
	if (ex2 == NULL)
	{
		ex2 = CLH2_AllocExplosion();
		ex2->model = iceMod;
		ex2->exflags |= EXFLAG_STILL_FRAME;
		ex2->data = 0;

		ex2->flags |= H2MLS_ABSLIGHT;
		ex2->skin = 0;
		ex2->scale = newScale;
	}
	VectorCopy(ex1->origin, ex2->origin);
	ex2->startTime = cl.serverTime;
	ex2->endTime = ex2->startTime + cls.frametime + 200;
	ex2->scale = (ex2->scale * (.75 - cls.frametime * 0.001 * 1.5) + newScale * (.25 + cls.frametime * 0.001 * 1.5));
	ex2->angles[0] = ex1->angles[0];
	ex2->angles[1] = ex1->angles[1];
	ex2->angles[2] = cl.serverTime * 0.001 * -360;
	ex2->abslight = 96 + (128 * cos(cl.serverTime * 0.001 * 4.5));

	CLHW_TargetBallEffectParticles(ex1->origin, clh2_targetDistance);
}
