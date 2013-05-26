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
#include "../sky.h"
#include "../cvars.h"
#include "../../../common/Common.h"
#include "../../../common/common_defs.h"
#include "../../../common/strings.h"
#include "../../../common/content_types.h"
#include "../../../common/endian.h"

#define SUBDIVIDE_SIZE  64

mbrush29_texture_t* r_notexture_mip;

static byte* mod_base;

static mbrush29_surface_t* warpface;

static byte mod_novis[ BSP29_MAX_MAP_LEAFS / 8 ];

void R_InitBsp29NoTextureMip() {
	// create a simple checkerboard texture for the default
	r_notexture_mip = ( mbrush29_texture_t* )Mem_Alloc( sizeof ( mbrush29_texture_t ) + 16 * 16 + 8 * 8 + 4 * 4 + 2 * 2 );

	r_notexture_mip->width = r_notexture_mip->height = 16;
	r_notexture_mip->offsets[ 0 ] = sizeof ( mbrush29_texture_t );
	r_notexture_mip->offsets[ 1 ] = r_notexture_mip->offsets[ 0 ] + 16 * 16;
	r_notexture_mip->offsets[ 2 ] = r_notexture_mip->offsets[ 1 ] + 8 * 8;
	r_notexture_mip->offsets[ 3 ] = r_notexture_mip->offsets[ 2 ] + 4 * 4;

	for ( int m = 0; m < 4; m++ ) {
		byte* dest = ( byte* )r_notexture_mip + r_notexture_mip->offsets[ m ];
		for ( int y = 0; y < ( 16 >> m ); y++ ) {
			for ( int x = 0; x < ( 16 >> m ); x++ ) {
				if ( ( y < ( 8 >> m ) ) ^ ( x < ( 8 >> m ) ) ) {
					*dest++ = 0;
				} else {
					*dest++ = 0xff;
				}
			}
		}
	}
}

