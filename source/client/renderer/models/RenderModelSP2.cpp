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

#include "RenderModelSP2.h"
#include "../../../common/Common.h"
#include "../../../common/endian.h"

idRenderModelSP2::idRenderModelSP2() {
}

idRenderModelSP2::~idRenderModelSP2() {
}

bool idRenderModelSP2::Load( idList<byte>& buffer, idSkinTranslation* skinTranslation ) {
	dsprite2_t* sprin = ( dsprite2_t* )buffer.Ptr();
	dsprite2_t* sprout = ( dsprite2_t* )Mem_Alloc( buffer.Num() );

	sprout->ident = LittleLong( sprin->ident );
	sprout->version = LittleLong( sprin->version );
	sprout->numframes = LittleLong( sprin->numframes );

	if ( sprout->version != SPRITE2_VERSION ) {
		common->Error( "%s has wrong version number (%i should be %i)",
			name, sprout->version, SPRITE2_VERSION );
	}

	if ( sprout->numframes > MAX_MD2_SKINS ) {
		common->Error( "%s has too many frames (%i > %i)",
			name, sprout->numframes, MAX_MD2_SKINS );
	}

	// byte swap everything
	for ( int i = 0; i < sprout->numframes; i++ ) {
		sprout->frames[ i ].width = LittleLong( sprin->frames[ i ].width );
		sprout->frames[ i ].height = LittleLong( sprin->frames[ i ].height );
		sprout->frames[ i ].origin_x = LittleLong( sprin->frames[ i ].origin_x );
		sprout->frames[ i ].origin_y = LittleLong( sprin->frames[ i ].origin_y );
		Com_Memcpy( sprout->frames[ i ].name, sprin->frames[ i ].name, MAX_SP2_SKINNAME );
		image_t* image = R_FindImageFile( sprout->frames[ i ].name, true, true, GL_CLAMP );
		q2_skins_shader[ i ] = R_BuildSp2Shader( image );
	}

	q2_sp2 = new idSurfaceSP2;
	q2_sp2->data = ( surface_base_t* )sprout;
	q2_extradatasize = buffer.Num();
	type = MOD_SPRITE2;
	return true;
}
