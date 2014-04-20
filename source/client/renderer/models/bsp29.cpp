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
#include "RenderModelBSP29.h"
#include "../surfaces.h"

mbrush29_texture_t* r_notexture_mip;

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
	delete[] mod->brush29_surfaces;
	delete[] mod->brush29_nodes;
	delete[] mod->brush29_leafs;
	delete[] mod->brush29_marksurfaces;
	delete[] mod->brush29_surfedges;
	delete[] mod->brush29_planes;
	delete[] mod->brush29_submodels;
}

void Mod_FreeBsp29NonMap( idRenderModel* mod ) {
	if ( mod->name[ 0 ] == '*' ) {
		return;
	}

	if ( mod->brush29nm_textures ) {
		for ( int i = 0; i < mod->brush29nm_numtextures; i++ ) {
			if ( mod->brush29nm_textures[ i ] ) {
				Mem_Free( mod->brush29nm_textures[ i ] );
			}
		}
		delete[] mod->brush29nm_textures;
	}
	if ( mod->brush29nm_lightdata ) {
		delete[] mod->brush29nm_lightdata;
	}
	delete[] mod->brush29nm_vertexes;
	delete[] mod->brush29nm_edges;
	delete[] mod->brush29nm_texinfo;
	delete[] mod->brush29nm_surfaces;
	delete[] mod->brush29nm_surfedges;
	delete[] mod->brush29nm_planes;
	delete[] mod->brush29nm_submodels;
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
