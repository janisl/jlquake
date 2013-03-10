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

#include "local.h"
#include "../../common/Common.h"
#include "../../common/common_defs.h"
#include "../../common/content_types.h"

static surfaceType_t q2SkySurface = SF_SKYBOX_Q2;

static bool R_CullTriSurf( srfTriangles_t* cv ) {
	int boxCull = R_CullLocalBox( cv->bounds );

	return boxCull == CULL_OUT;
}

//	Returns true if the grid is completely culled away.
//	Also sets the clipped hint bit in tess
static bool R_CullGrid( srfGridMesh_t* cv ) {
	if ( r_nocurves->integer ) {
		return true;
	}

	int sphereCull;
	if ( tr.currentEntityNum != REF_ENTITYNUM_WORLD ) {
		sphereCull = R_CullLocalPointAndRadius( cv->localOrigin, cv->radius );
	} else {
		sphereCull = R_CullPointAndRadius( cv->localOrigin, cv->radius );
	}

	// check for trivial reject
	if ( sphereCull == CULL_OUT ) {
		tr.pc.c_sphere_cull_patch_out++;
		return true;
	}
	// check bounding box if necessary
	else if ( sphereCull == CULL_CLIP ) {
		tr.pc.c_sphere_cull_patch_clip++;

		int boxCull = R_CullLocalBox( cv->bounds );

		if ( boxCull == CULL_OUT ) {
			tr.pc.c_box_cull_patch_out++;
			return true;
		} else if ( boxCull == CULL_IN ) {
			tr.pc.c_box_cull_patch_in++;
		} else {
			tr.pc.c_box_cull_patch_clip++;
		}
	} else {
		tr.pc.c_sphere_cull_patch_in++;
	}

	return false;
}

static bool R_CullSurfaceET( surfaceType_t* surface, shader_t* shader, int* frontFace ) {
	// force to non-front facing
	*frontFace = 0;

	if ( r_nocull->integer ) {
		return false;
	}

	// ydnar: made surface culling generic, inline with q3map2 surface classification
	switch ( *surface ) {
	case SF_FACE:
	case SF_TRIANGLES:
		break;
	case SF_GRID:
		if ( r_nocurves->integer ) {
			return true;
		}
		break;
	case SF_FOLIAGE:
		if ( !r_drawfoliage->value ) {
			return true;
		}
		break;

	default:
		return true;
	}

	// get generic surface
	srfGeneric_t* gen = ( srfGeneric_t* )surface;

	// plane cull
	if ( gen->plane.type != PLANE_NON_PLANAR && r_facePlaneCull->integer ) {
		float d = DotProduct( tr.orient.viewOrigin, gen->plane.normal ) - gen->plane.dist;
		if ( d > 0.0f ) {
			*frontFace = 1;
		}

		// don't cull exactly on the plane, because there are levels of rounding
		// through the BSP, ICD, and hardware that may cause pixel gaps if an
		// epsilon isn't allowed here
		if ( shader->cullType == CT_FRONT_SIDED ) {
			if ( d < -8.0f ) {
				tr.pc.c_plane_cull_out++;
				return true;
			}
		} else if ( shader->cullType == CT_BACK_SIDED ) {
			if ( d > 8.0f ) {
				tr.pc.c_plane_cull_out++;
				return true;
			}
		}

		tr.pc.c_plane_cull_in++;
	}

	// try sphere cull
	int cull;
	if ( tr.currentEntityNum != REF_ENTITYNUM_WORLD ) {
		cull = R_CullLocalPointAndRadius( gen->localOrigin, gen->radius );
	} else {
		cull = R_CullPointAndRadius( gen->localOrigin, gen->radius );
	}
	if ( cull == CULL_OUT ) {
		tr.pc.c_sphere_cull_out++;
		return true;
	}

	tr.pc.c_sphere_cull_in++;

	// must be visible
	return false;
}

//	Tries to back face cull surfaces before they are lighted or added to the
// sorting list.
//
//	This will also allow mirrors on both sides of a model without recursion.
static bool R_CullSurface( surfaceType_t* surface, shader_t* shader, int* frontFace ) {
	if ( GGameType & GAME_ET ) {
		return R_CullSurfaceET( surface, shader, frontFace );
	}

	// force to non-front facing
	*frontFace = 0;

	if ( r_nocull->integer ) {
		return false;
	}

	if ( *surface == SF_GRID ) {
		return R_CullGrid( ( srfGridMesh_t* )surface );
	}

	if ( *surface == SF_TRIANGLES ) {
		return R_CullTriSurf( ( srfTriangles_t* )surface );
	}

	if ( *surface != SF_FACE ) {
		return false;
	}

	if ( shader->cullType == CT_TWO_SIDED ) {
		return false;
	}

	// face culling
	if ( !r_facePlaneCull->integer ) {
		return false;
	}

	srfSurfaceFace_t* sface = ( srfSurfaceFace_t* )surface;
	float d = DotProduct( tr.orient.viewOrigin, sface->plane.normal );

	// don't cull exactly on the plane, because there are levels of rounding
	// through the BSP, ICD, and hardware that may cause pixel gaps if an
	// epsilon isn't allowed here
	if ( shader->cullType == CT_FRONT_SIDED ) {
		if ( d < sface->plane.dist - 8 ) {
			return true;
		}
	} else {
		if ( d > sface->plane.dist + 8 ) {
			return true;
		}
	}

	return false;
}

