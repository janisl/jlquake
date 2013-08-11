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

#ifndef __idSurfaceEntity__
#define __idSurfaceEntity__

#include "Surface.h"

// entities that will have procedurally generated surfaces will just
// point at this for their sorting surface
class idSurfaceEntity : public idSurface {
public:
	idSurfaceEntity();

	virtual void Draw();

private:
	static void RB_SurfaceSprite();
	static void RB_SurfaceBeam();
	static void RB_SurfaceBeamQ2();
	static void DoRailCore( const vec3_t start, const vec3_t end, const vec3_t up, float len, float spanWidth );
	static void RB_SurfaceRailCore();
	static void DoRailDiscs( int numSegs, const vec3_t start, const vec3_t dir, const vec3_t right, const vec3_t up );
	static void RB_SurfaceRailRings();
	static void RB_SurfaceLightningBolt();
	static void RB_SurfaceSplash();
	static void RB_SurfaceAxis();
};

extern idSurfaceEntity entitySurface;

#endif
