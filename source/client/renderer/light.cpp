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

#define DLIGHT_AT_RADIUS        16
// at the edge of a dlight's influence, this amount of light will be added

#define DLIGHT_MINIMUM_RADIUS   16
// never calculate a range less than this to prevent huge light numbers

vec3_t lightspot;

static vec3_t pointcolor;

/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

static int RecursiveLightPointQ1( mbrush29_node_t* node, vec3_t start, vec3_t end ) {
	if ( node->contents < 0 ) {
		return -1;		// didn't hit anything
	}

	// calculate mid point

	// FIXME: optimize for axial
	cplane_t* plane = node->plane;
	float front = DotProduct( start, plane->normal ) - plane->dist;
	float back = DotProduct( end, plane->normal ) - plane->dist;
	int side = front < 0;

	if ( ( back < 0 ) == side ) {
		return RecursiveLightPointQ1( node->children[ side ], start, end );
	}

	float frac = front / ( front - back );
	vec3_t mid;
	mid[ 0 ] = start[ 0 ] + ( end[ 0 ] - start[ 0 ] ) * frac;
	mid[ 1 ] = start[ 1 ] + ( end[ 1 ] - start[ 1 ] ) * frac;
	mid[ 2 ] = start[ 2 ] + ( end[ 2 ] - start[ 2 ] ) * frac;

	// go down front side
	int r = RecursiveLightPointQ1( node->children[ side ], start, mid );
	if ( r >= 0 ) {
		return r;		// hit something
	}

	if ( ( back < 0 ) == side ) {
		return -1;		// didn't hit anuthing
	}

	// check for impact on this node
	VectorCopy( mid, lightspot );

	mbrush29_surface_t* surf = tr.worldModel->brush29_surfaces + node->firstsurface;
	for ( int i = 0; i < node->numsurfaces; i++, surf++ ) {
		if ( surf->flags & BRUSH29_SURF_DRAWTILED ) {
			continue;	// no lightmaps
		}

		mbrush29_texinfo_t* tex = surf->texinfo;

		int s = DotProduct( mid, tex->vecs[ 0 ] ) + tex->vecs[ 0 ][ 3 ];
		int t = DotProduct( mid, tex->vecs[ 1 ] ) + tex->vecs[ 1 ][ 3 ];;

		if ( s < surf->texturemins[ 0 ] || t < surf->texturemins[ 1 ] ) {
			continue;
		}

		int ds = s - surf->texturemins[ 0 ];
		int dt = t - surf->texturemins[ 1 ];

		if ( ds > surf->extents[ 0 ] || dt > surf->extents[ 1 ] ) {
			continue;
		}

		if ( !surf->samples ) {
			return 0;
		}

		ds >>= 4;
		dt >>= 4;

		byte* lightmap = surf->samples;
		r = 0;
		if ( lightmap ) {

			lightmap += dt * ( ( surf->extents[ 0 ] >> 4 ) + 1 ) + ds;

			for ( int maps = 0; maps < BSP29_MAXLIGHTMAPS && surf->styles[ maps ] != 255; maps++ ) {
				r += *lightmap * tr.refdef.lightstyles[ surf->styles[ maps ] ].rgb[ 0 ];
				lightmap += ( ( surf->extents[ 0 ] >> 4 ) + 1 ) * ( ( surf->extents[ 1 ] >> 4 ) + 1 );
			}
		}

		return r;
	}

	// go down back side
	return RecursiveLightPointQ1( node->children[ !side ], mid, end );
}

int R_LightPointQ1( vec3_t p ) {
	if ( !tr.worldModel->brush29_lightdata ) {
		return 255;
	}

	vec3_t end;
	end[ 0 ] = p[ 0 ];
	end[ 1 ] = p[ 1 ];
	end[ 2 ] = p[ 2 ] - 2048;

	int r = RecursiveLightPointQ1( tr.worldModel->brush29_nodes, p, end );

	if ( r == -1 ) {
		r = 0;
	}

	return r;
}

