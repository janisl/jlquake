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

#include "RenderModelSPR.h"
#include "../../../common/Common.h"
#include "../../../common/endian.h"
#include "../../../common/strings.h"
#include "model.h"

idRenderModelSPR::idRenderModelSPR() {
}

idRenderModelSPR::~idRenderModelSPR() {
}

static void* Mod_LoadSpriteFrame( void* pin, msprite1frame_t** ppframe, int framenum ) {
	dsprite1frame_t* pinframe = ( dsprite1frame_t* )pin;

	int width = LittleLong( pinframe->width );
	int height = LittleLong( pinframe->height );
	int size = width * height;

	msprite1frame_t* pspriteframe = new msprite1frame_t;

	Com_Memset( pspriteframe, 0, sizeof ( msprite1frame_t ) );

	*ppframe = pspriteframe;

	pspriteframe->width = width;
	pspriteframe->height = height;
	int origin[ 2 ];
	origin[ 0 ] = LittleLong( pinframe->origin[ 0 ] );
	origin[ 1 ] = LittleLong( pinframe->origin[ 1 ] );

	pspriteframe->up = origin[ 1 ];
	pspriteframe->down = origin[ 1 ] - height;
	pspriteframe->left = origin[ 0 ];
	pspriteframe->right = width + origin[ 0 ];

	char name[ 64 ];
	String::StripExtension2( loadmodel->name, name, sizeof( name ) );
	String::FixPath( name );
	String::Cat( name, sizeof( name ), va( "_%i", framenum ) );
	byte* pic32 = R_ConvertImage8To32( ( byte* )( pinframe + 1 ), width, height, IMG8MODE_Normal );
	image_t* image = R_CreateImage( name, pic32, width, height, true, true, GL_CLAMP );
	delete[] pic32;
	pspriteframe->shader = R_BuildSprShader( image );

	return ( void* )( ( byte* )pinframe + sizeof ( dsprite1frame_t ) + size );
}

static void* Mod_LoadSpriteGroup( void* pin, msprite1frame_t** ppframe, int framenum ) {
	dsprite1group_t* pingroup = ( dsprite1group_t* )pin;

	int numframes = LittleLong( pingroup->numframes );

	msprite1group_t* pspritegroup = ( msprite1group_t* )Mem_Alloc( sizeof ( msprite1group_t ) +
		( numframes - 1 ) * sizeof ( pspritegroup->frames[ 0 ] ) );

	pspritegroup->numframes = numframes;

	*ppframe = ( msprite1frame_t* )pspritegroup;

	dsprite1interval_t* pin_intervals = ( dsprite1interval_t* )( pingroup + 1 );

	float* poutintervals = new float[ numframes ];

	pspritegroup->intervals = poutintervals;

	for ( int i = 0; i < numframes; i++ ) {
		*poutintervals = LittleFloat( pin_intervals->interval );
		if ( *poutintervals <= 0.0 ) {
			common->FatalError( "Mod_LoadSpriteGroup: interval<=0" );
		}

		poutintervals++;
		pin_intervals++;
	}

	void* ptemp = ( void* )pin_intervals;

	for ( int i = 0; i < numframes; i++ ) {
		ptemp = Mod_LoadSpriteFrame( ptemp, &pspritegroup->frames[ i ], framenum * 100 + i );
	}

	return ptemp;
}

bool idRenderModelSPR::Load( idList<byte>& buffer, idSkinTranslation* skinTranslation ) {
	dsprite1_t* pin = ( dsprite1_t* )buffer.Ptr();

	int version = LittleLong( pin->version );
	if ( version != SPRITE1_VERSION ) {
		common->FatalError( "%s has wrong version number "
							"(%i should be %i)", name, version, SPRITE1_VERSION );
	}

	int numframes = LittleLong( pin->numframes );

	idSurfaceSPR* psprite = new idSurfaceSPR;

	q1_spr = psprite;

	psprite->header.type = LittleLong( pin->type );
	psprite->header.maxwidth = LittleLong( pin->width );
	psprite->header.maxheight = LittleLong( pin->height );
	psprite->header.beamlength = LittleFloat( pin->beamlength );
	q1_synctype = ( synctype_t )LittleLong( pin->synctype );
	psprite->header.numframes = numframes;
	psprite->header.frames = new msprite1framedesc_t[ numframes ];

	q1_mins[ 0 ] = q1_mins[ 1 ] = -psprite->header.maxwidth / 2;
	q1_maxs[ 0 ] = q1_maxs[ 1 ] = psprite->header.maxwidth / 2;
	q1_mins[ 2 ] = -psprite->header.maxheight / 2;
	q1_maxs[ 2 ] = psprite->header.maxheight / 2;

	//
	// load the frames
	//
	if ( numframes < 1 ) {
		common->FatalError( "Mod_LoadSpriteModel: Invalid # of frames: %d\n", numframes );
	}

	q1_numframes = numframes;

	dsprite1frametype_t* pframetype = ( dsprite1frametype_t* )( pin + 1 );

	for ( int i = 0; i < numframes; i++ ) {
		sprite1frametype_t frametype = ( sprite1frametype_t )LittleLong( pframetype->type );
		psprite->header.frames[ i ].type = frametype;

		if ( frametype == SPR_SINGLE ) {
			pframetype = ( dsprite1frametype_t* )Mod_LoadSpriteFrame( pframetype + 1,
				&psprite->header.frames[ i ].frameptr, i );
		} else {
			pframetype = ( dsprite1frametype_t* )Mod_LoadSpriteGroup( pframetype + 1,
				&psprite->header.frames[ i ].frameptr, i );
		}
	}

	type = MOD_SPRITE;
	return true;
}
