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

#include "SurfaceFoliage.h"
#include "surfaces.h"
#include "backend.h"
#include "cvars.h"

void idSurfaceFoliage::Draw() {
	srfFoliage_t* srf = ( srfFoliage_t* )data;
	// basic setup
	int numVerts = srf->numVerts;
	int numIndexes = srf->numIndexes;
	vec3_t viewOrigin;
	VectorCopy( backEnd.orient.viewOrigin, viewOrigin );

	// set fov scale
	float fovScale = backEnd.viewParms.fovX * ( 1.0 / 90.0 );

	// calculate distance vector
	vec3_t local;
	VectorSubtract( backEnd.orient.origin, backEnd.viewParms.orient.origin, local );
	vec4_t distanceVector;
	distanceVector[ 0 ] = -backEnd.orient.modelMatrix[ 2 ];
	distanceVector[ 1 ] = -backEnd.orient.modelMatrix[ 6 ];
	distanceVector[ 2 ] = -backEnd.orient.modelMatrix[ 10 ];
	distanceVector[ 3 ] = DotProduct( local, backEnd.viewParms.orient.axis[ 0 ] );

	// attempt distance cull
	vec4_t distanceCull;
	VectorCopy( tess.shader->distanceCull, distanceCull );
	distanceCull[ 3 ] = tess.shader->distanceCull[ 3 ];
	if ( distanceCull[ 1 ] > 0 ) {
		float z = fovScale * ( DotProduct( srf->localOrigin, distanceVector ) + distanceVector[ 3 ] - srf->radius );
		float alpha = ( distanceCull[ 1 ] - z ) * distanceCull[ 3 ];
		if ( alpha < distanceCull[ 2 ] ) {
			return;
		}
	}

	// set dlight bits
	int dlightBits = this->dlightBits[ backEnd.smpFrame ];
	tess.dlightBits |= dlightBits;

	// iterate through origin list
	foliageInstance_t* instance = srf->instances;
	for ( int o = 0; o < srf->numInstances; o++, instance++ ) {
		// fade alpha based on distance between inner and outer radii
		int srcColor = *( int* )instance->color;
		if ( distanceCull[ 1 ] > 0.0f ) {
			// calculate z distance
			float z = fovScale * ( DotProduct( instance->origin, distanceVector ) + distanceVector[ 3 ] );
			if ( z < -64.0f ) {
				// epsilon so close-by foliage doesn't pop in and out
				continue;
			}

			// check against frustum planes
			int i;
			for ( i = 0; i < 5; i++ ) {
				float dist = DotProduct( instance->origin, backEnd.viewParms.frustum[ i ].normal ) - backEnd.viewParms.frustum[ i ].dist;
				if ( dist < -64.0 ) {
					break;
				}
			}
			if ( i != 5 ) {
				continue;
			}

			// radix
			if ( o & 1 ) {
				z *= 1.25;
				if ( o & 2 ) {
					z *= 1.25;
				}
			}

			// calculate alpha
			float alpha = ( distanceCull[ 1 ] - z ) * distanceCull[ 3 ];
			if ( alpha < distanceCull[ 2 ] ) {
				continue;
			}

			// set color
			int a = alpha > 1.0f ? 255 : alpha * 255;
			( ( byte* )&srcColor )[ 3 ] = a;
		}

		RB_CHECKOVERFLOW( numVerts, numIndexes );

		// ydnar: set after overflow check so dlights work properly
		tess.dlightBits |= dlightBits;

		// copy indexes
		Com_Memcpy( &tess.indexes[ tess.numIndexes ], srf->indexes, numIndexes * sizeof ( srf->indexes[ 0 ] ) );
		for ( int i = 0; i < numIndexes; i++ ) {
			tess.indexes[ tess.numIndexes + i ] += tess.numVertexes;
		}

		// copy and offset xyz, copy normal, st and color
		vec_t* xyz = tess.xyz[ tess.numVertexes ];
		int* color = ( int* )tess.vertexColors[ tess.numVertexes ];
		for ( int i = 0; i < numVerts; i++, xyz += 4 ) {
			vec3_t old;
			vertexes[ i ].xyz.ToOldVec3( old );
			VectorAdd( old, instance->origin, xyz );
		}
		if ( tess.shader->needsNormal ) {
			Com_Memcpy( &tess.normal[ tess.numVertexes ], srf->normal, numVerts * sizeof ( srf->normal[ 0 ] ) );
		}
		for ( int i = 0; i < numVerts; i++ ) {
			tess.texCoords[ tess.numVertexes + i ][ 0 ][ 0 ] = srf->texCoords[ i ][ 0 ];
			tess.texCoords[ tess.numVertexes + i ][ 0 ][ 1 ] = srf->texCoords[ i ][ 1 ];
			tess.texCoords[ tess.numVertexes + i ][ 1 ][ 0 ] = srf->lmTexCoords[ i ][ 0 ];
			tess.texCoords[ tess.numVertexes + i ][ 1 ][ 1 ] = srf->lmTexCoords[ i ][ 1 ];
			color[ i ] = srcColor;
		}

		// increment
		tess.numIndexes += numIndexes;
		tess.numVertexes += numVerts;
	}
}

bool idSurfaceFoliage::DoCullET( shader_t* shader, int* frontFace ) const {
	if ( !r_drawfoliage->value ) {
		return true;
	}
	return idWorldSurface::DoCullET( shader, frontFace );
}
