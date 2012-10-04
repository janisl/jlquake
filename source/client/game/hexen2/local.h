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

#define H2STREAM_ATTACHED           16

#define MAX_STATIC_ENTITIES_H2      256			// torches, etc

#define MAX_EFFECT_ENTITIES_H2      256

#define BE_ON           (1 << 0)

#define ABILITIES_STR_INDEX         400

struct effect_entity_t
{
	h2entity_state_t state;
	qhandle_t model;			// 0 = no model
};

extern h2entity_state_t clh2_baselines[MAX_EDICTS_QH];
extern h2entity_t h2cl_entities[MAX_EDICTS_QH];
extern qhandle_t clh2_player_models[MAX_PLAYER_CLASS];

extern effect_entity_t EffectEntities[MAX_EFFECT_ENTITIES_H2];

extern sfxHandle_t clh2_sfx_icestorm;

extern int clh2_ravenindex;
extern int clh2_raven2index;
extern int clh2_ballindex;
extern int clh2_missilestarindex;

extern Cvar* clh2_playerclass;
extern Cvar* clhw_teamcolor;

extern int clhw_playerindex[MAX_PLAYER_CLASS];

extern bool h2intro_playing;

extern bool clhw_siege;
extern int clhw_keyholder;
extern int clhw_doc;
extern float clhw_timelimit;
extern float clhw_server_time_offset;
extern byte clhw_fraglimit;
extern unsigned int clhw_defLosses;				// Defenders losses in Siege
extern unsigned int clhw_attLosses;				// Attackers Losses in Siege

extern int sbqh_lines;					// scan lines to draw

extern const char* h2_ClassNames[NUM_CLASSES_H2MP];
extern const char* hw_ClassNames[MAX_PLAYER_CLASS];

void CLH2_InitColourShadeTables();
void CLH2_ClearEntityTextureArrays();
int CLH2_GetMaxPlayerClasses();
h2entity_t* CLH2_EntityNum(int num);
void CLH2_ParseSpawnBaseline(QMsg& message);
void CLH2_ParseSpawnStatic(QMsg& message);
void CLH2_ParseReference(QMsg& message);
void CLH2_ParseUpdate(QMsg& message, int bits);
void CLH2_ParseClearEdicts(QMsg& message);
void CLHW_ParsePacketEntities(QMsg& message);
void CLHW_ParseDeltaPacketEntities(QMsg& message);
void CLHW_ParsePlayerinfo(QMsg& message);
void CLHW_SavePlayer(QMsg& message);
void CLH2_SetRefEntAxis(refEntity_t* entity, vec3_t entityAngles, vec3_t angleAdd, int scale, int colourShade, int absoluteLight, int drawFlags);
void CLH2_TranslatePlayerSkin(int playernum);
void CLH2_HandleCustomSkin(refEntity_t* entity, int playerIndex);
void CLH2_EmitEntities();
void CLHW_EmitEntities();

void CLH2_ClearStreams();
void CLH2_CreateStreamChain(int ent, int tag, int flags, int skin, int duration, const vec3_t source, const vec3_t dest);
void CLH2_CreateStreamSunstaff1(int ent, int tag, int flags, int skin, int duration, const vec3_t source, const vec3_t dest);
void CLH2_CreateStreamSunstaffPower(int ent, const vec3_t source, const vec3_t dest);
void CLH2_CreateStreamLightning(int ent, int tag, int flags, int skin, int duration, const vec3_t source, const vec3_t dest);
void CLH2_CreateStreamColourBeam(int ent, int tag, int flags, int skin, int duration, const vec3_t source, const vec3_t dest);
void CLH2_CreateStreamIceChunks(int ent, int tag, int flags, int skin, int duration, const vec3_t source, const vec3_t dest);
void CLH2_ParseStream(QMsg& message, int type);
void CLH2_UpdateStreams();

void CLH2_InitTEnts();
void CLH2_ClearTEnts();
int CLH2_TempSoundChannel();
h2entity_state_t* CLH2_FindState(int EntNum);
void CLH2_ParseTEnt(QMsg& message);
void CLHW_ParseTEnt(QMsg& message);
void CLHW_UpdateHammer(refEntity_t* ent, int edict_num);
void CLHW_UpdateBug(refEntity_t* ent);
void CLH2_UpdateTEnts();

void CLHW_InitEffects();
void CLH2_ClearEffects();
int CLH2_NewEffectEntity();
void CLH2_ParseEffect(QMsg& message);
void CLHW_ParseMultiEffect(QMsg& message);
void CLHW_ParseReviseEffect(QMsg& message);
void CLHW_ParseTurnEffect(QMsg& message);
void CLH2_ParseEndEffect(QMsg& message);
void CLH2_UpdateEffects();

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
void CLHW_UpdateOnFire(refEntity_t* ent, const vec3_t angles, int edict_num);
void CLHW_UpdatePowerFlameBurn(refEntity_t* ent, int edict_num);
void CLHW_UpdateIceStorm(refEntity_t* ent, int edict_num);
void CLH2_UpdateExplosions();
void CLHW_ClearTarget();
void CLHW_ParseTarget(QMsg& message);
void CLHW_UpdateTargetBall();

void CLH2_ClearProjectiles();
void CLH2_ClearMissiles();
void CLHW_ParseNails(QMsg& message);
void CLHW_ParsePackMissiles(QMsg& message);
void CLH2_LinkProjectiles();
void CLH2_LinkMissiles();

void CLH2_SignonReply();

int SbarQH_itoa(int num, char* buf);
void SbarH2_Init();
void SbarH2_DeathmatchOverlay();
void SbarH2_Draw();
void SbarH2_InvChanged();
void SbarH2_InvReset();

#define MAX_INFO_H2 1024

extern int clh2_total_loading_size, clh2_current_loading_size, clh2_loading_stage;
extern char scrh2_infomessage[MAX_INFO_H2];
extern const char* clh2_plaquemessage;		// Pointer to current plaque

void SCRH2_DrawCenterString(const char* message);
void SCRH2_DrawPause();
void SCRH2_DrawLoading();
void SCRH2_UpdateInfoMessage();
void SCRH2_Plaque_Draw(const char* message, bool AlwaysDraw);
void SCRH2_Info_Plaque_Draw(const char* message);
void SBH2_IntermissionOverlay();
void CLHW_NetGraph();

#include "../quake_hexen2/main.h"
#include "../quake_hexen2/predict.h"
