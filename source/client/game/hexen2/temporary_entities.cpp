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

static sfxHandle_t clh2_sfx_tink1;
static sfxHandle_t clh2_sfx_ric1;
static sfxHandle_t clh2_sfx_ric2;
static sfxHandle_t clh2_sfx_ric3;
static sfxHandle_t clh2_sfx_r_exp3;
sfxHandle_t clh2_sfx_icestorm;
static sfxHandle_t clh2_sfx_lightning1;
static sfxHandle_t clh2_sfx_lightning2;
static sfxHandle_t clh2_sfx_sunstaff;
static sfxHandle_t clh2_sfx_sunhit;
static sfxHandle_t clh2_sfx_hammersound;
static sfxHandle_t clh2_sfx_buzzbee;

void CLH2_InitTEnts()
{
	clh2_sfx_tink1 = S_RegisterSound("weapons/tink1.wav");
	clh2_sfx_ric1 = S_RegisterSound("weapons/ric1.wav");
	clh2_sfx_ric2 = S_RegisterSound("weapons/ric2.wav");
	clh2_sfx_ric3 = S_RegisterSound("weapons/ric3.wav");

	if (GGameType & GAME_HexenWorld)
	{
		clh2_sfx_r_exp3 = S_RegisterSound("weapons/r_exp3.wav");
		clh2_sfx_icestorm = S_RegisterSound("crusader/blizzard.wav");
		clh2_sfx_lightning1 = S_RegisterSound("crusader/lghtn1.wav");
		clh2_sfx_lightning2 = S_RegisterSound("crusader/lghtn2.wav");
		clh2_sfx_sunstaff = S_RegisterSound("crusader/sunhum.wav");
		clh2_sfx_sunhit = S_RegisterSound("crusader/sunhit.wav");
		clh2_sfx_hammersound = S_RegisterSound("paladin/axblade.wav");
		clh2_sfx_buzzbee = S_RegisterSound("assassin/scrbfly.wav");

		CLHW_InitExplosionSounds();
	}
}

void CLH2_ClearTEnts()
{
	CLH2_ClearExplosions();
	CLH2_ClearStreams();
}

int CLH2_TempSoundChannel()
{
	static int last = -1;
	if (!(GGameType & GAME_HexenWorld))
	{
		return -1;
	}
	last--;
	if (last < -20)
	{
		last = -1;
	}
	return last;
}

h2entity_state_t* CLH2_FindState(int entityNumber)
{
	static h2entity_state_t pretend_player;

	if (!(GGameType & GAME_HexenWorld))
	{
		return &h2cl_entities[entityNumber].state;
	}

	if (entityNumber >= 1 && entityNumber <= HWMAX_CLIENTS)
	{
		if (entityNumber == cl.viewentity)
		{
			VectorCopy(CL_GetSimOrg(), pretend_player.origin);
		}
		else
		{
			VectorCopy(cl.hw_frames[clc.netchan.incomingSequence & UPDATE_MASK_HW].playerstate[entityNumber - 1].origin, pretend_player.origin);
		}
		return &pretend_player;
	}

	hwpacket_entities_t* pack = &cl.hw_frames[clc.netchan.incomingSequence & UPDATE_MASK_HW].packet_entities;
	for (int pnum = 0; pnum < pack->num_entities; pnum++)
	{
		if (pack->entities[pnum].number == entityNumber)
		{
			return &pack->entities[pnum];
		}
	}

	return NULL;
}

static void CLH2_ParseWizSpike(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	CLH2_RunParticleEffect(pos, vec3_origin, 30);
}

static void CLH2_ParseKnightSpike(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	CLH2_RunParticleEffect(pos, vec3_origin, 20);
}