static int R_DlightFace( srfSurfaceFace_t* face, int dlightBits ) {
	for ( int i = 0; i < tr.refdef.num_dlights; i++ ) {
		if ( !( dlightBits & ( 1 << i ) ) ) {
			continue;
		}
		dlight_t* dl = &tr.refdef.dlights[ i ];
		float d = DotProduct( dl->origin, face->plane.normal ) - face->plane.dist;
		if ( d < -dl->radius || d > dl->radius ) {
			// dlight doesn't reach the plane
			dlightBits &= ~( 1 << i );
		}
	}

	if ( !dlightBits ) {
		tr.pc.c_dlightSurfacesCulled++;
	}

	face->dlightBits[ tr.smpFrame ] = dlightBits;
	return dlightBits;
}

static int R_DlightGrid( srfGridMesh_t* grid, int dlightBits ) {
	for ( int i = 0; i < tr.refdef.num_dlights; i++ ) {
		if ( !( dlightBits & ( 1 << i ) ) ) {
			continue;
		}
		dlight_t* dl = &tr.refdef.dlights[ i ];
		if ( dl->origin[ 0 ] - dl->radius > grid->bounds[ 1 ][ 0 ] ||
			 dl->origin[ 0 ] + dl->radius <grid->bounds[ 0 ][ 0 ] ||
										   dl->origin[ 1 ] - dl->radius> grid->bounds[ 1 ][ 1 ] ||
			 dl->origin[ 1 ] + dl->radius <grid->bounds[ 0 ][ 1 ] ||
										   dl->origin[ 2 ] - dl->radius> grid->bounds[ 1 ][ 2 ] ||
			 dl->origin[ 2 ] + dl->radius < grid->bounds[ 0 ][ 2 ] ) {
			// dlight doesn't reach the bounds
			dlightBits &= ~( 1 << i );
		}
	}

	if ( !dlightBits ) {
		tr.pc.c_dlightSurfacesCulled++;
	}

	grid->dlightBits[ tr.smpFrame ] = dlightBits;
	return dlightBits;
}

static int R_DlightTrisurf( srfTriangles_t* surf, int dlightBits ) {
	// FIXME: more dlight culling to trisurfs...
	surf->dlightBits[ tr.smpFrame ] = dlightBits;
	return dlightBits;
}

// ydnar: made this use generic surface
static int R_DlightSurfaceET( mbrush46_surface_t* surface, int dlightBits ) {
	int i;
	vec3_t origin;
	float radius;
	srfGeneric_t* gen;


	// get generic surface
	gen = ( srfGeneric_t* )surface->data;

	// ydnar: made surface dlighting generic, inline with q3map2 surface classification
	switch ( ( surfaceType_t )*surface->data ) {
	case SF_FACE:
	case SF_TRIANGLES:
	case SF_GRID:
	case SF_FOLIAGE:
		break;

	default:
		gen->dlightBits[ tr.smpFrame ] = 0;
		return 0;
	}

	// debug code
	//%	gen->dlightBits[ tr.smpFrame ] = dlightBits;
	//%	return dlightBits;

	// try to cull out dlights
	for ( i = 0; i < tr.refdef.num_dlights; i++ ) {
		if ( !( dlightBits & ( 1 << i ) ) ) {
			continue;
		}

		// junior dlights don't affect world surfaces
		if ( tr.refdef.dlights[ i ].flags & REF_JUNIOR_DLIGHT ) {
			dlightBits &= ~( 1 << i );
			continue;
		}

		// lightning dlights affect all surfaces
		if ( tr.refdef.dlights[ i ].flags & REF_DIRECTED_DLIGHT ) {
			continue;
		}

		// test surface bounding sphere against dlight bounding sphere
		VectorCopy( tr.refdef.dlights[ i ].transformed, origin );
		radius = tr.refdef.dlights[ i ].radius;

		if ( ( gen->localOrigin[ 0 ] + gen->radius ) < ( origin[ 0 ] - radius ) ||
			 ( gen->localOrigin[ 0 ] - gen->radius ) > ( origin[ 0 ] + radius ) ||
			 ( gen->localOrigin[ 1 ] + gen->radius ) < ( origin[ 1 ] - radius ) ||
			 ( gen->localOrigin[ 1 ] - gen->radius ) > ( origin[ 1 ] + radius ) ||
			 ( gen->localOrigin[ 2 ] + gen->radius ) < ( origin[ 2 ] - radius ) ||
			 ( gen->localOrigin[ 2 ] - gen->radius ) > ( origin[ 2 ] + radius ) ) {
			dlightBits &= ~( 1 << i );
		}
	}

	// common->Printf( "Surf: 0x%08X dlightBits: 0x%08X\n", srf, dlightBits );

	// set counters
	if ( dlightBits == 0 ) {
		tr.pc.c_dlightSurfacesCulled++;
	} else {
		tr.pc.c_dlightSurfaces++;
	}

	// set surface dlight bits and return
	gen->dlightBits[ tr.smpFrame ] = dlightBits;
	return dlightBits;
}

//	The given surface is going to be drawn, and it touches a leaf that is
// touched by one or more dlights, so try to throw out more dlights if possible.
static int R_DlightSurface( mbrush46_surface_t* surf, int dlightBits ) {
	if ( GGameType & GAME_ET ) {
		return R_DlightSurfaceET( surf, dlightBits );
	}

	if ( *surf->data == SF_FACE ) {
		dlightBits = R_DlightFace( ( srfSurfaceFace_t* )surf->data, dlightBits );
	} else if ( *surf->data == SF_GRID ) {
		dlightBits = R_DlightGrid( ( srfGridMesh_t* )surf->data, dlightBits );
	} else if ( *surf->data == SF_TRIANGLES ) {
		dlightBits = R_DlightTrisurf( ( srfTriangles_t* )surf->data, dlightBits );
	} else {
		dlightBits = 0;
	}

	if ( dlightBits ) {
		tr.pc.c_dlightSurfaces++;
	}

	return dlightBits;
}

