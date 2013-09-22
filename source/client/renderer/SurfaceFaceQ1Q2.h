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

#ifndef __idSurfaceFaceQ1Q2__
#define __idSurfaceFaceQ1Q2__

#include "SurfaceFace.h"

enum { SURFACE_MAX_LIGHTMAPS = 4 };

class idSurfaceFaceQ1Q2 : public idSurfaceFace {
public:
	idTextureInfo* textureInfo;

	short textureMins[ 2 ];
	short extents[ 2 ];

	// gl lightmap coordinates
	int lightS, lightT;

	int lightMapTextureNum;
	byte styles[ SURFACE_MAX_LIGHTMAPS ];
	// values currently used in lightmap
	float cachedLight[ SURFACE_MAX_LIGHTMAPS ];
	// true if dynamic light in cache
	bool cachedDLight;
	// [numstyles*surfsize]
	byte* samples;

	idSurfaceFaceQ1Q2();

	void CalcSurfaceExtents();
};

inline idSurfaceFaceQ1Q2::idSurfaceFaceQ1Q2() {
	textureInfo = NULL;
	textureMins[ 0 ] = 0;
	textureMins[ 1 ] = 0;
	extents[ 0 ] = 0;
	extents[ 1 ] = 0;
	lightS = 0;
	lightT = 0;
	lightMapTextureNum = 0;
	styles[ 0 ] = 0;
	styles[ 1 ] = 0;
	styles[ 2 ] = 0;
	styles[ 3 ] = 0;
	cachedLight[ 0 ] = 0;
	cachedLight[ 1 ] = 0;
	cachedLight[ 2 ] = 0;
	cachedLight[ 3 ] = 0;
	cachedDLight = false;
	samples = NULL;
}

#endif
