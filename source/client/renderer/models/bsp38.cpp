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

#include "model.h"
#include "../main.h"
#include "../surfaces.h"
#include "../cvars.h"
#include "../../../common/Common.h"
#include "../../../common/strings.h"
#include "../../../common/endian.h"
#include "RenderModelBSP38.h"
#include "../BspSurfaceBuilder.h"

struct idBrush38ShaderInfoBuild {
	int flags;
	int numframes;
	int next;		// animation chain
	int lightMapIndex;
	image_t* image;
};

struct idSurface2LoadTimeInfo
{
	int shaderInfoIndex;
};

static byte* mod_base;

static byte mod_novis[ BSP38MAX_MAP_LEAFS / 8 ];

static void Mod_LoadLighting( bsp_lump_t* l ) {
	if ( !l->filelen ) {
		loadmodel->brush38_lightdata = NULL;
		return;
	}
	loadmodel->brush38_lightdata = new byte[ l->filelen ];
	Com_Memcpy( loadmodel->brush38_lightdata, mod_base + l->fileofs, l->filelen );
}

static void Mod_LoadVisibility( bsp_lump_t* l ) {
	if ( !l->filelen ) {
		loadmodel->brush38_vis = NULL;
		return;
	}
	loadmodel->brush38_vis = ( bsp38_dvis_t* )Mem_Alloc( l->filelen );
	Com_Memcpy( loadmodel->brush38_vis, mod_base + l->fileofs, l->filelen );

	loadmodel->brush38_vis->numclusters = LittleLong( loadmodel->brush38_vis->numclusters );
	for ( int i = 0; i < loadmodel->brush38_vis->numclusters; i++ ) {
		loadmodel->brush38_vis->bitofs[ i ][ 0 ] = LittleLong( loadmodel->brush38_vis->bitofs[ i ][ 0 ] );
		loadmodel->brush38_vis->bitofs[ i ][ 1 ] = LittleLong( loadmodel->brush38_vis->bitofs[ i ][ 1 ] );
	}
}

static void Mod_LoadTexinfo( bsp_lump_t* l ) {
	bsp38_texinfo_t* in = ( bsp38_texinfo_t* )( mod_base + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->Error( "MOD_LoadBmodel: funny lump size in %s", loadmodel->name );
	}
	int count = l->filelen / sizeof ( *in );
	mbrush38_texinfo_t* _out = new mbrush38_texinfo_t[ count ];
	idTextureInfo* out = new idTextureInfo[ count ];

	loadmodel->brush38_texinfo = _out;
	loadmodel->textureInfos = out;
	loadmodel->brush38_numtexinfo = count;

	for ( int i = 0; i < count; i++, in++, out++, _out++ ) {
		for ( int j = 0; j < 8; j++ ) {
			out->vecs[ 0 ][ j ] = LittleFloat( in->vecs[ 0 ][ j ] );
		}

		_out->flags = LittleLong( in->flags );
		int next = LittleLong( in->nexttexinfo );
		if ( next > 0 ) {
			_out->next = loadmodel->brush38_texinfo + next;
		} else {
			_out->next = NULL;
		}
		char name[ MAX_QPATH ];
		String::Sprintf( name, sizeof ( name ), "textures/%s.wal", in->texture );

		_out->image = R_FindImageFile( name, true, true, GL_REPEAT );
		if ( !_out->image ) {
			common->Printf( "Couldn't load %s\n", name );
			_out->image = tr.defaultImage;
		}
	}

	// count animation frames
	for ( int i = 0; i < count; i++ ) {
		_out = &loadmodel->brush38_texinfo[ i ];
		_out->numframes = 1;
		for ( mbrush38_texinfo_t* step = _out->next; step && step != _out; step = step->next ) {
			_out->numframes++;
		}
	}
}

