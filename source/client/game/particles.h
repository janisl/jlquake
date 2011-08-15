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

enum { MAX_PARTICLES = 7000 };

enum ptype_t
{
	pt_q2static,

	pt_q1static,
	pt_q1grav,
	pt_q1slowgrav,
	pt_q1fire,
	pt_q1explode,
	pt_q1explode2,
	pt_q1blob,
	pt_q1blob2,

	pt_h2static,
	pt_h2grav,
	pt_h2fastgrav,
	pt_h2slowgrav,
	pt_h2fire,
	pt_h2explode,
	pt_h2explode2,
	pt_h2blob,
	pt_h2blob2,
	pt_h2rain,
	pt_h2c_explode,
	pt_h2c_explode2,
	pt_h2spit,
	pt_h2fireball,
	pt_h2ice,
	pt_h2spell,
	pt_h2test,
	pt_h2quake,
	pt_h2rd,			// rider's death
	pt_h2vorpal,
	pt_h2setstaff,
	pt_h2magicmissile,
	pt_h2boneshard,
	pt_h2scarab,
	pt_h2acidball,
	pt_h2darken,
	pt_h2snow,
	pt_h2gravwell,
	pt_h2redfire,
	pt_h2grensmoke,
	pt_h2bluestep,
};

struct cparticle_t
{
	cparticle_t*	next;
	vec3_t			org;
	vec3_t			vel;
	int				color;

	ptype_t			type;
	float			ramp;
	float			die;

	vec3_t			min_org;
	vec3_t			max_org;
	byte			flags;
	byte			count;

	float			time;
	vec3_t			accel;
	float			alpha;
	float			alphavel;
};

extern cparticle_t particles[MAX_PARTICLES];
extern int cl_numparticles;
extern cparticle_t* active_particles;
extern cparticle_t* free_particles;
extern Cvar* snow_flurry;
extern Cvar* snow_active;

void CL_ClearParticles();
