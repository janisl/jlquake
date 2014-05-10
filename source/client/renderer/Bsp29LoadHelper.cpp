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

#include "Bsp29LoadHelper.h"
#include "../../common/Common.h"
#include "../../common/endian.h"
#include "../../common/strings.h"
#include "sky.h"
#include "models/model.h"
#include "cvars.h"
#include "surfaces.h"

idBsp29LoadHelper::idBsp29LoadHelper( const idStr& name, byte* fileBase ) : idBspSurfaceBuilder( name, fileBase ) {
	numplanes = 0;
	planes = NULL;
	lightdata = NULL;
	numtextures = 0;
	textures = NULL;
	numtexinfo = 0;
	texinfo = NULL;
	textureInfos = NULL;
	numsurfaces = 0;
	surfaces = NULL;
	numsubmodels = 0;
	submodels = NULL;
}

idBsp29LoadHelper::~idBsp29LoadHelper() {
}

void idBsp29LoadHelper::LoadVertexes( bsp_lump_t* l ) {
	bsp_vertex_t* in = ( bsp_vertex_t* )( fileBase + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->FatalError( "MOD_LoadBmodel: funny lump size in %s", name.CStr() );
	}
	int count = l->filelen / sizeof ( *in );
	mbrush_vertex_t* out = new mbrush_vertex_t[ count ];

	vertexes = out;
	numvertexes = count;

	for ( int i = 0; i < count; i++, in++, out++ ) {
		out->position[ 0 ] = LittleFloat( in->point[ 0 ] );
		out->position[ 1 ] = LittleFloat( in->point[ 1 ] );
		out->position[ 2 ] = LittleFloat( in->point[ 2 ] );
	}
}

void idBsp29LoadHelper::LoadEdges( bsp_lump_t* l ) {
	bsp_edge_t* in = ( bsp_edge_t* )( fileBase + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->FatalError( "MOD_LoadBmodel: funny lump size in %s", name.CStr() );
	}
	int count = l->filelen / sizeof ( *in );
	mbrush_edge_t* out = new mbrush_edge_t[ count + 1 ];
	Com_Memset( out, 0, sizeof ( mbrush_edge_t ) * ( count + 1 ) );

	edges = out;
	numedges = count;

	for ( int i = 0; i < count; i++, in++, out++ ) {
		out->v[ 0 ] = ( unsigned short )LittleShort( in->v[ 0 ] );
		out->v[ 1 ] = ( unsigned short )LittleShort( in->v[ 1 ] );
	}
}

void idBsp29LoadHelper::LoadSurfedges( bsp_lump_t* l ) {
	int* in = ( int* )( fileBase + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->FatalError( "MOD_LoadBmodel: funny lump size in %s", name.CStr() );
	}
	int count = l->filelen / sizeof ( *in );
	int* out = new int[ count ];

	surfedges = out;
	numsurfedges = count;

	for ( int i = 0; i < count; i++ ) {
		out[ i ] = LittleLong( in[ i ] );
	}
}

void idBsp29LoadHelper::LoadPlanes( bsp_lump_t* l ) {
	bsp29_dplane_t* in = ( bsp29_dplane_t* )( fileBase + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->FatalError( "MOD_LoadBmodel: funny lump size in %s", name.CStr() );
	}
	int count = l->filelen / sizeof ( *in );
	cplane_t* out = new cplane_t[ count * 2 ];
	Com_Memset( out, 0, sizeof ( cplane_t ) * ( count * 2 ) );

	planes = out;
	numplanes = count;

	for ( int i = 0; i < count; i++, in++, out++ ) {
		for ( int j = 0; j < 3; j++ ) {
			out->normal[ j ] = LittleFloat( in->normal[ j ] );
		}
		out->dist = LittleFloat( in->dist );
		out->type = LittleLong( in->type );

		SetPlaneSignbits( out );
	}
}

void idBsp29LoadHelper::LoadLighting( bsp_lump_t* l ) {
	if ( !l->filelen ) {
		lightdata = NULL;
		return;
	}
	lightdata = new byte[ l->filelen ];
	Com_Memcpy( lightdata, fileBase + l->fileofs, l->filelen );
}