static void CLH2_ParseSpikeCommon(QMsg& message, int count)
{
	vec3_t pos;
	message.ReadPos(pos);
	CLH2_RunParticleEffect(pos, vec3_origin, count);
	if (rand() % 5)
	{
		S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_tink1, 1, 1);
	}
	else
	{
		int rnd = rand() & 3;
		if (rnd == 1)
		{
			S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_ric1, 1, 1);
		}
		else if (rnd == 2)
		{
			S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_ric2, 1, 1);
		}
		else
		{
			S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_ric3, 1, 1);
		}
	}
}

static void CLH2_ParseSpike(QMsg& message)
{
	CLH2_ParseSpikeCommon(message, 10);
}

static void CLH2_ParseSuperSpike(QMsg& message)
{
	CLH2_ParseSpikeCommon(message, 20);
}

static void CLH2_ParseExplosion(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	CLH2_ParticleExplosion(pos);
}

static void CLHW_ParseExplosion(QMsg& message)
{
	// particles
	vec3_t pos;
	message.ReadPos(pos);
	CLH2_ParticleExplosion(pos);

	// light
	CLH2_ExplosionLight(pos);

	// sound
	S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_r_exp3, 1, 1);
}

static void CLH2_ParseBeam(QMsg& message)
{
	message.ReadShort();
	message.ReadCoord();
	message.ReadCoord();
	message.ReadCoord();
	message.ReadCoord();
	message.ReadCoord();
	message.ReadCoord();
}

static void CLHW_ParseTarExplosion(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	CLH2_BlobExplosion(pos);

	S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_r_exp3, 1, 1);
}

static void CLH2_ParseLavaSplash(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	CLH2_LavaSplash(pos);
}

static void CLH2_ParseTeleport(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	CLH2_TeleportSplash(pos);
}

static void CLHW_ParseGunShot(QMsg& message)
{
	int cnt = message.ReadByte();
	vec3_t pos;
	message.ReadPos(pos);
	CLH2_RunParticleEffect(pos, vec3_origin, 20 * cnt);
}

static void CLHW_ParseLightningBlood(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	CLH2_RunParticleEffect(pos, vec3_origin, 50);
}

static void CLHW_ParseBoneRic(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	int cnt = message.ReadByte();

	CLH2_RunParticleEffect4(pos, 3, 368 + rand() % 16, pt_h2grav, cnt);
	int rnd = rand() % 100;
	if (rnd > 95)
	{
		S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_ric1, 1, 1);
	}
	else if (rnd > 91)
	{
		S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_ric2, 1, 1);
	}
	else if (rnd > 87)
	{
		S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_ric3, 1, 1);
	}
}

static void CLHW_ParseIceStorm(QMsg& message)
{
	static int playIceSound = 600;

	int ent = message.ReadShort();

	h2entity_state_t* state = CLH2_FindState(ent);
	if (!state)
	{
		return;
	}
	vec3_t center;
	VectorCopy(state->origin, center);

	playIceSound += cls_common->frametime;
	if (playIceSound >= 600)
	{
		S_StartSound(center, CLH2_TempSoundChannel(), 0, clh2_sfx_icestorm, 1, 1);
		playIceSound -= 600;
	}

	for (int i = 0; i < 5; i++)
	{
		// make some ice beams...
		vec3_t source;
		VectorCopy(center, source);
		source[0] += rand() % 100 - 50;
		source[1] += rand() % 100 - 50;

		vec3_t dest;
		VectorCopy(source, dest);
		dest[2] += 128;

		CLH2_CreateStreamIceChunks(ent, i, i + H2STREAM_ATTACHED, 0, 300, source, dest);
	}
}

static void CLHW_ParseSunstuffCheap(QMsg& message)
{
	int ent = message.ReadShort();
	int reflect_count = message.ReadByte();

	// read in up to 4 points for up to 3 beams
	vec3_t points[4];
	for (int i = 0; i < 2 + reflect_count; i++)
	{
		message.ReadPos(points[i]);
	}

	h2entity_state_t* state = CLH2_FindState(ent);
	if (!state)
	{
		return;
	}
	// actually create the sun model pieces
	for (int i = 0; i < reflect_count + 1; i++)
	{
		CLH2_CreateStreamSunstaff1(ent, i, !i ? i + H2STREAM_ATTACHED : i, 0, 500, points[i], points[i + 1]);
	}
}

