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
	return p;
}