static void GL_BuildPolygonFromSurface( idSurfaceFaceQ2* fa ) {
	int lnumverts = fa->numVertexes;

	fa->numIndexes = ( lnumverts - 2 ) * 3;
	fa->indexes = new int[ fa->numIndexes ];
	for ( int i = 0; i < lnumverts; i++ ) {
		fa->vertexes[ i ].normal = fa->plane.Normal();

		vec3_t vec;
		fa->vertexes[ i ].xyz.ToOldVec3( vec );
		float s = DotProduct( vec, fa->textureInfo->vecs[ 0 ] ) + fa->textureInfo->vecs[ 0 ][ 3 ];
		s /= fa->texinfo->image->width;

		float t = DotProduct( vec, fa->textureInfo->vecs[ 1 ] ) + fa->textureInfo->vecs[ 1 ][ 3 ];
		t /= fa->texinfo->image->height;

		fa->vertexes[ i ].st.x = s;
		fa->vertexes[ i ].st.y = t;

		//
		// lightmap texture coordinates
		//
		s = DotProduct( vec, fa->textureInfo->vecs[ 0 ] ) + fa->textureInfo->vecs[ 0 ][ 3 ];
		s -= fa->textureMins[ 0 ];
		s += fa->lightS * 16;
		s += 8;
		s /= BLOCK_WIDTH * 16;

		t = DotProduct( vec, fa->textureInfo->vecs[ 1 ] ) + fa->textureInfo->vecs[ 1 ][ 3 ];
		t -= fa->textureMins[ 1 ];
		t += fa->lightT * 16;
		t += 8;
		t /= BLOCK_HEIGHT * 16;

		fa->vertexes[ i ].lightmap.x = s;
		fa->vertexes[ i ].lightmap.y = t;
	}

	for ( int i = 0; i < lnumverts - 2; i++ ) {
		fa->indexes[ i * 3 + 0 ] = 0;
		fa->indexes[ i * 3 + 1 ] = i + 1;
		fa->indexes[ i * 3 + 2 ] = i + 2;
	}
}

//	We need to duplicate texinfos for different lightmap indexes.
// Since shader infos don't need texture axis, we can merge many
// of them as well.
static int AddShaderInfo( idList<idBrush38ShaderInfoBuild>& shaderInfos, mbrush38_texinfo_t* texinfo, int lightmapIndex )
{
	//	Check for duplicate.
	for ( int i = 0; i < shaderInfos.Num(); i++ ) {
		const idBrush38ShaderInfoBuild& check = shaderInfos[ i ];
		if ( check.image != texinfo->image ||
			check.lightMapIndex != lightmapIndex ||
			check.flags != texinfo->flags ||
			check.numframes != texinfo->numframes ) {
			continue;
		}
		if ( texinfo->next ) {
			mbrush38_texinfo_t* anim = texinfo->next;
			int checkAnim = check.next;
			while ( anim != texinfo ) {
				if ( shaderInfos[ checkAnim ].image != anim->image ) {
					break;
				}
				anim = anim->next;
				checkAnim = shaderInfos[ checkAnim ].next;
			}
			if ( anim != texinfo ) {
				continue;
			}
		}
		return i;
	}

	int index = shaderInfos.Num();
	idBrush38ShaderInfoBuild& base = shaderInfos.Alloc();
	base.flags = texinfo->flags;
	base.image = texinfo->image;
	base.lightMapIndex = lightmapIndex;
	base.numframes = 1;
	if ( !texinfo->next ) {
		base.next = -1;
	} else {
		base.next = shaderInfos.Num();
		mbrush38_texinfo_t* anim = texinfo->next;
		while ( anim != texinfo ) {
			idBrush38ShaderInfoBuild& animFrame = shaderInfos.Alloc();
			animFrame.flags = texinfo->flags;
			animFrame.image = anim->image;
			animFrame.lightMapIndex = lightmapIndex;
			animFrame.numframes = texinfo->numframes;
			animFrame.next = shaderInfos.Num();
			anim = anim->next;
		}
		shaderInfos[ shaderInfos.Num() - 1 ].next = index;
	}
	return index;
}