static void CLHW_ParseLightningHammer(QMsg& message)
{
	int ent = message.ReadShort();

	h2entity_state_t* state = CLH2_FindState(ent);

	if (state)
	{
		if (rand() & 1)
		{
			S_StartSound(state->origin, CLH2_TempSoundChannel(), 0, clh2_sfx_lightning1, 1, 1);
		}
		else
		{
			S_StartSound(state->origin, CLH2_TempSoundChannel(), 0, clh2_sfx_lightning2, 1, 1);
		}

		for (int i = 0; i < 5; i++)
		{
			// make some lightning
			vec3_t source;
			VectorCopy(state->origin, source);
			source[0] += rand() % 30 - 15;
			source[1] += rand() % 30 - 15;

			vec3_t dest;
			VectorCopy(source, dest);
			dest[0] += rand() % 80 - 40;
			dest[1] += rand() % 80 - 40;
			dest[2] += 64 + (rand() % 48);

			CLH2_CreateStreamLightning(ent, i, i, 0, 500, source, dest);
		}
	}
}

static void CLHW_ParseSwordExplosion(QMsg& message)
{
	vec3_t pos;
	message.ReadPos(pos);
	int ent = message.ReadShort();

	h2entity_state_t* state = CLH2_FindState(ent);
	if (state)
	{
		if(rand() & 1)
		{
			S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_lightning1, 1, 1);
		}
		else
		{
			S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_lightning2, 1, 1);
		}

		for (int i = 0; i < 5; i++)
		{
			// make some lightning
			vec3_t source;
			VectorCopy(pos, source);
			source[0] += rand() % 30 - 15;
			source[1] += rand() % 30 - 15;

			vec3_t dest;
			VectorCopy(source, dest);
			dest[0] += rand() % 80 - 40;
			dest[1] += rand() % 80 - 40;
			dest[2] += 64 + (rand() % 48);

			CLH2_CreateStreamLightning(ent, i, i, 0, 500, source, dest);
		}
	}
	CLHW_SwordExplosion(pos);
}

static void CLHW_ParseSunstaffPower(QMsg& message)
{
	int ent = message.ReadShort();
	vec3_t source;
	message.ReadPos(source);
	vec3_t dest;
	message.ReadPos(dest);

	CLHW_SunStaffExplosions(dest);

	h2entity_state_t* state = CLH2_FindState(ent);
	if (!state)
	{
		return;
	}
	S_StartSound(state->origin, CLH2_TempSoundChannel(), 0, clh2_sfx_sunstaff, 1, 1);

	CLH2_CreateStreamSunstaffPower(ent, source, dest);
}

static void CLHW_ParseCubeBeam(QMsg& message)
{
	int ent = message.ReadShort();
	int ent2 = message.ReadShort();

	h2entity_state_t* state = CLH2_FindState(ent);
	h2entity_state_t* state2 = CLH2_FindState(ent2);

	if (state || state2)
	{
		vec3_t source;
		if (state)
		{
			VectorCopy(state->origin, source);
		}
		else//don't know where the damn cube is--prolly won't see beam anyway then, so put it all at the target
		{
			VectorCopy(state2->origin, source);
			source[2] += 10;
		}

		vec3_t dest;
		if (state2)
		{
			VectorCopy(state2->origin, dest);//in case they're both valid, copy me again
			dest[2] += 30;
		}
		else//don't know where the damn victim is--prolly won't see beam anyway then, so put it all at the cube
		{
			VectorCopy(source, dest);
			dest[2] += 10;
		}

		S_StartSound(source, CLH2_TempSoundChannel(), 0, clh2_sfx_sunstaff, 1, 1);
		S_StartSound(dest, CLH2_TempSoundChannel(), 0, clh2_sfx_sunhit, 1, 1);
		CLH2_SunStaffTrail(dest, source);
		int skin = rand() % 5;
		CLH2_CreateStreamColourBeam(ent, 0, 0, skin, 100, source, dest);
	}
}

