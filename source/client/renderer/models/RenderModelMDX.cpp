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

#include "RenderModelMDX.h"
#include "../../../common/Common.h"
#include "../../../common/strings.h"
#include "../../../common/endian.h"

#define LL( x ) x = LittleLong( x )

idRenderModelMDX::idRenderModelMDX() {
}

idRenderModelMDX::~idRenderModelMDX() {
}

bool idRenderModelMDX::Load( idList<byte>& buffer, idSkinTranslation* skinTranslation ) {
	mdxHeader_t* pinmodel = ( mdxHeader_t* )buffer.Ptr();

	int version = LittleLong( pinmodel->version );
	if ( version != MDX_VERSION ) {
		common->Printf( S_COLOR_YELLOW "R_LoadMdx: %s has wrong version (%i should be %i)\n",
			name, version, MDX_VERSION );
		return false;
	}

	type = MOD_MDX;
	int size = LittleLong( pinmodel->ofsEnd );
	q3_dataSize += size;
	mdxHeader_t* mdx = q3_mdx = ( mdxHeader_t* )Mem_Alloc( size );

	memcpy( mdx, buffer.Ptr(), LittleLong( pinmodel->ofsEnd ) );

	LL( mdx->ident );
	LL( mdx->version );
	LL( mdx->numFrames );
	LL( mdx->numBones );
	LL( mdx->ofsFrames );
	LL( mdx->ofsBones );
	LL( mdx->ofsEnd );
	LL( mdx->torsoParent );

	if ( LittleLong( 1 ) != 1 ) {
		// swap all the frames
		int frameSize = ( int )( sizeof ( mdxBoneFrameCompressed_t ) ) * mdx->numBones;
		for ( int i = 0; i < mdx->numFrames; i++ ) {
			mdxFrame_t* frame = ( mdxFrame_t* )( ( byte* )mdx + mdx->ofsFrames + i * frameSize + i * sizeof ( mdxFrame_t ) );
			frame->radius = LittleFloat( frame->radius );
			for ( int j = 0; j < 3; j++ ) {
				frame->bounds[ 0 ][ j ] = LittleFloat( frame->bounds[ 0 ][ j ] );
				frame->bounds[ 1 ][ j ] = LittleFloat( frame->bounds[ 1 ][ j ] );
				frame->localOrigin[ j ] = LittleFloat( frame->localOrigin[ j ] );
				frame->parentOffset[ j ] = LittleFloat( frame->parentOffset[ j ] );
			}

			short* bframe = ( short* )( ( byte* )mdx + mdx->ofsFrames + i * frameSize + ( ( i + 1 ) * sizeof ( mdxFrame_t ) ) );
			for ( int j = 0; j < mdx->numBones * ( int )sizeof ( mdxBoneFrameCompressed_t ) / ( int )sizeof ( short ); j++ ) {
				bframe[ j ] = LittleShort( bframe[ j ] );
			}
		}

		// swap all the bones
		for ( int i = 0; i < mdx->numBones; i++ ) {
			mdxBoneInfo_t* bi = ( mdxBoneInfo_t* )( ( byte* )mdx + mdx->ofsBones + i * sizeof ( mdxBoneInfo_t ) );
			LL( bi->parent );
			bi->torsoWeight = LittleFloat( bi->torsoWeight );
			bi->parentDist = LittleFloat( bi->parentDist );
			LL( bi->flags );
		}
	}

	return true;
}
