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

#include "../client.h"
#include "../game/quake_hexen2/view.h"

static Cvar* cl_polyblend;

void V_SharedInit()
{
	cl_polyblend = Cvar_Get("cl_polyblend", "1", 0);

	if (GGameType & GAME_QuakeHexen)
	{
		VQH_Init();
	}
}

float CalcFov(float fovX, float width, float height)
{
	if (fovX < 1 || fovX > 179)
	{
		common->Error("Bad fov: %f", fovX);
	}

	float x = width / tan(fovX / 360 * M_PI);

	float a = atan(height / x);

	a = a * 360 / M_PI;

	return a;
}

void R_PolyBlend(refdef_t* fd, float* blendColour)
{
	if (!cl_polyblend->value)
	{
		return;
	}
	if (!blendColour[3])
	{
		return;
	}

	R_Draw2DQuad(fd->x, fd->y, fd->width, fd->height, NULL, 0, 0, 0, 0, blendColour[0], blendColour[1], blendColour[2], blendColour[3]);
}
