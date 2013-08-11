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

#ifndef __idSurfacePolyBuffer__
#define __idSurfacePolyBuffer__

#include "Surface.h"
#include "public.h"

struct srfPolyBuffer_t : surface_base_t {
	int fogIndex;
	polyBuffer_t* pPolyBuffer;
};

class idSurfacePolyBuffer : public idSurface {
public:
	srfPolyBuffer_t surf;

	idSurfacePolyBuffer();

	virtual void Draw();
};

#endif
