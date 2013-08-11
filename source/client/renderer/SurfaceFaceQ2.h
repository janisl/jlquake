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

#ifndef __idSurfaceFaceQ2__
#define __idSurfaceFaceQ2__

#include "Surface.h"
#include "../../common/file_formats/bsp38.h"
#include "shader.h"

#define BRUSH38_VERTEXSIZE  7

struct mbrush38_texinfo_t {
	float vecs[ 2 ][ 4 ];
	int flags;
	int numframes;
	mbrush38_texinfo_t* next;		// animation chain
	image_t* image;
};

struct mbrush38_shaderInfo_t {
	int numframes;
	mbrush38_shaderInfo_t* next;	// animation chain
	shader_t* shader;
};

struct mbrush38_glpoly_t {
	mbrush38_glpoly_t* next;
	int numverts;
	float verts[ 4 ][ BRUSH38_VERTEXSIZE ];		// variable sized (xyz s1t1 s2t2)
};

struct mbrush38_surface_t : surface_base_t {
	int visframe;				// should be drawn when node is crossed

	cplane_t* plane;
	int flags;

	int firstedge;			// look up in model->surfedges[], negative numbers
	int numedges;			// are backwards edges

	short texturemins[ 2 ];
	short extents[ 2 ];

	int light_s, light_t;			// gl lightmap coordinates

	mbrush38_glpoly_t* polys;				// multiple if warped

	mbrush38_texinfo_t* texinfo;
	mbrush38_shaderInfo_t* shaderInfo;

	// lighting info
	int dlightframe;
	int dlightbits;

	int lightmaptexturenum;
	byte styles[ BSP38_MAXLIGHTMAPS ];
	float cached_light[ BSP38_MAXLIGHTMAPS ];			// values currently used in lightmap
	qboolean cached_dlight;					// true if dynamic light in cache
	byte* samples;				// [numstyles*surfsize]
};

class idSurfaceFaceQ2 : public idSurface {
public:
	mbrush38_surface_t surf;
	idSurfaceFaceQ2* texturechain;

	idSurfaceFaceQ2();

	virtual void Draw();
};

#endif