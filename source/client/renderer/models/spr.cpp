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
	sprintf( name, "%s_%i", loadmodel->name, framenum );
	byte* pic32 = R_ConvertImage8To32( ( byte* )( pinframe + 1 ), width, height, IMG8MODE_Normal );
	pspriteframe->gl_texture = R_CreateImage( name, pic32, width, height, true, true, GL_CLAMP, false );
	delete[] pic32;

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

void Mod_LoadSpriteModel( model_t* mod, void* buffer ) {
	dsprite1_t* pin = ( dsprite1_t* )buffer;

	int version = LittleLong( pin->version );
	if ( version != SPRITE1_VERSION ) {
		common->FatalError( "%s has wrong version number "
							"(%i should be %i)", mod->name, version, SPRITE1_VERSION );
	}

	int numframes = LittleLong( pin->numframes );

	int size = sizeof ( msprite1_t ) + ( numframes - 1 ) * sizeof ( msprite1framedesc_t );

	msprite1_t* psprite = ( msprite1_t* )Mem_Alloc( size );

	mod->q1_cache = psprite;

	psprite->surfaceType = SF_SPR;
	psprite->type = LittleLong( pin->type );
	psprite->maxwidth = LittleLong( pin->width );
	psprite->maxheight = LittleLong( pin->height );
	psprite->beamlength = LittleFloat( pin->beamlength );
	mod->q1_synctype = ( synctype_t )LittleLong( pin->synctype );
	psprite->numframes = numframes;

	mod->q1_mins[ 0 ] = mod->q1_mins[ 1 ] = -psprite->maxwidth / 2;
	mod->q1_maxs[ 0 ] = mod->q1_maxs[ 1 ] = psprite->maxwidth / 2;
	mod->q1_mins[ 2 ] = -psprite->maxheight / 2;
	mod->q1_maxs[ 2 ] = psprite->maxheight / 2;

	//
	// load the frames
	//
	if ( numframes < 1 ) {
		common->FatalError( "Mod_LoadSpriteModel: Invalid # of frames: %d\n", numframes );
	}

	mod->q1_numframes = numframes;

	dsprite1frametype_t* pframetype = ( dsprite1frametype_t* )( pin + 1 );

	for ( int i = 0; i < numframes; i++ ) {
		sprite1frametype_t frametype = ( sprite1frametype_t )LittleLong( pframetype->type );
		psprite->frames[ i ].type = frametype;

		if ( frametype == SPR_SINGLE ) {
			pframetype = ( dsprite1frametype_t* )Mod_LoadSpriteFrame( pframetype + 1,
				&psprite->frames[ i ].frameptr, i );
		} else   {
			pframetype = ( dsprite1frametype_t* )Mod_LoadSpriteGroup( pframetype + 1,
				&psprite->frames[ i ].frameptr, i );
		}
	}

	mod->type = MOD_SPRITE;
}

void Mod_FreeSpriteModel( model_t* mod ) {
	msprite1_t* psprite = ( msprite1_t* )mod->q1_cache;
	for ( int i = 0; i < psprite->numframes; i++ ) {
		if ( psprite->frames[ i ].type == SPR_SINGLE ) {
			delete psprite->frames[ i ].frameptr;
		} else   {
			msprite1group_t* pspritegroup = ( msprite1group_t* )psprite->frames[ i ].frameptr;
			for ( int j = 0; j < pspritegroup->numframes; j++ ) {
				delete pspritegroup->frames[ j ];
			}
			delete[] pspritegroup->intervals;
			Mem_Free( pspritegroup );
		}
	}
	Mem_Free( psprite );
}

