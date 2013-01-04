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

// HEADER FILES ------------------------------------------------------------

#include "../local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

struct qpic_t
{
	int width, height;
	byte data[4];					// variably sized
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	R_LoadPICMem
//
//==========================================================================

void R_LoadPICMem(byte* Data, byte** Pic, int* Width, int* Height, byte* TransPixels, int Mode)
{
	qpic_t* QPic = (qpic_t*)Data;
	int w = LittleLong(QPic->width);
	int h = LittleLong(QPic->height);
	if (Width)
	{
		*Width = w;
	}
	if (Height)
	{
		*Height = h;
	}

	// HACK HACK HACK --- we need to keep the bytes for the translatable
	// player picture just for the menu configuration dialog
	if (TransPixels)
	{
		Com_Memcpy(TransPixels, QPic->data, w * h);
	}

	*Pic = R_ConvertImage8To32(QPic->data, w, h, Mode);
}

//==========================================================================
//
//	R_LoadPIC
//
//==========================================================================

void R_LoadPIC(const char* FileName, byte** Pic, int* Width, int* Height, byte* TransPixels, int Mode)
{
	Array<byte> Buffer;
	if (FS_ReadFile(FileName, Buffer) <= 0)
	{
		*Pic = NULL;
		return;
	}

	R_LoadPICMem(Buffer.Ptr(), Pic, Width, Height, TransPixels, Mode);
}