static void R_AddWorldSurface( mbrush46_surface_t* surf, shader_t* shader, int dlightBits, int decalBits ) {
	if ( surf->viewCount == tr.viewCount ) {
		return;		// already in this view
	}

	surf->viewCount = tr.viewCount;
	// FIXME: bmodel fog?

	// try to cull before dlighting or adding
	int frontFace;
	if ( R_CullSurface( surf->data, shader, &frontFace ) ) {
		return;
	}

	// check for dlighting
	if ( dlightBits ) {
		dlightBits = R_DlightSurface( surf, dlightBits );
		dlightBits = ( dlightBits != 0 );
	}

	// add decals
	if ( decalBits ) {
		// ydnar: project any decals
		for ( int i = 0; i < tr.refdef.numDecalProjectors; i++ ) {
			if ( decalBits & ( 1 << i ) ) {
				R_ProjectDecalOntoSurface( &tr.refdef.decalProjectors[ i ], surf, tr.currentBModel );
			}
		}
	}

	R_AddDrawSurf( surf->data, shader, surf->fogIndex, dlightBits, frontFace, ATI_TESS_NONE, 0 );
}

/*
=============================================================

    BRUSH MODELS

=============================================================
*/

void R_DrawBrushModelQ1( trRefEntity_t* e, int forcedSortIndex ) {
	if ( ( e->e.renderfx & RF_THIRD_PERSON ) && !tr.viewParms.isPortal ) {
		return;
	}

	model_t* clmodel = R_GetModelByHandle( e->e.hModel );

	if ( R_CullLocalBox( &clmodel->q1_mins ) == CULL_OUT ) {
		return;
	}

	mbrush29_surface_t* psurf = &clmodel->brush29_surfaces[ clmodel->brush29_firstmodelsurface ];

	// calculate dynamic lighting for bmodel if it's not an
	// instanced model
	if ( clmodel->brush29_firstmodelsurface != 0 ) {
		for ( int k = 0; k < tr.refdef.num_dlights; k++ ) {
			R_MarkLightsQ1( &tr.refdef.dlights[ k ], 1 << k,
				clmodel->brush29_nodes + clmodel->brush29_firstnode );
		}
	}

	Com_Memset( lightmap_polys, 0, sizeof ( lightmap_polys ) );

	//
	// draw texture
	//
	for ( int i = 0; i < clmodel->brush29_nummodelsurfaces; i++, psurf++ ) {
		// find which side of the node we are on
		cplane_t* pplane = psurf->plane;

		float dot = DotProduct( tr.orient.viewOrigin, pplane->normal ) - pplane->dist;

		// draw the polygon
		if ( ( ( psurf->flags & BRUSH29_SURF_PLANEBACK ) && ( dot < -BACKFACE_EPSILON ) ) ||
			 ( !( psurf->flags & BRUSH29_SURF_PLANEBACK ) && ( dot > BACKFACE_EPSILON ) ) ) {
			R_AddDrawSurf( (surfaceType_t* )psurf, tr.defaultShader, 0, false, false, ATI_TESS_NONE, forcedSortIndex );
		}
	}
}

static void R_DrawInlineBModel(int forcedSortIndex) {
	// calculate dynamic lighting for bmodel
	dlight_t* lt = tr.refdef.dlights;
	for ( int k = 0; k < tr.refdef.num_dlights; k++, lt++ ) {
		R_MarkLightsQ2( lt, 1 << k, tr.currentModel->brush38_nodes + tr.currentModel->brush38_firstnode );
	}

	mbrush38_surface_t* psurf = &tr.currentModel->brush38_surfaces[ tr.currentModel->brush38_firstmodelsurface ];

	//
	// draw texture
	//
	for ( int i = 0; i < tr.currentModel->brush38_nummodelsurfaces; i++, psurf++ ) {
		// find which side of the node we are on
		cplane_t* pplane = psurf->plane;

		float dot = DotProduct( tr.orient.viewOrigin, pplane->normal ) - pplane->dist;

		// draw the polygon
		if ( ( ( psurf->flags & BRUSH38_SURF_PLANEBACK ) && ( dot < -BACKFACE_EPSILON ) ) ||
			 ( !( psurf->flags & BRUSH38_SURF_PLANEBACK ) && ( dot > BACKFACE_EPSILON ) ) ) {
			if ( psurf->texinfo->flags & ( BSP38SURF_TRANS33 | BSP38SURF_TRANS66 ) ) {
				// add to the translucent chain
				psurf->texturechain = r_alpha_surfaces;
				r_alpha_surfaces = psurf;
			} else {
				R_AddDrawSurf( ( surfaceType_t* )psurf, tr.defaultShader, 0, false, false, ATI_TESS_NONE, forcedSortIndex );
			}
		}
	}
}

void R_DrawBrushModelQ2( trRefEntity_t* e, int forcedSortIndex ) {
	if ( tr.currentModel->brush38_nummodelsurfaces == 0 ) {
		return;
	}

	if ( R_CullLocalBox( &tr.currentModel->q2_mins ) == CULL_OUT ) {
		return;
	}

	R_DrawInlineBModel(forcedSortIndex);
}

//	See if a sprite is inside a fog volume
// Return positive with /any part/ of the brush falling within a fog volume
static int R_BmodelFogNum( trRefEntity_t* re, mbrush46_model_t* bmodel ) {
	int i, j;
	mbrush46_fog_t* fog;

	for ( i = 1; i < tr.world->numfogs; i++ ) {
		fog = &tr.world->fogs[ i ];
		for ( j = 0; j < 3; j++ ) {
			if ( re->e.origin[ j ] + bmodel->bounds[ 0 ][ j ] >= fog->bounds[ 1 ][ j ] ) {
				break;
			}
			if ( re->e.origin[ j ] + bmodel->bounds[ 1 ][ j ] <= fog->bounds[ 0 ][ j ] ) {
				break;
			}
		}
		if ( j == 3 ) {
			return i;
		}
	}

	return 0;
}

