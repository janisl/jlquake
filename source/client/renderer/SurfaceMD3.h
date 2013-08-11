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

#ifndef __idSurfaceMD3__
#define __idSurfaceMD3__

#include "Surface.h"
#include "../../common/file_formats/md3.h"

class idSurfaceMD3 : public idSurface {
public:
	virtual void Draw();

	md3Surface_t* GetMd3Data() const;
	void SetMd3Data( md3Surface_t* data );
};

inline md3Surface_t* idSurfaceMD3::GetMd3Data() const {
	return ( md3Surface_t* )data;
}

inline void idSurfaceMD3::SetMd3Data( md3Surface_t* data ) {
	this->data = ( surface_base_t* )data;
}

#endif
