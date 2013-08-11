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

#include "particle.h"
#include "main.h"
#include "backend.h"
#include "surfaces.h"
#include "cvars.h"
#include "state.h"
#include "../../common/common_defs.h"

static byte dottexture[ 16 ][ 16 ] =
{
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},	//1
	{0,0,0,1,0,1,0,0,0,0,0,0,1,1,0,0},
	{0,0,0,0,1,0,0,0,0,0,0,1,1,1,1,0},
	{0,1,0,1,1,1,0,1,0,0,0,1,1,1,1,0},	//4
	{0,0,1,1,1,1,1,0,0,0,0,0,1,1,0,0},
	{0,1,0,1,1,1,0,1,0,0,0,0,0,0,0,0},
	{0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0},	//8
	{0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0},
	{0,0,0,0,0,0,0,0,0,1,1,1,0,1,0,0},
	{0,0,0,0,0,0,0,0,1,1,0,1,1,0,1,0},
	{0,0,0,1,0,0,0,0,1,1,1,1,1,0,1,0},	//12
	{0,1,1,1,0,0,0,0,1,1,0,1,1,0,1,0},
	{0,0,1,1,1,0,0,0,0,1,1,1,0,1,0,0},
	{0,0,1,0,0,0,0,0,0,0,1,1,1,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},	//16
};

void R_InitParticleTexture() {
	//
	// particle texture
	//
	byte data[ 16 ][ 16 ][ 4 ];
	for ( int x = 0; x < 16; x++ ) {
		for ( int y = 0; y < 16; y++ ) {
			data[ y ][ x ][ 0 ] = 255;
			data[ y ][ x ][ 1 ] = 255;
			data[ y ][ x ][ 2 ] = 255;
			data[ y ][ x ][ 3 ] = dottexture[ y ][ x ] * 255;
		}
	}
	tr.particleImage = R_CreateImage( "*particle", ( byte* )data, 16, 16, true, false, GL_CLAMP );
}
