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

void CLQ1_MuzzleFlashLight(int key, vec3_t origin, vec3_t angles)
{
	cdlight_t* dl = CL_AllocDlight(key);
	VectorCopy(origin,  dl->origin);
	if (!(GGameType & GAME_QuakeWorld))
	{
		dl->origin[2] += 16;
	}
	vec3_t fv, rv, uv;
	AngleVectors(angles, fv, rv, uv);
	VectorMA(dl->origin, 18, fv, dl->origin);

	dl->radius = 200 + (rand() & 31);
	dl->minlight = 32;
	dl->die = cl_common->serverTime + 100;
	if (GGameType & GAME_QuakeWorld)
	{
		dl->color[0] = 0.2;
		dl->color[1] = 0.1;
		dl->color[2] = 0.05;
	}
}

void CLQ1_BrightLight(int key, vec3_t origin)
{
	cdlight_t* dl = CL_AllocDlight(key);
	VectorCopy(origin,  dl->origin);
	dl->origin[2] += 16;
	dl->radius = 400 + (rand() & 31);
	if (GGameType & GAME_QuakeWorld)
	{
		dl->die = cl_common->serverTime + 100;
		dl->color[0] = 0.2;
		dl->color[1] = 0.1;
		dl->color[2] = 0.05;
	}
	else
	{
		dl->die = cl_common->serverTime + 1;
	}
}

void CLQ1_DimLight(int key, vec3_t origin, int type)
{
	cdlight_t* dl = CL_AllocDlight(key);
	VectorCopy(origin, dl->origin);
	dl->radius = 200 + (rand() & 31);
	if (GGameType & GAME_QuakeWorld)
	{
		dl->die = cl_common->serverTime + 100;
		if (type == 0)
		{
			dl->color[0] = 0.2;
			dl->color[1] = 0.1;
			dl->color[2] = 0.05;
		}
		else if (type == 1)
		{
			dl->color[0] = 0.05;
			dl->color[1] = 0.05;
			dl->color[2] = 0.3;
		}
		else if (type == 2)
		{
			dl->color[0] = 0.5;
			dl->color[1] = 0.05;
			dl->color[2] = 0.05;
		}
		else if (type == 3)
		{
			dl->color[0] = 0.5;
			dl->color[1] = 0.05;
			dl->color[2] = 0.4;
		}
	}
	else
	{
		dl->die = cl_common->serverTime + 10;
	}
}

void CLQ1_RocketLight(int key, vec3_t origin)
{
	cdlight_t* dl = CL_AllocDlight(key);
	VectorCopy(origin, dl->origin);
	dl->radius = 200;
	if (GGameType & GAME_QuakeWorld)
	{
		dl->die = cl_common->serverTime + 100;
	}
	else
	{
		dl->die = cl_common->serverTime + 10;
	}
}

void CLQ1_ExplosionLight(vec3_t origin)
{
	cdlight_t* dl = CL_AllocDlight(0);
	VectorCopy(origin, dl->origin);
	dl->radius = 350;
	dl->die = cl_common->serverTime + 500;
	dl->decay = 300;
	if (GGameType & GAME_QuakeWorld)
	{
		dl->color[0] = 0.2;
		dl->color[1] = 0.1;
		dl->color[2] = 0.05;
	}
}
