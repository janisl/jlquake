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

enum { MAX_EXPLOSIONS_Q2 = 32 };

enum q2exptype_t
{
	ex_free,
	ex_explosion,
	ex_misc,
	ex_flash,
	ex_mflash,
	ex_poly,
	ex_poly2,
	ex_sparks
};

struct q2explosion_t
{
	q2exptype_t type;
	refEntity_t entity;
	int frames;
	float light;
	vec3_t lightColour;
	int start;
	int baseFrame;
};

static q2explosion_t q2cl_explosions[MAX_EXPLOSIONS_Q2];

static qhandle_t clq2_mod_explode;
static qhandle_t clq2_mod_smoke;
static qhandle_t clq2_mod_flash;
static qhandle_t clq2_mod_explo4;
static qhandle_t clq2_mod_bfg_explo;
static qhandle_t clq2_mod_explo4_big;

void CLQ2_ClearExplosions()
{
	Com_Memset(q2cl_explosions, 0, sizeof(q2cl_explosions));
}

void CLQ2_RegisterExplosionModels()
{
	clq2_mod_explode = R_RegisterModel("models/objects/explode/tris.md2");
	clq2_mod_smoke = R_RegisterModel("models/objects/smoke/tris.md2");
	clq2_mod_flash = R_RegisterModel("models/objects/flash/tris.md2");
	clq2_mod_explo4 = R_RegisterModel("models/objects/r_explode/tris.md2");
	clq2_mod_bfg_explo = R_RegisterModel("sprites/s_bfg2.sp2");
	clq2_mod_explo4_big = R_RegisterModel("models/objects/r_explode2/tris.md2");
}

