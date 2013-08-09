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

void Mod_FreeSprite2Model( idRenderModel* mod ) {
	Mem_Free( mod->q2_sp2 );
}

void R_AddSp2Surfaces( trRefEntity_t* e, int forcedSortIndex ) {
	if ( ( tr.currentEntity->e.renderfx & RF_THIRD_PERSON ) && !tr.viewParms.isPortal ) {
		return;
	}

	// don't even bother culling, because it's just a single
	// polygon without a surface cache

	dsprite2_t* psprite = tr.currentModel->q2_sp2;
	e->e.frame %= psprite->numframes;

	R_AddDrawSurfOld( ( surfaceType_t* )psprite, tr.currentModel->q2_skins_shader[ e->e.frame ], 0, false, false, ATI_TESS_NONE, forcedSortIndex );
}

void RB_SurfaceSp2( dsprite2_t* psprite ) {
	vec3_t point;

	dsp2_frame_t* frame = &psprite->frames[ backEnd.currentEntity->e.frame ];

	float* up = backEnd.viewParms.orient.axis[ 2 ];
	float* left = backEnd.viewParms.orient.axis[ 1 ];

	vec3_t normal;
	VectorSubtract( vec3_origin, backEnd.viewParms.orient.axis[ 0 ], normal );

	int numVerts = tess.numVertexes;
	int numIndexes = tess.numIndexes;

	tess.texCoords[ numVerts ][ 0 ][ 0 ] = 0;
	tess.texCoords[ numVerts ][ 0 ][ 1 ] = 1;
	VectorScale( up, -frame->origin_y, point );
	VectorMA( point, frame->origin_x, left, point );
	tess.xyz[ numVerts ][ 0 ] = point[ 0 ];
	tess.xyz[ numVerts ][ 1 ] = point[ 1 ];
	tess.xyz[ numVerts ][ 2 ] = point[ 2 ];
	tess.normal[ numVerts ][ 0 ] = normal[ 0 ];
	tess.normal[ numVerts ][ 1 ] = normal[ 1 ];
	tess.normal[ numVerts ][ 2 ] = normal[ 2 ];

	tess.texCoords[ numVerts + 1 ][ 0 ][ 0 ] = 0;
	tess.texCoords[ numVerts + 1 ][ 0 ][ 1 ] = 0;
	VectorScale( up, frame->height - frame->origin_y, point );
	VectorMA( point, frame->origin_x, left, point );
	tess.xyz[ numVerts + 1 ][ 0 ] = point[ 0 ];
	tess.xyz[ numVerts + 1 ][ 1 ] = point[ 1 ];
	tess.xyz[ numVerts + 1 ][ 2 ] = point[ 2 ];
	tess.normal[ numVerts + 1 ][ 0 ] = normal[ 0 ];
	tess.normal[ numVerts + 1 ][ 1 ] = normal[ 1 ];
	tess.normal[ numVerts + 1 ][ 2 ] = normal[ 2 ];

	tess.texCoords[ numVerts + 2 ][ 0 ][ 0 ] = 1;
	tess.texCoords[ numVerts + 2 ][ 0 ][ 1 ] = 0;
	VectorScale( up, frame->height - frame->origin_y, point );
	VectorMA( point, -( frame->width - frame->origin_x ), left, point );
	tess.xyz[ numVerts + 2 ][ 0 ] = point[ 0 ];
	tess.xyz[ numVerts + 2 ][ 1 ] = point[ 1 ];
	tess.xyz[ numVerts + 2 ][ 2 ] = point[ 2 ];
	tess.normal[ numVerts + 2 ][ 0 ] = normal[ 0 ];
	tess.normal[ numVerts + 2 ][ 1 ] = normal[ 1 ];
	tess.normal[ numVerts + 2 ][ 2 ] = normal[ 2 ];

	tess.texCoords[ numVerts + 3 ][ 0 ][ 0 ] = 1;
	tess.texCoords[ numVerts + 3 ][ 0 ][ 1 ] = 1;
	VectorScale( up, -frame->origin_y, point );
	VectorMA( point, -( frame->width - frame->origin_x ), left, point );
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
