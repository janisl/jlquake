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

#include "RenderModelMD3.h"
#include "../../../common/strings.h"
#include "../../../common/endian.h"
#include "../../../common/Common.h"
#include "../../../common/common_defs.h"

#define LL( x ) x = LittleLong( x )

idRenderModelMD3::idRenderModelMD3() {
}

idRenderModelMD3::~idRenderModelMD3() {
}

static bool R_LoadMd3Lod( idRenderModel* mod, int lod, const void* buffer, const char* mod_name ) {
	md3Header_t* pinmodel = ( md3Header_t* )buffer;

	int version = LittleLong( pinmodel->version );
	if ( version != MD3_VERSION ) {
		common->Printf( S_COLOR_YELLOW "R_LoadMD3: %s has wrong version (%i should be %i)\n",
			mod_name, version, MD3_VERSION );
		return false;
	}

	mod->type = MOD_MESH3;
	int size = LittleLong( pinmodel->ofsEnd );
	mod->q3_dataSize += size;
	mod->q3_md3[ lod ].header = ( md3Header_t* )Mem_Alloc( size );

	Com_Memcpy( mod->q3_md3[ lod ].header, buffer, LittleLong( pinmodel->ofsEnd ) );

	LL( mod->q3_md3[ lod ].header->ident );
	LL( mod->q3_md3[ lod ].header->version );
	LL( mod->q3_md3[ lod ].header->numFrames );
	LL( mod->q3_md3[ lod ].header->numTags );
	LL( mod->q3_md3[ lod ].header->numSurfaces );
	LL( mod->q3_md3[ lod ].header->ofsFrames );
	LL( mod->q3_md3[ lod ].header->ofsTags );
	LL( mod->q3_md3[ lod ].header->ofsSurfaces );
	LL( mod->q3_md3[ lod ].header->ofsEnd );

	if ( mod->q3_md3[ lod ].header->numFrames < 1 ) {
		common->Printf( S_COLOR_YELLOW "R_LoadMD3: %s has no frames\n", mod_name );
		return false;
	}

	bool fixRadius = false;
	if ( GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) &&
		 ( strstr( mod->name,"sherman" ) || strstr( mod->name, "mg42" ) ) ) {
		fixRadius = true;
	}

	// swap all the frames
	md3Frame_t* frame = ( md3Frame_t* )( ( byte* )mod->q3_md3[ lod ].header + mod->q3_md3[ lod ].header->ofsFrames );
	for ( int i = 0; i < mod->q3_md3[ lod ].header->numFrames; i++, frame++ ) {
		frame->radius = LittleFloat( frame->radius );
		if ( fixRadius ) {
			frame->radius = 256;
			for ( int j = 0; j < 3; j++ ) {
				frame->bounds[ 0 ][ j ] = 128;
				frame->bounds[ 1 ][ j ] = -128;
				frame->localOrigin[ j ] = LittleFloat( frame->localOrigin[ j ] );
			}
		}
		// Hack for Bug using plugin generated model
		else if ( GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) && frame->radius == 1 ) {
			frame->radius = 256;
			for ( int j = 0; j < 3; j++ ) {
				frame->bounds[ 0 ][ j ] = 128;
				frame->bounds[ 1 ][ j ] = -128;
				frame->localOrigin[ j ] = LittleFloat( frame->localOrigin[ j ] );
			}
		} else {
			for ( int j = 0; j < 3; j++ ) {
				frame->bounds[ 0 ][ j ] = LittleFloat( frame->bounds[ 0 ][ j ] );
				frame->bounds[ 1 ][ j ] = LittleFloat( frame->bounds[ 1 ][ j ] );
				frame->localOrigin[ j ] = LittleFloat( frame->localOrigin[ j ] );
			}
		}
	}

	// swap all the tags
	md3Tag_t* tag = ( md3Tag_t* )( ( byte* )mod->q3_md3[ lod ].header + mod->q3_md3[ lod ].header->ofsTags );
	for ( int i = 0; i < mod->q3_md3[ lod ].header->numTags * mod->q3_md3[ lod ].header->numFrames; i++, tag++ ) {
		for ( int j = 0; j < 3; j++ ) {
			tag->origin[ j ] = LittleFloat( tag->origin[ j ] );
			tag->axis[ 0 ][ j ] = LittleFloat( tag->axis[ 0 ][ j ] );
			tag->axis[ 1 ][ j ] = LittleFloat( tag->axis[ 1 ][ j ] );
			tag->axis[ 2 ][ j ] = LittleFloat( tag->axis[ 2 ][ j ] );
		}
	}

	// swap all the surfaces
	mod->q3_md3[ lod ].surfaces = new idSurfaceMD3[ mod->q3_md3[ lod ].header->numSurfaces ];
	md3Surface_t* surf = ( md3Surface_t* )( ( byte* )mod->q3_md3[ lod ].header + mod->q3_md3[ lod ].header->ofsSurfaces );
	for ( int i = 0; i < mod->q3_md3[ lod ].header->numSurfaces; i++ ) {
		mod->q3_md3[ lod ].surfaces[ i ].SetMd3Data( surf );
		LL( surf->ident );
		LL( surf->flags );
		LL( surf->numFrames );
		LL( surf->numShaders );
		LL( surf->numTriangles );
		LL( surf->ofsTriangles );
		LL( surf->numVerts );
		LL( surf->ofsShaders );
		LL( surf->ofsSt );
		LL( surf->ofsXyzNormals );
		LL( surf->ofsEnd );

		if ( surf->numVerts > SHADER_MAX_VERTEXES ) {
			common->Error( "R_LoadMD3: %s has more than %i verts on a surface (%i)",
				mod_name, SHADER_MAX_VERTEXES, surf->numVerts );
		}
		if ( surf->numTriangles * 3 > SHADER_MAX_INDEXES ) {
			common->Error( "R_LoadMD3: %s has more than %i triangles on a surface (%i)",
				mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles );
		}

		// change to surface identifier
		surf->ident = SF_MD3;

		// lowercase the surface name so skin compares are faster
		String::ToLower( surf->name );

		// strip off a trailing _1 or _2
		// this is a crutch for q3data being a mess
		int Len = String::Length( surf->name );
		if ( Len > 2 && surf->name[ Len - 2 ] == '_' ) {
			surf->name[ Len - 2 ] = 0;
		}

		// register the shaders
		md3Shader_t* shader = ( md3Shader_t* )( ( byte* )surf + surf->ofsShaders );
		for ( int j = 0; j < surf->numShaders; j++, shader++ ) {
			shader_t* sh = R_FindShader( shader->name, LIGHTMAP_NONE, true );
			if ( sh->defaultShader ) {
				shader->shaderIndex = 0;
			} else {
				shader->shaderIndex = sh->index;
			}
		}

		// swap all the triangles
		md3Triangle_t* tri = ( md3Triangle_t* )( ( byte* )surf + surf->ofsTriangles );
		for ( int j = 0; j < surf->numTriangles; j++, tri++ ) {
			LL( tri->indexes[ 0 ] );
			LL( tri->indexes[ 1 ] );
			LL( tri->indexes[ 2 ] );
		}

		// swap all the ST
		md3St_t* st = ( md3St_t* )( ( byte* )surf + surf->ofsSt );
		for ( int j = 0; j < surf->numVerts; j++, st++ ) {
			st->st[ 0 ] = LittleFloat( st->st[ 0 ] );
			st->st[ 1 ] = LittleFloat( st->st[ 1 ] );
		}

		// swap all the XyzNormals
		md3XyzNormal_t* xyz = ( md3XyzNormal_t* )( ( byte* )surf + surf->ofsXyzNormals );
		for ( int j = 0; j < surf->numVerts * surf->numFrames; j++, xyz++ ) {
			xyz->xyz[ 0 ] = LittleShort( xyz->xyz[ 0 ] );
			xyz->xyz[ 1 ] = LittleShort( xyz->xyz[ 1 ] );
			xyz->xyz[ 2 ] = LittleShort( xyz->xyz[ 2 ] );

			xyz->normal = LittleShort( xyz->normal );
		}

		// find the next surface
		surf = ( md3Surface_t* )( ( byte* )surf + surf->ofsEnd );
	}

	return true;
}

