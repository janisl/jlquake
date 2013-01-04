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

#include "../dynamic_lights.h"
#include "../../client_main.h"

void CLQ2_MuzzleFlashLight(int key, vec3_t origin, vec3_t angles, float radius, int delay, vec3_t colour)
{
	cdlight_t* dl = CL_AllocDlight(key);
	VectorCopy(origin,  dl->origin);
	vec3_t fv, rv;
	AngleVectors(angles, fv, rv, NULL);
	VectorMA(dl->origin, 18, fv, dl->origin);
	VectorMA(dl->origin, 16, rv, dl->origin);
	dl->radius = radius;
	dl->minlight = 32;
	dl->die = cl.serverTime + delay;
	VectorCopy(colour, dl->color);
}

void CLQ2_MuzzleFlash2Light(int key, vec3_t origin, float radius, int delay, vec3_t colour)
{
	cdlight_t* dl = CL_AllocDlight(key);
	VectorCopy(origin,  dl->origin);
	dl->minlight = 32;
	dl->radius = radius;
	dl->die = cl.serverTime + delay;
	VectorCopy(colour, dl->color);
}

void CLQ2_Flashlight(int key, vec3_t origin)
{
	cdlight_t* dl = CL_AllocDlight(key);
	VectorCopy(origin,  dl->origin);
	dl->radius = 400;
	dl->minlight = 250;
	dl->die = cl.serverTime + 100;
	dl->color[0] = 1;
	dl->color[1] = 1;
	dl->color[2] = 1;
}

void CLQ2_ColorFlash(int key, vec3_t origin, int intensity, float r, float g, float b)
{
	cdlight_t* dl = CL_AllocDlight(key);
	VectorCopy(origin,  dl->origin);
	dl->radius = intensity;
	dl->minlight = 250;
	dl->die = cl.serverTime + 100;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
}
