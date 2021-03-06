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

#ifndef __idSurfaceMDS__
#define __idSurfaceMDS__

#include "Surface.h"
#include "../../common/file_formats/mds.h"

class idSurfaceMDS : public idSurface {
public:
	virtual void Draw();

	mdsSurface_t* GetMdsData() const;
	void SetMdsData( mdsSurface_t* data );

private:
	mdsSurface_t* data;
};

inline mdsSurface_t* idSurfaceMDS::GetMdsData() const {
	return data;
}

inline void idSurfaceMDS::SetMdsData( mdsSurface_t* data ) {
	this->data = data;
}

#endif
