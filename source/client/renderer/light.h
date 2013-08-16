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

#ifndef _R_LIGHT_H
#define _R_LIGHT_H

#include "main.h"

#define DLIGHT_CUTOFF       64

extern vec3_t lightspot;

int R_LightPointQ1( vec3_t p );
void R_LightPointQ2( vec3_t p, vec3_t color, trRefdef_t& refdef );
void R_SetupEntityLighting( const trRefdef_t* refdef, trRefEntity_t* ent );
void R_TransformDlights( int count, dlight_t* dl, orientationr_t* orient );
void R_DlightBmodelQ1( idRenderModel* bmodel );
void R_DlightBmodelQ2( idRenderModel* bmodel );
void R_DlightBmodel( mbrush46_model_t* bmodel );
void R_CullDlights();

#endif