static void LoadShaderInfos( idList<idBrush38ShaderInfoBuild>& shaderInfos ) {
	loadmodel->brush38_numShaderInfo = shaderInfos.Num();
	loadmodel->brush38_shaderInfo = new mbrush38_shaderInfo_t[ shaderInfos.Num() ];
	idBrush38ShaderInfoBuild* in = shaderInfos.Ptr();
	mbrush38_shaderInfo_t* out = loadmodel->brush38_shaderInfo;
	for ( int i = 0; i < shaderInfos.Num(); i++, in++, out++ ) {
		out->numframes = in->numframes;
		out->next = in->next < 0 ? NULL : &loadmodel->brush38_shaderInfo[ in->next ];
		out->shader = R_BuildBsp38Shader( in->image, in->flags, in->lightMapIndex );
	}
}

static void Mod_LoadFaces( idBspSurfaceBuilder& surfaceBuilder, bsp_lump_t* l ) {
	bsp38_dface_t* in = ( bsp38_dface_t* )( mod_base + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->Error( "MOD_LoadBmodel: funny lump size in %s", loadmodel->name );
	}
	int count = l->filelen / sizeof ( *in );
	idSurfaceFaceQ2* out = new idSurfaceFaceQ2[ count ];

	loadmodel->brush38_surfaces = out;
	loadmodel->brush38_numsurfaces = count;

	tr.currentModel = loadmodel;

	GL_BeginBuildingLightmaps();
	idList<idBrush38ShaderInfoBuild> shaderInfoBuild;
	idList<idSurface2LoadTimeInfo> loadTimeInfo;
	loadTimeInfo.SetNum( count );

	for ( int surfnum = 0; surfnum < count; surfnum++, in++, out++ ) {
		surfaceBuilder.BuildSurfaceVertexesList( out, LittleLong( in->firstedge ), LittleShort( in->numedges ) );

		int planenum = LittleShort( in->planenum );
		int side = LittleShort( in->side );
		out->plane.FromOldCPlane( loadmodel->brush38_planes[ planenum ] );
		if ( side ) {
			out->plane = -out->plane;
		}

		int ti = LittleShort( in->texinfo );
		if ( ti < 0 || ti >= loadmodel->brush38_numtexinfo ) {
			common->Error( "MOD_LoadBmodel: bad texinfo number" );
		}
		out->texinfo = loadmodel->brush38_texinfo + ti;
		out->textureInfo = loadmodel->textureInfos + ti;

		out->CalcSurfaceExtents();

		// lighting info

		for ( int i = 0; i < BSP38_MAXLIGHTMAPS; i++ ) {
			out->styles[ i ] = in->styles[ i ];
		}
		int lightofs = LittleLong( in->lightofs );
		if ( lightofs == -1 ) {
			out->samples = NULL;
		} else {
			out->samples = loadmodel->brush38_lightdata + lightofs;
		}

		// create lightmaps
		// don't bother if we're set to fullbright
		if ( r_fullbright->value || !loadmodel->brush38_lightdata ||
			out->texinfo->flags & ( BSP38SURF_SKY | BSP38SURF_TRANS33 | BSP38SURF_TRANS66 | BSP38SURF_WARP ) ) {
			out->lightMapTextureNum = LIGHTMAP_NONE;
		} else {
			GL_CreateSurfaceLightmapQ2( out );
		}
		loadTimeInfo[ surfnum ].shaderInfoIndex = AddShaderInfo( shaderInfoBuild, out->texinfo, out->lightMapTextureNum );

		// create polygons
		if ( out->texinfo->flags & BSP38SURF_WARP ) {
			for ( int i = 0; i < 2; i++ ) {
				out->extents[ i ] = 16384;
				out->textureMins[ i ] = -8192;
			}
			surfaceBuilder.Subdivide( out );		// cut up polygon for warps
		} else {
			GL_BuildPolygonFromSurface( out );
		}
	}

	GL_EndBuildingLightmaps();

	LoadShaderInfos( shaderInfoBuild );

	out = loadmodel->brush38_surfaces;
	for ( int surfnum = 0; surfnum < count; surfnum++, out++ ) {
		out->shaderInfo = &loadmodel->brush38_shaderInfo[ loadTimeInfo[ surfnum ].shaderInfoIndex ];
		out->shader = out->shaderInfo->shader;
	}
}

