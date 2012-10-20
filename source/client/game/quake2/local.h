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

extern Cvar* q2_hand;
extern Cvar* clq2_footsteps;
extern Cvar* clq2_name;
extern Cvar* clq2_skin;
extern Cvar* clq2_vwep;
extern Cvar* clq2_predict;
extern Cvar* clq2_noskins;
extern Cvar* clq2_showmiss;
extern Cvar* clq2_gender;
extern Cvar* clq2_gender_auto;

extern sfxHandle_t clq2_sfx_footsteps[4];

extern qhandle_t clq2_mod_powerscreen;

extern q2centity_t clq2_entities[MAX_EDICTS_Q2];

extern char clq2_weaponmodels[MAX_CLIENTWEAPONMODELS_Q2][MAX_QPATH];
extern int clq2_num_weaponmodels;

extern vec3_t monster_flash_offset[];

extern const char* svcq2_strings[256];

//
//	Beam
//
void CLQ2_ClearBeams();
void CLQ2_RegisterBeamModels();
void CLQ2_ParasiteBeam(int ent, vec3_t start, vec3_t end);
void CLQ2_GrappleCableBeam(int ent, vec3_t start, vec3_t end, vec3_t offset);
void CLQ2_HeatBeam(int ent, vec3_t start, vec3_t end);
void CLQ2_MonsterHeatBeam(int ent, vec3_t start, vec3_t end);
void CLQ2_LightningBeam(int srcEnt, int destEnt, vec3_t start, vec3_t end);
void CLQ2_AddBeams();
void CLQ2_AddPlayerBeams();

//
//	Connection
//
extern int clq2_precache_check;	// for autodownload of precache items
extern int clq2_precache_model_skin;
extern byte* clq2_precache_model;	// used for skin checking in alias models
extern int clq2_precache_spawncount;

void CLQ2_RegisterSounds();
void CLQ2_RequestNextDownload();
void CLQ2_SendCmd();
void CLQ2_Disconnect();
void CLQ2_CheckForResend();
void CLQ2_Connect_f();
void CLQ2_Reconnect_f();
void CLQ2_ReadPackets();

//
//	Demo
//
void CLQ2_WriteDemoMessage(QMsg* msg, int headerBytes);
void CLQ2_Stop_f();
void CLQ2_Record_f();

//
//	Effects
//
void CLQ2_LogoutEffect(vec3_t org, int type);
void CLQ2_ParseMuzzleFlash(QMsg& message);
void CLQ2_ParseMuzzleFlash2(QMsg& message);
void CLQ2_FlyEffect(q2centity_t* ent, vec3_t origin);
void CLQ2_EntityEvent(q2entity_state_t* ent);

//
//	Entities
//
// the cl_parse_entities must be large enough to hold UPDATE_BACKUP_Q2 frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original
#define MAX_PARSE_ENTITIES_Q2  1024
extern q2entity_state_t clq2_parse_entities[MAX_PARSE_ENTITIES_Q2];

void CLQ2_ParseBaseline(QMsg& message);
void CLQ2_ParseFrame(QMsg& message);
void CLQ2_AddPacketEntities(q2frame_t* frame);
// the sound code makes callbacks to the client for entitiy position
// information, so entities can be dynamically re-spatialized
void CLQ2_GetEntitySoundOrigin(int ent, vec3_t org);

//
//	Explosion
//
void CLQ2_ClearExplosions();
void CLQ2_RegisterExplosionModels();
void CLQ2_SmokeAndFlash(vec3_t origin);
void CLQ2_BlasterExplosion(vec3_t origin, vec3_t direction);
void CLQ2_GrenadeExplosion(vec3_t origin);
void CLQ2_RocketExplosion(vec3_t origin);
void CLQ2_BigExplosion(vec3_t origin);
void CLQ2_BfgExplosion(vec3_t origin);
void CLQ2_WeldingSparks(vec3_t origin);
void CLQ2_Blaster2Explosion(vec3_t origin, vec3_t direction);
void CLQ2_FlechetteExplosion(vec3_t origin, vec3_t direction);
void CLQ2_AddExplosions();

//
//	HUD
//
extern const char* sb_nums[2][11];

void CLQ2_ParseInventory(QMsg& message);
void SCRQ2_DrawHud();

//
//	Laser
//
void CLQ2_ClearLasers();
void CLQ2_NewLaser(vec3_t start, vec3_t end, int colors);
void CLQ2_AddLasers();

//
//	Main
//
void CLQ2_PingServers_f();
void CLQ2_ClearState();
void CLQ2_PrepRefresh();
void CLQ2_FixUpGender();
void CLQ2_Init();

//
//	Parse
//
void CLQ2_LoadClientinfo(q2clientinfo_t* ci, const char* s);
void CLQ2_ParseClientinfo(int player);
void CLQ2_ParseServerMessage(QMsg& message);

//
//	Predict
//
void CLQ2_CheckPredictionError();
void CLQ2_PredictMovement();

//
//	Screen
//
void CLQ2_AddNetgraph();
void SCRQ2_DrawScreen(stereoFrame_t stereoFrame, float separation);
void SCRQ2_Init();

//
//	Sustain
//
void CLQ2_ClearSustains();
void CLQ2_SustainParticleStream(int id, int cnt, vec3_t pos, vec3_t dir, int r, int magnitude, int interval);
void CLQ2_SustainWindow(int id, vec3_t pos);
void CLQ2_SustainNuke(vec3_t pos);
void CLQ2_ProcessSustain();

//
//	Temporary entities
//
void CLQ2_RegisterTEntSounds();
void CLQ2_RegisterTEntModels();
void CLQ2_ClearTEnts();
void CLQ2_ParseTEnt(QMsg& message);
void CLQ2_AddTEnts();

//
//	View
//
void SCR_TouchPics();
void VQ2_RenderView(float stereo_separation);
void VQ2_Init();