bool idRenderModelMD3::Load( idList<byte>& buffer, idSkinTranslation* skinTranslation ) {
	q3_numLods = 0;

	//
	// load the files
	//
	int numLoaded = 0;

	int lod = MD3_MAX_LODS - 1;
	for (; lod >= 0; lod-- ) {
		char filename[ 1024 ];

		String::Cpy( filename, name );

		void* buf;
		if ( lod == 0 ) {
			buf = buffer.Ptr();
		} else {
			char namebuf[ 80 ];

			if ( String::RChr( filename, '.' ) ) {
				*String::RChr( filename, '.' ) = 0;
			}
			sprintf( namebuf, "_%d.md3", lod );
			String::Cat( filename, sizeof ( filename ), namebuf );

			FS_ReadFile( filename, ( void** )&buf );
			if ( !buf ) {
				continue;
			}
		}

		int ident = LittleLong( *( unsigned* )buf );
		if ( ident != MD3_IDENT ) {
			common->Printf( S_COLOR_YELLOW "R_LoadMd3: unknown fileid for %s\n", filename );
			return false;
		}

		bool loaded = R_LoadMd3Lod( this, lod, buf, filename );

		if ( lod != 0 ) {
			FS_FreeFile( buf );
		}

		if ( !loaded ) {
			if ( lod == 0 ) {
				return false;
			} else {
				break;
			}
		} else {
			q3_numLods++;
			numLoaded++;
			// if we have a valid model and are biased
			// so that we won't see any higher detail ones,
			// stop loading them
//			if ( lod <= r_lodbias->integer ) {
//				break;
//			}
		}
	}

	if ( !numLoaded ) {
		return false;
	}

	// duplicate into higher lod spots that weren't
	// loaded, in case the user changes r_lodbias on the fly
	for ( lod--; lod >= 0; lod-- ) {
		q3_numLods++;
		q3_md3[ lod ] = q3_md3[ lod + 1 ];
	}

	return true;
}