static void Mod_SetParent( mbrush38_node_t* node, mbrush38_node_t* parent ) {
	node->parent = parent;
	if ( node->contents != -1 ) {
		return;
	}
	Mod_SetParent( node->children[ 0 ], node );
	Mod_SetParent( node->children[ 1 ], node );
}

static void Mod_LoadNodes( bsp_lump_t* l ) {
	bsp38_dnode_t* in = ( bsp38_dnode_t* )( mod_base + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->Error( "MOD_LoadBmodel: funny lump size in %s", loadmodel->name );
	}
	int count = l->filelen / sizeof ( *in );
	mbrush38_node_t* out = new mbrush38_node_t[ count ];
	Com_Memset( out, 0, sizeof ( mbrush38_node_t ) * count );

	loadmodel->brush38_nodes = out;
	loadmodel->brush38_numnodes = count;

	for ( int i = 0; i < count; i++, in++, out++ ) {
		for ( int j = 0; j < 3; j++ ) {
			out->minmaxs[ j ] = LittleShort( in->mins[ j ] );
			out->minmaxs[ 3 + j ] = LittleShort( in->maxs[ j ] );
		}

		int p = LittleLong( in->planenum );
		out->plane = loadmodel->brush38_planes + p;

		out->firstsurface = LittleShort( in->firstface );
		out->numsurfaces = LittleShort( in->numfaces );
		out->contents = -1;	// differentiate from leafs

		for ( int j = 0; j < 2; j++ ) {
			p = LittleLong( in->children[ j ] );
			if ( p >= 0 ) {
				out->children[ j ] = loadmodel->brush38_nodes + p;
			} else {
				out->children[ j ] = ( mbrush38_node_t* )( loadmodel->brush38_leafs + ( -1 - p ) );
			}
		}
	}

	Mod_SetParent( loadmodel->brush38_nodes, NULL );	// sets nodes and leafs
}

static void Mod_LoadLeafs( bsp_lump_t* l ) {
	bsp38_dleaf_t* in = ( bsp38_dleaf_t* )( mod_base + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->Error( "MOD_LoadBmodel: funny lump size in %s", loadmodel->name );
	}
	int count = l->filelen / sizeof ( *in );
	mbrush38_leaf_t* out = new mbrush38_leaf_t[ count ];
	Com_Memset( out, 0, sizeof ( mbrush38_leaf_t ) * count );

	loadmodel->brush38_leafs = out;
	loadmodel->brush38_numleafs = count;

	for ( int i = 0; i < count; i++, in++, out++ ) {
		for ( int j = 0; j < 3; j++ ) {
			out->minmaxs[ j ] = LittleShort( in->mins[ j ] );
			out->minmaxs[ 3 + j ] = LittleShort( in->maxs[ j ] );
		}

		int p = LittleLong( in->contents );
		out->contents = p;

		out->cluster = LittleShort( in->cluster );
		out->area = LittleShort( in->area );

		out->firstmarksurface = loadmodel->brush38_marksurfaces +
								LittleShort( in->firstleafface );
		out->nummarksurfaces = LittleShort( in->numleaffaces );
	}
}

static void Mod_LoadMarksurfaces( bsp_lump_t* l ) {
	short* in = ( short* )( mod_base + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->Error( "MOD_LoadBmodel: funny lump size in %s", loadmodel->name );
	}
	int count = l->filelen / sizeof ( *in );
	idSurfaceFaceQ2** out = new idSurfaceFaceQ2*[ count ];

	loadmodel->brush38_marksurfaces = out;
	loadmodel->brush38_nummarksurfaces = count;

	for ( int i = 0; i < count; i++ ) {
		int j = LittleShort( in[ i ] );
		if ( j < 0 ||  j >= loadmodel->brush38_numsurfaces ) {
			common->Error( "Mod_ParseMarksurfaces: bad surface number" );
		}
		out[ i ] = loadmodel->brush38_surfaces + j;
	}
}

