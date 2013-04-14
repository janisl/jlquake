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

#include "../../hexen2clientdefs.h"
#include "../local.h"
#include "../../../common/Common.h"
#include "../../../common/common_defs.h"
#include "../../../common/strings.h"
#include "../../../common/endian.h"

#define MAX_LBM_HEIGHT      480

#define ALIAS_BASE_SIZE_RATIO       ( 1.0 / 11.0 )
// normalizing factor so player model works out to about
//  1 pixel per triangle

#define MAXALIASFRAMES      256
#define MAXALIASVERTS       2000
#define MAXALIASTRIS        2048

struct mmesh1triangle_t {
	int facesfront;
	int vertindex[ 3 ];
	int stindex[ 3 ];
};

struct idMdlVertexRemap {
	int xyzIndex;
	int stIndex;
	bool onBackSide;
};

byte q1_player_8bit_texels[ 320 * 200 ];
byte h2_player_8bit_texels[ MAX_PLAYER_CLASS ][ 620 * 245 ];

static float aliastransform[ 3 ][ 4 ];

static vec3_t mins;
static vec3_t maxs;

static int posenum;
static mesh1hdr_t* pheader;

// a pose is a single set of vertexes.  a frame may be
// an animating sequence of poses
static const dmdl_trivertx_t* poseverts[ MAXALIASFRAMES ];
static dmdl_stvert_t stverts[ MAXALIASVERTS ];
static mmesh1triangle_t triangles[ MAXALIASTRIS ];

static model_t* aliasmodel;
static mesh1hdr_t* paliashdr;

// all frames will have their vertexes rearranged and expanded
// so they are in the order expected by the command list
static idMdlVertexRemap vertexorder[ 8192 ];
static int numorder;

static void R_AliasTransformVector( const vec3_t in, vec3_t out ) {
	out[ 0 ] = DotProduct( in, aliastransform[ 0 ] ) + aliastransform[ 0 ][ 3 ];
	out[ 1 ] = DotProduct( in, aliastransform[ 1 ] ) + aliastransform[ 1 ][ 3 ];
	out[ 2 ] = DotProduct( in, aliastransform[ 2 ] ) + aliastransform[ 2 ][ 3 ];
}

static const void* Mod_LoadAliasFrame( const void* pin, mmesh1framedesc_t* frame ) {
	const dmdl_frame_t* pdaliasframe = ( const dmdl_frame_t* )pin;

	String::Cpy( frame->name, pdaliasframe->name );
	frame->firstpose = posenum;
	frame->numposes = 1;

	for ( int i = 0; i < 3; i++ ) {
		// these are byte values, so we don't have to worry about
		// endianness
		frame->bboxmin.v[ i ] = pdaliasframe->bboxmin.v[ i ];
		frame->bboxmin.v[ i ] = pdaliasframe->bboxmax.v[ i ];
	}

	const dmdl_trivertx_t* pinframe = ( const dmdl_trivertx_t* )( pdaliasframe + 1 );

	aliastransform[ 0 ][ 0 ] = pheader->scale[ 0 ];
	aliastransform[ 1 ][ 1 ] = pheader->scale[ 1 ];
	aliastransform[ 2 ][ 2 ] = pheader->scale[ 2 ];
	aliastransform[ 0 ][ 3 ] = pheader->scale_origin[ 0 ];
	aliastransform[ 1 ][ 3 ] = pheader->scale_origin[ 1 ];
	aliastransform[ 2 ][ 3 ] = pheader->scale_origin[ 2 ];

	for ( int j = 0; j < pheader->numverts; j++ ) {
		vec3_t in;
		in[ 0 ] = pinframe[ j ].v[ 0 ];
		in[ 1 ] = pinframe[ j ].v[ 1 ];
		in[ 2 ] = pinframe[ j ].v[ 2 ];
		vec3_t out;
		R_AliasTransformVector( in, out );
		for ( int i = 0; i < 3; i++ ) {
			if ( mins[ i ] > out[ i ] ) {
				mins[ i ] = out[ i ];
			}
			if ( maxs[ i ] < out[ i ] ) {
				maxs[ i ] = out[ i ];
			}
		}
	}
	poseverts[ posenum ] = pinframe;
	posenum++;

	pinframe += pheader->numverts;

	return ( const void* )pinframe;
}

static const void* Mod_LoadAliasGroup( const void* pin, mmesh1framedesc_t* frame ) {
	const dmdl_group_t* pingroup = ( const dmdl_group_t* )pin;

	int numframes = LittleLong( pingroup->numframes );

	frame->firstpose = posenum;
	frame->numposes = numframes;

	for ( int i = 0; i < 3; i++ ) {
		// these are byte values, so we don't have to worry about endianness
		frame->bboxmin.v[ i ] = pingroup->bboxmin.v[ i ];
		frame->bboxmax.v[ i ] = pingroup->bboxmax.v[ i ];
	}

	const dmdl_interval_t* pin_intervals = ( const dmdl_interval_t* )( pingroup + 1 );

	frame->interval = LittleFloat( pin_intervals->interval );

	pin_intervals += numframes;

	const void* ptemp = ( const void* )pin_intervals;

	aliastransform[ 0 ][ 0 ] = pheader->scale[ 0 ];
	aliastransform[ 1 ][ 1 ] = pheader->scale[ 1 ];
	aliastransform[ 2 ][ 2 ] = pheader->scale[ 2 ];
	aliastransform[ 0 ][ 3 ] = pheader->scale_origin[ 0 ];
	aliastransform[ 1 ][ 3 ] = pheader->scale_origin[ 1 ];
	aliastransform[ 2 ][ 3 ] = pheader->scale_origin[ 2 ];

	for ( int i = 0; i < numframes; i++ ) {
		poseverts[ posenum ] = ( const dmdl_trivertx_t* )( ( const dmdl_frame_t* )ptemp + 1 );

		for ( int j = 0; j < pheader->numverts; j++ ) {
			vec3_t in;
			in[ 0 ] = poseverts[ posenum ][ j ].v[ 0 ];
			in[ 1 ] = poseverts[ posenum ][ j ].v[ 1 ];
			in[ 2 ] = poseverts[ posenum ][ j ].v[ 2 ];
			vec3_t out;
			R_AliasTransformVector( in, out );
			for ( int k = 0; k < 3; k++ ) {
				if ( mins[ k ] > out[ k ] ) {
					mins[ k ] = out[ k ];
				}
				if ( maxs[ k ] < out[ k ] ) {
					maxs[ k ] = out[ k ];
				}
			}
		}

		posenum++;

		ptemp = ( const dmdl_trivertx_t* )( ( const dmdl_frame_t* )ptemp + 1 ) + pheader->numverts;
	}

	return ptemp;
}

