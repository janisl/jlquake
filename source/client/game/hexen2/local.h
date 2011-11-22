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

#define MAX_STREAMS_H2				32
#define H2STREAM_ATTACHED			16

struct h2stream_t
{
	int type;
	int entity;
	int tag;
	int flags;
	int skin;
	qhandle_t models[4];
	vec3_t source;
	vec3_t dest;
	vec3_t offset;
	float endTime;
	float lastTrailTime;
};

#define MAX_EFFECT_ENTITIES_H2		256

struct effect_entity_t
{
	h2entity_state_t state;
	qhandle_t model;			// 0 = no model
};

extern h2stream_t clh2_Streams[MAX_STREAMS_H2];

extern h2entity_state_t clh2_baselines[MAX_EDICTS_H2];
extern qhandle_t clh2_player_models[MAX_PLAYER_CLASS];

extern effect_entity_t EffectEntities[MAX_EFFECT_ENTITIES_H2];

extern sfxHandle_t clh2_fxsfx_explode;
extern sfxHandle_t clh2_fxsfx_scarabgrab;
extern sfxHandle_t clh2_fxsfx_scarabhome;
extern sfxHandle_t clh2_fxsfx_scarabbyebye;
extern sfxHandle_t clh2_fxsfx_ravengo;
extern sfxHandle_t clh2_fxsfx_drillameat;

extern sfxHandle_t clh2_fxsfx_arr2flsh;
extern sfxHandle_t clh2_fxsfx_arr2wood;
extern sfxHandle_t clh2_fxsfx_met2stn;

void CLH2_InitColourShadeTables();
void CLH2_ClearEntityTextureArrays();
int CLH2_GetMaxPlayerClasses();
void CLH2_SetRefEntAxis(refEntity_t* entity, vec3_t entityAngles, vec3_t angleAdd, int scale, int colourShade, int absoluteLight, int drawFlags);
void CLH2_TranslatePlayerSkin (int playernum);
void CLH2_HandleCustomSkin(refEntity_t* entity, int playerIndex);

void CLH2_ClearStreams();

int CLH2_TempSoundChannel();

void CLHW_InitEffects();
void CLH2_ClearEffects();
int CLH2_NewEffectEntity();
void CLH2_FreeEffect(int index);
void CLH2_ParseEffect(QMsg& message);
void CLHW_ParseMultiEffect(QMsg& message);
void CLH2_LinkEffectEntity(effect_entity_t* entity);
void CLH2_UpdateEffectRain(int index, float frametime);
void CLH2_UpdateEffectSnow(int index, float frametime);
void CLH2_UpdateEffectFountain(int index);
void CLH2_UpdateEffectQuake(int index);
void CLH2_UpdateEffectSmoke(int index, float frametime);
void CLHW_UpdateEffectTeleportSmoke1(int index, float frametime);
void CLHW_UpdateEffectRipple(int index, float frametime);
void CLH2_UpdateEffectExplosion(int index, float frametime);
void CLH2_UpdateEffectBigCircleExplosion(int index, float frametime);
void CLH2_UpdateEffectLShock(int index);
void CLH2_UpdateEffectFlash(int index, float frametime);
void CLH2_UpdateEffectGravityWell(int index, float frametime);
void CLH2_UpdateEffectTeleporterPuffs(int index, float frametime);
void CLH2_UpdateEffectTeleporterBody(int index, float frametime);
void CLH2_UpdateEffectMissile(int index, float frametime);
void CLHW_UpdateEffectBoneBall(int index, float frametime);
void CLHW_UpdateEffectRavenPower(int index, float frametime);
void CLHW_UpdateEffectDrilla(int index, float frametime);
void CLHW_UpdateEffectXBowShot(int index, float frametime);
void CLHW_UpdateEffectSheepinator(int index, float frametime);
void CLHW_UpdateEffectEidolonStar(int index, float frametime);
void CLHW_UpdateEffectMissileStar(int index, float frametime);
void CLH2_UpdateEffectChunk(int index, float frametime);
void CLH2_UpdateEffectRiderDeath(int index, float frametime);
void CLH2_UpdateEffect(int index, float frametime);

