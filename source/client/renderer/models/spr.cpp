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

#include "model.h"
#include "../main.h"
#include "../backend.h"
#include "../../../common/Common.h"
#include "../../../common/endian.h"
#include "../../../common/strings.h"

void Mod_FreeSpriteModel( idRenderModel* mod ) {
	msprite1_t* psprite = mod->q1_spr;
	for ( int i = 0; i < psprite->numframes; i++ ) {
		if ( psprite->frames[ i ].type == SPR_SINGLE ) {
			delete psprite->frames[ i ].frameptr;
		} else {
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
	} else {
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

	// don't even bother culling, because it's just a single
	// polygon without a surface cache

	msprite1_t* psprite = tr.currentModel->q1_spr;
	msprite1frame_t* frame = R_GetSpriteFrame( psprite, backEnd.currentEntity );

	R_AddDrawSurfOld( ( surfaceType_t* )psprite, frame->shader, 0, false, false, ATI_TESS_NONE, forcedSortIndex );
}

void RB_SurfaceSpr( msprite1_t* psprite ) {
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
	} else if ( psprite->type == SPR_VP_PARALLEL ) {
		// generate the sprite's axes, completely parallel to the viewplane. There
		// are no problem situations, because the sprite is always in the same
		// position relative to the viewer
		VectorCopy( backEnd.viewParms.orient.axis[ 2 ], up );
		VectorSubtract( vec3_origin, backEnd.viewParms.orient.axis[ 1 ], right );
	} else if ( psprite->type == SPR_VP_PARALLEL_UPRIGHT ) {
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
	} else if ( psprite->type == SPR_ORIENTED ) {
		// generate the sprite's axes, according to the sprite's world orientation
		VectorCopy( backEnd.currentEntity->e.axis[ 2 ], up );
		VectorSubtract( vec3_origin, backEnd.currentEntity->e.axis[ 1 ], right );
	} else if ( psprite->type == SPR_VP_PARALLEL_ORIENTED ) {
		// generate the sprite's axes, parallel to the viewplane, but rotated in
		// that plane around the center according to the sprite entity's roll
		// angle. So vpn stays the same, but vright and vup rotate
		float sr;
		float cr;
		if ( backEnd.currentEntity->e.axis[ 0 ][ 2 ] == 1 ) {
			sr = -backEnd.currentEntity->e.axis[ 1 ][ 0 ];
			cr = backEnd.currentEntity->e.axis[ 1 ][ 1 ];
		} else if ( backEnd.currentEntity->e.axis[ 0 ][ 2 ] == -1 ) {
			sr = backEnd.currentEntity->e.axis[ 1 ][ 0 ];
			cr = backEnd.currentEntity->e.axis[ 1 ][ 1 ];
		} else {
			float cp = sqrt( 1 - backEnd.currentEntity->e.axis[ 0 ][ 2 ] * backEnd.currentEntity->e.axis[ 0 ][ 2 ] );
			sr = backEnd.currentEntity->e.axis[ 1 ][ 2 ] / cp;
			cr = backEnd.currentEntity->e.axis[ 2 ][ 2 ] / cp;
		}

		for ( int i = 0; i < 3; i++ ) {
			right[ i ] = -backEnd.viewParms.orient.axis[ 1 ][ i ] * cr + backEnd.viewParms.orient.axis[ 2 ][ i ] * sr;
			up[ i ] = backEnd.viewParms.orient.axis[ 1 ][ i ] * sr + backEnd.viewParms.orient.axis[ 2 ][ i ] * cr;
		}
	} else {
		common->FatalError( "R_DrawSprite: Bad sprite type %d", psprite->type );
	}

	vec3_t normal;
	CrossProduct( right, up, normal );

	int numVerts = tess.numVertexes;
	int numIndexes = tess.numIndexes;

	vec3_t point;

	tess.texCoords[ numVerts ][ 0 ][ 0 ] = 0;
	tess.texCoords[ numVerts ][ 0 ][ 1 ] = 1;
	VectorScale( up, frame->down, point );
	VectorMA( point, frame->left, right, point );
	tess.xyz[ numVerts ][ 0 ] = point[ 0 ];
	tess.xyz[ numVerts ][ 1 ] = point[ 1 ];
	tess.xyz[ numVerts ][ 2 ] = point[ 2 ];
	tess.normal[ numVerts ][ 0 ] = normal[ 0 ];
	tess.normal[ numVerts ][ 1 ] = normal[ 1 ];
	tess.normal[ numVerts ][ 2 ] = normal[ 2 ];

	tess.texCoords[ numVerts + 1 ][ 0 ][ 0 ] = 0;
	tess.texCoords[ numVerts + 1 ][ 0 ][ 1 ] = 0;
	VectorScale( up, frame->up, point );
	VectorMA( point, frame->left, right, point );
	tess.xyz[ numVerts + 1 ][ 0 ] = point[ 0 ];
	tess.xyz[ numVerts + 1 ][ 1 ] = point[ 1 ];
	tess.xyz[ numVerts + 1 ][ 2 ] = point[ 2 ];
	tess.normal[ numVerts + 1 ][ 0 ] = normal[ 0 ];
	tess.normal[ numVerts + 1 ][ 1 ] = normal[ 1 ];
	tess.normal[ numVerts + 1 ][ 2 ] = normal[ 2 ];

	tess.texCoords[ numVerts + 2 ][ 0 ][ 0 ] = 1;
	tess.texCoords[ numVerts + 2 ][ 0 ][ 1 ] = 0;
	VectorScale( up, frame->up, point );
	VectorMA( point, frame->right, right, point );
	tess.xyz[ numVerts + 2 ][ 0 ] = point[ 0 ];
	tess.xyz[ numVerts + 2 ][ 1 ] = point[ 1 ];
	tess.xyz[ numVerts + 2 ][ 2 ] = point[ 2 ];
	tess.normal[ numVerts + 2 ][ 0 ] = normal[ 0 ];
	tess.normal[ numVerts + 2 ][ 1 ] = normal[ 1 ];
	tess.normal[ numVerts + 2 ][ 2 ] = normal[ 2 ];

	tess.texCoords[ numVerts + 3 ][ 0 ][ 0 ] = 1;
	tess.texCoords[ numVerts + 3 ][ 0 ][ 1 ] = 1;
	VectorScale( up, frame->down, point );
	VectorMA( point, frame->right, right, point );
	tess.xyz[ numVerts + 3 ][ 0 ] = point[ 0 ];
	tess.xyz[ numVerts + 3 ][ 1 ] = point[ 1 ];
	tess.xyz[ numVerts + 3 ][ 2 ] = point[ 2 ];
	tess.normal[ numVerts + 3 ][ 0 ] = normal[ 0 ];
	tess.normal[ numVerts + 3 ][ 1 ] = normal[ 1 ];
	tess.normal[ numVerts + 3 ][ 2 ] = normal[ 2 ];

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ] = numVerts + 0;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
	tess.indexes[ numIndexes + 3 ] = numVerts + 2;
	tess.indexes[ numIndexes + 4 ] = numVerts + 0;
	tess.indexes[ numIndexes + 5 ] = numVerts + 1;

	tess.numIndexes += 6;
	tess.numVertexes += 4;
}