static void Mod_LoadPlanes( bsp_lump_t* l ) {
	bsp38_dplane_t* in = ( bsp38_dplane_t* )( mod_base + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->Error( "MOD_LoadBmodel: funny lump size in %s", loadmodel->name );
	}
	int count = l->filelen / sizeof ( *in );
	//JL Why 2 times more?
	cplane_t* out = new cplane_t[ count * 2 ];
	Com_Memset( out, 0, sizeof ( cplane_t ) * count * 2 );

	loadmodel->brush38_planes = out;
	loadmodel->brush38_numplanes = count;

	for ( int i = 0; i < count; i++, in++, out++ ) {
		for ( int j = 0; j < 3; j++ ) {
			out->normal[ j ] = LittleFloat( in->normal[ j ] );
		}
		out->dist = LittleFloat( in->dist );
		out->type = LittleLong( in->type );

		SetPlaneSignbits( out );
	}
}

static void Mod_LoadSubmodels( bsp_lump_t* l ) {
	bsp38_dmodel_t* in = ( bsp38_dmodel_t* )( mod_base + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->Error( "MOD_LoadBmodel: funny lump size in %s", loadmodel->name );
	}
	int count = l->filelen / sizeof ( *in );
	mbrush38_model_t* out = new mbrush38_model_t[ count ];

	loadmodel->brush38_submodels = out;
	loadmodel->brush38_numsubmodels = count;

	for ( int i = 0; i < count; i++, in++, out++ ) {
		for ( int j = 0; j < 3; j++ ) {
			// spread the mins / maxs by a pixel
			out->mins[ j ] = LittleFloat( in->mins[ j ] ) - 1;
			out->maxs[ j ] = LittleFloat( in->maxs[ j ] ) + 1;
			out->origin[ j ] = LittleFloat( in->origin[ j ] );
		}
		out->radius = RadiusFromBounds( out->mins, out->maxs );
		out->headnode = LittleLong( in->headnode );
		out->firstface = LittleLong( in->firstface );
		out->numfaces = LittleLong( in->numfaces );
		out->visleafs = 0;
	}
}

