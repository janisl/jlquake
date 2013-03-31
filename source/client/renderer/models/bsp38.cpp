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

#include "../local.h"
#include "../../../common/Common.h"
#include "../../../common/strings.h"
#include "../../../common/endian.h"

#define SUBDIVIDE_SIZE  64

static byte* mod_base;

static mbrush38_surface_t* warpface;

static byte mod_novis[ BSP38MAX_MAP_LEAFS / 8 ];

static void Mod_LoadLighting( bsp38_lump_t* l ) {
	if ( !l->filelen ) {
		loadmodel->brush38_lightdata = NULL;
		return;
	}
	loadmodel->brush38_lightdata = new byte[ l->filelen ];
	Com_Memcpy( loadmodel->brush38_lightdata, mod_base + l->fileofs, l->filelen );
}

static void Mod_LoadVisibility( bsp38_lump_t* l ) {
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

static void Mod_LoadVertexes( bsp38_lump_t* l ) {
	bsp38_dvertex_t* in = ( bsp38_dvertex_t* )( mod_base + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->Error( "MOD_LoadBmodel: funny lump size in %s", loadmodel->name );
	}
	int count = l->filelen / sizeof ( *in );
	mbrush38_vertex_t* out = new mbrush38_vertex_t[ count ];

	loadmodel->brush38_vertexes = out;
	loadmodel->brush38_numvertexes = count;

	for ( int i = 0; i < count; i++, in++, out++ ) {
		out->position[ 0 ] = LittleFloat( in->point[ 0 ] );
		out->position[ 1 ] = LittleFloat( in->point[ 1 ] );
		out->position[ 2 ] = LittleFloat( in->point[ 2 ] );
	}
}

static void Mod_LoadEdges( bsp38_lump_t* l ) {
	bsp38_dedge_t* in = ( bsp38_dedge_t* )( mod_base + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->Error( "MOD_LoadBmodel: funny lump size in %s", loadmodel->name );
	}
	int count = l->filelen / sizeof ( *in );
	//JL What's the extra edge?
	mbrush38_edge_t* out = new mbrush38_edge_t[ count + 1 ];
	Com_Memset( out, 0, sizeof ( mbrush38_edge_t ) * ( count + 1 ) );

	loadmodel->brush38_edges = out;
	loadmodel->brush38_numedges = count;

	for ( int i = 0; i < count; i++, in++, out++ ) {
		out->v[ 0 ] = ( unsigned short )LittleShort( in->v[ 0 ] );
		out->v[ 1 ] = ( unsigned short )LittleShort( in->v[ 1 ] );
	}
}

static void Mod_LoadTexinfo( bsp38_lump_t* l ) {
	bsp38_texinfo_t* in = ( bsp38_texinfo_t* )( mod_base + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->Error( "MOD_LoadBmodel: funny lump size in %s", loadmodel->name );
	}
	int count = l->filelen / sizeof ( *in );
	mbrush38_texinfo_t* out = new mbrush38_texinfo_t[ count ];

	loadmodel->brush38_texinfo = out;
	loadmodel->brush38_numtexinfo = count;

	for ( int i = 0; i < count; i++, in++, out++ ) {
		for ( int j = 0; j < 8; j++ ) {
			out->vecs[ 0 ][ j ] = LittleFloat( in->vecs[ 0 ][ j ] );
		}

		out->flags = LittleLong( in->flags );
		int next = LittleLong( in->nexttexinfo );
		if ( next > 0 ) {
			out->next = loadmodel->brush38_texinfo + next;
		} else   {
			out->next = NULL;
		}
		char name[ MAX_QPATH ];
		String::Sprintf( name, sizeof ( name ), "textures/%s.wal", in->texture );

		out->image = R_FindImageFile( name, true, true, GL_REPEAT );
		if ( !out->image ) {
			common->Printf( "Couldn't load %s\n", name );
			out->image = tr.defaultImage;
		}
	}

	// count animation frames
	for ( int i = 0; i < count; i++ ) {
		out = &loadmodel->brush38_texinfo[ i ];
		out->numframes = 1;
		for ( mbrush38_texinfo_t* step = out->next; step && step != out; step = step->next ) {
			out->numframes++;
		}
	}
}

//	Fills in s->texturemins[] and s->extents[]
static void CalcSurfaceExtents( mbrush38_surface_t* s ) {
	float mins[ 2 ];
	mins[ 0 ] = mins[ 1 ] = 999999;
	float maxs[ 2 ];
	maxs[ 0 ] = maxs[ 1 ] = -99999;

	mbrush38_texinfo_t* tex = s->texinfo;

	for ( int i = 0; i < s->numedges; i++ ) {
		int e = loadmodel->brush38_surfedges[ s->firstedge + i ];
		mbrush38_vertex_t* v;
		if ( e >= 0 ) {
			v = &loadmodel->brush38_vertexes[ loadmodel->brush38_edges[ e ].v[ 0 ] ];
		} else   {
			v = &loadmodel->brush38_vertexes[ loadmodel->brush38_edges[ -e ].v[ 1 ] ];
		}

		for ( int j = 0; j < 2; j++ ) {
			float val = v->position[ 0 ] * tex->vecs[ j ][ 0 ] +
						v->position[ 1 ] * tex->vecs[ j ][ 1 ] +
						v->position[ 2 ] * tex->vecs[ j ][ 2 ] +
						tex->vecs[ j ][ 3 ];
			if ( val < mins[ j ] ) {
				mins[ j ] = val;
			}
			if ( val > maxs[ j ] ) {
				maxs[ j ] = val;
			}
		}
	}

	int bmins[ 2 ], bmaxs[ 2 ];
	for ( int i = 0; i < 2; i++ ) {
		bmins[ i ] = floor( mins[ i ] / 16 );
		bmaxs[ i ] = ceil( maxs[ i ] / 16 );

		s->texturemins[ i ] = bmins[ i ] * 16;
		s->extents[ i ] = ( bmaxs[ i ] - bmins[ i ] ) * 16;
	}
}

static void BoundPoly( int numverts, float* verts, vec3_t mins, vec3_t maxs ) {
	ClearBounds( mins, maxs );
	float* v = verts;
	for ( int i = 0; i < numverts; i++, v += 3 ) {
		AddPointToBounds( v, mins, maxs );
	}
}

static void SubdividePolygon( int numverts, float* verts ) {
	if ( numverts > 60 ) {
		common->Error( "numverts = %i", numverts );
	}

	vec3_t mins, maxs;
	BoundPoly( numverts, verts, mins, maxs );

	for ( int i = 0; i < 3; i++ ) {
		float m = ( mins[ i ] + maxs[ i ] ) * 0.5;
		m = SUBDIVIDE_SIZE * floor( m / SUBDIVIDE_SIZE + 0.5 );
		if ( maxs[ i ] - m < 8 ) {
			continue;
		}
		if ( m - mins[ i ] < 8 ) {
			continue;
		}

		// cut it
		float* v = verts + i;
		float dist[ 64 ];
		for ( int j = 0; j < numverts; j++, v += 3 ) {
			dist[ j ] = *v - m;
		}

		// wrap cases
		dist[ numverts ] = dist[ 0 ];
		v -= i;
		VectorCopy( verts, v );

		int f = 0;
		int b = 0;
		v = verts;
		vec3_t front[ 64 ], back[ 64 ];
		for ( int j = 0; j < numverts; j++, v += 3 ) {
			if ( dist[ j ] >= 0 ) {
				VectorCopy( v, front[ f ] );
				f++;
			}
			if ( dist[ j ] <= 0 ) {
				VectorCopy( v, back[ b ] );
				b++;
			}
			if ( dist[ j ] == 0 || dist[ j + 1 ] == 0 ) {
				continue;
			}
			if ( ( dist[ j ] > 0 ) != ( dist[ j + 1 ] > 0 ) ) {
				// clip point
				float frac = dist[ j ] / ( dist[ j ] - dist[ j + 1 ] );
				for ( int k = 0; k < 3; k++ ) {
					front[ f ][ k ] = back[ b ][ k ] = v[ k ] + frac * ( v[ 3 + k ] - v[ k ] );
				}
				f++;
				b++;
			}
		}

		SubdividePolygon( f, front[ 0 ] );
		SubdividePolygon( b, back[ 0 ] );
		return;
	}

	// add a point in the center to help keep warp valid
	mbrush38_glpoly_t* poly = ( mbrush38_glpoly_t* )Mem_Alloc( sizeof ( mbrush38_glpoly_t ) + ( ( numverts - 4 ) + 2 ) * BRUSH38_VERTEXSIZE * sizeof ( float ) );
	Com_Memset( poly, 0, sizeof ( mbrush38_glpoly_t ) + ( ( numverts - 4 ) + 2 ) * BRUSH38_VERTEXSIZE * sizeof ( float ) );
	poly->next = warpface->polys;
	warpface->polys = poly;
	poly->numverts = numverts + 2;
	vec3_t total;
	VectorClear( total );
	float total_s = 0;
	float total_t = 0;
	for ( int i = 0; i < numverts; i++, verts += 3 ) {
		VectorCopy( verts, poly->verts[ i + 1 ] );
		float s = DotProduct( verts, warpface->texinfo->vecs[ 0 ] ) / 64.0f;
		float t = DotProduct( verts, warpface->texinfo->vecs[ 1 ] ) / 64.0f;

		total_s += s;
		total_t += t;
		VectorAdd( total, verts, total );

		poly->verts[ i + 1 ][ 3 ] = s;
		poly->verts[ i + 1 ][ 4 ] = t;
	}

	VectorScale( total, ( 1.0 / numverts ), poly->verts[ 0 ] );
	poly->verts[ 0 ][ 3 ] = total_s / numverts;
	poly->verts[ 0 ][ 4 ] = total_t / numverts;

	// copy first vertex to last
	Com_Memcpy( poly->verts[ numverts + 1 ], poly->verts[ 1 ], sizeof ( poly->verts[ 0 ] ) );
}

//	Breaks a polygon up along axial 64 unit boundaries so that turbulent and
// sky warps can be done reasonably.
static void GL_SubdivideSurface( mbrush38_surface_t* fa ) {
	warpface = fa;

	//
	// convert edges back to a normal polygon
	//
	int numverts = 0;
	vec3_t verts[ 64 ];
	for ( int i = 0; i < fa->numedges; i++ ) {
		int lindex = loadmodel->brush38_surfedges[ fa->firstedge + i ];

		float* vec;
		if ( lindex > 0 ) {
			vec = loadmodel->brush38_vertexes[ loadmodel->brush38_edges[ lindex ].v[ 0 ] ].position;
		} else   {
			vec = loadmodel->brush38_vertexes[ loadmodel->brush38_edges[ -lindex ].v[ 1 ] ].position;
		}
		VectorCopy( vec, verts[ numverts ] );
		numverts++;
	}

	SubdividePolygon( numverts, verts[ 0 ] );
}

static void GL_BuildPolygonFromSurface( mbrush38_surface_t* fa ) {
	// reconstruct the polygon
	mbrush38_edge_t* pedges = tr.currentModel->brush38_edges;
	int lnumverts = fa->numedges;

	vec3_t total;
	VectorClear( total );
	//
	// draw texture
	//
	mbrush38_glpoly_t* poly = ( mbrush38_glpoly_t* )Mem_Alloc( sizeof ( mbrush38_glpoly_t ) + ( lnumverts - 4 ) * BRUSH38_VERTEXSIZE * sizeof ( float ) );
	Com_Memset( poly, 0, sizeof ( mbrush38_glpoly_t ) + ( lnumverts - 4 ) * BRUSH38_VERTEXSIZE * sizeof ( float ) );
	poly->next = fa->polys;
	fa->polys = poly;
	poly->numverts = lnumverts;

	for ( int i = 0; i < lnumverts; i++ ) {
		int lindex = tr.currentModel->brush38_surfedges[ fa->firstedge + i ];

		mbrush38_edge_t* r_pedge;
		float* vec;
		if ( lindex > 0 ) {
			r_pedge = &pedges[ lindex ];
			vec = tr.currentModel->brush38_vertexes[ r_pedge->v[ 0 ] ].position;
		} else   {
			r_pedge = &pedges[ -lindex ];
			vec = tr.currentModel->brush38_vertexes[ r_pedge->v[ 1 ] ].position;
		}
		float s = DotProduct( vec, fa->texinfo->vecs[ 0 ] ) + fa->texinfo->vecs[ 0 ][ 3 ];
		s /= fa->texinfo->image->width;

		float t = DotProduct( vec, fa->texinfo->vecs[ 1 ] ) + fa->texinfo->vecs[ 1 ][ 3 ];
		t /= fa->texinfo->image->height;

		VectorAdd( total, vec, total );
		VectorCopy( vec, poly->verts[ i ] );
		poly->verts[ i ][ 3 ] = s;
		poly->verts[ i ][ 4 ] = t;

		//
		// lightmap texture coordinates
		//
		s = DotProduct( vec, fa->texinfo->vecs[ 0 ] ) + fa->texinfo->vecs[ 0 ][ 3 ];
		s -= fa->texturemins[ 0 ];
		s += fa->light_s * 16;
		s += 8;
		s /= BLOCK_WIDTH * 16;	//fa->texinfo->texture->width;

		t = DotProduct( vec, fa->texinfo->vecs[ 1 ] ) + fa->texinfo->vecs[ 1 ][ 3 ];
		t -= fa->texturemins[ 1 ];
		t += fa->light_t * 16;
		t += 8;
		t /= BLOCK_HEIGHT * 16;	//fa->texinfo->texture->height;

		poly->verts[ i ][ 5 ] = s;
		poly->verts[ i ][ 6 ] = t;
	}

	poly->numverts = lnumverts;

}

static void Mod_LoadFaces( bsp38_lump_t* l ) {
	bsp38_dface_t* in = ( bsp38_dface_t* )( mod_base + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->Error( "MOD_LoadBmodel: funny lump size in %s", loadmodel->name );
	}
	int count = l->filelen / sizeof ( *in );
	mbrush38_surface_t* out = new mbrush38_surface_t[ count ];
	Com_Memset( out, 0, sizeof ( mbrush38_surface_t ) * count );

	loadmodel->brush38_surfaces = out;
	loadmodel->brush38_numsurfaces = count;

	tr.currentModel = loadmodel;

	GL_BeginBuildingLightmaps( loadmodel );

	for ( int surfnum = 0; surfnum < count; surfnum++, in++, out++ ) {
		out->surfaceType = SF_FACE_Q2;
		out->firstedge = LittleLong( in->firstedge );
		out->numedges = LittleShort( in->numedges );
		out->flags = 0;
		out->polys = NULL;

		int planenum = LittleShort( in->planenum );
		int side = LittleShort( in->side );
		if ( side ) {
			out->flags |= BRUSH38_SURF_PLANEBACK;
		}

		out->plane = loadmodel->brush38_planes + planenum;

		int ti = LittleShort( in->texinfo );
		if ( ti < 0 || ti >= loadmodel->brush38_numtexinfo ) {
			common->Error( "MOD_LoadBmodel: bad texinfo number" );
		}
		out->texinfo = loadmodel->brush38_texinfo + ti;

		CalcSurfaceExtents( out );

		// lighting info

		for ( int i = 0; i < BSP38_MAXLIGHTMAPS; i++ ) {
			out->styles[ i ] = in->styles[ i ];
		}
		int lightofs = LittleLong( in->lightofs );
		if ( lightofs == -1 ) {
			out->samples = NULL;
		} else   {
			out->samples = loadmodel->brush38_lightdata + lightofs;
		}

		// set the drawing flags

		if ( out->texinfo->flags & BSP38SURF_WARP ) {
			out->flags |= BRUSH38_SURF_DRAWTURB;
			for ( int i = 0; i < 2; i++ ) {
				out->extents[ i ] = 16384;
				out->texturemins[ i ] = -8192;
			}
			GL_SubdivideSurface( out );		// cut up polygon for warps
		}

		// create lightmaps and polygons
		if ( !( out->texinfo->flags & ( BSP38SURF_SKY | BSP38SURF_TRANS33 | BSP38SURF_TRANS66 | BSP38SURF_WARP ) ) ) {
			GL_CreateSurfaceLightmapQ2( out );
		}

		if ( !( out->texinfo->flags & BSP38SURF_WARP ) ) {
			GL_BuildPolygonFromSurface( out );
		}
	}

	GL_EndBuildingLightmaps();
}

static void Mod_SetParent( mbrush38_node_t* node, mbrush38_node_t* parent ) {
	node->parent = parent;
	if ( node->contents != -1 ) {
		return;
	}
	Mod_SetParent( node->children[ 0 ], node );
	Mod_SetParent( node->children[ 1 ], node );
}

static void Mod_LoadNodes( bsp38_lump_t* l ) {
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
			} else   {
				out->children[ j ] = ( mbrush38_node_t* )( loadmodel->brush38_leafs + ( -1 - p ) );
			}
		}
	}

	Mod_SetParent( loadmodel->brush38_nodes, NULL );	// sets nodes and leafs
}

