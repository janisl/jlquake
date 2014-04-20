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

#include "RenderModelBSP29.h"
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

static byte* mod_base;

static byte mod_novis[ BSP29_MAX_MAP_LEAFS / 8 ];

idRenderModelBSP29::idRenderModelBSP29() {
}

idRenderModelBSP29::~idRenderModelBSP29() {
}

static void Mod_LoadVisibility( bsp29_lump_t* l ) {
	if ( !l->filelen ) {
		loadmodel->brush29_visdata = NULL;
		return;
	}
	loadmodel->brush29_visdata = new byte[ l->filelen ];
	Com_Memcpy( loadmodel->brush29_visdata, mod_base + l->fileofs, l->filelen );
}

static void Mod_LoadEntities( bsp29_lump_t* l ) {
	if ( !l->filelen ) {
		loadmodel->brush29_entities = NULL;
		return;
	}
	loadmodel->brush29_entities = new char[ l->filelen ];
	Com_Memcpy( loadmodel->brush29_entities, mod_base + l->fileofs, l->filelen );
}

static void Mod_SetParent( mbrush29_node_t* node, mbrush29_node_t* parent ) {
	node->parent = parent;
	if ( node->contents < 0 ) {
		return;
	}
	Mod_SetParent( node->children[ 0 ], node );
	Mod_SetParent( node->children[ 1 ], node );
}

static void Mod_LoadNodes( bsp29_lump_t* l ) {
	bsp29_dnode_t* in = ( bsp29_dnode_t* )( mod_base + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->FatalError( "MOD_LoadBmodel: funny lump size in %s", loadmodel->name );
	}
	int count = l->filelen / sizeof ( *in );
	mbrush29_node_t* out = new mbrush29_node_t[ count ];
	Com_Memset( out, 0, sizeof ( mbrush29_node_t ) * count );

	loadmodel->brush29_nodes = out;
	loadmodel->brush29_numnodes = count;

	for ( int i = 0; i < count; i++, in++, out++ ) {
		for ( int j = 0; j < 3; j++ ) {
			out->minmaxs[ j ] = LittleShort( in->mins[ j ] );
			out->minmaxs[ 3 + j ] = LittleShort( in->maxs[ j ] );
		}

		int p = LittleLong( in->planenum );
		out->plane = loadmodel->brush29_planes + p;

		out->firstsurface = LittleShort( in->firstface );
		out->numsurfaces = LittleShort( in->numfaces );

		for ( int j = 0; j < 2; j++ ) {
			p = LittleShort( in->children[ j ] );
			if ( p >= 0 ) {
				out->children[ j ] = loadmodel->brush29_nodes + p;
			} else {
				out->children[ j ] = ( mbrush29_node_t* )( loadmodel->brush29_leafs + ( -1 - p ) );
			}
		}
	}

	Mod_SetParent( loadmodel->brush29_nodes, NULL );	// sets nodes and leafs
}

static void Mod_LoadLeafs( bsp29_lump_t* l ) {
	bsp29_dleaf_t* in = ( bsp29_dleaf_t* )( mod_base + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->FatalError( "MOD_LoadBmodel: funny lump size in %s", loadmodel->name );
	}
	int count = l->filelen / sizeof ( *in );
	mbrush29_leaf_t* out = new mbrush29_leaf_t[ count ];
	Com_Memset( out, 0, sizeof ( mbrush29_leaf_t ) * count );

	loadmodel->brush29_leafs = out;
	loadmodel->brush29_numleafs = count;

	for ( int i = 0; i < count; i++, in++, out++ ) {
		for ( int j = 0; j < 3; j++ ) {
			out->minmaxs[ j ] = LittleShort( in->mins[ j ] );
			out->minmaxs[ 3 + j ] = LittleShort( in->maxs[ j ] );
		}

		int p = LittleLong( in->contents );
		out->contents = p;

		out->firstmarksurface = loadmodel->brush29_marksurfaces +
								( quint16 )LittleShort( in->firstmarksurface );
		out->nummarksurfaces = LittleShort( in->nummarksurfaces );

		p = LittleLong( in->visofs );
		if ( p == -1 ) {
			out->compressed_vis = NULL;
		} else {
			out->compressed_vis = loadmodel->brush29_visdata + p;
		}

		for ( int j = 0; j < 4; j++ ) {
			out->ambient_sound_level[ j ] = in->ambient_level[ j ];
		}
	}
}