static int RecursiveLightPointQ2( mbrush38_node_t* node, vec3_t start, vec3_t end ) {
	if ( node->contents != -1 ) {
		return -1;		// didn't hit anything
	}

	// calculate mid point

	// FIXME: optimize for axial
	cplane_t* plane = node->plane;
	float front = DotProduct( start, plane->normal ) - plane->dist;
	float back = DotProduct( end, plane->normal ) - plane->dist;
	int side = front < 0;

	if ( ( back < 0 ) == side ) {
		return RecursiveLightPointQ2( node->children[ side ], start, end );
	}

	float frac = front / ( front - back );
	vec3_t mid;
	mid[ 0 ] = start[ 0 ] + ( end[ 0 ] - start[ 0 ] ) * frac;
	mid[ 1 ] = start[ 1 ] + ( end[ 1 ] - start[ 1 ] ) * frac;
	mid[ 2 ] = start[ 2 ] + ( end[ 2 ] - start[ 2 ] ) * frac;

	// go down front side
	int r = RecursiveLightPointQ2( node->children[ side ], start, mid );
	if ( r >= 0 ) {
		return r;		// hit something
	}

	if ( ( back < 0 ) == side ) {
		return -1;		// didn't hit anuthing
	}

	// check for impact on this node
	VectorCopy( mid, lightspot );

	mbrush38_surface_t* surf = tr.worldModel->brush38_surfaces + node->firstsurface;
	for ( int i = 0; i < node->numsurfaces; i++, surf++ ) {
		if ( surf->flags & ( BRUSH38_SURF_DRAWTURB | BRUSH38_SURF_DRAWSKY ) ) {
			continue;	// no lightmaps
		}

		mbrush38_texinfo_t* tex = surf->texinfo;

		int s = ( int )( DotProduct( mid, tex->vecs[ 0 ] ) + tex->vecs[ 0 ][ 3 ] );
		int t = ( int )( DotProduct( mid, tex->vecs[ 1 ] ) + tex->vecs[ 1 ][ 3 ] );

		if ( s < surf->texturemins[ 0 ] || t < surf->texturemins[ 1 ] ) {
			continue;
		}

		int ds = s - surf->texturemins[ 0 ];
		int dt = t - surf->texturemins[ 1 ];

		if ( ds > surf->extents[ 0 ] || dt > surf->extents[ 1 ] ) {
			continue;
		}

		if ( !surf->samples ) {
			return 0;
		}

		ds >>= 4;
		dt >>= 4;

		byte* lightmap = surf->samples;
		VectorCopy( vec3_origin, pointcolor );
		if ( lightmap ) {
			vec3_t scale;

			lightmap += 3 * ( dt * ( ( surf->extents[ 0 ] >> 4 ) + 1 ) + ds );

			for ( int maps = 0; maps < BSP38_MAXLIGHTMAPS && surf->styles[ maps ] != 255; maps++ ) {
				for ( int j = 0; j < 3; j++ ) {
					scale[ j ] = r_modulate->value * tr.refdef.lightstyles[ surf->styles[ maps ] ].rgb[ j ];
				}

				pointcolor[ 0 ] += lightmap[ 0 ] * scale[ 0 ] * ( 1.0 / 255 );
				pointcolor[ 1 ] += lightmap[ 1 ] * scale[ 1 ] * ( 1.0 / 255 );
				pointcolor[ 2 ] += lightmap[ 2 ] * scale[ 2 ] * ( 1.0 / 255 );
				lightmap += 3 * ( ( surf->extents[ 0 ] >> 4 ) + 1 ) * ( ( surf->extents[ 1 ] >> 4 ) + 1 );
			}
		}
		return 1;
	}

	// go down back side
	return RecursiveLightPointQ2( node->children[ !side ], mid, end );
}

void R_LightPointQ2( vec3_t p, vec3_t color, trRefdef_t& refdef ) {
	if ( !tr.worldModel->brush38_lightdata ) {
		color[ 0 ] = color[ 1 ] = color[ 2 ] = 1.0;
		return;
	}

	vec3_t end;
	end[ 0 ] = p[ 0 ];
	end[ 1 ] = p[ 1 ];
	end[ 2 ] = p[ 2 ] - 2048;

	int r = RecursiveLightPointQ2( tr.worldModel->brush38_nodes, p, end );

	if ( r == -1 ) {
		VectorCopy( vec3_origin, color );
	} else   {
		VectorCopy( pointcolor, color );
	}

	//
	// add dynamic lights
	//
	dlight_t* dl = refdef.dlights;
	for ( int lnum = 0; lnum < refdef.num_dlights; lnum++, dl++ ) {
		vec3_t dist;
		VectorSubtract( p, dl->origin, dist );
		float add = dl->radius - VectorLength( dist );
		add *= ( 1.0 / 256 );
		if ( add > 0 ) {
			VectorMA( color, add, dl->color, color );
		}
	}

	VectorScale( color, r_modulate->value, color );
}

