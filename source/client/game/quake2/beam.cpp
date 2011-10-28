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

q2beam_t clq2_beams[MAX_BEAMS_Q2];
//PMM - added this for player-linked beams.  Currently only used by the plasma beam
q2beam_t clq2_playerbeams[MAX_BEAMS_Q2];

static qhandle_t clq2_mod_parasite_segment;
static qhandle_t clq2_mod_grapple_cable;
qhandle_t clq2_mod_heatbeam;
qhandle_t clq2_mod_lightning;

void CLQ2_ClearBeams()
{
	Com_Memset(clq2_beams, 0, sizeof(clq2_beams));
	Com_Memset(clq2_playerbeams, 0, sizeof(clq2_playerbeams));
}

void CLQ2_RegisterBeamModels()
{
	clq2_mod_parasite_segment = R_RegisterModel("models/monsters/parasite/segment/tris.md2");
	clq2_mod_grapple_cable = R_RegisterModel("models/ctf/segment/tris.md2");
	clq2_mod_heatbeam = R_RegisterModel("models/proj/beam/tris.md2");
	clq2_mod_lightning = R_RegisterModel("models/proj/lightning/tris.md2");
}

static void CLQ2_InitBeam(q2beam_t* b, int ent, int destEnt, qhandle_t model,
	int duration, vec3_t start, vec3_t end, vec3_t offset)
{
	b->entity = ent;
	b->dest_entity = destEnt;
	b->model = model;
	b->endtime = cl_common->serverTime + duration;
	VectorCopy(start, b->start);
	VectorCopy(end, b->end);
	VectorCopy(offset, b->offset);
}

static void CLQ2_NewBeamInternal(q2beam_t* beams, int ent, int destEnt, qhandle_t model,
	int duration, vec3_t start, vec3_t end, vec3_t offset)
{
	// override any beam with the same source AND destination entities
	q2beam_t* b = beams;
	for (int i = 0; i < MAX_BEAMS_Q2; i++, b++)
	{
		if (b->entity == ent && b->dest_entity == destEnt)
		{
			CLQ2_InitBeam(b, ent, destEnt, model, 200, start, end, offset);
			return;
		}
	}

	b = beams;
	for (int i = 0; i < MAX_BEAMS_Q2; i++, b++)
	{
		if (!b->model || b->endtime < cl_common->serverTime)
		{
			CLQ2_InitBeam(b, ent, destEnt, model, duration, start, end, offset);
			return;
		}
	}
	Log::write("beam list overflow!\n");
}

static void CLQ2_NewBeam(int ent, int destEnt, qhandle_t model, vec3_t start, vec3_t end, vec3_t offset)
{
	CLQ2_NewBeamInternal(clq2_beams, ent, destEnt, model, 200, start, end, offset);
}

static void CLQ2_NewPlayerBeam(int ent, qhandle_t model, vec3_t start, vec3_t end, vec3_t offset)
{
	// PMM - this needs to be 100 to prevent multiple heatbeams
	CLQ2_NewBeamInternal(clq2_playerbeams, ent, 0, model, 100, start, end, offset);
}

void CLQ2_ParasiteBeam(int ent, vec3_t start, vec3_t end)
{
	CLQ2_NewBeam(ent, 0, clq2_mod_parasite_segment, start, end, vec3_origin);
}

void CLQ2_GrappleCableBeam(int ent, vec3_t start, vec3_t end, vec3_t offset)
{
	CLQ2_NewBeam(ent, 0, clq2_mod_grapple_cable, start, end, offset);
}

void CLQ2_HeatBeam(int ent, vec3_t start, vec3_t end)
{
	vec3_t offset;
	VectorSet(offset, 2, 7, -3);
	CLQ2_NewPlayerBeam(ent, clq2_mod_heatbeam, start, end, offset);
}

void CLQ2_MonsterHeatBeam(int ent, vec3_t start, vec3_t end)
{
	vec3_t offset;
	VectorSet(offset, 0, 0, 0);
	CLQ2_NewPlayerBeam(ent, clq2_mod_heatbeam, start, end, offset);
}

void CLQ2_LightningBeam(int srcEnt, int destEnt, vec3_t start, vec3_t end)
{
	CLQ2_NewBeam(srcEnt, destEnt, clq2_mod_lightning, start, end, vec3_origin);
}
