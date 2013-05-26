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

#include "image.h"
#include "../../../common/endian.h"

struct qpic_t {
	int width, height;
	byte data[ 4 ];						// variably sized
};

void R_LoadPICMem( byte* data, byte** pic, int* width, int* height, int mode ) {
	qpic_t* qPic = ( qpic_t* ) data;
	int w = LittleLong( qPic->width );
	int h = LittleLong( qPic->height );
	if ( width ) {
		*width = w;
	}
	if ( height ) {
		*height = h;
	}
	*pic = R_ConvertImage8To32( qPic->data, w, h, mode );
}

void R_LoadPIC( const char* fileName, byte** pic, int* width, int* height, int mode ) {
	idList<byte> buffer;
	if ( FS_ReadFile( fileName, buffer ) <= 0 ) {
		*pic = NULL;
		return;
	}

	R_LoadPICMem( buffer.Ptr(), pic, width, height, mode );
}

void R_LoadPICTranslated( const char* fileName, const idSkinTranslation& translation, byte** pic, byte** picTop, byte** picBottom, int* width, int* height, int mode ) {
	idList<byte> buffer;
	if ( FS_ReadFile( fileName, buffer ) <= 0 ) {
		*pic = NULL;
		return;
	}

	qpic_t* qPic = ( qpic_t* ) buffer.Ptr();
	int w = LittleLong( qPic->width );
	int h = LittleLong( qPic->height );
	if ( width ) {
		*width = w;
	}
	if ( height ) {
		*height = h;
	}

	*picTop = new byte[ w * h * 4 ];
	*picBottom = new byte[ w * h * 4 ];

	R_ExtractTranslatedImages( translation, qPic->data, *picTop, *picBottom, w, h );

	*pic = R_ConvertImage8To32( qPic->data, w, h, mode );
}
