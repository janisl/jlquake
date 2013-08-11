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

#ifndef __idSurfaceMD2__
#define __idSurfaceMD2__

#include "Surface.h"
#include "../../common/math/Vec2.h"
#include "shader.h"

struct mmd2_t {
	int framesize;				// byte size of each frame

	int num_skins;
	int numVertexes;
	int numIndexes;
	int num_frames;

	idVec2* texCoords;
	glIndex_t* indexes;
	byte* frames;
};

class idSurfaceMD2 : public idSurface {
public:
	mmd2_t header;

	idSurfaceMD2();

	virtual void Draw();
};

#endif
