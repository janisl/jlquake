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

#include "../../client.h"
#include "local.h"

q2explosion_t q2cl_explosions[MAX_EXPLOSIONS_Q2];

static qhandle_t cl_mod_explode;
static qhandle_t cl_mod_smoke;
static qhandle_t cl_mod_flash;
static qhandle_t cl_mod_explo4;
static qhandle_t cl_mod_bfg_explo;
static qhandle_t cl_mod_explo4_big;

void CLQ2_ClearExplosions()
{
	Com_Memset(q2cl_explosions, 0, sizeof(q2cl_explosions));
}

void CLQ2_RegisterExplosionModels()
{
	cl_mod_explode = R_RegisterModel("models/objects/explode/tris.md2");
	cl_mod_smoke = R_RegisterModel("models/objects/smoke/tris.md2");
	cl_mod_flash = R_RegisterModel("models/objects/flash/tris.md2");
	cl_mod_explo4 = R_RegisterModel("models/objects/r_explode/tris.md2");
	cl_mod_bfg_explo = R_RegisterModel("sprites/s_bfg2.sp2");
	cl_mod_explo4_big = R_RegisterModel("models/objects/r_explode2/tris.md2");
}

static q2explosion_t* CLQ2_AllocExplosion()
{
	for (int i = 0; i < MAX_EXPLOSIONS_Q2; i++)
	{
		if (q2cl_explosions[i].type == ex_free)
		{
			Com_Memset(&q2cl_explosions[i], 0, sizeof (q2cl_explosions[i]));
			q2cl_explosions[i].ent.reType = RT_MODEL;
			return &q2cl_explosions[i];
		}
	}

	// find the oldest explosion
	int time = cl_common->serverTime;
	int index = 0;
	for (int i = 0; i < MAX_EXPLOSIONS_Q2; i++)
	{
		if (q2cl_explosions[i].start < time)
		{
			time = q2cl_explosions[i].start;
			index = i;
		}
	}
	Com_Memset(&q2cl_explosions[index], 0, sizeof (q2cl_explosions[index]));
	q2cl_explosions[index].ent.reType = RT_MODEL;
	return &q2cl_explosions[index];
}

void CLQ2_SmokeAndFlash(vec3_t origin)
{
	q2explosion_t* ex = CLQ2_AllocExplosion();
	VectorCopy(origin, ex->ent.origin);
	ex->type = ex_misc;
	ex->frames = 4;
	ex->ent.renderfx = RF_TRANSLUCENT;
	ex->start = cl_common->q2_frame.servertime - 100;
	ex->ent.hModel = cl_mod_smoke;

	ex = CLQ2_AllocExplosion();
	VectorCopy(origin, ex->ent.origin);
	ex->type = ex_flash;
	ex->ent.renderfx = RF_ABSOLUTE_LIGHT;
	ex->ent.radius = 1;
	ex->frames = 2;
	ex->start = cl_common->q2_frame.servertime - 100;
	ex->ent.hModel = cl_mod_flash;
}

void CLQ2_BlasterExplosion(vec3_t pos, vec3_t dir)
{
	q2explosion_t* ex = CLQ2_AllocExplosion();
	VectorCopy(pos, ex->ent.origin);
	vec3_t angles;
	angles[0] = acos(dir[2]) / M_PI * 180;
	// PMM - fixed to correct for pitch of 0
	if (dir[0])
	{
		angles[1] = atan2(dir[1], dir[0]) / M_PI * 180;
	}
	else if (dir[1] > 0)
	{
		angles[1] = 90;
	}
	else if (dir[1] < 0)
	{
		angles[1] = 270;
	}
	else
	{
		angles[1] = 0;
	}
	angles[2] = 0;
	AnglesToAxis(angles, ex->ent.axis);

	ex->type = ex_misc;
	ex->ent.renderfx = RF_TRANSLUCENT | RF_ABSOLUTE_LIGHT;
	ex->ent.radius = 1;
	ex->start = cl_common->q2_frame.servertime - 100;
	ex->light = 150;
	ex->lightcolor[0] = 1;
	ex->lightcolor[1] = 1;
	ex->ent.hModel = cl_mod_explode;
	ex->frames = 4;
}