static void* Mod_LoadAllSkins( int numskins, dmdl_skintype_t* pskintype, int mdl_flags ) {
	if ( numskins < 1 || numskins > MAX_MESH1_SKINS ) {
		common->FatalError( "Mod_LoadMdlModel: Invalid # of skins: %d\n", numskins );
	}

	int s = pheader->skinwidth * pheader->skinheight;

	int texture_mode = IMG8MODE_Skin;
	if ( GGameType & GAME_Hexen2 ) {
		if ( mdl_flags & H2MDLEF_HOLEY ) {
			texture_mode = IMG8MODE_SkinHoley;
		} else if ( mdl_flags & H2MDLEF_TRANSPARENT )     {
			texture_mode = IMG8MODE_SkinTransparent;
		} else if ( mdl_flags & H2MDLEF_SPECIAL_TRANS )     {
			texture_mode = IMG8MODE_SkinSpecialTrans;
		}
	}

	for ( int i = 0; i < numskins; i++ ) {
		if ( pskintype->type == ALIAS_SKIN_SINGLE ) {
			byte* pic32 = R_ConvertImage8To32( ( byte* )( pskintype + 1 ), pheader->skinwidth, pheader->skinheight, texture_mode );
			byte* picFullBright = R_GetFullBrightImage( ( byte* )( pskintype + 1 ), pic32, pheader->skinwidth, pheader->skinheight );

			// save 8 bit texels for the player model to remap
			if ( ( GGameType & GAME_Quake ) && !String::Cmp( loadmodel->name,"progs/player.mdl" ) ) {
				if ( s > ( int )sizeof ( q1_player_8bit_texels ) ) {
					common->FatalError( "Player skin too large" );
				}
				Com_Memcpy( q1_player_8bit_texels, ( byte* )( pskintype + 1 ), s );
			} else if ( GGameType & GAME_Hexen2 )     {
				if ( !String::Cmp( loadmodel->name,"models/paladin.mdl" ) ) {
					if ( s > ( int )sizeof ( h2_player_8bit_texels[ 0 ] ) ) {
						common->FatalError( "Player skin too large" );
					}
					Com_Memcpy( h2_player_8bit_texels[ 0 ], ( byte* )( pskintype + 1 ), s );
				} else if ( !String::Cmp( loadmodel->name,"models/crusader.mdl" ) )       {
					if ( s > ( int )sizeof ( h2_player_8bit_texels[ 1 ] ) ) {
						common->FatalError( "Player skin too large" );
					}
					Com_Memcpy( h2_player_8bit_texels[ 1 ], ( byte* )( pskintype + 1 ), s );
				} else if ( !String::Cmp( loadmodel->name,"models/necro.mdl" ) )       {
					if ( s > ( int )sizeof ( h2_player_8bit_texels[ 2 ] ) ) {
						common->FatalError( "Player skin too large" );
					}
					Com_Memcpy( h2_player_8bit_texels[ 2 ], ( byte* )( pskintype + 1 ), s );
				} else if ( !String::Cmp( loadmodel->name,"models/assassin.mdl" ) )       {
					if ( s > ( int )sizeof ( h2_player_8bit_texels[ 3 ] ) ) {
						common->FatalError( "Player skin too large" );
					}
					Com_Memcpy( h2_player_8bit_texels[ 3 ], ( byte* )( pskintype + 1 ), s );
				} else if ( !String::Cmp( loadmodel->name,"models/succubus.mdl" ) )       {
					if ( s > ( int )sizeof ( h2_player_8bit_texels[ 4 ] ) ) {
						common->FatalError( "Player skin too large" );
					}
					Com_Memcpy( h2_player_8bit_texels[ 4 ], ( byte* )( pskintype + 1 ), s );
				} else if ( !String::Cmp( loadmodel->name,"models/hank.mdl" ) )       {
					if ( s > ( int )sizeof ( h2_player_8bit_texels[ 5 ] ) ) {
						common->FatalError( "Player skin too large" );
					}
					Com_Memcpy( h2_player_8bit_texels[ 5 ], ( byte* )( pskintype + 1 ), s );
				}
			}

			char name[ 32 ];
			sprintf( name, "%s_%i", loadmodel->name, i );

			pheader->gl_texture[ i ][ 0 ] =
				pheader->gl_texture[ i ][ 1 ] =
					pheader->gl_texture[ i ][ 2 ] =
						pheader->gl_texture[ i ][ 3 ] =
							R_CreateImage( name, pic32, pheader->skinwidth, pheader->skinheight, true, true, GL_REPEAT, false );
			delete[] pic32;
			if ( picFullBright ) {
				char fbname[ 32 ];
				sprintf( fbname, "%s_%i_fb", loadmodel->name, i );
				pheader->fullBrightTexture[ i ][ 0 ] =
					pheader->fullBrightTexture[ i ][ 1 ] =
						pheader->fullBrightTexture[ i ][ 2 ] =
							pheader->fullBrightTexture[ i ][ 3 ] =
								R_CreateImage( fbname, picFullBright, pheader->skinwidth, pheader->skinheight, true, true, GL_REPEAT, false );
				delete[] picFullBright;
			} else {
				pheader->fullBrightTexture[ i ][ 0 ] =
					pheader->fullBrightTexture[ i ][ 1 ] =
						pheader->fullBrightTexture[ i ][ 2 ] =
							pheader->fullBrightTexture[ i ][ 3 ] = NULL;
			}
			pskintype = ( dmdl_skintype_t* )( ( byte* )( pskintype + 1 ) + s );
		} else {
			// animating skin group.  yuck.
			pskintype++;
			dmdl_skingroup_t* pinskingroup = ( dmdl_skingroup_t* )pskintype;
			int groupskins = LittleLong( pinskingroup->numskins );
			dmdl_skininterval_t* pinskinintervals = ( dmdl_skininterval_t* )( pinskingroup + 1 );

			pskintype = ( dmdl_skintype_t* )( pinskinintervals + groupskins );

			int j;
			bool haveFullBrightFrame = false;
			for ( j = 0; j < groupskins; j++ ) {
				char name[ 32 ];
				sprintf( name, "%s_%i_%i", loadmodel->name, i, j );

				byte* pic32 = R_ConvertImage8To32( ( byte* )pskintype, pheader->skinwidth, pheader->skinheight, texture_mode );
				byte* picFullBright = R_GetFullBrightImage( ( byte* )pskintype, pic32, pheader->skinwidth, pheader->skinheight );
				pheader->gl_texture[ i ][ j & 3 ] = R_CreateImage( name, pic32, pheader->skinwidth, pheader->skinheight, true, true, GL_REPEAT, false );
				delete[] pic32;
				if ( picFullBright ) {
					char fbname[ 32 ];
					sprintf( fbname, "%s_%i_%i_fb", loadmodel->name, i, j );
					pheader->fullBrightTexture[ i ][ j & 3 ] = R_CreateImage( fbname, picFullBright, pheader->skinwidth, pheader->skinheight, true, true, GL_REPEAT, false );
					delete[] picFullBright;
					haveFullBrightFrame = true;
				} else {
					pheader->fullBrightTexture[ i ][ j & 3 ] = NULL;
				}
				pskintype = ( dmdl_skintype_t* )( ( byte* )pskintype + s );
			}
			int k = j;
			for ( /* */; j < 4; j++ ) {
				pheader->gl_texture[ i ][ j & 3 ] = pheader->gl_texture[ i ][ j - k ];
				pheader->fullBrightTexture[ i ][ j & 3 ] = pheader->fullBrightTexture[ i ][ j - k ];
			}
			//	Make sure all fullbright textures are either all NULL or all are valid textures.
			// If some frames don't have it, use a fully transparent image.
			if ( haveFullBrightFrame ) {
				for ( int j = 0; j < 4; j++ ) {
					if ( !pheader->fullBrightTexture[ i ][ j ] ) {
						pheader->fullBrightTexture[ i ][ j ] = tr.transparentImage;
					}
				}
			}
		}
	}

	return ( void* )pskintype;
}

