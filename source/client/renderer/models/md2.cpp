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

struct idMd2VertexRemap {
	int xyzIndex;
	float s;
	float t;
};

static int AddToVertexMap( idList< idMd2VertexRemap >& vertexMap, int xyzIndex, float s, float t ) {
	idMd2VertexRemap& v = vertexMap.Alloc();
	v.xyzIndex = xyzIndex;
	v.s = s;
	v.t = t;
	return vertexMap.Num() - 1;
}

static void ExtractMd2Triangles( int* order, idList<glIndex_t>& indexes, idList<idMd2VertexRemap>& vertexMap ) {
	while ( 1 ) {
		// get the vertex count and primitive type
		int count = *order++;
		if ( !count ) {
			break;		// done
		}
		bool isFan = count < 0;
		if ( isFan ) {
			count = -count;
		}

		int i = 0;
		int triangle[3] = { -1, -1, -1 };
		do {
			// texture coordinates come from the draw list
			int index = AddToVertexMap( vertexMap, order[ 2 ], ( ( float* )order )[ 0 ], ( ( float* )order )[ 1 ] );
			order += 3;
			if ( i < 3 ) {
				triangle[ i ] = index;
			} else if ( isFan ) {
				triangle[ 1 ] = triangle[ 2 ];
				triangle[ 2 ] = index;
			} else if ( i & 1 ) {
				triangle[ 0 ] = triangle[ 1 ];
				triangle[ 1 ] = index;
				triangle[ 2 ] = triangle[ 2 ];
			} else {
				triangle[ 0 ] = triangle[ 2 ];
				triangle[ 2 ] = index;
			}
			i++;
			if ( i >= 3 ) {
				indexes.Append( triangle[ 0 ] );
				indexes.Append( triangle[ 1 ] );
				indexes.Append( triangle[ 2 ] );
			}
		} while ( --count );
	}
}

