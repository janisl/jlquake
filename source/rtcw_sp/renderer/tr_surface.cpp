/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// tr_surf.c
#include "tr_local.h"

/*

  THIS ENTIRE FILE IS BACK END

backEnd.currentEntity will be valid.

Tess_Begin has already been called for the surface's shader.

The modelview matrix will be set.

It is safe to actually issue drawing commands here if you don't want to
use the shader system.
*/

void RB_SurfacePolychain( srfPoly_t *p );
void RB_SurfaceTriangles( srfTriangles_t *srf );
void LerpMeshVertexes( md3Surface_t *surf, float backlerp );

/*
** R_LatLongToNormal
*/
void R_LatLongToNormal( vec3_t outNormal, short latLong ) {
	unsigned lat, lng;

	lat = ( latLong >> 8 ) & 0xff;
	lng = ( latLong & 0xff );
	lat *= ( FUNCTABLE_SIZE / 256 );
	lng *= ( FUNCTABLE_SIZE / 256 );

	// decode X as cos( lat ) * sin( long )
	// decode Y as sin( lat ) * sin( long )
	// decode Z as cos( long )

	outNormal[0] = tr.sinTable[( lat + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK] * tr.sinTable[lng];
	outNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
	outNormal[2] = tr.sinTable[( lng + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK];
}

// Ridah
/*
** LerpCMeshVertexes
*/
static void LerpCMeshVertexes( mdcSurface_t *surf, float backlerp ) {
	short   *oldXyz, *newXyz, *oldNormals, *newNormals;
	float   *outXyz, *outNormal;
	float oldXyzScale, newXyzScale;
	float oldNormalScale, newNormalScale;
	int vertNum;
	unsigned lat, lng;
	int numVerts;

	int oldBase, newBase;
	short   *oldComp = NULL, *newComp = NULL; // TTimo: init
	mdcXyzCompressed_t *oldXyzComp = NULL, *newXyzComp = NULL; // TTimo: init
	vec3_t oldOfsVec, newOfsVec;

	qboolean hasComp;

	outXyz = tess.xyz[tess.numVertexes];
	outNormal = tess.normal[tess.numVertexes];

	newBase = (int)*( ( short * )( (byte *)surf + surf->ofsFrameBaseFrames ) + backEnd.currentEntity->e.frame );
	newXyz = ( short * )( (byte *)surf + surf->ofsXyzNormals )
			 + ( newBase * surf->numVerts * 4 );
	newNormals = newXyz + 3;

	hasComp = ( surf->numCompFrames > 0 );
	if ( hasComp ) {
		newComp = ( ( short * )( (byte *)surf + surf->ofsFrameCompFrames ) + backEnd.currentEntity->e.frame );
		if ( *newComp >= 0 ) {
			newXyzComp = ( mdcXyzCompressed_t * )( (byte *)surf + surf->ofsXyzCompressed )
						 + ( *newComp * surf->numVerts );
		}
	}

	newXyzScale = MD3_XYZ_SCALE * ( 1.0 - backlerp );
	newNormalScale = 1.0 - backlerp;

	numVerts = surf->numVerts;

	if ( backlerp == 0 ) {
		//
		// just copy the vertexes
		//
		for ( vertNum = 0 ; vertNum < numVerts ; vertNum++,
			  newXyz += 4, newNormals += 4,
			  outXyz += 4, outNormal += 4 )
		{

			outXyz[0] = newXyz[0] * newXyzScale;
			outXyz[1] = newXyz[1] * newXyzScale;
			outXyz[2] = newXyz[2] * newXyzScale;

			// add the compressed ofsVec
			if ( hasComp && *newComp >= 0 ) {
				R_MDC_DecodeXyzCompressed( newXyzComp->ofsVec, newOfsVec, outNormal );
				newXyzComp++;
				VectorAdd( outXyz, newOfsVec, outXyz );
			} else {
				lat = ( newNormals[0] >> 8 ) & 0xff;
				lng = ( newNormals[0] & 0xff );
				lat *= 4;
				lng *= 4;

				outNormal[0] = tr.sinTable[( lat + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK] * tr.sinTable[lng];
				outNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
				outNormal[2] = tr.sinTable[( lng + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK];
			}
		}
	} else {
		//
		// interpolate and copy the vertex and normal
		//
		oldBase = (int)*( ( short * )( (byte *)surf + surf->ofsFrameBaseFrames ) + backEnd.currentEntity->e.oldframe );
		oldXyz = ( short * )( (byte *)surf + surf->ofsXyzNormals )
				 + ( oldBase * surf->numVerts * 4 );
		oldNormals = oldXyz + 3;

		if ( hasComp ) {
			oldComp = ( ( short * )( (byte *)surf + surf->ofsFrameCompFrames ) + backEnd.currentEntity->e.oldframe );
			if ( *oldComp >= 0 ) {
				oldXyzComp = ( mdcXyzCompressed_t * )( (byte *)surf + surf->ofsXyzCompressed )
							 + ( *oldComp * surf->numVerts );
			}
		}

		oldXyzScale = MD3_XYZ_SCALE * backlerp;
		oldNormalScale = backlerp;

		for ( vertNum = 0 ; vertNum < numVerts ; vertNum++,
			  oldXyz += 4, newXyz += 4, oldNormals += 4, newNormals += 4,
			  outXyz += 4, outNormal += 4 )
		{
			vec3_t uncompressedOldNormal, uncompressedNewNormal;

			// interpolate the xyz
			outXyz[0] = oldXyz[0] * oldXyzScale + newXyz[0] * newXyzScale;
			outXyz[1] = oldXyz[1] * oldXyzScale + newXyz[1] * newXyzScale;
			outXyz[2] = oldXyz[2] * oldXyzScale + newXyz[2] * newXyzScale;

			// add the compressed ofsVec
			if ( hasComp && *newComp >= 0 ) {
				R_MDC_DecodeXyzCompressed( newXyzComp->ofsVec, newOfsVec, uncompressedNewNormal );
				newXyzComp++;
				VectorMA( outXyz, 1.0 - backlerp, newOfsVec, outXyz );
			} else {
				lat = ( newNormals[0] >> 8 ) & 0xff;
				lng = ( newNormals[0] & 0xff );
				lat *= 4;
				lng *= 4;

				uncompressedNewNormal[0] = tr.sinTable[( lat + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK] * tr.sinTable[lng];
				uncompressedNewNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
				uncompressedNewNormal[2] = tr.sinTable[( lng + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK];
			}

			if ( hasComp && *oldComp >= 0 ) {
				R_MDC_DecodeXyzCompressed( oldXyzComp->ofsVec, oldOfsVec, uncompressedOldNormal );
				oldXyzComp++;
				VectorMA( outXyz, backlerp, oldOfsVec, outXyz );
			} else {
				lat = ( oldNormals[0] >> 8 ) & 0xff;
				lng = ( oldNormals[0] & 0xff );
				lat *= 4;
				lng *= 4;

				uncompressedOldNormal[0] = tr.sinTable[( lat + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK] * tr.sinTable[lng];
				uncompressedOldNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
				uncompressedOldNormal[2] = tr.sinTable[( lng + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK];
			}

			outNormal[0] = uncompressedOldNormal[0] * oldNormalScale + uncompressedNewNormal[0] * newNormalScale;
			outNormal[1] = uncompressedOldNormal[1] * oldNormalScale + uncompressedNewNormal[1] * newNormalScale;
			outNormal[2] = uncompressedOldNormal[2] * oldNormalScale + uncompressedNewNormal[2] * newNormalScale;

			VectorNormalize( outNormal );
		}
	}
}

/*
=============
RB_SurfaceCMesh
=============
*/
void RB_SurfaceCMesh( mdcSurface_t *surface ) {
	int j;
	float backlerp;
	int             *triangles;
	float           *texCoords;
	int indexes;
	int Bob, Doug;
	int numVerts;

	// RF, check for REFLAG_HANDONLY
	if ( backEnd.currentEntity->e.reFlags & REFLAG_ONLYHAND ) {
		if ( !strstr( surface->name, "hand" ) ) {
			return;
		}
	}

	if (  backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame ) {
		backlerp = 0;
	} else  {
		backlerp = backEnd.currentEntity->e.backlerp;
	}

	RB_CHECKOVERFLOW( surface->numVerts, surface->numTriangles * 3 );

	LerpCMeshVertexes( surface, backlerp );

	triangles = ( int * )( (byte *)surface + surface->ofsTriangles );
	indexes = surface->numTriangles * 3;
	Bob = tess.numIndexes;
	Doug = tess.numVertexes;
	for ( j = 0 ; j < indexes ; j++ ) {
		tess.indexes[Bob + j] = Doug + triangles[j];
	}
	tess.numIndexes += indexes;

	texCoords = ( float * )( (byte *)surface + surface->ofsSt );

	numVerts = surface->numVerts;
	for ( j = 0; j < numVerts; j++ ) {
		tess.texCoords[Doug + j][0][0] = texCoords[j * 2 + 0];
		tess.texCoords[Doug + j][0][1] = texCoords[j * 2 + 1];
		// FIXME: fill in lightmapST for completeness?
	}

	tess.numVertexes += surface->numVerts;
}
// done.

void RB_SurfaceFace( srfSurfaceFace_t *surf );
void RB_SurfaceGrid( srfGridMesh_t *cv );
void RB_SurfaceEntity( surfaceType_t *surfType );
void RB_SurfaceBad( surfaceType_t *surfType );
void RB_SurfaceFlare( srfFlare_t *surf );
void RB_SurfaceFoliage(srfFoliage_t* srf);
void RB_SurfaceDisplayList( srfDisplayList_t *surf );
void RB_SurfaceSkip( void *surf );
void RB_SurfacePolyBuffer( srfPolyBuffer_t *surf );
void RB_SurfaceDecal( srfDecal_t *srf );

void NewTypeMD4(void*)
{
	common->Error("New unsupported surface type MD4");
}

void NewTypeMDM(void*)
{
	common->Error("New unsupported surface type MDM");
}

void NewTypePolyBuffer(void*)
{
	common->Error("New unsupported surface type poly buffer");
}

void NewTypeDecal(void*)
{
	common->Error("New unsupported surface type decal");
}

void( *rb_surfaceTable[SF_NUM_SURFACE_TYPES] ) ( void * ) = {
	( void( * ) ( void* ) )RB_SurfaceBad,          // SF_BAD,
	( void( * ) ( void* ) )RB_SurfaceSkip,         // SF_SKIP,
	( void( * ) ( void* ) )RB_SurfaceFace,         // SF_FACE,
	( void( * ) ( void* ) )RB_SurfaceGrid,         // SF_GRID,
	( void( * ) ( void* ) )RB_SurfaceTriangles,    // SF_TRIANGLES,
	( void( * ) ( void* ) )RB_SurfaceFoliage,      //SF_FOLIAGE,
	( void( * ) ( void* ) )RB_SurfacePolychain,    // SF_POLY,
	( void( * ) ( void* ) )RB_SurfaceMesh,         // SF_MD3,
	NewTypeMD4,//SF_MD4
	( void( * ) ( void* ) )RB_SurfaceCMesh,        // SF_MDC,
	( void( * ) ( void* ) )RB_SurfaceAnim,         // SF_MDS,
	NewTypeMDM,//SF_MDM
	( void( * ) ( void* ) )RB_SurfaceFlare,        // SF_FLARE,
	( void( * ) ( void* ) )RB_SurfaceEntity,       // SF_ENTITY
	( void( * ) ( void* ) )RB_SurfaceDisplayList,  // SF_DISPLAY_LIST
	( void( * ) ( void* ) )RB_SurfacePolyBuffer,   // SF_POLYBUFFER
	( void( * ) ( void* ) )RB_SurfaceDecal,        // SF_DECAL
};
