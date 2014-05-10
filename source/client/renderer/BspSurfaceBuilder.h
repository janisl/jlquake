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

#ifndef __idSurfaceSubdivider__
#define __idSurfaceSubdivider__

#include "SurfaceFaceQ1Q2.h"

class idBspSurfaceBuilder {
public:
	void Subdivide( idSurfaceFaceQ1Q2* fa );

private:
	enum { SUBDIVIDE_SIZE = 64 };

	struct glpoly_t {
		glpoly_t* next;
		int numverts;
		int indexes[ 4 ];		// variable sized
	};

	idList<idVec3> verts;
	glpoly_t* polys;

	void SubdividePolygon( const idList<int>& vertIndexes );
	void EmitPolygon( const idList<int>& vertIndexes );
	idBounds BoundPoly( const idList<int>& vertIndexes );
};

#endif