void R_AddBrushModelSurfaces( trRefEntity_t* ent ) {
	model_t* pModel = R_GetModelByHandle( ent->e.hModel );

	mbrush46_model_t* bmodel = pModel->q3_bmodel;

	int clip = R_CullLocalBox( bmodel->bounds );
	if ( clip == CULL_OUT ) {
		return;
	}

	// ydnar: set current brush model to world
	tr.currentBModel = bmodel;

	// ydnar: set model state for decals and dynamic fog
	VectorCopy( ent->e.origin, bmodel->orientation[ tr.smpFrame ].origin );
	VectorCopy( ent->e.axis[ 0 ], bmodel->orientation[ tr.smpFrame ].axis[ 0 ] );
	VectorCopy( ent->e.axis[ 1 ], bmodel->orientation[ tr.smpFrame ].axis[ 1 ] );
	VectorCopy( ent->e.axis[ 2 ], bmodel->orientation[ tr.smpFrame ].axis[ 2 ] );
	bmodel->visible[ tr.smpFrame ] = true;
	bmodel->entityNum[ tr.smpFrame ] = tr.currentEntityNum;

	R_DlightBmodel( bmodel );

	// determine if in fog
	int fognum = R_BmodelFogNum( ent, bmodel );

	// ydnar: project any decals
	int decalBits = 0;
	decalProjector_t localProjectors[ MAX_DECAL_PROJECTORS ];
	int numLocalProjectors = 0;
	for ( int i = 0; i < tr.refdef.numDecalProjectors; i++ ) {
		// early out
		if ( tr.refdef.decalProjectors[ i ].shader == NULL ) {
			continue;
		}

		// transform entity bbox (fixme: rotated entities have invalid bounding boxes)
		vec3_t mins, maxs;
		VectorAdd( bmodel->bounds[ 0 ], tr.orient.origin, mins );
		VectorAdd( bmodel->bounds[ 1 ], tr.orient.origin, maxs );

		// set bit
		if ( R_TestDecalBoundingBox( &tr.refdef.decalProjectors[ i ], mins, maxs ) ) {
			R_TransformDecalProjector( &tr.refdef.decalProjectors[ i ], tr.orient.axis, tr.orient.origin, &localProjectors[ numLocalProjectors ] );
			numLocalProjectors++;
			decalBits <<= 1;
			decalBits |= 1;
		}
	}

	// ydnar: save old decal projectors
	int savedNumDecalProjectors = tr.refdef.numDecalProjectors;
	decalProjector_t* savedDecalProjectors = tr.refdef.decalProjectors;

	// set local decal projectors
	tr.refdef.numDecalProjectors = numLocalProjectors;
	tr.refdef.decalProjectors = localProjectors;

	// add model surfaces
	for ( int i = 0; i < bmodel->numSurfaces; i++ ) {
		if ( GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) ) {
			( bmodel->firstSurface + i )->fogIndex = fognum;
		}
		// Arnout: custom shader support for brushmodels
		if ( GGameType & ( GAME_WolfMP | GAME_ET ) && ent->e.customShader ) {
			R_AddWorldSurface( bmodel->firstSurface + i, R_GetShaderByHandle( ent->e.customShader ), tr.currentEntity->dlightBits, decalBits );
		} else {
			R_AddWorldSurface( bmodel->firstSurface + i, ( bmodel->firstSurface + i )->shader, tr.currentEntity->dlightBits, 0 );
		}
	}

	// ydnar: restore old decal projectors
	tr.refdef.numDecalProjectors = savedNumDecalProjectors;
	tr.refdef.decalProjectors = savedDecalProjectors;

	// ydnar: add decal surfaces
	R_AddDecalSurfaces( bmodel );

	// ydnar: clear current brush model
	tr.currentBModel = NULL;
}

/*
=============================================================

    WORLD MODEL

=============================================================
*/

static void R_RecursiveWorldNodeQ1( mbrush29_node_t* node ) {
	if ( node->contents == BSP29CONTENTS_SOLID ) {
		return;		// solid
	}

	if ( node->visframe != tr.visCount ) {
		return;
	}
	if ( R_CullLocalBox( ( vec3_t* )node->minmaxs ) == CULL_OUT ) {
		return;
	}

	// if a leaf node, draw stuff
	if ( node->contents < 0 ) {
		mbrush29_leaf_t* pleaf = ( mbrush29_leaf_t* )node;

		mbrush29_surface_t** mark = pleaf->firstmarksurface;
		int c = pleaf->nummarksurfaces;

		if ( c ) {
			do {
				( *mark )->visframe = tr.viewCount;
				mark++;
			} while ( --c );
		}

		return;
	}

	// node is just a decision point, so go down the apropriate sides

	// find which side of the node we are on
	cplane_t* plane = node->plane;

	double dot;
	switch ( plane->type ) {
	case PLANE_X:
		dot = tr.orient.viewOrigin[ 0 ] - plane->dist;
		break;
	case PLANE_Y:
		dot = tr.orient.viewOrigin[ 1 ] - plane->dist;
		break;
	case PLANE_Z:
		dot = tr.orient.viewOrigin[ 2 ] - plane->dist;
		break;
	default:
		dot = DotProduct( tr.orient.viewOrigin, plane->normal ) - plane->dist;
		break;
	}

	int side;
	if ( dot >= 0 ) {
		side = 0;
	} else {
		side = 1;
	}

	// recurse down the children, front side first
	R_RecursiveWorldNodeQ1( node->children[ side ] );

	// draw stuff
	int c = node->numsurfaces;

	if ( c ) {
		mbrush29_surface_t* surf = tr.worldModel->brush29_surfaces + node->firstsurface;

		if ( dot < 0 - BACKFACE_EPSILON ) {
			side = BRUSH29_SURF_PLANEBACK;
		} else if ( dot > BACKFACE_EPSILON ) {
			side = 0;
		}
		for (; c; c--, surf++ ) {
			if ( surf->visframe != tr.viewCount ) {
				continue;
			}

			// don't backface underwater surfaces, because they warp
			if ( !( ( ( r_viewleaf->contents == BSP29CONTENTS_EMPTY && ( surf->flags & BRUSH29_SURF_UNDERWATER ) ) ||
					  ( r_viewleaf->contents != BSP29CONTENTS_EMPTY && !( surf->flags & BRUSH29_SURF_UNDERWATER ) ) ) &&
					!( surf->flags & BRUSH29_SURF_DONTWARP ) ) && ( ( dot < 0 ) ^ !!( surf->flags & BRUSH29_SURF_PLANEBACK ) ) ) {
				continue;		// wrong side
			}

			// if sorting by texture, just store it out
			if ( surf->flags & BRUSH29_SURF_DRAWTURB ) {
				surf->texturechain = waterchain;
				waterchain = surf;
			} else {
				R_AddDrawSurf( ( surfaceType_t* )surf, tr.defaultShader, 0, false, false, ATI_TESS_NONE, 0 );
			}
		}
	}

	// recurse down the back side
	R_RecursiveWorldNodeQ1( node->children[ !side ] );
}