void idBsp29LoadHelper::LoadTextures( bsp_lump_t* l ) {
	if ( !l->filelen ) {
		textures = NULL;
		return;
	}
	bsp29_dmiptexlump_t* m = ( bsp29_dmiptexlump_t* )( fileBase + l->fileofs );

	m->nummiptex = LittleLong( m->nummiptex );

	numtextures = m->nummiptex;
	textures = new mbrush29_texture_t*[ m->nummiptex ];
	Com_Memset( textures, 0, sizeof ( mbrush29_texture_t* ) * m->nummiptex );

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
		textures[ i ] = tx;

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
		mbrush29_texture_t* tx = textures[ i ];
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
			mbrush29_texture_t* tx2 = textures[ j ];
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

void idBsp29LoadHelper::LoadTexinfo( bsp_lump_t* l ) {
	bsp29_texinfo_t* in = ( bsp29_texinfo_t* )( fileBase + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->FatalError( "MOD_LoadBmodel: funny lump size in %s", name.CStr() );
	}
	int count = l->filelen / sizeof ( *in );
	mbrush29_texinfo_t* _out = new mbrush29_texinfo_t[ count ];
	Com_Memset( _out, 0, sizeof ( mbrush29_texinfo_t ) * count );
	idTextureInfo* out = new idTextureInfo[ count ];

	texinfo = _out;
	textureInfos = out;
	numtexinfo = count;

	for ( int i = 0; i < count; i++, in++, out++, _out++ ) {
		for ( int j = 0; j < 8; j++ ) {
			out->vecs[ 0 ][ j ] = LittleFloat( in->vecs[ 0 ][ j ] );
		}

		int miptex = LittleLong( in->miptex );
		_out->flags = LittleLong( in->flags );

		if ( !textures ) {
			_out->texture = r_notexture_mip;	// checkerboard texture
			_out->flags = 0;
		} else {
			if ( miptex >= numtextures ) {
				common->FatalError( "miptex >= numtextures" );
			}
			_out->texture = textures[ miptex ];
			if ( !_out->texture ) {
				_out->texture = r_notexture_mip;	// texture not found
				_out->flags = 0;
			}
		}
	}
}

static void BuildSurfaceDisplayList( idSurfaceFaceQ1* fa ) {
	int lnumverts = fa->numVertexes;

	fa->numIndexes = ( lnumverts - 2 ) * 3;
	fa->indexes = new int[ fa->numIndexes ];
	for ( int i = 0; i < lnumverts; i++ ) {
		fa->vertexes[ i ].normal = fa->plane.Normal();

		vec3_t vec;
		fa->vertexes[ i ].xyz.ToOldVec3( vec );
		float s = DotProduct( vec, fa->textureInfo->vecs[ 0 ] ) + fa->textureInfo->vecs[ 0 ][ 3 ];
		s /= fa->surf.texinfo->texture->width;

		float t = DotProduct( vec, fa->textureInfo->vecs[ 1 ] ) + fa->textureInfo->vecs[ 1 ][ 3 ];
		t /= fa->surf.texinfo->texture->height;

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

void idBsp29LoadHelper::LoadFaces( bsp_lump_t* l ) {
	bsp29_dface_t* in = ( bsp29_dface_t* )( fileBase + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->FatalError( "MOD_LoadBmodel: funny lump size in %s", name.CStr() );
	}
	int count = l->filelen / sizeof ( *in );
	idSurfaceFaceQ1* out = new idSurfaceFaceQ1[ count ];

	surfaces = out;
	numsurfaces = count;

	for ( int surfnum = 0; surfnum < count; surfnum++, in++, out++ ) {
		BuildSurfaceVertexesList( out, LittleLong( in->firstedge ), LittleShort( in->numedges ) );

		out->surf.flags = 0;

		int planenum = LittleShort( in->planenum );
		int side = LittleShort( in->side );
		out->plane.FromOldCPlane( planes[ planenum ] );
		if ( side ) {
			out->plane = -out->plane;
		}

		int ti = LittleShort( in->texinfo );
		if ( ti < 0 || ti >= numtexinfo ) {
			common->Error( "MOD_LoadBmodel: bad texinfo number" );
		}
		out->surf.texinfo = texinfo + ti;
		out->textureInfo = textureInfos + ti;

		out->CalcSurfaceExtents();

		// lighting info

		for ( int i = 0; i < BSP29_MAXLIGHTMAPS; i++ ) {
			out->styles[ i ] = in->styles[ i ];
		}
		int lightofs = LittleLong( in->lightofs );
		if ( lightofs == -1 ) {
			out->samples = NULL;
		} else {
			out->samples = lightdata + lightofs;
		}

		// set the drawing flags flag

		if ( !String::NCmp( out->surf.texinfo->texture->name, "sky", 3 ) ) {	// sky
			out->surf.flags |= ( BRUSH29_SURF_DRAWSKY | BRUSH29_SURF_DRAWTILED );
			out->shader = out->surf.texinfo->texture->shader;
			out->surf.altShader = out->surf.texinfo->texture->shader;
			Subdivide( out );		// cut up polygon for warps
			continue;
		}

		if ( out->surf.texinfo->texture->name[ 0 ] == '*' ) {	// turbulent
			out->surf.flags |= ( BRUSH29_SURF_DRAWTURB | BRUSH29_SURF_DRAWTILED );
			out->shader = out->surf.texinfo->texture->shader;
			out->surf.altShader = out->surf.texinfo->texture->shader;
			for ( int i = 0; i < 2; i++ ) {
				out->extents[ i ] = 16384;
				out->textureMins[ i ] = -8192;
			}
			Subdivide( out );		// cut up polygon for warps
			continue;
		}

		GL_CreateSurfaceLightmapQ1( out, lightdata );
		BuildSurfaceDisplayList( out );

		out->shader = R_BuildBsp29Shader( out->surf.texinfo->texture, out->lightMapTextureNum );
		if ( out->surf.texinfo->texture->alternate_anims ) {
			out->surf.altShader = R_BuildBsp29Shader( out->surf.texinfo->texture->alternate_anims, out->lightMapTextureNum );
		} else {
			out->surf.altShader = out->shader;
		}
	}
}

void idBsp29LoadHelper::BuildSurfaceVertexesList( idSurfaceFaceQ1Q2* fa, int firstedge, int lnumverts ) {
	// reconstruct the polygon
	fa->numVertexes = lnumverts;
	fa->vertexes = new idWorldVertex[ lnumverts ];
	fa->bounds.Clear();
	for ( int i = 0; i < lnumverts; i++ ) {
		int lindex = surfedges[ firstedge + i ];

		float* vec;
		if ( lindex > 0 ) {
			vec = vertexes[ edges[ lindex ].v[ 0 ] ].position;
		} else {
			vec = vertexes[ edges[ -lindex ].v[ 1 ] ].position;
		}
		fa->vertexes[ i ].xyz.FromOldVec3( vec );
		fa->bounds.AddPoint( fa->vertexes[ i ].xyz );
	}
	fa->boundingSphere = fa->bounds.ToSphere();
}

void idBsp29LoadHelper::LoadSubmodelsQ1( bsp_lump_t* l ) {
	bsp29_dmodel_q1_t* in = ( bsp29_dmodel_q1_t* )( fileBase + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->FatalError( "MOD_LoadBmodel: funny lump size in %s", name.CStr() );
	}
	int count = l->filelen / sizeof ( *in );
	mbrush29_submodel_t* out = new mbrush29_submodel_t[ count ];

	submodels = out;
	numsubmodels = count;

	for ( int i = 0; i < count; i++, in++, out++ ) {
		for ( int j = 0; j < 3; j++ ) {
			// spread the mins / maxs by a pixel
			out->mins[ j ] = LittleFloat( in->mins[ j ] ) - 1;
			out->maxs[ j ] = LittleFloat( in->maxs[ j ] ) + 1;
		}
		out->headnode = LittleLong( in->headnode[ 0 ] );
		out->visleafs = LittleLong( in->visleafs );
		out->firstface = LittleLong( in->firstface );
		out->numfaces = LittleLong( in->numfaces );
	}
}

void idBsp29LoadHelper::LoadSubmodelsH2( bsp_lump_t* l ) {
	bsp29_dmodel_h2_t* in = ( bsp29_dmodel_h2_t* )( fileBase + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->FatalError( "MOD_LoadBmodel: funny lump size in %s", name.CStr() );
	}
	int count = l->filelen / sizeof ( *in );
	mbrush29_submodel_t* out = new mbrush29_submodel_t[ count ];

	submodels = out;
	numsubmodels = count;

	for ( int i = 0; i < count; i++, in++, out++ ) {
		for ( int j = 0; j < 3; j++ ) {
			// spread the mins / maxs by a pixel
			out->mins[ j ] = LittleFloat( in->mins[ j ] ) - 1;
			out->maxs[ j ] = LittleFloat( in->maxs[ j ] ) + 1;
		}
		out->headnode = LittleLong( in->headnode[ 0 ] );
		out->visleafs = LittleLong( in->visleafs );
		out->firstface = LittleLong( in->firstface );
		out->numfaces = LittleLong( in->numfaces );
	}
}