static void R_SetupEntityLightingGrid( trRefEntity_t* ent ) {
	vec3_t lightOrigin;
	if ( ent->e.renderfx & RF_LIGHTING_ORIGIN ) {
		// seperate lightOrigins are needed so an object that is
		// sinking into the ground can still be lit, and so
		// multi-part models can be lit identically
		VectorCopy( ent->e.lightingOrigin, lightOrigin );
	} else   {
		VectorCopy( ent->e.origin, lightOrigin );
	}

	VectorSubtract( lightOrigin, tr.world->lightGridOrigin, lightOrigin );
	int pos[ 3 ];
	float frac[ 3 ];
	for ( int i = 0; i < 3; i++ ) {
		float v = lightOrigin[ i ] * tr.world->lightGridInverseSize[ i ];
		pos[ i ] = floor( v );
		frac[ i ] = v - pos[ i ];
		if ( pos[ i ] < 0 ) {
			pos[ i ] = 0;
			frac[ i ] = 0;
		} else if ( pos[ i ] >= tr.world->lightGridBounds[ i ] - 1 )         {
			pos[ i ] = tr.world->lightGridBounds[ i ] - 1;
			frac[ i ] = 0;
		}
	}

	VectorClear( ent->ambientLight );
	VectorClear( ent->directedLight );
	vec3_t direction;
	VectorClear( direction );

	assert( tr.world->lightGridData );		// bk010103 - NULL with -nolight maps

	// trilerp the light value
	int gridStep[ 3 ];
	gridStep[ 0 ] = 8;
	gridStep[ 1 ] = 8 * tr.world->lightGridBounds[ 0 ];
	gridStep[ 2 ] = 8 * tr.world->lightGridBounds[ 0 ] * tr.world->lightGridBounds[ 1 ];
	byte* gridData = tr.world->lightGridData + pos[ 0 ] * gridStep[ 0 ] +
					 pos[ 1 ] * gridStep[ 1 ] + pos[ 2 ] * gridStep[ 2 ];
	int GridDataSize = tr.world->lightGridBounds[ 0 ] * tr.world->lightGridBounds[ 1 ] * tr.world->lightGridBounds[ 2 ] * 8;

	float totalFactor = 0;
	for ( int i = 0; i < 8; i++ ) {
		float factor = 1.0;
		byte* data = gridData;
		for ( int j = 0; j < 3; j++ ) {
			if ( i & ( 1 << j ) ) {
				factor *= frac[ j ];
				data += gridStep[ j ];
			} else   {
				factor *= ( 1.0f - frac[ j ] );
			}
		}
		if ( !factor ) {
			//	No effect from this point.
			continue;
		}

		if ( data >= tr.world->lightGridData + GridDataSize ) {
			//	Out of bounds. Shouldn't happen now.
			continue;
		}
		if ( !( data[ 0 ] + data[ 1 ] + data[ 2 ] ) ) {
			continue;	// ignore samples in walls
		}
		totalFactor += factor;
		ent->ambientLight[ 0 ] += factor * data[ 0 ];
		ent->ambientLight[ 1 ] += factor * data[ 1 ];
		ent->ambientLight[ 2 ] += factor * data[ 2 ];

		ent->directedLight[ 0 ] += factor * data[ 3 ];
		ent->directedLight[ 1 ] += factor * data[ 4 ];
		ent->directedLight[ 2 ] += factor * data[ 5 ];
		int lat = data[ 7 ];
		int lng = data[ 6 ];
		lat *= ( FUNCTABLE_SIZE / 256 );
		lng *= ( FUNCTABLE_SIZE / 256 );

		// decode X as cos( lat ) * sin( long )
		// decode Y as sin( lat ) * sin( long )
		// decode Z as cos( long )

		vec3_t normal;
		normal[ 0 ] = tr.sinTable[ ( lat + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ] * tr.sinTable[ lng ];
		normal[ 1 ] = tr.sinTable[ lat ] * tr.sinTable[ lng ];
		normal[ 2 ] = tr.sinTable[ ( lng + ( FUNCTABLE_SIZE  / 4 ) ) & FUNCTABLE_MASK ];

		VectorMA( direction, factor, normal, direction );
	}

	if ( totalFactor > 0 && totalFactor < 0.99 ) {
		totalFactor = 1.0f / totalFactor;
		VectorScale( ent->ambientLight, totalFactor, ent->ambientLight );
		VectorScale( ent->directedLight, totalFactor, ent->directedLight );
	}

	VectorScale( ent->ambientLight, r_ambientScale->value, ent->ambientLight );
	VectorScale( ent->directedLight, r_directedScale->value, ent->directedLight );

	//	cheats?  check for single player?
	if ( tr.lightGridMulDirected ) {
		VectorScale( ent->directedLight, tr.lightGridMulDirected, ent->directedLight );
	}
	if ( tr.lightGridMulAmbient ) {
		VectorScale( ent->ambientLight, tr.lightGridMulAmbient, ent->ambientLight );
	}

	VectorNormalize2( direction, ent->lightDir );
}

static void LogLight( trRefEntity_t* ent ) {
	if ( !( ent->e.renderfx & RF_FIRST_PERSON ) ) {
		return;
	}

	int max1 = ent->ambientLight[ 0 ];
	if ( ent->ambientLight[ 1 ] > max1 ) {
		max1 = ent->ambientLight[ 1 ];
	} else if ( ent->ambientLight[ 2 ] > max1 )       {
		max1 = ent->ambientLight[ 2 ];
	}

	int max2 = ent->directedLight[ 0 ];
	if ( ent->directedLight[ 1 ] > max2 ) {
		max2 = ent->directedLight[ 1 ];
	} else if ( ent->directedLight[ 2 ] > max2 )       {
		max2 = ent->directedLight[ 2 ];
	}

	common->Printf( "amb:%i  dir:%i\n", max1, max2 );
}

//	Calculates all the lighting values that will be used
// by the Calc_* functions
void R_SetupEntityLighting( const trRefdef_t* refdef, trRefEntity_t* ent ) {
	// lighting calculations
	if ( ent->lightingCalculated ) {
		return;
	}
	ent->lightingCalculated = true;

	//
	// trace a sample point down to find ambient light
	//
	vec3_t lightOrigin;
	if ( ent->e.renderfx & RF_LIGHTING_ORIGIN ) {
		// seperate lightOrigins are needed so an object that is
		// sinking into the ground can still be lit, and so
		// multi-part models can be lit identically
		VectorCopy( ent->e.lightingOrigin, lightOrigin );
	} else   {
		VectorCopy( ent->e.origin, lightOrigin );
	}

	// if NOWORLDMODEL, only use dynamic lights (menu system, etc)
	if ( tr.world && tr.world->lightGridData &&
		 ( !( refdef->rdflags & RDF_NOWORLDMODEL ) ||
		   ( GGameType & GAME_ET && ( refdef->rdflags & RDF_NOWORLDMODEL ) && ( ent->e.renderfx & RF_LIGHTING_ORIGIN ) ) ) ) {
		R_SetupEntityLightingGrid( ent );
	} else if ( GGameType & GAME_ET )     {
		ent->ambientLight[ 0 ] = tr.identityLight * 64;
		ent->ambientLight[ 1 ] = tr.identityLight * 64;
		ent->ambientLight[ 2 ] = tr.identityLight * 96;
		ent->directedLight[ 0 ] = tr.identityLight * 255;
		ent->directedLight[ 1 ] = tr.identityLight * 232;
		ent->directedLight[ 2 ] = tr.identityLight * 224;
		VectorSet( ent->lightDir, -1, 1, 1.25 );
		VectorNormalize( ent->lightDir );
	} else   {
		ent->ambientLight[ 0 ] = ent->ambientLight[ 1 ] =
									 ent->ambientLight[ 2 ] = tr.identityLight * 150;
		ent->directedLight[ 0 ] = ent->directedLight[ 1 ] =
									  ent->directedLight[ 2 ] = tr.identityLight * 150;
		VectorCopy( tr.sunDirection, ent->lightDir );
	}

	// bonus items and view weapons have a fixed minimum add
	if ( ent->e.hilightIntensity ) {
		// level of intensity was set because the item was looked at
		ent->ambientLight[ 0 ] += tr.identityLight * 128 * ent->e.hilightIntensity;
		ent->ambientLight[ 1 ] += tr.identityLight * 128 * ent->e.hilightIntensity;
		ent->ambientLight[ 2 ] += tr.identityLight * 128 * ent->e.hilightIntensity;
	} else if ( !( GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) ) || ent->e.renderfx & RF_MINLIGHT )         {
		// give everything a minimum light add
		ent->ambientLight[ 0 ] += tr.identityLight * 32;
		ent->ambientLight[ 1 ] += tr.identityLight * 32;
		ent->ambientLight[ 2 ] += tr.identityLight * 32;
	}

	if ( refdef->rdflags & RDF_SNOOPERVIEW &&
		 ( ( GGameType & GAME_WolfSP && ent->e.entityNum < MAX_CLIENTS_WS ) ||
		   ( GGameType & GAME_WolfMP && ent->e.entityNum < MAX_CLIENTS_WM ) ||
		   ( GGameType & GAME_ET && ent->e.entityNum < MAX_CLIENTS_ET ) ) ) {
		// allow a little room for flicker from directed light
		VectorSet( ent->ambientLight, 245, 245, 245 );
	}

	//
	// modify the light by dynamic lights
	//
	float d = VectorLength( ent->directedLight );
	vec3_t lightDir;
	VectorScale( ent->lightDir, d, lightDir );

	for ( int i = 0; i < refdef->num_dlights; i++ ) {
		dlight_t* dl = &refdef->dlights[ i ];
		if ( dl->shader ) {
			//	if the dlight has a diff shader specified, you don't know
			// what it does, so don't let it affect entities lighting
			continue;
		}

		float modulate;
		vec3_t dir;
		if ( !( GGameType & GAME_ET ) ) {
			VectorSubtract( dl->origin, lightOrigin, dir );
			d = VectorNormalize( dir );

			float power = DLIGHT_AT_RADIUS * ( dl->radius * dl->radius );
			if ( d < DLIGHT_MINIMUM_RADIUS ) {
				d = DLIGHT_MINIMUM_RADIUS;
			}
			modulate = power / ( d * d );
		} else   {
			// directional dlight, origin is a directional normal
			if ( dl->flags & REF_DIRECTED_DLIGHT ) {
				modulate = dl->intensity * 255.0;
				VectorCopy( dl->origin, dir );
			} else   {
				// ball dlight
				VectorSubtract( dl->origin, lightOrigin, dir );
				d = dl->radius - VectorNormalize( dir );
				if ( d <= 0.0 ) {
					modulate = 0;
				} else   {
					modulate = dl->intensity * d;
				}
			}
		}

		VectorMA( ent->directedLight, modulate, dl->color, ent->directedLight );
		VectorMA( lightDir, modulate, dir, lightDir );
	}

	// clamp ambient
	for ( int i = 0; i < 3; i++ ) {
		if ( ent->ambientLight[ i ] > tr.identityLightByte ) {
			ent->ambientLight[ i ] = tr.identityLightByte;
		}
	}

	if ( r_debugLight->integer ) {
		LogLight( ent );
	}

	// save out the byte packet version
	( ( byte* )&ent->ambientLightInt )[ 0 ] = idMath::FtoiFast( ent->ambientLight[ 0 ] );
	( ( byte* )&ent->ambientLightInt )[ 1 ] = idMath::FtoiFast( ent->ambientLight[ 1 ] );
	( ( byte* )&ent->ambientLightInt )[ 2 ] = idMath::FtoiFast( ent->ambientLight[ 2 ] );
	( ( byte* )&ent->ambientLightInt )[ 3 ] = 0xff;

	if ( GGameType & GAME_ET ) {
		// ydnar: save out the light table
		d = 0.0f;
		byte* entityLight = ( byte* )ent->entityLightInt;
		float modulate = 1.0f / ( ENTITY_LIGHT_STEPS - 1 );
		for ( int i = 0; i < ENTITY_LIGHT_STEPS; i++ ) {
			vec3_t lightValue;
			VectorMA( ent->ambientLight, d, ent->directedLight, lightValue );
			entityLight[ 0 ] = lightValue[ 0 ] > 255.0f ? 255 : idMath::FtoiFast( lightValue[ 0 ] );
			entityLight[ 1 ] = lightValue[ 1 ] > 255.0f ? 255 : idMath::FtoiFast( lightValue[ 1 ] );
			entityLight[ 2 ] = lightValue[ 2 ] > 255.0f ? 255 : idMath::FtoiFast( lightValue[ 2 ] );
			entityLight[ 3 ] = 0xFF;

			d += modulate;
			entityLight += 4;
		}
	}

	// transform the direction to local space
	VectorNormalize( lightDir );
	ent->lightDir[ 0 ] = DotProduct( lightDir, ent->e.axis[ 0 ] );
	ent->lightDir[ 1 ] = DotProduct( lightDir, ent->e.axis[ 1 ] );
	ent->lightDir[ 2 ] = DotProduct( lightDir, ent->e.axis[ 2 ] );

	// ydnar: renormalize if necessary
	if ( GGameType & GAME_ET && ent->e.nonNormalizedAxes ) {
		VectorNormalize( ent->lightDir );
	}
}

