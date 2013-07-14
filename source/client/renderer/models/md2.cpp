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
#include "../main.h"
#include "../backend.h"
#include "../surface.h"
#include "../cvars.h"
#include "../light.h"
#include "../skin.h"
#include "../../../common/Common.h"
#include "../../../common/endian.h"
#include "../../../common/math/Math.h"

void Mod_FreeMd2Model( idRenderModel* mod ) {
	Mem_Free( mod->q2_md2 );
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

static void R_Md2SetupEntityLighting( trRefEntity_t* ent ) {
	//
	// get lighting information
	//
	vec3_t md2_shadelight;
	if ( ent->e.renderfx & RF_ABSOLUTE_LIGHT ) {
		for ( int i = 0; i < 3; i++ ) {
			md2_shadelight[ i ] = ent->e.absoluteLight * 255;
		}
	} else {
		float l[4];
		R_LightPointQ2( ent->e.origin, l, backEnd.refdef );
		md2_shadelight[0] = Min(l[0] * 255, 255.0f);
		md2_shadelight[1] = Min(l[1] * 255, 255.0f);
		md2_shadelight[2] = Min(l[2] * 255, 255.0f);

		// player lighting hack for communication back to server
		// big hack!
		if ( ent->e.renderfx & RF_FIRST_PERSON ) {
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
	ent->e.shadowPlane = lightspot[ 2 ];

	if ( ent->e.renderfx & RF_MINLIGHT ) {
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

	if ( ent->e.renderfx & RF_GLOW ) {
		// bonus items will pulse with time
		int scale = 25 * sin( backEnd.refdef.floatTime * 7 );
		for ( int i = 0; i < 3; i++ ) {
			int min = md2_shadelight[ i ] * 0.8;
			md2_shadelight[ i ] = Min(md2_shadelight[ i ] + scale, 255.0f);
			if ( md2_shadelight[ i ] < min ) {
				md2_shadelight[ i ] = min;
			}
		}
	}

	// =================
	// PGM	ir goggles color override
	if ( backEnd.refdef.rdflags & RDF_IRGOGGLES && ent->e.renderfx & RF_IR_VISIBLE ) {
		md2_shadelight[ 0 ] = 255;
		md2_shadelight[ 1 ] = 0;
		md2_shadelight[ 2 ] = 0;
	}
	// PGM
	// =================
	VectorCopy( md2_shadelight, ent->ambientLight );
	VectorCopy( md2_shadelight, ent->directedLight );

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
}

void R_AddMd2Surfaces( trRefEntity_t* ent, int forcedSortIndex ) {
	if ( ( tr.currentEntity->e.renderfx & RF_THIRD_PERSON ) && !tr.viewParms.isPortal ) {
		return;
	}

	if ( !( ent->e.renderfx & RF_FIRST_PERSON ) ) {
		if ( R_CullMd2Model( ent ) ) {
			return;
		}
	}

	R_Md2SetupEntityLighting( ent );

	mmd2_t* paliashdr = tr.currentModel->q2_md2;

	if ( ( ent->e.frame >= paliashdr->num_frames ) || ( ent->e.frame < 0 ) ) {
		common->Printf( "R_AddMd2Surfaces %s: no such frame %d\n",
			R_GetModelByHandle( ent->e.hModel )->name, ent->e.frame );
		ent->e.frame = 0;
		ent->e.oldframe = 0;
	}

	if ( ( ent->e.oldframe >= paliashdr->num_frames ) || ( ent->e.oldframe < 0 ) ) {
		common->Printf( "R_AddMd2Surfaces %s: no such oldframe %d\n",
			R_GetModelByHandle( ent->e.hModel )->name, ent->e.oldframe );
		ent->e.frame = 0;
		ent->e.oldframe = 0;
	}

	if ( !r_lerpmodels->value ) {
		ent->e.backlerp = 0;
	}

	shader_t* shader;
	if ( ent->e.renderfx & RF_COLOUR_SHELL ) {
		shader = tr.colorShellShader;
	} else if ( ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins ) {
		// custom player skin
		skin_t* skin = R_GetSkinByHandle( ent->e.customSkin );
		shader = skin->surfaces[ 0 ]->shader;
	} else {
		if ( ent->e.skinNum >= MAX_MD2_SKINS ) {
			shader = tr.currentModel->q2_skins_shader[ 0 ];
		} else {
			shader = tr.currentModel->q2_skins_shader[ ent->e.skinNum ];
			if ( !shader ) {
				shader = tr.currentModel->q2_skins_shader[ 0 ];
			}
		}
		if ( !shader ) {
			shader = tr.defaultShader;	// fallback...
		}
	}
	R_AddDrawSurf( ( surfaceType_t* )paliashdr, shader, 0, false, false, ATI_TESS_NONE, forcedSortIndex );
	if ( r_shadows->value && !( tr.currentEntity->e.renderfx & ( RF_TRANSLUCENT | RF_FIRST_PERSON ) ) ) {
		R_AddDrawSurf( ( surfaceType_t* )paliashdr, tr.projectionShadowShader, 0, false, false, ATI_TESS_NONE, 1 );
	}
}

static void GL_LerpVerts( mmd2_t* paliashdr, dmd2_trivertx_t* v, dmd2_trivertx_t* ov,
	float* lerp, float* normals, float move[ 3 ], float frontv[ 3 ], float backv[ 3 ] ) {
	for ( int i = 0; i < paliashdr->numVertexes; i++, lerp += 4, normals += 4 ) {
		lerp[ 0 ] = move[ 0 ] + ov[ i ].v[ 0 ] * backv[ 0 ] + v[ i ].v[ 0 ] * frontv[ 0 ];
		lerp[ 1 ] = move[ 1 ] + ov[ i ].v[ 1 ] * backv[ 1 ] + v[ i ].v[ 1 ] * frontv[ 1 ];
		lerp[ 2 ] = move[ 2 ] + ov[ i ].v[ 2 ] * backv[ 2 ] + v[ i ].v[ 2 ] * frontv[ 2 ];
		const float* normal = bytedirs[ v[ i ].lightnormalindex ];
		VectorCopy( normal, normals );
		tess.texCoords[ i ][ 0 ][ 0 ] = paliashdr->texCoords[ i ].x;
		tess.texCoords[ i ][ 0 ][ 1 ] = paliashdr->texCoords[ i ].y;
	}
}

static void RB_EmitMd2VertexesAndIndexes( trRefEntity_t* ent, mmd2_t* paliashdr ) {
	RB_CHECKOVERFLOW( paliashdr->numVertexes, paliashdr->numIndexes );

	dmd2_frame_t* frame = ( dmd2_frame_t* )( paliashdr->frames +
											 ent->e.frame * paliashdr->framesize );
	dmd2_trivertx_t* v = frame->verts;

	dmd2_frame_t* oldframe = ( dmd2_frame_t* )( paliashdr->frames +
												ent->e.oldframe * paliashdr->framesize );
	dmd2_trivertx_t* ov = oldframe->verts;

	float backlerp = ent->e.backlerp;
	float frontlerp = 1.0 - backlerp;

	// move should be the delta back to the previous frame * backlerp
	vec3_t delta;
	VectorSubtract( ent->e.oldorigin, ent->e.origin, delta );

	vec3_t move;
	move[ 0 ] = DotProduct( delta, ent->e.axis[ 0 ] );		// forward
	move[ 1 ] = DotProduct( delta, ent->e.axis[ 1 ] );		// left
	move[ 2 ] = DotProduct( delta, ent->e.axis[ 2 ] );		// up

	VectorAdd( move, oldframe->translate, move );

	for ( int i = 0; i < 3; i++ ) {
		move[ i ] = backlerp * move[ i ] + frontlerp * frame->translate[ i ];
	}

	vec3_t frontv, backv;
	for ( int i = 0; i < 3; i++ ) {
		frontv[ i ] = frontlerp * frame->scale[ i ];
		backv[ i ] = backlerp * oldframe->scale[ i ];
	}

	int numVerts = tess.numVertexes;
	GL_LerpVerts( paliashdr, v, ov, tess.xyz[ numVerts ], tess.normal[ numVerts ], move, frontv, backv );
	tess.numVertexes += paliashdr->numVertexes;

	int numIndexes = tess.numIndexes;
	for ( int i = 0; i < paliashdr->numIndexes; i++ ) {
		tess.indexes[ numIndexes + i ] = numVerts + paliashdr->indexes[ i ];
	}
	tess.numIndexes += paliashdr->numIndexes;
}

//	interpolates between two frames and origins
//	FIXME: batch lerp all vertexes
void RB_SurfaceMd2( mmd2_t* paliashdr ) {
	trRefEntity_t* ent = backEnd.currentEntity;

	c_alias_polys += paliashdr->numIndexes / 3;

	if ( ent->e.renderfx & RF_LEFTHAND ) {
		qglMatrixMode( GL_PROJECTION );
		qglPushMatrix();
		qglLoadIdentity();
		qglScalef( -1, 1, 1 );
		qglMultMatrixf( backEnd.viewParms.projectionMatrix );
		qglMatrixMode( GL_MODELVIEW );
	}

	RB_EmitMd2VertexesAndIndexes( ent, paliashdr );

	if ( ent->e.renderfx & RF_LEFTHAND ) {
		qglMatrixMode( GL_PROJECTION );
		qglPopMatrix();
		qglMatrixMode( GL_MODELVIEW );
	}
}