static void Mod_LoadTextures( bsp29_lump_t* l ) {
	if ( !l->filelen ) {
		loadmodel->brush29_textures = NULL;
		return;
	}
	bsp29_dmiptexlump_t* m = ( bsp29_dmiptexlump_t* )( mod_base + l->fileofs );

	m->nummiptex = LittleLong( m->nummiptex );

	loadmodel->brush29_numtextures = m->nummiptex;
	loadmodel->brush29_textures = new mbrush29_texture_t*[ m->nummiptex ];
	Com_Memset( loadmodel->brush29_textures, 0, sizeof ( mbrush29_texture_t* ) * m->nummiptex );

	for ( int i = 0; i < m->nummiptex; i++ ) {
		m->dataofs[ i ] = LittleLong( m->dataofs[ i ] );
		if ( m->dataofs[ i ] == -1 ) {
			continue;
		}
		bsp29_miptex_t* mt = ( bsp29_miptex_t* )( ( byte* )m + m->dataofs[ i ] );
		mt->width = LittleLong( mt->width );
		mt->height = LittleLong( mt->height );
		for ( int j = 0; j < BSP29_MIPLEVELS; j++ ) {
			mt->offsets[ j ] = LittleLong( mt->offsets[ j ] );
		}

		if ( ( mt->width & 15 ) || ( mt->height & 15 ) ) {
			common->FatalError( "Texture %s is not 16 aligned", mt->name );
		}
		int pixels = mt->width * mt->height / 64 * 85;
		mbrush29_texture_t* tx = ( mbrush29_texture_t* )Mem_Alloc( sizeof ( mbrush29_texture_t ) + pixels );
		Com_Memset( tx, 0, sizeof ( mbrush29_texture_t ) + pixels );
		loadmodel->brush29_textures[ i ] = tx;

		Com_Memcpy( tx->name, mt->name, sizeof ( tx->name ) );
		tx->width = mt->width;
		tx->height = mt->height;
		for ( int j = 0; j < BSP29_MIPLEVELS; j++ ) {
			tx->offsets[ j ] = mt->offsets[ j ] + sizeof ( mbrush29_texture_t ) - sizeof ( bsp29_miptex_t );
		}
		// the pixels immediately follow the structures
		Com_Memcpy( tx + 1, mt + 1, pixels );

		if ( !String::NCmp( mt->name,"sky",3 ) ) {
			tx->shader = R_InitSky( tx );
		} else {
			// see if the texture is allready present
			char search[ 64 ];
			sprintf( search, "%s_%d_%d", mt->name, tx->width, tx->height );
			tx->gl_texture = R_FindImage( search );

			char searchFullBright[ 64 ];
			sprintf( searchFullBright, "%s_%d_%d_fb", mt->name, tx->width, tx->height );
			tx->fullBrightTexture = r_fullBrightColours->integer ? R_FindImage( searchFullBright ) : NULL;

			if ( !tx->gl_texture ) {
				byte* pic32 = R_ConvertImage8To32( ( byte* )( tx + 1 ), tx->width, tx->height, IMG8MODE_Normal );
				byte* picFullBright = R_GetFullBrightImage( ( byte* )( tx + 1 ), pic32, tx->width, tx->height );
				tx->gl_texture = R_CreateImage( search, pic32, tx->width, tx->height, true, true, GL_REPEAT );
				delete[] pic32;
				if ( picFullBright ) {
					tx->fullBrightTexture = R_CreateImage( searchFullBright, picFullBright, tx->width, tx->height, true, true, GL_REPEAT );
					delete[] picFullBright;
				} else {
					tx->fullBrightTexture = NULL;
				}
			}
			if ( mt->name[ 0 ] == '*' ) {
				tx->shader = R_BuildBsp29WarpShader( tx->name, tx->gl_texture );
			}
		}
	}

	//
	// sequence the animations
	//
	for ( int i = 0; i < m->nummiptex; i++ ) {
		mbrush29_texture_t* tx = loadmodel->brush29_textures[ i ];
		if ( !tx || tx->name[ 0 ] != '+' ) {
			continue;
		}
		if ( tx->anim_next ) {
			continue;	// allready sequenced
		}

		// find the number of frames in the animation
		mbrush29_texture_t* anims[ 10 ];
		mbrush29_texture_t* altanims[ 10 ];
		Com_Memset( anims, 0, sizeof ( anims ) );
		Com_Memset( altanims, 0, sizeof ( altanims ) );

		int max = tx->name[ 1 ];
		int altmax = 0;
		if ( max >= 'a' && max <= 'z' ) {
			max -= 'a' - 'A';
		}
		if ( max >= '0' && max <= '9' ) {
			max -= '0';
			altmax = 0;
			anims[ max ] = tx;
			max++;
		} else if ( max >= 'A' && max <= 'J' ) {
			altmax = max - 'A';
			max = 0;
			altanims[ altmax ] = tx;
			altmax++;
		} else {
			common->FatalError( "Bad animating texture %s", tx->name );
		}

		for ( int j = i + 1; j < m->nummiptex; j++ ) {
			mbrush29_texture_t* tx2 = loadmodel->brush29_textures[ j ];
			if ( !tx2 || tx2->name[ 0 ] != '+' ) {
				continue;
			}
			if ( String::Cmp( tx2->name + 2, tx->name + 2 ) ) {
				continue;
			}

			int num = tx2->name[ 1 ];
			if ( num >= 'a' && num <= 'z' ) {
				num -= 'a' - 'A';
			}
			if ( num >= '0' && num <= '9' ) {
				num -= '0';
				anims[ num ] = tx2;
				if ( num + 1 > max ) {
					max = num + 1;
				}
			} else if ( num >= 'A' && num <= 'J' ) {
				num = num - 'A';
				altanims[ num ] = tx2;
				if ( num + 1 > altmax ) {
					altmax = num + 1;
				}
			} else {
				common->FatalError( "Bad animating texture %s", tx->name );
			}
		}

		// link them all together
		for ( int j = 0; j < max; j++ ) {
			mbrush29_texture_t* tx2 = anims[ j ];
			if ( !tx2 ) {
				common->FatalError( "Missing frame %i of %s", j, tx->name );
			}
			tx2->anim_total = max;
			tx2->anim_next = anims[ ( j + 1 ) % max ];
			tx2->anim_base = anims[ 0 ];
			if ( altmax ) {
				tx2->alternate_anims = altanims[ 0 ];
			}
		}
		for ( int j = 0; j < altmax; j++ ) {
			mbrush29_texture_t* tx2 = altanims[ j ];
			if ( !tx2 ) {
				common->FatalError( "Missing frame %i of %s", j, tx->name );
			}
			tx2->anim_total = altmax;
			tx2->anim_next = altanims[ ( j + 1 ) % altmax ];
			tx2->anim_base = altanims[ 0 ];
			if ( max ) {
				tx2->alternate_anims = anims[ 0 ];
			}
		}
	}
}

