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

#ifndef __idSurfaceFaceQ1__
#define __idSurfaceFaceQ1__

#include "SurfaceFace.h"
#include "../../common/file_formats/bsp29.h"
#include "shader.h"

struct mbrush29_texture_t {
	char name[ 16 ];
	unsigned width, height;
	shader_t* shader;
	image_t* gl_texture;
	image_t* fullBrightTexture;
	int anim_total;						// total tenths in sequence ( 0 = no)
	mbrush29_texture_t* anim_next;		// in the animation sequence
	mbrush29_texture_t* anim_base;		// first frame of animation
	mbrush29_texture_t* alternate_anims;	// bmodels in frmae 1 use these
	unsigned offsets[ BSP29_MIPLEVELS ];			// four mip maps stored
};

struct mbrush29_texinfo_t {
	float vecs[ 2 ][ 4 ];
	float mipadjust;
	mbrush29_texture_t* texture;
	int flags;
};

struct mbrush29_surface_t {
	int flags;

	short texturemins[ 2 ];
	short extents[ 2 ];

	int light_s, light_t;			// gl lightmap coordinates

	mbrush29_texinfo_t* texinfo;
	shader_t* altShader;

	int lightmaptexturenum;
	byte styles[ BSP29_MAXLIGHTMAPS ];
	int cached_light[ BSP29_MAXLIGHTMAPS ];				// values currently used in lightmap
	qboolean cached_dlight;					// true if dynamic light in cache
	byte* samples;				// [numstyles*surfsize]
};

class idSurfaceFaceQ1 : public idSurfaceFace {
public:
	mbrush29_surface_t surf;
	idSurfaceFaceQ1* texturechain;
	
    idSurfaceFaceQ1();
};

#endif
