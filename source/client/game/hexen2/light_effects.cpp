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

#include "../../client.h"

void CLH2_MuzzleFlashLight(int key, vec3_t origin, const vec3_t angles, bool adjustZ)
{
	cdlight_t* dl = CL_AllocDlight(key);
	VectorCopy(origin,  dl->origin);
	if (adjustZ)
	{
		dl->origin[2] += 16;
	}
	vec3_t fv, rv, uv;
	AngleVectors(angles, fv, rv, uv);
	VectorMA(dl->origin, 18, fv, dl->origin);
	dl->radius = 200 + (rand() & 31);
	dl->minlight = 32;
	dl->die = cl.serverTime + 100;
	if (!adjustZ)
	{
		dl->color[0] = 0.2;
		dl->color[1] = 0.1;
		dl->color[2] = 0.05;
	}
}

void CLH2_BrightLight(int key, vec3_t origin)
{
	cdlight_t* dl = CL_AllocDlight(key);
	VectorCopy(origin,  dl->origin);
	dl->origin[2] += 16;
	dl->radius = 400 + (rand() & 31);
	dl->die = cl.serverTime + 1;
}

void CLH2_DimLight(int key, vec3_t origin)
{
	cdlight_t* dl = CL_AllocDlight(key);
	VectorCopy(origin,  dl->origin);
	dl->radius = 200 + (rand() & 31);
	dl->die = cl.serverTime + 1;
}

void CLH2_DarkLight(int key, vec3_t origin)
{
	cdlight_t* dl = CL_AllocDlight(key);
	VectorCopy(origin,  dl->origin);
	dl->radius = 200.0 + (rand() & 31);
	dl->die = cl.serverTime + 1;
	dl->dark = true;
}

void CLH2_Light(int key, vec3_t origin)
{
	cdlight_t* dl = CL_AllocDlight(key);
	VectorCopy(origin,  dl->origin);
	dl->radius = 200;
	dl->die = cl.serverTime + 1;
}

void CLH2_FireBallLight(int key, vec3_t origin)
{
	cdlight_t* dl = CL_AllocDlight(key);
	VectorCopy(origin, dl->origin);
	dl->radius = 120 - (rand() % 20);
	dl->die = cl.serverTime + 10;
}

void CLH2_SpitLight(int key, vec3_t origin)
{
	cdlight_t* dl = CL_AllocDlight(key);
	VectorCopy(origin, dl->origin);
	dl->radius = 120 + (rand() % 20);
	dl->die = cl.serverTime + 50;
}

void CLH2_BrightFieldLight(int key, vec3_t origin)
{
	cdlight_t* dl = CL_AllocDlight(key);
	VectorCopy(origin,  dl->origin);
	dl->radius = 200 + cos(cl.serverTime * 0.005) * 100;
	dl->die = cl.serverTime + 1;
}

void CLH2_ExplosionLight(vec3_t origin)
{
	cdlight_t* dl = CL_AllocDlight(0);
	VectorCopy(origin, dl->origin);
	dl->radius = 350;
	dl->die = cl.serverTime + 500;
	dl->decay = 300;
	if (GGameType & GAME_HexenWorld)
	{
		dl->color[0] = 0.2;
		dl->color[1] = 0.1;
		dl->color[2] = 0.05;
	}
}