static int AddMdlVertex( int xyzIndex, int stIndex, bool onBackSide ) {
	for ( int i = 0; i < numorder; i++ ) {
		if ( vertexorder[ i ].xyzIndex == xyzIndex && vertexorder[ i ].stIndex == stIndex &&
			vertexorder[ i ].onBackSide == onBackSide ) {
			return i;
		}
	}

	vertexorder[ numorder ].xyzIndex = xyzIndex;
	vertexorder[ numorder ].stIndex = stIndex;
	vertexorder[ numorder ].onBackSide = onBackSide;
	return numorder++;
}

static void GL_MakeAliasModelDisplayLists( model_t* m, mesh1hdr_t* hdr ) {
	aliasmodel = m;
	paliashdr = hdr;	// (mesh1hdr_t *)Mod_Extradata (m);

	common->Printf( "meshing %s...\n",m->name );

	numorder = 0;
	paliashdr->numIndexes = pheader->numtris * 3;
	paliashdr->indexes = new glIndex_t[ paliashdr->numIndexes ];
	glIndex_t* indexes = paliashdr->indexes;
	for ( int i = 0; i < pheader->numtris; i++ ) {
		mmesh1triangle_t* triangle = &triangles[ i ];
		for ( int j = 0; j < 3; j++ ) {
			*indexes++ = AddMdlVertex( triangle->vertindex[ j ], triangle->stindex[ j ],
				!triangle->facesfront && stverts[ triangle->stindex[ j ] ].onseam );
		}
	}

	// save the data out

	paliashdr->poseverts = numorder;

	dmdl_trivertx_t* verts = new dmdl_trivertx_t[ paliashdr->numposes * paliashdr->poseverts ];
	paliashdr->posedata = verts;
	for ( int i = 0; i < paliashdr->numposes; i++ ) {
		for ( int j = 0; j < numorder; j++ ) {
			*verts++ = poseverts[ i ][ vertexorder[ j ].xyzIndex ];
		}
	}

	idVec2* texCoords = new idVec2[ paliashdr->poseverts ];
	paliashdr->texCoords = texCoords;
	for ( int i = 0; i < paliashdr->poseverts; i++, texCoords++ ) {
		int k = vertexorder[ i ].stIndex;

		// emit s/t coords into the commands stream
		float s = stverts[ k ].s;
		float t = stverts[ k ].t;

		if ( vertexorder[ i ].onBackSide ) {
			s += pheader->skinwidth / 2;	// on back side
		}
		texCoords->x = ( s + 0.5f ) / pheader->skinwidth;
		texCoords->y = ( t + 0.5f ) / pheader->skinheight;
	}
}

