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

enum { MAX_BEAMS_Q2 = 32 };

struct q2beam_t
{
	int entity;
	int dest_entity;
	qhandle_t model;
	int endtime;
	vec3_t offset;
	vec3_t start;
	vec3_t end;
};

extern q2beam_t clq2_beams[MAX_BEAMS_Q2];
//PMM - added this for player-linked beams.  Currently only used by the plasma beam
extern q2beam_t clq2_playerbeams[MAX_BEAMS_Q2];

extern qhandle_t clq2_mod_heatbeam;
extern qhandle_t clq2_mod_lightning;

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