void CLH2_InitChunkModel(int chType, int* model, int* skinNum, int* drawFlags, int* frame, int* absoluteLight);
void CLH2_InitChunkVelocity(vec3_t srcvel, vec3_t velocity);
void CLH2_InitChunkAngles(vec3_t angles);
void CLH2_InitChunkAngleVelocity(vec3_t avel);
void CLH2_InitChunkEffect(h2EffectT& effect);

void CLHW_InitExplosionSounds();
void CLH2_ClearExplosions();
void CLHW_SpawnDeathBubble(const vec3_t pos);
void CLHW_ParseMissileFlash(QMsg& message);
void CLHW_ParseDrillaExplode(QMsg& message);
void CLHW_ParseBigGrenade(QMsg& message);
void CLHW_ParseXBowHit(QMsg& message);
void CLHW_ParseBonePower(QMsg& message);
void CLHW_ParseBonePower2(QMsg& message);
void CLHW_CreateRavenDeath(const vec3_t pos);
void CLHW_ParseRavenDie(QMsg& message);
void CLHW_CreateRavenExplosions(const vec3_t pos);
void CLHW_ParseRavenExplode(QMsg& message);
void CLHW_ParseChunk(QMsg& message);
void CLHW_ParseChunk2(QMsg& message);
void CLHW_ParseMeteorHit(QMsg& message);
void CLHW_ParseIceHit(QMsg& message);
void CLHW_ParsePlayerDeath(QMsg& message);
void CLHW_ParseAcidBlob(QMsg& message);
void CLHW_XbowImpact(const vec3_t pos, const vec3_t vel, int chType, int damage, int arrowType);//so xbow effect can use tents
void CLHW_CreateIceChunk(const vec3_t origin);
void CLHW_ParseTeleport(QMsg& message);
void CLHW_SwordExplosion(const vec3_t pos);
void CLHW_ParseAxeBounce(QMsg& message);
void CLHW_ParseAxeExplode(QMsg& message);
void CLHW_ParseTimeBomb(QMsg& message);
void CLHW_ParseFireBall(QMsg& message);
void CLHW_SunStaffExplosions(const vec3_t pos);
void CLHW_ParsePurify2Explode(QMsg& message);
void CLHW_ParsePurify1Effect(QMsg& message);
void CLHW_ParseTeleportLinger(QMsg& message);
void CLHW_ParseLineExplosion(QMsg& message);
void CLHW_ParseMeteorCrush(QMsg& message);
void CLHW_ParseAcidBall(QMsg& message);
void CLHW_ParseFireWall(QMsg& message);
void CLHW_ParseFireWallImpact(QMsg& message);
void CLHW_ParsePowerFlame(QMsg& message);
void CLHW_ParseBloodRain(QMsg& message);
void CLHW_ParseAxe(QMsg& message);
void CLHW_ParsePurify2Missile(QMsg& message);
void CLHW_ParseSwordShot(QMsg& message);
void CLHW_ParseIceShot(QMsg& message);
void CLHW_ParseMeteor(QMsg& message);
void CLHW_ParseMegaMeteor(QMsg& message);
void CLHW_ParseLightningBall(QMsg& message);
void CLHW_ParseAcidBallFly(QMsg& message);
void CLHW_ParseAcidBlobFly(QMsg& message);
void CLHW_ChainLightningExplosion(const vec3_t pos);
void CLHW_CreateExplosionWithSound(const vec3_t pos);
void CLHW_UpdatePoisonGas(const vec3_t pos, const vec3_t angles);
void CLHW_UpdateAcidBlob(const vec3_t pos, const vec3_t angles);
void CLHW_UpdateOnFire(refEntity_t *ent, vec3_t angles, int edict_num);
void CLHW_UpdatePowerFlameBurn(refEntity_t *ent, int edict_num);
void CLHW_UpdateTargetBall(float targetDistance, float targetAngle, float targetPitch, const vec3_t viewOrigin);
void CLH2_UpdateExplosions();
