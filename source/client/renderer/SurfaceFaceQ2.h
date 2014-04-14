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

#include "SurfaceFaceQ1Q2.h"
#include "../../common/file_formats/bsp38.h"
#include "shader.h"

struct mbrush38_texinfo_t {
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
	mbrush38_texinfo_t* texinfo;
	mbrush38_shaderInfo_t* shaderInfo;
};

class idSurfaceFaceQ2 : public idSurfaceFaceQ1Q2 {
public:
	mbrush38_surface_t surf;

	idSurfaceFaceQ2();
};

#endif