int R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir ) {
	// bk010103 - this segfaults with -nolight maps
	if ( tr.world->lightGridData == NULL ) {
		return false;
	}

	trRefEntity_t ent;
	Com_Memset( &ent, 0, sizeof ( ent ) );
	VectorCopy( point, ent.e.origin );
	R_SetupEntityLightingGrid( &ent );
	VectorCopy( ent.ambientLight, ambientLight );
	VectorCopy( ent.directedLight, directedLight );
	VectorCopy( ent.lightDir, lightDir );

	return true;
}

/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/

void R_MarkLightsQ1( dlight_t* light, int bit, mbrush29_node_t* node ) {
	if ( node->contents < 0 ) {
		return;
	}

	cplane_t* splitplane = node->plane;
	float dist = DotProduct( light->origin, splitplane->normal ) - splitplane->dist;

	if ( dist > light->radius ) {
		R_MarkLightsQ1( light, bit, node->children[ 0 ] );
		return;
	}
	if ( dist < -light->radius ) {
		R_MarkLightsQ1( light, bit, node->children[ 1 ] );
		return;
	}

	// mark the polygons
	mbrush29_surface_t* surf = tr.worldModel->brush29_surfaces + node->firstsurface;
	for ( int i = 0; i < node->numsurfaces; i++, surf++ ) {
		if ( surf->dlightframe != tr.frameCount ) {
			surf->dlightbits = 0;
			surf->dlightframe = tr.frameCount;
		}
		surf->dlightbits |= bit;
	}

	R_MarkLightsQ1( light, bit, node->children[ 0 ] );
	R_MarkLightsQ1( light, bit, node->children[ 1 ] );
}