void Mod_LoadMdlModel( model_t* mod, const void* buffer ) {
	mdl_t* pinmodel = ( mdl_t* )buffer;

	int version = LittleLong( pinmodel->version );
	if ( version != MESH1_VERSION ) {
		common->FatalError( "%s has wrong version number (%i should be %i)",
			mod->name, version, MESH1_VERSION );
	}

	//
	// allocate space for a working header, plus all the data except the frames,
	// skin and group info
	//
	int size = sizeof ( mesh1hdr_t ) + ( LittleLong( pinmodel->numframes ) - 1 ) * sizeof ( pheader->frames[ 0 ] );
	pheader = ( mesh1hdr_t* )Mem_Alloc( size );
	pheader->ident = SF_MDL;

	mod->q1_flags = LittleLong( pinmodel->flags );

	//
	// endian-adjust and copy the data, starting with the alias model header
	//
	pheader->boundingradius = LittleFloat( pinmodel->boundingradius );
	pheader->numskins = LittleLong( pinmodel->numskins );
	pheader->skinwidth = LittleLong( pinmodel->skinwidth );
	pheader->skinheight = LittleLong( pinmodel->skinheight );

	if ( pheader->skinheight > MAX_LBM_HEIGHT ) {
		common->FatalError( "model %s has a skin taller than %d", mod->name, MAX_LBM_HEIGHT );
	}

	pheader->numverts = LittleLong( pinmodel->numverts );

	if ( pheader->numverts <= 0 ) {
		common->FatalError( "model %s has no vertices", mod->name );
	}

	if ( pheader->numverts > MAXALIASVERTS ) {
		common->FatalError( "model %s has too many vertices", mod->name );
	}

	pheader->numtris = LittleLong( pinmodel->numtris );

	if ( pheader->numtris <= 0 ) {
		common->FatalError( "model %s has no triangles", mod->name );
	}

	pheader->numframes = LittleLong( pinmodel->numframes );
	int numframes = pheader->numframes;
	if ( numframes < 1 ) {
		common->FatalError( "Mod_LoadMdlModel: Invalid # of frames: %d\n", numframes );
	}

	pheader->size = LittleFloat( pinmodel->size ) * ALIAS_BASE_SIZE_RATIO;
	mod->q1_synctype = ( synctype_t )LittleLong( pinmodel->synctype );
	mod->q1_numframes = pheader->numframes;

	for ( int i = 0; i < 3; i++ ) {
		pheader->scale[ i ] = LittleFloat( pinmodel->scale[ i ] );
		pheader->scale_origin[ i ] = LittleFloat( pinmodel->scale_origin[ i ] );
		pheader->eyeposition[ i ] = LittleFloat( pinmodel->eyeposition[ i ] );
	}

	//
	// load the skins
	//
	dmdl_skintype_t* pskintype = ( dmdl_skintype_t* )&pinmodel[ 1 ];
	pskintype = ( dmdl_skintype_t* )Mod_LoadAllSkins( pheader->numskins, pskintype, mod->q1_flags );

	//
	// load base s and t vertices
	//
	dmdl_stvert_t* pinstverts = ( dmdl_stvert_t* )pskintype;
	for ( int i = 0; i < pheader->numverts; i++ ) {
		stverts[ i ].onseam = LittleLong( pinstverts[ i ].onseam );
		stverts[ i ].s = LittleLong( pinstverts[ i ].s );
		stverts[ i ].t = LittleLong( pinstverts[ i ].t );
	}

	//
	// load triangle lists
	//
	dmdl_triangle_t* pintriangles = ( dmdl_triangle_t* )&pinstverts[ pheader->numverts ];

	for ( int i = 0; i < pheader->numtris; i++ ) {
		triangles[ i ].facesfront = LittleLong( pintriangles[ i ].facesfront );

		for ( int j = 0; j < 3; j++ ) {
			triangles[ i ].vertindex[ j ] = LittleLong( pintriangles[ i ].vertindex[ j ] );
			triangles[ i ].stindex[ j ]   = triangles[ i ].vertindex[ j ];
		}
	}

	//
	// load the frames
	//
	posenum = 0;
	dmdl_frametype_t* pframetype = ( dmdl_frametype_t* )&pintriangles[ pheader->numtris ];

	mins[ 0 ] = mins[ 1 ] = mins[ 2 ] = 32768;
	maxs[ 0 ] = maxs[ 1 ] = maxs[ 2 ] = -32768;

	for ( int i = 0; i < numframes; i++ ) {
		mdl_frametype_t frametype = ( mdl_frametype_t )LittleLong( pframetype->type );
		if ( frametype == ALIAS_SINGLE ) {
			pframetype = ( dmdl_frametype_t* )Mod_LoadAliasFrame( pframetype + 1, &pheader->frames[ i ] );
		} else {
			pframetype = ( dmdl_frametype_t* )Mod_LoadAliasGroup( pframetype + 1, &pheader->frames[ i ] );
		}
	}

	pheader->numposes = posenum;

	mod->type = MOD_MESH1;

	// FIXME: do this right
	if ( GGameType & GAME_Hexen2 ) {
		mod->q1_mins[ 0 ] = mins[ 0 ] - 10;
		mod->q1_mins[ 1 ] = mins[ 1 ] - 10;
		mod->q1_mins[ 2 ] = mins[ 2 ] - 10;
		mod->q1_maxs[ 0 ] = maxs[ 0 ] + 10;
		mod->q1_maxs[ 1 ] = maxs[ 1 ] + 10;
		mod->q1_maxs[ 2 ] = maxs[ 2 ] + 10;
	} else {
		mod->q1_mins[ 0 ] = mod->q1_mins[ 1 ] = mod->q1_mins[ 2 ] = -16;
		mod->q1_maxs[ 0 ] = mod->q1_maxs[ 1 ] = mod->q1_maxs[ 2 ] = 16;
	}

	//
	// build the draw lists
	//
	GL_MakeAliasModelDisplayLists( mod, pheader );

	mod->q1_cache = pheader;
}

