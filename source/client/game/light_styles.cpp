//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../client.h"

clightstyle_t cl_lightstyle[MAX_LIGHTSTYLES];
int lastofs;

void CL_ClearLightStyles()
{
	Com_Memset(cl_lightstyle, 0, sizeof(cl_lightstyle));
	lastofs = -1;
}

void CL_SetLightStyle(int i, const char* s)
{
	if (i >= MAX_LIGHTSTYLES)
	{
		throw DropException("svc_lightstyle > MAX_LIGHTSTYLES");
	}

	int j = String::Length(s);
	if (j >= MAX_STYLESTRING)
	{
		throw DropException(va("svc_lightstyle length=%i", j));
	}

	cl_lightstyle[i].length = j;
	String::Cpy(cl_lightstyle[i].mapStr,  s);

	if (GGameType & GAME_Quake2)
	{
		for (int k = 0; k < j; k++)
		{
			cl_lightstyle[i].map[k] = (float)(s[k] - 'a') / (float)('m' - 'a');
		}
	}
}