void Mod_LoadBrush38Model( idRenderModel* mod, void* buffer ) {
	Com_Memset( mod_novis, 0xff, sizeof ( mod_novis ) );

	loadmodel->type = MOD_BRUSH38;

	bsp38_dheader_t* header = ( bsp38_dheader_t* )buffer;

	int version = LittleLong( header->version );
	if ( version != BSP38_VERSION ) {
		common->Error( "Mod_LoadBrushModel: %s has wrong version number (%i should be %i)", mod->name, version, BSP38_VERSION );
	}

	// swap all the lumps
	mod_base = ( byte* )header;

	for ( int i = 0; i < ( int )sizeof ( bsp38_dheader_t ) / 4; i++ ) {
		( ( int* )header )[ i ] = LittleLong( ( ( int* )header )[ i ] );
	}

	// load into heap
	idBspSurfaceBuilder surfaceBuilder( mod->name, mod_base );

	surfaceBuilder.LoadVertexes( &header->lumps[ BSP38LUMP_VERTEXES ] );
	surfaceBuilder.LoadEdges( &header->lumps[ BSP38LUMP_EDGES ] );
	surfaceBuilder.LoadSurfedges( &header->lumps[ BSP38LUMP_SURFEDGES ] );
	Mod_LoadLighting( &header->lumps[ BSP38LUMP_LIGHTING ] );
	Mod_LoadPlanes( &header->lumps[ BSP38LUMP_PLANES ] );
	Mod_LoadTexinfo( &header->lumps[ BSP38LUMP_TEXINFO ] );
	Mod_LoadFaces( surfaceBuilder, &header->lumps[ BSP38LUMP_FACES ] );
	Mod_LoadMarksurfaces( &header->lumps[ BSP38LUMP_LEAFFACES ] );
	Mod_LoadVisibility( &header->lumps[ BSP38LUMP_VISIBILITY ] );
	Mod_LoadLeafs( &header->lumps[ BSP38LUMP_LEAFS ] );
	Mod_LoadNodes( &header->lumps[ BSP38LUMP_NODES ] );
	Mod_LoadSubmodels( &header->lumps[ BSP38LUMP_MODELS ] );
	mod->q2_numframes = 2;		// regular and alternate animation

	//
	// set up the submodels
	//
	for ( int i = 0; i < mod->brush38_numsubmodels; i++ ) {
		idRenderModel* starmod;

		mbrush38_model_t* bm = &mod->brush38_submodels[ i ];
		if ( i == 0 ) {
			starmod = loadmodel;
		} else {
			starmod = new idRenderModelBSP38();
			R_AddModel( starmod );

			int saved_index = starmod->index;
			*starmod = *loadmodel;
			starmod->index = saved_index;
			String::Sprintf( starmod->name, sizeof ( starmod->name ), "*%d", i );

			starmod->brush38_numleafs = bm->visleafs;
		}

		starmod->brush38_firstmodelsurface = bm->firstface;
		starmod->brush38_nummodelsurfaces = bm->numfaces;
		starmod->brush38_firstnode = bm->headnode;
		if ( starmod->brush38_firstnode >= loadmodel->brush38_numnodes ) {
			common->Error( "Inline model %i has bad firstnode", i );
		}

		VectorCopy( bm->maxs, starmod->q2_maxs );
		VectorCopy( bm->mins, starmod->q2_mins );
		starmod->q2_radius = bm->radius;
	}
}

void Mod_FreeBsp38( idRenderModel* mod ) {
	if ( mod->name[ 0 ] == '*' ) {
		return;
	}

	if ( mod->brush38_lightdata ) {
		delete[] mod->brush38_lightdata;
	}
	if ( mod->brush38_vis ) {
		Mem_Free( mod->brush38_vis );
	}
	delete[] mod->brush38_texinfo;
	delete[] mod->brush38_surfaces;
	delete[] mod->brush38_shaderInfo;
	delete[] mod->brush38_nodes;
	delete[] mod->brush38_leafs;
	delete[] mod->brush38_marksurfaces;
	delete[] mod->brush38_planes;
	delete[] mod->brush38_submodels;
}

static byte* Mod_DecompressVis( byte* in, idRenderModel* model ) {
	static byte decompressed[ BSP38MAX_MAP_LEAFS / 8 ];

	int row = ( model->brush38_vis->numclusters + 7 ) >> 3;
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

byte* Mod_ClusterPVS( int cluster, idRenderModel* model ) {
	if ( cluster == -1 || !model->brush38_vis ) {
		return mod_novis;
	}
	return Mod_DecompressVis( ( byte* )model->brush38_vis + model->brush38_vis->bitofs[ cluster ][ BSP38DVIS_PVS ], model );
}

mbrush38_leaf_t* Mod_PointInLeafQ2( vec3_t p, idRenderModel* model ) {
	if ( !model || !model->brush38_nodes ) {
		common->Error( "Mod_PointInLeafQ2: bad model" );
	}

	mbrush38_node_t* node = model->brush38_nodes;
	while ( 1 ) {
		if ( node->contents != -1 ) {
			return ( mbrush38_leaf_t* )node;
		}
		cplane_t* plane = node->plane;
		float d = DotProduct( p, plane->normal ) - plane->dist;
		if ( d > 0 ) {
			node = node->children[ 0 ];
		} else {
			node = node->children[ 1 ];
		}
	}

	return NULL;	// never reached
}
