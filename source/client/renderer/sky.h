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

#ifndef _R_SKY_H
#define _R_SKY_H

#include "models/model.h"

shader_t* R_InitSky( mbrush29_texture_t* mt );
void R_InitSkyTexCoords( float cloudLayerHeight );
void RB_StageIteratorSky();
void RB_DrawSun();

#endif
