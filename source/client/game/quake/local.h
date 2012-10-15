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

#define MAX_STATIC_ENTITIES_Q1  128			// torches, etc

extern Cvar* cl_doubleeyes;

extern q1entity_state_t clq1_baselines[MAX_EDICTS_QH];
extern q1entity_t clq1_entities[MAX_EDICTS_QH];
extern q1entity_t clq1_static_entities[MAX_STATIC_ENTITIES_Q1];

extern image_t* clq1_playertextures[BIGGEST_MAX_CLIENTS_QH];

extern int clq1_spikeindex;
extern int clq1_playerindex;
extern int clqw_flagindex;

extern Cvar* clqw_baseskin;
extern Cvar* clqw_noskins;

//
//	Connection
//
void CLQ1_SignonReply();
int CLQW_CalcNet();
bool CLQW_IsUploading();
void CLQW_NextUpload();
void CLQW_StartUpload(const byte* data, int size);
void CLQW_StopUpload();

//
//	Entities
//
void CLQ1_ParseSpawnBaseline(QMsg& message);
void CLQ1_ParseSpawnStatic(QMsg& message);
void CLQ1_ParseUpdate(QMsg& message, int bits);
void CLQW_ParsePacketEntities(QMsg& message);
void CLQW_ParseDeltaPacketEntities(QMsg& message);
void CLQW_ParsePlayerinfo(QMsg& message);
void CLQ1_SetRefEntAxis(refEntity_t* ent, vec3_t ent_angles);
void CLQ1_TranslatePlayerSkin(int playernum);
void CLQ1_EmitEntities();
void CLQW_EmitEntities();

//
//	Main
//
void CLQ1_ClearState();

//
//	Parse
//
void CLQ1_ParseServerMessage(QMsg& message);
void CLQW_ParseServerMessage(QMsg& message);

//
//	Projectiles
//
void CLQ1_ClearProjectiles();
void CLQW_ParseNails(QMsg& message);
void CLQ1_LinkProjectiles();

//
//	Screen
//
void SCRQ1_DrawScreen(stereoFrame_t stereoFrame);
void SCRQ1_Init();

//
//	Skin
//
void CLQW_SkinFind(q1player_info_t* sc);
byte* CLQW_SkinCache(qw_skin_t* skin);
void CLQW_SkinNextDownload();
void CLQW_SkinSkins_f();
void CLQW_SkinAllSkins_f();

//
//	Status bar
//
extern int sbqh_lines;					// scan lines to draw
extern Cvar* clqw_hudswap;

void SbarQ1_Init();
void SbarQ1_InitImages();
int SbarQH_itoa(int num, char* buf);
// called every frame by screen
void SbarQ1_Draw();
// called each frame after the level has been completed
void SbarQ1_IntermissionOverlay();
void SbarQ1_FinaleOverlay();

//
//	Temporary entities
//
void CLQ1_InitTEnts();
void CLQ1_ClearTEnts();
void CLQ1_ParseTEnt(QMsg& message);
void CLQW_ParseTEnt(QMsg& message);
void CLQ1_UpdateTEnts();

#include "../quake_hexen2/main.h"
#include "../quake_hexen2/predict.h"
