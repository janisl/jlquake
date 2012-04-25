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
//**
//**	Endianess handling, swapping 16bit and 32bit.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "qcommon.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

bool GBigEndian;

qint16 (* LittleShort)(qint16);
qint16 (* BigShort)(qint16);
qint32 (* LittleLong)(qint32);
qint32 (* BigLong)(qint32);
float (* LittleFloat)(float);
float (* BigFloat)(float);

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	ShortSwap
//
//==========================================================================

static qint16 ShortSwap(qint16 x)
{
	return ((quint16)x >> 8) |
		   ((quint16)x << 8);
}

//==========================================================================
//
//	ShortNoSwap
//
//==========================================================================

static qint16 ShortNoSwap(qint16 x)
{
	return x;
}

//==========================================================================
//
//	LongSwap
//
//==========================================================================

static qint32 LongSwap(qint32 x)
{
	return ((quint32)x >> 24) |
		   (((quint32)x >> 8) & 0xff00) |
		   (((quint32)x << 8) & 0xff0000) |
		   ((quint32)x << 24);
}

//==========================================================================
//
//	LongNoSwap
//
//==========================================================================

static qint32 LongNoSwap(qint32 x)
{
	return x;
}

//==========================================================================
//
//	FloatSwap
//
//==========================================================================

static float FloatSwap(float x)
{
	union { float f; qint32 l; } a;
	a.f = x;
	a.l = LongSwap(a.l);
	return a.f;
}

//==========================================================================
//
//	FloatNoSwap
//
//==========================================================================

static float FloatNoSwap(float x)
{
	return x;
}

//==========================================================================
//
//	Com_InitByteOrder
//
//==========================================================================

void Com_InitByteOrder()
{
	quint8 swaptest[2] = {1, 0};

	// set the byte swapping variables in a portable manner
	if (*(qint16*)swaptest == 1)
	{
		GBigEndian = false;
		BigShort = ShortSwap;
		LittleShort = ShortNoSwap;
		BigLong = LongSwap;
		LittleLong = LongNoSwap;
		BigFloat = FloatSwap;
		LittleFloat = FloatNoSwap;
	}
	else
	{
		GBigEndian = true;
		BigShort = ShortNoSwap;
		LittleShort = ShortSwap;
		BigLong = LongNoSwap;
		LittleLong = LongSwap;
		BigFloat = FloatNoSwap;
		LittleFloat = FloatSwap;
	}
}
