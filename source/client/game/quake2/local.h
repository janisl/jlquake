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

enum { MAX_EXPLOSIONS_Q2 = 32 };

enum q2exptype_t
{
	ex_free, ex_explosion, ex_misc, ex_flash, ex_mflash, ex_poly, ex_poly2
};

struct q2explosion_t
{
	q2exptype_t type;
	refEntity_t ent;

	int frames;
	float light;
	vec3_t lightcolor;
	int start;
	int baseframe;
};

extern q2explosion_t q2cl_explosions[MAX_EXPLOSIONS_Q2];

void CLQ2_ClearExplosions();
q2explosion_t* CLQ2_AllocExplosion();
