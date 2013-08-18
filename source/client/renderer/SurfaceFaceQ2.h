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

#include "SurfaceFace.h"
#include "../../common/file_formats/bsp38.h"
#include "shader.h"

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

struct mbrush38_surface_t {
	int firstedge;			// look up in model->surfedges[], negative numbers
	int numedges;			// are backwards edges

	short texturemins[ 2 ];
	short extents[ 2 ];

	int light_s, light_t;			// gl lightmap coordinates

	mbrush38_texinfo_t* texinfo;
	mbrush38_shaderInfo_t* shaderInfo;

	int lightmaptexturenum;
	byte styles[ BSP38_MAXLIGHTMAPS ];
	float cached_light[ BSP38_MAXLIGHTMAPS ];			// values currently used in lightmap
	qboolean cached_dlight;					// true if dynamic light in cache
	byte* samples;				// [numstyles*surfsize]

	glIndex_t* indexes;
	int numIndexes;
};

class idSurfaceFaceQ2 : public idSurfaceFace {
public:
	mbrush38_surface_t surf;
	idSurfaceFaceQ2* texturechain;

	idSurfaceFaceQ2();

	virtual void Draw();

protected:
	virtual bool DoCull( shader_t* shader ) const;
	virtual int DoMarkDynamicLights( int dlightBits );
};

#endif
