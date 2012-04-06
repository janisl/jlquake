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

#include "../../client.h"
#include "../local.h"

float r_anormals[NUMMDCVERTEXNORMALS][3] =
{
#include "anorms256.h"
};

void R_LatLongToNormal(vec3_t outNormal, short latLong)
{
	unsigned lat = (latLong >> 8) & 0xff;
	unsigned lng = (latLong & 0xff);
	lat *= (FUNCTABLE_SIZE / 256);
	lng *= (FUNCTABLE_SIZE / 256);

	// decode X as cos( lat ) * sin( long )
	// decode Y as sin( lat ) * sin( long )
	// decode Z as cos( long )

	outNormal[0] = tr.sinTable[(lat + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK] * tr.sinTable[lng];
	outNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
	outNormal[2] = tr.sinTable[(lng + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK];
}

static void LerpCMeshVertexes(mdcSurface_t* surf, float backlerp)
{
	float* outXyz = tess.xyz[tess.numVertexes];
	float* outNormal = tess.normal[tess.numVertexes];

	int newBase = (int)*((short*)((byte*)surf + surf->ofsFrameBaseFrames) + backEnd.currentEntity->e.frame);
	short* newXyz = (short*)((byte*)surf + surf->ofsXyzNormals) + (newBase * surf->numVerts * 4);
	short* newNormals = newXyz + 3;

	bool hasComp = (surf->numCompFrames > 0);
	short* newComp = NULL;
	mdcXyzCompressed_t* newXyzComp = NULL;
	if (hasComp)
	{
		newComp = ((short*)((byte*)surf + surf->ofsFrameCompFrames) + backEnd.currentEntity->e.frame);
		if (*newComp >= 0)
		{
			newXyzComp = (mdcXyzCompressed_t*)((byte*)surf + surf->ofsXyzCompressed) +
				(*newComp * surf->numVerts);
		}
	}

	float newXyzScale = MD3_XYZ_SCALE * (1.0 - backlerp);
	float newNormalScale = 1.0 - backlerp;

	int numVerts = surf->numVerts;

	if (backlerp == 0)
	{
		//
		// just copy the vertexes
		//
		for (int vertNum = 0; vertNum < numVerts; vertNum++,
			newXyz += 4, newNormals += 4, outXyz += 4, outNormal += 4)
		{
			outXyz[0] = newXyz[0] * newXyzScale;
			outXyz[1] = newXyz[1] * newXyzScale;
			outXyz[2] = newXyz[2] * newXyzScale;

			// add the compressed ofsVec
			if (hasComp && *newComp >= 0)
			{
				vec3_t newOfsVec;
				R_MDC_DecodeXyzCompressed(newXyzComp->ofsVec, newOfsVec, outNormal);
				newXyzComp++;
				VectorAdd(outXyz, newOfsVec, outXyz);
			}
			else
			{
				unsigned lat = (newNormals[0] >> 8) & 0xff;
				unsigned lng = (newNormals[0] & 0xff);
				lat *= (FUNCTABLE_SIZE / 256);
				lng *= (FUNCTABLE_SIZE / 256);

				outNormal[0] = tr.sinTable[(lat + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK] * tr.sinTable[lng];
				outNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
				outNormal[2] = tr.sinTable[(lng + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK];
			}
		}
	}
	else
	{
		//
		// interpolate and copy the vertex and normal
		//
		int oldBase = (int)*((short*)((byte*)surf + surf->ofsFrameBaseFrames) + backEnd.currentEntity->e.oldframe);
		short* oldXyz = (short*)((byte*)surf + surf->ofsXyzNormals) + (oldBase * surf->numVerts * 4);
		short* oldNormals = oldXyz + 3;

		short* oldComp = NULL;
		mdcXyzCompressed_t* oldXyzComp = NULL;
		if (hasComp)
		{
			oldComp = ((short*)((byte*)surf + surf->ofsFrameCompFrames) + backEnd.currentEntity->e.oldframe);
			if (*oldComp >= 0)
			{
				oldXyzComp = (mdcXyzCompressed_t*)((byte*)surf + surf->ofsXyzCompressed) +
					(*oldComp * surf->numVerts);
			}
		}

		float oldXyzScale = MD3_XYZ_SCALE * backlerp;
		float oldNormalScale = backlerp;

		for (int vertNum = 0 ; vertNum < numVerts ; vertNum++,
			  oldXyz += 4, newXyz += 4, oldNormals += 4, newNormals += 4,
			  outXyz += 4, outNormal += 4 )
		{
			// interpolate the xyz
			outXyz[0] = oldXyz[0] * oldXyzScale + newXyz[0] * newXyzScale;
			outXyz[1] = oldXyz[1] * oldXyzScale + newXyz[1] * newXyzScale;
			outXyz[2] = oldXyz[2] * oldXyzScale + newXyz[2] * newXyzScale;

			// add the compressed ofsVec
			vec3_t uncompressedNewNormal;
			if (hasComp && *newComp >= 0)
			{
				vec3_t newOfsVec;
				R_MDC_DecodeXyzCompressed(newXyzComp->ofsVec, newOfsVec, uncompressedNewNormal);
				newXyzComp++;
				VectorMA(outXyz, 1.0 - backlerp, newOfsVec, outXyz);
			}
			else
			{
				unsigned lat = (newNormals[0] >> 8) & 0xff;
				unsigned lng = (newNormals[0] & 0xff);
				lat *= (FUNCTABLE_SIZE / 256);
				lng *= (FUNCTABLE_SIZE / 256);

				uncompressedNewNormal[0] = tr.sinTable[(lat + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK] * tr.sinTable[lng];
				uncompressedNewNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
				uncompressedNewNormal[2] = tr.sinTable[(lng + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK];
			}

			vec3_t uncompressedOldNormal;
			if (hasComp && *oldComp >= 0)
			{
				vec3_t oldOfsVec;
				R_MDC_DecodeXyzCompressed(oldXyzComp->ofsVec, oldOfsVec, uncompressedOldNormal);
				oldXyzComp++;
				VectorMA(outXyz, backlerp, oldOfsVec, outXyz);
			}
			else
			{
				unsigned lat = (oldNormals[0] >> 8) & 0xff;
				unsigned lng = (oldNormals[0] & 0xff);
				lat *= (FUNCTABLE_SIZE / 256);
				lng *= (FUNCTABLE_SIZE / 256);

				uncompressedOldNormal[0] = tr.sinTable[(lat + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK] * tr.sinTable[lng];
				uncompressedOldNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
				uncompressedOldNormal[2] = tr.sinTable[(lng + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK];
			}

			outNormal[0] = uncompressedOldNormal[0] * oldNormalScale + uncompressedNewNormal[0] * newNormalScale;
			outNormal[1] = uncompressedOldNormal[1] * oldNormalScale + uncompressedNewNormal[1] * newNormalScale;
			outNormal[2] = uncompressedOldNormal[2] * oldNormalScale + uncompressedNewNormal[2] * newNormalScale;

			VectorNormalizeFast(outNormal);
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