static msprite1frame_t* R_GetSpriteFrame( msprite1_t* psprite, trRefEntity_t* currententity ) {
	int frame = currententity->e.frame;

	if ( frame >= psprite->numframes || frame < 0 ) {
		common->Printf( "R_DrawSprite: no such frame %d\n", frame );
		frame = 0;
	}

	if ( psprite->frames[ frame ].type == SPR_SINGLE ) {
		return psprite->frames[ frame ].frameptr;
	} else   {
		msprite1group_t* pspritegroup = ( msprite1group_t* )psprite->frames[ frame ].frameptr;
		float* pintervals = pspritegroup->intervals;
		int numframes = pspritegroup->numframes;
		float fullinterval = pintervals[ numframes - 1 ];

		float time = tr.refdef.floatTime + currententity->e.syncBase;

		// when loading in Mod_LoadSpriteGroup, we guaranteed all interval values
		// are positive, so we don't have to worry about division by 0
		float targettime = time - ( ( int )( time / fullinterval ) ) * fullinterval;

		int i;
		for ( i = 0; i < ( numframes - 1 ); i++ ) {
			if ( pintervals[ i ] > targettime ) {
				break;
			}
		}

		return pspritegroup->frames[ i ];
	}
}

void R_AddSprSurfaces( trRefEntity_t* e, int forcedSortIndex ) {
	if ( ( tr.currentEntity->e.renderfx & RF_THIRD_PERSON ) && !tr.viewParms.isPortal ) {
		return;
	}

	msprite1_t* psprite = ( msprite1_t* )tr.currentModel->q1_cache;
	R_AddDrawSurf( ( surfaceType_t* )psprite, tr.spriteDummyShader, 0, false, false, ATI_TESS_NONE, forcedSortIndex );
}

