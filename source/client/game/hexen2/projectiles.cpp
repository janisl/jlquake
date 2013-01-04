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

#include "../../client_main.h"
#include "../particles.h"
#include "local.h"

#define MAX_PROJECTILES_H2  32
#define MAX_MISSILES_H2     32

struct h2projectile_t
{
	int modelindex;
	vec3_t origin;
	vec3_t angles;
	int frame;
};

struct h2missile_t
{
	int modelindex;
	vec3_t origin;
	int type;
};

int clh2_ravenindex;
int clh2_raven2index;
int clh2_ballindex;
int clh2_missilestarindex;

static h2projectile_t clh2_projectiles[MAX_PROJECTILES_H2];
static int clh2_num_projectiles;

static h2missile_t clh2_missiles[MAX_MISSILES_H2];
static int clh2_num_missiles;

static vec3_t missilestar_angle;

void CLH2_ClearProjectiles()
{
	clh2_num_projectiles = 0;
}

void CLH2_ClearMissiles()
{
	clh2_num_missiles = 0;
}

//	Nails are passed as efficient temporary entities
// Note: all references to these functions have been replaced with
//	calls to the Missile functions below
void CLHW_ParseNails(QMsg& message)
{
	int c = message.ReadByte();
	for (int i = 0; i < c; i++)
	{
		byte bits[6];
		for (int j = 0; j < 6; j++)
		{
			bits[j] = message.ReadByte();
		}

		if (clh2_num_projectiles == MAX_PROJECTILES_H2)
		{
			continue;
		}

		h2projectile_t* pr = &clh2_projectiles[clh2_num_projectiles];
		clh2_num_projectiles++;

		pr->modelindex = clh2_ravenindex;
		pr->origin[0] = ((bits[0] + ((bits[1] & 15) << 8)) << 1) - 4096;
		pr->origin[1] = (((bits[1] >> 4) + (bits[2] << 4)) << 1) - 4096;
		pr->origin[2] = ((bits[3] + ((bits[4] & 15) << 8)) << 1) - 4096;
		pr->angles[0] = 360 * (bits[4] >> 4) / 16;
		pr->angles[1] = 360 * (bits[5] & 31) / 32;
		pr->frame = (bits[5] >> 5) & 7;
	}

	c = message.ReadByte();
	for (int i = 0; i < c; i++)
	{
		byte bits[6];
		for (int j = 0; j < 6; j++)
		{
			bits[j] = message.ReadByte();
		}

		if (clh2_num_projectiles == MAX_PROJECTILES_H2)
		{
			continue;
		}

		h2projectile_t* pr = &clh2_projectiles[clh2_num_projectiles];
		clh2_num_projectiles++;

		pr->modelindex = clh2_raven2index;
		pr->origin[0] = ((bits[0] + ((bits[1] & 15) << 8)) << 1) - 4096;
		pr->origin[1] = (((bits[1] >> 4) + (bits[2] << 4)) << 1) - 4096;
		pr->origin[2] = ((bits[3] + ((bits[4] & 15) << 8)) << 1) - 4096;
		pr->angles[0] = 360 * (bits[4] >> 4) / 16;
		pr->angles[1] = 360 * bits[5] / 256;
		pr->frame = 0;
	}
}

//	Missiles are passed as efficient temporary entities
void CLHW_ParsePackMissiles(QMsg& message)
{
	int c = message.ReadByte();
	for (int i = 0; i < c; i++)
	{
		byte bits[5];
		for (int j = 0; j < 5; j++)
		{
			bits[j] = message.ReadByte();
		}

		if (clh2_num_missiles == MAX_MISSILES_H2)
		{
			continue;
		}

		h2missile_t* pr = &clh2_missiles[clh2_num_missiles];
		clh2_num_missiles++;

		pr->origin[0] = ((bits[0] + ((bits[1] & 15) << 8)) << 1) - 4096;
		pr->origin[1] = (((bits[1] >> 4) + (bits[2] << 4)) << 1) - 4096;
		pr->origin[2] = ((bits[3] + ((bits[4] & 15) << 8)) << 1) - 4096;
		pr->type = bits[4] >> 4;
		//type may be used later to select models
	}
}

void CLH2_LinkProjectiles()
{
	int i;
	h2projectile_t* pr;

	for (i = 0, pr = clh2_projectiles; i < clh2_num_projectiles; i++, pr++)
	{
		// grab an entity to fill in
		if (pr->modelindex < 1)
		{
			continue;
		}
		refEntity_t ent;
		Com_Memset(&ent, 0, sizeof(ent));
		ent.reType = RT_MODEL;

		ent.hModel = cl.model_draw[pr->modelindex];
		ent.frame = pr->frame;
		VectorCopy(pr->origin, ent.origin);
		CLH2_SetRefEntAxis(&ent, pr->angles, vec3_origin, 0, 0, 0, 0);
		R_AddRefEntityToScene(&ent);
	}
}

void CLH2_LinkMissiles()
{
	missilestar_angle[1] += cls.frametime * 0.3;
	missilestar_angle[2] += cls.frametime * 0.4;

	h2missile_t* pr = clh2_missiles;
	for (int i = 0; i < clh2_num_missiles; i++, pr++)
	{
		// grab an entity to fill in for missile itself
		refEntity_t ent;
		Com_Memset(&ent, 0, sizeof(ent));
		ent.reType = RT_MODEL;

		VectorCopy(pr->origin, ent.origin);
		if (pr->type == 1)
		{
			//ball
			ent.hModel = cl.model_draw[clh2_ballindex];
			CLH2_SetRefEntAxis(&ent, vec3_origin, vec3_origin, 10, 0, 0, H2SCALE_ORIGIN_CENTER);
		}
		else
		{
			//missilestar
			ent.hModel = cl.model_draw[clh2_missilestarindex];
			CLH2_SetRefEntAxis(&ent, missilestar_angle, vec3_origin, 50, 0, 0, H2SCALE_ORIGIN_CENTER);
		}
		if (rand() % 10 < 3)
		{
			CLH2_RunParticleEffect4(ent.origin, 7, 148 + rand() % 11, pt_h2grav, 10 + rand() % 10);
		}
		R_AddRefEntityToScene(&ent);
	}
}