void R_PushDlightsQ1() {
	dlight_t* l = tr.refdef.dlights;

	for ( int i = 0; i < tr.refdef.num_dlights; i++, l++ ) {
		R_MarkLightsQ1( l, 1 << i, tr.worldModel->brush29_nodes );
	}
}

void R_MarkLightsQ2( dlight_t* light, int bit, mbrush38_node_t* node ) {
	if ( node->contents != -1 ) {
		return;
	}

	cplane_t* splitplane = node->plane;
	float dist = DotProduct( light->origin, splitplane->normal ) - splitplane->dist;

	if ( dist > light->radius - DLIGHT_CUTOFF ) {
		R_MarkLightsQ2( light, bit, node->children[ 0 ] );
		return;
	}
	if ( dist < -light->radius + DLIGHT_CUTOFF ) {
		R_MarkLightsQ2( light, bit, node->children[ 1 ] );
		return;
	}

	// mark the polygons
	mbrush38_surface_t* surf = tr.worldModel->brush38_surfaces + node->firstsurface;
	for ( int i = 0; i < node->numsurfaces; i++, surf++ ) {
		if ( surf->dlightframe != tr.frameCount ) {
			surf->dlightbits = 0;
			surf->dlightframe = tr.frameCount;
		}
		surf->dlightbits |= bit;
	}

	R_MarkLightsQ2( light, bit, node->children[ 0 ] );
	R_MarkLightsQ2( light, bit, node->children[ 1 ] );
}

