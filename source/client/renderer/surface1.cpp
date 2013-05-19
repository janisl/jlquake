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

#include "local.h"
#include "../../common/Common.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"
#include "../../common/content_types.h"

mbrush29_leaf_t* r_viewleaf;
mbrush29_leaf_t* r_oldviewleaf;

mbrush29_surface_t* waterchain = NULL;

int skytexturenum;

static int allocated[ MAX_LIGHTMAPS ][ BLOCK_WIDTH ];
static bool lightmap_modified[ MAX_LIGHTMAPS ];
static glRect_t lightmap_rectchange[ MAX_LIGHTMAPS ];

// the lightmap texture data needs to be kept in
// main memory so texsubimage can update properly
static byte lightmaps[ 4 * MAX_LIGHTMAPS * BLOCK_WIDTH * BLOCK_HEIGHT ];

static unsigned blocklights_q1[ 18 * 18 ];

static mbrush29_vertex_t* r_pcurrentvertbase;

/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/

// returns a texture number and the position inside it
static int AllocBlock( int w, int h, int* x, int* y ) {
	for ( int texnum = 0; texnum < MAX_LIGHTMAPS / 2; texnum++ ) {
		int best = BLOCK_HEIGHT;

		for ( int i = 0; i < BLOCK_WIDTH - w; i++ ) {
			int best2 = 0;

			int j;
			for ( j = 0; j < w; j++ ) {
				if ( allocated[ texnum ][ i + j ] >= best ) {
					break;
				}
				if ( allocated[ texnum ][ i + j ] > best2 ) {
					best2 = allocated[ texnum ][ i + j ];
				}
			}
			if ( j == w ) {
				// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if ( best + h > BLOCK_HEIGHT ) {
			continue;
		}

		for ( int i = 0; i < w; i++ ) {
			allocated[ texnum ][ *x + i ] = best + h;
		}

		return texnum;
	}

	common->FatalError( "AllocBlock: full" );
	return 0;
}

static void R_AddDynamicLightsQ1( mbrush29_surface_t* surf ) {
	int smax = ( surf->extents[ 0 ] >> 4 ) + 1;
	int tmax = ( surf->extents[ 1 ] >> 4 ) + 1;
	mbrush29_texinfo_t* tex = surf->texinfo;

	for ( int lnum = 0; lnum < tr.refdef.num_dlights; lnum++ ) {
		if ( !( surf->dlightbits & ( 1 << lnum ) ) ) {
			continue;		// not lit by this light
		}

		float rad = tr.refdef.dlights[ lnum ].radius;
		float dist = DotProduct( tr.refdef.dlights[ lnum ].origin, surf->plane->normal ) - surf->plane->dist;
		rad -= idMath::Fabs( dist );
		float minlight = 0;	//tr.refdef.dlights[lnum].minlight;
		if ( rad < minlight ) {
			continue;
		}
		minlight = rad - minlight;

		vec3_t impact;
		for ( int i = 0; i < 3; i++ ) {
			impact[ i ] = tr.refdef.dlights[ lnum ].origin[ i ] - surf->plane->normal[ i ] * dist;
		}

		vec3_t local;
		local[ 0 ] = DotProduct( impact, tex->vecs[ 0 ] ) + tex->vecs[ 0 ][ 3 ];
		local[ 1 ] = DotProduct( impact, tex->vecs[ 1 ] ) + tex->vecs[ 1 ][ 3 ];

		local[ 0 ] -= surf->texturemins[ 0 ];
		local[ 1 ] -= surf->texturemins[ 1 ];

		for ( int t = 0; t < tmax; t++ ) {
			int td = local[ 1 ] - t * 16;
			if ( td < 0 ) {
				td = -td;
			}
			for ( int s = 0; s < smax; s++ ) {
				int sd = local[ 0 ] - s * 16;
				if ( sd < 0 ) {
					sd = -sd;
				}
				if ( sd > td ) {
					dist = sd + ( td >> 1 );
				} else {
					dist = td + ( sd >> 1 );
				}
				if ( dist < minlight ) {
					blocklights_q1[ t * smax + s ] += ( rad - dist ) * 256;
				}
			}
		}
	}
}

//	Combine and scale multiple lightmaps into the 8.8 format in blocklights_q1
static void R_BuildLightMapQ1( mbrush29_surface_t* surf, byte* dest, byte* overbrightDest, int stride ) {
	int smax, tmax;
	int t;
	int i, j, size;
	byte* lightmap;
	unsigned scale;
	int maps;
	unsigned* bl;

	surf->cached_dlight = ( surf->dlightframe == tr.frameCount );

	smax = ( surf->extents[ 0 ] >> 4 ) + 1;
	tmax = ( surf->extents[ 1 ] >> 4 ) + 1;
	size = smax * tmax;
	lightmap = surf->samples;

// set to full bright if no light data
	if ( !tr.worldModel->brush29_lightdata ) {
		for ( i = 0; i < size; i++ )
			blocklights_q1[ i ] = 255 * 256;
		goto store;
	}

// clear to no light
	for ( i = 0; i < size; i++ )
		blocklights_q1[ i ] = 0;

// add all the lightmaps
	if ( lightmap ) {
		for ( maps = 0; maps < BSP29_MAXLIGHTMAPS && surf->styles[ maps ] != 255;
			  maps++ ) {
			scale = tr.refdef.lightstyles[ surf->styles[ maps ] ].rgb[ 0 ] * 256;
			surf->cached_light[ maps ] = scale;		// 8.8 fraction
			for ( i = 0; i < size; i++ )
				blocklights_q1[ i ] += lightmap[ i ] * scale;
			lightmap += size;	// skip to next lightmap
		}
	}

// add all the dynamic lights
	if ( surf->dlightframe == tr.frameCount ) {
		R_AddDynamicLightsQ1( surf );
	}

// bound, invert, and shift
store:
	stride -= ( smax << 2 );
	bl = blocklights_q1;
	for ( i = 0; i < tmax; i++, dest += stride, overbrightDest += stride ) {
		for ( j = 0; j < smax; j++ ) {
			t = *bl++;
			t >>= 7;
			int t2 = t - 256;
			if ( t > 255 ) {
				t = 255;
			}
			dest[ 0 ] = t;
			dest[ 1 ] = t;
			dest[ 2 ] = t;
			dest += 4;
			if ( t2 < 0 ) {
				t2 = 0;
			} else if ( t2 > 255 ) {
				t2 = 255;
			}
			overbrightDest[ 0 ] = t2;
			overbrightDest[ 1 ] = t2;
			overbrightDest[ 2 ] = t2;
			overbrightDest += 4;
		}
	}
}

static void GL_CreateSurfaceLightmapQ1( mbrush29_surface_t* surf ) {
	if ( surf->flags & ( BRUSH29_SURF_DRAWSKY | BRUSH29_SURF_DRAWTURB ) ) {
		return;
	}

	int smax = ( surf->extents[ 0 ] >> 4 ) + 1;
	int tmax = ( surf->extents[ 1 ] >> 4 ) + 1;

	surf->lightmaptexturenum = AllocBlock( smax, tmax, &surf->light_s, &surf->light_t );
	byte* base = lightmaps + surf->lightmaptexturenum * 4 * BLOCK_WIDTH * BLOCK_HEIGHT;
	base += ( surf->light_t * BLOCK_WIDTH + surf->light_s ) * 4;
	byte* overbrightBase = lightmaps + ( surf->lightmaptexturenum + MAX_LIGHTMAPS / 2 ) * 4 * BLOCK_WIDTH * BLOCK_HEIGHT;
	overbrightBase += ( surf->light_t * BLOCK_WIDTH + surf->light_s ) * 4;
	R_BuildLightMapQ1( surf, base, overbrightBase, BLOCK_WIDTH * 4 );
}

static void BuildSurfaceDisplayList( mbrush29_surface_t* fa ) {
	// reconstruct the polygon
	mbrush29_edge_t* pedges = tr.currentModel->brush29_edges;
	int lnumverts = fa->numedges;

	//
	// draw texture
	//
	mbrush29_glpoly_t* poly = ( mbrush29_glpoly_t* )Mem_Alloc( sizeof ( mbrush29_glpoly_t ) + ( lnumverts - 4 ) * BRUSH29_VERTEXSIZE * sizeof ( float ) );
	poly->next = fa->polys;
	fa->polys = poly;
	poly->numverts = lnumverts;

	for ( int i = 0; i < lnumverts; i++ ) {
		int lindex = tr.currentModel->brush29_surfedges[ fa->firstedge + i ];

		float* vec;
		if ( lindex > 0 ) {
			mbrush29_edge_t* r_pedge = &pedges[ lindex ];
			vec = r_pcurrentvertbase[ r_pedge->v[ 0 ] ].position;
		} else {
			mbrush29_edge_t* r_pedge = &pedges[ -lindex ];
			vec = r_pcurrentvertbase[ r_pedge->v[ 1 ] ].position;
		}
		float s = DotProduct( vec, fa->texinfo->vecs[ 0 ] ) + fa->texinfo->vecs[ 0 ][ 3 ];
		s /= fa->texinfo->texture->width;

		float t = DotProduct( vec, fa->texinfo->vecs[ 1 ] ) + fa->texinfo->vecs[ 1 ][ 3 ];
		t /= fa->texinfo->texture->height;

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

	//
	// remove co-linear points - Ed
	//
	if ( !r_keeptjunctions->value ) {
		for ( int i = 0; i < lnumverts; ++i ) {
			vec3_t v1, v2;
			float* prev, * thisv, * next;

			prev = poly->verts[ ( i + lnumverts - 1 ) % lnumverts ];
			thisv = poly->verts[ i ];
			next = poly->verts[ ( i + 1 ) % lnumverts ];

			VectorSubtract( thisv, prev, v1 );
			VectorNormalize( v1 );
			VectorSubtract( next, prev, v2 );
			VectorNormalize( v2 );

			// skip co-linear points
			#define COLINEAR_EPSILON 0.001
			if ( ( idMath::Fabs( v1[ 0 ] - v2[ 0 ] ) <= COLINEAR_EPSILON ) &&
				 ( idMath::Fabs( v1[ 1 ] - v2[ 1 ] ) <= COLINEAR_EPSILON ) &&
				 ( idMath::Fabs( v1[ 2 ] - v2[ 2 ] ) <= COLINEAR_EPSILON ) ) {
				for ( int j = i + 1; j < lnumverts; ++j ) {
					for ( int k = 0; k < BRUSH29_VERTEXSIZE; ++k ) {
						poly->verts[ j - 1 ][ k ] = poly->verts[ j ][ k ];
					}
				}
				--lnumverts;
				// retry next vertex next time, which is now current vertex
				--i;
			}
		}
	}
	poly->numverts = lnumverts;
}

//	Builds the lightmap texture with all the surfaces from all brush models
void GL_BuildLightmaps() {
	Com_Memset( allocated, 0, sizeof ( allocated ) );

	tr.frameCount = 1;		// no dlightcache

	for ( int i = 0; i < MAX_LIGHTSTYLES; i++ ) {
		backEndData[ tr.smpFrame ]->lightstyles[ i ].rgb[ 0 ] = 1;
		backEndData[ tr.smpFrame ]->lightstyles[ i ].rgb[ 1 ] = 1;
		backEndData[ tr.smpFrame ]->lightstyles[ i ].rgb[ 2 ] = 1;
		backEndData[ tr.smpFrame ]->lightstyles[ i ].white = 3;
	}
	tr.refdef.lightstyles = backEndData[ tr.smpFrame ]->lightstyles;

	if ( !tr.lightmaps[ 0 ] ) {
		for ( int i = 0; i < MAX_LIGHTMAPS; i++ ) {
			tr.lightmaps[ i ] = R_CreateImage( va( "*lightmap%d", i ), lightmaps + i * BLOCK_WIDTH * BLOCK_HEIGHT * 4, BLOCK_WIDTH, BLOCK_HEIGHT, false, false, GL_CLAMP );
		}
	}

	for ( int j = 1; j < MAX_MOD_KNOWN; j++ ) {
		model_t* m = tr.models[ j ];
		if ( !m ) {
			break;
		}
		if ( m->type != MOD_BRUSH29 ) {
			continue;
		}
		if ( m->name[ 0 ] == '*' ) {
			continue;
		}
		r_pcurrentvertbase = m->brush29_vertexes;
		tr.currentModel = m;
		for ( int i = 0; i < m->brush29_numsurfaces; i++ ) {
			GL_CreateSurfaceLightmapQ1( m->brush29_surfaces + i );
			if ( m->brush29_surfaces[ i ].flags & BRUSH29_SURF_DRAWTURB ) {
				continue;
			}
			if ( m->brush29_surfaces[ i ].flags & BRUSH29_SURF_DRAWSKY ) {
				continue;
			}
			BuildSurfaceDisplayList( m->brush29_surfaces + i );
		}
	}

	//
	// upload all lightmaps that were filled
	//
	for ( int i = 0; i < MAX_LIGHTMAPS / 2; i++ ) {
		if ( !allocated[ i ][ 0 ] ) {
			break;		// no more used
		}
		lightmap_modified[ i ] = false;
		lightmap_modified[ i + MAX_LIGHTMAPS / 2 ] = false;
		lightmap_rectchange[ i ].l = BLOCK_WIDTH;
		lightmap_rectchange[ i ].t = BLOCK_HEIGHT;
		lightmap_rectchange[ i ].w = 0;
		lightmap_rectchange[ i ].h = 0;
		R_ReUploadImage( tr.lightmaps[ i ], lightmaps + i * BLOCK_WIDTH * BLOCK_HEIGHT * 4 );
		R_ReUploadImage( tr.lightmaps[ i + MAX_LIGHTMAPS / 2 ], lightmaps + ( i + MAX_LIGHTMAPS / 2 ) * BLOCK_WIDTH * BLOCK_HEIGHT * 4 );
	}
}

//	Returns the proper texture for a given time and base texture
void R_TextureAnimationQ1( mbrush29_texture_t* base, textureBundle_t* bundle ) {
	if ( backEnd.currentEntity->e.frame ) {
		if ( base->alternate_anims ) {
			base = base->alternate_anims;
		}
	}

	if ( !base->anim_total ) {
		bundle->image[ 0 ] = base->gl_texture;
		bundle->numImageAnimations = 1;
		return;
	}

	if ( !base->anim_base ) {
		common->FatalError( "No anim base\n" );
	}
	base = base->anim_base;
	for ( int i = 0; i < base->anim_total; i++ ) {
		bundle->image[ i ] = base->gl_texture;
		base = base->anim_next;
	}
	bundle->numImageAnimations = base->anim_total;
	bundle->imageAnimationSpeed = 5;
}

//	Returns the proper texture for a given time and base texture
bool R_TextureFullbrightAnimationQ1( mbrush29_texture_t* base, textureBundle_t* bundle ) {
	if ( backEnd.currentEntity->e.frame ) {
		if ( base->alternate_anims ) {
			base = base->alternate_anims;
		}
	}

	if ( !base->anim_total ) {
		bundle->image[ 0 ] = base->fullBrightTexture;
		bundle->numImageAnimations = 1;
		return !!base->fullBrightTexture;
	}

	if ( !base->anim_base ) {
		common->FatalError( "No anim base\n" );
	}
	base = base->anim_base;
	bool haveFullBrights = false;
	for ( int i = 0; i < base->anim_total; i++ ) {
		if ( base->fullBrightTexture ) {
			bundle->image[ i ] = base->fullBrightTexture;
			haveFullBrights = true;
		} else {
			bundle->image[ i ] = tr.transparentImage;
		}
		base = base->anim_next;
	}
	bundle->numImageAnimations = base->anim_total;
	bundle->imageAnimationSpeed = 5;

	return haveFullBrights;
}

//	Multitexture
static void R_RenderDynamicLightmaps( mbrush29_surface_t* fa ) {
	c_brush_polys++;

	// check for lightmap modification
	for ( int maps = 0; maps < BSP29_MAXLIGHTMAPS && fa->styles[ maps ] != 255; maps++ ) {
		if ( tr.refdef.lightstyles[ fa->styles[ maps ] ].rgb[ 0 ] * 256 != fa->cached_light[ maps ] ) {
			goto dynamic;
		}
	}

	if ( fa->dlightframe == tr.frameCount ||// dynamic this frame
		 fa->cached_dlight ) {				// dynamic previously
dynamic:
		if ( r_dynamic->value ) {
			lightmap_modified[ fa->lightmaptexturenum ] = true;
			lightmap_modified[ fa->lightmaptexturenum + MAX_LIGHTMAPS / 2 ] = true;
			glRect_t* theRect = &lightmap_rectchange[ fa->lightmaptexturenum ];
			if ( fa->light_t < theRect->t ) {
				if ( theRect->h ) {
					theRect->h += theRect->t - fa->light_t;
				}
				theRect->t = fa->light_t;
			}
			if ( fa->light_s < theRect->l ) {
				if ( theRect->w ) {
					theRect->w += theRect->l - fa->light_s;
				}
				theRect->l = fa->light_s;
			}
			int smax = ( fa->extents[ 0 ] >> 4 ) + 1;
			int tmax = ( fa->extents[ 1 ] >> 4 ) + 1;
			if ( ( theRect->w + theRect->l ) < ( fa->light_s + smax ) ) {
				theRect->w = ( fa->light_s - theRect->l ) + smax;
			}
			if ( ( theRect->h + theRect->t ) < ( fa->light_t + tmax ) ) {
				theRect->h = ( fa->light_t - theRect->t ) + tmax;
			}
			byte* base = lightmaps + fa->lightmaptexturenum * 4 * BLOCK_WIDTH * BLOCK_HEIGHT;
			base += fa->light_t * BLOCK_WIDTH * 4 + fa->light_s * 4;
			byte* overbrightBase = lightmaps + ( fa->lightmaptexturenum + MAX_LIGHTMAPS / 2 ) * 4 * BLOCK_WIDTH * BLOCK_HEIGHT;
			overbrightBase += fa->light_t * BLOCK_WIDTH * 4 + fa->light_s * 4;
			R_BuildLightMapQ1( fa, base, overbrightBase, BLOCK_WIDTH * 4 );
		}
	}

	int i = fa->lightmaptexturenum;
	if ( lightmap_modified[ i ] ) {
		GL_Bind( tr.lightmaps[ i ] );
		lightmap_modified[ i ] = false;
		glRect_t* theRect = &lightmap_rectchange[ i ];
		qglTexSubImage2D( GL_TEXTURE_2D, 0, 0, theRect->t,
			BLOCK_WIDTH, theRect->h, GL_RGBA, GL_UNSIGNED_BYTE,
			lightmaps + ( i * BLOCK_HEIGHT + theRect->t ) * BLOCK_WIDTH * 4 );
		theRect->l = BLOCK_WIDTH;
		theRect->t = BLOCK_HEIGHT;
		theRect->h = 0;
		theRect->w = 0;
	}
	if ( lightmap_modified[ i + MAX_LIGHTMAPS / 2 ] ) {
		GL_Bind( tr.lightmaps[ i + MAX_LIGHTMAPS / 2 ] );
		lightmap_modified[ i + MAX_LIGHTMAPS / 2 ] = false;
		glRect_t* theRect = &lightmap_rectchange[ i ];
		qglTexSubImage2D( GL_TEXTURE_2D, 0, 0, theRect->t,
			BLOCK_WIDTH, theRect->h, GL_RGBA, GL_UNSIGNED_BYTE,
			lightmaps + ( ( i + MAX_LIGHTMAPS / 2 ) * BLOCK_HEIGHT + theRect->t ) * BLOCK_WIDTH * 4 );
		theRect->l = BLOCK_WIDTH;
		theRect->t = BLOCK_HEIGHT;
		theRect->h = 0;
		theRect->w = 0;
	}
}

void R_AddWorldSurfaceBsp29( mbrush29_surface_t* surf, int forcedSortIndex ) {
	if ( surf->texinfo->texture->shader ) {
		R_AddDrawSurf( ( surfaceType_t* )surf, surf->texinfo->texture->shader, 0, false, false, ATI_TESS_NONE, forcedSortIndex );
	} else {
		R_AddDrawSurf( ( surfaceType_t* )surf, tr.defaultShader, 0, false, false, ATI_TESS_NONE, forcedSortIndex );
	}
}

//	Systems that have fast state and texture changes can just do everything
// as it passes with no need to sort
void R_DrawSequentialPoly( mbrush29_surface_t* s ) {
	shader_t shader = {};
	shaderStage_t stage1 = {};
	shaderStage_t stage2 = {};
	shaderStage_t stage3 = {};
	shader.cullType = CT_FRONT_SIDED;
	shader.optimalStageIteratorFunc = RB_StageIteratorGeneric;

	if ( s->flags & BRUSH29_SURF_DRAWTURB ) {
		//
		// subdivided water surface warp
		//
	} else if ( s->flags & BRUSH29_SURF_DRAWSKY ) {
		//
		// subdivided sky warp
		//
	} else {
		R_TextureAnimationQ1( s->texinfo->texture, &stage1.bundle[ 0 ] );
		stage1.bundle[ 0 ].tcGen = TCGEN_TEXTURE;
		if ( GGameType & GAME_Hexen2 ) {
			stage1.rgbGen = CGEN_ENTITY_ABSOLUTE_LIGHT;
			stage1.alphaGen = AGEN_ENTITY_CONDITIONAL_TRANSLUCENT;
		} else {
			stage1.rgbGen = CGEN_IDENTITY;
			stage1.alphaGen = AGEN_IDENTITY;
		}
		stage1.stateBits = GLS_DEFAULT;
		stage1.translucentStateBits = GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
		shader.stages[ 0 ] = &stage1;
		if ( qglActiveTextureARB ) {
			shader.multitextureEnv = GL_MODULATE;

			stage1.bundle[ 1 ].image[ 0 ] = tr.lightmaps[ s->lightmaptexturenum ];
			stage1.bundle[ 1 ].numImageAnimations = 1;
			stage1.bundle[ 1 ].tcGen = TCGEN_LIGHTMAP;
			stage1.bundle[ 1 ].isLightmap = true;

			if ( r_drawOverBrights->integer ) {
				R_TextureAnimationQ1( s->texinfo->texture, &stage2.bundle[ 0 ] );
				stage2.isOverbright = true;
				stage2.rgbGen = CGEN_IDENTITY;
				stage2.alphaGen = AGEN_IDENTITY;
				stage2.stateBits = GLS_DEFAULT | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
				stage2.bundle[ 0 ].tcGen = TCGEN_TEXTURE;
				stage2.bundle[ 1 ].image[ 0 ] = tr.lightmaps[ s->lightmaptexturenum + MAX_LIGHTMAPS / 2 ];
				stage2.bundle[ 1 ].numImageAnimations = 1;
				stage2.bundle[ 1 ].tcGen = TCGEN_LIGHTMAP;
				shader.stages[ 1 ] = &stage2;
			}
		} else {
			stage2.rgbGen = CGEN_IDENTITY;
			stage2.alphaGen = AGEN_IDENTITY;
			stage2.stateBits = GLS_DEFAULT | GLS_SRCBLEND_ZERO | GLS_DSTBLEND_SRC_COLOR;
			stage2.bundle[ 0 ].image[ 0 ] = tr.lightmaps[ s->lightmaptexturenum ];
			stage2.bundle[ 0 ].numImageAnimations = 1;
			stage2.bundle[ 0 ].isLightmap = true;
			stage2.bundle[ 0 ].tcGen = TCGEN_LIGHTMAP;

			shader.stages[ 1 ] = &stage2;
		}

		if ( R_TextureFullbrightAnimationQ1( s->texinfo->texture, &stage3.bundle[ 0 ] ) ) {
			stage3.bundle[ 0 ].tcGen = TCGEN_TEXTURE;
			stage3.rgbGen = CGEN_IDENTITY;
			stage3.alphaGen = AGEN_IDENTITY;
			stage3.stateBits = GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			shader.stages[ 2 ] = &stage3;
		}

		RB_BeginSurface( &shader, 0 );
	}

	if ( !( s->flags & ( BRUSH29_SURF_DRAWTURB | BRUSH29_SURF_DRAWSKY ) ) &&
		!( backEnd.currentEntity->e.renderfx & ( RF_TRANSLUCENT | RF_ABSOLUTE_LIGHT ) ) ) {
		R_RenderDynamicLightmaps( s );
	}

	for ( mbrush29_glpoly_t* p = s->polys; p; p = p->next ) {
		int numVerts = tess.numVertexes;
		int numIndexes = tess.numIndexes;

		tess.numVertexes += p->numverts;
		tess.numIndexes += ( p->numverts - 2 ) * 3;

		float* v = p->verts[ 0 ];
		for ( int i = 0; i < p->numverts; i++, v += BRUSH29_VERTEXSIZE ) {
			tess.xyz[ numVerts + i ][ 0 ] = v[ 0 ];
			tess.xyz[ numVerts + i ][ 1 ] = v[ 1 ];
			tess.xyz[ numVerts + i ][ 2 ] = v[ 2 ];
			tess.texCoords[ numVerts + i ][ 0 ][ 0 ] = v[ 3 ];
			tess.texCoords[ numVerts + i ][ 0 ][ 1 ] = v[ 4 ];
			tess.texCoords[ numVerts + i ][ 1 ][ 0 ] = v[ 5 ];
			tess.texCoords[ numVerts + i ][ 1 ][ 1 ] = v[ 6 ];
			if ( s->flags & BRUSH29_SURF_PLANEBACK ) {
				tess.normal[ numVerts + i ][ 0 ] = -s->plane->normal[ 0 ];
				tess.normal[ numVerts + i ][ 1 ] = -s->plane->normal[ 1 ];
				tess.normal[ numVerts + i ][ 2 ] = -s->plane->normal[ 2 ];
			} else {
				tess.normal[ numVerts + i ][ 0 ] = s->plane->normal[ 0 ];
				tess.normal[ numVerts + i ][ 1 ] = s->plane->normal[ 1 ];
				tess.normal[ numVerts + i ][ 2 ] = s->plane->normal[ 2 ];
			}
		}
		for ( int i = 0; i < p->numverts - 2; i++ ) {
			tess.indexes[ numIndexes + i * 3 + 0 ] = numVerts;
			tess.indexes[ numIndexes + i * 3 + 1 ] = numVerts + i + 1;
			tess.indexes[ numIndexes + i * 3 + 2 ] = numVerts + i + 2;
		}
	}

	if ( !( s->flags & ( BRUSH29_SURF_DRAWSKY | BRUSH29_SURF_DRAWTURB ) ) ) {
		RB_EndSurface();
	}
}

void R_DrawWaterSurfaces(int& forcedSortIndex) {
	for ( mbrush29_surface_t* s = waterchain; s; s = s->texturechain ) {
		R_AddWorldSurfaceBsp29( s, forcedSortIndex++ );
	}
	waterchain = NULL;
}
