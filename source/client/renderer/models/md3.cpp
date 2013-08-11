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
#include "../surfaces.h"
#include "../cvars.h"
#include "../light.h"
#include "../skin.h"
#include "../../../common/Common.h"
#include "../../../common/common_defs.h"
#include "../../../common/strings.h"
#include "../../../common/endian.h"

void R_FreeMd3( idRenderModel* mod ) {
	Mem_Free( mod->q3_md3[ 0 ].header );
	delete[] mod->q3_md3[ 0 ].surfaces;
	for ( int lod = 1; lod < MD3_MAX_LODS; lod++ ) {
		if ( mod->q3_md3[ lod ].header != mod->q3_md3[ lod - 1 ].header ) {
			Mem_Free( mod->q3_md3[ lod ].header );
			delete[] mod->q3_md3[ lod ].surfaces;
		}
	}
}

void R_RegisterMd3Shaders( idRenderModel* mod, int lod ) {
	// swap all the surfaces
	md3Surface_t* surf = ( md3Surface_t* )( ( byte* )mod->q3_md3[ lod ].header + mod->q3_md3[ lod ].header->ofsSurfaces );
	for ( int i = 0; i < mod->q3_md3[ lod ].header->numSurfaces; i++ ) {
		// register the shaders
		md3Shader_t* shader = ( md3Shader_t* )( ( byte* )surf + surf->ofsShaders );
		for ( int j = 0; j < surf->numShaders; j++, shader++ ) {
			shader_t* sh = R_FindShader( shader->name, LIGHTMAP_NONE, true );
			if ( sh->defaultShader ) {
				shader->shaderIndex = 0;
			} else {
				shader->shaderIndex = sh->index;
			}
		}
		// find the next surface
		surf = ( md3Surface_t* )( ( byte* )surf + surf->ofsEnd );
	}
}

static float ProjectRadius( float r, vec3_t location ) {
	float c = DotProduct( tr.viewParms.orient.axis[ 0 ], tr.viewParms.orient.origin );
	float dist = DotProduct( tr.viewParms.orient.axis[ 0 ], location ) - c;

	if ( dist <= 0 ) {
		return 0;
	}

	vec3_t p;
	p[ 0 ] = 0;
	p[ 1 ] = idMath::Fabs( r );
	p[ 2 ] = -dist;

	float projected[ 4 ];
	projected[ 0 ] = p[ 0 ] * tr.viewParms.projectionMatrix[ 0 ] +
					 p[ 1 ] * tr.viewParms.projectionMatrix[ 4 ] +
					 p[ 2 ] * tr.viewParms.projectionMatrix[ 8 ] +
					 tr.viewParms.projectionMatrix[ 12 ];

	projected[ 1 ] = p[ 0 ] * tr.viewParms.projectionMatrix[ 1 ] +
					 p[ 1 ] * tr.viewParms.projectionMatrix[ 5 ] +
					 p[ 2 ] * tr.viewParms.projectionMatrix[ 9 ] +
					 tr.viewParms.projectionMatrix[ 13 ];

	projected[ 2 ] = p[ 0 ] * tr.viewParms.projectionMatrix[ 2 ] +
					 p[ 1 ] * tr.viewParms.projectionMatrix[ 6 ] +
					 p[ 2 ] * tr.viewParms.projectionMatrix[ 10 ] +
					 tr.viewParms.projectionMatrix[ 14 ];

	projected[ 3 ] = p[ 0 ] * tr.viewParms.projectionMatrix[ 3 ] +
					 p[ 1 ] * tr.viewParms.projectionMatrix[ 7 ] +
					 p[ 2 ] * tr.viewParms.projectionMatrix[ 11 ] +
					 tr.viewParms.projectionMatrix[ 15 ];


	float pr = projected[ 1 ] / projected[ 3 ];

	if ( pr > 1.0f ) {
		pr = 1.0f;
	}

	return pr;
}

