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

extern sfxHandle_t clq2_sfx_footsteps[4];

extern qhandle_t clq2_mod_powerscreen;

extern q2centity_t clq2_entities[MAX_EDICTS_Q2];

extern vec3_t monster_flash_offset[];

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

void CLQ2_ClearLasers();
void CLQ2_NewLaser(vec3_t start, vec3_t end, int colors);
void CLQ2_AddLasers();

void CLQ2_ClearBeams();
void CLQ2_RegisterBeamModels();
void CLQ2_ParasiteBeam(int ent, vec3_t start, vec3_t end);
void CLQ2_GrappleCableBeam(int ent, vec3_t start, vec3_t end, vec3_t offset);
void CLQ2_HeatBeam(int ent, vec3_t start, vec3_t end);
void CLQ2_MonsterHeatBeam(int ent, vec3_t start, vec3_t end);
void CLQ2_LightningBeam(int srcEnt, int destEnt, vec3_t start, vec3_t end);
void CLQ2_AddBeams();
void CLQ2_AddPlayerBeams();

void CLQ2_ClearSustains();
void CLQ2_SustainParticleStream(int id, int cnt, vec3_t pos, vec3_t dir, int r, int magnitude, int interval);
void CLQ2_SustainWindow(int id, vec3_t pos);
void CLQ2_SustainNuke(vec3_t pos);
void CLQ2_ProcessSustain();

void CLQ2_RegisterTEntSounds();
void CLQ2_RegisterTEntModels();
void CLQ2_ClearTEnts();
void CLQ2_ParseTEnt(QMsg& message);
void CLQ2_AddTEnts();

void CLQ2_ClearEffects();
void CLQ2_LogoutEffect(vec3_t org, int type);
void CLQ2_ParseMuzzleFlash(QMsg& message);
void CLQ2_ParseMuzzleFlash2(QMsg& message);
void CLQ2_FlyEffect(q2centity_t* ent, vec3_t origin);
void CLQ2_EntityEvent(q2entity_state_t* ent);

void CLQ2_PingServers_f();