static void Mod_LoadLighting( bsp29_lump_t* l ) {
	if ( !l->filelen ) {
		loadmodel->brush29_lightdata = NULL;
		return;
	}
	loadmodel->brush29_lightdata = new byte[ l->filelen ];
	Com_Memcpy( loadmodel->brush29_lightdata, mod_base + l->fileofs, l->filelen );
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

static void Mod_LoadVertexes( bsp29_lump_t* l ) {
	bsp29_dvertex_t* in = ( bsp29_dvertex_t* )( mod_base + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->FatalError( "MOD_LoadBmodel: funny lump size in %s", loadmodel->name );
	}
	int count = l->filelen / sizeof ( *in );
	mbrush29_vertex_t* out = new mbrush29_vertex_t[ count ];

	loadmodel->brush29_vertexes = out;
	loadmodel->brush29_numvertexes = count;

	for ( int i = 0; i < count; i++, in++, out++ ) {
		out->position[ 0 ] = LittleFloat( in->point[ 0 ] );
		out->position[ 1 ] = LittleFloat( in->point[ 1 ] );
		out->position[ 2 ] = LittleFloat( in->point[ 2 ] );
	}
}

static void Mod_LoadEdges( bsp29_lump_t* l ) {
	bsp29_dedge_t* in = ( bsp29_dedge_t* )( mod_base + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->FatalError( "MOD_LoadBmodel: funny lump size in %s",loadmodel->name );
	}
	int count = l->filelen / sizeof ( *in );
	mbrush29_edge_t* out = new mbrush29_edge_t[ count + 1 ];
	Com_Memset( out, 0, sizeof ( mbrush29_edge_t ) * ( count + 1 ) );

	loadmodel->brush29_edges = out;
	loadmodel->brush29_numedges = count;

	for ( int i = 0; i < count; i++, in++, out++ ) {
		out->v[ 0 ] = ( unsigned short )LittleShort( in->v[ 0 ] );
		out->v[ 1 ] = ( unsigned short )LittleShort( in->v[ 1 ] );
	}
}

static void Mod_LoadTexinfo( bsp29_lump_t* l ) {
	bsp29_texinfo_t* in = ( bsp29_texinfo_t* )( mod_base + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->FatalError( "MOD_LoadBmodel: funny lump size in %s", loadmodel->name );
	}
	int count = l->filelen / sizeof ( *in );
	mbrush29_texinfo_t* out = new mbrush29_texinfo_t[ count ];
	Com_Memset( out, 0, sizeof ( mbrush29_texinfo_t ) * count );

	loadmodel->brush29_texinfo = out;
	loadmodel->brush29_numtexinfo = count;

	for ( int i = 0; i < count; i++, in++, out++ ) {
		for ( int j = 0; j < 8; j++ ) {
			out->vecs[ 0 ][ j ] = LittleFloat( in->vecs[ 0 ][ j ] );
		}
		float len1 = VectorLength( out->vecs[ 0 ] );
		float len2 = VectorLength( out->vecs[ 1 ] );
		len1 = ( len1 + len2 ) / 2;
		if ( len1 < 0.32 ) {
			out->mipadjust = 4;
		} else if ( len1 < 0.49 ) {
			out->mipadjust = 3;
		} else if ( len1 < 0.99 ) {
			out->mipadjust = 2;
		} else {
			out->mipadjust = 1;
		}

		int miptex = LittleLong( in->miptex );
		out->flags = LittleLong( in->flags );

		if ( !loadmodel->brush29_textures ) {
			out->texture = r_notexture_mip;	// checkerboard texture
			out->flags = 0;
		} else {
			if ( miptex >= loadmodel->brush29_numtextures ) {
				common->FatalError( "miptex >= loadmodel->numtextures" );
			}
			out->texture = loadmodel->brush29_textures[ miptex ];
			if ( !out->texture ) {
				out->texture = r_notexture_mip;	// texture not found
				out->flags = 0;
			}
		}
	}
}

//	Fills in s->texturemins[] and s->extents[]
static void CalcSurfaceExtents( mbrush29_surface_t* s ) {
	float mins[ 2 ], maxs[ 2 ];
	mins[ 0 ] = mins[ 1 ] = 999999;
	maxs[ 0 ] = maxs[ 1 ] = -99999;

	mbrush29_texinfo_t* tex = s->texinfo;

	for ( int i = 0; i < s->numedges; i++ ) {
		int e = loadmodel->brush29_surfedges[ s->firstedge + i ];
		mbrush29_vertex_t* v;
		if ( e >= 0 ) {
			v = &loadmodel->brush29_vertexes[ loadmodel->brush29_edges[ e ].v[ 0 ] ];
		} else {
			v = &loadmodel->brush29_vertexes[ loadmodel->brush29_edges[ -e ].v[ 1 ] ];
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
		if ( !( tex->flags & BSP29TEX_SPECIAL ) && s->extents[ i ] > 512 ) {
			common->FatalError( "Bad surface extents" );
		}
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
		common->FatalError( "numverts = %i", numverts );
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

	mbrush29_glpoly_t* poly = ( mbrush29_glpoly_t* )Mem_Alloc( sizeof ( mbrush29_glpoly_t ) + ( numverts - 4 ) * BRUSH29_VERTEXSIZE * sizeof ( float ) );
	poly->next = warpface->polys;
	warpface->polys = poly;
	poly->numverts = numverts;
	for ( int i = 0; i < numverts; i++, verts += 3 ) {
		VectorCopy( verts, poly->verts[ i ] );
		float s = DotProduct( verts, warpface->texinfo->vecs[ 0 ] ) / 64.0f;
		float t = DotProduct( verts, warpface->texinfo->vecs[ 1 ] ) / 64.0f;
		poly->verts[ i ][ 3 ] = s;
		poly->verts[ i ][ 4 ] = t;
	}
}

//	Breaks a polygon up along axial 64 unit boundaries so that turbulent and
// sky warps can be done reasonably.
static void GL_SubdivideSurface( mbrush29_surface_t* fa ) {
	warpface = fa;

	//
	// convert edges back to a normal polygon
	//
	vec3_t verts[ 64 ];
	int numverts = 0;
	for ( int i = 0; i < fa->numedges; i++ ) {
		int lindex = loadmodel->brush29_surfedges[ fa->firstedge + i ];

		float* vec;
		if ( lindex > 0 ) {
			vec = loadmodel->brush29_vertexes[ loadmodel->brush29_edges[ lindex ].v[ 0 ] ].position;
		} else {
			vec = loadmodel->brush29_vertexes[ loadmodel->brush29_edges[ -lindex ].v[ 1 ] ].position;
		}
		VectorCopy( vec, verts[ numverts ] );
		numverts++;
	}

	SubdividePolygon( numverts, verts[ 0 ] );
}

static void Mod_LoadFaces( bsp29_lump_t* l ) {
	bsp29_dface_t* in = ( bsp29_dface_t* )( mod_base + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->FatalError( "MOD_LoadBmodel: funny lump size in %s", loadmodel->name );
	}
	int count = l->filelen / sizeof ( *in );
	mbrush29_surface_t* out = new mbrush29_surface_t[ count ];
	Com_Memset( out, 0, sizeof ( mbrush29_surface_t ) * count );

	loadmodel->brush29_surfaces = out;
	loadmodel->brush29_numsurfaces = count;

	for ( int surfnum = 0; surfnum < count; surfnum++, in++, out++ ) {
		out->surfaceType = SF_FACE_Q1;
		out->firstedge = LittleLong( in->firstedge );
		out->numedges = LittleShort( in->numedges );
		out->flags = 0;

		int planenum = LittleShort( in->planenum );
		int side = LittleShort( in->side );
		if ( side ) {
			out->flags |= BRUSH29_SURF_PLANEBACK;
		}

		out->plane = loadmodel->brush29_planes + planenum;

		out->texinfo = loadmodel->brush29_texinfo + LittleShort( in->texinfo );

		CalcSurfaceExtents( out );

		// lighting info

		for ( int i = 0; i < BSP29_MAXLIGHTMAPS; i++ ) {
			out->styles[ i ] = in->styles[ i ];
		}
		int lightofs = LittleLong( in->lightofs );
		if ( lightofs == -1 ) {
			out->samples = NULL;
		} else {
			out->samples = loadmodel->brush29_lightdata + lightofs;
		}

		// set the drawing flags flag

		if ( !String::NCmp( out->texinfo->texture->name, "sky", 3 ) ) {	// sky
			out->flags |= ( BRUSH29_SURF_DRAWSKY | BRUSH29_SURF_DRAWTILED );
			out->shader = out->texinfo->texture->shader;
			out->altShader = out->texinfo->texture->shader;
			GL_SubdivideSurface( out );		// cut up polygon for warps
			continue;
		}

		if ( out->texinfo->texture->name[ 0 ] == '*' ) {	// turbulent
			out->flags |= ( BRUSH29_SURF_DRAWTURB | BRUSH29_SURF_DRAWTILED );
			out->shader = out->texinfo->texture->shader;
			out->altShader = out->texinfo->texture->shader;
			for ( int i = 0; i < 2; i++ ) {
				out->extents[ i ] = 16384;
				out->texturemins[ i ] = -8192;
			}
			GL_SubdivideSurface( out );		// cut up polygon for warps

			continue;
		}
	}
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
	mbrush29_surface_t** out = new mbrush29_surface_t*[ count ];

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

static void Mod_LoadSurfedges( bsp29_lump_t* l ) {
	int* in = ( int* )( mod_base + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->FatalError( "MOD_LoadBmodel: funny lump size in %s", loadmodel->name );
	}
	int count = l->filelen / sizeof ( *in );
	int* out = new int[ count ];

	loadmodel->brush29_surfedges = out;
	loadmodel->brush29_numsurfedges = count;

	for ( int i = 0; i < count; i++ ) {
		out[ i ] = LittleLong( in[ i ] );
	}
}

static void Mod_LoadPlanes( bsp29_lump_t* l ) {
	bsp29_dplane_t* in = ( bsp29_dplane_t* )( mod_base + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->FatalError( "MOD_LoadBmodel: funny lump size in %s", loadmodel->name );
	}
	int count = l->filelen / sizeof ( *in );
	cplane_t* out = new cplane_t[ count * 2 ];
	Com_Memset( out, 0, sizeof ( cplane_t ) * ( count * 2 ) );

	loadmodel->brush29_planes = out;
	loadmodel->brush29_numplanes = count;

	for ( int i = 0; i < count; i++, in++, out++ ) {
		for ( int j = 0; j < 3; j++ ) {
			out->normal[ j ] = LittleFloat( in->normal[ j ] );
		}
		out->dist = LittleFloat( in->dist );
		out->type = LittleLong( in->type );

		SetPlaneSignbits( out );
	}
}

static void Mod_LoadSubmodelsQ1( bsp29_lump_t* l ) {
	bsp29_dmodel_q1_t* in = ( bsp29_dmodel_q1_t* )( mod_base + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->FatalError( "MOD_LoadBmodel: funny lump size in %s", loadmodel->name );
	}
	int count = l->filelen / sizeof ( *in );
	mbrush29_submodel_t* out = new mbrush29_submodel_t[ count ];

	loadmodel->brush29_submodels = out;
	loadmodel->brush29_numsubmodels = count;

	for ( int i = 0; i < count; i++, in++, out++ ) {
		for ( int j = 0; j < 3; j++ ) {
			// spread the mins / maxs by a pixel
			out->mins[ j ] = LittleFloat( in->mins[ j ] ) - 1;
			out->maxs[ j ] = LittleFloat( in->maxs[ j ] ) + 1;
			out->origin[ j ] = LittleFloat( in->origin[ j ] );
		}
		out->headnode = LittleLong( in->headnode[ 0 ] );
		out->visleafs = LittleLong( in->visleafs );
		out->firstface = LittleLong( in->firstface );
		out->numfaces = LittleLong( in->numfaces );
	}
}

static void Mod_LoadSubmodelsH2( bsp29_lump_t* l ) {
	bsp29_dmodel_h2_t* in = ( bsp29_dmodel_h2_t* )( mod_base + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->FatalError( "MOD_LoadBmodel: funny lump size in %s", loadmodel->name );
	}
	int count = l->filelen / sizeof ( *in );
	mbrush29_submodel_t* out = new mbrush29_submodel_t[ count ];

	loadmodel->brush29_submodels = out;
	loadmodel->brush29_numsubmodels = count;

	for ( int i = 0; i < count; i++, in++, out++ ) {
		for ( int j = 0; j < 3; j++ ) {
			// spread the mins / maxs by a pixel
			out->mins[ j ] = LittleFloat( in->mins[ j ] ) - 1;
			out->maxs[ j ] = LittleFloat( in->maxs[ j ] ) + 1;
			out->origin[ j ] = LittleFloat( in->origin[ j ] );
		}
		out->headnode = LittleLong( in->headnode[ 0 ] );
		out->visleafs = LittleLong( in->visleafs );
		out->firstface = LittleLong( in->firstface );
		out->numfaces = LittleLong( in->numfaces );
	}
}

void Mod_LoadBrush29Model( idRenderModel* mod, void* buffer ) {
	Com_Memset( mod_novis, 0xff, sizeof ( mod_novis ) );

	loadmodel->type = MOD_BRUSH29;

	bsp29_dheader_t* header = ( bsp29_dheader_t* )buffer;

	int version = LittleLong( header->version );
	if ( version != BSP29_VERSION ) {
		common->FatalError( "Mod_LoadBrush29Model: %s has wrong version number (%i should be %i)", mod->name, version, BSP29_VERSION );
	}

	// swap all the lumps
	mod_base = ( byte* )header;

	for ( int i = 0; i < ( int )sizeof ( bsp29_dheader_t ) / 4; i++ ) {
		( ( int* )header )[ i ] = LittleLong( ( ( int* )header )[ i ] );
	}

	// load into heap

	Mod_LoadVertexes( &header->lumps[ BSP29LUMP_VERTEXES ] );
	Mod_LoadEdges( &header->lumps[ BSP29LUMP_EDGES ] );
	Mod_LoadSurfedges( &header->lumps[ BSP29LUMP_SURFEDGES ] );
	Mod_LoadTextures( &header->lumps[ BSP29LUMP_TEXTURES ] );
	Mod_LoadLighting( &header->lumps[ BSP29LUMP_LIGHTING ] );
	Mod_LoadPlanes( &header->lumps[ BSP29LUMP_PLANES ] );
	Mod_LoadTexinfo( &header->lumps[ BSP29LUMP_TEXINFO ] );
	Mod_LoadFaces( &header->lumps[ BSP29LUMP_FACES ] );
	Mod_LoadMarksurfaces( &header->lumps[ BSP29LUMP_MARKSURFACES ] );
	Mod_LoadVisibility( &header->lumps[ BSP29LUMP_VISIBILITY ] );
	Mod_LoadLeafs( &header->lumps[ BSP29LUMP_LEAFS ] );
	Mod_LoadNodes( &header->lumps[ BSP29LUMP_NODES ] );
	Mod_LoadEntities( &header->lumps[ BSP29LUMP_ENTITIES ] );
	if ( GGameType & GAME_Hexen2 ) {
		Mod_LoadSubmodelsH2( &header->lumps[ BSP29LUMP_MODELS ] );
	} else {
		Mod_LoadSubmodelsQ1( &header->lumps[ BSP29LUMP_MODELS ] );
	}

	mod->q1_numframes = 2;		// regular and alternate animation

	//
	// set up the submodels (FIXME: this is confusing)
	//
	for ( int i = 0; i < mod->brush29_numsubmodels; i++ ) {
		mbrush29_submodel_t* bm = &mod->brush29_submodels[ i ];

		mod->brush29_firstnode = bm->headnode;
		mod->brush29_firstmodelsurface = bm->firstface;
		mod->brush29_nummodelsurfaces = bm->numfaces;

		VectorCopy( bm->maxs, mod->q1_maxs );
		VectorCopy( bm->mins, mod->q1_mins );

		mod->q1_radius = RadiusFromBounds( mod->q1_mins, mod->q1_maxs );

		mod->brush29_numleafs = bm->visleafs;

		if ( i < mod->brush29_numsubmodels - 1 ) {
			// duplicate the basic information
			loadmodel = R_AllocModel();
			int saved_index = loadmodel->index;
			*loadmodel = *mod;
			loadmodel->index = saved_index;
			String::Sprintf( loadmodel->name, sizeof ( loadmodel->name ), "*%i", i + 1 );
			mod = loadmodel;
		}
	}
}

void Mod_FreeBsp29( idRenderModel* mod ) {
	if ( mod->name[ 0 ] == '*' ) {
		return;
	}

	if ( mod->brush29_textures ) {
		for ( int i = 0; i < mod->brush29_numtextures; i++ ) {
			if ( mod->brush29_textures[ i ] ) {
				Mem_Free( mod->brush29_textures[ i ] );
			}
		}
		delete[] mod->brush29_textures;
	}
	if ( mod->brush29_lightdata ) {
		delete[] mod->brush29_lightdata;
	}
	if ( mod->brush29_visdata ) {
		delete[] mod->brush29_visdata;
	}
	if ( mod->brush29_entities ) {
		delete[] mod->brush29_entities;
	}
	delete[] mod->brush29_vertexes;
	delete[] mod->brush29_edges;
	delete[] mod->brush29_texinfo;
	for ( int i = 0; i < mod->brush29_numsurfaces; i++ ) {
		mbrush29_glpoly_t* poly = mod->brush29_surfaces[ i ].polys;
		while ( poly ) {
			mbrush29_glpoly_t* tmp = poly;
			poly = poly->next;
			Mem_Free( tmp );
		}
	}
	delete[] mod->brush29_surfaces;
	delete[] mod->brush29_nodes;
	delete[] mod->brush29_leafs;
	delete[] mod->brush29_marksurfaces;
	delete[] mod->brush29_surfedges;
	delete[] mod->brush29_planes;
	delete[] mod->brush29_submodels;
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

mbrush29_leaf_t* Mod_PointInLeafQ1( vec3_t p, idRenderModel* model ) {
	if ( !model || !model->brush29_nodes ) {
		common->FatalError( "Mod_PointInLeafQ1: bad model" );
	}

	mbrush29_node_t* node = model->brush29_nodes;
	while ( 1 ) {
		if ( node->contents < 0 ) {
			return ( mbrush29_leaf_t* )node;
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
