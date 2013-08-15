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

#ifndef __idSurfaceSkip__
#define __idSurfaceSkip__

#include "WorldSurface.h"

class idSurfaceSkip : public idWorldSurface {
public:
	virtual void Draw();
	virtual bool AddToNodeBounds() const;

protected:
	virtual bool DoCullET( shader_t* shader, int* frontFace ) const;
	virtual int DoMarkDynamicLightsET( int dlightBits );
};

#endif