static int R_CullModel( md3Header_t* header, trRefEntity_t* ent ) {
	// compute frame pointers
	md3Frame_t* newFrame = ( md3Frame_t* )( ( byte* )header + header->ofsFrames ) + ent->e.frame;
	md3Frame_t* oldFrame = ( md3Frame_t* )( ( byte* )header + header->ofsFrames ) + ent->e.oldframe;

	// cull bounding sphere ONLY if this is not an upscaled entity
	if ( !ent->e.nonNormalizedAxes ) {
		if ( ent->e.frame == ent->e.oldframe ) {
			switch ( R_CullLocalPointAndRadius( newFrame->localOrigin, newFrame->radius ) ) {
			case CULL_OUT:
				tr.pc.c_sphere_cull_md3_out++;
				return CULL_OUT;

			case CULL_IN:
				tr.pc.c_sphere_cull_md3_in++;
				return CULL_IN;

			case CULL_CLIP:
				tr.pc.c_sphere_cull_md3_clip++;
				break;
			}
		} else {
			int sphereCull = R_CullLocalPointAndRadius( newFrame->localOrigin, newFrame->radius );
			int sphereCullB;
			if ( newFrame == oldFrame ) {
				sphereCullB = sphereCull;
			} else {
				sphereCullB = R_CullLocalPointAndRadius( oldFrame->localOrigin, oldFrame->radius );
			}

			if ( sphereCull == sphereCullB ) {
				if ( sphereCull == CULL_OUT ) {
					tr.pc.c_sphere_cull_md3_out++;
					return CULL_OUT;
				} else if ( sphereCull == CULL_IN ) {
					tr.pc.c_sphere_cull_md3_in++;
					return CULL_IN;
				} else {
					tr.pc.c_sphere_cull_md3_clip++;
				}
			}
		}
	}

	// calculate a bounding box in the current coordinate system
	vec3_t bounds[ 2 ];
	for ( int i = 0; i < 3; i++ ) {
		bounds[ 0 ][ i ] = oldFrame->bounds[ 0 ][ i ] < newFrame->bounds[ 0 ][ i ] ? oldFrame->bounds[ 0 ][ i ] : newFrame->bounds[ 0 ][ i ];
		bounds[ 1 ][ i ] = oldFrame->bounds[ 1 ][ i ] > newFrame->bounds[ 1 ][ i ] ? oldFrame->bounds[ 1 ][ i ] : newFrame->bounds[ 1 ][ i ];
	}

	switch ( R_CullLocalBox( bounds ) ) {
	case CULL_IN:
		tr.pc.c_box_cull_md3_in++;
		return CULL_IN;

	case CULL_CLIP:
		tr.pc.c_box_cull_md3_clip++;
		return CULL_CLIP;

	case CULL_OUT:
	default:
		tr.pc.c_box_cull_md3_out++;
		return CULL_OUT;
	}
}

static int R_ComputeLOD( trRefEntity_t* ent ) {
	int lod;
	if ( tr.currentModel->q3_numLods < 2 ) {
		// model has only 1 LOD level, skip computations and bias
		lod = 0;
	} else {
		// multiple LODs exist, so compute projected bounding sphere
		// and use that as a criteria for selecting LOD

		// RF, checked for a forced lowest LOD
		if ( ent->e.reFlags & REFLAG_FORCE_LOD ) {
			return tr.currentModel->q3_numLods - 1;
		}

		md3Frame_t* frame = ( md3Frame_t* )( ( ( byte* )tr.currentModel->q3_md3[ 0 ].header ) + tr.currentModel->q3_md3[ 0 ].header->ofsFrames );

		frame += ent->e.frame;

		float radius = RadiusFromBounds( frame->bounds[ 0 ], frame->bounds[ 1 ] );

		float projectedRadius = ProjectRadius( radius, ent->e.origin );
		float flod;
		if ( projectedRadius != 0 ) {
			float lodscale = r_lodscale->value;
			if ( lodscale > 20 ) {
				lodscale = 20;
			}
			flod = 1.0f - projectedRadius * lodscale;
		} else {
			// object intersects near view plane, e.g. view weapon
			flod = 0;
		}

		flod *= tr.currentModel->q3_numLods;
		lod = idMath::FtoiFast( flod );

		if ( lod < 0 ) {
			lod = 0;
		} else if ( lod >= tr.currentModel->q3_numLods ) {
			lod = tr.currentModel->q3_numLods - 1;
		}
	}

	lod += r_lodbias->integer;

	if ( lod >= tr.currentModel->q3_numLods ) {
		lod = tr.currentModel->q3_numLods - 1;
	}
	if ( lod < 0 ) {
		lod = 0;
	}

	return lod;
}

