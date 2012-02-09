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

#include "core.h"

movevars_t movevars;
qhplayermove_t qh_pmove;
qhpml_t qh_pml;

vec3_t pmqh_player_mins;
vec3_t pmqh_player_maxs;

vec3_t pmqh_player_maxs_crouch;

void PMQH_Init()
{
	if (GGameType & GAME_Quake)
	{
		VectorSet(pmqh_player_mins, -16, -16, -24);
		VectorSet(pmqh_player_maxs, 16, 16, 32);
	}
	else
	{
		VectorSet(pmqh_player_mins, -16, -16, 0);
		VectorSet(pmqh_player_maxs, 16, 16, 56);
		VectorSet(pmqh_player_maxs_crouch, 16, 16, 28);
	}
}
