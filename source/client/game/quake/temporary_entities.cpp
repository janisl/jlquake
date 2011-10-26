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

void CLQ1_ParseBeam(QMsg& message, qhandle_t model)
{
	int entity = message.ReadShort();

	vec3_t start;
	start[0] = message.ReadCoord();
	start[1] = message.ReadCoord();
	start[2] = message.ReadCoord();

	vec3_t end;
	end[0] = message.ReadCoord();
	end[1] = message.ReadCoord();
	end[2] = message.ReadCoord();

	// override any beam with the same entity
	q1beam_t* beam = clq1_beams;
	for (int i = 0; i < MAX_BEAMS_Q1; i++, beam++)
	{
		if (beam->entity == entity)
		{
			beam->entity = entity;
			beam->model = model;
			beam->endtime = cl_common->serverTime * 0.001 + 0.2;
			VectorCopy(start, beam->start);
			VectorCopy(end, beam->end);
			return;
		}
	}

	// find a free beam
	beam = clq1_beams;
	for (int i = 0; i < MAX_BEAMS_Q1; i++, beam++)
	{
		if (!beam->model || beam->endtime < cl_common->serverTime * 0.001)
		{
			beam->entity = entity;
			beam->model = model;
			beam->endtime = cl_common->serverTime * 0.001 + 0.2;
			VectorCopy (start, beam->start);
			VectorCopy (end, beam->end);
			return;
		}
	}
	Log::write("beam list overflow!\n");
}