static int R_ComputeFogNum( md3Header_t* header, trRefEntity_t* ent ) {
	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return 0;
	}

	// FIXME: non-normalized axis issues
	md3Frame_t* md3Frame = ( md3Frame_t* )( ( byte* )header + header->ofsFrames ) + ent->e.frame;
	vec3_t localOrigin;
	VectorAdd( ent->e.origin, md3Frame->localOrigin, localOrigin );
	for ( int i = 1; i < tr.world->numfogs; i++ ) {
		mbrush46_fog_t* fog = &tr.world->fogs[ i ];
		int j;
		for ( j = 0; j < 3; j++ ) {
			if ( localOrigin[ j ] - md3Frame->radius >= fog->bounds[ 1 ][ j ] ) {
				break;
			}
			if ( localOrigin[ j ] + md3Frame->radius <= fog->bounds[ 0 ][ j ] ) {
				break;
			}
		}
		if ( j == 3 ) {
			return i;
		}
	}

	return 0;
}

void R_AddMD3Surfaces( trRefEntity_t* ent ) {
	// don't add third_person objects if not in a portal
	bool personalModel = ( ent->e.renderfx & RF_THIRD_PERSON ) && !tr.viewParms.isPortal;

	if ( ent->e.renderfx & RF_WRAP_FRAMES ) {
		ent->e.frame %= tr.currentModel->q3_md3[ 0 ].header->numFrames;
		ent->e.oldframe %= tr.currentModel->q3_md3[ 0 ].header->numFrames;
	}

	//
	// compute LOD
	//
	int lod = ent->e.renderfx & RF_FORCENOLOD ? 0 : R_ComputeLOD( ent );

	//
	// Validate the frames so there is no chance of a crash.
	// This will write directly into the entity structure, so
	// when the surfaces are rendered, they don't need to be
	// range checked again.
	//
	if ( ( ent->e.frame >= tr.currentModel->q3_md3[ lod ].header->numFrames ) ||
		 ( ent->e.frame < 0 ) ||
		 ( ent->e.oldframe >= tr.currentModel->q3_md3[ lod ].header->numFrames ) ||
		 ( ent->e.oldframe < 0 ) ) {
		common->DPrintf( S_COLOR_RED "R_AddMD3Surfaces: no such frame %d to %d for '%s'\n",
			ent->e.oldframe, ent->e.frame, tr.currentModel->name );
		ent->e.frame = 0;
		ent->e.oldframe = 0;
	}

	md3Header_t* header = tr.currentModel->q3_md3[ lod ].header;

	//
	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum.
	//
	int cull = R_CullModel( header, ent );
	if ( cull == CULL_OUT ) {
		return;
	}

	//
	// set up lighting now that we know we aren't culled
	//
	if ( !personalModel || r_shadows->integer > 1 ) {
		R_SetupEntityLighting( &tr.refdef, ent );
	}

	//
	// see if we are in a fog volume
	//
	int fogNum = R_ComputeFogNum( header, ent );

	//
	// draw all surfaces
	//
	for ( int i = 0; i < header->numSurfaces; i++ ) {
		idSurfaceMD3* surface = &tr.currentModel->q3_md3[ lod ].surfaces[ i ];
		md3Surface_t* surfaceData = ( md3Surface_t* ) surface->GetData();
		//	blink will change to be an overlay rather than replacing the head texture.
		// think of it like batman's mask.  the polygons that have eye texture are duplicated
		// and the 'lids' rendered with polygonoffset shader parm over the top of the open eyes.  this gives
		// minimal overdraw/alpha blending/texture use without breaking the model and causing seams
		if ( GGameType & GAME_WolfSP && !String::ICmp( surfaceData->name, "h_blink" ) ) {
			if ( !( ent->e.renderfx & RF_BLINK ) ) {
				continue;
			}
		}

		shader_t* shader;
		if ( ent->e.customShader ) {
			shader = R_GetShaderByHandle( ent->e.customShader );
		} else if ( ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins ) {
			skin_t* skin = R_GetSkinByHandle( ent->e.customSkin );

			// match the surface name to something in the skin file
			shader = tr.defaultShader;

			//----(SA)	added blink
			if ( GGameType & ( GAME_WolfMP | GAME_ET ) && ent->e.renderfx & RF_BLINK ) {
				const char* s = va( "%s_b", surfaceData->name );	// append '_b' for 'blink'
				int hash = Com_HashKey( s, String::Length( s ) );
				for ( int j = 0; j < skin->numSurfaces; j++ ) {
					if ( GGameType & GAME_ET && hash != skin->surfaces[ j ]->hash ) {
						continue;
					}
					if ( !String::Cmp( skin->surfaces[ j ]->name, s ) ) {
						shader = skin->surfaces[ j ]->shader;
						break;
					}
				}
			}

			if ( shader == tr.defaultShader ) {
				// blink reference in skin was not found
				int hash = Com_HashKey( surfaceData->name, sizeof ( surfaceData->name ) );
				for ( int j = 0; j < skin->numSurfaces; j++ ) {
					// the names have both been lowercased
					if ( GGameType & GAME_ET && hash != skin->surfaces[ j ]->hash ) {
						continue;
					}
					if ( !String::Cmp( skin->surfaces[ j ]->name, surfaceData->name ) ) {
						shader = skin->surfaces[ j ]->shader;
						break;
					}
				}
			}

			if ( shader == tr.defaultShader ) {
				common->DPrintf( S_COLOR_RED "WARNING: no shader for surface %s in skin %s\n", surfaceData->name, skin->name );
			} else if ( shader->defaultShader ) {
				common->DPrintf( S_COLOR_RED "WARNING: shader %s in skin %s not found\n", shader->name, skin->name );
			}
		} else if ( surfaceData->numShaders <= 0 ) {
			shader = tr.defaultShader;
		} else {
			md3Shader_t* md3Shader = ( md3Shader_t* )( ( byte* ) surfaceData + surfaceData->ofsShaders );
			md3Shader += ent->e.skinNum % surfaceData->numShaders;
			shader = tr.shaders[ md3Shader->shaderIndex ];
		}

		// we will add shadows even if the main object isn't visible in the view

		// stencil shadows can't do personal models unless I polyhedron clip
		if ( !personalModel &&
			 r_shadows->integer == 2 &&
			 fogNum == 0 &&
			 !( ent->e.renderfx & ( RF_NOSHADOW | RF_DEPTHHACK ) ) &&
			 shader->sort == SS_OPAQUE ) {
			R_AddDrawSurf( surface, tr.shadowShader, 0, false, 0, tr.currentModel->q3_ATI_tess, 0 );
		}

		// projection shadows work fine with personal models
		if ( !( GGameType & GAME_WolfSP ) &&
			 r_shadows->integer == 3 &&
			 fogNum == 0 &&
			 ( ent->e.renderfx & RF_SHADOW_PLANE ) &&
			 shader->sort == SS_OPAQUE ) {
			R_AddDrawSurf( surface, tr.projectionShadowShader, 0, false, 0, tr.currentModel->q3_ATI_tess, 0 );
		}

		// for testing polygon shadows (on /all/ models)
		if ( GGameType & ( GAME_WolfMP | GAME_ET ) && r_shadows->integer == 4 ) {
			R_AddDrawSurf( surface, tr.projectionShadowShader, 0, 0, 0, 0, 0 );
		}

		// don't add third_person objects if not viewing through a portal
		if ( !personalModel ) {
			R_AddDrawSurf( surface, shader, fogNum, false, 0, tr.currentModel->q3_ATI_tess, 0 );
		}
	}
}