//	Reads extra field for num ST verts, and extra index list of them
void Mod_LoadMdlModelNew( model_t* mod, const void* buffer ) {
	newmdl_t* pinmodel = ( newmdl_t* )buffer;

	int version = LittleLong( pinmodel->version );
	if ( version != MESH1_NEWVERSION ) {
		common->FatalError( "%s has wrong version number (%i should be %i)",
			mod->name, version, MESH1_NEWVERSION );
	}

	//
	// allocate space for a working header, plus all the data except the frames,
	// skin and group info
	//
	int size = sizeof ( mesh1hdr_t ) + ( LittleLong( pinmodel->numframes ) - 1 ) * sizeof ( pheader->frames[ 0 ] );
	pheader = ( mesh1hdr_t* )Mem_Alloc( size );
	pheader->ident = SF_MDL;

	mod->q1_flags = LittleLong( pinmodel->flags );

	//
	// endian-adjust and copy the data, starting with the alias model header
	//
	pheader->boundingradius = LittleFloat( pinmodel->boundingradius );
	pheader->numskins = LittleLong( pinmodel->numskins );
	pheader->skinwidth = LittleLong( pinmodel->skinwidth );
	pheader->skinheight = LittleLong( pinmodel->skinheight );

	if ( pheader->skinheight > MAX_LBM_HEIGHT ) {
		common->FatalError( "model %s has a skin taller than %d", mod->name, MAX_LBM_HEIGHT );
	}

	pheader->numverts = LittleLong( pinmodel->numverts );
	pheader->version = LittleLong( pinmodel->num_st_verts );	//hide num_st in version

	if ( pheader->numverts <= 0 ) {
		common->FatalError( "model %s has no vertices", mod->name );
	}

	if ( pheader->numverts > MAXALIASVERTS ) {
		common->FatalError( "model %s has too many vertices", mod->name );
	}

	pheader->numtris = LittleLong( pinmodel->numtris );

	if ( pheader->numtris <= 0 ) {
		common->FatalError( "model %s has no triangles", mod->name );
	}

	pheader->numframes = LittleLong( pinmodel->numframes );
	int numframes = pheader->numframes;
	if ( numframes < 1 ) {
		common->FatalError( "Mod_LoadMdlModel: Invalid # of frames: %d\n", numframes );
	}

	pheader->size = LittleFloat( pinmodel->size ) * ALIAS_BASE_SIZE_RATIO;
	mod->q1_synctype = ( synctype_t )LittleLong( pinmodel->synctype );
	mod->q1_numframes = pheader->numframes;

	for ( int i = 0; i < 3; i++ ) {
		pheader->scale[ i ] = LittleFloat( pinmodel->scale[ i ] );
		pheader->scale_origin[ i ] = LittleFloat( pinmodel->scale_origin[ i ] );
		pheader->eyeposition[ i ] = LittleFloat( pinmodel->eyeposition[ i ] );
	}

	//
	// load the skins
	//
	dmdl_skintype_t* pskintype = ( dmdl_skintype_t* )&pinmodel[ 1 ];
	pskintype = ( dmdl_skintype_t* )Mod_LoadAllSkins( pheader->numskins, pskintype, mod->q1_flags );

	//
	// load base s and t vertices
	//
	dmdl_stvert_t* pinstverts = ( dmdl_stvert_t* )pskintype;

	for ( int i = 0; i < pheader->version; i++ ) {	//version holds num_st_verts
		stverts[ i ].onseam = LittleLong( pinstverts[ i ].onseam );
		stverts[ i ].s = LittleLong( pinstverts[ i ].s );
		stverts[ i ].t = LittleLong( pinstverts[ i ].t );
	}

	//
	// load triangle lists
	//
	dmdl_newtriangle_t* pintriangles = ( dmdl_newtriangle_t* )&pinstverts[ pheader->version ];

	for ( int i = 0; i < pheader->numtris; i++ ) {
		triangles[ i ].facesfront = LittleLong( pintriangles[ i ].facesfront );

		for ( int j = 0; j < 3; j++ ) {
			triangles[ i ].vertindex[ j ] = LittleShort( pintriangles[ i ].vertindex[ j ] );
			triangles[ i ].stindex[ j ] = LittleShort( pintriangles[ i ].stindex[ j ] );
		}
	}

	//
	// load the frames
	//
	posenum = 0;
	dmdl_frametype_t* pframetype = ( dmdl_frametype_t* )&pintriangles[ pheader->numtris ];

	mins[ 0 ] = mins[ 1 ] = mins[ 2 ] = 32768;
	maxs[ 0 ] = maxs[ 1 ] = maxs[ 2 ] = -32768;

	for ( int i = 0; i < numframes; i++ ) {
		mdl_frametype_t frametype;

		frametype = ( mdl_frametype_t )LittleLong( pframetype->type );

		if ( frametype == ALIAS_SINGLE ) {
			pframetype = ( dmdl_frametype_t* )Mod_LoadAliasFrame( pframetype + 1, &pheader->frames[ i ] );
		} else {
			pframetype = ( dmdl_frametype_t* )Mod_LoadAliasGroup( pframetype + 1, &pheader->frames[ i ] );
		}
	}

	pheader->numposes = posenum;

	mod->type = MOD_MESH1;

	mod->q1_mins[ 0 ] = mins[ 0 ] - 10;
	mod->q1_mins[ 1 ] = mins[ 1 ] - 10;
	mod->q1_mins[ 2 ] = mins[ 2 ] - 10;
	mod->q1_maxs[ 0 ] = maxs[ 0 ] + 10;
	mod->q1_maxs[ 1 ] = maxs[ 1 ] + 10;
	mod->q1_maxs[ 2 ] = maxs[ 2 ] + 10;


	//
	// build the draw lists
	//
	GL_MakeAliasModelDisplayLists( mod, pheader );

	mod->q1_cache = pheader;
}

