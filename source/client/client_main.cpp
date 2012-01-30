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

// HEADER FILES ------------------------------------------------------------

#include "client.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

Cvar* cl_inGameVideo;

// these two are not intended to be set directly
Cvar* clqh_name;
Cvar* clqh_color;

clientActive_t cl;
clientConnection_t clc;
clientStaticCommon_t* cls_common;

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

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	CL_SharedInit
//
//==========================================================================

void CL_SharedInit()
{
	cl_inGameVideo = Cvar_Get("r_inGameVideo", "1", CVAR_ARCHIVE);
}

//==========================================================================
//
//	CL_ScaledMilliseconds
//
//==========================================================================

int CL_ScaledMilliseconds()
{
	return Sys_Milliseconds() * com_timescale->value;
}

//==========================================================================
//
//	CL_CalcQuakeSkinTranslation
//
//==========================================================================

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

//==========================================================================
//
//	CL_CalcHexen2SkinTranslation
//
//==========================================================================

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