static q2explosion_t* CLQ2_AllocExplosion()
{
	for (int i = 0; i < MAX_EXPLOSIONS_Q2; i++)
	{
		if (q2cl_explosions[i].type == ex_free)
		{
			Com_Memset(&q2cl_explosions[i], 0, sizeof (q2cl_explosions[i]));
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
	return &q2cl_explosions[index];
}

static q2explosion_t* NewExplosion(vec3_t origin)
{
	q2explosion_t* explosion = CLQ2_AllocExplosion();
	explosion->start = cl_common->q2_frame.servertime - 100;
	explosion->entity.reType = RT_MODEL;
	VectorCopy(origin, explosion->entity.origin);
	return explosion;
}

void CLQ2_SmokeAndFlash(vec3_t origin)
{
	q2explosion_t* explosion = NewExplosion(origin);
	explosion->type = ex_misc;
	explosion->frames = 4;
	explosion->entity.renderfx = RF_TRANSLUCENT;
	explosion->entity.hModel = clq2_mod_smoke;
	AxisClear(explosion->entity.axis);

	explosion = NewExplosion(origin);
	explosion->type = ex_flash;
	explosion->entity.renderfx = RF_ABSOLUTE_LIGHT;
	explosion->entity.radius = 1;
	explosion->frames = 2;
	explosion->entity.hModel = clq2_mod_flash;
	AxisClear(explosion->entity.axis);
}

static q2explosion_t* NewBlasterExplosion(vec3_t pos, vec3_t dir)
{
	q2explosion_t* explosion = NewExplosion(pos);
	vec3_t angles;
	float forward, yaw, pitch;
	if (dir[0] && dir[1])
	{
		VecToAnglesCommon(dir, angles, forward, yaw, pitch);
	}
	else
	{
		yaw = 0;
		pitch = acos(dir[2]) / M_PI * 180;
	}
	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0;
	AnglesToAxis(angles, explosion->entity.axis);

	explosion->type = ex_misc;
	explosion->entity.renderfx = RF_TRANSLUCENT | RF_ABSOLUTE_LIGHT;
	explosion->entity.radius = 1;
	explosion->entity.hModel = clq2_mod_explode;
	explosion->frames = 4;
	explosion->light = 150;
	return explosion;
}

void CLQ2_BlasterExplosion(vec3_t pos, vec3_t dir)
{
	q2explosion_t* explosion = NewBlasterExplosion(pos, dir);
	explosion->lightColour[0] = 1;
	explosion->lightColour[1] = 1;
}

void CLQ2_GrenadeExplosion(vec3_t pos)
{
	q2explosion_t* explosion = NewExplosion(pos);
	explosion->type = ex_poly;
	explosion->entity.renderfx = RF_ABSOLUTE_LIGHT;
	explosion->entity.radius = 1;
	explosion->light = 350;
	explosion->lightColour[0] = 1.0;
	explosion->lightColour[1] = 0.5;
	explosion->lightColour[2] = 0.5;
	explosion->entity.hModel = clq2_mod_explo4;
	explosion->frames = 19;
	explosion->baseFrame = 30;
	vec3_t	angles;
	angles[0] = 0;
	angles[1] = rand() % 360;
	angles[2] = 0;
	AnglesToAxis(angles, explosion->entity.axis);
}

void CLQ2_RocketExplosion(vec3_t pos)
{
	q2explosion_t* explosion = NewExplosion(pos);
	explosion->type = ex_poly;
	explosion->entity.renderfx = RF_ABSOLUTE_LIGHT;
	explosion->entity.radius = 1;
	explosion->light = 350;
	explosion->lightColour[0] = 1.0;
	explosion->lightColour[1] = 0.5;
	explosion->lightColour[2] = 0.5;
	vec3_t angles;
	angles[0] = 0;
	angles[1] = rand() % 360;
	angles[2] = 0;
	AnglesToAxis(angles, explosion->entity.axis);
	explosion->entity.hModel = clq2_mod_explo4;
	if (frand() < 0.5)
	{
		explosion->baseFrame = 15;
	}
	explosion->frames = 15;
}

void CLQ2_BigExplosion(vec3_t pos)
{
	q2explosion_t* explosion = NewExplosion(pos);
	explosion->type = ex_poly;
	explosion->entity.renderfx = RF_ABSOLUTE_LIGHT;
	explosion->entity.radius = 1;
	explosion->light = 350;
	explosion->lightColour[0] = 1.0;
	explosion->lightColour[1] = 0.5;
	explosion->lightColour[2] = 0.5;
	vec3_t angles;
	angles[0] = 0;
	angles[1] = rand() % 360;
	angles[2] = 0;
	AnglesToAxis(angles, explosion->entity.axis);
	explosion->entity.hModel = clq2_mod_explo4_big;
	if (frand() < 0.5)
	{
		explosion->baseFrame = 15;
	}
	explosion->frames = 15;
}

void CLQ2_BfgExplosion(vec3_t pos)
{
	q2explosion_t* explosion = NewExplosion(pos);
	explosion->type = ex_poly;
	explosion->entity.renderfx = RF_ABSOLUTE_LIGHT;
	explosion->entity.radius = 1;
	explosion->light = 350;
	explosion->lightColour[0] = 0.0;
	explosion->lightColour[1] = 1.0;
	explosion->lightColour[2] = 0.0;
	explosion->entity.hModel = clq2_mod_bfg_explo;
	explosion->entity.renderfx |= RF_TRANSLUCENT;
	explosion->entity.shaderRGBA[3] = 76;
	AxisClear(explosion->entity.axis);
	explosion->frames = 4;
}

void CLQ2_WeldingSparks(vec3_t pos)
{
	q2explosion_t* explosion = NewExplosion(pos);
	explosion->type = ex_sparks;
	explosion->light = 100 + (rand() % 75);
	explosion->lightColour[0] = 1.0;
	explosion->lightColour[1] = 1.0;
	explosion->lightColour[2] = 0.3;
	explosion->frames = 2;
}

void CLQ2_Blaster2Explosion(vec3_t pos, vec3_t dir)
{
	q2explosion_t* explosion = NewBlasterExplosion(pos, dir);
	explosion->entity.skinNum = 1;
	explosion->lightColour[1] = 1;
}

void CLQ2_FlechetteExplosion(vec3_t pos, vec3_t dir)
{
	q2explosion_t* explosion = NewBlasterExplosion(pos, dir);
	explosion->entity.skinNum = 2;
	explosion->lightColour[0] = 0.19;
	explosion->lightColour[1] = 0.41;
	explosion->lightColour[2] = 0.75;
}

void CLQ2_AddExplosions()
{
	q2explosion_t* explosion = q2cl_explosions;
	for (int i = 0; i < MAX_EXPLOSIONS_Q2; i++, explosion++)
	{
		if (explosion->type == ex_free)
		{
			continue;
		}
		float fraction = (cl_common->serverTime - explosion->start) / 100.0;
		int frame = (int)floor(fraction);

		refEntity_t* entity = &explosion->entity;

		switch (explosion->type)
		{
		case ex_mflash:
		case ex_sparks:
			if (frame >= explosion->frames - 1)
			{
				explosion->type = ex_free;
			}
			break;
		case ex_misc:
			if (frame >= explosion->frames - 1)
			{
				explosion->type = ex_free;
				break;
			}
			entity->shaderRGBA[3] = (int)((1.0 - fraction / (explosion->frames - 1)) * 255);
			break;
		case ex_flash:
			if (frame >= 1)
			{
				explosion->type = ex_free;
				break;
			}
			entity->shaderRGBA[3] = 255;
			break;
		case ex_poly:
			if (frame >= explosion->frames - 1)
			{
				explosion->type = ex_free;
				break;
			}

			entity->shaderRGBA[3] = (int)((16.0 - (float)frame) / 16.0 * 255);

			if (frame < 10)
			{
				entity->skinNum = (frame >> 1);
				if (entity->skinNum < 0)
				{
					entity->skinNum = 0;
				}
			}
			else
			{
				entity->renderfx |= RF_TRANSLUCENT;
				if (frame < 13)
				{
					entity->skinNum = 5;
				}
				else
				{
					entity->skinNum = 6;
				}
			}
			break;
		case ex_poly2:
			if (frame >= explosion->frames - 1)
			{
				explosion->type = ex_free;
				break;
			}

			entity->shaderRGBA[3] = (int)((5.0 - (float)frame) / 5.0 * 255);
			entity->skinNum = 0;
			entity->renderfx |= RF_TRANSLUCENT;
			break;
		default:
			break;
		}

		if (explosion->type == ex_free)
		{
			continue;
		}
		if (explosion->light)
		{
			R_AddLightToScene(entity->origin, explosion->light * (float)entity->shaderRGBA[3] / 255.0,
				explosion->lightColour[0], explosion->lightColour[1], explosion->lightColour[2]);
		}
		if (explosion->type == ex_sparks)
		{
			continue;
		}

		VectorCopy(entity->origin, entity->oldorigin);

		if (frame < 0)
		{
			frame = 0;
		}
		entity->frame = explosion->baseFrame + frame + 1;
		entity->oldframe = explosion->baseFrame + frame;
		entity->backlerp = 1.0 - cl_common->q2_lerpfrac;

		R_AddRefEntityToScene(entity);
	}
}