void CLQ2_GrenadeExplosion(vec3_t pos)
{
	q2explosion_t* ex = CLQ2_AllocExplosion();
	VectorCopy(pos, ex->ent.origin);
	ex->type = ex_poly;
	ex->ent.renderfx = RF_ABSOLUTE_LIGHT;
	ex->ent.radius = 1;
	ex->start = cl_common->q2_frame.servertime - 100;
	ex->light = 350;
	ex->lightcolor[0] = 1.0;
	ex->lightcolor[1] = 0.5;
	ex->lightcolor[2] = 0.5;
	ex->ent.hModel = cl_mod_explo4;
	ex->frames = 19;
	ex->baseframe = 30;
	vec3_t	angles;
	angles[0] = 0;
	angles[1] = rand() % 360;
	angles[2] = 0;
	AnglesToAxis(angles, ex->ent.axis);
}

void CLQ2_PlasmaExplosion(vec3_t pos)
{
	q2explosion_t* ex = CLQ2_AllocExplosion();
	VectorCopy(pos, ex->ent.origin);
	ex->type = ex_poly;
	ex->ent.renderfx = RF_ABSOLUTE_LIGHT;
	ex->ent.radius = 1;
	ex->start = cl_common->q2_frame.servertime - 100;
	ex->light = 350;
	ex->lightcolor[0] = 1.0; 
	ex->lightcolor[1] = 0.5;
	ex->lightcolor[2] = 0.5;
	vec3_t angles;
	angles[0] = 0;
	angles[1] = rand() % 360;
	angles[2] = 0;
	AnglesToAxis(angles, ex->ent.axis);
	ex->ent.hModel = cl_mod_explo4;
	if (frand() < 0.5)
	{
		ex->baseframe = 15;
	}
	ex->frames = 15;
}

void CLQ2_RocketExplosion(vec3_t pos)
{
	q2explosion_t* ex = CLQ2_AllocExplosion();
	VectorCopy(pos, ex->ent.origin);
	ex->type = ex_poly;
	ex->ent.renderfx = RF_ABSOLUTE_LIGHT;
	ex->ent.radius = 1;
	ex->start = cl_common->q2_frame.servertime - 100;
	ex->light = 350;
	ex->lightcolor[0] = 1.0;
	ex->lightcolor[1] = 0.5;
	ex->lightcolor[2] = 0.5;
	vec3_t angles;
	angles[0] = 0;
	angles[1] = rand() % 360;
	angles[2] = 0;
	AnglesToAxis(angles, ex->ent.axis);
	ex->ent.hModel = cl_mod_explo4;			// PMM
	if (frand() < 0.5)
	{
		ex->baseframe = 15;
	}
	ex->frames = 15;
}

void CLQ2_BigExplosion(vec3_t pos)
{
	q2explosion_t* ex = CLQ2_AllocExplosion();
	VectorCopy(pos, ex->ent.origin);
	ex->type = ex_poly;
	ex->ent.renderfx = RF_ABSOLUTE_LIGHT;
	ex->ent.radius = 1;
	ex->start = cl_common->q2_frame.servertime - 100;
	ex->light = 350;
	ex->lightcolor[0] = 1.0;
	ex->lightcolor[1] = 0.5;
	ex->lightcolor[2] = 0.5;
	vec3_t angles;
	angles[0] = 0;
	angles[1] = rand() % 360;
	angles[2] = 0;
	AnglesToAxis(angles, ex->ent.axis);
	ex->ent.hModel = cl_mod_explo4_big;
	if (frand() < 0.5)
	{
		ex->baseframe = 15;
	}
	ex->frames = 15;
}

void CLQ2_BfgExplosion(vec3_t pos)
{
	q2explosion_t* ex = CLQ2_AllocExplosion();
	VectorCopy(pos, ex->ent.origin);
	ex->type = ex_poly;
	ex->ent.renderfx = RF_ABSOLUTE_LIGHT;
	ex->ent.radius = 1;
	ex->start = cl_common->q2_frame.servertime - 100;
	ex->light = 350;
	ex->lightcolor[0] = 0.0;
	ex->lightcolor[1] = 1.0;
	ex->lightcolor[2] = 0.0;
	ex->ent.hModel = cl_mod_bfg_explo;
	ex->ent.renderfx |= RF_TRANSLUCENT;
	ex->ent.shaderRGBA[3] = 76;
	ex->frames = 4;
}

