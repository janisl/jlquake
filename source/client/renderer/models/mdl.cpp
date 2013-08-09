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
#include "model.h"
#include "../main.h"
#include "../backend.h"
#include "../surfaces.h"
#include "../cvars.h"
#include "../light.h"
#include "../../../common/Common.h"
#include "../../../common/common_defs.h"
#include "../../../common/strings.h"
#include "../../../common/endian.h"
#include "RenderModelMDL.h"

struct idMdlVertexRemap {
	int xyzIndex;
	int stIndex;
	bool onBackSide;
};

static float aliastransform[ 3 ][ 4 ];

vec3_t mdl_mins;
vec3_t mdl_maxs;

int mdl_posenum;
mesh1hdr_t* mdl_pheader;

// a pose is a single set of vertexes.  a frame may be
// an animating sequence of poses
static const dmdl_trivertx_t* poseverts[ MAXALIASFRAMES ];
dmdl_stvert_t mdl_stverts[ MAXALIASVERTS ];
mmesh1triangle_t mdl_triangles[ MAXALIASTRIS ];

static idRenderModel* aliasmodel;
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

const void* Mod_LoadAliasFrame( const void* pin, mmesh1framedesc_t* frame ) {
	const dmdl_frame_t* pdaliasframe = ( const dmdl_frame_t* )pin;

	String::Cpy( frame->name, pdaliasframe->name );
	frame->firstpose = mdl_posenum;
	frame->numposes = 1;

	for ( int i = 0; i < 3; i++ ) {
		// these are byte values, so we don't have to worry about
		// endianness
		frame->bboxmin.v[ i ] = pdaliasframe->bboxmin.v[ i ];
		frame->bboxmin.v[ i ] = pdaliasframe->bboxmax.v[ i ];
	}

	const dmdl_trivertx_t* pinframe = ( const dmdl_trivertx_t* )( pdaliasframe + 1 );

	aliastransform[ 0 ][ 0 ] = mdl_pheader->scale[ 0 ];
	aliastransform[ 1 ][ 1 ] = mdl_pheader->scale[ 1 ];
	aliastransform[ 2 ][ 2 ] = mdl_pheader->scale[ 2 ];
	aliastransform[ 0 ][ 3 ] = mdl_pheader->scale_origin[ 0 ];
	aliastransform[ 1 ][ 3 ] = mdl_pheader->scale_origin[ 1 ];
	aliastransform[ 2 ][ 3 ] = mdl_pheader->scale_origin[ 2 ];

	for ( int j = 0; j < mdl_pheader->numverts; j++ ) {
		vec3_t in;
		in[ 0 ] = pinframe[ j ].v[ 0 ];
		in[ 1 ] = pinframe[ j ].v[ 1 ];
		in[ 2 ] = pinframe[ j ].v[ 2 ];
		vec3_t out;
		R_AliasTransformVector( in, out );
		for ( int i = 0; i < 3; i++ ) {
			if ( mdl_mins[ i ] > out[ i ] ) {
				mdl_mins[ i ] = out[ i ];
			}
			if ( mdl_maxs[ i ] < out[ i ] ) {
				mdl_maxs[ i ] = out[ i ];
			}
		}
	}
	poseverts[ mdl_posenum ] = pinframe;
	mdl_posenum++;

	pinframe += mdl_pheader->numverts;

	return ( const void* )pinframe;
}

const void* Mod_LoadAliasGroup( const void* pin, mmesh1framedesc_t* frame ) {
	const dmdl_group_t* pingroup = ( const dmdl_group_t* )pin;

	int numframes = LittleLong( pingroup->numframes );

	frame->firstpose = mdl_posenum;
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

	aliastransform[ 0 ][ 0 ] = mdl_pheader->scale[ 0 ];
	aliastransform[ 1 ][ 1 ] = mdl_pheader->scale[ 1 ];
	aliastransform[ 2 ][ 2 ] = mdl_pheader->scale[ 2 ];
	aliastransform[ 0 ][ 3 ] = mdl_pheader->scale_origin[ 0 ];
	aliastransform[ 1 ][ 3 ] = mdl_pheader->scale_origin[ 1 ];
	aliastransform[ 2 ][ 3 ] = mdl_pheader->scale_origin[ 2 ];

	for ( int i = 0; i < numframes; i++ ) {
		poseverts[ mdl_posenum ] = ( const dmdl_trivertx_t* )( ( const dmdl_frame_t* )ptemp + 1 );

		for ( int j = 0; j < mdl_pheader->numverts; j++ ) {
			vec3_t in;
			in[ 0 ] = poseverts[ mdl_posenum ][ j ].v[ 0 ];
			in[ 1 ] = poseverts[ mdl_posenum ][ j ].v[ 1 ];
			in[ 2 ] = poseverts[ mdl_posenum ][ j ].v[ 2 ];
			vec3_t out;
			R_AliasTransformVector( in, out );
			for ( int k = 0; k < 3; k++ ) {
				if ( mdl_mins[ k ] > out[ k ] ) {
					mdl_mins[ k ] = out[ k ];
				}
				if ( mdl_maxs[ k ] < out[ k ] ) {
					mdl_maxs[ k ] = out[ k ];
				}
			}
		}

		mdl_posenum++;

		ptemp = ( const dmdl_trivertx_t* )( ( const dmdl_frame_t* )ptemp + 1 ) + mdl_pheader->numverts;
	}

	return ptemp;
}