static void Mod_LoadLeafs( bsp38_lump_t* l ) {
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

static void Mod_LoadMarksurfaces( bsp38_lump_t* l ) {
	short* in = ( short* )( mod_base + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->Error( "MOD_LoadBmodel: funny lump size in %s", loadmodel->name );
	}
	int count = l->filelen / sizeof ( *in );
	mbrush38_surface_t** out = new mbrush38_surface_t*[ count ];

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

static void Mod_LoadSurfedges( bsp38_lump_t* l ) {
	int* in = ( int* )( mod_base + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->Error( "MOD_LoadBmodel: funny lump size in %s", loadmodel->name );
	}
	int count = l->filelen / sizeof ( *in );
	if ( count < 1 || count >= BSP38MAX_MAP_SURFEDGES ) {
		common->Error( "MOD_LoadBmodel: bad surfedges count in %s: %i",
			loadmodel->name, count );
	}

	int* out = new int[ count ];

	loadmodel->brush38_surfedges = out;
	loadmodel->brush38_numsurfedges = count;

	for ( int i = 0; i < count; i++ ) {
		out[ i ] = LittleLong( in[ i ] );
	}
}

static void Mod_LoadPlanes( bsp38_lump_t* l ) {
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

static void Mod_LoadSubmodels( bsp38_lump_t* l ) {
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

void Mod_LoadBrush38Model( model_t* mod, void* buffer ) {
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

	Mod_LoadVertexes( &header->lumps[ BSP38LUMP_VERTEXES ] );
	Mod_LoadEdges( &header->lumps[ BSP38LUMP_EDGES ] );
	Mod_LoadSurfedges( &header->lumps[ BSP38LUMP_SURFEDGES ] );
	Mod_LoadLighting( &header->lumps[ BSP38LUMP_LIGHTING ] );
	Mod_LoadPlanes( &header->lumps[ BSP38LUMP_PLANES ] );
	Mod_LoadTexinfo( &header->lumps[ BSP38LUMP_TEXINFO ] );
	Mod_LoadFaces( &header->lumps[ BSP38LUMP_FACES ] );
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
		model_t* starmod;

		mbrush38_model_t* bm = &mod->brush38_submodels[ i ];
		if ( i == 0 ) {
			starmod = loadmodel;
		} else   {
			starmod = R_AllocModel();

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

void Mod_FreeBsp38( model_t* mod ) {
	if ( mod->name[ 0 ] == '*' ) {
		return;
	}

	if ( mod->brush38_lightdata ) {
		delete[] mod->brush38_lightdata;
	}
	if ( mod->brush38_vis ) {
		Mem_Free( mod->brush38_vis );
	}
	delete[] mod->brush38_vertexes;
	delete[] mod->brush38_edges;
	delete[] mod->brush38_texinfo;
	for ( int i = 0; i < mod->brush38_numsurfaces; i++ ) {
		mbrush38_glpoly_t* poly = mod->brush38_surfaces[ i ].polys;
		while ( poly ) {
			mbrush38_glpoly_t* tmp = poly;
			poly = poly->next;
			Mem_Free( tmp );
		}
	}
	delete[] mod->brush38_surfaces;
	delete[] mod->brush38_nodes;
	delete[] mod->brush38_leafs;
	delete[] mod->brush38_marksurfaces;
	delete[] mod->brush38_surfedges;
	delete[] mod->brush38_planes;
	delete[] mod->brush38_submodels;
}

static byte* Mod_DecompressVis( byte* in, model_t* model ) {
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

byte* Mod_ClusterPVS( int cluster, model_t* model ) {
	if ( cluster == -1 || !model->brush38_vis ) {
		return mod_novis;
	}
	return Mod_DecompressVis( ( byte* )model->brush38_vis + model->brush38_vis->bitofs[ cluster ][ BSP38DVIS_PVS ], model );
}

mbrush38_leaf_t* Mod_PointInLeafQ2( vec3_t p, model_t* model ) {
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
		} else   {
			node = node->children[ 1 ];
		}
	}

	return NULL;	// never reached
}
