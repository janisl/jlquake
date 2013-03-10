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

static qboolean used[ 8192 ];

// the command list holds counts and s/t values that are valid for
// every frame
static int commands[ 8192 ];
static int numcommands;

// all frames will have their vertexes rearranged and expanded
// so they are in the order expected by the command list
static int vertexorder[ 8192 ];
static int numorder;

static int stripverts[ 128 ];
static int striptris[ 128 ];
static int stripstverts[ 128 ];
static int stripcount;

static int lastposenum;
static float shadelight;
static float ambientlight;
static float model_constant_alpha;

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
			} else   {
				pheader->fullBrightTexture[ i ][ 0 ] =
					pheader->fullBrightTexture[ i ][ 1 ] =
						pheader->fullBrightTexture[ i ][ 2 ] =
							pheader->fullBrightTexture[ i ][ 3 ] = NULL;
			}
			pskintype = ( dmdl_skintype_t* )( ( byte* )( pskintype + 1 ) + s );
		} else   {
			// animating skin group.  yuck.
			pskintype++;
			dmdl_skingroup_t* pinskingroup = ( dmdl_skingroup_t* )pskintype;
			int groupskins = LittleLong( pinskingroup->numskins );
			dmdl_skininterval_t* pinskinintervals = ( dmdl_skininterval_t* )( pinskingroup + 1 );

			pskintype = ( dmdl_skintype_t* )( pinskinintervals + groupskins );

			int j;
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
				} else   {
					pheader->fullBrightTexture[ i ][ j & 3 ] = NULL;
				}
				pskintype = ( dmdl_skintype_t* )( ( byte* )pskintype + s );
			}
			int k = j;
			for ( /* */; j < 4; j++ ) {
				pheader->gl_texture[ i ][ j & 3 ] = pheader->gl_texture[ i ][ j - k ];
				pheader->fullBrightTexture[ i ][ j & 3 ] = pheader->fullBrightTexture[ i ][ j - k ];
			}
		}
	}

	return ( void* )pskintype;
}

static int StripLength( int starttri, int startv ) {
	used[ starttri ] = 2;

	mmesh1triangle_t* last = &triangles[ starttri ];

	stripverts[ 0 ] = last->vertindex[ startv % 3 ];
	stripstverts[ 0 ] = last->stindex[ startv % 3 ];

	stripverts[ 1 ] = last->vertindex[ ( startv + 1 ) % 3 ];
	stripstverts[ 1 ] = last->stindex[ ( startv + 1 ) % 3 ];

	stripverts[ 2 ] = last->vertindex[ ( startv + 2 ) % 3 ];
	stripstverts[ 2 ] = last->stindex[ ( startv + 2 ) % 3 ];

	striptris[ 0 ] = starttri;
	stripcount = 1;

	int m1 = last->vertindex[ ( startv + 2 ) % 3 ];
	int st1 = last->stindex[ ( startv + 2 ) % 3 ];
	int m2 = last->vertindex[ ( startv + 1 ) % 3 ];
	int st2 = last->stindex[ ( startv + 1 ) % 3 ];

	// look for a matching triangle
nexttri:
	mmesh1triangle_t * check = &triangles[ starttri + 1 ];
	for ( int j = starttri + 1; j < pheader->numtris; j++, check++ ) {
		if ( check->facesfront != last->facesfront ) {
			continue;
		}
		for ( int k = 0; k < 3; k++ ) {
			if ( check->vertindex[ k ] != m1 ) {
				continue;
			}
			if ( check->stindex[ k ] != st1 ) {
				continue;
			}
			if ( check->vertindex[ ( k + 1 ) % 3 ] != m2 ) {
				continue;
			}
			if ( check->stindex[ ( k + 1 ) % 3 ] != st2 ) {
				continue;
			}

			// this is the next part of the fan

			// if we can't use this triangle, this tristrip is done
			if ( used[ j ] ) {
				goto done;
			}

			// the new edge
			if ( stripcount & 1 ) {
				m2 = check->vertindex[ ( k + 2 ) % 3 ];
				st2 = check->stindex[ ( k + 2 ) % 3 ];
			} else   {
				m1 = check->vertindex[ ( k + 2 ) % 3 ];
				st1 = check->stindex[ ( k + 2 ) % 3 ];
			}

			stripverts[ stripcount + 2 ] = check->vertindex[ ( k + 2 ) % 3 ];
			stripstverts[ stripcount + 2 ] = check->stindex[ ( k + 2 ) % 3 ];
			striptris[ stripcount ] = j;
			stripcount++;

			used[ j ] = 2;
			goto nexttri;
		}
	}
done:

	// clear the temp used flags
	for ( int j = starttri + 1; j < pheader->numtris; j++ ) {
		if ( used[ j ] == 2 ) {
			used[ j ] = 0;
		}
	}

	return stripcount;
}

