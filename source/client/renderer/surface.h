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

#ifndef _R_SURFACE_H
#define _R_SURFACE_H

#include "models/model.h"

struct glRect_t {
	byte l, t, w, h;
};

#define LIGHTMAP_BYTES 4

struct gllightmapstate_t {
	int current_lightmap_texture;

	int allocated[ BLOCK_WIDTH ];

	// the lightmap texture data needs to be kept in
	// main memory so texsubimage can update properly
	byte lightmap_buffer[ 4 * BLOCK_WIDTH * BLOCK_HEIGHT ];
};

void GL_BuildLightmaps();
void R_TextureAnimationQ1( mbrush29_texture_t* base, textureBundle_t* bundle );
bool R_TextureFullbrightAnimationQ1( mbrush29_texture_t* base, textureBundle_t* bundle );
void R_RenderBrushPolyQ1( mbrush29_surface_t* fa, bool override );
void R_AddWorldSurfaceBsp29( mbrush29_surface_t* surf, int forcedSortIndex );
void R_DrawSequentialPoly( mbrush29_surface_t* s );
void DrawTextureChainsQ1();
void R_DrawWaterSurfaces(int& forcedSortIndex);

void GL_BeginBuildingLightmaps( model_t* m );
void GL_CreateSurfaceLightmapQ2( mbrush38_surface_t* surf );
void GL_EndBuildingLightmaps();
void R_AddWorldSurfaceBsp38( mbrush38_surface_t* surf, int forcedSortIndex );
void GL_RenderLightmappedPoly( mbrush38_surface_t* surf );
void R_DrawAlphaSurfaces(int& forcedSortIndex);

void RB_CheckOverflow( int verts, int indexes );
#define RB_CHECKOVERFLOW( v,i ) if ( tess.numVertexes + ( v ) >= SHADER_MAX_VERTEXES || tess.numIndexes + ( i ) >= SHADER_MAX_INDEXES ) {RB_CheckOverflow( v,i ); }
void RB_AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, byte* color );
void RB_AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, byte* color, float s1, float t1, float s2, float t2 );

extern mbrush29_leaf_t* r_viewleaf;
extern mbrush29_leaf_t* r_oldviewleaf;
extern mbrush29_surface_t* waterchain;
extern int skytexturenum;		// index in cl.loadmodel, not gl texture object

extern gllightmapstate_t gl_lms;
extern mbrush38_surface_t* r_alpha_surfaces;
extern int r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

extern void( *rb_surfaceTable[ SF_NUM_SURFACE_TYPES ] ) ( void* );

#endif
