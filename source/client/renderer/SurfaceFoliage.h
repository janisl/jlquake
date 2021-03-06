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

#ifndef __idSurfaceFoliage__
#define __idSurfaceFoliage__

#include "WorldSurface.h"

// ydnar: foliage surfaces are autogenerated from models into geometry lists by q3map2
typedef byte fcolor4ub_t[ 4 ];

struct foliageInstance_t {
	vec3_t origin;
	fcolor4ub_t color;
};

class idSurfaceFoliage : public idWorldSurface {
public:
	// origins
	int numInstances;
	foliageInstance_t* instances;

	virtual ~idSurfaceFoliage();
	virtual void Draw();

protected:
	virtual bool DoCullET( shader_t* shader, int* frontFace ) const;
};

#endif
