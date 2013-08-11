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

#ifndef __idSurfaceParticles__
#define __idSurfaceParticles__

#include "Surface.h"
#include "main.h"

class idSurfaceParticles : public idSurface {
public:
	idSurfaceParticles();

	virtual void Draw();

private:
	vec3_t up;
	vec3_t right;
	vec3_t normal;

	void DrawParticle( const particle_t* p, float s1, float t1, float s2, float t2 ) const;
	void DrawParticleTriangles();
	static void DrawParticlePoints();
};

extern idSurfaceParticles particlesSurface;

#endif