static int FanLength( int starttri, int startv ) {
	used[ starttri ] = 2;

	mmesh1triangle_t* last = &triangles[ starttri ];

	stripverts[ 0 ] = last->vertindex[ startv % 3 ];
	stripstverts[ 0 ] = last->stindex[ startv % 3 ];

	stripverts[ 1 ] = last->vertindex[ ( startv + 1 ) % 3 ];
	stripstverts[ 1 ] = last->stindex[ ( startv + 1 ) % 3 ];

	stripverts[ 2 ] = last->vertindex[ ( startv + 2 ) % 3 ];
	stripstverts[ 2 ] = last->stindex[ ( startv + 2 ) % 3 ];

	striptris[ 0 ] = starttri;
	stripcount = 1;

	int m1 = last->vertindex[ ( startv + 0 ) % 3 ];
	int st1 = last->stindex[ ( startv + 2 ) % 3 ];
	int m2 = last->vertindex[ ( startv + 2 ) % 3 ];
	int st2 = last->stindex[ ( startv + 1 ) % 3 ];


	// look for a matching triangle
nexttri:
	mmesh1triangle_t * check = &triangles[ starttri + 1 ];
	for ( int j = starttri + 1; j < pheader->numtris; j++, check++ ) {
		if ( check->facesfront != last->facesfront ) {
			continue;
		}
		for ( int k = 0; k < 3; k++ ) {
			if ( check->vertindex[ k ] != m1 ) {
				continue;
			}
			if ( check->stindex[ k ] != st1 ) {
				continue;
			}
			if ( check->vertindex[ ( k + 1 ) % 3 ] != m2 ) {
				continue;
			}
			if ( check->stindex[ ( k + 1 ) % 3 ] != st2 ) {
				continue;
			}

			// this is the next part of the fan

			// if we can't use this triangle, this tristrip is done
			if ( used[ j ] ) {
				goto done;
			}

			// the new edge
			m2 = check->vertindex[ ( k + 2 ) % 3 ];
			st2 = check->stindex[ ( k + 2 ) % 3 ];

			stripverts[ stripcount + 2 ] = m2;
			stripstverts[ stripcount + 2 ] = st2;
			striptris[ stripcount ] = j;
			stripcount++;

			used[ j ] = 2;
			goto nexttri;
		}
	}
done:

	// clear the temp used flags
	for ( int j = starttri + 1; j < pheader->numtris; j++ ) {
		if ( used[ j ] == 2 ) {
			used[ j ] = 0;
		}
	}

	return stripcount;
}

