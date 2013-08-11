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

#include "RenderModelMDM.h"
#include "../../../common/Common.h"
#include "../../../common/endian.h"
#include "../../../common/strings.h"

#define LL( x ) x = LittleLong( x )

idRenderModelMDM::idRenderModelMDM() {
}

idRenderModelMDM::~idRenderModelMDM() {
}

bool idRenderModelMDM::Load( idList<byte>& buffer, idSkinTranslation* skinTranslation ) {
	mdmHeader_t* pinmodel = ( mdmHeader_t* )buffer.Ptr();

	int version = LittleLong( pinmodel->version );
	if ( version != MDM_VERSION ) {
		common->Printf( S_COLOR_YELLOW "R_LoadMdm: %s has wrong version (%i should be %i)\n",
			name, version, MDM_VERSION );
		return false;
	}

	type = MOD_MDM;
	int size = LittleLong( pinmodel->ofsEnd );
	q3_dataSize += size;
	mdmHeader_t* mdm = q3_mdm = ( mdmHeader_t* )Mem_Alloc( size );

	memcpy( mdm, buffer.Ptr(), LittleLong( pinmodel->ofsEnd ) );

	LL( mdm->ident );
	LL( mdm->version );
	LL( mdm->numTags );
	LL( mdm->numSurfaces );
	LL( mdm->ofsTags );
	LL( mdm->ofsEnd );
	LL( mdm->ofsSurfaces );
	mdm->lodBias = LittleFloat( mdm->lodBias );
	mdm->lodScale = LittleFloat( mdm->lodScale );

	if ( LittleLong( 1 ) != 1 ) {
		// swap all the tags
		mdmTag_t* tag = ( mdmTag_t* )( ( byte* )mdm + mdm->ofsTags );
		for ( int i = 0; i < mdm->numTags; i++ ) {
			for ( int ii = 0; ii < 3; ii++ ) {
				tag->axis[ ii ][ 0 ] = LittleFloat( tag->axis[ ii ][ 0 ] );
				tag->axis[ ii ][ 1 ] = LittleFloat( tag->axis[ ii ][ 1 ] );
				tag->axis[ ii ][ 2 ] = LittleFloat( tag->axis[ ii ][ 2 ] );
			}

			LL( tag->boneIndex );
			tag->offset[ 0 ] = LittleFloat( tag->offset[ 0 ] );
			tag->offset[ 1 ] = LittleFloat( tag->offset[ 1 ] );
			tag->offset[ 2 ] = LittleFloat( tag->offset[ 2 ] );

			LL( tag->numBoneReferences );
			LL( tag->ofsBoneReferences );
			LL( tag->ofsEnd );

			// swap the bone references
			int* boneref = ( int* )( ( byte* )tag + tag->ofsBoneReferences );
			for ( int j = 0; j < tag->numBoneReferences; j++, boneref++ ) {
				*boneref = LittleLong( *boneref );
			}

			// find the next tag
			tag = ( mdmTag_t* )( ( byte* )tag + tag->ofsEnd );
		}
	}

	// swap all the surfaces
	q3_mdmSurfaces = new idSurfaceMDM[ mdm->numSurfaces ];
	mdmSurface_t* surf = ( mdmSurface_t* )( ( byte* )mdm + mdm->ofsSurfaces );
	for ( int i = 0; i < mdm->numSurfaces; i++ ) {
		q3_mdmSurfaces[ i ].SetMdmData( surf );
		if ( LittleLong( 1 ) != 1 ) {
			//LL(surf->ident);
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

		if ( surf->numVerts > SHADER_MAX_VERTEXES ) {
			common->Error( "R_LoadMdm: %s has more than %i verts on a surface (%i)",
				name, SHADER_MAX_VERTEXES, surf->numVerts );
		}
		if ( surf->numTriangles * 3 > SHADER_MAX_INDEXES ) {
			common->Error( "R_LoadMdm: %s has more than %i triangles on a surface (%i)",
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
			mdmTriangle_t* tri = ( mdmTriangle_t* )( ( byte* )surf + surf->ofsTriangles );
			for ( int j = 0; j < surf->numTriangles; j++, tri++ ) {
				LL( tri->indexes[ 0 ] );
				LL( tri->indexes[ 1 ] );
				LL( tri->indexes[ 2 ] );
			}

			// swap all the vertexes
			mdmVertex_t* v = ( mdmVertex_t* )( ( byte* )surf + surf->ofsVerts );
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

				v = ( mdmVertex_t* )&v->weights[ v->numWeights ];
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
		surf = ( mdmSurface_t* )( ( byte* )surf + surf->ofsEnd );
	}

	return true;
}