static void R_MarkLeavesQ1() {
	// current viewleaf
	r_oldviewleaf = r_viewleaf;
	r_viewleaf = Mod_PointInLeafQ1( tr.viewParms.orient.origin, tr.worldModel );

	if ( r_oldviewleaf == r_viewleaf && !r_novis->value ) {
		return;
	}

	tr.visCount++;
	r_oldviewleaf = r_viewleaf;

	byte* vis;
	byte solid[ 4096 ];
	if ( r_novis->value ) {
		vis = solid;
		Com_Memset( solid, 0xff, ( tr.worldModel->brush29_numleafs + 7 ) >> 3 );
	} else {
		vis = Mod_LeafPVS( r_viewleaf, tr.worldModel );
	}

	for ( int i = 0; i < tr.worldModel->brush29_numleafs; i++ ) {
		if ( vis[ i >> 3 ] & ( 1 << ( i & 7 ) ) ) {
			mbrush29_node_t* node = ( mbrush29_node_t* )&tr.worldModel->brush29_leafs[ i + 1 ];
			do {
				if ( node->visframe == tr.visCount ) {
					break;
				}
				node->visframe = tr.visCount;
				node = node->parent;
			} while ( node );
		}
	}
}

void R_DrawWorldQ1() {
	R_MarkLeavesQ1();

	VectorCopy( tr.viewParms.orient.origin, tr.orient.viewOrigin );

	tr.currentEntity = &tr.worldEntity;

	tr.currentEntityNum = REF_ENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

	Com_Memset( lightmap_polys, 0, sizeof ( lightmap_polys ) );

	R_RecursiveWorldNodeQ1( tr.worldModel->brush29_nodes );
}

static void R_RecursiveWorldNodeQ2( mbrush38_node_t* node ) {
	if ( node->contents == BSP38CONTENTS_SOLID ) {
		return;		// solid
	}

	if ( node->visframe != tr.visCount ) {
		return;
	}
	if ( R_CullLocalBox( ( vec3_t* )node->minmaxs ) == CULL_OUT ) {
		return;
	}

	// if a leaf node, draw stuff
	if ( node->contents != -1 ) {
		mbrush38_leaf_t* pleaf = ( mbrush38_leaf_t* )node;

		// check for door connected areas
		if ( tr.refdef.areamask[ pleaf->area >> 3 ] & ( 1 << ( pleaf->area & 7 ) ) ) {
			return;		// not visible
		}

		mbrush38_surface_t** mark = pleaf->firstmarksurface;
		int c = pleaf->nummarksurfaces;

		if ( c ) {
			do {
				( *mark )->visframe = tr.viewCount;
				mark++;
			} while ( --c );
		}

		return;
	}

	// node is just a decision point, so go down the apropriate sides

	// find which side of the node we are on
	cplane_t* plane = node->plane;

	float dot;
	switch ( plane->type ) {
	case PLANE_X:
		dot = tr.orient.viewOrigin[ 0 ] - plane->dist;
		break;
	case PLANE_Y:
		dot = tr.orient.viewOrigin[ 1 ] - plane->dist;
		break;
	case PLANE_Z:
		dot = tr.orient.viewOrigin[ 2 ] - plane->dist;
		break;
	default:
		dot = DotProduct( tr.orient.viewOrigin, plane->normal ) - plane->dist;
		break;
	}

	int side, sidebit;
	if ( dot >= 0 ) {
		side = 0;
		sidebit = 0;
	} else {
		side = 1;
		sidebit = BRUSH38_SURF_PLANEBACK;
	}

	// recurse down the children, front side first
	R_RecursiveWorldNodeQ2( node->children[ side ] );

	// draw stuff
	mbrush38_surface_t* surf = tr.worldModel->brush38_surfaces + node->firstsurface;
	for ( int c = node->numsurfaces; c; c--, surf++ ) {
		if ( surf->visframe != tr.viewCount ) {
			continue;
		}

		if ( ( surf->flags & BRUSH38_SURF_PLANEBACK ) != sidebit ) {
			continue;		// wrong side
		}

		if ( surf->texinfo->flags & BSP38SURF_SKY ) {
			// just adds to visible sky bounds
			R_AddSkySurface( surf );
		} else if ( surf->texinfo->flags & ( BSP38SURF_TRANS33 | BSP38SURF_TRANS66 ) ) {
			// add to the translucent chain
			surf->texturechain = r_alpha_surfaces;
			r_alpha_surfaces = surf;
		} else {
			R_AddDrawSurf( ( surfaceType_t* )surf, tr.defaultShader, 0, false, false, ATI_TESS_NONE, 0 );
		}
	}

	// recurse down the back side
	R_RecursiveWorldNodeQ2( node->children[ !side ] );
}