void Mod_FreeMdlModel( model_t* mod ) {
	mesh1hdr_t* pheader = ( mesh1hdr_t* )mod->q1_cache;
	delete[] pheader->indexes;
	delete[] pheader->posedata;
	delete[] pheader->texCoords;
	Mem_Free( pheader );
}

void R_AddMdlSurfaces( trRefEntity_t* e, int forcedSortIndex ) {
	if ( ( tr.currentEntity->e.renderfx & RF_THIRD_PERSON ) && !tr.viewParms.isPortal ) {
		return;
	}

	if ( R_CullLocalBox( &tr.currentModel->q1_mins ) == CULL_OUT ) {
		return;
	}
	mesh1hdr_t* paliashdr = ( mesh1hdr_t* )tr.currentModel->q1_cache;
	R_AddDrawSurf( ( surfaceType_t* )paliashdr, tr.defaultShader, 0, false, false, ATI_TESS_NONE, forcedSortIndex );
	if ( r_shadows->value ) {
		R_AddDrawSurf( ( surfaceType_t* )paliashdr, tr.shadowShader, 0, false, false, ATI_TESS_NONE, 1 );
	}
}

static void GL_DrawAliasShadow() {
	float lheight = backEnd.currentEntity->e.origin[ 2 ] - lightspot[ 2 ];

	float height = 0;

	height = -lheight + 1.0;

	vec3_t shadevector;
	VectorCopy( backEnd.currentEntity->e.axis[ 0 ], shadevector );
	shadevector[ 2 ] = 1;
	VectorNormalize( shadevector );

	for ( int i = 0; i < tess.numVertexes; i++ ) {
		// normals and vertexes come from the frame list
		vec3_t point;
		point[ 0 ] = tess.xyz[ i ][ 0 ];
		point[ 1 ] = tess.xyz[ i ][ 1 ];
		point[ 2 ] = tess.xyz[ i ][ 2 ];

		point[ 0 ] -= shadevector[ 0 ] * ( point[ 2 ] + lheight );
		point[ 1 ] -= shadevector[ 1 ] * ( point[ 2 ] + lheight );
		point[ 2 ] = height;
		tess.xyz[ i ][ 0 ] = point[ 0 ];
		tess.xyz[ i ][ 1 ] = point[ 1 ];
		tess.xyz[ i ][ 2 ] = point[ 2 ];
	}

	shaderStage_t stage = {};
	stage.bundle[ 0 ].image[ 0 ] = tr.whiteImage;
	stage.bundle[ 0 ].numImageAnimations = 1;
	stage.bundle[ 0 ].tcGen = TCGEN_IDENTITY;
	stage.stateBits = GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	stage.rgbGen = CGEN_CONST;
	stage.alphaGen = AGEN_CONST;
	stage.constantColor[ 3 ] = 127;
	shader_t shader = {};
	shader.stages[ 0 ] = &stage;
	tess.shader = &shader;
	tess.xstages = shader.stages;
	tess.dlightBits = 0;
	shader.cullType = CT_FRONT_SIDED;
	GL_Cull( shader.cullType );
	RB_StageIteratorGenericTemp( &tess );
	tess.numIndexes = 0;
	tess.numVertexes = 0;
}

float R_CalcEntityLight( refEntity_t* e ) {
	float* lorg = e->origin;
	if ( e->renderfx & RF_LIGHTING_ORIGIN ) {
		lorg = e->lightingOrigin;
	}

	float light;
	if ( GGameType & GAME_Hexen2 ) {
		vec3_t adjust_origin;
		VectorCopy( lorg, adjust_origin );
		model_t* clmodel = R_GetModelByHandle( e->hModel );
		adjust_origin[ 2 ] += ( clmodel->q1_mins[ 2 ] + clmodel->q1_maxs[ 2 ] ) / 2;
		light = R_LightPointQ1( adjust_origin );
	} else {
		light = R_LightPointQ1( lorg );
	}

	// allways give the gun some light
	if ( ( e->renderfx & RF_MINLIGHT ) && light < 24 ) {
		light = 24;
	}

	for ( int lnum = 0; lnum < tr.refdef.num_dlights; lnum++ ) {
		vec3_t dist;
		VectorSubtract( lorg, tr.refdef.dlights[ lnum ].origin, dist );
		float add = tr.refdef.dlights[ lnum ].radius - VectorLength( dist );

		if ( add > 0 ) {
			light += add;
		}
	}
	return light;
}

