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
#include "../../../common/endian.h"
#include "../../../common/math/Math.h"

#define POWERSUIT_SCALE         4.0F

static int md2_shadelight[ 3 ];

void Mod_LoadMd2Model( model_t* mod, const void* buffer ) {
	const dmd2_t* pinmodel = ( const dmd2_t* )buffer;

	int version = LittleLong( pinmodel->version );
	if ( version != MESH2_VERSION ) {
		common->Error( "%s has wrong version number (%i should be %i)",
			mod->name, version, MESH2_VERSION );
	}

	dmd2_t* pheader = ( dmd2_t* )Mem_Alloc( LittleLong( pinmodel->ofs_end ) );

	// byte swap the header fields and sanity check
	for ( int i = 0; i < ( int )sizeof ( dmd2_t ) / 4; i++ ) {
		( ( int* )pheader )[ i ] = LittleLong( ( ( int* )buffer )[ i ] );
	}
	pheader->ident = SF_MD2;

	if ( pheader->num_xyz <= 0 ) {
		common->Error( "model %s has no vertices", mod->name );
	}

	if ( pheader->num_xyz > MAX_MD2_VERTS ) {
		common->Error( "model %s has too many vertices", mod->name );
	}

	if ( pheader->num_st <= 0 ) {
		common->Error( "model %s has no st vertices", mod->name );
	}

	if ( pheader->num_tris <= 0 ) {
		common->Error( "model %s has no triangles", mod->name );
	}

	if ( pheader->num_frames <= 0 ) {
		common->Error( "model %s has no frames", mod->name );
	}

	//
	// load base s and t vertices (not used in gl version)
	//
	const dmd2_stvert_t* pinst = ( const dmd2_stvert_t* )( ( byte* )pinmodel + pheader->ofs_st );
	dmd2_stvert_t* poutst = ( dmd2_stvert_t* )( ( byte* )pheader + pheader->ofs_st );

	for ( int i = 0; i < pheader->num_st; i++ ) {
		poutst[ i ].s = LittleShort( pinst[ i ].s );
		poutst[ i ].t = LittleShort( pinst[ i ].t );
	}

	//
	// load triangle lists
	//
	const dmd2_triangle_t* pintri = ( const dmd2_triangle_t* )( ( byte* )pinmodel + pheader->ofs_tris );
	dmd2_triangle_t* pouttri = ( dmd2_triangle_t* )( ( byte* )pheader + pheader->ofs_tris );

	for ( int i = 0; i < pheader->num_tris; i++ ) {
		for ( int j = 0; j < 3; j++ ) {
			pouttri[ i ].index_xyz[ j ] = LittleShort( pintri[ i ].index_xyz[ j ] );
			pouttri[ i ].index_st[ j ] = LittleShort( pintri[ i ].index_st[ j ] );
		}
	}

	//
	// load the frames
	//
	for ( int i = 0; i < pheader->num_frames; i++ ) {
		const dmd2_frame_t* pinframe = ( const dmd2_frame_t* )( ( byte* )pinmodel +
																pheader->ofs_frames + i * pheader->framesize );
		dmd2_frame_t* poutframe = ( dmd2_frame_t* )( ( byte* )pheader +
													 pheader->ofs_frames + i * pheader->framesize );

		Com_Memcpy( poutframe->name, pinframe->name, sizeof ( poutframe->name ) );
		for ( int j = 0; j < 3; j++ ) {
			poutframe->scale[ j ] = LittleFloat( pinframe->scale[ j ] );
			poutframe->translate[ j ] = LittleFloat( pinframe->translate[ j ] );
		}
		// verts are all 8 bit, so no swapping needed
		Com_Memcpy( poutframe->verts, pinframe->verts, pheader->num_xyz * sizeof ( dmd2_trivertx_t ) );
	}

	mod->type = MOD_MESH2;
	mod->q2_extradata = pheader;
	mod->q2_extradatasize = pheader->ofs_end;
	mod->q2_numframes = pheader->num_frames;

	//
	// load the glcmds
	//
	const int* pincmd = ( const int* )( ( byte* )pinmodel + pheader->ofs_glcmds );
	int* poutcmd = ( int* )( ( byte* )pheader + pheader->ofs_glcmds );
	for ( int i = 0; i < pheader->num_glcmds; i++ ) {
		poutcmd[ i ] = LittleLong( pincmd[ i ] );
	}

	// register all skins
	Com_Memcpy( ( char* )pheader + pheader->ofs_skins, ( char* )pinmodel + pheader->ofs_skins,
		pheader->num_skins * MAX_MD2_SKINNAME );
	for ( int i = 0; i < pheader->num_skins; i++ ) {
		mod->q2_skins[ i ] = R_FindImageFile( ( char* )pheader + pheader->ofs_skins + i * MAX_MD2_SKINNAME,
			true, true, GL_CLAMP, false, IMG8MODE_Skin );
	}

	mod->q2_mins[ 0 ] = -32;
	mod->q2_mins[ 1 ] = -32;
	mod->q2_mins[ 2 ] = -32;
	mod->q2_maxs[ 0 ] = 32;
	mod->q2_maxs[ 1 ] = 32;
	mod->q2_maxs[ 2 ] = 32;
}

