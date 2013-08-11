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

#ifndef __idSurfaceMD4__
#define __idSurfaceMD4__

#include "Surface.h"
#include "../../common/file_formats/md4.h"

class idSurfaceMD4 : public idSurface {
public:
	virtual void Draw();

	md4Surface_t* GetMd4Data() const;
	void SetMd4Data( md4Surface_t* data );

private:
	md4Surface_t* data;
};

inline md4Surface_t* idSurfaceMD4::GetMd4Data() const {
	return data;
}

inline void idSurfaceMD4::SetMd4Data( md4Surface_t* data ) {
	this->data = data;
}

#endif
