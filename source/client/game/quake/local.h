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

enum { MAX_BEAMS_Q1 = 24 };

struct q1beam_t
{
	int entity;
	qhandle_t model;
	float endtime;
	vec3_t start;
	vec3_t end;
};

enum { MAX_EXPLOSIONS_Q1 = 8 };

struct q1explosion_t
{
	vec3_t		origin;
	float		start;
	qhandle_t	model;
};

extern q1beam_t clq1_beams[MAX_BEAMS_Q1];
extern q1explosion_t cl_explosions[MAX_EXPLOSIONS_Q1];