static void R_FindViewCluster() {
	r_oldviewcluster = r_viewcluster;
	r_oldviewcluster2 = r_viewcluster2;
	mbrush38_leaf_t* leaf = Mod_PointInLeafQ2( tr.viewParms.orient.origin, tr.worldModel );
	r_viewcluster = r_viewcluster2 = leaf->cluster;

	// check above and below so crossing solid water doesn't draw wrong
	if ( !leaf->contents ) {
		// look down a bit
		vec3_t temp;
		VectorCopy( tr.viewParms.orient.origin, temp );
		temp[ 2 ] -= 16;
		leaf = Mod_PointInLeafQ2( temp, tr.worldModel );
		if ( !( leaf->contents & BSP38CONTENTS_SOLID ) && ( leaf->cluster != r_viewcluster2 ) ) {
			r_viewcluster2 = leaf->cluster;
		}
	} else {
		// look up a bit
		vec3_t temp;
		VectorCopy( tr.viewParms.orient.origin, temp );
		temp[ 2 ] += 16;
		leaf = Mod_PointInLeafQ2( temp, tr.worldModel );
		if ( !( leaf->contents & BSP38CONTENTS_SOLID ) && ( leaf->cluster != r_viewcluster2 ) ) {
			r_viewcluster2 = leaf->cluster;
		}
	}
}

//	Mark the leaves and nodes that are in the PVS for the current cluster
static void R_MarkLeavesQ2() {
	// current viewcluster
	R_FindViewCluster();

	if ( r_oldviewcluster == r_viewcluster && r_oldviewcluster2 == r_viewcluster2 && !r_novis->value && r_viewcluster != -1 ) {
		return;
	}

	// development aid to let you run around and see exactly where
	// the pvs ends
	if ( r_lockpvs->value ) {
		return;
	}

	tr.visCount++;
	r_oldviewcluster = r_viewcluster;
	r_oldviewcluster2 = r_viewcluster2;

	if ( r_novis->value || r_viewcluster == -1 || !tr.worldModel->brush38_vis ) {
		// mark everything
		for ( int i = 0; i < tr.worldModel->brush38_numleafs; i++ ) {
			tr.worldModel->brush38_leafs[ i ].visframe = tr.visCount;
		}
		for ( int i = 0; i < tr.worldModel->brush38_numnodes; i++ ) {
			tr.worldModel->brush38_nodes[ i ].visframe = tr.visCount;
		}
		return;
	}

	byte* vis = Mod_ClusterPVS( r_viewcluster, tr.worldModel );
	byte fatvis[ BSP38MAX_MAP_LEAFS / 8 ];
	// may have to combine two clusters because of solid water boundaries
	if ( r_viewcluster2 != r_viewcluster ) {
		Com_Memcpy( fatvis, vis, ( tr.worldModel->brush38_numleafs + 7 ) / 8 );
		vis = Mod_ClusterPVS( r_viewcluster2, tr.worldModel );
		int c = ( tr.worldModel->brush38_numleafs + 31 ) / 32;
		for ( int i = 0; i < c; i++ ) {
			( ( int* )fatvis )[ i ] |= ( ( int* )vis )[ i ];
		}
		vis = fatvis;
	}

	mbrush38_leaf_t* leaf = tr.worldModel->brush38_leafs;
	for ( int i = 0; i < tr.worldModel->brush38_numleafs; i++, leaf++ ) {
		int cluster = leaf->cluster;
		if ( cluster == -1 ) {
			continue;
		}
		if ( vis[ cluster >> 3 ] & ( 1 << ( cluster & 7 ) ) ) {
			mbrush38_node_t* node = ( mbrush38_node_t* )leaf;
			do {
				if ( node->visframe == tr.visCount ) {
					break;
				}
				node->visframe = tr.visCount;
				node = node->parent;
			} while ( node );
		}
	}
}

void R_DrawWorldQ2() {
	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return;
	}

	R_MarkLeavesQ2();

	if ( !r_drawworld->value ) {
		return;
	}

	tr.currentModel = tr.worldModel;

	VectorCopy( tr.viewParms.orient.origin, tr.orient.viewOrigin );

	// auto cycle the world frame for texture animation
	tr.worldEntity.e.frame = ( int )( tr.refdef.floatTime * 2 );
	tr.currentEntity = &tr.worldEntity;

	tr.currentEntityNum = REF_ENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

	R_ClearSkyBox();

	R_RecursiveWorldNodeQ2( tr.worldModel->brush38_nodes );

	R_AddDrawSurf( &q2SkySurface, tr.defaultShader, 0, false, false, ATI_TESS_NONE, 0 );
}

