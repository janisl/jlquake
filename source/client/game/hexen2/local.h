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

extern effect_entity_t EffectEntities[MAX_EFFECT_ENTITIES_H2];

extern sfxHandle_t clh2_fxsfx_xbowshoot;
extern sfxHandle_t clh2_fxsfx_xbowfshoot;
extern sfxHandle_t clh2_fxsfx_explode;
extern sfxHandle_t clh2_fxsfx_mmfire;
extern sfxHandle_t clh2_fxsfx_eidolon;
extern sfxHandle_t clh2_fxsfx_scarabwhoosh;
extern sfxHandle_t clh2_fxsfx_scarabgrab;
extern sfxHandle_t clh2_fxsfx_scarabhome;
extern sfxHandle_t clh2_fxsfx_scarabrip;
extern sfxHandle_t clh2_fxsfx_scarabbyebye;
extern sfxHandle_t clh2_fxsfx_ravengo;
extern sfxHandle_t clh2_fxsfx_drillashoot;
extern sfxHandle_t clh2_fxsfx_drillaspin;
extern sfxHandle_t clh2_fxsfx_drillameat;

extern sfxHandle_t clh2_fxsfx_arr2flsh;
extern sfxHandle_t clh2_fxsfx_arr2wood;
extern sfxHandle_t clh2_fxsfx_met2stn;

void CLH2_ClearStreams();

int CLH2_TempSoundChannel();

void CLHW_InitEffects();
void CLH2_ClearEffects();
int CLH2_NewEffectEntity();
void CLH2_FreeEffect(int index);
void CLH2_ParseEffectRain(int index, QMsg& message);
void CLH2_ParseEffectSnow(int index, QMsg& message);
void CLH2_ParseEffectFountain(int index, QMsg& message);
void CLH2_ParseEffectQuake(int index, QMsg& message);
bool CLH2_ParseEffectWhiteSmoke(int index, QMsg& message);
bool CLH2_ParseEffectGreenSmoke(int index, QMsg& message);
bool CLH2_ParseEffectGraySmoke(int index, QMsg& message);
bool CLH2_ParseEffectRedSmoke(int index, QMsg& message);
bool CLH2_ParseEffectTeleportSmoke1(int index, QMsg& message);
bool CLH2_ParseEffectTeleportSmoke2(int index, QMsg& message);
bool CLH2_ParseEffectGhost(int index, QMsg& message);
bool CLH2_ParseEffectRedCloud(int index, QMsg& message);
bool CLH2_ParseEffectAcidMuzzleFlash(int index, QMsg& message);
bool CLH2_ParseEffectFlameStream(int index, QMsg& message);
bool CLH2_ParseEffectFlameWall(int index, QMsg& message);
bool CLH2_ParseEffectFlameWall2(int index, QMsg& message);
bool CLH2_ParseEffectOnFire(int index, QMsg& message);
bool CLHW_ParseEffectRipple(int index, QMsg& message);
bool CLH2_ParseEffectSmallWhiteFlash(int index, QMsg& message);
bool CLH2_ParseEffectYellowRedFlash(int index, QMsg& message);
bool CLH2_ParseEffectBlueSpark(int index, QMsg& message);
bool CLH2_ParseEffectYellowSpark(int index, QMsg& message);
bool CLH2_ParseEffectSmallCircleExplosion(int index, QMsg& message);
bool CLH2_ParseEffectBigCircleExplosion(int index, QMsg& message);
bool CLH2_ParseEffectSmallExplosion(int index, QMsg& message);
bool CLHW_ParseEffectSmallExplosionWithSound(int index, QMsg& message);
bool CLH2_ParseEffectBigExplosion(int index, QMsg& message);
bool CLHW_ParseEffectBigExplosionWithSound(int index, QMsg& message);
bool CLH2_ParseEffectFloorExplosion(int index, QMsg& message);
bool CLH2_ParseEffectFloorExplosion2(int index, QMsg& message);
bool CLH2_ParseEffectFloorExplosion3(int index, QMsg& message);
bool CLH2_ParseEffectBlueExplosion(int index, QMsg& message);
bool CLH2_ParseEffectRedSpark(int index, QMsg& message);
bool CLH2_ParseEffectGreenSpark(int index, QMsg& message);
bool CLH2_ParseEffectIceHit(int index, QMsg& message);
bool CLH2_ParseEffectMedusaHit(int index, QMsg& message);
bool CLH2_ParseEffectMezzoReflect(int index, QMsg& message);
bool CLH2_ParseEffectXBowExplosion(int index, QMsg& message);
bool CLH2_ParseEffectNewExplosion(int index, QMsg& message);
bool CLH2_ParseEffectMagicMissileExplosion(int index, QMsg& message);
bool CLHW_ParseEffectMagicMissileExplosionWithSound(int index, QMsg& message);
bool CLH2_ParseEffectBoneExplosion(int index, QMsg& message);
bool CLH2_ParseEffectBldrnExplosion(int index, QMsg& message);
bool CLH2_ParseEffectLShock(int index, QMsg& message);
bool CLH2_ParseEffectAcidHit(int index, QMsg& message);
bool CLH2_ParseEffectAcidSplat(int index, QMsg& message);
bool CLH2_ParseEffectAcidExplosion(int index, QMsg& message);
bool CLH2_ParseEffectLBallExplosion(int index, QMsg& message);
bool CLH2_ParseEffectFBoom(int index, QMsg& message);
bool CLH2_ParseEffectBomb(int index, QMsg& message);
bool CLH2_ParseEffectFirewallSall(int index, QMsg& message);
bool CLH2_ParseEffectFirewallMedium(int index, QMsg& message);
bool CLH2_ParseEffectFirewallLarge(int index, QMsg& message);
bool CLH2_ParseEffectWhiteFlash(int index, QMsg& message);
bool CLH2_ParseEffectBlueFlash(int index, QMsg& message);
bool CLH2_ParseEffectSmallBlueFlash(int index, QMsg& message);
bool CLHW_ParseEffectSplitFlash(int index, QMsg& message);
bool CLH2_ParseEffectRedFlash(int index, QMsg& message);
void CLH2_ParseEffectRiderDeath(int index, QMsg& message);
void CLH2_ParseEffectGravityWell(int index, QMsg& message);
void CLH2_ParseEffectTeleporterPuffs(int index, QMsg& message);
void CLH2_ParseEffectTeleporterBody(int index, QMsg& message);
bool CLH2_ParseEffectBoneShard(int index, QMsg& message);
bool CLHW_ParseEffectBoneShard(int index, QMsg& message);
bool CLH2_ParseEffectBoneShrapnel(int index, QMsg& message);
bool CLHW_ParseEffectBoneBall(int index, QMsg& message);
bool CLHW_ParseEffectRavenStaff(int index, QMsg& message);
bool CLHW_ParseEffectRavenPower(int index, QMsg& message);