//	Generate a list of trifans or strips for the model, which holds for all
// frames
static void BuildTris() {
	//
	// build tristrips
	//
	numorder = 0;
	numcommands = 0;
	Com_Memset( used, 0, sizeof ( used ) );
	for ( int i = 0; i < pheader->numtris; i++ ) {
		// pick an unused triangle and start the trifan
		if ( used[ i ] ) {
			continue;
		}

		int bestlen = 0;
		int besttype = 0;
		int bestverts[ 1024 ];
		int besttris[ 1024 ];
		int beststverts[ 1024 ];
		for ( int type = 0; type < 2; type++ ) {
			for ( int startv = 0; startv < 3; startv++ ) {
				int len;
				if ( type == 1 ) {
					len = StripLength( i, startv );
				} else   {
					len = FanLength( i, startv );
				}
				if ( len > bestlen ) {
					besttype = type;
					bestlen = len;
					for ( int j = 0; j < bestlen + 2; j++ ) {
						beststverts[ j ] = stripstverts[ j ];
						bestverts[ j ] = stripverts[ j ];
					}
					for ( int j = 0; j < bestlen; j++ ) {
						besttris[ j ] = striptris[ j ];
					}
				}
			}
		}

		// mark the tris on the best strip as used
		for ( int j = 0; j < bestlen; j++ ) {
			used[ besttris[ j ] ] = 1;
		}

		if ( besttype == 1 ) {
			commands[ numcommands++ ] = ( bestlen + 2 );
		} else   {
			commands[ numcommands++ ] = -( bestlen + 2 );
		}

		for ( int j = 0; j < bestlen + 2; j++ ) {
			// emit a vertex into the reorder buffer
			int k = bestverts[ j ];
			vertexorder[ numorder++ ] = k;

			k = beststverts[ j ];

			// emit s/t coords into the commands stream
			float s = stverts[ k ].s;
			float t = stverts[ k ].t;

			if ( !triangles[ besttris[ 0 ] ].facesfront && stverts[ k ].onseam ) {
				s += pheader->skinwidth / 2;	// on back side
			}
			s = ( s + 0.5 ) / pheader->skinwidth;
			t = ( t + 0.5 ) / pheader->skinheight;

			*( float* )&commands[ numcommands++ ] = s;
			*( float* )&commands[ numcommands++ ] = t;
		}
	}

	commands[ numcommands++ ] = 0;			// end of list marker

	common->DPrintf( "%3i tri %3i vert %3i cmd\n", pheader->numtris, numorder, numcommands );
}

