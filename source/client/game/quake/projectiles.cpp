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

#include "../../client.h"
#include "local.h"

#define MAX_PROJECTILES_Q1	32

struct q1projectile_t
{
	int modelindex;
	vec3_t origin;
	vec3_t angles;
};

static q1projectile_t clq1_projectiles[MAX_PROJECTILES_Q1];
static int clq1_num_projectiles;
int clq1_spikeindex;

void CLQ1_ClearProjectiles()
{
	clq1_num_projectiles = 0;
}

//	Nails are passed as efficient temporary entities
void CLQW_ParseNails(QMsg& message)
{
	int c = message.ReadByte();
	for (int i = 0; i < c; i++)
	{
		byte bits[6];
		for (int j = 0; j < 6; j++)
		{
			bits[j] = message.ReadByte();
		}

		if (clq1_num_projectiles == MAX_PROJECTILES_Q1)
		{
			continue;
		}

		q1projectile_t* pr = &clq1_projectiles[clq1_num_projectiles];
		clq1_num_projectiles++;

		pr->modelindex = clq1_spikeindex;
		pr->origin[0] = ((bits[0] + ((bits[1] & 15) << 8)) << 1) - 4096;
		pr->origin[1] = (((bits[1] >> 4) + (bits[2] << 4)) << 1) - 4096;
		pr->origin[2] = ((bits[3] + ((bits[4] & 15) << 8)) << 1) - 4096;
		pr->angles[0] = 360 * (bits[4] >> 4) / 16;
		pr->angles[1] = 360 * bits[5] / 256;
	}
}

void CLQ1_LinkProjectiles()
{
	q1projectile_t* pr = clq1_projectiles;
	for (int i = 0; i < clq1_num_projectiles; i++, pr++)
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
		VectorCopy(pr->origin, ent.origin);
		CLQ1_SetRefEntAxis(&ent, pr->angles);
		R_AddRefEntityToScene(&ent);
	}
}
