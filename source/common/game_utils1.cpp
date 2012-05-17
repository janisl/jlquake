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

#include "qcommon.h"

static Cvar* cl_rollspeed;
static Cvar* cl_rollangle;

void VQH_InitRollCvars()
{
	cl_rollspeed = Cvar_Get("cl_rollspeed", "200", 0);
	cl_rollangle = Cvar_Get("cl_rollangle", "2.0", 0);
}

float VQH_CalcRoll(const vec3_t angles, const vec3_t velocity)
{
	vec3_t forward, right, up;
	float sign;
	float side;
	float value;

	AngleVectors(angles, forward, right, up);
	side = DotProduct(velocity, right);
	sign = side < 0 ? -1 : 1;
	side = Q_fabs(side);

	value = cl_rollangle->value;

	if (side < cl_rollspeed->value)
	{
		side = side * value / cl_rollspeed->value;
	}
	else
	{
		side = value;
	}

	return side * sign;
}