void CLQ2_WeldingSparks(vec3_t pos)
{
	q2explosion_t* ex = CLQ2_AllocExplosion();
	VectorCopy(pos, ex->ent.origin);
	ex->type = ex_flash;
	// note to self
	// we need a better no draw flag
	ex->ent.reType = RT_BEAM;
	ex->start = cl_common->q2_frame.servertime - 100;
	ex->light = 100 + (rand() % 75);
	ex->lightcolor[0] = 1.0;
	ex->lightcolor[1] = 1.0;
	ex->lightcolor[2] = 0.3;
	ex->ent.hModel = cl_mod_flash;
	ex->frames = 2;
}

void CLQ2_Blaster2Explosion(vec3_t pos, vec3_t dir)
{
	q2explosion_t* ex = CLQ2_AllocExplosion();
	VectorCopy(pos, ex->ent.origin);
	vec3_t angles;
	angles[0] = acos(dir[2]) / M_PI * 180;
	// PMM - fixed to correct for pitch of 0
	if (dir[0])
	{
		angles[1] = atan2(dir[1], dir[0]) / M_PI * 180;
	}
	else if (dir[1] > 0)
	{
		angles[1] = 90;
	}
	else if (dir[1] < 0)
	{
		angles[1] = 270;
	}
	else
	{
		angles[1] = 0;
	}
	angles[2] = 0;
	AnglesToAxis(angles, ex->ent.axis);

	ex->type = ex_misc;
	ex->ent.renderfx = RF_TRANSLUCENT | RF_ABSOLUTE_LIGHT;
	ex->ent.radius = 1;
	ex->ent.skinNum = 1;
	ex->start = cl_common->q2_frame.servertime - 100;
	ex->light = 150;
	ex->lightcolor[1] = 1;
	ex->ent.hModel = cl_mod_explode;
	ex->frames = 4;
}

void CLQ2_FlechetteExplosion(vec3_t pos, vec3_t dir)
{
	q2explosion_t* ex = CLQ2_AllocExplosion();
	VectorCopy(pos, ex->ent.origin);
	vec3_t angles;
	angles[0] = acos(dir[2]) / M_PI * 180;
	// PMM - fixed to correct for pitch of 0
	if (dir[0])
	{
		angles[1] = atan2(dir[1], dir[0]) / M_PI * 180;
	}
	else if (dir[1] > 0)
	{
		angles[1] = 90;
	}
	else if (dir[1] < 0)
	{
		angles[1] = 270;
	}
	else
	{
		angles[1] = 0;
	}
	angles[2] = 0;
	AnglesToAxis(angles, ex->ent.axis);

	ex->type = ex_misc;
	ex->ent.renderfx = RF_TRANSLUCENT | RF_ABSOLUTE_LIGHT;
	ex->ent.radius = 1;
	ex->ent.skinNum = 2;
	ex->start = cl_common->q2_frame.servertime - 100;
	ex->light = 150;
	ex->lightcolor[0] = 0.19;
	ex->lightcolor[1] = 0.41;
	ex->lightcolor[2] = 0.75;
	ex->ent.hModel = cl_mod_explode;
	ex->frames = 4;
}

void CLQ2_PlainExplosion(vec3_t pos)
{
	q2explosion_t* ex = CLQ2_AllocExplosion();
	VectorCopy(pos, ex->ent.origin);
	ex->type = ex_poly;
	ex->ent.renderfx = RF_ABSOLUTE_LIGHT;
	ex->ent.radius = 1;
	ex->start = cl_common->q2_frame.servertime - 100;
	ex->light = 350;
	ex->lightcolor[0] = 1.0;
	ex->lightcolor[1] = 0.5;
	ex->lightcolor[2] = 0.5;
	vec3_t angles;
	angles[0] = 0;
	angles[1] = rand() % 360;
	angles[2] = 0;
	AnglesToAxis(angles, ex->ent.axis);
	ex->ent.hModel = cl_mod_explo4;
	if (frand() < 0.5)
	{
		ex->baseframe = 15;
	}
	ex->frames = 15;
}
