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

#include "RenderModelMDC.h"
#include "../../../common/strings.h"
#include "../../../common/endian.h"
#include "../../../common/Common.h"
#include "../cvars.h"

#define LL( x ) x = LittleLong( x )

idRenderModelMDC::idRenderModelMDC() {
}

idRenderModelMDC::~idRenderModelMDC() {
}

static bool R_LoadMdcLod( idRenderModel* mod, int lod, void* buffer, const char* mod_name ) {
	mdcHeader_t* pinmodel = ( mdcHeader_t* )buffer;

	int version = LittleLong( pinmodel->version );
	if ( version != MDC_VERSION ) {
		common->Printf( S_COLOR_YELLOW "R_LoadMdcLod: %s has wrong version (%i should be %i)\n",
			mod_name, version, MDC_VERSION );
		return false;
	}

	mod->type = MOD_MDC;
	int size = LittleLong( pinmodel->ofsEnd );
	mod->q3_dataSize += size;
	mod->q3_mdc[ lod ].header = ( mdcHeader_t* )Mem_Alloc( size );

	memcpy( mod->q3_mdc[ lod ].header, buffer, LittleLong( pinmodel->ofsEnd ) );

	LL( mod->q3_mdc[ lod ].header->ident );
	LL( mod->q3_mdc[ lod ].header->version );
	LL( mod->q3_mdc[ lod ].header->numFrames );
	LL( mod->q3_mdc[ lod ].header->numTags );
	LL( mod->q3_mdc[ lod ].header->numSurfaces );
	LL( mod->q3_mdc[ lod ].header->ofsFrames );
	LL( mod->q3_mdc[ lod ].header->ofsTagNames );
	LL( mod->q3_mdc[ lod ].header->ofsTags );
	LL( mod->q3_mdc[ lod ].header->ofsSurfaces );
	LL( mod->q3_mdc[ lod ].header->ofsEnd );
	LL( mod->q3_mdc[ lod ].header->flags );
	LL( mod->q3_mdc[ lod ].header->numSkins );


	if ( mod->q3_mdc[ lod ].header->numFrames < 1 ) {
		common->Printf( S_COLOR_YELLOW "R_LoadMdcLod: %s has no frames\n", mod_name );
		return false;
	}

	// swap all the frames
	md3Frame_t* frame = ( md3Frame_t* )( ( byte* )mod->q3_mdc[ lod ].header + mod->q3_mdc[ lod ].header->ofsFrames );
	for ( int i = 0; i < mod->q3_mdc[ lod ].header->numFrames; i++, frame++ ) {
		frame->radius = LittleFloat( frame->radius );
		if ( strstr( mod->name, "sherman" ) || strstr( mod->name, "mg42" ) ) {
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
	mdcTag_t* tag = ( mdcTag_t* )( ( byte* )mod->q3_mdc[ lod ].header + mod->q3_mdc[ lod ].header->ofsTags );
	if ( LittleLong( 1 ) != 1 ) {
		for ( int i = 0; i < mod->q3_mdc[ lod ].header->numTags * mod->q3_mdc[ lod ].header->numFrames; i++, tag++ ) {
			for ( int j = 0; j < 3; j++ ) {
				tag->xyz[ j ] = LittleShort( tag->xyz[ j ] );
				tag->angles[ j ] = LittleShort( tag->angles[ j ] );
			}
		}
	}

	// swap all the surfaces
	mod->q3_mdc[ lod ].surfaces = new idSurfaceMDC[ mod->q3_mdc[ lod ].header->numSurfaces ];
	mdcSurface_t* surf = ( mdcSurface_t* )( ( byte* )mod->q3_mdc[ lod ].header + mod->q3_mdc[ lod ].header->ofsSurfaces );
	for ( int i = 0; i < mod->q3_mdc[ lod ].header->numSurfaces; i++ ) {
		mod->q3_mdc[ lod ].surfaces[ i ].SetMdcData( surf );

		LL( surf->ident );
		LL( surf->flags );
		LL( surf->numBaseFrames );
		LL( surf->numCompFrames );
		LL( surf->numShaders );
		LL( surf->numTriangles );
		LL( surf->ofsTriangles );
		LL( surf->numVerts );
		LL( surf->ofsShaders );
		LL( surf->ofsSt );
		LL( surf->ofsXyzNormals );
		LL( surf->ofsXyzCompressed );
		LL( surf->ofsFrameBaseFrames );
		LL( surf->ofsFrameCompFrames );
		LL( surf->ofsEnd );

		if ( surf->numVerts > SHADER_MAX_VERTEXES ) {
			common->Error( "R_LoadMdcLod: %s has more than %i verts on a surface (%i)",
				mod_name, SHADER_MAX_VERTEXES, surf->numVerts );
		}
		if ( surf->numTriangles * 3 > SHADER_MAX_INDEXES ) {
			common->Error( "R_LoadMdcLod: %s has more than %i triangles on a surface (%i)",
				mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles );
		}

		// change to surface identifier
		surf->ident = SF_MDC;

		// lowercase the surface name so skin compares are faster
		String::ToLower( surf->name );

		// strip off a trailing _1 or _2
		// this is a crutch for q3data being a mess
		int j = String::Length( surf->name );
		if ( j > 2 && surf->name[ j - 2 ] == '_' ) {
			surf->name[ j - 2 ] = 0;
		}

		// register the shaders
		md3Shader_t* shader = ( md3Shader_t* )( ( byte* )surf + surf->ofsShaders );
		for ( j = 0; j < surf->numShaders; j++, shader++ ) {
			shader_t* sh;

			sh = R_FindShader( shader->name, LIGHTMAP_NONE, true );
			if ( sh->defaultShader ) {
				shader->shaderIndex = 0;
			} else {
				shader->shaderIndex = sh->index;
			}
		}

		// Ridah, optimization, only do the swapping if we really need to
		if ( LittleShort( 1 ) != 1 ) {

			// swap all the triangles
			md3Triangle_t* tri = ( md3Triangle_t* )( ( byte* )surf + surf->ofsTriangles );
			for ( j = 0; j < surf->numTriangles; j++, tri++ ) {
				LL( tri->indexes[ 0 ] );
				LL( tri->indexes[ 1 ] );
				LL( tri->indexes[ 2 ] );
			}

			// swap all the ST
			md3St_t* st = ( md3St_t* )( ( byte* )surf + surf->ofsSt );
			for ( j = 0; j < surf->numVerts; j++, st++ ) {
				st->st[ 0 ] = LittleFloat( st->st[ 0 ] );
				st->st[ 1 ] = LittleFloat( st->st[ 1 ] );
			}

			// swap all the XyzNormals
			md3XyzNormal_t* xyz = ( md3XyzNormal_t* )( ( byte* )surf + surf->ofsXyzNormals );
			for ( j = 0; j < surf->numVerts * surf->numBaseFrames; j++, xyz++ ) {
				xyz->xyz[ 0 ] = LittleShort( xyz->xyz[ 0 ] );
				xyz->xyz[ 1 ] = LittleShort( xyz->xyz[ 1 ] );
				xyz->xyz[ 2 ] = LittleShort( xyz->xyz[ 2 ] );

				xyz->normal = LittleShort( xyz->normal );
			}

			// swap all the XyzCompressed
			mdcXyzCompressed_t* xyzComp = ( mdcXyzCompressed_t* )( ( byte* )surf + surf->ofsXyzCompressed );
			for ( j = 0; j < surf->numVerts * surf->numCompFrames; j++, xyzComp++ ) {
				LL( xyzComp->ofsVec );
			}

			// swap the frameBaseFrames
			short* ps = ( short* )( ( byte* )surf + surf->ofsFrameBaseFrames );
			for ( j = 0; j < mod->q3_mdc[ lod ].header->numFrames; j++, ps++ ) {
				*ps = LittleShort( *ps );
			}

			// swap the frameCompFrames
			ps = ( short* )( ( byte* )surf + surf->ofsFrameCompFrames );
			for ( j = 0; j < mod->q3_mdc[ lod ].header->numFrames; j++, ps++ ) {
				*ps = LittleShort( *ps );
			}
		}
		// done.

		// find the next surface
		surf = ( mdcSurface_t* )( ( byte* )surf + surf->ofsEnd );
	}

	return true;
}

bool idRenderModelMDC::Load( idList<byte>& buffer, idSkinTranslation* skinTranslation ) {
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
			sprintf( namebuf, "_%d.mdc", lod );
			String::Cat( filename, sizeof ( filename ), namebuf );

			FS_ReadFile( filename, &buf );
			if ( !buf ) {
				continue;
			}
		}

		int ident = LittleLong( *( unsigned* )buf );
		if ( ident != MDC_IDENT ) {
			common->Printf( S_COLOR_YELLOW "R_LoadMdc: unknown fileid for %s\n", filename );
			return false;
		}

		bool loaded = R_LoadMdcLod( this, lod, buf, filename );

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
			if ( lod <= r_lodbias->integer ) {
				break;
			}
		}
	}

	if ( !numLoaded ) {
		return false;
	}

	// duplicate into higher lod spots that weren't
	// loaded, in case the user changes r_lodbias on the fly
	for ( lod--; lod >= 0; lod-- ) {
		q3_numLods++;
		q3_mdc[ lod ] = q3_mdc[ lod + 1 ];
	}

	return true;
}
