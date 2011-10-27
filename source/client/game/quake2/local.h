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

enum { MAX_EXPLOSIONS_Q2 = 32 };

enum q2exptype_t
{
	ex_free, ex_explosion, ex_misc, ex_flash, ex_mflash, ex_poly, ex_poly2
};

struct q2explosion_t
{
	q2exptype_t type;
	refEntity_t ent;

	int frames;
	float light;
	vec3_t lightcolor;
	int start;
	int baseframe;
};

extern q2explosion_t q2cl_explosions[MAX_EXPLOSIONS_Q2];

extern qhandle_t cl_mod_explode;
extern qhandle_t cl_mod_flash;
extern qhandle_t cl_mod_explo4;
extern qhandle_t cl_mod_bfg_explo;
extern qhandle_t cl_mod_explo4_big;

void CLQ2_ClearExplosions();
void CLQ2_RegisterExplosionModels();
q2explosion_t* CLQ2_AllocExplosion();
void CLQ2_SmokeAndFlash(vec3_t origin);
void CLQ2_BlasterExplosion(vec3_t pos, vec3_t dir);
void CLQ2_GrenadeExplosion(vec3_t pos);
void CLQ2_PlasmaExplosion(vec3_t pos);
void CLQ2_RocketExplosion(vec3_t pos);
void CLQ2_BigExplosion(vec3_t pos);
void CLQ2_BfgExplosion(vec3_t pos);
void CLQ2_WeldingSparks(vec3_t pos);
void CLQ2_Blaster2Explosion(vec3_t pos, vec3_t dir);
void CLQ2_FlechetteExplosion(vec3_t pos, vec3_t dir);
void CLQ2_PlainExplosion(vec3_t pos);
