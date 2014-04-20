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
#include "../Bsp29LoadHelper.h"

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
	idBsp29LoadHelper loader( name, mod_base );
	loader.LoadVertexes( &header->lumps[ BSP29LUMP_VERTEXES ] );
	loader.LoadEdges( &header->lumps[ BSP29LUMP_EDGES ] );
	loader.LoadSurfedges( &header->lumps[ BSP29LUMP_SURFEDGES ] );
	loader.LoadPlanes( &header->lumps[ BSP29LUMP_PLANES ] );
	loader.LoadLighting( &header->lumps[ BSP29LUMP_LIGHTING ] );
	loader.LoadTextures( &header->lumps[ BSP29LUMP_TEXTURES ] );
	loader.LoadTexinfo( &header->lumps[ BSP29LUMP_TEXINFO ] );
	loader.LoadFaces( &header->lumps[ BSP29LUMP_FACES ] );
	delete[] loader.planes;
	brush29nm_lightdata = loader.lightdata;
	brush29nm_numtextures = loader.numtextures;
	brush29nm_textures = loader.textures;
	brush29nm_numtexinfo = loader.numtexinfo;
	brush29nm_texinfo = loader.texinfo;
	brush29nm_textureInfos = loader.textureInfos;
	brush29nm_numsurfaces = loader.numsurfaces;
	brush29nm_surfaces = loader.surfaces;
	if ( GGameType & GAME_Hexen2 ) {
		loader.LoadSubmodelsH2( &header->lumps[ BSP29LUMP_MODELS ] );
	} else {
		loader.LoadSubmodelsQ1( &header->lumps[ BSP29LUMP_MODELS ] );
	}

	q1_numframes = 2;		// regular and alternate animation

	//
	// set up the submodels
	//
	mbrush29_submodel_t* bm = loader.submodels;

	brush29nm_firstmodelsurface = bm->firstface;
	brush29nm_nummodelsurfaces = bm->numfaces;

	VectorCopy( bm->maxs, q1_maxs );
	VectorCopy( bm->mins, q1_mins );

	q1_radius = RadiusFromBounds( q1_mins, q1_maxs );
	delete[] loader.submodels;
	return true;
}
