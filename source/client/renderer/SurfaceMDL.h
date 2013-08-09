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

#ifndef __idSurfaceMDL__
#define __idSurfaceMDL__

#include "Surface.h"
#include "shader.h"
#include "../../common/file_formats/mdl.h"
#include "../../common/math/Vec2.h"

#define MAX_MESH1_SKINS     32

struct mmesh1framedesc_t {
	int firstpose;
	int numposes;
	float interval;
	dmdl_trivertx_t bboxmin;
	dmdl_trivertx_t bboxmax;
	int frame;
	char name[ 16 ];
};

struct mesh1hdr_t : surface_base_t {
	int version;
	vec3_t scale;
	vec3_t scale_origin;
	float boundingradius;
	vec3_t eyeposition;
	int numskins;
	int skinwidth;
	int skinheight;
	int numverts;
	int numtris;
	int numframes;
	synctype_t synctype;
	int flags;
	float size;

	int numposes;
	int poseverts;
	int numIndexes;
	dmdl_trivertx_t* posedata;		// numposes*poseverts trivert_t
	idVec2* texCoords;
	glIndex_t* indexes;
	shader_t* shaders[ MAX_MESH1_SKINS ];
	mmesh1framedesc_t* frames;
};

class idSurfaceMDL : public idSurface {
public:
	mesh1hdr_t header;
	
	idSurfaceMDL();
};

#endif