void R_PushDlightsQ2() {
	dlight_t* l = tr.refdef.dlights;
	for ( int i = 0; i < tr.refdef.num_dlights; i++, l++ ) {
		R_MarkLightsQ2( l, 1 << i, tr.worldModel->brush38_nodes );
	}
}

//	Transforms the origins of an array of dlights. Used by both the front end
// (for DlightBmodel) and the back end (before doing the lighting calculation)
void R_TransformDlights( int count, dlight_t* dl, orientationr_t* orient ) {
	for ( int i = 0; i < count; i++, dl++ ) {
		vec3_t temp;
		VectorSubtract( dl->origin, orient->origin, temp );
		dl->transformed[ 0 ] = DotProduct( temp, orient->axis[ 0 ] );
		dl->transformed[ 1 ] = DotProduct( temp, orient->axis[ 1 ] );
		dl->transformed[ 2 ] = DotProduct( temp, orient->axis[ 2 ] );
	}
}

//	Determine which dynamic lights may effect this bmodel
void R_DlightBmodel( mbrush46_model_t* bmodel ) {
	// transform all the lights
	R_TransformDlights( tr.refdef.num_dlights, tr.refdef.dlights, &tr.orient );

	int mask = 0;
	for ( int i = 0; i < tr.refdef.num_dlights; i++ ) {
		dlight_t* dl = &tr.refdef.dlights[ i ];

		// ydnar: parallel dlights affect all entities
		if ( !( GGameType & GAME_ET ) || !( dl->flags & REF_DIRECTED_DLIGHT ) ) {
			// see if the point is close enough to the bounds to matter
			int j;
			for ( j = 0; j < 3; j++ ) {
				if ( dl->transformed[ j ] - bmodel->bounds[ 1 ][ j ] > dl->radius ) {
					break;
				}
				if ( bmodel->bounds[ 0 ][ j ] - dl->transformed[ j ] > dl->radius ) {
					break;
				}
			}
			if ( j < 3 ) {
				continue;
			}
		}

		// we need to check this light
		mask |= 1 << i;
	}

	tr.currentEntity->dlightBits = mask;

	// set the dlight bits in all the surfaces
	for ( int i = 0; i < bmodel->numSurfaces; i++ ) {
		mbrush46_surface_t* surf = bmodel->firstSurface + i;

		if ( *surf->data == SF_FACE ) {
			( ( srfSurfaceFace_t* )surf->data )->dlightBits[ tr.smpFrame ] = mask;
		} else if ( *surf->data == SF_GRID )     {
			( ( srfGridMesh_t* )surf->data )->dlightBits[ tr.smpFrame ] = mask;
		} else if ( *surf->data == SF_TRIANGLES )     {
			( ( srfTriangles_t* )surf->data )->dlightBits[ tr.smpFrame ] = mask;
		} else if ( *surf->data == SF_FOLIAGE )     {
			( ( srfFoliage_t* )surf->data )->dlightBits[ tr.smpFrame ] = mask;
		}
	}
}

//	frustum culls dynamic lights
void R_CullDlights() {
	//	limit
	if ( tr.refdef.num_dlights > MAX_DLIGHTS ) {
		tr.refdef.num_dlights = MAX_DLIGHTS;
	}

	//	walk dlight list
	int numDlights = 0;
	int dlightBits = 0;
	dlight_t* dl = tr.refdef.dlights;
	for ( int i = 0; i < tr.refdef.num_dlights; i++, dl++ ) {
		if ( ( dl->flags & REF_DIRECTED_DLIGHT ) || R_CullPointAndRadius( dl->origin, dl->radius ) != CULL_OUT ) {
			numDlights = i + 1;
			dlightBits |= ( 1 << i );
		}
	}

	//	reset count
	tr.refdef.num_dlights = numDlights;

	//	set bits
	tr.refdef.dlightBits = dlightBits;
}
