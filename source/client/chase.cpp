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

#include "client.h"

Cvar* chase_active;

static Cvar* chase_back;
static Cvar* chase_up;
static Cvar* chase_right;

void Chase_Init()
{
	chase_back = Cvar_Get("chase_back", "100", 0);
	chase_up = Cvar_Get("chase_up", "16", 0);
	chase_right = Cvar_Get("chase_right", "0", 0);
	chase_active = Cvar_Get("chase_active", "0", 0);
}

static void TraceLine(vec3_t start, vec3_t end, vec3_t impact)
{
	q1trace_t trace;
	Com_Memset(&trace, 0, sizeof(trace));
	CM_HullCheckQ1(0, start, end, &trace);

	VectorCopy(trace.endpos, impact);
}

void Chase_Update()
{
	// if can't see player, reset
	vec3_t forward, up, right;
	AngleVectors(cl.viewangles, forward, right, up);

	// calc exact destination
	vec3_t chase_dest;
	for (int i = 0; i < 3; i++)
	{
		chase_dest[i] = cl.refdef.vieworg[i] - forward[i] * chase_back->value - right[i] * chase_right->value;
	}
	chase_dest[2] = cl.refdef.vieworg[2] + chase_up->value;

	// find the spot the player is looking at
	vec3_t dest;
	VectorMA(cl.refdef.vieworg, 4096, forward, dest);
	vec3_t stop;
	TraceLine(cl.refdef.vieworg, dest, stop);

	// calculate pitch to look at the same spot from camera
	VectorSubtract(stop, cl.refdef.vieworg, cl.refdef.viewaxis[0]);
	VectorNormalize(cl.refdef.viewaxis[0]);
	CrossProduct(cl.refdef.viewaxis[2], cl.refdef.viewaxis[0], cl.refdef.viewaxis[1]);

	// move towards destination
	VectorCopy(chase_dest, cl.refdef.vieworg);
}