static void R_DrawBaseMdlSurface( trRefEntity_t* ent, mesh1hdr_t* paliashdr, model_t* clmodel ) {
	//
	// draw all the triangles
	//

	bool doOverBright = !!r_drawOverBrights->integer;
	shader_t shader = {};
	shader.cullType = CT_FRONT_SIDED;
	shaderStage_t stage1 = {};
	shaderStage_t stage2 = {};
	shaderStage_t stage3 = {};
	shaderStage_t stage4 = {};
	if ( GGameType & GAME_Hexen2 ) {
		if ( clmodel->q1_flags & H2MDLEF_SPECIAL_TRANS ) {
			shader.cullType = CT_TWO_SIDED;
			stage1.stateBits = GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA | GLS_DSTBLEND_SRC_ALPHA;
			stage1.alphaGen = AGEN_IDENTITY;
			doOverBright = false;
		} else if ( backEnd.currentEntity->e.renderfx & RF_WATERTRANS ) {
			stage1.stateBits = GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			stage1.alphaGen = AGEN_CONST;
			stage1.constantColor[ 3 ] = r_wateralpha->value * 255;
			doOverBright = false;
		} else if ( clmodel->q1_flags & H2MDLEF_TRANSPARENT ) {
			stage1.stateBits = GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			stage1.alphaGen = AGEN_IDENTITY;
			doOverBright = false;
		} else if ( clmodel->q1_flags & H2MDLEF_HOLEY ) {
			stage1.stateBits = GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			stage1.alphaGen = AGEN_IDENTITY;
			doOverBright = false;
		} else {
			stage1.stateBits = GLS_DEPTHMASK_TRUE;
			stage1.alphaGen = AGEN_IDENTITY;
		}
	} else {
		stage1.stateBits = GLS_DEFAULT;
		stage1.alphaGen = AGEN_IDENTITY;
	}

	if ( ent->e.customSkin ) {
		stage1.bundle[ 0 ].image[ 0 ] = tr.images[ ent->e.customSkin ];
		stage2.bundle[ 0 ].image[ 0 ] = tr.images[ ent->e.customSkin ];
		stage1.bundle[ 0 ].numImageAnimations = 1;
		stage2.bundle[ 0 ].numImageAnimations = 1;
	} else {
		stage1.bundle[ 0 ].image[ 0 ] = paliashdr->gl_texture[ ent->e.skinNum ][ 0 ];
		stage1.bundle[ 0 ].image[ 1 ] = paliashdr->gl_texture[ ent->e.skinNum ][ 1 ];
		stage1.bundle[ 0 ].image[ 2 ] = paliashdr->gl_texture[ ent->e.skinNum ][ 2 ];
		stage1.bundle[ 0 ].image[ 3 ] = paliashdr->gl_texture[ ent->e.skinNum ][ 3 ];
		stage2.bundle[ 0 ].image[ 0 ] = paliashdr->gl_texture[ ent->e.skinNum ][ 0 ];
		stage2.bundle[ 0 ].image[ 1 ] = paliashdr->gl_texture[ ent->e.skinNum ][ 1 ];
		stage2.bundle[ 0 ].image[ 2 ] = paliashdr->gl_texture[ ent->e.skinNum ][ 2 ];
		stage2.bundle[ 0 ].image[ 3 ] = paliashdr->gl_texture[ ent->e.skinNum ][ 3 ];
		stage1.bundle[ 0 ].numImageAnimations = 4;
		stage1.bundle[ 0 ].imageAnimationSpeed = 10;
		stage2.bundle[ 0 ].numImageAnimations = 4;
		stage2.bundle[ 0 ].imageAnimationSpeed = 10;
	}
	stage1.bundle[ 0 ].tcGen = TCGEN_TEXTURE;
	stage2.bundle[ 0 ].tcGen = TCGEN_TEXTURE;
	stage1.rgbGen = CGEN_LIGHTING_DIFFUSE;
	stage2.rgbGen = CGEN_LIGHTING_DIFFUSE_OVER_BRIGHT;
	stage2.alphaGen = AGEN_IDENTITY;

	int numStages = 0;
	shader.stages[ numStages++ ] = &stage1;

	if ( doOverBright ) {
		stage2.stateBits = GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
		shader.stages[ numStages++ ] = &stage2;
	}

	if ( !ent->e.customSkin && paliashdr->fullBrightTexture[ ent->e.skinNum ][ 0 ] ) {
		stage3.bundle[ 0 ].image[ 0 ] = paliashdr->fullBrightTexture[ ent->e.skinNum ][ 0 ];
		stage3.bundle[ 0 ].image[ 1 ] = paliashdr->fullBrightTexture[ ent->e.skinNum ][ 1 ];
		stage3.bundle[ 0 ].image[ 2 ] = paliashdr->fullBrightTexture[ ent->e.skinNum ][ 2 ];
		stage3.bundle[ 0 ].image[ 3 ] = paliashdr->fullBrightTexture[ ent->e.skinNum ][ 3 ];
		stage3.bundle[ 0 ].numImageAnimations = 4;
		stage3.bundle[ 0 ].imageAnimationSpeed = 10;
		stage3.bundle[ 0 ].tcGen = TCGEN_TEXTURE;
		stage3.stateBits = GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
		stage3.rgbGen = CGEN_IDENTITY;
		stage3.alphaGen = AGEN_IDENTITY;
		shader.stages[ numStages++ ] = &stage3;
	}

	if ( ent->e.renderfx & RF_COLORSHADE ) {
		stage4.bundle[ 0 ].image[ 0 ] = tr.whiteImage;
		stage4.bundle[ 0 ].numImageAnimations = 1;
		stage4.bundle[ 0 ].tcGen = TCGEN_TEXTURE;
		stage4.stateBits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
		stage4.rgbGen = CGEN_ENTITY;
		stage4.alphaGen = AGEN_ENTITY;
		shader.stages[ numStages++ ] = &stage4;
	}

	tess.shader = &shader;
	tess.xstages = shader.stages;
	tess.dlightBits = 0;
	GL_Cull( shader.cullType );
	RB_StageIteratorGenericTemp( &tess );

	tess.numIndexes = 0;
	tess.numVertexes = 0;
}

