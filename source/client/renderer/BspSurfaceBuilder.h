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
#include "../../common/file_formats/bsp.h"

class idBspSurfaceBuilder {
public:
	idStr name;
	byte* fileBase;

	idBspSurfaceBuilder( const idStr& name, byte* fileBase );
	~idBspSurfaceBuilder();
	void LoadVertexes( bsp_lump_t* l );
	void LoadEdges( bsp_lump_t* l );
	void LoadSurfedges( bsp_lump_t* l );
	void BuildSurfaceVertexesList( idSurfaceFaceQ1Q2* fa, int firstedge, int numedges );
	void Subdivide( idSurfaceFaceQ1Q2* fa );

private:
	enum { SUBDIVIDE_SIZE = 64 };

	struct mbrush_vertex_t {
		vec3_t position;
	};

	struct mbrush_edge_t {
		unsigned short v[ 2 ];
	};

	struct glpoly_t {
		glpoly_t* next;
		int numverts;
		int indexes[ 4 ];		// variable sized
	};

	int numvertexes;
	mbrush_vertex_t* vertexes;

	int numedges;
	mbrush_edge_t* edges;

	int numsurfedges;
	int* surfedges;

	idList<idVec3> verts;
	glpoly_t* polys;

	void SubdividePolygon( const idList<int>& vertIndexes );
	void EmitPolygon( const idList<int>& vertIndexes );
	idBounds BoundPoly( const idList<int>& vertIndexes );
};

#endif
