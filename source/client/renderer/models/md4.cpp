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

/*

All bones should be an identity orientation to display the mesh exactly
as it is specified.

For all other frames, the bones represent the transformation from the
orientation of the bone in the base frame to the orientation in this
frame.

*/

#include "model.h"
#include "../main.h"
#include "../backend.h"
#include "../surface.h"
#include "../../../common/Common.h"
#include "../../../common/strings.h"
#include "../../../common/endian.h"

void R_FreeMd4( idRenderModel* mod ) {
	Mem_Free( mod->q3_md4 );
}

void R_AddAnimSurfaces( trRefEntity_t* ent ) {
	md4Header_t* header = tr.currentModel->q3_md4;
	md4LOD_t* lod = ( md4LOD_t* )( ( byte* )header + header->ofsLODs );

	md4Surface_t* surface = ( md4Surface_t* )( ( byte* )lod + lod->ofsSurfaces );
	for ( int i = 0; i < lod->numSurfaces; i++ ) {
		shader_t* shader = R_GetShaderByHandle( surface->shaderIndex );
		R_AddDrawSurf( ( surfaceType_t* )surface, shader, 0	/*fogNum*/, false, 0, 0, 0 );
		surface = ( md4Surface_t* )( ( byte* )surface + surface->ofsEnd );
	}
}

void RB_SurfaceAnim( md4Surface_t* surface ) {
	float frontlerp, backlerp;
	if ( backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame ) {
		backlerp = 0;
		frontlerp = 1;
	} else {
		backlerp = backEnd.currentEntity->e.backlerp;
		frontlerp = 1.0f - backlerp;
	}
	md4Header_t* header = ( md4Header_t* )( ( byte* )surface + surface->ofsHeader );

	int frameSize = ( qintptr )( &( ( md4Frame_t* )0 )->bones[ header->numBones ] );

	md4Frame_t* frame = ( md4Frame_t* )( ( byte* )header + header->ofsFrames +
										 backEnd.currentEntity->e.frame * frameSize );
	md4Frame_t* oldFrame = ( md4Frame_t* )( ( byte* )header + header->ofsFrames +
											backEnd.currentEntity->e.oldframe * frameSize );

	RB_CheckOverflow( surface->numVerts, surface->numTriangles * 3 );

	int* triangles = ( int* )( ( byte* )surface + surface->ofsTriangles );
	int indexes = surface->numTriangles * 3;
	int baseIndex = tess.numIndexes;
	int baseVertex = tess.numVertexes;
	for ( int j = 0; j < indexes; j++ ) {
		tess.indexes[ baseIndex + j ] = baseIndex + triangles[ j ];
	}
	tess.numIndexes += indexes;

	//
	// lerp all the needed bones
	//
	md4Bone_t* bonePtr;
	md4Bone_t bones[ MD4_MAX_BONES ];
	if ( !backlerp ) {
		// no lerping needed
		bonePtr = frame->bones;
	} else {
		bonePtr = bones;
		for ( int i = 0; i < header->numBones * 12; i++ ) {
			( ( float* )bonePtr )[ i ] = frontlerp * ( ( float* )frame->bones )[ i ] +
										 backlerp * ( ( float* )oldFrame->bones )[ i ];
		}
	}

	//
	// deform the vertexes by the lerped bones
	//
	int numVerts = surface->numVerts;
	// FIXME
	// This makes TFC's skeletons work.  Shouldn't be necessary anymore, but left
	// in for reference.
	//v = (md4Vertex_t *) ((byte *)surface + surface->ofsVerts + 12);
	md4Vertex_t* v = ( md4Vertex_t* )( ( byte* )surface + surface->ofsVerts );
	for ( int j = 0; j < numVerts; j++ ) {
		vec3_t tempVert, tempNormal;
		VectorClear( tempVert );
		VectorClear( tempNormal );
		md4Weight_t* w = v->weights;
		for ( int k = 0; k < v->numWeights; k++, w++ ) {
			md4Bone_t* bone = bonePtr + w->boneIndex;

			tempVert[ 0 ] += w->boneWeight * ( DotProduct( bone->matrix[ 0 ], w->offset ) + bone->matrix[ 0 ][ 3 ] );
			tempVert[ 1 ] += w->boneWeight * ( DotProduct( bone->matrix[ 1 ], w->offset ) + bone->matrix[ 1 ][ 3 ] );
			tempVert[ 2 ] += w->boneWeight * ( DotProduct( bone->matrix[ 2 ], w->offset ) + bone->matrix[ 2 ][ 3 ] );

			tempNormal[ 0 ] += w->boneWeight * DotProduct( bone->matrix[ 0 ], v->normal );
			tempNormal[ 1 ] += w->boneWeight * DotProduct( bone->matrix[ 1 ], v->normal );
			tempNormal[ 2 ] += w->boneWeight * DotProduct( bone->matrix[ 2 ], v->normal );
		}

		tess.xyz[ baseVertex + j ][ 0 ] = tempVert[ 0 ];
		tess.xyz[ baseVertex + j ][ 1 ] = tempVert[ 1 ];
		tess.xyz[ baseVertex + j ][ 2 ] = tempVert[ 2 ];

		tess.normal[ baseVertex + j ][ 0 ] = tempNormal[ 0 ];
		tess.normal[ baseVertex + j ][ 1 ] = tempNormal[ 1 ];
		tess.normal[ baseVertex + j ][ 2 ] = tempNormal[ 2 ];

		tess.texCoords[ baseVertex + j ][ 0 ][ 0 ] = v->texCoords[ 0 ];
		tess.texCoords[ baseVertex + j ][ 0 ][ 1 ] = v->texCoords[ 1 ];

		// FIXME
		// This makes TFC's skeletons work.  Shouldn't be necessary anymore, but left
		// in for reference.
		//v = (md4Vertex_t *)( ( byte * )&v->weights[v->numWeights] + 12 );
		v = ( md4Vertex_t* )&v->weights[ v->numWeights ];
	}

	tess.numVertexes += surface->numVerts;
}
