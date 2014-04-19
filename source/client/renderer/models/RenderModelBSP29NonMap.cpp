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

#include "RenderModelBSP29NonMap.h"
#include "../../../common/endian.h"
#include "../../../common/Common.h"
#include "../../../common/strings.h"
#include "../../../common/common_defs.h"
#include "model.h"
#include "../sky.h"
#include "../cvars.h"
#include "../main.h"
#include "../surfaces.h"

idRenderModelBSP29NonMap::idRenderModelBSP29NonMap() {
}

idRenderModelBSP29NonMap::~idRenderModelBSP29NonMap() {
}

bool idRenderModelBSP29NonMap::Load( idList<byte>& buffer, idSkinTranslation* skinTranslation ) {
	type = MOD_BRUSH29_NON_MAP;
	tr.currentModel = this;

	bsp29_dheader_t* header = ( bsp29_dheader_t* )buffer.Ptr();

	int version = LittleLong( header->version );
	if ( version != BSP29_VERSION ) {
		common->FatalError( "Mod_LoadBrush29Model: %s has wrong version number (%i should be %i)", name, version, BSP29_VERSION );
	}

	// swap all the lumps
	byte* mod_base = ( byte* )header;

	for ( int i = 0; i < ( int )sizeof ( bsp29_dheader_t ) / 4; i++ ) {
		( ( int* )header )[ i ] = LittleLong( ( ( int* )header )[ i ] );
	}

	// load into heap

	ModBsp29_LoadVertexes( mod_base, &header->lumps[ BSP29LUMP_VERTEXES ], loadmodel->brush29nm_numvertexes, loadmodel->brush29nm_vertexes );
	ModBsp20_LoadEdges( mod_base, &header->lumps[ BSP29LUMP_EDGES ], loadmodel->brush29nm_numedges, loadmodel->brush29nm_edges );
	ModBsp29_LoadSurfedges( mod_base, &header->lumps[ BSP29LUMP_SURFEDGES ], loadmodel->brush29nm_numsurfedges, loadmodel->brush29nm_surfedges );
	ModBsp29_LoadTextures( mod_base, &header->lumps[ BSP29LUMP_TEXTURES ], loadmodel->brush29nm_numtextures, loadmodel->brush29nm_textures );
	ModBsp29_LoadLighting( mod_base, &header->lumps[ BSP29LUMP_LIGHTING ], loadmodel->brush29nm_lightdata );
	ModBsp29_LoadPlanes( mod_base, &header->lumps[ BSP29LUMP_PLANES ], loadmodel->brush29nm_numplanes, loadmodel->brush29nm_planes );
	ModBsp29_LoadTexinfo( mod_base, &header->lumps[ BSP29LUMP_TEXINFO ], loadmodel->brush29nm_numtextures, loadmodel->brush29nm_textures, loadmodel->brush29nm_numtexinfo, loadmodel->brush29nm_texinfo, loadmodel->brush29nm_textureInfos );
	ModBsp29_LoadFaces( mod_base, &header->lumps[ BSP29LUMP_FACES ], loadmodel->brush29nm_edges, loadmodel->brush29nm_surfedges, loadmodel->brush29nm_vertexes, loadmodel->brush29nm_numtexinfo, loadmodel->brush29nm_texinfo, loadmodel->brush29nm_textureInfos, loadmodel->brush29nm_planes, loadmodel->brush29nm_lightdata, loadmodel->brush29nm_numsurfaces, loadmodel->brush29nm_surfaces );
	if ( GGameType & GAME_Hexen2 ) {
		ModBsp29_LoadSubmodelsH2( mod_base, &header->lumps[ BSP29LUMP_MODELS ], loadmodel->brush29nm_numsubmodels, loadmodel->brush29nm_submodels );
	} else {
		ModBsp29_LoadSubmodelsQ1( mod_base, &header->lumps[ BSP29LUMP_MODELS ], loadmodel->brush29nm_numsubmodels, loadmodel->brush29nm_submodels );
	}

	q1_numframes = 2;		// regular and alternate animation

	//
	// set up the submodels
	//
	mbrush29_submodel_t* bm = brush29nm_submodels;

	brush29nm_firstmodelsurface = bm->firstface;
	brush29nm_nummodelsurfaces = bm->numfaces;

	VectorCopy( bm->maxs, q1_maxs );
	VectorCopy( bm->mins, q1_mins );

	q1_radius = RadiusFromBounds( q1_mins, q1_maxs );
	return true;
}