static void CLHW_ParseLightningExplode(QMsg& message)
{
	int ent = message.ReadShort();
	vec3_t pos;
	message.ReadPos(pos);

	if (rand() & 1)
	{
		S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_lightning1, 1, 1);
	}
	else
	{
		S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_lightning2, 1, 1);
	}

	for (int i = 0; i < 10; i++)
	{
		// make some lightning
		float tempAng = (rand() % 628) / 100.0;
		float tempPitch = (rand() % 628) / 100.0;

		vec3_t dest;
		VectorCopy(pos, dest);
		dest[0] += 75.0 * cos(tempAng) * cos(tempPitch);
		dest[1] += 75.0 * sin(tempAng) * cos(tempPitch);
		dest[2] += 75.0 * sin(tempPitch);

		CLH2_CreateStreamLightning(ent, i, i, 0, 500, pos, dest);
	}
}

static void CLHW_ParseChainLightning(QMsg& message)
{
	int ent = message.ReadShort();

	int numTargs = 0;
	int oldNum;
	vec3_t points[12];
	do
	{
		message.ReadPos(points[numTargs]);

		oldNum = numTargs;

		if (points[numTargs][0] || points[numTargs][1] || points[numTargs][2])
		{
			if (numTargs < 9)
			{
				numTargs++;
			}
		}
	}
	while (points[oldNum][0] || points[oldNum][1] || points[oldNum][2]);

	if (numTargs == 0)
	{
		return;
	}

	for (int temp = 0; temp < numTargs - 1; temp++)
	{
		// make the connecting lightning...
		CLH2_CreateStreamLightning(ent, temp, temp, 0, 300, points[temp], points[temp + 1]);
		CLHW_ChainLightningExplosion(points[temp + 1]);
	}
}

void CLH2_ParseTEnt(QMsg& message)
{
	int type = message.ReadByte();
	switch (type)
	{
	case H2TE_WIZSPIKE:	// spike hitting wall
		CLH2_ParseWizSpike(message);
		break;
	case H2TE_KNIGHTSPIKE:	// spike hitting wall
	case H2TE_GUNSHOT:		// bullet hitting wall
		CLH2_ParseKnightSpike(message);
		break;
	case H2TE_SPIKE:			// spike hitting wall
		CLH2_ParseSpike(message);
		break;
	case H2TE_SUPERSPIKE:			// super spike hitting wall
		CLH2_ParseSuperSpike(message);
		break;
	case H2TE_EXPLOSION:			// rocket explosion
		CLH2_ParseExplosion(message);
		break;
	case H2TE_LIGHTNING1:
	case H2TE_LIGHTNING2:
	case H2TE_LIGHTNING3:
		CLH2_ParseBeam(message);
		break;
	case H2TE_LAVASPLASH:
		CLH2_ParseLavaSplash(message);
		break;
	case H2TE_TELEPORT:
		CLH2_ParseTeleport(message);
		break;
	case H2TE_STREAM_CHAIN:
	case H2TE_STREAM_SUNSTAFF1:
	case H2TE_STREAM_SUNSTAFF2:
	case H2TE_STREAM_LIGHTNING:
	case H2TE_STREAM_LIGHTNING_SMALL:
	case H2TE_STREAM_COLORBEAM:
	case H2TE_STREAM_ICECHUNKS:
	case H2TE_STREAM_GAZE:
	case H2TE_STREAM_FAMINE:
		CLH2_ParseStream(message, type);
		break;
	default:
		throw Exception("CL_ParseTEnt: bad type");
	}
}

