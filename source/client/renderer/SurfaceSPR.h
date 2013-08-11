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

#ifndef __idSurfaceSPR__
#define __idSurfaceSPR__

#include "Surface.h"
#include "../../common/file_formats/spr.h"
#include "shader.h"

struct msprite1frame_t {
	int width;
	int height;
	float up, down, left, right;
	shader_t* shader;
};

struct msprite1group_t {
	int numframes;
	float* intervals;
	msprite1frame_t* frames[ 1 ];
};

struct msprite1framedesc_t {
	sprite1frametype_t type;
	msprite1frame_t* frameptr;
};

struct msprite1_t : surface_base_t {
	int type;
	int maxwidth;
	int maxheight;
	int numframes;
	float beamlength;					// remove?
	msprite1framedesc_t* frames;
};

class idSurfaceSPR : public idSurface {
public:
	msprite1_t header;

	idSurfaceSPR();

	virtual void Draw();
};

#endif