static void R_AddLeafSurfacesQ3( mbrush46_node_t* node, int dlightBits, int decalBits ) {
	tr.pc.c_leafs++;

	// add to z buffer bounds
	if ( node->mins[ 0 ] < tr.viewParms.visBounds[ 0 ][ 0 ] ) {
		tr.viewParms.visBounds[ 0 ][ 0 ] = node->mins[ 0 ];
	}
	if ( node->mins[ 1 ] < tr.viewParms.visBounds[ 0 ][ 1 ] ) {
		tr.viewParms.visBounds[ 0 ][ 1 ] = node->mins[ 1 ];
	}
	if ( node->mins[ 2 ] < tr.viewParms.visBounds[ 0 ][ 2 ] ) {
		tr.viewParms.visBounds[ 0 ][ 2 ] = node->mins[ 2 ];
	}

	if ( node->maxs[ 0 ] > tr.viewParms.visBounds[ 1 ][ 0 ] ) {
		tr.viewParms.visBounds[ 1 ][ 0 ] = node->maxs[ 0 ];
	}
	if ( node->maxs[ 1 ] > tr.viewParms.visBounds[ 1 ][ 1 ] ) {
		tr.viewParms.visBounds[ 1 ][ 1 ] = node->maxs[ 1 ];
	}
	if ( node->maxs[ 2 ] > tr.viewParms.visBounds[ 1 ][ 2 ] ) {
		tr.viewParms.visBounds[ 1 ][ 2 ] = node->maxs[ 2 ];
	}

	// add the individual surfaces
	mbrush46_surface_t** mark = node->firstmarksurface;
	int c = node->nummarksurfaces;
	while ( c-- ) {
		// the surface may have already been added if it
		// spans multiple leafs
		mbrush46_surface_t* surf = *mark;
		R_AddWorldSurface( surf, surf->shader, dlightBits, decalBits );
		mark++;
	}
}

static void R_RecursiveWorldNodeQ3( mbrush46_node_t* node, int planeBits, int dlightBits, int decalBits ) {
	do {
		// if the node wasn't marked as potentially visible, exit
		if ( node->visframe != tr.visCount ) {
			return;
		}

		// if the bounding volume is outside the frustum, nothing
		// inside can be visible OPTIMIZE: don't do this all the way to leafs?

		if ( !r_nocull->integer ) {
			if ( planeBits & 1 ) {
				int r = BoxOnPlaneSide( node->mins, node->maxs, &tr.viewParms.frustum[ 0 ] );
				if ( r == 2 ) {
					return;						// culled
				}
				if ( r == 1 ) {
					planeBits &= ~1;			// all descendants will also be in front
				}
			}

			if ( planeBits & 2 ) {
				int r = BoxOnPlaneSide( node->mins, node->maxs, &tr.viewParms.frustum[ 1 ] );
				if ( r == 2 ) {
					return;						// culled
				}
				if ( r == 1 ) {
					planeBits &= ~2;			// all descendants will also be in front
				}
			}

			if ( planeBits & 4 ) {
				int r = BoxOnPlaneSide( node->mins, node->maxs, &tr.viewParms.frustum[ 2 ] );
				if ( r == 2 ) {
					return;						// culled
				}
				if ( r == 1 ) {
					planeBits &= ~4;			// all descendants will also be in front
				}
			}

			if ( planeBits & 8 ) {
				int r = BoxOnPlaneSide( node->mins, node->maxs, &tr.viewParms.frustum[ 3 ] );
				if ( r == 2 ) {
					return;						// culled
				}
				if ( r == 1 ) {
					planeBits &= ~8;			// all descendants will also be in front
				}
			}

			// ydnar: farplane culling
			if ( planeBits & 16 ) {
				int r = BoxOnPlaneSide( node->mins, node->maxs, &tr.viewParms.frustum[ 4 ] );
				if ( r == 2 ) {
					return;						// culled
				}
				if ( r == 1 ) {
					//JL WTF?
					planeBits &= ~8;			// all descendants will also be in front
				}
			}
		}

		if ( !( GGameType & GAME_ET ) && node->contents != -1 ) {
			break;
		}

		// node is just a decision point, so go down both sides
		// since we don't care about sort orders, just go positive to negative

		// determine which dlights are needed
		int newDlights[ 2 ];
		if ( GGameType & ( GAME_WolfSP | GAME_WolfMP ) ) {
			// RF, hack, dlight elimination above is unreliable
			newDlights[ 0 ] = 0xffffffff;
			newDlights[ 1 ] = 0xffffffff;
		} else if ( GGameType & GAME_ET ) {
			// ydnar: cull dlights
			if ( dlightBits ) {		//%	&& node->contents != -1 )
				for ( int i = 0; i < tr.refdef.num_dlights; i++ ) {
					if ( dlightBits & ( 1 << i ) ) {
						// directional dlights don't get culled
						if ( tr.refdef.dlights[ i ].flags & REF_DIRECTED_DLIGHT ) {
							continue;
						}

						// test dlight bounds against node surface bounds
						dlight_t* dl = &tr.refdef.dlights[ i ];
						if ( node->surfMins[ 0 ] >= ( dl->origin[ 0 ] + dl->radius ) || node->surfMaxs[ 0 ] <= ( dl->origin[ 0 ] - dl->radius ) ||
							 node->surfMins[ 1 ] >= ( dl->origin[ 1 ] + dl->radius ) || node->surfMaxs[ 1 ] <= ( dl->origin[ 1 ] - dl->radius ) ||
							 node->surfMins[ 2 ] >= ( dl->origin[ 2 ] + dl->radius ) || node->surfMaxs[ 2 ] <= ( dl->origin[ 2 ] - dl->radius ) ) {
							dlightBits &= ~( 1 << i );
						}
					}
				}
			}
			newDlights[ 0 ] = dlightBits;
			newDlights[ 1 ] = dlightBits;
		} else {
			newDlights[ 0 ] = 0;
			newDlights[ 1 ] = 0;
			if ( dlightBits ) {
				for ( int i = 0; i < tr.refdef.num_dlights; i++ ) {
					if ( dlightBits & ( 1 << i ) ) {
						dlight_t* dl = &tr.refdef.dlights[ i ];
						float dist = DotProduct( dl->origin, node->plane->normal ) - node->plane->dist;

						if ( dist > -dl->radius ) {
							newDlights[ 0 ] |= ( 1 << i );
						}
						if ( dist < dl->radius ) {
							newDlights[ 1 ] |= ( 1 << i );
						}
					}
				}
			}
		}

		// ydnar: cull decals
		if ( decalBits ) {
			for ( int i = 0; i < tr.refdef.numDecalProjectors; i++ ) {
				if ( decalBits & ( 1 << i ) ) {
					// test decal bounds against node surface bounds
					if ( tr.refdef.decalProjectors[ i ].shader == NULL ||
						 !R_TestDecalBoundingBox( &tr.refdef.decalProjectors[ i ], node->surfMins, node->surfMaxs ) ) {
						decalBits &= ~( 1 << i );
					}
				}
			}
		}

		// handle leaf nodes
		if ( node->contents != -1 ) {
			break;
		}

		// recurse down the children, front side first
		R_RecursiveWorldNodeQ3( node->children[ 0 ], planeBits, newDlights[ 0 ], decalBits );

		// tail recurse
		node = node->children[ 1 ];
		dlightBits = newDlights[ 1 ];
	} while ( 1 );

	// short circuit
	if ( node->nummarksurfaces == 0 ) {
		return;
	}

	R_AddLeafSurfacesQ3( node, dlightBits, decalBits );
}

