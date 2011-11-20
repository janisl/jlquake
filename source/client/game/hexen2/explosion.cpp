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

#define FRANDOM() (rand() * (1.0 / RAND_MAX))

h2explosion_t clh2_explosions[H2MAX_EXPLOSIONS];

sfxHandle_t clh2_sfx_explode;
static sfxHandle_t clh2_sfx_bonephit;
static sfxHandle_t clh2_sfx_bonehit;
static sfxHandle_t clh2_sfx_bonewal;
static sfxHandle_t clh2_sfx_ravendie;
static sfxHandle_t clh2_sfx_ravengo;
static sfxHandle_t clh2_sfx_dropfizzle;
sfxHandle_t clh2_sfx_axeExplode;
static sfxHandle_t clh2_sfx_icewall;
static sfxHandle_t clh2_sfx_iceshatter;
static sfxHandle_t clh2_sfx_iceflesh;
static sfxHandle_t clh2_sfx_big_gib;
static sfxHandle_t clh2_sfx_gib1;
static sfxHandle_t clh2_sfx_gib2;
static sfxHandle_t clh2_sfx_telefrag;
sfxHandle_t clh2_sfx_acidhit;

static int MultiGrenadeCurrentChannel;

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
}

void CLH2_ClearExplosions()
{
	Com_Memset(clh2_explosions, 0, sizeof(clh2_explosions));
}

//**** CAREFUL!!! This may overwrite an explosion!!!!!
h2explosion_t* CLH2_AllocExplosion()
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
		float time = cl_common->serverTime * 0.001;

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
		if (ex->data < cl_common->serverTime * 0.001)//change course
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
		ex->endTime = cl_common->serverTime * 0.001;
	}
}

void CLHW_SpawnDeathBubble(const vec3_t pos)
{
	//generic spinny impact image
	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	VectorSet(ex->velocity, 0, 0, 17);
	ex->data = cl_common->serverTime * 0.001;
	ex->scale = 128;
	ex->frameFunc = BubbleThink;
	ex->startTime = cl_common->serverTime * 0.001;
	ex->endTime = cl_common->serverTime * 0.001 + 15;
	ex->model = R_RegisterModel("models/s_bubble.spr");
	ex->flags = H2DRF_TRANSLUCENT | H2MLS_ABSLIGHT;
	ex->abslight = 175;
}

void MissileFlashThink(h2explosion_t* ex)
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
	ex->startTime = cl_common->serverTime * 0.001;
	ex->endTime = ex->startTime + 2;
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

	ex->startTime = cl_common->serverTime * 0.001;
	ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.1;

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
	ex->startTime = cl_common->serverTime * 0.001 + ((1-(ex->data - 69) / 180.0) + ftemp - 0.3) * 1.25;
	if (ex->startTime < cl_common->serverTime * 0.001)
	{
		ex->startTime = cl_common->serverTime * 0.001;
	}
	ftemp = (rand() / RAND_MAX * 0.4) - 0.4;
	ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.1 + ftemp;

	if (R_ModelNumFrames(ex->model) > 14)
	{
		ex->startTime -= 0.4;
		ex->endTime -= 0.4;
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

		missile->startTime = cl_common->serverTime * 0.001 + 0.01;
		missile->endTime = missile->startTime + R_ModelNumFrames(missile->model) * 0.1;
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

	ex->startTime = cl_common->serverTime * 0.001 + ((1 - (ex->data - 69) / 200.0) * 1.5) - 0.2;
	float ftemp = (rand() / RAND_MAX * 0.4) - 0.2;
	ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.1 + ftemp;

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

		missile->startTime = cl_common->serverTime * 0.001 + 0.01;
		missile->endTime = missile->startTime + R_ModelNumFrames(missile->model) * 0.1;
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

			missile->startTime = cl_common->serverTime * 0.001;
			ftemp = (rand() / RAND_MAX * 0.3) - 0.15;
			missile->endTime = missile->startTime + R_ModelNumFrames(missile->model) * 0.1 + ftemp;
			break;
		case 1:
			missile->frameFunc = MultiGrenadePiece2Think;
			missile->velocity[0] = (rand() % 80) - 40;
			missile->velocity[1] = (rand() % 80) - 40;
			missile->velocity[2] = (rand() % 150) + 150;

			missile->startTime = cl_common->serverTime * 0.001;
			ftemp = (rand() / RAND_MAX  * 0.3) - 0.15;
			missile->endTime = missile->startTime + R_ModelNumFrames(missile->model) * 0.1 + ftemp;
			break;
		default://some extra explosions for at first
			missile->frameFunc = NULL;
			missile->origin[0] += (rand() % 50) - 25;
			missile->origin[1] += (rand() % 50) - 25;
			missile->origin[2] += (rand() % 50) - 25;

			ftemp = rand() / RAND_MAX * 0.2;
			missile->startTime = cl_common->serverTime * 0.001 + ftemp;
			ftemp = (rand() / RAND_MAX * 0.2) - 0.1;
			missile->endTime = missile->startTime + R_ModelNumFrames(missile->model) * 0.1 + ftemp;
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
	ex->startTime = cl_common->serverTime * 0.001;
	ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.1;
}

