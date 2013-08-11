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

#include "SurfacePoly.h"
#include "surfaces.h"

idSurfacePoly::idSurfacePoly() {
	data = &surf;
}

void idSurfacePoly::Draw() {
	RB_SurfacePolychain( &surf );
}

cplane_t idSurfacePoly::GetPlane() const {
	srfPoly_t* poly = ( srfPoly_t* )data;
	vec4_t plane4;
	PlaneFromPoints( plane4, poly->verts[ 0 ].xyz, poly->verts[ 1 ].xyz, poly->verts[ 2 ].xyz );
	cplane_t plane;
	VectorCopy( plane4, plane.normal );
	plane.dist = plane4[ 3 ];
	return plane;
}