//	Mark the leaves and nodes that are in the PVS for the current cluster
static void R_MarkLeavesQ3() {
	// lockpvs lets designers walk around to determine the
	// extent of the current pvs
	if ( r_lockpvs->integer ) {
		return;
	}

	// current viewcluster
	mbrush46_node_t* leaf = R_PointInLeaf( tr.viewParms.pvsOrigin );
	int cluster = leaf->cluster;

	// if the cluster is the same and the area visibility matrix
	// hasn't changed, we don't need to mark everything again

	// if r_showcluster was just turned on, remark everything
	if ( tr.viewCluster == cluster && !tr.refdef.areamaskModified && !r_showcluster->modified ) {
		return;
	}

	if ( r_showcluster->modified || r_showcluster->integer ) {
		r_showcluster->modified = false;
		if ( r_showcluster->integer ) {
			common->Printf( "cluster:%i  area:%i\n", cluster, leaf->area );
		}
	}

	tr.visCount++;
	tr.viewCluster = cluster;

	if ( r_novis->integer || tr.viewCluster == -1 ) {
		for ( int i = 0; i < tr.world->numnodes; i++ ) {
			if ( tr.world->nodes[ i ].contents != BSP46CONTENTS_SOLID ) {
				tr.world->nodes[ i ].visframe = tr.visCount;
			}
		}
		return;
	}

	const byte* vis = R_ClusterPVS( tr.viewCluster );

	leaf = tr.world->nodes;
	for ( int i = 0; i < tr.world->numnodes; i++, leaf++ ) {
		cluster = leaf->cluster;
		if ( cluster < 0 || cluster >= tr.world->numClusters ) {
			continue;
		}

		// check general pvs
		if ( !( vis[ cluster >> 3 ] & ( 1 << ( cluster & 7 ) ) ) ) {
			continue;
		}

		// check for door connection
		if ( ( tr.refdef.areamask[ leaf->area >> 3 ] & ( 1 << ( leaf->area & 7 ) ) ) ) {
			continue;		// not visible
		}

		// ydnar: don't want to walk the entire bsp to add skybox surfaces
		if ( GGameType & GAME_ET && tr.refdef.rdflags & RDF_SKYBOXPORTAL ) {
			// this only happens once, as game/cgame know the origin of the skybox
			// this also means the skybox portal cannot move, as this list is calculated once and never again
			if ( tr.world->numSkyNodes < WORLD_MAX_SKY_NODES ) {
				tr.world->skyNodes[ tr.world->numSkyNodes++ ] = leaf;
			}
			R_AddLeafSurfacesQ3( leaf, 0, 0 );
			continue;
		}

		mbrush46_node_t* parent = leaf;
		do {
			if ( parent->visframe == tr.visCount ) {
				break;
			}
			parent->visframe = tr.visCount;
			parent = parent->parent;
		} while ( parent );
	}
}

void R_AddWorldSurfaces() {
	if ( !r_drawworld->integer ) {
		return;
	}

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return;
	}

	tr.currentEntityNum = REF_ENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

	// ydnar: set current brush model to world
	tr.currentBModel = &tr.world->bmodels[ 0 ];

	// clear out the visible min/max
	ClearBounds( tr.viewParms.visBounds[ 0 ], tr.viewParms.visBounds[ 1 ] );

	// render sky or world?
	if ( GGameType & GAME_ET && tr.refdef.rdflags & RDF_SKYBOXPORTAL && tr.world->numSkyNodes > 0 ) {
		mbrush46_node_t** node = tr.world->skyNodes;
		for ( int i = 0; i < tr.world->numSkyNodes; i++, node++ ) {
			R_AddLeafSurfacesQ3( *node, tr.refdef.dlightBits, 0 );			// no decals on skybox nodes
		}
	} else {
		// determine which leaves are in the PVS / areamask
		R_MarkLeavesQ3();

		// perform frustum culling and add all the potentially visible surfaces
		if ( GGameType & GAME_ET ) {
			R_RecursiveWorldNodeQ3( tr.world->nodes, 31, tr.refdef.dlightBits, tr.refdef.decalBits );
		} else {
			R_RecursiveWorldNodeQ3( tr.world->nodes, 15, ( 1 << tr.refdef.num_dlights ) - 1, 0 );
		}

		// ydnar: add decal surfaces
		R_AddDecalSurfaces( tr.world->bmodels );
	}

	// clear brush model
	tr.currentBModel = NULL;
}
