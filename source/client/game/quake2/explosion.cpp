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

void CLQ2_ClearExplosions()
{
	Com_Memset(q2cl_explosions, 0, sizeof(q2cl_explosions));
}

q2explosion_t* CLQ2_AllocExplosion()
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