static void CLHW_XBowHit(const vec3_t pos, const vec3_t vel, int damage, int absoluteLight, void (*frameFunc)(h2explosion_t *ex))
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
	ex->startTime = cl_common->serverTime * 0.001;
	ex->endTime = cl_common->serverTime * 0.001 + 0.3;
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
		ex->startTime = cl_common->serverTime * 0.001;
		ex->endTime = cl_common->serverTime * 0.001 + 0.35;
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
	ex->startTime = cl_common->serverTime * 0.001;
	ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.1;
	for (int cnt2 = 0; cnt2 < cnt; cnt2++)
	{
		vec3_t offset;
		offset[0] = rand() % 40 - 20;
		offset[1] = rand() % 40 - 20;
		offset[2] = rand() % 40 - 20;
		ex = CLH2_AllocExplosion ();
		VectorAdd(pos, offset, ex->origin); 
		VectorMA(ex->origin, -8, movedir, ex->origin);
		VectorCopy(offset, ex->velocity);
		ex->velocity[2] += 30;
		ex->data=250;
		ex->model = R_RegisterModel("models/ghost.spr");
		ex->abslight = 128;
		ex->flags = H2DRF_TRANSLUCENT | H2MLS_ABSLIGHT;
		ex->startTime = cl_common->serverTime * 0.001;
		ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.1;
	}			
	for (int cnt2 = 0; cnt2 < 20; cnt2++)
	{
		// want faster velocity to hide the fact that these 
		// aren't in the real location
		vec3_t offset;
		offset[0] = rand() % 400 + 300;
		if(rand() % 2)
			offset[0] = -offset[0];
		offset[1] = rand() % 400 + 300;
		if(rand() % 2)
			offset[1] = -offset[1];
		offset[2] = rand() % 400 + 300;
		if(rand() % 2)
			offset[2] = -offset[2];
		ex = CLH2_AllocExplosion ();
		VectorMA(pos, 1/700, offset, ex->origin); 
		VectorCopy(offset, ex->velocity);
		ex->data=250;
		ex->model = R_RegisterModel("models/boneshrd.mdl");
		ex->startTime = cl_common->serverTime * 0.001;
		ex->endTime = ex->startTime + rand() * 50 / 100;
		ex->flags |= EXFLAG_ROTATE|EXFLAG_COLLIDE;
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
	ex->startTime = cl_common->serverTime * 0.001;
	ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.1;

	//sound
	if(cnt2)
	{
		S_StartSound(pos, CLH2_TempSoundChannel(), 1, clh2_sfx_bonehit, 1, 1);
	}
	else
	{
		S_StartSound(pos, CLH2_TempSoundChannel(), 1, clh2_sfx_bonewal, 1, 1);
	}

	CLH2_RunParticleEffect4 (pos, 3, 368 + rand() % 16, pt_h2grav, 7);
}

