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
#include "shader.h"

// everything that is needed by the backend needs
// to be double buffered to allow it to run in
// parallel on a dual cpu machine
#define SMP_FRAMES      2

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
};

struct surface_base_t {
	surfaceType_t surfaceType;
};

class idWorldSurface : public idSurface {
public:
	// Q1, Q2 - should be drawn when node is crossed
	// Q3 - if == tr.viewCount, already added
	int viewCount;
	shader_t* shader;
	int fogIndex;

	// dynamic lighting information
	int dlightBits[ SMP_FRAMES ];

	idWorldSurface();

	virtual void ProjectDecal( struct decalProjector_t* dp, struct mbrush46_model_t* bmodel );

	surface_base_t* GetBrush46Data();
	void SetBrush46Data( surface_base_t* data );

protected:
	surface_base_t* data;		// any of srf*_t

	bool ClipDecal( struct decalProjector_t* dp );
};

inline idWorldSurface::idWorldSurface() {
	viewCount = 0;
	fogIndex = 0;
	shader = NULL;
	dlightBits[ 0 ] = 0;
	dlightBits[ 1 ] = 0;
	data = NULL;
}

inline surface_base_t* idWorldSurface::GetBrush46Data() {
	return data;
}

inline void idWorldSurface::SetBrush46Data( surface_base_t* data ) {
	this->data = data;
}

#endif
