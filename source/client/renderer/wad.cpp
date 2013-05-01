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

#include "local.h"
#include "../../common/Common.h"
#include "../../common/strings.h"
#include "../../common/endian.h"

struct wadinfo_t {
	char identification[ 4 ];				// should be WAD2 or 2DAW
	int numlumps;
	int infotableofs;
};

struct lumpinfo_t {
	int filepos;
	int disksize;
	int size;							// uncompressed
	char type;
	char compression;
	char pad1, pad2;
	char name[ 16 ];						// must be null terminated
};

static int wad_numlumps;
static lumpinfo_t* wad_lumps;
static idList<byte>  wad_base;

//	Lowercases name and pads with terminating 0 to the length of lumpinfo_t->name.
//	Used so lumpname lookups can proceed rapidly by comparing 4 chars at a time
//	Can safely be performed in place.
static void CleanupName( const char* in, char* out ) {
	int i;
	for ( i = 0; i < 16; i++ ) {
		int c = in[ i ];
		if ( !c ) {
			break;
		}

		if ( c >= 'A' && c <= 'Z' ) {
			c += ( 'a' - 'A' );
		}
		out[ i ] = c;
	}

	for (; i < 16; i++ ) {
		out[ i ] = 0;
	}
}

void R_LoadWadFile() {
	if ( FS_ReadFile( "gfx.wad", wad_base ) <= 0 ) {
		common->FatalError( "W_LoadWadFile: couldn't load gfx.wad" );
	}

	wadinfo_t* header = ( wadinfo_t* )wad_base.Ptr();

	if ( header->identification[ 0 ] != 'W' ||
		 header->identification[ 1 ] != 'A' ||
		 header->identification[ 2 ] != 'D' ||
		 header->identification[ 3 ] != '2' ) {
		common->FatalError( "Wad file gfx.wad doesn't have WAD2 id\n" );
	}

	wad_numlumps = LittleLong( header->numlumps );
	int infotableofs = LittleLong( header->infotableofs );
	wad_lumps = ( lumpinfo_t* )&wad_base[ infotableofs ];

	lumpinfo_t* lump_p = wad_lumps;
	for ( int i = 0; i < wad_numlumps; i++, lump_p++ ) {
		lump_p->filepos = LittleLong( lump_p->filepos );
		lump_p->size = LittleLong( lump_p->size );
		CleanupName( lump_p->name, lump_p->name );
	}
}

void* R_GetWadLumpByName( const char* name ) {
	char clean[ 16 ];
	CleanupName( name, clean );

	lumpinfo_t* lump_p = wad_lumps;
	for ( int i = 0; i < wad_numlumps; i++, lump_p++ ) {
		if ( !String::Cmp( clean, lump_p->name ) ) {
			return &wad_base[ lump_p->filepos ];
		}
	}

	common->Error( "R_GetWadLumpByName: %s not found", name );
	return NULL;
}

static qhandle_t R_PicFromWad( const char* name, GLenum WrapClampMode ) {
	byte* qpic = ( byte* )R_GetWadLumpByName( name );

	int width;
	int height;
	byte* pic;
	R_LoadPICMem( qpic, &pic, &width, &height );

	image_t* image = R_CreateImage( va( "gfx/wad/%s", name ), pic, width, height, false, false, WrapClampMode );

	delete[] pic;

	return R_Build2DShaderFromImage( image )->index;
}

qhandle_t R_ShaderFromWad( const char* name ) {
	return R_PicFromWad( name, GL_CLAMP );
}

qhandle_t R_ShaderFromWadRepeat( const char* name ) {
	return R_PicFromWad( name, GL_REPEAT );
}
