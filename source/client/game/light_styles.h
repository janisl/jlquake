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

#ifndef __LIGHT_STYLES_H__
#define __LIGHT_STYLES_H__

#include "../renderer/public.h"

#define MAX_STYLESTRING 64

struct clightstyle_t
{
	int length;
	int rate;
	float value[3];
	char mapStr[MAX_STYLESTRING];
	float map[MAX_STYLESTRING];
};

extern clightstyle_t cl_lightstyle[MAX_LIGHTSTYLES];

void CL_ClearLightStyles();
void CL_SetLightStyle(int i, const char* map);
void CL_RunLightStyles();
void CL_AddLightStyles();

#endif