void Mod_FreeMd2Model( model_t* mod ) {
	Mem_Free( mod->q2_extradata );
}

static void GL_LerpVerts( int nverts, dmd2_trivertx_t* v, dmd2_trivertx_t* ov, dmd2_trivertx_t* verts,
	float* lerp, float move[ 3 ], float frontv[ 3 ], float backv[ 3 ] ) {
	if ( backEnd.currentEntity->e.renderfx & RF_COLOUR_SHELL ) {
		for ( int i = 0; i < nverts; i++, v++, ov++, lerp += 4 ) {
			float* normal = bytedirs[ verts[ i ].lightnormalindex ];

			lerp[ 0 ] = move[ 0 ] + ov->v[ 0 ] * backv[ 0 ] + v->v[ 0 ] * frontv[ 0 ] + normal[ 0 ] * POWERSUIT_SCALE;
			lerp[ 1 ] = move[ 1 ] + ov->v[ 1 ] * backv[ 1 ] + v->v[ 1 ] * frontv[ 1 ] + normal[ 1 ] * POWERSUIT_SCALE;
			lerp[ 2 ] = move[ 2 ] + ov->v[ 2 ] * backv[ 2 ] + v->v[ 2 ] * frontv[ 2 ] + normal[ 2 ] * POWERSUIT_SCALE;
		}
	} else   {
		for ( int i = 0; i < nverts; i++, v++, ov++, lerp += 4 ) {
			lerp[ 0 ] = move[ 0 ] + ov->v[ 0 ] * backv[ 0 ] + v->v[ 0 ] * frontv[ 0 ];
			lerp[ 1 ] = move[ 1 ] + ov->v[ 1 ] * backv[ 1 ] + v->v[ 1 ] * frontv[ 1 ];
			lerp[ 2 ] = move[ 2 ] + ov->v[ 2 ] * backv[ 2 ] + v->v[ 2 ] * frontv[ 2 ];
		}
	}
}