static void Mod_LoadMarksurfaces( bsp29_lump_t* l ) {
	short* in = ( short* )( mod_base + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->FatalError( "MOD_LoadBmodel: funny lump size in %s", loadmodel->name );
	}
	int count = l->filelen / sizeof ( *in );
	idSurfaceFaceQ1** out = new idSurfaceFaceQ1*[ count ];

	loadmodel->brush29_marksurfaces = out;
	loadmodel->brush29_nummarksurfaces = count;

	for ( int i = 0; i < count; i++ ) {
		int j = ( quint16 )LittleShort( in[ i ] );
		if ( j >= loadmodel->brush29_numsurfaces ) {
			common->FatalError( "Mod_ParseMarksurfaces: bad surface number" );
		}
		out[ i ] = loadmodel->brush29_surfaces + j;
	}
}

bool idRenderModelBSP29::Load( idList<byte>& buffer, idSkinTranslation* skinTranslation ) {
	Com_Memset( mod_novis, 0xff, sizeof ( mod_novis ) );

	type = MOD_BRUSH29;
	tr.currentModel = this;

	bsp29_dheader_t* header = ( bsp29_dheader_t* )buffer.Ptr();

	int version = LittleLong( header->version );
	if ( version != BSP29_VERSION ) {
		common->FatalError( "Mod_LoadBrush29Model: %s has wrong version number (%i should be %i)", name, version, BSP29_VERSION );
	}

	// swap all the lumps
	mod_base = ( byte* )header;

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
	brush29_numplanes = loader.numplanes;
	brush29_planes = loader.planes;
	brush29_lightdata = loader.lightdata;
	brush29_numtextures = loader.numtextures;
	brush29_textures = loader.textures;
	brush29_numtexinfo = loader.numtexinfo;
	brush29_texinfo = loader.texinfo;
	textureInfos = loader.textureInfos;
	brush29_numsurfaces = loader.numsurfaces;
	brush29_surfaces = loader.surfaces;
	Mod_LoadMarksurfaces( &header->lumps[ BSP29LUMP_MARKSURFACES ] );
	Mod_LoadVisibility( &header->lumps[ BSP29LUMP_VISIBILITY ] );
	Mod_LoadLeafs( &header->lumps[ BSP29LUMP_LEAFS ] );
	Mod_LoadNodes( &header->lumps[ BSP29LUMP_NODES ] );
	Mod_LoadEntities( &header->lumps[ BSP29LUMP_ENTITIES ] );
	if ( GGameType & GAME_Hexen2 ) {
		loader.LoadSubmodelsH2( &header->lumps[ BSP29LUMP_MODELS ] );
	} else {
		loader.LoadSubmodelsQ1( &header->lumps[ BSP29LUMP_MODELS ] );
	}
	brush29_numsubmodels = loader.numsubmodels;
	brush29_submodels = loader.submodels;

	q1_numframes = 2;		// regular and alternate animation

	//
	// set up the submodels (FIXME: this is confusing)
	//
	idRenderModel* mod = this;
	for ( int i = 0; i < mod->brush29_numsubmodels; i++ ) {
		mbrush29_submodel_t* bm = &mod->brush29_submodels[ i ];

		mod->brush29_firstnode = bm->headnode;
		mod->brush29_firstmodelsurface = bm->firstface;
		mod->brush29_nummodelsurfaces = bm->numfaces;
		mod->brush29_numleafs = bm->visleafs;

		VectorCopy( bm->maxs, mod->q1_maxs );
		VectorCopy( bm->mins, mod->q1_mins );

		mod->q1_radius = RadiusFromBounds( mod->q1_mins, mod->q1_maxs );

		if ( i < mod->brush29_numsubmodels - 1 ) {
			// duplicate the basic information
			loadmodel = new idRenderModelBSP29();
			R_AddModel( loadmodel );
			int saved_index = loadmodel->index;
			*loadmodel = *mod;
			loadmodel->index = saved_index;
			String::Sprintf( loadmodel->name, sizeof ( loadmodel->name ), "*%i", i + 1 );
			mod = loadmodel;
		}
	}
	return true;
}

static byte* Mod_DecompressVis( byte* in, idRenderModel* model ) {
	static byte decompressed[ BSP29_MAX_MAP_LEAFS / 8 ];

	int row = ( model->brush29_numleafs + 7 ) >> 3;
	byte* out = decompressed;

	if ( !in ) {
		// no vis info, so make all visible
		while ( row ) {
			*out++ = 0xff;
			row--;
		}
		return decompressed;
	}

	do {
		if ( *in ) {
			*out++ = *in++;
			continue;
		}

		int c = in[ 1 ];
		in += 2;
		while ( c ) {
			*out++ = 0;
			c--;
		}
	} while ( out - decompressed < row );

	return decompressed;
}

byte* Mod_LeafPVS( mbrush29_leaf_t* leaf, idRenderModel* model ) {
	if ( leaf == model->brush29_leafs ) {
		return mod_novis;
	}
	return Mod_DecompressVis( leaf->compressed_vis, model );
}
