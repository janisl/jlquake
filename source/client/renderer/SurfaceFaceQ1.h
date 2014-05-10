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

#include "SurfaceFaceQ1Q2.h"
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
	mbrush29_texture_t* texture;
	int flags;
};

class idSurfaceFaceQ1 : public idSurfaceFaceQ1Q2 {
public:
	int flags;
	shader_t* altShader;
	mbrush29_texinfo_t* texinfo;

	idSurfaceFaceQ1();
};

#endif