//	interpolates between two frames and origins
//	FIXME: batch lerp all vertexes
static void GL_DrawMd2FrameLerp( dmd2_t* paliashdr, float backlerp ) {
	dmd2_frame_t* frame = ( dmd2_frame_t* )( ( byte* )paliashdr + paliashdr->ofs_frames +
											 backEnd.currentEntity->e.frame * paliashdr->framesize );
	dmd2_trivertx_t* verts = frame->verts;
	dmd2_trivertx_t* v = verts;

	dmd2_frame_t* oldframe = ( dmd2_frame_t* )( ( byte* )paliashdr + paliashdr->ofs_frames +
												backEnd.currentEntity->e.oldframe * paliashdr->framesize );
	dmd2_trivertx_t* ov = oldframe->verts;

	int* order = ( int* )( ( byte* )paliashdr + paliashdr->ofs_glcmds );

	int alpha;
	if ( backEnd.currentEntity->e.renderfx & RF_TRANSLUCENT ) {
		alpha = backEnd.currentEntity->e.shaderRGBA[ 3 ];
	} else   {
		alpha = 255;
	}

	if ( backEnd.currentEntity->e.renderfx & RF_COLOUR_SHELL ) {
		qglDisable( GL_TEXTURE_2D );
	}

	float frontlerp = 1.0 - backlerp;

	// move should be the delta back to the previous frame * backlerp
	vec3_t delta;
	VectorSubtract( backEnd.currentEntity->e.oldorigin, backEnd.currentEntity->e.origin, delta );

	vec3_t move;
	move[ 0 ] = DotProduct( delta, backEnd.currentEntity->e.axis[ 0 ] );		// forward
	move[ 1 ] = DotProduct( delta, backEnd.currentEntity->e.axis[ 1 ] );		// left
	move[ 2 ] = DotProduct( delta, backEnd.currentEntity->e.axis[ 2 ] );		// up

	VectorAdd( move, oldframe->translate, move );

	for ( int i = 0; i < 3; i++ ) {
		move[ i ] = backlerp * move[ i ] + frontlerp * frame->translate[ i ];
	}

	vec3_t frontv, backv;
	for ( int i = 0; i < 3; i++ ) {
		frontv[ i ] = frontlerp * frame->scale[ i ];
		backv[ i ] = backlerp * oldframe->scale[ i ];
	}

	GL_LerpVerts( paliashdr->num_xyz, v, ov, verts, tess.xyz[ 0 ], move, frontv, backv );

	if ( r_vertex_arrays->value ) {
		float colorArray[ MAX_MD2_VERTS * 4 ];

		qglVertexPointer( 3, GL_FLOAT, 16, tess.xyz );		// padded for SIMD

		if ( backEnd.currentEntity->e.renderfx & RF_COLOUR_SHELL ) {
			qglColor4ub( md2_shadelight[ 0 ], md2_shadelight[ 1 ], md2_shadelight[ 2 ], alpha );
		} else   {
			qglEnableClientState( GL_COLOR_ARRAY );
			qglColorPointer( 3, GL_FLOAT, 0, colorArray );

			//
			// pre light everything
			//
			for ( int i = 0; i < paliashdr->num_xyz; i++ ) {
				float l = shadedots[ verts[ i ].lightnormalindex ];

				colorArray[ i * 3 + 0 ] = l * md2_shadelight[ 0 ] / 255.0;
				colorArray[ i * 3 + 1 ] = l * md2_shadelight[ 1 ] / 255.0;
				colorArray[ i * 3 + 2 ] = l * md2_shadelight[ 2 ] / 255.0;
			}
		}

		if ( qglLockArraysEXT ) {
			qglLockArraysEXT( 0, paliashdr->num_xyz );
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
				int index_xyz = order[ 2 ];
				if ( !( backEnd.currentEntity->e.renderfx & RF_COLOUR_SHELL ) ) {
					// texture coordinates come from the draw list
					qglTexCoord2f( ( ( float* )order )[ 0 ], ( ( float* )order )[ 1 ] );
				}
				order += 3;

				// normals and vertexes come from the frame list
				qglArrayElement( index_xyz );
			} while ( --count );
			qglEnd();
		}
	} else   {
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
				int index_xyz = order[ 2 ];
				// texture coordinates come from the draw list
				tess.svars.texcoords[ 0 ][ index_xyz ][ 0 ] = ( ( float* )order )[ 0 ];
				tess.svars.texcoords[ 0 ][ index_xyz ][ 1 ] = ( ( float* )order )[ 1 ];
				if ( backEnd.currentEntity->e.renderfx & RF_COLOUR_SHELL ) {
					tess.svars.colors[ index_xyz ][ 0 ] = md2_shadelight[ 0 ];
					tess.svars.colors[ index_xyz ][ 1 ] = md2_shadelight[ 1 ];
					tess.svars.colors[ index_xyz ][ 2 ] = md2_shadelight[ 2 ];
					tess.svars.colors[ index_xyz ][ 3 ] = alpha;
				} else {
					// normals and vertexes come from the frame list
					float l = shadedots[ verts[ index_xyz ].lightnormalindex ];

					tess.svars.colors[ index_xyz ][ 0 ] = Min(l * md2_shadelight[ 0 ], 255.0f);
					tess.svars.colors[ index_xyz ][ 1 ] = Min(l * md2_shadelight[ 1 ], 255.0f);
					tess.svars.colors[ index_xyz ][ 2 ] = Min(l * md2_shadelight[ 2 ], 255.0f);
					tess.svars.colors[ index_xyz ][ 3 ] = alpha;
				}
				R_ArrayElementDiscrete( index_xyz );
				order += 3;
			} while ( --count );
			qglEnd();
		}
	}

	if ( r_vertex_arrays->value && qglUnlockArraysEXT ) {
		qglUnlockArraysEXT();
	}

	if ( backEnd.currentEntity->e.renderfx & RF_COLOUR_SHELL ) {
		qglEnable( GL_TEXTURE_2D );
	}
}

