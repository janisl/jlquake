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

#include "RenderModelMDS.h"
#include "../../../common/Common.h"
#include "../../../common/endian.h"
#include "../../../common/strings.h"

#define LL( x ) x = LittleLong( x )

idRenderModelMDS::idRenderModelMDS() {
}

idRenderModelMDS::~idRenderModelMDS() {
}

bool idRenderModelMDS::Load( idList<byte>& buffer, idSkinTranslation* skinTranslation ) {
	mdsHeader_t* pinmodel = ( mdsHeader_t* )buffer.Ptr();

	int version = LittleLong( pinmodel->version );
	if ( version != MDS_VERSION ) {
		common->Printf( S_COLOR_YELLOW "R_LoadMds: %s has wrong version (%i should be %i)\n",
			name, version, MDS_VERSION );
		return false;
	}

	type = MOD_MDS;
	int size = LittleLong( pinmodel->ofsEnd );
	q3_dataSize += size;
	mdsHeader_t* mds = ( mdsHeader_t* )Mem_Alloc( size );
	q3_mds = mds;

	memcpy( mds, buffer.Ptr(), LittleLong( pinmodel->ofsEnd ) );

	LL( mds->ident );
	LL( mds->version );
	LL( mds->numFrames );
	LL( mds->numBones );
	LL( mds->numTags );
	LL( mds->numSurfaces );
	LL( mds->ofsFrames );
	LL( mds->ofsBones );
	LL( mds->ofsTags );
	LL( mds->ofsEnd );
	LL( mds->ofsSurfaces );
	mds->lodBias = LittleFloat( mds->lodBias );
	mds->lodScale = LittleFloat( mds->lodScale );
	LL( mds->torsoParent );

	if ( mds->numFrames < 1 ) {
		common->Printf( S_COLOR_YELLOW "R_LoadMds: %s has no frames\n", name );
		return false;
	}

	if ( LittleLong( 1 ) != 1 ) {
		// swap all the frames
		//frameSize = (int)( &((mdsFrame_t *)0)->bones[ mds->numBones ] );
		int frameSize = ( int )( sizeof ( mdsFrame_t ) - sizeof ( mdsBoneFrameCompressed_t ) + mds->numBones * sizeof ( mdsBoneFrameCompressed_t ) );
		for ( int i = 0; i < mds->numFrames; i++ ) {
			mdsFrame_t* frame = ( mdsFrame_t* )( ( byte* )mds + mds->ofsFrames + i * frameSize );
			frame->radius = LittleFloat( frame->radius );
			for ( int j = 0; j < 3; j++ ) {
				frame->bounds[ 0 ][ j ] = LittleFloat( frame->bounds[ 0 ][ j ] );
				frame->bounds[ 1 ][ j ] = LittleFloat( frame->bounds[ 1 ][ j ] );
				frame->localOrigin[ j ] = LittleFloat( frame->localOrigin[ j ] );
				frame->parentOffset[ j ] = LittleFloat( frame->parentOffset[ j ] );
			}
			for ( int j = 0; j < mds->numBones * ( int )( sizeof ( mdsBoneFrameCompressed_t ) / sizeof ( short ) ); j++ ) {
				( ( short* )frame->bones )[ j ] = LittleShort( ( ( short* )frame->bones )[ j ] );
			}
		}

		// swap all the tags
		mdsTag_t* tag = ( mdsTag_t* )( ( byte* )mds + mds->ofsTags );
		for ( int i = 0; i < mds->numTags; i++, tag++ ) {
			LL( tag->boneIndex );
			tag->torsoWeight = LittleFloat( tag->torsoWeight );
		}

		// swap all the bones
		for ( int i = 0; i < mds->numBones; i++ ) {
			mdsBoneInfo_t* bi = ( mdsBoneInfo_t* )( ( byte* )mds + mds->ofsBones + i * sizeof ( mdsBoneInfo_t ) );
			LL( bi->parent );
			bi->torsoWeight = LittleFloat( bi->torsoWeight );
			bi->parentDist = LittleFloat( bi->parentDist );
			LL( bi->flags );
		}
	}

	// swap all the surfaces
	mdsSurface_t* surf = ( mdsSurface_t* )( ( byte* )mds + mds->ofsSurfaces );
	for ( int i = 0; i < mds->numSurfaces; i++ ) {
		if ( LittleLong( 1 ) != 1 ) {
			LL( surf->ident );
			LL( surf->shaderIndex );
			LL( surf->minLod );
			LL( surf->ofsHeader );
			LL( surf->ofsCollapseMap );
			LL( surf->numTriangles );
			LL( surf->ofsTriangles );
			LL( surf->numVerts );
			LL( surf->ofsVerts );
			LL( surf->numBoneReferences );
			LL( surf->ofsBoneReferences );
			LL( surf->ofsEnd );
		}

		// change to surface identifier
		surf->ident = SF_MDS;

		if ( surf->numVerts > SHADER_MAX_VERTEXES ) {
			common->Error( "R_LoadMds: %s has more than %i verts on a surface (%i)",
				name, SHADER_MAX_VERTEXES, surf->numVerts );
		}
		if ( surf->numTriangles * 3 > SHADER_MAX_INDEXES ) {
			common->Error( "R_LoadMds: %s has more than %i triangles on a surface (%i)",
				name, SHADER_MAX_INDEXES / 3, surf->numTriangles );
		}

		// register the shaders
		if ( surf->shader[ 0 ] ) {
			shader_t* sh = R_FindShader( surf->shader, LIGHTMAP_NONE, true );
			if ( sh->defaultShader ) {
				surf->shaderIndex = 0;
			} else {
				surf->shaderIndex = sh->index;
			}
		} else {
			surf->shaderIndex = 0;
		}

		if ( LittleLong( 1 ) != 1 ) {
			// swap all the triangles
			mdsTriangle_t* tri = ( mdsTriangle_t* )( ( byte* )surf + surf->ofsTriangles );
			for ( int j = 0; j < surf->numTriangles; j++, tri++ ) {
				LL( tri->indexes[ 0 ] );
				LL( tri->indexes[ 1 ] );
				LL( tri->indexes[ 2 ] );
			}

			// swap all the vertexes
			mdsVertex_t* v = ( mdsVertex_t* )( ( byte* )surf + surf->ofsVerts );
			for ( int j = 0; j < surf->numVerts; j++ ) {
				v->normal[ 0 ] = LittleFloat( v->normal[ 0 ] );
				v->normal[ 1 ] = LittleFloat( v->normal[ 1 ] );
				v->normal[ 2 ] = LittleFloat( v->normal[ 2 ] );

				v->texCoords[ 0 ] = LittleFloat( v->texCoords[ 0 ] );
				v->texCoords[ 1 ] = LittleFloat( v->texCoords[ 1 ] );

				v->numWeights = LittleLong( v->numWeights );

				for ( int k = 0; k < v->numWeights; k++ ) {
					v->weights[ k ].boneIndex = LittleLong( v->weights[ k ].boneIndex );
					v->weights[ k ].boneWeight = LittleFloat( v->weights[ k ].boneWeight );
					v->weights[ k ].offset[ 0 ] = LittleFloat( v->weights[ k ].offset[ 0 ] );
					v->weights[ k ].offset[ 1 ] = LittleFloat( v->weights[ k ].offset[ 1 ] );
					v->weights[ k ].offset[ 2 ] = LittleFloat( v->weights[ k ].offset[ 2 ] );
				}

				v = ( mdsVertex_t* )&v->weights[ v->numWeights ];
			}

			// swap the collapse map
			int* collapseMap = ( int* )( ( byte* )surf + surf->ofsCollapseMap );
			for ( int j = 0; j < surf->numVerts; j++, collapseMap++ ) {
				*collapseMap = LittleLong( *collapseMap );
			}

			// swap the bone references
			int* boneref = ( int* )( ( byte* )surf + surf->ofsBoneReferences );
			for ( int j = 0; j < surf->numBoneReferences; j++, boneref++ ) {
				*boneref = LittleLong( *boneref );
			}
		}

		// find the next surface
		surf = ( mdsSurface_t* )( ( byte* )surf + surf->ofsEnd );
	}

	return true;
}
