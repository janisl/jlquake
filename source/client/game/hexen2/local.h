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
extern bool EntityUsed[MAX_EFFECT_ENTITIES_H2];
extern int EffectEntityCount;

void CLH2_ClearStreams();

int CLH2_NewEffectEntity();
void CLH2_FreeEffectEntity(int index);
