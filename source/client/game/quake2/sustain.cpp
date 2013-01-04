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
#include "../../utils.h"
#include "../particles.h"
#include "local.h"

enum { MAX_SUSTAINS_Q2 = 32 };

struct q2cl_sustain_t
{
	int id;
	int type;
	int endtime;
	int nextthink;
	int thinkinterval;
	vec3_t org;
	vec3_t dir;
	int color;
	int count;
	int magnitude;
	void (* think)(q2cl_sustain_t* self);
};

static q2cl_sustain_t clq2_sustains[MAX_SUSTAINS_Q2];

void CLQ2_ClearSustains()
{
	Com_Memset(clq2_sustains, 0, sizeof(clq2_sustains));
}

static q2cl_sustain_t* CLQ2_FindFreeSustain()
{
	q2cl_sustain_t* s = clq2_sustains;
	for (int i = 0; i < MAX_SUSTAINS_Q2; i++, s++)
	{
		if (s->id == 0)
		{
			return s;
		}
	}
	return NULL;
}

static void CLQ2_ParticleSteamEffect2(q2cl_sustain_t* self)
{
	vec3_t dir;
	VectorCopy(self->dir, dir);
	vec3_t r, u;
	MakeNormalVectors(dir, r, u);

	for (int i = 0; i < self->count; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;

		p->color = self->color + (rand() & 7);

		for (int j = 0; j < 3; j++)
		{
			p->org[j] = self->org[j] + self->magnitude * 0.1 * crand();
		}
		VectorScale(dir, self->magnitude, p->vel);
		float d = crand() * self->magnitude / 3;
		VectorMA(p->vel, d, r, p->vel);
		d = crand() * self->magnitude / 3;
		VectorMA(p->vel, d, u, p->vel);

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY / 2;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (0.5 + frand() * 0.3);
	}
	self->nextthink += self->thinkinterval;
}

void CLQ2_SustainParticleStream(int id, int cnt, vec3_t pos, vec3_t dir, int r, int magnitude, int interval)
{
	q2cl_sustain_t* s = CLQ2_FindFreeSustain();
	if (!s)
	{
		return;
	}
	s->id = id;
	s->count = cnt;
	VectorCopy(pos, s->org);
	VectorCopy(dir, s->dir);
	s->color = r & 0xff;
	s->magnitude = magnitude;
	s->endtime = cl.serverTime + interval;
	s->think = CLQ2_ParticleSteamEffect2;
	s->thinkinterval = 100;
	s->nextthink = cl.serverTime;
}

static void CLQ2_Widowbeamout(q2cl_sustain_t* self)
{
	static int colortable[4] = {2 * 8, 13 * 8, 21 * 8, 18 * 8};

	float ratio = 1.0 - (((float)self->endtime - (float)cl.serverTime) / 2100.0);

	for (int i = 0; i < 300; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;
		VectorClear(p->accel);

		p->alpha = 1.0;
		p->alphavel = INSTANT_PARTICLE;
		p->color = colortable[rand() & 3];

		vec3_t dir;
		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		VectorNormalize(dir);

		VectorMA(self->org, (45.0 * ratio), dir, p->org);
	}
}

void CLQ2_SustainWindow(int id, vec3_t pos)
{
	q2cl_sustain_t* s = CLQ2_FindFreeSustain();
	if (!s)
	{
		return;
	}
	s->id = id;
	VectorCopy(pos, s->org);
	s->endtime = cl.serverTime + 2100;
	s->think = CLQ2_Widowbeamout;
	s->thinkinterval = 1;
	s->nextthink = cl.serverTime;
}

static void CLQ2_Nukeblast(q2cl_sustain_t* self)
{
	static int colortable[4] = {110, 112, 114, 116};

	float ratio = 1.0 - (((float)self->endtime - (float)cl.serverTime) / 1000.0);

	for (int i = 0; i < 700; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;
		VectorClear(p->accel);

		p->alpha = 1.0;
		p->alphavel = INSTANT_PARTICLE;
		p->color = colortable[rand() & 3];

		vec3_t dir;
		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		VectorNormalize(dir);

		VectorMA(self->org, (200.0 * ratio), dir, p->org);
	}
}

void CLQ2_SustainNuke(vec3_t pos)
{
	q2cl_sustain_t* s = CLQ2_FindFreeSustain();
	if (!s)
	{
		return;
	}
	s->id = 21000;
	VectorCopy(pos, s->org);
	s->endtime = cl.serverTime + 1000;
	s->think = CLQ2_Nukeblast;
	s->thinkinterval = 1;
	s->nextthink = cl.serverTime;
}

void CLQ2_ProcessSustain()
{
	q2cl_sustain_t* s = clq2_sustains;
	for (int i = 0; i < MAX_SUSTAINS_Q2; i++, s++)
	{
		if (!s->id)
		{
			continue;
		}
		if (s->endtime < cl.serverTime)
		{
			s->id = 0;
			continue;
		}
		if (cl.serverTime >= s->nextthink)
		{
			s->think(s);
		}
	}
}
