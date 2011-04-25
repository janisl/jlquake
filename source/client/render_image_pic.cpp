//**************************************************************************
//**
//**	$Id$
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "render_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

struct qpic_t
{
	int			width, height;
	byte		data[4];			// variably sized
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