//	The inputs to this routing seem to always be close to length = 1.0 (about 0.6 to 2.0)
//	This means that we don't have to worry about zero length or enormously long vectors.
static void VectorArrayNormalize( vec4_t* normals, unsigned int count ) {
	// given the input, it's safe to call VectorNormalizeFast
	while ( count-- ) {
		VectorNormalizeFast( normals[ 0 ] );
		normals++;
	}
}

static void LerpMeshVertexes( md3Surface_t* surf, float backlerp ) {
	float* outXyz = tess.xyz[ tess.numVertexes ];
	float* outNormal = tess.normal[ tess.numVertexes ];

	short* newXyz = ( short* )( ( byte* )surf + surf->ofsXyzNormals ) +
					( backEnd.currentEntity->e.frame * surf->numVerts * 4 );
	short* newNormals = newXyz + 3;

	float newXyzScale = MD3_XYZ_SCALE * ( 1.0 - backlerp );
	float newNormalScale = 1.0 - backlerp;

	int numVerts = surf->numVerts;

	if ( backlerp == 0 ) {
		//
		// just copy the vertexes
		//
		for ( int vertNum = 0; vertNum < numVerts; vertNum++,
			  newXyz += 4, newNormals += 4,
			  outXyz += 4, outNormal += 4 ) {
			outXyz[ 0 ] = newXyz[ 0 ] * newXyzScale;
			outXyz[ 1 ] = newXyz[ 1 ] * newXyzScale;
			outXyz[ 2 ] = newXyz[ 2 ] * newXyzScale;

			unsigned lat = ( newNormals[ 0 ] >> 8 ) & 0xff;
			unsigned lng = ( newNormals[ 0 ] & 0xff );
			lat *= ( FUNCTABLE_SIZE / 256 );
			lng *= ( FUNCTABLE_SIZE / 256 );

			// decode X as cos( lat ) * sin( long )
			// decode Y as sin( lat ) * sin( long )
			// decode Z as cos( long )

			outNormal[ 0 ] = tr.sinTable[ ( lat + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ] * tr.sinTable[ lng ];
			outNormal[ 1 ] = tr.sinTable[ lat ] * tr.sinTable[ lng ];
			outNormal[ 2 ] = tr.sinTable[ ( lng + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ];
		}
	} else {
		//
		// interpolate and copy the vertex and normal
		//
		short* oldXyz = ( short* )( ( byte* )surf + surf->ofsXyzNormals ) +
						( backEnd.currentEntity->e.oldframe * surf->numVerts * 4 );
		short* oldNormals = oldXyz + 3;

		float oldXyzScale = MD3_XYZ_SCALE * backlerp;
		float oldNormalScale = backlerp;

		for ( int vertNum = 0; vertNum < numVerts; vertNum++,
			  oldXyz += 4, newXyz += 4, oldNormals += 4, newNormals += 4,
			  outXyz += 4, outNormal += 4 ) {
			// interpolate the xyz
			outXyz[ 0 ] = oldXyz[ 0 ] * oldXyzScale + newXyz[ 0 ] * newXyzScale;
			outXyz[ 1 ] = oldXyz[ 1 ] * oldXyzScale + newXyz[ 1 ] * newXyzScale;
			outXyz[ 2 ] = oldXyz[ 2 ] * oldXyzScale + newXyz[ 2 ] * newXyzScale;

			// FIXME: interpolate lat/long instead?
			if ( GGameType & GAME_ET ) {
				// ydnar: ok :)
				unsigned lat = idMath::FtoiFast( ( ( ( oldNormals[ 0 ] >> 8 ) & 0xFF ) * ( FUNCTABLE_SIZE / 256 ) * newNormalScale ) +
					( ( ( oldNormals[ 0 ] >> 8 ) & 0xFF ) * ( FUNCTABLE_SIZE / 256 ) * oldNormalScale ) );
				unsigned lng = idMath::FtoiFast( ( ( oldNormals[ 0 ] & 0xFF ) * ( FUNCTABLE_SIZE / 256 ) * newNormalScale ) +
					( ( oldNormals[ 0 ] & 0xFF ) * ( FUNCTABLE_SIZE / 256 ) * oldNormalScale ) );

				outNormal[ 0 ] = tr.sinTable[ ( lat + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ] * tr.sinTable[ lng ];
				outNormal[ 1 ] = tr.sinTable[ lat ] * tr.sinTable[ lng ];
				outNormal[ 2 ] = tr.sinTable[ ( lng + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ];
			} else {
				unsigned lat = ( newNormals[ 0 ] >> 8 ) & 0xff;
				unsigned lng = ( newNormals[ 0 ] & 0xff );
				lat *= ( FUNCTABLE_SIZE / 256 );
				lng *= ( FUNCTABLE_SIZE / 256 );
				vec3_t uncompressedNewNormal;
				uncompressedNewNormal[ 0 ] = tr.sinTable[ ( lat + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ] * tr.sinTable[ lng ];
				uncompressedNewNormal[ 1 ] = tr.sinTable[ lat ] * tr.sinTable[ lng ];
				uncompressedNewNormal[ 2 ] = tr.sinTable[ ( lng + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ];

				lat = ( oldNormals[ 0 ] >> 8 ) & 0xff;
				lng = ( oldNormals[ 0 ] & 0xff );
				lat *= ( FUNCTABLE_SIZE / 256 );
				lng *= ( FUNCTABLE_SIZE / 256 );

				vec3_t uncompressedOldNormal;
				uncompressedOldNormal[ 0 ] = tr.sinTable[ ( lat + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ] * tr.sinTable[ lng ];
				uncompressedOldNormal[ 1 ] = tr.sinTable[ lat ] * tr.sinTable[ lng ];
				uncompressedOldNormal[ 2 ] = tr.sinTable[ ( lng + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ];

				outNormal[ 0 ] = uncompressedOldNormal[ 0 ] * oldNormalScale + uncompressedNewNormal[ 0 ] * newNormalScale;
				outNormal[ 1 ] = uncompressedOldNormal[ 1 ] * oldNormalScale + uncompressedNewNormal[ 1 ] * newNormalScale;
				outNormal[ 2 ] = uncompressedOldNormal[ 2 ] * oldNormalScale + uncompressedNewNormal[ 2 ] * newNormalScale;
			}
		}
		// ydnar: unecessary because of lat/lng lerping
		if ( !( GGameType & GAME_ET ) ) {
			VectorArrayNormalize( ( vec4_t* )tess.normal[ tess.numVertexes ], numVerts );
		}
	}
}

void RB_SurfaceMesh( md3Surface_t* surface ) {
	// RF, check for REFLAG_HANDONLY
	if ( backEnd.currentEntity->e.reFlags & REFLAG_ONLYHAND ) {
		if ( !strstr( surface->name, "hand" ) ) {
			return;
		}
	}

	float backlerp;
	if ( backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame ) {
		backlerp = 0;
	} else {
		backlerp = backEnd.currentEntity->e.backlerp;
	}

	RB_CHECKOVERFLOW( surface->numVerts, surface->numTriangles * 3 );

	LerpMeshVertexes( surface, backlerp );

	int* triangles = ( int* )( ( byte* )surface + surface->ofsTriangles );
	int indexes = surface->numTriangles * 3;
	int Bob = tess.numIndexes;
	int Doug = tess.numVertexes;
	for ( int j = 0; j < indexes; j++ ) {
		tess.indexes[ Bob + j ] = Doug + triangles[ j ];
	}
	tess.numIndexes += indexes;

	float* texCoords = ( float* )( ( byte* )surface + surface->ofsSt );

	int numVerts = surface->numVerts;
	for ( int j = 0; j < numVerts; j++ ) {
		tess.texCoords[ Doug + j ][ 0 ][ 0 ] = texCoords[ j * 2 + 0 ];
		tess.texCoords[ Doug + j ][ 0 ][ 1 ] = texCoords[ j * 2 + 1 ];
		// FIXME: fill in lightmapST for completeness?
	}

	tess.numVertexes += surface->numVerts;
}
