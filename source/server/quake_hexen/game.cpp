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

#include "../server.h"
#include "../progsvm/progsvm.h"
#include "local.h"

//	This was a major timewaster in progs, so it was converted to C
void PF_changeyaw()
{
	qhedict_t* ent = PROG_TO_EDICT(*pr_globalVars.self);
	float current = AngleMod(ent->GetAngles()[1]);
	float ideal = ent->GetIdealYaw();
	float speed = ent->GetYawSpeed();

	if (current == ideal)
	{
		if (GGameType & GAME_Hexen2)
		{
			G_FLOAT(OFS_RETURN) = 0;
		}
		return;
	}
	float move = ideal - current;
	if (ideal > current)
	{
		if (move >= 180)
		{
			move = move - 360;
		}
	}
	else
	{
		if (move <= -180)
		{
			move = move + 360;
		}
	}

	if (GGameType & GAME_Hexen2)
	{
		G_FLOAT(OFS_RETURN) = move;
	}

	if (move > 0)
	{
		if (move > speed)
		{
			move = speed;
		}
	}
	else
	{
		if (move < -speed)
		{
			move = -speed;
		}
	}

	ent->GetAngles()[1] = AngleMod(current + move);
}