void* Mod_LoadAllSkins( int numskins, dmdl_skintype_t* pskintype, int mdl_flags, idSkinTranslation* skinTranslation ) {
	if ( numskins < 1 || numskins > MAX_MESH1_SKINS ) {
		common->FatalError( "Mod_LoadAllSkins: Invalid # of skins: %d\n", numskins );
	}

	int s = mdl_pheader->skinwidth * mdl_pheader->skinheight;

	int texture_mode = IMG8MODE_Skin;
	if ( GGameType & GAME_Hexen2 ) {
		if ( mdl_flags & H2MDLEF_HOLEY ) {
			texture_mode = IMG8MODE_SkinHoley;
		} else if ( mdl_flags & H2MDLEF_TRANSPARENT ) {
			texture_mode = IMG8MODE_SkinTransparent;
		} else if ( mdl_flags & H2MDLEF_SPECIAL_TRANS ) {
			texture_mode = IMG8MODE_SkinSpecialTrans;
		}
	}

	for ( int i = 0; i < numskins; i++ ) {
		if ( pskintype->type == ALIAS_SKIN_SINGLE ) {
			byte* pic = ( byte* )( pskintype + 1 );

			image_t* topTexture;
			image_t* bottomTexture;
			if ( skinTranslation ) {
				idList<byte> picTop;
				picTop.SetNum( s * 4 );
				idList<byte> picBottom;
				picBottom.SetNum( s * 4 );
				R_ExtractTranslatedImages( *skinTranslation, pic, picTop.Ptr(), picBottom.Ptr(), mdl_pheader->skinwidth, mdl_pheader->skinheight );
				char topName[ 32 ];
				sprintf( topName, "%s_%i_top", loadmodel->name, i );
				topTexture = R_CreateImage( topName, picTop.Ptr(), mdl_pheader->skinwidth, mdl_pheader->skinheight, true, true, GL_REPEAT );
				char bottomName[ 32 ];
				sprintf( bottomName, "%s_%i_bottom", loadmodel->name, i );
				bottomTexture = R_CreateImage( bottomName, picBottom.Ptr(), mdl_pheader->skinwidth, mdl_pheader->skinheight, true, true, GL_REPEAT );
			} else {
				topTexture = NULL;
				bottomTexture = NULL;
			}

			byte* pic32 = R_ConvertImage8To32( pic, mdl_pheader->skinwidth, mdl_pheader->skinheight, texture_mode );
			byte* picFullBright = R_GetFullBrightImage( pic, pic32, mdl_pheader->skinwidth, mdl_pheader->skinheight );

			char name[ 32 ];
			sprintf( name, "%s_%i", loadmodel->name, i );

			image_t* gl_texture = R_CreateImage( name, pic32, mdl_pheader->skinwidth, mdl_pheader->skinheight, true, true, GL_REPEAT );
			delete[] pic32;
			image_t* fullBrightTexture;
			if ( picFullBright ) {
				char fbname[ 32 ];
				sprintf( fbname, "%s_%i_fb", loadmodel->name, i );
				fullBrightTexture = R_CreateImage( fbname, picFullBright, mdl_pheader->skinwidth, mdl_pheader->skinheight, true, true, GL_REPEAT );
				delete[] picFullBright;
			} else {
				fullBrightTexture = NULL;
			}
			pskintype = ( dmdl_skintype_t* )( ( byte* )( pskintype + 1 ) + s );
			mdl_pheader->shaders[ i ] = R_BuildMdlShader( loadmodel->name, i, 1, &gl_texture,
				&fullBrightTexture, topTexture, bottomTexture, loadmodel->q1_flags );
		} else {
			// animating skin group.  yuck.
			pskintype++;
			dmdl_skingroup_t* pinskingroup = ( dmdl_skingroup_t* )pskintype;
			int groupskins = LittleLong( pinskingroup->numskins );
			dmdl_skininterval_t* pinskinintervals = ( dmdl_skininterval_t* )( pinskingroup + 1 );

			pskintype = ( dmdl_skintype_t* )( pinskinintervals + groupskins );

			int j;
			bool haveFullBrightFrame = false;
			image_t* gl_texture[ 4 ];
			image_t* fullBrightTexture[ 4 ];
			for ( j = 0; j < groupskins; j++ ) {
				char name[ 32 ];
				sprintf( name, "%s_%i_%i", loadmodel->name, i, j );

				byte* pic32 = R_ConvertImage8To32( ( byte* )pskintype, mdl_pheader->skinwidth, mdl_pheader->skinheight, texture_mode );
				byte* picFullBright = R_GetFullBrightImage( ( byte* )pskintype, pic32, mdl_pheader->skinwidth, mdl_pheader->skinheight );
				gl_texture[ j & 3 ] = R_CreateImage( name, pic32, mdl_pheader->skinwidth, mdl_pheader->skinheight, true, true, GL_REPEAT );
				delete[] pic32;
				if ( picFullBright ) {
					char fbname[ 32 ];
					sprintf( fbname, "%s_%i_%i_fb", loadmodel->name, i, j );
					fullBrightTexture[ j & 3 ] = R_CreateImage( fbname, picFullBright, mdl_pheader->skinwidth, mdl_pheader->skinheight, true, true, GL_REPEAT );
					delete[] picFullBright;
					haveFullBrightFrame = true;
				} else {
					fullBrightTexture[ j & 3 ] = NULL;
				}
				pskintype = ( dmdl_skintype_t* )( ( byte* )pskintype + s );
			}
			int k = j;
			for ( /* */; j < 4; j++ ) {
				gl_texture[ j & 3 ] = gl_texture[ j - k ];
				fullBrightTexture[ j & 3 ] = fullBrightTexture[ j - k ];
			}
			//	Make sure all fullbright textures are either all NULL or all are valid textures.
			// If some frames don't have it, use a fully transparent image.
			if ( haveFullBrightFrame ) {
				for ( int j = 0; j < 4; j++ ) {
					if ( !fullBrightTexture[ j ] ) {
						fullBrightTexture[ j ] = tr.transparentImage;
					}
				}
			}
			mdl_pheader->shaders[ i ] = R_BuildMdlShader( loadmodel->name, i, 4, gl_texture,
				fullBrightTexture, NULL, NULL, loadmodel->q1_flags );
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

void GL_MakeAliasModelDisplayLists( idRenderModel* m, mesh1hdr_t* hdr ) {
	aliasmodel = m;
	paliashdr = hdr;	// (mesh1hdr_t *)Mod_Extradata (m);

	common->Printf( "meshing %s...\n",m->name );

	numorder = 0;
	paliashdr->numIndexes = mdl_pheader->numtris * 3;
	paliashdr->indexes = new glIndex_t[ paliashdr->numIndexes ];
	glIndex_t* indexes = paliashdr->indexes;
	for ( int i = 0; i < mdl_pheader->numtris; i++ ) {
		mmesh1triangle_t* triangle = &mdl_triangles[ i ];
		for ( int j = 0; j < 3; j++ ) {
			*indexes++ = AddMdlVertex( triangle->vertindex[ j ], triangle->stindex[ j ],
				!triangle->facesfront && mdl_stverts[ triangle->stindex[ j ] ].onseam );
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
		float s = mdl_stverts[ k ].s;
		float t = mdl_stverts[ k ].t;

		if ( vertexorder[ i ].onBackSide ) {
			s += mdl_pheader->skinwidth / 2;	// on back side
		}
		texCoords->x = ( s + 0.5f ) / mdl_pheader->skinwidth;
		texCoords->y = ( t + 0.5f ) / mdl_pheader->skinheight;
	}
}

void Mod_FreeMdlModel( idRenderModel* mod ) {
	mesh1hdr_t* mdl_pheader = ( mesh1hdr_t* )mod->q1_cache;
	delete[] mdl_pheader->indexes;
	delete[] mdl_pheader->posedata;
	delete[] mdl_pheader->texCoords;
	Mem_Free( mdl_pheader );
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
		idRenderModel* clmodel = R_GetModelByHandle( e->hModel );
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

static void R_MdlSetupEntityLighting( trRefEntity_t* ent ) {
	//
	// get lighting information
	//
	float ambientlight = R_CalcEntityLight( &ent->e );
	ent->e.shadowPlane = lightspot[ 2 ];
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

	idRenderModel* clmodel = R_GetModelByHandle( ent->e.hModel );

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
}

void R_AddMdlSurfaces( trRefEntity_t* e, int forcedSortIndex ) {
	if ( ( tr.currentEntity->e.renderfx & RF_THIRD_PERSON ) && !tr.viewParms.isPortal ) {
		return;
	}

	if ( R_CullLocalBox( &tr.currentModel->q1_mins ) == CULL_OUT ) {
		return;
	}

	R_MdlSetupEntityLighting( e );

	mesh1hdr_t* paliashdr = ( mesh1hdr_t* )tr.currentModel->q1_cache;
	shader_t* shader;
	if ( e->e.renderfx & RF_COLORSHADE ) {
		shader = tr.colorShadeShader;
	} else if ( e->e.customShader ) {
		shader = R_GetShaderByHandle( e->e.customShader );
	} else {
		shader = paliashdr->shaders[ e->e.skinNum ];
	}
	R_AddDrawSurf( ( surfaceType_t* )paliashdr, shader, 0, false, false, ATI_TESS_NONE, forcedSortIndex );
	if ( r_shadows->value && !( e->e.renderfx & RF_NOSHADOW ) ) {
		R_AddDrawSurf( ( surfaceType_t* )paliashdr, tr.projectionShadowShader, 0, false, false, ATI_TESS_NONE, 1 );
	}
}

void RB_SurfaceMdl( mesh1hdr_t* paliashdr ) {
	trRefEntity_t* ent = backEnd.currentEntity;

	c_alias_polys += paliashdr->numtris;

	RB_CHECKOVERFLOW( paliashdr->poseverts, paliashdr->numIndexes );

	if ( ent->e.frame >= paliashdr->numframes || ent->e.frame < 0 ) {
		common->DPrintf( "R_AliasSetupFrame: no such frame %d\n", ent->e.frame );
		ent->e.frame = 0;
	}

	int mdl_posenum = paliashdr->frames[ ent->e.frame ].firstpose;
	int numposes = paliashdr->frames[ ent->e.frame ].numposes;

	if ( numposes > 1 ) {
		float interval = paliashdr->frames[ ent->e.frame ].interval;
		mdl_posenum += ( int )( tr.refdef.floatTime / interval ) % numposes;
	}

	dmdl_trivertx_t* verts = paliashdr->posedata;
	verts += mdl_posenum * paliashdr->poseverts;

	int numVerts = tess.numVertexes;
	for ( int i = 0; i < paliashdr->poseverts; i++ ) {
		tess.texCoords[ numVerts + i ][ 0 ][ 0 ] = paliashdr->texCoords[ i ].x;
		tess.texCoords[ numVerts + i ][ 0 ][ 1 ] = paliashdr->texCoords[ i ].y;
		tess.xyz[ numVerts + i ][ 0 ] = verts[ i ].v[ 0 ] * paliashdr->scale[ 0 ] + paliashdr->scale_origin[ 0 ];
		tess.xyz[ numVerts + i ][ 1 ] = verts[ i ].v[ 1 ] * paliashdr->scale[ 1 ] + paliashdr->scale_origin[ 1 ];
		tess.xyz[ numVerts + i ][ 2 ] = verts[ i ].v[ 2 ] * paliashdr->scale[ 2 ] + paliashdr->scale_origin[ 2 ];
		tess.normal[ numVerts + i ][ 0 ] = bytedirs[ verts[ i ].lightnormalindex ][ 0 ];
		tess.normal[ numVerts + i ][ 1 ] = bytedirs[ verts[ i ].lightnormalindex ][ 1 ];
		tess.normal[ numVerts + i ][ 2 ] = bytedirs[ verts[ i ].lightnormalindex ][ 2 ];
	}
	tess.numVertexes += paliashdr->poseverts;

	int numIndexes = tess.numIndexes;
	for ( int i = 0; i < paliashdr->numIndexes; i++ ) {
		tess.indexes[ numIndexes + i ] = numVerts + paliashdr->indexes[ i ];
	}
	tess.numIndexes += paliashdr->numIndexes;
}

bool R_MdlHasHexen2Transparency( idRenderModel* model ) {
	if ( model->type == MOD_MESH1 ) {
		return !!( model->q1_flags & ( H2MDLEF_TRANSPARENT | H2MDLEF_HOLEY | H2MDLEF_SPECIAL_TRANS ) );
	}
	if ( model->type == MOD_SPRITE ) {
		return true;
	}
	return false;
}
