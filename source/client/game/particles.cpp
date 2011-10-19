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

#include "../client.h"

cparticle_t particles[MAX_PARTICLES];
int cl_numparticles = MAX_PARTICLES;
cparticle_t* active_particles;
cparticle_t* free_particles;

Cvar* snow_flurry;
Cvar* snow_active;

vec3_t avelocities[NUMVERTEXNORMALS];

static unsigned d_8to24TranslucentTable[256];

void CL_ClearParticles()
{
	if ((GGameType & GAME_Hexen2) && !(GGameType & GAME_HexenWorld))
	{
		snow_flurry = Cvar_Get("snow_flurry", "1", CVAR_ARCHIVE);
		snow_active = Cvar_Get("snow_active", "1", CVAR_ARCHIVE);
	}

	active_particles = NULL;
	free_particles = &particles[0];
	for (int i = 0; i < cl_numparticles - 1; i++)
	{
		particles[i].next = &particles[i + 1];
	}
	particles[cl_numparticles - 1].next = NULL;

	unsigned* table = d_8to24TranslucentTable;
	for (int i = 0; i < 16; i++)
	{
		int c = ColorIndex[i];

		int r = r_palette[c][0];
		int g = r_palette[c][1];
		int b = r_palette[c][2];

		for (int p = 0; p < 16; p++)
		{
			int v = (ColorPercent[15 - p] << 24) + (r << 0) + (g << 8) + (b << 16);
			*table++ = v;
		}
	}
}

cparticle_t* CL_AllocParticle()
{
	if (!free_particles)
	{
		return NULL;
	}
	cparticle_t* p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cl_common->serverTime;
	return p;
}

static void CL_AddParticle(cparticle_t* p, cparticle_t*& active, cparticle_t*& tail)
{
	float time = (cl_common->serverTime - p->time) * 0.001;
	float alpha;
	if (p->type == pt_q2static)
	{
		if (p->alphavel != INSTANT_PARTICLE)
		{
			alpha = p->alpha + time * p->alphavel;
			if (alpha <= 0)
			{
				// faded out
				p->next = free_particles;
				free_particles = p;
				return;
			}
		}
		else
		{
			alpha = p->alpha;
		}
	}
	else
	{
		if (p->die - cl_common->serverTime < 0)
		{
			p->next = free_particles;
			free_particles = p;
			return;
		}
	}

	p->next = NULL;
	if (!tail)
	{
		active = p;
	}
	else
	{
		tail->next = p;
	}
	tail = p;

	if (p->color < 0 || p->color > 511)
	{
		Log::write("Invalid color for particle type %d\n", (int)p->type);
		return;
	}

	const byte* colour;
	byte theAlpha;
	if (p->type == pt_q2static)
	{
		if (alpha > 1.0)
		{
			alpha = 1;
		}

		colour = r_palette[p->color];
		theAlpha = (int)(alpha * 255);
	}
	else if (p->color <= 255)
	{
		colour = r_palette[p->color];
		if (((GGameType & GAME_QuakeWorld) && p->type == pt_q1fire) ||
			((GGameType & GAME_HexenWorld) && p->type == pt_h2fire))
		{
			theAlpha = 255 * (6 - p->ramp) / 6;
		}
		else
		{
			theAlpha = 255;
		}
	}
	else
	{
		colour = (byte *)&d_8to24TranslucentTable[p->color - 256];
		theAlpha = colour[3];
	}

	float time2 = time * time;

	vec3_t origin;
	if (p->type == pt_q2static)
	{
		origin[0] = p->org[0] + p->vel[0] * time + p->accel[0] * time2;
		origin[1] = p->org[1] + p->vel[1] * time + p->accel[1] * time2;
		origin[2] = p->org[2] + p->vel[2] * time + p->accel[2] * time2;
	}
	else
	{
		VectorCopy(p->org, origin);
	}

	QParticleTexture texture;
	float size;
	if (p->type == pt_h2snow)
	{
		if (p->count>=69)
		{
			texture = PARTTEX_Snow1;
		}
		else if (p->count>=40)
		{
			texture = PARTTEX_Snow2;
		}
		else if (p->count>=30)
		{
			texture = PARTTEX_Snow3;
		}
		else
		{
			texture = PARTTEX_Snow4;
		}
		size = p->count / 10;
	}
	else
	{
		texture = PARTTEX_Default;
		size = 1;
	}

	if (p->type == pt_h2rain)
	{
		float vel0 = p->vel[0] * .001;
		float vel1 = p->vel[1] * .001;
		float vel2 = p->vel[2] * .001;
		for (int i = 0; i < 4; i++)
		{
			R_AddParticleToScene(origin, colour[0], colour[1], colour[2], theAlpha, size, texture);

			origin[0] += vel0;
			origin[1] += vel1;
			origin[2] += vel2;
		}
	}
	else
	{
		R_AddParticleToScene(origin, colour[0], colour[1], colour[2], theAlpha, size, texture);
	}
}

void CL_AddParticles()
{
	cparticle_t* active = NULL;
	cparticle_t* tail = NULL;
	cparticle_t* next;

	for (cparticle_t* p = active_particles; p; p = next)
	{
		next = p->next;
		CL_AddParticle(p, active, tail);
	}

	active_particles = active;
}