static void GL_MakeAliasModelDisplayLists( model_t* m, mesh1hdr_t* hdr ) {
	aliasmodel = m;
	paliashdr = hdr;	// (mesh1hdr_t *)Mod_Extradata (m);

	//
	// look for a cached version
	//
	char cache[ MAX_QPATH ];
	if ( GGameType & GAME_Hexen2 ) {
		String::Cpy( cache, "glhexen/" );
		String::StripExtension( m->name + String::Length( "models/" ), cache + String::Length( "glhexen/" ) );
	} else   {
		String::Cpy( cache, "glquake/" );
		String::StripExtension( m->name + String::Length( "progs/" ), cache + String::Length( "glquake/" ) );
	}
	String::Cat( cache, sizeof ( cache ), ".ms2" );

	fileHandle_t f;
	FS_FOpenFileRead( cache, &f, true );
	if ( f ) {
		FS_Read( &numcommands, 4, f );
		FS_Read( &numorder, 4, f );
		FS_Read( &commands, numcommands * sizeof ( commands[ 0 ] ), f );
		FS_Read( &vertexorder, numorder * sizeof ( vertexorder[ 0 ] ), f );
		FS_FCloseFile( f );
	} else   {
		//
		// build it from scratch
		//
		common->Printf( "meshing %s...\n",m->name );

		BuildTris();		// trifans or lists

		//
		// save out the cached version
		//
		f = FS_FOpenFileWrite( cache );
		if ( f ) {
			FS_Write( &numcommands, 4, f );
			FS_Write( &numorder, 4, f );
			FS_Write( &commands, numcommands * sizeof ( commands[ 0 ] ), f );
			FS_Write( &vertexorder, numorder * sizeof ( vertexorder[ 0 ] ), f );
			FS_FCloseFile( f );
		}
	}


	// save the data out

	paliashdr->poseverts = numorder;

	int* cmds = new int[ numcommands ];
	paliashdr->commands = cmds;
	Com_Memcpy( cmds, commands, numcommands * 4 );

	dmdl_trivertx_t* verts = new dmdl_trivertx_t[ paliashdr->numposes * paliashdr->poseverts ];
	paliashdr->posedata = verts;
	for ( int i = 0; i < paliashdr->numposes; i++ ) {
		for ( int j = 0; j < numorder; j++ ) {
			*verts++ = poseverts[ i ][ vertexorder[ j ] ];
		}
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
		} else   {
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
	} else   {
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
		} else   {
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
	delete[] pheader->commands;
	delete[] pheader->posedata;
	Mem_Free( pheader );
}

static void GL_DrawAliasFrame( mesh1hdr_t* paliashdr, int posenum, bool fullBrigts, bool overBrights ) {
	lastposenum = posenum;

	dmdl_trivertx_t* verts = paliashdr->posedata;
	verts += posenum * paliashdr->poseverts;
	int* order = paliashdr->commands;

	float r, g, b;
	if ( backEnd.currentEntity->e.renderfx & RF_COLORSHADE ) {
		r = backEnd.currentEntity->e.shaderRGBA[ 0 ] / 255.0;
		g = backEnd.currentEntity->e.shaderRGBA[ 1 ] / 255.0;
		b = backEnd.currentEntity->e.shaderRGBA[ 2 ] / 255.0;
	} else   {
		r = g = b = 1;
	}

	while ( 1 ) {
		// get the vertex count and primitive type
		int count = *order++;
		if ( !count ) {
			break;		// done
		}
		if ( count < 0 ) {
			count = -count;
			qglBegin( GL_TRIANGLE_FAN );
		} else   {
			qglBegin( GL_TRIANGLE_STRIP );
		}

		do {
			// texture coordinates come from the draw list
			tess.svars.texcoords[ 0 ][ 0 ][ 0 ] = ( ( float* )order )[ 0 ];
			tess.svars.texcoords[ 0 ][ 0 ][ 1 ] = ( ( float* )order )[ 1 ];
			order += 2;

			// normals and vertexes come from the frame list
			float l = fullBrigts ? 1 : ambientlight / 256 + ( shadedots[ verts->lightnormalindex ] - 1 ) * shadelight;
			if ( overBrights ) {
				l -= 1;
			}
			qglColor4f( r * l, g * l, b * l, model_constant_alpha );
			tess.xyz[ 0 ][ 0 ] = verts->v[ 0 ];
			tess.xyz[ 0 ][ 1 ] = verts->v[ 1 ];
			tess.xyz[ 0 ][ 2 ] = verts->v[ 2 ];
			R_ArrayElement( 0 );
			verts++;
		} while ( --count );

		qglEnd();
	}
}

static void GL_DrawAliasShadow( mesh1hdr_t* paliashdr, int posenum ) {
	float lheight = backEnd.currentEntity->e.origin[ 2 ] - lightspot[ 2 ];

	float height = 0;
	dmdl_trivertx_t* verts = paliashdr->posedata;
	verts += posenum * paliashdr->poseverts;
	int* order = paliashdr->commands;

	height = -lheight + 1.0;

	while ( 1 ) {
		// get the vertex count and primitive type
		int count = *order++;
		if ( !count ) {
			break;		// done
		}
		if ( count < 0 ) {
			count = -count;
			qglBegin( GL_TRIANGLE_FAN );
		} else   {
			qglBegin( GL_TRIANGLE_STRIP );
		}

		do {
			// texture coordinates come from the draw list
			tess.svars.texcoords[ 0 ][ 0 ][ 0 ] = ( ( float* )order )[ 0 ];
			tess.svars.texcoords[ 0 ][ 0 ][ 1 ] = ( ( float* )order )[ 1 ];
			order += 2;

			// normals and vertexes come from the frame list
			vec3_t point;
			point[ 0 ] = verts->v[ 0 ] * paliashdr->scale[ 0 ] + paliashdr->scale_origin[ 0 ];
			point[ 1 ] = verts->v[ 1 ] * paliashdr->scale[ 1 ] + paliashdr->scale_origin[ 1 ];
			point[ 2 ] = verts->v[ 2 ] * paliashdr->scale[ 2 ] + paliashdr->scale_origin[ 2 ];

			point[ 0 ] -= shadevector[ 0 ] * ( point[ 2 ] + lheight );
			point[ 1 ] -= shadevector[ 1 ] * ( point[ 2 ] + lheight );
			point[ 2 ] = height;
			tess.xyz[ 0 ][ 0 ] = point[ 0 ];
			tess.xyz[ 0 ][ 1 ] = point[ 1 ];
			tess.xyz[ 0 ][ 2 ] = point[ 2 ];
			R_ArrayElement( 0 );

			verts++;
		} while ( --count );

		qglEnd();
	}
}

static void R_SetupAliasFrame( int frame, mesh1hdr_t* paliashdr, bool fullBrigts, bool overBrights ) {
	if ( frame >= paliashdr->numframes || frame < 0 ) {
		common->DPrintf( "R_AliasSetupFrame: no such frame %d\n", frame );
		frame = 0;
	}

	int pose = paliashdr->frames[ frame ].firstpose;
	int numposes = paliashdr->frames[ frame ].numposes;

	if ( numposes > 1 ) {
		float interval = paliashdr->frames[ frame ].interval;
		pose += ( int )( tr.refdef.floatTime / interval ) % numposes;
	}

	GL_DrawAliasFrame( paliashdr, pose, fullBrigts, overBrights );
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
	} else   {
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

void R_AddMdlSurfaces( trRefEntity_t* e, int forcedSortIndex ) {
	if ( ( tr.currentEntity->e.renderfx & RF_THIRD_PERSON ) && !tr.viewParms.isPortal ) {
		return;
	}

	if ( R_CullLocalBox( &tr.currentModel->q1_mins ) == CULL_OUT ) {
		return;
	}
	mesh1hdr_t* paliashdr = ( mesh1hdr_t* )tr.currentModel->q1_cache;
	R_AddDrawSurf( ( surfaceType_t* )paliashdr, tr.defaultShader, 0, false, false, ATI_TESS_NONE, forcedSortIndex );
}

void RB_SurfaceMdl( mesh1hdr_t* paliashdr ) {
	//
	// get lighting information
	//
	ambientlight = shadelight = R_CalcEntityLight( &backEnd.currentEntity->e );

	if ( backEnd.currentEntity->e.renderfx & RF_FIRST_PERSON ) {
		r_lightlevel->value = ambientlight;
	}

	// clamp lighting so it doesn't overbright as much
	if ( ambientlight > 128 ) {
		ambientlight = 128;
	}
	if ( ambientlight + shadelight > 192 ) {
		shadelight = 192 - ambientlight;
	}

	model_t* clmodel = R_GetModelByHandle( backEnd.currentEntity->e.hModel );

	// ZOID: never allow players to go totally black
	if ( ( GGameType & GAME_Quake ) && !String::Cmp( clmodel->name, "progs/player.mdl" ) ) {
		if ( ambientlight < 8 ) {
			ambientlight = shadelight = 8;
		}
	}

	if ( backEnd.currentEntity->e.renderfx & RF_ABSOLUTE_LIGHT ) {
		ambientlight = shadelight = backEnd.currentEntity->e.absoluteLight * 256.0;
	}

	float tmp_yaw = VecToYaw( backEnd.currentEntity->e.axis[ 0 ] );
	shadedots = r_avertexnormal_dots[ ( ( int )( tmp_yaw * ( SHADEDOT_QUANT / 360.0 ) ) ) & ( SHADEDOT_QUANT - 1 ) ];
	shadelight = shadelight / 200.0;

	VectorCopy( backEnd.currentEntity->e.axis[ 0 ], shadevector );
	shadevector[ 2 ] = 1;
	VectorNormalize( shadevector );

	c_alias_polys += paliashdr->numtris;

	//
	// draw all the triangles
	//

	qglPushMatrix();
	qglTranslatef( paliashdr->scale_origin[ 0 ], paliashdr->scale_origin[ 1 ], paliashdr->scale_origin[ 2 ] );
	qglScalef( paliashdr->scale[ 0 ], paliashdr->scale[ 1 ], paliashdr->scale[ 2 ] );

	bool doOverBright = !!r_drawOverBrights->integer;
	GL_Cull( CT_FRONT_SIDED );
	if ( GGameType & GAME_Hexen2 ) {
		if ( clmodel->q1_flags & H2MDLEF_SPECIAL_TRANS ) {
			model_constant_alpha = 1.0f;
			GL_Cull( CT_TWO_SIDED );
			GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA | GLS_DSTBLEND_SRC_ALPHA );
			doOverBright = false;
		} else if ( backEnd.currentEntity->e.renderfx & RF_WATERTRANS ) {
			model_constant_alpha = r_wateralpha->value;
			GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
			doOverBright = false;
		} else if ( clmodel->q1_flags & H2MDLEF_TRANSPARENT ) {
			model_constant_alpha = 1.0f;
			GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
			doOverBright = false;
		} else if ( clmodel->q1_flags & H2MDLEF_HOLEY ) {
			model_constant_alpha = 1.0f;
			GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
			doOverBright = false;
		} else {
			model_constant_alpha = 1.0f;
			GL_State( GLS_DEPTHMASK_TRUE );
		}
	} else   {
		model_constant_alpha = 1.0f;
		GL_State( GLS_DEFAULT );
	}

	int anim = ( int )( backEnd.refdef.floatTime * 10 ) & 3;
	if ( backEnd.currentEntity->e.customSkin ) {
		GL_Bind( tr.images[ backEnd.currentEntity->e.customSkin ] );
	} else   {
		GL_Bind( paliashdr->gl_texture[ backEnd.currentEntity->e.skinNum ][ anim ] );
	}

	R_SetupAliasFrame( backEnd.currentEntity->e.frame, paliashdr, false, false );

	if ( doOverBright ) {
		GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
		R_SetupAliasFrame( backEnd.currentEntity->e.frame, paliashdr, false, true );
	}

	if ( !backEnd.currentEntity->e.customSkin && paliashdr->fullBrightTexture[ backEnd.currentEntity->e.skinNum ][ anim ] ) {
		GL_Bind( paliashdr->fullBrightTexture[ backEnd.currentEntity->e.skinNum ][ anim ] );
		GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
		R_SetupAliasFrame( backEnd.currentEntity->e.frame, paliashdr, true, false );
	}

	if ( ( GGameType & GAME_Hexen2 ) && ( clmodel->q1_flags & H2MDLEF_SPECIAL_TRANS ) ) {
		GL_Cull( CT_FRONT_SIDED );
	}

	qglPopMatrix();

	if ( r_shadows->value ) {
		qglPushMatrix();
		qglDisable( GL_TEXTURE_2D );
		GL_State( GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
		qglColor4f( 0, 0, 0, 0.5 );
		GL_DrawAliasShadow( paliashdr, lastposenum );
		qglEnable( GL_TEXTURE_2D );
		qglColor4f( 1,1,1,1 );
		qglPopMatrix();
	}
}

bool R_MdlHasHexen2Transparency( model_t* Model ) {
	return !!( Model->q1_flags & ( H2MDLEF_TRANSPARENT | H2MDLEF_HOLEY | H2MDLEF_SPECIAL_TRANS ) );
}