void RB_SurfaceSpr( msprite1_t* psprite ) {
	vec3_t point;

	// don't even bother culling, because it's just a single
	// polygon without a surface cache

	if ( backEnd.currentEntity->e.renderfx & RF_WATERTRANS ) {
		qglColor4f( 1, 1, 1, r_wateralpha->value );
	} else   {
		qglColor3f( 1, 1, 1 );
	}

	GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	msprite1frame_t* frame = R_GetSpriteFrame( psprite, backEnd.currentEntity );

	vec3_t up;
	vec3_t right;
	if ( psprite->type == SPR_FACING_UPRIGHT ) {
		// generate the sprite's axes, with vup straight up in worldspace, and
		// r_spritedesc.vright perpendicular to backEnd.viewParms.orient.origin.
		// This will not work if the view direction is very close to straight up or
		// down, because the cross product will be between two nearly parallel
		// vectors and starts to approach an undefined state, so we don't draw if
		// the two vectors are less than 1 degree apart
		vec3_t tvec;
		tvec[ 0 ] = -backEnd.viewParms.orient.origin[ 0 ];
		tvec[ 1 ] = -backEnd.viewParms.orient.origin[ 1 ];
		tvec[ 2 ] = -backEnd.viewParms.orient.origin[ 2 ];
		VectorNormalize( tvec );
		float dot = tvec[ 2 ];		// same as DotProduct (tvec, r_spritedesc.vup) because
									//  r_spritedesc.vup is 0, 0, 1
		if ( ( dot > 0.999848 ) || ( dot < -0.999848 ) ) {	// cos(1 degree) = 0.999848
			return;
		}
		up[ 0 ] = 0;
		up[ 1 ] = 0;
		up[ 2 ] = 1;
		right[ 0 ] = tvec[ 1 ];		// CrossProduct(r_spritedesc.vup, -backEnd.viewParms.orient.origin,
		right[ 1 ] = -tvec[ 0 ];	//              r_spritedesc.vright)
		right[ 2 ] = 0;
		VectorNormalize( right );
	} else if ( psprite->type == SPR_VP_PARALLEL )     {
		// generate the sprite's axes, completely parallel to the viewplane. There
		// are no problem situations, because the sprite is always in the same
		// position relative to the viewer
		VectorCopy( backEnd.viewParms.orient.axis[ 2 ], up );
		VectorSubtract( vec3_origin, backEnd.viewParms.orient.axis[ 1 ], right );
	} else if ( psprite->type == SPR_VP_PARALLEL_UPRIGHT )     {
		// generate the sprite's axes, with vup straight up in worldspace, and
		// r_spritedesc.vright parallel to the viewplane.
		// This will not work if the view direction is very close to straight up or
		// down, because the cross product will be between two nearly parallel
		// vectors and starts to approach an undefined state, so we don't draw if
		// the two vectors are less than 1 degree apart
		float dot = backEnd.viewParms.orient.axis[ 0 ][ 2 ];	// same as DotProduct (vpn, r_spritedesc.vup) because
		//  r_spritedesc.vup is 0, 0, 1
		if ( ( dot > 0.999848 ) || ( dot < -0.999848 ) ) {	// cos(1 degree) = 0.999848
			return;
		}
		up[ 0 ] = 0;
		up[ 1 ] = 0;
		up[ 2 ] = 1;
		right[ 0 ] = backEnd.viewParms.orient.axis[ 0 ][ 1 ];	// CrossProduct (r_spritedesc.vup, vpn,
		right[ 1 ] = -backEnd.viewParms.orient.axis[ 0 ][ 0 ];	//  r_spritedesc.vright)
		right[ 2 ] = 0;
		VectorNormalize( right );
	} else if ( psprite->type == SPR_ORIENTED )     {
		// generate the sprite's axes, according to the sprite's world orientation
		VectorCopy( backEnd.currentEntity->e.axis[ 2 ], up );
		VectorSubtract( vec3_origin, backEnd.currentEntity->e.axis[ 1 ], right );
	} else if ( psprite->type == SPR_VP_PARALLEL_ORIENTED )     {
		// generate the sprite's axes, parallel to the viewplane, but rotated in
		// that plane around the center according to the sprite entity's roll
		// angle. So vpn stays the same, but vright and vup rotate
		float sr;
		float cr;
		if ( backEnd.currentEntity->e.axis[ 0 ][ 2 ] == 1 ) {
			sr = -backEnd.currentEntity->e.axis[ 1 ][ 0 ];
			cr = backEnd.currentEntity->e.axis[ 1 ][ 1 ];
		} else if ( backEnd.currentEntity->e.axis[ 0 ][ 2 ] == -1 )         {
			sr = backEnd.currentEntity->e.axis[ 1 ][ 0 ];
			cr = backEnd.currentEntity->e.axis[ 1 ][ 1 ];
		} else   {
			float cp = sqrt( 1 - backEnd.currentEntity->e.axis[ 0 ][ 2 ] * backEnd.currentEntity->e.axis[ 0 ][ 2 ] );
			sr = backEnd.currentEntity->e.axis[ 1 ][ 2 ] / cp;
			cr = backEnd.currentEntity->e.axis[ 2 ][ 2 ] / cp;
		}

		for ( int i = 0; i < 3; i++ ) {
			right[ i ] = -backEnd.viewParms.orient.axis[ 1 ][ i ] * cr + backEnd.viewParms.orient.axis[ 2 ][ i ] * sr;
			up[ i ] = backEnd.viewParms.orient.axis[ 1 ][ i ] * sr + backEnd.viewParms.orient.axis[ 2 ][ i ] * cr;
		}
	} else   {
		common->FatalError( "R_DrawSprite: Bad sprite type %d", psprite->type );
	}

	GL_Bind( frame->gl_texture );

	qglBegin( GL_QUADS );

	qglTexCoord2f( 0, 1 );
	VectorScale( up, frame->down, point );
	VectorMA( point, frame->left, right, point );
	qglVertex3fv( point );

	qglTexCoord2f( 0, 0 );
	VectorScale( up, frame->up, point );
	VectorMA( point, frame->left, right, point );
	qglVertex3fv( point );

	qglTexCoord2f( 1, 0 );
	VectorScale( up, frame->up, point );
	VectorMA( point, frame->right, right, point );
	qglVertex3fv( point );

	qglTexCoord2f( 1, 1 );
	VectorScale( up, frame->down, point );
	VectorMA( point, frame->right, right, point );
	qglVertex3fv( point );

	qglEnd();
}
