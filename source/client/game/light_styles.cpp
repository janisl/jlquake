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
//
// light animations
// 'm' is normal light, 'a' is no light, 'z' is double bright

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
	else
	{
		for (int k = 0; k < j; k++)
		{
			cl_lightstyle[i].map[k] = s[k];
		}
	}
}

void CL_RunLightStyles()
{
	int locusHz[3];
	locusHz[0] = cl_common->serverTime / 100;
	locusHz[1] = cl_common->serverTime / 50;
	locusHz[2] = cl_common->serverTime * 3 / 100;

	if (GGameType & GAME_Quake2)
	{
		if (locusHz[0] == lastofs)
		{
			return;
		}
		lastofs = locusHz[0];
	}

	clightstyle_t* ls = cl_lightstyle;
	for (int i = 0; i < MAX_LIGHTSTYLES; i++, ls++)
	{
		if (!ls->length)
		{
			ls->value[0] = ls->value[1] = ls->value[2] = 1.0;
			continue;
		}
		if (GGameType & GAME_Quake)
		{
			int k = locusHz[0] % ls->length;
			k = ls->map[k] - 'a';
			k = k*22;
			ls->value[0] = k / 256.0;
			ls->value[1] = k / 256.0;
			ls->value[2] = k / 256.0;
		}
		else if (GGameType & GAME_Hexen2)
		{
			int c = ls->mapStr[0];
			int v;
			if (c == '1' || c == '2' || c == '3')
			{
				// Explicit anim rate
				if (ls->length == 1)
				{
					// Bad style def
					ls->value[0] = ls->value[1] = ls->value[2] = 1;
					continue;
				}
				v = locusHz[c - '1'] % (ls->length - 1);
				v++;
			}
			else
			{
				// Default anim rate (10 Hz)
				v = locusHz[0] % ls->length;
			}
			ls->value[0] = ls->value[1] = ls->value[2] = (ls->map[v] - 'a') * 22 / 256.0;
		}
		else
		{
			if (ls->length == 1)
			{
				ls->value[0] = ls->value[1] = ls->value[2] = ls->map[0];
			}
			else
			{
				ls->value[0] = ls->value[1] = ls->value[2] = ls->map[locusHz[0] % ls->length];
			}
		}
	}	
}

void CL_AddLightStyles()
{
	clightstyle_t* ls = cl_lightstyle;
	for (int i = 0; i < MAX_LIGHTSTYLES; i++, ls++)
	{
		R_AddLightStyleToScene(i, ls->value[0], ls->value[1], ls->value[2]);
	}
}