static void NewRavenDieExplosion(const vec3_t pos, float velocityY, const char* modelName)
{
	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->velocity[1] = velocityY;
	ex->velocity[2] = -10;
	ex->model = R_RegisterModel(modelName);
	ex->startTime = cl_common->serverTime * 0.001;
	ex->endTime = ex->startTime + HX_FRAME_TIME * 10;
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
	ex->startTime = cl_common->serverTime * 0.001;
	ex->endTime = ex->startTime + HX_FRAME_TIME * 10;
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

static void ChunkThink(h2explosion_t *ex)
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

	if (cl_common->serverTime * 0.001 + cls_common->frametime * 0.001 * 5 > ex->endTime)
	{
		// chunk leaves in 5 frames about
		switch((int)ex->data)
		{
		case H2THINGTYPE_METEOR:
			if (cl_common->serverTime * 0.001 + cls_common->frametime * 0.001 * 4 < ex->endTime)
			{
				// just crossed the threshold
				ex->abslight = 200;
			}
			else
			{
				ex->abslight -= 35;
			}
			ex->flags |= H2DRF_TRANSLUCENT|H2MLS_ABSLIGHT;
			ex->scale *= 1.4;
			ex->angles[0] += 20;
			break;
		default:
			ex->scale *= .8;
			break;
		}
	}

	ex->velocity[2] -= cls_common->frametime * 0.001 * movevars.gravity; // this is gravity

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
		ex->velocity[2] += cls_common->frametime * 0.001 * movevars.gravity * 0.5; // lower gravity for ice chunks
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

void CLHW_InitChunkExplosionCommon(h2explosion_t* ex, int chType)
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

	ex->startTime = cl_common->serverTime * 0.001;
	ex->endTime = ex->startTime + 4.0;
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
	int i = (cls_common->frametime < 70) ? 0 : 4;	// based on framerate
	for( ; i < 8; i++)
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

		ex->startTime = cl_common->serverTime * 0.001;
		ex->endTime = ex->startTime + 4.0;
	}

	// make the actual explosion
	i = (cls_common->frametime < 70) ? 0 : 8;	// based on framerate
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
		if (cls_common->frametime * 0.001 < .07)
		{
			ex->flags |= H2MLS_ABSLIGHT | H2DRF_TRANSLUCENT;
		}
		ex->abslight = 160 + rand() % 64;
		ex->skin = 0;
		ex->scale = 80 + rand() % 40;
		ex->startTime = cl_common->serverTime * 0.001 + (rand() % 50 / 200.0);
		ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.04;
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
		int i = (cls_common->frametime < 70) ? 0 : 5;	// based on framerate
		for (; i < 9; i++)
		{
			h2explosion_t* ex = CLH2_AllocExplosion();
			VectorCopy(pos,ex->origin);
			ex->frameFunc = ChunkThink;

			ex->velocity[0] += (rand()%1000)-500;
			ex->velocity[1] += (rand()%1000)-500;
			ex->velocity[2] += (rand()%200)-50;

			// are these in degrees or radians?
			ex->angles[0] = rand()%360;
			ex->angles[1] = rand()%360;
			ex->angles[2] = rand()%360;
			ex->exflags = EXFLAG_ROTATE;

			ex->avel[0] = rand()%850 - 425;
			ex->avel[1] = rand()%850 - 425;
			ex->avel[2] = rand()%850 - 425;

			if(cnt2 == 2)
			{
				ex->scale = 65 + rand()%10;
			}
			else
			{
				ex->scale = 35 + rand()%10;
			}
			ex->data = H2THINGTYPE_ICE;

			ex->model = R_RegisterModel("models/shard.mdl");
			ex->skin = 0;
			//ent->frame = rand()%2;
			ex->flags |= H2DRF_TRANSLUCENT|H2MLS_ABSLIGHT;
			ex->abslight = 128;

			ex->startTime = cl_common->serverTime * 0.001;
			ex->endTime = ex->startTime + 2.0;
			if(cnt2 == 2)
			{
				ex->endTime += 3.0;
			}
		}
	}
	else
	{
		vec3_t dmin = {-10, -10, -10};
		vec3_t dmax = {10, 10, 10};
		CLH2_ColouredParticleExplosion(pos,14,10,10);
		CLH2_RunParticleEffect2 (pos, dmin, dmax, 145, pt_h2explode, 14);
	}
	// make the actual explosion
	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(pos, ex->origin);
	ex->model = R_RegisterModel("models/icehit.spr");
	ex->startTime = cl_common->serverTime * 0.001;
	ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.1;

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
	float			throwPower, curAng, curPitch;

	vec3_t pos;
	message.ReadPos(pos);
	int angle = message.ReadByte();	// from 0 to 256
	int pitch = message.ReadByte();	// from 0 to 256
	int force = message.ReadByte();
	int style = message.ReadByte();


	int i = (cls_common->frametime < 70) ? 0 : 8;
	for (; i < 12; i++)
	{
		h2explosion_t* ex = CLH2_AllocExplosion();
		VectorCopy(pos, ex->origin);
		ex->origin[0] += (rand()%40)-20;
		ex->origin[1] += (rand()%40)-20;
		ex->origin[2] += rand()%40;
		ex->frameFunc = ChunkThink;

		throwPower = 3.5 + ((rand()%100)/100.0);
		curAng = angle*6.28/256.0 + ((rand()%100)/50.0) - 1.0;
		curPitch = pitch*6.28/256.0 + ((rand()%100)/100.0) - .5;

		ex->velocity[0] = force*throwPower * cos(curAng) * cos(curPitch);
		ex->velocity[1] = force*throwPower * sin(curAng) * cos(curPitch);
		ex->velocity[2] = force*throwPower * sin(curPitch);

		// are these in degrees or radians?
		ex->angles[0] = rand()%360;
		ex->angles[1] = rand()%360;
		ex->angles[2] = rand()%360;
		ex->exflags = EXFLAG_ROTATE;

		ex->avel[0] = rand()%850 - 425;
		ex->avel[1] = rand()%850 - 425;
		ex->avel[2] = rand()%850 - 425;

		ex->scale = 80 + rand()%40;
		ex->data = H2THINGTYPE_FLESH;

		switch(rand()%3)
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

		ex->startTime = cl_common->serverTime * 0.001;
		ex->endTime = ex->startTime + 4.0;
	}

	switch(style)
	{
	case 0:
		S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_big_gib, 1, 1);
		break;
	case 1:
		if(rand()%2)
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

	int i = (cls_common->frametime < 70) ? 0 : 7;
	for (; i < 12; i++)
	{
		h2explosion_t* ex = CLH2_AllocExplosion();
		VectorCopy(pos, ex->origin);
		ex->origin[0] += rand()%6 - 3;
		ex->origin[1] += rand()%6 - 3;
		ex->origin[2] += rand()%6 - 3;

		ex->velocity[0] = (ex->origin[0] - pos[0])*25;
		ex->velocity[1] = (ex->origin[1] - pos[1])*25;
		ex->velocity[2] = (ex->origin[2] - pos[2])*25;

		switch(rand()%4)
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
		if (cls_common->frametime < 70)
		{
			ex->flags |= H2MLS_ABSLIGHT|H2DRF_TRANSLUCENT;
		}
		ex->abslight = 1;
		ex->skin = 0;
		ex->scale = 80 + rand()%40;
		ex->startTime = cl_common->serverTime * 0.001 + (rand()%50 / 200.0);
		ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.05;
	}

	// always make 8 meteors
	i = (cls_common->frametime < 70) ? 0 : 4;
	for (; i < 8; i++)
	{
		h2explosion_t* ex = CLH2_AllocExplosion();
		VectorCopy(pos,ex->origin);
		ex->frameFunc = ChunkThink;

		// temp modify them...
		ex->velocity[0] = (rand()%500)-250;
		ex->velocity[1] = (rand()%500)-250;
		ex->velocity[2] = (rand()%200)+200;

		// are these in degrees or radians?
		ex->angles[0] = rand()%360;
		ex->angles[1] = rand()%360;
		ex->angles[2] = rand()%360;
		ex->exflags = EXFLAG_ROTATE;

		ex->avel[0] = rand()%850 - 425;
		ex->avel[1] = rand()%850 - 425;
		ex->avel[2] = rand()%850 - 425;

		ex->scale = 45 + rand()%10;
		ex->data = H2THINGTYPE_ACID;

		ex->model = R_RegisterModel("models/sucwp2p.mdl");
		ex->skin = 0;
		VectorScale(ex->avel, 4.0, ex->avel);

		ex->startTime = cl_common->serverTime * 0.001;
		ex->endTime = ex->startTime + 4.0;
	}

	S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_acidhit, 1, 1);
}

void CLHW_XbowImpact(const vec3_t pos, const vec3_t vel, int chType, int damage, int arrowType)//arrowType is total # of arrows in effect
{
	CLHW_XBowHit(pos, vel, damage, 175, MissileFlashThink);

	if (!damage)
	{
		if (arrowType == 3)//little arrows go away
		{
			if (rand() & 3)//chunky go
			{
				int cnt	= rand() % 2 + 1;
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

					ex->startTime = cl_common->serverTime * 0.001;
					ex->endTime = ex->startTime + 4.0;
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

				ex->startTime = cl_common->serverTime * 0.001;
				ex->endTime = ex->startTime + 4.0;
			}
		}
	}
}

void CLHW_CreateIceChunk(const vec3_t origin)
{
	h2explosion_t* ex = CLH2_AllocExplosion();
	VectorCopy(origin, ex->origin);
	ex->origin[0] += rand() % 32 - 16;
	ex->origin[1] += rand() % 32 - 16;
	ex->origin[2] += 48 + rand() %32;
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
	ex->flags |= H2DRF_TRANSLUCENT|H2MLS_ABSLIGHT;
	ex->abslight = 128;

	ex->startTime = cl_common->serverTime * 0.001;
	ex->endTime = ex->startTime + 2.0;
}