void CLHW_ParseTEnt(QMsg& message)
{
	int type = message.ReadByte();
	switch (type)
	{
	case H2TE_WIZSPIKE:	// spike hitting wall
		CLH2_ParseWizSpike(message);
		break;
	case H2TE_KNIGHTSPIKE:	// spike hitting wall
		CLH2_ParseKnightSpike(message);
		break;
	case H2TE_SPIKE:			// spike hitting wall
		CLH2_ParseSpike(message);
		break;
	case H2TE_SUPERSPIKE:			// super spike hitting wall
		CLH2_ParseSuperSpike(message);
		break;
	case HWTE_DRILLA_EXPLODE:
		CLHW_ParseDrillaExplode(message);
		break;
	case H2TE_EXPLOSION:			// rocket explosion
		CLHW_ParseExplosion(message);
		break;
	case HWTE_TAREXPLOSION:			// tarbaby explosion
		CLHW_ParseTarExplosion(message);
		break;
	case H2TE_LIGHTNING1:				// lightning bolts
	case H2TE_LIGHTNING2:
	case H2TE_LIGHTNING3:
		CLH2_ParseBeam(message);
		break;
	case H2TE_LAVASPLASH:	
		CLH2_ParseLavaSplash(message);
		break;
	case H2TE_TELEPORT:
		CLH2_ParseTeleport(message);
		break;
	case H2TE_GUNSHOT:			// bullet hitting wall
	case HWTE_BLOOD:				// bullets hitting body
		CLHW_ParseGunShot(message);
		break;
	case HWTE_LIGHTNINGBLOOD:		// lightning hitting body
		CLHW_ParseLightningBlood(message);
		break;
	case H2TE_STREAM_CHAIN:
	case H2TE_STREAM_SUNSTAFF1:
	case H2TE_STREAM_SUNSTAFF2:
	case H2TE_STREAM_LIGHTNING:
	case HWTE_STREAM_LIGHTNING_SMALL:
	case H2TE_STREAM_COLORBEAM:
	case H2TE_STREAM_ICECHUNKS:
	case H2TE_STREAM_GAZE:
	case H2TE_STREAM_FAMINE:
		CLH2_ParseStream(message, type);
		break;
	case HWTE_BIGGRENADE:
		CLHW_ParseBigGrenade(message);
		break;
	case HWTE_CHUNK:
		CLHW_ParseChunk(message);
		break;
	case HWTE_CHUNK2:
		CLHW_ParseChunk2(message);
		break;
	case HWTE_XBOWHIT:
		CLHW_ParseXBowHit(message);
		break;
	case HWTE_METEORHIT:
		CLHW_ParseMeteorHit(message);
		break;
	case HWTE_HWBONEPOWER:
		CLHW_ParseBonePower(message);
		break;
	case HWTE_HWBONEPOWER2:
		CLHW_ParseBonePower2(message);
		break;
	case HWTE_HWRAVENDIE:
		CLHW_ParseRavenDie(message);
		break;
	case HWTE_HWRAVENEXPLODE:
		CLHW_ParseRavenExplode(message);
		break;
	case HWTE_ICEHIT:
		CLHW_ParseIceHit(message);
		break;
	case HWTE_ICESTORM:
		CLHW_ParseIceStorm(message);
		break;
	case HWTE_HWMISSILEFLASH:
		CLHW_ParseMissileFlash(message);
		break;
	case HWTE_SUNSTAFF_CHEAP:
		CLHW_ParseSunstuffCheap(message);
		break;
	case HWTE_LIGHTNING_HAMMER:
		CLHW_ParseLightningHammer(message);
		break;
	case HWTE_HWTELEPORT:
		CLHW_ParseTeleport(message);
		break;
	case HWTE_SWORD_EXPLOSION:
		CLHW_ParseSwordExplosion(message);
		break;
	case HWTE_AXE_BOUNCE:
		CLHW_ParseAxeBounce(message);
		break;
	case HWTE_AXE_EXPLODE:
		CLHW_ParseAxeExplode(message);
		break;
	case HWTE_TIME_BOMB:
		CLHW_ParseTimeBomb(message);
		break;
	case HWTE_FIREBALL:
		CLHW_ParseFireBall(message);
		break;
	case HWTE_SUNSTAFF_POWER:
		CLHW_ParseSunstaffPower(message);
		break;
	case HWTE_PURIFY2_EXPLODE:
		CLHW_ParsePurify2Explode(message);
		break;
	case HWTE_PLAYER_DEATH:
		CLHW_ParsePlayerDeath(message);
		break;
	case HWTE_PURIFY1_EFFECT:
		CLHW_ParsePurify1Effect(message);
		break;
	case HWTE_TELEPORT_LINGER:
		CLHW_ParseTeleportLinger(message);
		break;
	case HWTE_LINE_EXPLOSION:
		CLHW_ParseLineExplosion(message);
		break;
	case HWTE_METEOR_CRUSH:
		CLHW_ParseMeteorCrush(message);
		break;
	case HWTE_ACIDBALL:
		CLHW_ParseAcidBall(message);
		break;
	case HWTE_ACIDBLOB:
		CLHW_ParseAcidBlob(message);
		break;
	case HWTE_FIREWALL:
		CLHW_ParseFireWall(message);
		break;
	case HWTE_FIREWALL_IMPACT:
		CLHW_ParseFireWallImpact(message);
		break;
	case HWTE_HWBONERIC:
		CLHW_ParseBoneRic(message);
		break;
	case HWTE_POWERFLAME:
		CLHW_ParsePowerFlame(message);
		break;
	case HWTE_BLOODRAIN:
		CLHW_ParseBloodRain(message);
		break;
	case HWTE_AXE:
		CLHW_ParseAxe(message);
		break;
	case HWTE_PURIFY2_MISSILE:
		CLHW_ParsePurify2Missile(message);
		break;
	case HWTE_SWORD_SHOT:
		CLHW_ParseSwordShot(message);
		break;
	case HWTE_ICESHOT:
		CLHW_ParseIceShot(message);
		break;
	case HWTE_METEOR:
		CLHW_ParseMeteor(message);
		break;
	case HWTE_LIGHTNINGBALL:
		CLHW_ParseLightningBall(message);
		break;
	case HWTE_MEGAMETEOR:
		CLHW_ParseMegaMeteor(message);
		break;
	case HWTE_CUBEBEAM:
		CLHW_ParseCubeBeam(message);
		break;
	case HWTE_LIGHTNINGEXPLODE:
		CLHW_ParseLightningExplode(message);
		break;
	case HWTE_ACID_BALL_FLY:
		CLHW_ParseAcidBallFly(message);
		break;
	case HWTE_ACID_BLOB_FLY:
		CLHW_ParseAcidBlobFly(message);
		break;
	case HWTE_CHAINLIGHTNING:
		CLHW_ParseChainLightning(message);
		break;
	default:
		throw Exception("CL_ParseTEnt: bad type");
	}
}

void CLHW_UpdateHammer(refEntity_t* ent, int edict_num)
{
	// do this every .3 seconds
	int testVal = cl.serverTime / 100;
	int testVal2 = (cl.serverTime - cls_common->frametime) / 100;
	if (testVal != testVal2)
	{
		if (!(testVal % 3))
		{
			S_StartSound(ent->origin, edict_num, 2, clh2_sfx_hammersound, 1, 1);
		}
	}
}

void CLHW_UpdateBug(refEntity_t* ent)
{
	int testVal = cl.serverTime / 100;
	int testVal2 = (cl.serverTime - cls_common->frametime) / 100;
	if (testVal != testVal2)
	{
		S_StartSound(ent->origin, CLH2_TempSoundChannel(), 1, clh2_sfx_buzzbee, 1, 1);
	}
}

void CLH2_UpdateTEnts()
{
	CLH2_UpdateExplosions();
	CLH2_UpdateStreams();
	CLHW_UpdateTargetBall();
}
