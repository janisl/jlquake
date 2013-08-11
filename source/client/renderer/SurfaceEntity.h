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

#ifndef __idSurfaceEntity__
#define __idSurfaceEntity__

#include "Surface.h"

// entities that will have procedurally generated surfaces will just
// point at this for their sorting surface
class idSurfaceEntity : public idSurface {
public:
	idSurfaceEntity();

	virtual void Draw();
};

extern idSurfaceEntity entitySurface;

#endif
