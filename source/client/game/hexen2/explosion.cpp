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

h2explosion_t clh2_explosions[H2MAX_EXPLOSIONS];

void CLH2_ClearExplosions()
{
	Com_Memset(clh2_explosions, 0, sizeof(clh2_explosions));
}

//**** CAREFUL!!! This may overwrite an explosion!!!!!
h2explosion_t* CLH2_AllocExplosion()
{
	int index = 0;
	bool freeSlot = false;

	for (int i = 0; i < H2MAX_EXPLOSIONS; i++)
	{
		if (!clh2_explosions[i].model)
		{
			index = i;
			freeSlot = true;
			break;
		}
	}


	// find the oldest explosion
	if (!freeSlot)
	{
		float time = cl_common->serverTime * 0.001;

		for (int i = 0; i < H2MAX_EXPLOSIONS; i++)
		{
			if (clh2_explosions[i].startTime < time)
			{
				time = clh2_explosions[i].startTime;
				index = i;
			}
		}
	}

	//zero out velocity and acceleration, funcs
	Com_Memset(&clh2_explosions[index], 0, sizeof(h2explosion_t));

	return &clh2_explosions[index];
}
