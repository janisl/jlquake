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

#ifndef __idSurface__
#define __idSurface__

#include "../../common/qcommon.h"

// any changes in surfaceType must be mirrored in rb_surfaceTable[]
enum surfaceType_t
{
	SF_BAD,
	SF_SKIP,				// ignore
	SF_FACE_Q1,
	SF_FACE_Q2,
	SF_SKYBOX_Q2,
	SF_FACE,
	SF_GRID,
	SF_TRIANGLES,
	SF_FOLIAGE,
	SF_POLY,
	SF_SPR,
	SF_SP2,
	SF_MDL,
	SF_MD2,
	SF_MD3,
	SF_MD4,
	SF_MDC,
	SF_MDS,
	SF_MDM,
	SF_FLARE,
	SF_ENTITY,				// beams, rails, lightning, etc that can be determined by entity
	SF_POLYBUFFER,
	SF_DECAL,				// ydnar: decal surfaces
	SF_PARTICLES,

	SF_NUM_SURFACE_TYPES,
	SF_MAX = 0x7fffffff			// ensures that sizeof( surfaceType_t ) == sizeof( int )
};

struct surface_base_t {
	surfaceType_t surfaceType;
};

class idSurface {
public:
	surface_base_t* data;					// any of srf*_t
	
	idSurface()
	: data(NULL)
	{
	}
	virtual ~idSurface();
};

/* Pending:
	SF_BAD,
	SF_DECAL,				// ydnar: decal surfaces
 */
#endif
