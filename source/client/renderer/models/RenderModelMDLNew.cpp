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

#include "RenderModelMDLNew.h"
#include "../../../common/Common.h"
#include "../../../common/endian.h"

idRenderModelMDLNew::idRenderModelMDLNew() {
}

idRenderModelMDLNew::~idRenderModelMDLNew() {
}

//	Reads extra field for num ST verts, and extra index list of them
bool idRenderModelMDLNew::Load( idList<byte>& buffer, idSkinTranslation* skinTranslation ) {
	newmdl_t* pinmodel = ( newmdl_t* )buffer.Ptr();

	int version = LittleLong( pinmodel->version );
	if ( version != MESH1_NEWVERSION ) {
		common->FatalError( "%s has wrong version number (%i should be %i)",
			name, version, MESH1_NEWVERSION );
	}

	//
	// allocate space for a working header, plus all the data except the frames,
	// skin and group info
	//
	int size = sizeof ( mesh1hdr_t ) + ( LittleLong( pinmodel->numframes ) - 1 ) * sizeof ( mdl_pheader->frames[ 0 ] );
	mdl_pheader = ( mesh1hdr_t* )Mem_Alloc( size );
	mdl_pheader->ident = SF_MDL;

	q1_flags = LittleLong( pinmodel->flags );

	//
	// endian-adjust and copy the data, starting with the alias model header
	//
	mdl_pheader->boundingradius = LittleFloat( pinmodel->boundingradius );
	mdl_pheader->numskins = LittleLong( pinmodel->numskins );
	mdl_pheader->skinwidth = LittleLong( pinmodel->skinwidth );
	mdl_pheader->skinheight = LittleLong( pinmodel->skinheight );

	if ( mdl_pheader->skinheight > MAX_LBM_HEIGHT ) {
		common->FatalError( "model %s has a skin taller than %d", name, MAX_LBM_HEIGHT );
	}

	mdl_pheader->numverts = LittleLong( pinmodel->numverts );
	mdl_pheader->version = LittleLong( pinmodel->num_st_verts );	//hide num_st in version

	if ( mdl_pheader->numverts <= 0 ) {
		common->FatalError( "model %s has no vertices", name );
	}

	if ( mdl_pheader->numverts > MAXALIASVERTS ) {
		common->FatalError( "model %s has too many vertices", name );
	}

	mdl_pheader->numtris = LittleLong( pinmodel->numtris );

	if ( mdl_pheader->numtris <= 0 ) {
		common->FatalError( "model %s has no mdl_triangles", name );
	}

	mdl_pheader->numframes = LittleLong( pinmodel->numframes );
	int numframes = mdl_pheader->numframes;
	if ( numframes < 1 ) {
		common->FatalError( "Mod_LoadMdlModel: Invalid # of frames: %d\n", numframes );
	}

	mdl_pheader->size = LittleFloat( pinmodel->size ) * ALIAS_BASE_SIZE_RATIO;
	q1_synctype = ( synctype_t )LittleLong( pinmodel->synctype );
	q1_numframes = mdl_pheader->numframes;

	for ( int i = 0; i < 3; i++ ) {
		mdl_pheader->scale[ i ] = LittleFloat( pinmodel->scale[ i ] );
		mdl_pheader->scale_origin[ i ] = LittleFloat( pinmodel->scale_origin[ i ] );
		mdl_pheader->eyeposition[ i ] = LittleFloat( pinmodel->eyeposition[ i ] );
	}

	//
	// load the skins
	//
	dmdl_skintype_t* pskintype = ( dmdl_skintype_t* )&pinmodel[ 1 ];
	pskintype = ( dmdl_skintype_t* )Mod_LoadAllSkins( mdl_pheader->numskins, pskintype, q1_flags, skinTranslation );

	//
	// load base s and t vertices
	//
	dmdl_stvert_t* pinstverts = ( dmdl_stvert_t* )pskintype;

	for ( int i = 0; i < mdl_pheader->version; i++ ) {	//version holds num_st_verts
		mdl_stverts[ i ].onseam = LittleLong( pinstverts[ i ].onseam );
		mdl_stverts[ i ].s = LittleLong( pinstverts[ i ].s );
		mdl_stverts[ i ].t = LittleLong( pinstverts[ i ].t );
	}

	//
	// load triangle lists
	//
	dmdl_newtriangle_t* pintriangles = ( dmdl_newtriangle_t* )&pinstverts[ mdl_pheader->version ];

	for ( int i = 0; i < mdl_pheader->numtris; i++ ) {
		mdl_triangles[ i ].facesfront = LittleLong( pintriangles[ i ].facesfront );

		for ( int j = 0; j < 3; j++ ) {
			mdl_triangles[ i ].vertindex[ j ] = LittleShort( pintriangles[ i ].vertindex[ j ] );
			mdl_triangles[ i ].stindex[ j ] = LittleShort( pintriangles[ i ].stindex[ j ] );
		}
	}

	//
	// load the frames
	//
	mdl_posenum = 0;
	dmdl_frametype_t* pframetype = ( dmdl_frametype_t* )&pintriangles[ mdl_pheader->numtris ];

	mdl_mins[ 0 ] = mdl_mins[ 1 ] = mdl_mins[ 2 ] = 32768;
	mdl_maxs[ 0 ] = mdl_maxs[ 1 ] = mdl_maxs[ 2 ] = -32768;

	for ( int i = 0; i < numframes; i++ ) {
		mdl_frametype_t frametype;

		frametype = ( mdl_frametype_t )LittleLong( pframetype->type );

		if ( frametype == ALIAS_SINGLE ) {
			pframetype = ( dmdl_frametype_t* )Mod_LoadAliasFrame( pframetype + 1, &mdl_pheader->frames[ i ] );
		} else {
			pframetype = ( dmdl_frametype_t* )Mod_LoadAliasGroup( pframetype + 1, &mdl_pheader->frames[ i ] );
		}
	}

	mdl_pheader->numposes = mdl_posenum;

	type = MOD_MESH1;

	q1_mins[ 0 ] = mdl_mins[ 0 ] - 10;
	q1_mins[ 1 ] = mdl_mins[ 1 ] - 10;
	q1_mins[ 2 ] = mdl_mins[ 2 ] - 10;
	q1_maxs[ 0 ] = mdl_maxs[ 0 ] + 10;
	q1_maxs[ 1 ] = mdl_maxs[ 1 ] + 10;
	q1_maxs[ 2 ] = mdl_maxs[ 2 ] + 10;


	//
	// build the draw lists
	//
	GL_MakeAliasModelDisplayLists( this, mdl_pheader );

	q1_cache = mdl_pheader;
	return true;
}