void Mod_LoadMd2Model( model_t* mod, const void* buffer ) {
	// byte swap the header fields and sanity check
	dmd2_t pinmodel;
	for ( int i = 0; i < ( int )sizeof ( dmd2_t ) / 4; i++ ) {
		( ( int* )&pinmodel )[ i ] = LittleLong( ( ( int* )buffer )[ i ] );
	}

	if ( pinmodel.version != MESH2_VERSION ) {
		common->Error( "%s has wrong version number (%i should be %i)",
			mod->name, pinmodel.version, MESH2_VERSION );
	}

	if ( pinmodel.num_xyz <= 0 ) {
		common->Error( "model %s has no vertices", mod->name );
	}

	if ( pinmodel.num_xyz > MAX_MD2_VERTS ) {
		common->Error( "model %s has too many vertices", mod->name );
	}

	if ( pinmodel.num_st <= 0 ) {
		common->Error( "model %s has no st vertices", mod->name );
	}

	if ( pinmodel.num_tris <= 0 ) {
		common->Error( "model %s has no triangles", mod->name );
	}

	if ( pinmodel.num_frames <= 0 ) {
		common->Error( "model %s has no frames", mod->name );
	}

	//
	// load the glcmds
	//
	const int* pincmd = ( const int* )( ( const byte* )buffer + pinmodel.ofs_glcmds );
	idList<int> glcmds;
	glcmds.SetNum( pinmodel.num_glcmds );
	for ( int i = 0; i < pinmodel.num_glcmds; i++ ) {
		glcmds[ i ] = LittleLong( pincmd[ i ] );
	}

	idList<glIndex_t> indexes;
	idList<idMd2VertexRemap> vertexMap;
	ExtractMd2Triangles( glcmds.Ptr(), indexes, vertexMap );

	int frameSize = sizeof( dmd2_frame_t ) + ( vertexMap.Num() - 1 ) * sizeof( dmd2_trivertx_t );

	mod->type = MOD_MESH2;
	mod->q2_extradatasize = sizeof( mmd2_t ) +
		pinmodel.num_frames * frameSize +
		sizeof( idVec2 ) * vertexMap.Num() +
		sizeof( glIndex_t ) * indexes.Num();
	mod->q2_md2 = ( mmd2_t* )Mem_Alloc( mod->q2_extradatasize );
	mod->q2_numframes = pinmodel.num_frames;

	mmd2_t* pheader = mod->q2_md2;

	pheader->surfaceType = SF_MD2;
	pheader->framesize = frameSize;
	pheader->num_skins = pinmodel.num_skins;
	pheader->num_frames = pinmodel.num_frames;
	pheader->numVertexes = vertexMap.Num();
	pheader->numIndexes = indexes.Num();

	//
	// load the frames
	//
	pheader->frames = ( byte* )pheader + sizeof( mmd2_t );
	for ( int i = 0; i < pheader->num_frames; i++ ) {
		const dmd2_frame_t* pinframe = ( const dmd2_frame_t* )( ( const byte* )buffer +
			pinmodel.ofs_frames + i * pinmodel.framesize );
		dmd2_frame_t* poutframe = ( dmd2_frame_t* )( pheader->frames + i * pheader->framesize );

		Com_Memcpy( poutframe->name, pinframe->name, sizeof ( poutframe->name ) );
		for ( int j = 0; j < 3; j++ ) {
			poutframe->scale[ j ] = LittleFloat( pinframe->scale[ j ] );
			poutframe->translate[ j ] = LittleFloat( pinframe->translate[ j ] );
		}
		for ( int j = 0; j < vertexMap.Num(); j++ ) {
			Com_Memcpy( &poutframe->verts[ j ], &pinframe->verts[ vertexMap[ j ].xyzIndex ], sizeof ( dmd2_trivertx_t ) );
		}
	}

	//	Copy texture coordinates
	pheader->texCoords = ( idVec2* )( pheader->frames + pheader->num_frames * pheader->framesize );
	for ( int i = 0; i < vertexMap.Num(); i++ ) {
		pheader->texCoords[ i ].x = vertexMap[ i ].s;
		pheader->texCoords[ i ].y = vertexMap[ i ].t;
	}

	//	Copy indexes
	pheader->indexes = ( glIndex_t* )( ( byte* )pheader->texCoords + sizeof( idVec2 ) * vertexMap.Num() );
	Com_Memcpy( pheader->indexes, indexes.Ptr(), pheader->numIndexes * sizeof( glIndex_t ) );

	// register all skins
	for ( int i = 0; i < pheader->num_skins; i++ ) {
		mod->q2_skins[ i ] = R_FindImageFile( ( const char* )buffer + pinmodel.ofs_skins + i * MAX_MD2_SKINNAME,
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
	Mem_Free( mod->q2_md2 );
}

static void GL_LerpVerts( mmd2_t* paliashdr, dmd2_trivertx_t* v, dmd2_trivertx_t* ov,
	float* lerp, float* normals, float move[ 3 ], float frontv[ 3 ], float backv[ 3 ] ) {
	for ( int i = 0; i < paliashdr->numVertexes; i++, lerp += 4, normals += 4 ) {
		lerp[ 0 ] = move[ 0 ] + ov[ i ].v[ 0 ] * backv[ 0 ] + v[ i ].v[ 0 ] * frontv[ 0 ];
		lerp[ 1 ] = move[ 1 ] + ov[ i ].v[ 1 ] * backv[ 1 ] + v[ i ].v[ 1 ] * frontv[ 1 ];
		lerp[ 2 ] = move[ 2 ] + ov[ i ].v[ 2 ] * backv[ 2 ] + v[ i ].v[ 2 ] * frontv[ 2 ];
		const float* normal = bytedirs[ v[ i ].lightnormalindex ];
		VectorCopy( normal, normals );
		if ( backEnd.currentEntity->e.renderfx & RF_COLOUR_SHELL ) {
			lerp[ 0 ] += normal[ 0 ] * POWERSUIT_SCALE;
			lerp[ 1 ] += normal[ 1 ] * POWERSUIT_SCALE;
			lerp[ 2 ] += normal[ 2 ] * POWERSUIT_SCALE;
		}
		tess.svars.texcoords[ 0 ][ i ][ 0 ] = paliashdr->texCoords[ i ].x;
		tess.svars.texcoords[ 0 ][ i ][ 1 ] = paliashdr->texCoords[ i ].y;
	}
}

//	interpolates between two frames and origins
//	FIXME: batch lerp all vertexes
static void GL_DrawMd2FrameLerp( mmd2_t* paliashdr, float backlerp ) {
	dmd2_frame_t* frame = ( dmd2_frame_t* )( paliashdr->frames +
											 backEnd.currentEntity->e.frame * paliashdr->framesize );
	dmd2_trivertx_t* v = frame->verts;

	dmd2_frame_t* oldframe = ( dmd2_frame_t* )( paliashdr->frames +
												backEnd.currentEntity->e.oldframe * paliashdr->framesize );
	dmd2_trivertx_t* ov = oldframe->verts;

	int alpha;
	if ( backEnd.currentEntity->e.renderfx & RF_TRANSLUCENT ) {
		alpha = backEnd.currentEntity->e.shaderRGBA[ 3 ];
	} else {
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

	GL_LerpVerts( paliashdr, v, ov, tess.xyz[ 0 ], tess.normal[ 0 ], move, frontv, backv );

	for ( int i = 0; i < paliashdr->numVertexes; i++ ) {
		if ( backEnd.currentEntity->e.renderfx & RF_COLOUR_SHELL ) {
			tess.svars.colors[ i ][ 0 ] = md2_shadelight[ 0 ];
			tess.svars.colors[ i ][ 1 ] = md2_shadelight[ 1 ];
			tess.svars.colors[ i ][ 2 ] = md2_shadelight[ 2 ];
			tess.svars.colors[ i ][ 3 ] = alpha;
		} else {
			float l = shadedots[ v[ i ].lightnormalindex ];

			tess.svars.colors[ i ][ 0 ] = Min(l * md2_shadelight[ 0 ], 255.0f);
			tess.svars.colors[ i ][ 1 ] = Min(l * md2_shadelight[ 1 ], 255.0f);
			tess.svars.colors[ i ][ 2 ] = Min(l * md2_shadelight[ 2 ], 255.0f);
			tess.svars.colors[ i ][ 3 ] = alpha;
		}
	}

	EnableArrays( paliashdr->numVertexes );
	R_DrawElements( paliashdr->numIndexes, paliashdr->indexes );
	DisableArrays();

	if ( backEnd.currentEntity->e.renderfx & RF_COLOUR_SHELL ) {
		qglEnable( GL_TEXTURE_2D );
	}
}

static void GL_DrawMd2Shadow( mmd2_t* paliashdr, int posenum ) {
	float lheight = backEnd.currentEntity->e.origin[ 2 ] - lightspot[ 2 ];

	float height = -lheight + 1.0;

	for ( int i = 0; i < paliashdr->numVertexes; i++ ) {
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

	EnableArrays( paliashdr->numVertexes );
	R_DrawElements( paliashdr->numIndexes, paliashdr->indexes );
	DisableArrays();
}

static bool R_CullMd2Model( trRefEntity_t* e ) {
	mmd2_t* paliashdr = tr.currentModel->q2_md2;

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

	dmd2_frame_t* pframe = ( dmd2_frame_t* )( paliashdr->frames + e->e.frame * paliashdr->framesize );

	dmd2_frame_t* poldframe = ( dmd2_frame_t* )( paliashdr->frames + e->e.oldframe * paliashdr->framesize );

	/*
	** compute axially aligned mins and maxs
	*/
	vec3_t bounds[ 2 ];
	if ( pframe == poldframe ) {
		for ( int i = 0; i < 3; i++ ) {
			bounds[ 0 ][ i ] = pframe->translate[ i ];
			bounds[ 1 ][ i ] = bounds[ 0 ][ i ] + pframe->scale[ i ] * 255;
		}
	} else {
		for ( int i = 0; i < 3; i++ ) {
			vec3_t thismins, thismaxs;
			thismins[ i ] = pframe->translate[ i ];
			thismaxs[ i ] = thismins[ i ] + pframe->scale[ i ] * 255;

			vec3_t oldmins, oldmaxs;
			oldmins[ i ]  = poldframe->translate[ i ];
			oldmaxs[ i ]  = oldmins[ i ] + poldframe->scale[ i ] * 255;

			if ( thismins[ i ] < oldmins[ i ] ) {
				bounds[ 0 ][ i ] = thismins[ i ];
			} else {
				bounds[ 0 ][ i ] = oldmins[ i ];
			}

			if ( thismaxs[ i ] > oldmaxs[ i ] ) {
				bounds[ 1 ][ i ] = thismaxs[ i ];
			} else {
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

	mmd2_t* paliashdr = tr.currentModel->q2_md2;
	R_AddDrawSurf( ( surfaceType_t* )paliashdr, tr.defaultShader, 0, false, false, ATI_TESS_NONE, forcedSortIndex );
}

void RB_SurfaceMd2( mmd2_t* paliashdr ) {

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
				} else {
					r_lightlevel->value = 150 * md2_shadelight[ 2 ] / 255;
				}
			} else {
				if ( md2_shadelight[ 1 ] > md2_shadelight[ 2 ] ) {
					r_lightlevel->value = 150 * md2_shadelight[ 1 ] / 255;
				} else {
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

	c_alias_polys += paliashdr->numIndexes / 3;

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

	// select skin
	image_t* skin;
	if ( backEnd.currentEntity->e.customSkin ) {
		skin = tr.images[ backEnd.currentEntity->e.customSkin ];		// custom player skin
	} else {
		if ( backEnd.currentEntity->e.skinNum >= MAX_MD2_SKINS ) {
			skin = R_GetModelByHandle( backEnd.currentEntity->e.hModel )->q2_skins[ 0 ];
		} else {
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
	} else {
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
