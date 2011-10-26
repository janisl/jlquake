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

q1beam_t clq1_beams[MAX_BEAMS_Q1];
q1explosion_t cl_explosions[MAX_EXPLOSIONS_Q1];

sfxHandle_t clq1_sfx_wizhit;
sfxHandle_t clq1_sfx_knighthit;
sfxHandle_t clq1_sfx_tink1;
sfxHandle_t clq1_sfx_ric1;
sfxHandle_t clq1_sfx_ric2;
sfxHandle_t clq1_sfx_ric3;
sfxHandle_t clq1_sfx_r_exp3;

void CLQ1_InitTEnts()
{
	clq1_sfx_wizhit = S_RegisterSound("wizard/hit.wav");
	clq1_sfx_knighthit = S_RegisterSound("hknight/hit.wav");
	clq1_sfx_tink1 = S_RegisterSound("weapons/tink1.wav");
	clq1_sfx_ric1 = S_RegisterSound("weapons/ric1.wav");
	clq1_sfx_ric2 = S_RegisterSound("weapons/ric2.wav");
	clq1_sfx_ric3 = S_RegisterSound("weapons/ric3.wav");
	clq1_sfx_r_exp3 = S_RegisterSound("weapons/r_exp3.wav");
}

void CLQ1_ClearTEnts()
{
	Com_Memset(clq1_beams, 0, sizeof(clq1_beams));
	Com_Memset(cl_explosions, 0, sizeof(cl_explosions));
}
