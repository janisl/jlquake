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

#ifndef __idWorldSurface__
#define __idWorldSurface__

#include "Surface.h"

enum surfaceType_t
{
	SF_BAD,
	SF_SKIP,				// ignore
	SF_FACE,
	SF_FACE_Q1,
	SF_FACE_Q2,
	SF_GRID,
	SF_TRIANGLES,
	SF_FOLIAGE,
	SF_FLARE,
};

struct surface_base_t {
	surfaceType_t surfaceType;
};

class idWorldSurface : public idSurface {
public:
	// Q1, Q2 - should be drawn when node is crossed
	// Q3 - if == tr.viewCount, already added
	int viewCount;

	surface_base_t* data;		// any of srf*_t

	idWorldSurface();
};

inline idWorldSurface::idWorldSurface() {
	viewCount = 0;
	data = NULL;
}

#endif
