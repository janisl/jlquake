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

#include "surfaces.h"
#include "main.h"
#include "backend.h"
#include "cvars.h"
#include "state.h"
#include "../../common/Common.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"
#include "../../common/content_types.h"

mbrush29_leaf_t* r_viewleaf;
mbrush29_leaf_t* r_oldviewleaf;

idSurfaceFaceQ1* waterchain = NULL;

int skytexturenum;

static int allocated[ MAX_LIGHTMAPS ][ BLOCK_WIDTH ];
static bool lightmap_modified[ MAX_LIGHTMAPS ];
static glRect_t lightmap_rectchange[ MAX_LIGHTMAPS ];

// the lightmap texture data needs to be kept in
// main memory so texsubimage can update properly
static byte lightmaps[ 4 * MAX_LIGHTMAPS * BLOCK_WIDTH * BLOCK_HEIGHT ];

static unsigned blocklights_q1[ 18 * 18 ];

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

static void R_AddDynamicLightsQ1( idSurfaceFaceQ1* surf ) {
	int smax = ( surf->surf.extents[ 0 ] >> 4 ) + 1;
	int tmax = ( surf->surf.extents[ 1 ] >> 4 ) + 1;
	mbrush29_texinfo_t* tex = surf->surf.texinfo;

	for ( int lnum = 0; lnum < tr.refdef.num_dlights; lnum++ ) {
		if ( !( surf->dlightBits[ tr.smpFrame ] & ( 1 << lnum ) ) ) {
			continue;		// not lit by this light
		}

		float rad = tr.refdef.dlights[ lnum ].radius;
		vec3_t old;
		surf->plane.Normal().ToOldVec3( old );
		float dist = DotProduct( tr.refdef.dlights[ lnum ].origin, old ) - surf->plane.Dist();
		rad -= idMath::Fabs( dist );
		float minlight = 0;	//tr.refdef.dlights[lnum].minlight;
		if ( rad < minlight ) {
			continue;
		}
		minlight = rad - minlight;

		vec3_t impact;
		for ( int i = 0; i < 3; i++ ) {
			impact[ i ] = tr.refdef.dlights[ lnum ].origin[ i ] - old[ i ] * dist;
		}

		vec3_t local;
		local[ 0 ] = DotProduct( impact, tex->vecs[ 0 ] ) + tex->vecs[ 0 ][ 3 ];
		local[ 1 ] = DotProduct( impact, tex->vecs[ 1 ] ) + tex->vecs[ 1 ][ 3 ];

		local[ 0 ] -= surf->surf.texturemins[ 0 ];
		local[ 1 ] -= surf->surf.texturemins[ 1 ];

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
static void R_BuildLightMapQ1( idRenderModel* model, idSurfaceFaceQ1* surf, byte* dest, byte* overbrightDest, int stride ) {
	int smax, tmax;
	int t;
	int i, j, size;
	byte* lightmap;
	unsigned scale;
	int maps;
	unsigned* bl;

	surf->surf.cached_dlight = surf->dlightBits[ backEnd.smpFrame ];

	smax = ( surf->surf.extents[ 0 ] >> 4 ) + 1;
	tmax = ( surf->surf.extents[ 1 ] >> 4 ) + 1;
	size = smax * tmax;
	lightmap = surf->surf.samples;

// set to full bright if no light data
	if ( !model->brush29_lightdata ) {
		for ( i = 0; i < size; i++ )
			blocklights_q1[ i ] = 255 * 256;
		goto store;
	}

// clear to no light
	for ( i = 0; i < size; i++ )
		blocklights_q1[ i ] = 0;

// add all the lightmaps
	if ( lightmap ) {
		for ( maps = 0; maps < BSP29_MAXLIGHTMAPS && surf->surf.styles[ maps ] != 255;
			  maps++ ) {
			scale = tr.refdef.lightstyles[ surf->surf.styles[ maps ] ].rgb[ 0 ] * 256;
			surf->surf.cached_light[ maps ] = scale;		// 8.8 fraction
			for ( i = 0; i < size; i++ )
				blocklights_q1[ i ] += lightmap[ i ] * scale;
			lightmap += size;	// skip to next lightmap
		}
	}

// add all the dynamic lights
	if ( surf->dlightBits[ backEnd.smpFrame ] ) {
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

void GL_CreateSurfaceLightmapQ1( idSurfaceFaceQ1* surf ) {
	int smax = ( surf->surf.extents[ 0 ] >> 4 ) + 1;
	int tmax = ( surf->surf.extents[ 1 ] >> 4 ) + 1;

	surf->surf.lightmaptexturenum = AllocBlock( smax, tmax, &surf->surf.light_s, &surf->surf.light_t );
	byte* base = lightmaps + surf->surf.lightmaptexturenum * 4 * BLOCK_WIDTH * BLOCK_HEIGHT;
	base += ( surf->surf.light_t * BLOCK_WIDTH + surf->surf.light_s ) * 4;
	byte* overbrightBase = lightmaps + ( surf->surf.lightmaptexturenum + MAX_LIGHTMAPS / 2 ) * 4 * BLOCK_WIDTH * BLOCK_HEIGHT;
	overbrightBase += ( surf->surf.light_t * BLOCK_WIDTH + surf->surf.light_s ) * 4;
	R_BuildLightMapQ1( loadmodel, surf, base, overbrightBase, BLOCK_WIDTH * 4 );
}

void R_BeginBuildingLightmapsQ1() {
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
}

//	Builds the lightmap texture with all the surfaces from all brush models
void GL_BuildLightmaps() {
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
static void R_RenderDynamicLightmaps( idSurfaceFaceQ1* surf ) {
	mbrush29_surface_t* fa = &surf->surf;

	// check for lightmap modification
	for ( int maps = 0; maps < BSP29_MAXLIGHTMAPS && fa->styles[ maps ] != 255; maps++ ) {
		if ( tr.refdef.lightstyles[ fa->styles[ maps ] ].rgb[ 0 ] * 256 != fa->cached_light[ maps ] ) {
			goto dynamic;
		}
	}

	if ( surf->dlightBits[ backEnd.smpFrame ] ||// dynamic this frame
		 fa->cached_dlight ) {					// dynamic previously
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
			R_BuildLightMapQ1( tr.worldModel, surf, base, overbrightBase, BLOCK_WIDTH * 4 );
		}
	}
}

void R_UploadModifiedLightmapsQ1() {
	for (int i = 0; i < MAX_LIGHTMAPS; i++) {
		if ( !lightmap_modified[ i ] ) {
			continue;
		}

		GL_Bind( tr.lightmaps[ i ] );
		lightmap_modified[ i ] = false;
		glRect_t* theRect = &lightmap_rectchange[ i < MAX_LIGHTMAPS / 2 ? i : i - MAX_LIGHTMAPS / 2 ];
		qglTexSubImage2D( GL_TEXTURE_2D, 0, 0, theRect->t,
			BLOCK_WIDTH, theRect->h, GL_RGBA, GL_UNSIGNED_BYTE,
			lightmaps + ( i * BLOCK_HEIGHT + theRect->t ) * BLOCK_WIDTH * 4 );
		theRect->l = BLOCK_WIDTH;
		theRect->t = BLOCK_HEIGHT;
		theRect->h = 0;
		theRect->w = 0;
	}
}

void R_AddWorldSurfaceBsp29( idSurfaceFaceQ1* surf, int forcedSortIndex ) {
	shader_t* shader;
	if ( tr.currentEntity->e.frame ) {
		shader = surf->surf.altShader;
	} else {
		shader = surf->shader;
	}

	int frontFace;
	if ( surf->Cull( shader, &frontFace ) ) {
		return;
	}

	if ( !( surf->surf.flags & ( BRUSH29_SURF_DRAWTURB | BRUSH29_SURF_DRAWSKY ) ) &&
		!( tr.currentEntity->e.renderfx & ( RF_TRANSLUCENT | RF_ABSOLUTE_LIGHT ) ) ) {
		R_RenderDynamicLightmaps( surf );
	}

	R_AddDrawSurf( surf, shader, 0, false, false, ATI_TESS_NONE, forcedSortIndex );
}

void R_DrawWaterSurfaces(int& forcedSortIndex) {
	for ( idSurfaceFaceQ1* s = waterchain; s; s = s->texturechain ) {
		R_AddWorldSurfaceBsp29( s, forcedSortIndex++ );
	}
	waterchain = NULL;
}
