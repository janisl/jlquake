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

#ifndef __idSurfaceBrush46__
#define __idSurfaceBrush46__

#include "Surface.h"
#include "shader.h"

class idSurfaceBrush46 : public idSurface {
public:
	int viewCount;							// if == tr.viewCount, already added
	shader_t* shader;
	int fogIndex;

	idSurfaceBrush46()
	: viewCount( 0 )
	, shader( NULL )
	, fogIndex( 0 )
	{}

	surface_base_t* GetBrush46Data();
	void SetBrush46Data( surface_base_t* data );
};

inline surface_base_t* idSurfaceBrush46::GetBrush46Data() {
	return data;
}

inline void idSurfaceBrush46::SetBrush46Data( surface_base_t* data ) {
	this->data = data;
}

#endif
