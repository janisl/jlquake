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

#include "../local.h"
#include "../../../common/Common.h"
#include "../../../common/endian.h"

void Mod_LoadSprite2Model( model_t* mod, void* buffer, int modfilelen ) {
	dsprite2_t* sprin = ( dsprite2_t* )buffer;
	dsprite2_t* sprout = ( dsprite2_t* )Mem_Alloc( modfilelen );

	sprout->ident = LittleLong( sprin->ident );
	sprout->version = LittleLong( sprin->version );
	sprout->numframes = LittleLong( sprin->numframes );

	if ( sprout->version != SPRITE2_VERSION ) {
		common->Error( "%s has wrong version number (%i should be %i)",
			mod->name, sprout->version, SPRITE2_VERSION );
	}

	if ( sprout->numframes > MAX_MD2_SKINS ) {
		common->Error( "%s has too many frames (%i > %i)",
			mod->name, sprout->numframes, MAX_MD2_SKINS );
	}

	// byte swap everything
	for ( int i = 0; i < sprout->numframes; i++ ) {
		sprout->frames[ i ].width = LittleLong( sprin->frames[ i ].width );
		sprout->frames[ i ].height = LittleLong( sprin->frames[ i ].height );
		sprout->frames[ i ].origin_x = LittleLong( sprin->frames[ i ].origin_x );
		sprout->frames[ i ].origin_y = LittleLong( sprin->frames[ i ].origin_y );
		Com_Memcpy( sprout->frames[ i ].name, sprin->frames[ i ].name, MAX_SP2_SKINNAME );
		mod->q2_skins[ i ] = R_FindImageFile( sprout->frames[ i ].name, true, true, GL_CLAMP );
	}

	mod->q2_extradata = sprout;
	mod->q2_extradatasize = modfilelen;
	mod->type = MOD_SPRITE2;
}

void Mod_FreeSprite2Model( model_t* mod ) {
	Mem_Free( mod->q2_extradata );
}

void R_AddSp2Surfaces( trRefEntity_t* e ) {
	if ( ( tr.currentEntity->e.renderfx & RF_THIRD_PERSON ) && !tr.viewParms.isPortal ) {
		return;
	}

	// don't even bother culling, because it's just a single
	// polygon without a surface cache

	dsprite2_t* psprite = ( dsprite2_t* )tr.currentModel->q2_extradata;
	R_AddDrawSurf( ( surfaceType_t* )psprite, tr.defaultShader, 0, false, false, ATI_TESS_NONE );
}

void RB_SurfaceSp2( dsprite2_t* psprite ) {
	vec3_t point;

	tr.currentEntity->e.frame %= psprite->numframes;

	dsp2_frame_t* frame = &psprite->frames[ tr.currentEntity->e.frame ];

	float* up = tr.viewParms.orient.axis[ 2 ];
	float* left = tr.viewParms.orient.axis[ 1 ];

	float alpha = 1.0F;
	if ( tr.currentEntity->e.renderfx & RF_TRANSLUCENT ) {
		alpha = tr.currentEntity->e.shaderRGBA[ 3 ] / 255.0;
	}

	if ( alpha != 1.0F ) {
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	} else   {
		GL_State( GLS_DEFAULT | GLS_ATEST_GE_80 );
	}

	qglColor4f( 1, 1, 1, alpha );

	GL_Bind( tr.currentModel->q2_skins[ tr.currentEntity->e.frame ] );

	qglBegin( GL_QUADS );

	qglTexCoord2f( 0, 1 );
	VectorScale( up, -frame->origin_y, point );
	VectorMA( point, frame->origin_x, left, point );
	qglVertex3fv( point );

	qglTexCoord2f( 0, 0 );
	VectorScale( up, frame->height - frame->origin_y, point );
	VectorMA( point, frame->origin_x, left, point );
	qglVertex3fv( point );

	qglTexCoord2f( 1, 0 );
	VectorScale( up, frame->height - frame->origin_y, point );
	VectorMA( point, -( frame->width - frame->origin_x ), left, point );
	qglVertex3fv( point );

	qglTexCoord2f( 1, 1 );
	VectorScale( up, -frame->origin_y, point );
	VectorMA( point, -( frame->width - frame->origin_x ), left, point );
	qglVertex3fv( point );

	qglEnd();

	qglColor4f( 1, 1, 1, 1 );
}
