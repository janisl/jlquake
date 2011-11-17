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
extern image_t* clh2_playertextures[H2BIGGEST_MAX_CLIENTS];

extern effect_entity_t EffectEntities[MAX_EFFECT_ENTITIES_H2];

extern sfxHandle_t clh2_fxsfx_explode;
extern sfxHandle_t clh2_fxsfx_scarabgrab;
extern sfxHandle_t clh2_fxsfx_scarabhome;
extern sfxHandle_t clh2_fxsfx_scarabbyebye;
extern sfxHandle_t clh2_fxsfx_ravengo;
extern sfxHandle_t clh2_fxsfx_drillaspin;
extern sfxHandle_t clh2_fxsfx_drillameat;

extern sfxHandle_t clh2_fxsfx_arr2flsh;
extern sfxHandle_t clh2_fxsfx_arr2wood;
extern sfxHandle_t clh2_fxsfx_met2stn;

void CLH2_InitColourShadeTables();
int CLH2_GetMaxPlayerClasses();
void CLH2_SetRefEntAxis(refEntity_t* ent, vec3_t ent_angles, vec3_t angleAdd, int scale, int colorshade, int abslight, int drawflags);
void CLH2_TranslatePlayerSkin (int playernum);

void CLH2_ClearStreams();

int CLH2_TempSoundChannel();

void CLHW_InitEffects();
void CLH2_ClearEffects();
int CLH2_NewEffectEntity();
void CLH2_FreeEffect(int index);
void CLH2_ParseEffect(QMsg& message);

void CLH2_InitChunkModel(int chType, int* model, int* skinNum, int* drawFlags, int* frame, int* absoluteLight);
void CLH2_InitChunkVelocity(vec3_t srcvel, vec3_t velocity);
void CLH2_InitChunkAngles(vec3_t angles);
void CLH2_InitChunkAngleVelocity(vec3_t avel);
void CLH2_InitChunkEffect(h2EffectT& effect);
