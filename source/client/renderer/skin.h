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

#ifndef _R_SKIN_H
#define _R_SKIN_H

#include "shader.h"
#include "../../common/file_formats/md3.h"

#define MAX_PART_MODELS         5

// skins allow models to be retextured without modifying the model file
struct skinSurface_t {
	char name[ MAX_QPATH ];
	shader_t* shader;
	int hash;
};

struct skinModel_t {
	char type[ MAX_QPATH ];			// md3_lower, md3_lbelt, md3_rbelt, etc.
	char model[ MAX_QPATH ];		// lower.md3, belt1.md3, etc.
	int hash;
};

struct skin_t {
	char name[ MAX_QPATH ];						// game path, including extension
	int numSurfaces;
	skinSurface_t* surfaces[ MD3_MAX_SURFACES ];
	int numModels;
	skinModel_t* models[ MAX_PART_MODELS ];
	vec3_t scale;		//----(SA)	added
};

void R_InitSkins();
skin_t* R_GetSkinByHandle( qhandle_t hSkin );
void R_SkinList_f();

#endif
