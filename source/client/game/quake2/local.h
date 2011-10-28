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

extern Cvar* q2_hand;

extern sfxHandle_t cl_sfx_ric1;
extern sfxHandle_t cl_sfx_ric2;
extern sfxHandle_t cl_sfx_ric3;
extern sfxHandle_t cl_sfx_lashit;
extern sfxHandle_t cl_sfx_spark5;
extern sfxHandle_t cl_sfx_spark6;
extern sfxHandle_t cl_sfx_spark7;
extern sfxHandle_t cl_sfx_railg;
extern sfxHandle_t cl_sfx_rockexp;
extern sfxHandle_t cl_sfx_grenexp;
extern sfxHandle_t cl_sfx_watrexp;
extern sfxHandle_t cl_sfx_footsteps[4];

extern qhandle_t cl_mod_parasite_tip;
extern qhandle_t cl_mod_powerscreen;

extern sfxHandle_t cl_sfx_lightning;
extern sfxHandle_t cl_sfx_disrexp;

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
void CLQ2_ParseBlood(QMsg& message);
void CLQ2_ParseGunShot(QMsg& message);
void CLQ2_ParseSparks(QMsg& message);
void CLQ2_ParseBulletSparks(QMsg& message);
void CLQ2_ParseScreenSparks(QMsg& message);
void CLQ2_ParseShieldSparks(QMsg& message);
void CLQ2_ParseShotgun(QMsg& message);
void CLQ2_ParseSplash(QMsg& message);
void CLQ2_ParseLaserSparks(QMsg& message);
void CLQ2_ParseBlueHyperBlaster(QMsg& message);
void CLQ2_ParseBlaster(QMsg& message);
void CLQ2_ParseRailTrail(QMsg& message);
void CLQ2_ParseGrenadeExplosion(QMsg& message);
void CLQ2_ParseGrenadeExplosionWater(QMsg& message);
void CLQ2_ParsePlasmaExplosion(QMsg& message);
void CLQ2_ParseRocketExplosion(QMsg& message);
void CLQ2_ParseExplosion1Big(QMsg& message);
void CLQ2_ParseRocketExplosionWater(QMsg& message);
void CLQ2_ParsePlainExplosion(QMsg& message);
void CLQ2_ParseBfgExplosion(QMsg& message);
void CLQ2_ParseBfgBigExplosion(QMsg& message);
void CLQ2_ParseBfgLaser(QMsg& message);
void CLQ2_ParseBubleTrail(QMsg& message);
void CLQ2_ParseParasiteAttack(QMsg& message);
void CLQ2_ParseBossTeleport(QMsg& message);
void CLQ2_ParseGrappleCable(QMsg& message);
void CLQ2_ParseGrappleCable(QMsg& message);
void CLQ2_ParseWeldingSparks(QMsg& message);
void CLQ2_ParseGreenBlood(QMsg& message);
void CLQ2_ParseTunnelSparks(QMsg& message);
void CLQ2_ParseBlaster2(QMsg& message);
void CLQ2_ParseFlechette(QMsg& message);
void CLQ2_ParseLightning(QMsg& message);
void CLQ2_ParseDebugTrail(QMsg& message);
void CLQ2_ParseFlashLight(QMsg& message);
void CLQ2_ParseForceWall(QMsg& message);
void CLQ2_ParseHeatBeam(QMsg& message);
void CLQ2_ParseMonsterHeatBeam(QMsg& message);
void CLQ2_ParseHeatBeamSparks(QMsg& message);
void CLQ2_ParseHeatBeamSteam(QMsg& message);
void CLQ2_ParseSteam(QMsg& message);
void CLQ2_ParseBubleTrail2(QMsg& message);
void CLQ2_ParseMoreBlood(QMsg& message);
void CLQ2_ParseChainfistSmoke(QMsg& message);
void CLQ2_ParseElectricSparks(QMsg& message);
void CLQ2_ParseTrackerExplosion(QMsg& message);
void CLQ2_ParseTeleportEffect(QMsg& message);
void CLQ2_ParseWidowBeamOut(QMsg& message);
void CLQ2_ParseNukeBlast(QMsg& message);
void CLQ2_ParseWidowSplash(QMsg& message);
