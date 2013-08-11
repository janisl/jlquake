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

#include "SurfaceSkyBoxQ2.h"
#include "sky.h"

idSurfaceSkyBoxQ2 skyBoxQ2Surface;

static surface_base_t q2SkySurface = { SF_SKYBOX_Q2 };

idSurfaceSkyBoxQ2::idSurfaceSkyBoxQ2() {
	data = &q2SkySurface;
}

void idSurfaceSkyBoxQ2::Draw() {
	R_DrawSkyBoxQ2( &data->surfaceType );
}
