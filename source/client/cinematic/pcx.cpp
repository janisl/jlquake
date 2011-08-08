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

#include "../client.h"
#include "local.h"
#include "../renderer/local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	QCinematicPcx::~QCinematicPcx
//
//==========================================================================

QCinematicPcx::~QCinematicPcx()
{
	if (OutputFrame)
	{
		delete[] OutputFrame;
		OutputFrame = NULL;
	}
}

//==========================================================================
//
//	QCinematicPcx::Open
//
//==========================================================================

bool QCinematicPcx::Open(const char* FileName)
{
	String::Cpy(Name, FileName);
	byte* pic;
	byte* palette;
	R_LoadPCX(FileName, &pic, &palette, &Width, &Height);
	if (!pic)
	{
		Log::write("%s not found.\n", Name);
		return false;
	}

	OutputFrame = new byte[Width * Height * 4];
	for (int i = 0; i < Width * Height; i++)
	{
		OutputFrame[i * 4 + 0] = palette[pic[i] * 3 + 0];
		OutputFrame[i * 4 + 1] = palette[pic[i] * 3 + 1];
		OutputFrame[i * 4 + 2] = palette[pic[i] * 3 + 2];
		OutputFrame[i * 4 + 3] = 255;
	}

	delete[] pic;
	delete[] palette;
	return true;
}

//==========================================================================
//
//	QCinematicPcx::Update
//
//==========================================================================

bool QCinematicPcx::Update(int)
{
	return true;
}

//==========================================================================
//
//	QCinematicPcx::GetCinematicTime
//
//==========================================================================

int QCinematicPcx::GetCinematicTime() const
{
	return 0;
}

//==========================================================================
//
//	QCinematicPcx::Reset
//
//==========================================================================

void QCinematicPcx::Reset()
{
}
