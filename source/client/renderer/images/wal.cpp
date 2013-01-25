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
#include "../../../common/endian.h"

// MACROS ------------------------------------------------------------------

#define MIPLEVELS   4

// TYPES -------------------------------------------------------------------

struct miptex_t {
	char name[ 32 ];
	unsigned width, height;
	unsigned offsets[ MIPLEVELS ];			// four mip maps stored
	char animname[ 32 ];					// next frame in animation chain
	int flags;
	int contents;
	int value;
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
//	R_LoadWAL
//
//==========================================================================

void R_LoadWAL( const char* FileName, byte** Pic, int* WidthPtr, int* HeightPtr ) {
	*Pic = NULL;

	miptex_t* mt;
	FS_ReadFile( FileName, ( void** )&mt );
	if ( !mt ) {
		return;
	}

	int width = LittleLong( mt->width );
	int height = LittleLong( mt->height );
	int ofs = LittleLong( mt->offsets[ 0 ] );
	if ( WidthPtr ) {
		*WidthPtr = width;
	}
	if ( HeightPtr ) {
		*HeightPtr = height;
	}

	*Pic = R_ConvertImage8To32( ( byte* )mt + ofs, width, height, IMG8MODE_Normal );

	FS_FreeFile( ( void* )mt );
}