static void EmitMdlVertexesAndIndexes( trRefEntity_t* ent, mesh1hdr_t* paliashdr ) {
	if ( ent->e.frame >= paliashdr->numframes || ent->e.frame < 0 ) {
		common->DPrintf( "R_AliasSetupFrame: no such frame %d\n", ent->e.frame );
		ent->e.frame = 0;
	}

	int posenum = paliashdr->frames[ ent->e.frame ].firstpose;
	int numposes = paliashdr->frames[ ent->e.frame ].numposes;

	if ( numposes > 1 ) {
		float interval = paliashdr->frames[ ent->e.frame ].interval;
		posenum += ( int )( tr.refdef.floatTime / interval ) % numposes;
	}

	dmdl_trivertx_t* verts = paliashdr->posedata;
	verts += posenum * paliashdr->poseverts;

	for ( int i = 0; i < paliashdr->poseverts; i++ ) {
		tess.texCoords[ i ][ 0 ][ 0 ] = paliashdr->texCoords[ i ].x;
		tess.texCoords[ i ][ 0 ][ 1 ] = paliashdr->texCoords[ i ].y;
		tess.xyz[ i ][ 0 ] = verts[ i ].v[ 0 ] * paliashdr->scale[ 0 ] + paliashdr->scale_origin[ 0 ];
		tess.xyz[ i ][ 1 ] = verts[ i ].v[ 1 ] * paliashdr->scale[ 1 ] + paliashdr->scale_origin[ 1 ];
		tess.xyz[ i ][ 2 ] = verts[ i ].v[ 2 ] * paliashdr->scale[ 2 ] + paliashdr->scale_origin[ 2 ];
		tess.normal[ i ][ 0 ] = bytedirs[ verts[ i ].lightnormalindex ][ 0 ];
		tess.normal[ i ][ 1 ] = bytedirs[ verts[ i ].lightnormalindex ][ 1 ];
		tess.normal[ i ][ 2 ] = bytedirs[ verts[ i ].lightnormalindex ][ 2 ];
	}
	tess.numVertexes = paliashdr->poseverts;
	tess.numIndexes = paliashdr->numIndexes;
	Com_Memcpy( tess.indexes, paliashdr->indexes, paliashdr->numIndexes * sizeof( glIndex_t ) );
}

void RB_SurfaceMdl( mesh1hdr_t* paliashdr ) {
	trRefEntity_t* ent = backEnd.currentEntity;

	//
	// get lighting information
	//
	float ambientlight = R_CalcEntityLight( &ent->e );
	float shadelight = ambientlight;

	if ( ent->e.renderfx & RF_FIRST_PERSON ) {
		r_lightlevel->value = ambientlight;
	}

	// clamp lighting so it doesn't overbright as much
	if ( ambientlight > 128 ) {
		ambientlight = 128;
	}
	if ( ambientlight + shadelight > 192 ) {
		shadelight = 192 - ambientlight;
	}

	model_t* clmodel = R_GetModelByHandle( ent->e.hModel );

	// ZOID: never allow players to go totally black
	if ( ( GGameType & GAME_Quake ) && !String::Cmp( clmodel->name, "progs/player.mdl" ) ) {
		if ( ambientlight < 8 ) {
			ambientlight = shadelight = 8;
		}
	}

	if ( ent->e.renderfx & RF_ABSOLUTE_LIGHT ) {
		ambientlight = ent->e.absoluteLight * 128.0;
		shadelight = 0;
	}

	ent->ambientLight[ 0 ] = ambientlight * 2;
	ent->ambientLight[ 1 ] = ambientlight * 2;
	ent->ambientLight[ 2 ] = ambientlight * 2;
	ent->directedLight[ 0 ] = shadelight * 2;
	ent->directedLight[ 1 ] = shadelight * 2;
	ent->directedLight[ 2 ] = shadelight * 2;

	// clamp ambient
	for ( int i = 0; i < 3; i++ ) {
		if ( ent->ambientLight[ i ] > tr.identityLightByte ) {
			ent->ambientLight[ i ] = tr.identityLightByte;
		}
	}

	// save out the byte packet version
	( ( byte* )&ent->ambientLightInt )[ 0 ] = idMath::FtoiFast( ent->ambientLight[ 0 ] );
	( ( byte* )&ent->ambientLightInt )[ 1 ] = idMath::FtoiFast( ent->ambientLight[ 1 ] );
	( ( byte* )&ent->ambientLightInt )[ 2 ] = idMath::FtoiFast( ent->ambientLight[ 2 ] );
	( ( byte* )&ent->ambientLightInt )[ 3 ] = 0xff;

	// transform the direction to local space
	float lightDir[3] = {1, 0, 1};
	VectorNormalize( lightDir );
	ent->lightDir[ 0 ] = DotProduct( lightDir, ent->e.axis[ 0 ] );
	ent->lightDir[ 1 ] = DotProduct( lightDir, ent->e.axis[ 1 ] );
	ent->lightDir[ 2 ] = DotProduct( lightDir, ent->e.axis[ 2 ] );

	c_alias_polys += paliashdr->numtris;

	EmitMdlVertexesAndIndexes( ent, paliashdr );

	if ( tess.shader == tr.shadowShader ) {
		GL_DrawAliasShadow();
	} else {
		R_DrawBaseMdlSurface( ent, paliashdr, clmodel );
	}
}

bool R_MdlHasHexen2Transparency( model_t* Model ) {
	return !!( Model->q1_flags & ( H2MDLEF_TRANSPARENT | H2MDLEF_HOLEY | H2MDLEF_SPECIAL_TRANS ) );
}
