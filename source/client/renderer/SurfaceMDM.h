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

#ifndef __idSurfaceMDM__
#define __idSurfaceMDM__

#include "Surface.h"
#include "../../common/file_formats/mdm.h"

class idSurfaceMDM : public idSurface {
public:
	virtual void Draw();

	mdmSurface_t* GetMdmData() const;
	void SetMdmData( mdmSurface_t* data );
};

inline mdmSurface_t* idSurfaceMDM::GetMdmData() const {
	return ( mdmSurface_t* )data;
}

inline void idSurfaceMDM::SetMdmData( mdmSurface_t* data ) {
	this->data = ( surface_base_t* )data;
}

#endif
