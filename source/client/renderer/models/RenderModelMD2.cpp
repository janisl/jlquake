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

#include "RenderModelMD2.h"
#include "../../../common/Common.h"
#include "../../../common/endian.h"

idRenderModelMD2::idRenderModelMD2() {
}

idRenderModelMD2::~idRenderModelMD2() {
}

struct idMd2VertexRemap {
	int xyzIndex;
	float s;
	float t;
};

static int AddToVertexMap( idList< idMd2VertexRemap >& vertexMap, int xyzIndex, float s, float t ) {
	for ( int i = 0; i < vertexMap.Num(); i++ ) {
		if ( vertexMap[ i ].xyzIndex == xyzIndex && vertexMap[ i ].s == s && vertexMap[ i ].t == t) {
			return i;
		}
	}

	idMd2VertexRemap& v = vertexMap.Alloc();
	v.xyzIndex = xyzIndex;
	v.s = s;
	v.t = t;
	return vertexMap.Num() - 1;
}

static void ExtractMd2Triangles( int* order, idList<glIndex_t>& indexes, idList<idMd2VertexRemap>& vertexMap ) {
	while ( 1 ) {
		// get the vertex count and primitive type
		int count = *order++;
		if ( !count ) {
			break;		// done
		}
		bool isFan = count < 0;
		if ( isFan ) {
			count = -count;
		}

		int i = 0;
		int triangle[3] = { -1, -1, -1 };
		do {
			// texture coordinates come from the draw list
			int index = AddToVertexMap( vertexMap, order[ 2 ], ( ( float* )order )[ 0 ], ( ( float* )order )[ 1 ] );
			order += 3;
			if ( i < 3 ) {
				triangle[ i ] = index;
			} else if ( isFan ) {
				triangle[ 1 ] = triangle[ 2 ];
				triangle[ 2 ] = index;
			} else if ( i & 1 ) {
				triangle[ 0 ] = triangle[ 1 ];
				triangle[ 1 ] = index;
				triangle[ 2 ] = triangle[ 2 ];
			} else {
				triangle[ 0 ] = triangle[ 2 ];
				triangle[ 2 ] = index;
			}
			i++;
			if ( i >= 3 ) {
				indexes.Append( triangle[ 0 ] );
				indexes.Append( triangle[ 1 ] );
				indexes.Append( triangle[ 2 ] );
			}
		} while ( --count );
	}
}

bool idRenderModelMD2::Load( idList<byte>& buffer, idSkinTranslation* skinTranslation ) {
	// byte swap the header fields and sanity check
	dmd2_t pinmodel;
	for ( int i = 0; i < ( int )sizeof ( dmd2_t ) / 4; i++ ) {
		( ( int* )&pinmodel )[ i ] = LittleLong( ( ( int* )buffer.Ptr() )[ i ] );
	}

	if ( pinmodel.version != MESH2_VERSION ) {
		common->Error( "%s has wrong version number (%i should be %i)",
			name, pinmodel.version, MESH2_VERSION );
	}

	if ( pinmodel.num_xyz <= 0 ) {
		common->Error( "model %s has no vertices", name );
	}

	if ( pinmodel.num_xyz > MAX_MD2_VERTS ) {
		common->Error( "model %s has too many vertices", name );
	}

	if ( pinmodel.num_st <= 0 ) {
		common->Error( "model %s has no st vertices", name );
	}

	if ( pinmodel.num_tris <= 0 ) {
		common->Error( "model %s has no triangles", name );
	}

	if ( pinmodel.num_frames <= 0 ) {
		common->Error( "model %s has no frames", name );
	}

	//
	// load the glcmds
	//
	const int* pincmd = ( const int* )( ( const byte* )buffer.Ptr() + pinmodel.ofs_glcmds );
	idList<int> glcmds;
	glcmds.SetNum( pinmodel.num_glcmds );
	for ( int i = 0; i < pinmodel.num_glcmds; i++ ) {
		glcmds[ i ] = LittleLong( pincmd[ i ] );
	}

	idList<glIndex_t> indexes;
	idList<idMd2VertexRemap> vertexMap;
	ExtractMd2Triangles( glcmds.Ptr(), indexes, vertexMap );

	int frameSize = sizeof( dmd2_frame_t ) + ( vertexMap.Num() - 1 ) * sizeof( dmd2_trivertx_t );

	type = MOD_MESH2;
	q2_extradatasize = sizeof( mmd2_t ) +
		pinmodel.num_frames * frameSize +
		sizeof( idVec2 ) * vertexMap.Num() +
		sizeof( glIndex_t ) * indexes.Num();
	q2_md2 = new idSurfaceMD2;
	q2_numframes = pinmodel.num_frames;

	idSurfaceMD2* pheader = q2_md2;

	pheader->header.framesize = frameSize;
	pheader->header.num_skins = pinmodel.num_skins;
	pheader->header.num_frames = pinmodel.num_frames;
	pheader->header.numVertexes = vertexMap.Num();
	pheader->header.numIndexes = indexes.Num();

	//
	// load the frames
	//
	pheader->header.frames = new byte[ pinmodel.num_frames * frameSize ];
	for ( int i = 0; i < pheader->header.num_frames; i++ ) {
		const dmd2_frame_t* pinframe = ( const dmd2_frame_t* )( ( const byte* )buffer.Ptr() +
			pinmodel.ofs_frames + i * pinmodel.framesize );
		dmd2_frame_t* poutframe = ( dmd2_frame_t* )( pheader->header.frames + i * pheader->header.framesize );

		Com_Memcpy( poutframe->name, pinframe->name, sizeof ( poutframe->name ) );
		for ( int j = 0; j < 3; j++ ) {
			poutframe->scale[ j ] = LittleFloat( pinframe->scale[ j ] );
			poutframe->translate[ j ] = LittleFloat( pinframe->translate[ j ] );
		}
		for ( int j = 0; j < vertexMap.Num(); j++ ) {
			Com_Memcpy( &poutframe->verts[ j ], &pinframe->verts[ vertexMap[ j ].xyzIndex ], sizeof ( dmd2_trivertx_t ) );
		}
	}

	//	Copy texture coordinates
	pheader->header.texCoords = new idVec2[ vertexMap.Num() ];
	for ( int i = 0; i < vertexMap.Num(); i++ ) {
		pheader->header.texCoords[ i ].x = vertexMap[ i ].s;
		pheader->header.texCoords[ i ].y = vertexMap[ i ].t;
	}

	//	Copy indexes
	pheader->header.indexes = new glIndex_t[ pheader->header.numIndexes ];
	Com_Memcpy( pheader->header.indexes, indexes.Ptr(), pheader->header.numIndexes * sizeof( glIndex_t ) );

	// register all skins
	for ( int i = 0; i < pheader->header.num_skins; i++ ) {
		image_t* image = R_FindImageFile( ( const char* )buffer.Ptr() + pinmodel.ofs_skins + i * MAX_MD2_SKINNAME,
			true, true, GL_CLAMP, IMG8MODE_Skin );
		q2_skins_shader[ i ] = R_BuildMd2Shader( image );
	}

	q2_mins[ 0 ] = -32;
	q2_mins[ 1 ] = -32;
	q2_mins[ 2 ] = -32;
	q2_maxs[ 0 ] = 32;
	q2_maxs[ 1 ] = 32;
	q2_maxs[ 2 ] = 32;
	return true;
}
