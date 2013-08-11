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

#ifndef __idSurfaceDecal__
#define __idSurfaceDecal__

#include "Surface.h"
#include "public.h"

#define MAX_DECAL_VERTS     10	// worst case is triangle clipped by 6 planes

struct srfDecal_t {
	int numVerts;
	polyVert_t verts[ MAX_DECAL_VERTS ];
};

class idSurfaceDecal : public idSurface {
public:
	srfDecal_t surf;

	idSurfaceDecal();

	virtual void Draw();
};

#endif