static void GL_DrawMd2Shadow( dmd2_t* paliashdr, int posenum ) {
	float lheight = backEnd.currentEntity->e.origin[ 2 ] - lightspot[ 2 ];

	int* order = ( int* )( ( byte* )paliashdr + paliashdr->ofs_glcmds );

	float height = -lheight + 1.0;

	for ( int i = 0; i < paliashdr->num_xyz; i++ ) {
		tess.svars.colors[ i ][ 0 ] = 0;
		tess.svars.colors[ i ][ 1 ] = 0;
		tess.svars.colors[ i ][ 2 ] = 0;
		tess.svars.colors[ i ][ 3 ] = 127;
		vec3_t point;
		Com_Memcpy( point, tess.xyz[ i ], sizeof ( point ) );

		point[ 0 ] -= shadevector[ 0 ] * ( point[ 2 ] + lheight );
		point[ 1 ] -= shadevector[ 1 ] * ( point[ 2 ] + lheight );
		point[ 2 ] = height;
		tess.xyz[ i ][ 0 ] = point[ 0 ];
		tess.xyz[ i ][ 1 ] = point[ 1 ];
		tess.xyz[ i ][ 2 ] = point[ 2 ];
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
			tess.svars.texcoords[ 0 ][ order[ 2 ] ][ 0 ] = ( ( float* )order )[ 0 ];
			tess.svars.texcoords[ 0 ][ order[ 2 ] ][ 1 ] = ( ( float* )order )[ 1 ];
			R_ArrayElementDiscrete( order[ 2 ] );

			order += 3;
		} while ( --count );

		qglEnd();
	}
}

static bool R_CullMd2Model( trRefEntity_t* e ) {
	dmd2_t* paliashdr = ( dmd2_t* )tr.currentModel->q2_extradata;

	if ( ( e->e.frame >= paliashdr->num_frames ) || ( e->e.frame < 0 ) ) {
		common->Printf( "R_CullMd2Model %s: no such frame %d\n",
			tr.currentModel->name, e->e.frame );
		e->e.frame = 0;
	}
	if ( ( e->e.oldframe >= paliashdr->num_frames ) || ( e->e.oldframe < 0 ) ) {
		common->Printf( "R_CullMd2Model %s: no such oldframe %d\n",
			tr.currentModel->name, e->e.oldframe );
		e->e.oldframe = 0;
	}

	dmd2_frame_t* pframe = ( dmd2_frame_t* )( ( byte* )paliashdr +
											  paliashdr->ofs_frames + e->e.frame * paliashdr->framesize );

	dmd2_frame_t* poldframe = ( dmd2_frame_t* )( ( byte* )paliashdr +
												 paliashdr->ofs_frames + e->e.oldframe * paliashdr->framesize );

	/*
	** compute axially aligned mins and maxs
	*/
	vec3_t bounds[ 2 ];
	if ( pframe == poldframe ) {
		for ( int i = 0; i < 3; i++ ) {
			bounds[ 0 ][ i ] = pframe->translate[ i ];
			bounds[ 1 ][ i ] = bounds[ 0 ][ i ] + pframe->scale[ i ] * 255;
		}
	} else   {
		for ( int i = 0; i < 3; i++ ) {
			vec3_t thismins, thismaxs;
			thismins[ i ] = pframe->translate[ i ];
			thismaxs[ i ] = thismins[ i ] + pframe->scale[ i ] * 255;

			vec3_t oldmins, oldmaxs;
			oldmins[ i ]  = poldframe->translate[ i ];
			oldmaxs[ i ]  = oldmins[ i ] + poldframe->scale[ i ] * 255;

			if ( thismins[ i ] < oldmins[ i ] ) {
				bounds[ 0 ][ i ] = thismins[ i ];
			} else   {
				bounds[ 0 ][ i ] = oldmins[ i ];
			}

			if ( thismaxs[ i ] > oldmaxs[ i ] ) {
				bounds[ 1 ][ i ] = thismaxs[ i ];
			} else   {
				bounds[ 1 ][ i ] = oldmaxs[ i ];
			}
		}
	}

	return R_CullLocalBox( bounds ) == CULL_OUT;
}

