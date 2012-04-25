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

enum { MAX_LASERS_Q2 = 32 };

struct q2laser_t
{
	refEntity_t entity;
	int endTime;
};

static q2laser_t clq2_lasers[MAX_LASERS_Q2];

void CLQ2_ClearLasers()
{
	Com_Memset(clq2_lasers, 0, sizeof(clq2_lasers));
}

void CLQ2_NewLaser(vec3_t start, vec3_t end, int colors)
{
	q2laser_t* laser = clq2_lasers;
	for (int i = 0; i < MAX_LASERS_Q2; i++, laser++)
	{
		if (laser->endTime < cl.serverTime)
		{
			Com_Memset(laser, 0, sizeof(*laser));
			laser->entity.reType = RT_BEAM;
			laser->entity.renderfx = RF_TRANSLUCENT;
			VectorCopy(start, laser->entity.origin);
			VectorCopy(end, laser->entity.oldorigin);
			laser->entity.shaderRGBA[3] = 76;
			laser->entity.skinNum = (colors >> ((rand() % 4) * 8)) & 0xff;
			laser->entity.hModel = 0;
			laser->entity.frame = 4;
			laser->endTime = cl.serverTime + 100;
			return;
		}
	}
}

void CLQ2_AddLasers()
{
	q2laser_t* laser = clq2_lasers;
	for (int i = 0; i < MAX_LASERS_Q2; i++, laser++)
	{
		if (laser->endTime >= cl.serverTime)
		{
			R_AddRefEntityToScene(&laser->entity);
		}
	}
}
