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

#ifndef _R_DECALS_H
#define _R_DECALS_H

#include "main.h"

// ydnar: decals
#define MAX_WORLD_DECALS        1024
#define MAX_ENTITY_DECALS       128

void R_AddModelShadow( const refEntity_t* ent );
void R_TransformDecalProjector( decalProjector_t * in, vec3_t axis[ 3 ], vec3_t origin, decalProjector_t * out );
bool R_TestDecalBoundingBox( decalProjector_t* dp, vec3_t mins, vec3_t maxs );
void R_ProjectDecalOntoSurface( decalProjector_t* dp, idWorldSurface* surf, mbrush46_model_t* bmodel );
void R_AddDecalSurfaces( mbrush46_model_t* bmodel );
void R_CullDecalProjectors();

#endif