void R_AddMd2Surfaces( trRefEntity_t* e, int forcedSortIndex ) {
	if ( ( tr.currentEntity->e.renderfx & RF_THIRD_PERSON ) && !tr.viewParms.isPortal ) {
		return;
	}

	if ( !( e->e.renderfx & RF_FIRST_PERSON ) ) {
		if ( R_CullMd2Model( e ) ) {
			return;
		}
	}

	dmd2_t* paliashdr = ( dmd2_t* )tr.currentModel->q2_extradata;
	R_AddDrawSurf( ( surfaceType_t* )paliashdr, tr.defaultShader, 0, false, false, ATI_TESS_NONE, forcedSortIndex );
}

void RB_SurfaceMd2( dmd2_t* paliashdr ) {

	//
	// get lighting information
	//
	if ( backEnd.currentEntity->e.renderfx & RF_COLOUR_SHELL ) {
		for ( int i = 0; i < 3; i++ ) {
			md2_shadelight[ i ] = backEnd.currentEntity->e.shaderRGBA[ i ];
		}
	} else if ( backEnd.currentEntity->e.renderfx & RF_ABSOLUTE_LIGHT )     {
		for ( int i = 0; i < 3; i++ ) {
			md2_shadelight[ i ] = backEnd.currentEntity->e.absoluteLight * 255;
		}
	} else {
		float l[4];
		R_LightPointQ2( backEnd.currentEntity->e.origin, l, backEnd.refdef );
		md2_shadelight[0] = Min(l[0] * 255, 255.0f);
		md2_shadelight[1] = Min(l[1] * 255, 255.0f);
		md2_shadelight[2] = Min(l[2] * 255, 255.0f);
		md2_shadelight[3] = Min(l[3] * 255, 255.0f);

		// player lighting hack for communication back to server
		// big hack!
		if ( backEnd.currentEntity->e.renderfx & RF_FIRST_PERSON ) {
			// pick the greatest component, which should be the same
			// as the mono value returned by software
			if ( md2_shadelight[ 0 ] > md2_shadelight[ 1 ] ) {
				if ( md2_shadelight[ 0 ] > md2_shadelight[ 2 ] ) {
					r_lightlevel->value = 150 * md2_shadelight[ 0 ] / 255;
				} else   {
					r_lightlevel->value = 150 * md2_shadelight[ 2 ] / 255;
				}
			} else   {
				if ( md2_shadelight[ 1 ] > md2_shadelight[ 2 ] ) {
					r_lightlevel->value = 150 * md2_shadelight[ 1 ] / 255;
				} else   {
					r_lightlevel->value = 150 * md2_shadelight[ 2 ] / 255;
				}
			}
		}
	}

	if ( backEnd.currentEntity->e.renderfx & RF_MINLIGHT ) {
		int i;
		for ( i = 0; i < 3; i++ ) {
			if ( md2_shadelight[ i ] > 25 ) {
				break;
			}
		}
		if ( i == 3 ) {
			md2_shadelight[ 0 ] = 25;
			md2_shadelight[ 1 ] = 25;
			md2_shadelight[ 2 ] = 25;
		}
	}

	if ( backEnd.currentEntity->e.renderfx & RF_GLOW ) {
		// bonus items will pulse with time
		int scale = 25 * sin( backEnd.refdef.floatTime * 7 );
		for ( int i = 0; i < 3; i++ ) {
			int min = md2_shadelight[ i ] * 0.8;
			md2_shadelight[ i ] = Min(md2_shadelight[ i ] + scale, 255);
			if ( md2_shadelight[ i ] < min ) {
				md2_shadelight[ i ] = min;
			}
		}
	}

	// =================
	// PGM	ir goggles color override
	if ( backEnd.refdef.rdflags & RDF_IRGOGGLES && backEnd.currentEntity->e.renderfx & RF_IR_VISIBLE ) {
		md2_shadelight[ 0 ] = 255;
		md2_shadelight[ 1 ] = 0;
		md2_shadelight[ 2 ] = 0;
	}
	// PGM
	// =================

	float tmp_yaw = VecToYaw( backEnd.currentEntity->e.axis[ 0 ] );
	shadedots = r_avertexnormal_dots[ ( ( int )( tmp_yaw * ( SHADEDOT_QUANT / 360.0 ) ) ) & ( SHADEDOT_QUANT - 1 ) ];

	VectorCopy( backEnd.currentEntity->e.axis[ 0 ], shadevector );
	shadevector[ 2 ] = 1;
	VectorNormalize( shadevector );

	//
	// locate the proper data
	//

	c_alias_polys += paliashdr->num_tris;

	//
	// draw all the triangles
	//
	if ( backEnd.currentEntity->e.renderfx & RF_LEFTHAND ) {
		qglMatrixMode( GL_PROJECTION );
		qglPushMatrix();
		qglLoadIdentity();
		qglScalef( -1, 1, 1 );
		qglMultMatrixf( backEnd.viewParms.projectionMatrix );
		qglMatrixMode( GL_MODELVIEW );

		GL_Cull( CT_BACK_SIDED );
	}
	else
		GL_Cull( CT_FRONT_SIDED );

	qglPushMatrix();

	// select skin
	image_t* skin;
	if ( backEnd.currentEntity->e.customSkin ) {
		skin = tr.images[ backEnd.currentEntity->e.customSkin ];		// custom player skin
	} else   {
		if ( backEnd.currentEntity->e.skinNum >= MAX_MD2_SKINS ) {
			skin = R_GetModelByHandle( backEnd.currentEntity->e.hModel )->q2_skins[ 0 ];
		} else   {
			skin = R_GetModelByHandle( backEnd.currentEntity->e.hModel )->q2_skins[ backEnd.currentEntity->e.skinNum ];
			if ( !skin ) {
				skin = R_GetModelByHandle( backEnd.currentEntity->e.hModel )->q2_skins[ 0 ];
			}
		}
	}
	if ( !skin ) {
		skin = tr.defaultImage;	// fallback...
	}
	GL_Bind( skin );

	// draw it

	if ( backEnd.currentEntity->e.renderfx & RF_TRANSLUCENT ) {
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	} else   {
		GL_State( GLS_DEFAULT );
	}


	if ( ( backEnd.currentEntity->e.frame >= paliashdr->num_frames ) || ( backEnd.currentEntity->e.frame < 0 ) ) {
		common->Printf( "R_AddMd2Surfaces %s: no such frame %d\n",
			R_GetModelByHandle( backEnd.currentEntity->e.hModel )->name, backEnd.currentEntity->e.frame );
		backEnd.currentEntity->e.frame = 0;
		backEnd.currentEntity->e.oldframe = 0;
	}

	if ( ( backEnd.currentEntity->e.oldframe >= paliashdr->num_frames ) || ( backEnd.currentEntity->e.oldframe < 0 ) ) {
		common->Printf( "R_AddMd2Surfaces %s: no such oldframe %d\n",
			R_GetModelByHandle( backEnd.currentEntity->e.hModel )->name, backEnd.currentEntity->e.oldframe );
		backEnd.currentEntity->e.frame = 0;
		backEnd.currentEntity->e.oldframe = 0;
	}

	if ( !r_lerpmodels->value ) {
		backEnd.currentEntity->e.backlerp = 0;
	}
	GL_DrawMd2FrameLerp( paliashdr, backEnd.currentEntity->e.backlerp );

	qglPopMatrix();

	if ( backEnd.currentEntity->e.renderfx & RF_LEFTHAND ) {
		qglMatrixMode( GL_PROJECTION );
		qglPopMatrix();
		qglMatrixMode( GL_MODELVIEW );
	}

	if ( r_shadows->value && !( backEnd.currentEntity->e.renderfx & ( RF_TRANSLUCENT | RF_FIRST_PERSON ) ) ) {
		GL_Cull( CT_FRONT_SIDED );
		qglPushMatrix();
		qglLoadMatrixf( backEnd.orient.modelMatrix );
		qglDisable( GL_TEXTURE_2D );
		GL_State( GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
		GL_DrawMd2Shadow( paliashdr, backEnd.currentEntity->e.frame );
		qglEnable( GL_TEXTURE_2D );
		qglPopMatrix();
	}
}
