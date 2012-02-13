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

#include "client.h"

Cvar* cl_inGameVideo;

Cvar* clqh_nolerp;

// these two are not intended to be set directly
Cvar* clqh_name;
Cvar* clqh_color;

clientActive_t cl;
clientConnection_t clc;
clientStatic_t cls;

byte* playerTranslation;

int color_offsets[MAX_PLAYER_CLASS] =
{
	2 * 14 * 256,
	0,
	1 * 14 * 256,
	2 * 14 * 256,
	2 * 14 * 256,
	2 * 14 * 256
};

int bitcounts[32];	/// just for protocol profiling

void CL_SharedInit()
{
	cl_inGameVideo = Cvar_Get("r_inGameVideo", "1", CVAR_ARCHIVE);
}

int CL_ScaledMilliseconds()
{
	return Sys_Milliseconds() * com_timescale->value;
}

void CL_CalcQuakeSkinTranslation(int top, int bottom, byte* translate)
{
	enum
	{
		// soldier uniform colors
		TOP_RANGE = 16,
		BOTTOM_RANGE = 96
	};

	top = (top < 0) ? 0 : ((top > 13) ? 13 : top);
	bottom = (bottom < 0) ? 0 : ((bottom > 13) ? 13 : bottom);
	top *= 16;
	bottom *= 16;

	for (int i = 0; i < 256; i++)
	{
		translate[i] = i;
	}

	for (int i = 0; i < 16; i++)
	{
		//	The artists made some backwards ranges. sigh.
		if (top < 128)
		{
			translate[TOP_RANGE + i] = top + i;
		}
		else
		{
			translate[TOP_RANGE + i] = top + 15 - i;
		}

		if (bottom < 128)
		{
			translate[BOTTOM_RANGE + i] = bottom + i;
		}
		else
		{
			translate[BOTTOM_RANGE + i] = bottom + 15 - i;
		}
	}
}

void CL_CalcHexen2SkinTranslation(int top, int bottom, int playerClass, byte* translate)
{
	for (int i = 0; i < 256; i++)
	{
		translate[i] = i;
	}

	if (top > 10)
	{
		top = 0;
	}
	if (bottom > 10)
	{
		bottom = 0;
	}

	top -= 1;
	bottom -= 1;

	byte* colorA = playerTranslation + 256 + color_offsets[playerClass - 1];
	byte* colorB = colorA + 256;
	byte* sourceA = colorB + 256 + top * 256;
	byte* sourceB = colorB + 256 + bottom * 256;
	for (int i = 0; i < 256; i++, colorA++, colorB++, sourceA++, sourceB++)
	{
		if (top >= 0 && *colorA != 255)
		{
			translate[i] = *sourceA;
		}
		if (bottom >= 0 && *colorB != 255)
		{
			translate[i] = *sourceB;
		}
	}
}

//	Determines the fraction between the last two messages that the objects
// should be put at.
float CLQH_LerpPoint()
{
	float f = cl.qh_mtime[0] - cl.qh_mtime[1];
	
	if (!f || clqh_nolerp->value || cls.qh_timedemo || CL_IsServerActive())
	{
		cl.qh_serverTimeFloat = cl.qh_mtime[0];
		cl.serverTime = (int)(cl.qh_serverTimeFloat * 1000);
		return 1;
	}
		
	if (f > 0.1)
	{
		// dropped packet, or start of demo
		cl.qh_mtime[1] = cl.qh_mtime[0] - 0.1;
		f = 0.1;
	}
	float frac = (cl.qh_serverTimeFloat - cl.qh_mtime[1]) / f;
	if (frac < 0)
	{
		if (frac < -0.01)
		{
			cl.qh_serverTimeFloat = cl.qh_mtime[1];
			cl.serverTime = (int)(cl.qh_serverTimeFloat * 1000);
		}
		frac = 0;
	}
	else if (frac > 1)
	{
		if (frac > 1.01)
		{
			cl.qh_serverTimeFloat = cl.qh_mtime[0];
			cl.serverTime = (int)(cl.qh_serverTimeFloat * 1000);
		}
		frac = 1;
	}
	return frac;
}
