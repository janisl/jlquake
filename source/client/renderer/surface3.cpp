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

/*
  THIS ENTIRE FILE IS BACK END

backEnd.currentEntity will be valid.

Tess_Begin has already been called for the surface's shader.

The modelview matrix will be set.

It is safe to actually issue drawing commands here if you don't want to
use the shader system.
*/

// HEADER FILES ------------------------------------------------------------

#include "../client.h"
#include "local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	RB_CheckOverflow
//
//==========================================================================

void RB_CheckOverflow(int verts, int indexes)
{
	if (tess.numVertexes + verts < SHADER_MAX_VERTEXES &&
		tess.numIndexes + indexes < SHADER_MAX_INDEXES)
	{
		return;
	}

	RB_EndSurface();

	if (verts >= SHADER_MAX_VERTEXES)
	{
		throw DropException(va("RB_CheckOverflow: verts > MAX (%d > %d)", verts, SHADER_MAX_VERTEXES));
	}
	if (indexes >= SHADER_MAX_INDEXES)
	{
		throw DropException(va("RB_CheckOverflow: indices > MAX (%d > %d)", indexes, SHADER_MAX_INDEXES));
	}

	RB_BeginSurface(tess.shader, tess.fogNum);
}

//==========================================================================
//
//	RB_AddQuadStampExt
//
//==========================================================================

void RB_AddQuadStampExt(vec3_t origin, vec3_t left, vec3_t up, byte* color, float s1, float t1, float s2, float t2)
{
	RB_CHECKOVERFLOW(4, 6);

	int ndx = tess.numVertexes;

	// triangle indexes for a simple quad
	tess.indexes[tess.numIndexes] = ndx;
	tess.indexes[tess.numIndexes + 1] = ndx + 1;
	tess.indexes[tess.numIndexes + 2] = ndx + 3;

	tess.indexes[tess.numIndexes + 3] = ndx + 3;
	tess.indexes[tess.numIndexes + 4] = ndx + 1;
	tess.indexes[tess.numIndexes + 5] = ndx + 2;

	tess.xyz[ndx][0] = origin[0] + left[0] + up[0];
	tess.xyz[ndx][1] = origin[1] + left[1] + up[1];
	tess.xyz[ndx][2] = origin[2] + left[2] + up[2];

	tess.xyz[ndx + 1][0] = origin[0] - left[0] + up[0];
	tess.xyz[ndx + 1][1] = origin[1] - left[1] + up[1];
	tess.xyz[ndx + 1][2] = origin[2] - left[2] + up[2];

	tess.xyz[ndx + 2][0] = origin[0] - left[0] - up[0];
	tess.xyz[ndx + 2][1] = origin[1] - left[1] - up[1];
	tess.xyz[ndx + 2][2] = origin[2] - left[2] - up[2];

	tess.xyz[ndx + 3][0] = origin[0] + left[0] - up[0];
	tess.xyz[ndx + 3][1] = origin[1] + left[1] - up[1];
	tess.xyz[ndx + 3][2] = origin[2] + left[2] - up[2];

	// constant normal all the way around
	vec3_t normal;
	VectorSubtract(vec3_origin, backEnd.viewParms.orient.axis[0], normal);

	tess.normal[ndx][0] = tess.normal[ndx + 1][0] = tess.normal[ndx + 2][0] = tess.normal[ndx + 3][0] = normal[0];
	tess.normal[ndx][1] = tess.normal[ndx + 1][1] = tess.normal[ndx + 2][1] = tess.normal[ndx + 3][1] = normal[1];
	tess.normal[ndx][2] = tess.normal[ndx + 1][2] = tess.normal[ndx + 2][2] = tess.normal[ndx + 3][2] = normal[2];

	// standard square texture coordinates
	tess.texCoords[ndx][0][0] = tess.texCoords[ndx][1][0] = s1;
	tess.texCoords[ndx][0][1] = tess.texCoords[ndx][1][1] = t1;

	tess.texCoords[ndx + 1][0][0] = tess.texCoords[ndx + 1][1][0] = s2;
	tess.texCoords[ndx + 1][0][1] = tess.texCoords[ndx + 1][1][1] = t1;

	tess.texCoords[ndx + 2][0][0] = tess.texCoords[ndx + 2][1][0] = s2;
	tess.texCoords[ndx + 2][0][1] = tess.texCoords[ndx + 2][1][1] = t2;

	tess.texCoords[ndx + 3][0][0] = tess.texCoords[ndx + 3][1][0] = s1;
	tess.texCoords[ndx + 3][0][1] = tess.texCoords[ndx + 3][1][1] = t2;

	// constant color all the way around
	// should this be identity and let the shader specify from entity?
	*(unsigned int*)&tess.vertexColors[ndx] =
		*(unsigned int*)&tess.vertexColors[ndx + 1] =
			*(unsigned int*)&tess.vertexColors[ndx + 2] =
				*(unsigned int*)&tess.vertexColors[ndx + 3] =
					*(unsigned int*)color;

	tess.numVertexes += 4;
	tess.numIndexes += 6;
}

//==========================================================================
//
//	RB_AddQuadStamp
//
//==========================================================================

void RB_AddQuadStamp(vec3_t origin, vec3_t left, vec3_t up, byte* color)
{
	RB_AddQuadStampExt(origin, left, up, color, 0, 0, 1, 1);
}

//==========================================================================
//
//	RB_SurfaceBad
//
//==========================================================================

static void RB_SurfaceBad(surfaceType_t* surfType)
{
	Log::write("Bad surface tesselated.\n");
}

//==========================================================================
//
//	RB_SurfaceBad
//
//==========================================================================

static void RB_SurfaceSkip(void*)
{
}

//==========================================================================
//
//	RB_SurfaceFace
//
//==========================================================================

static void RB_SurfaceFace(srfSurfaceFace_t* surf)
{
	RB_CHECKOVERFLOW(surf->numPoints, surf->numIndices);

	int dlightBits = surf->dlightBits[backEnd.smpFrame];
	tess.dlightBits |= dlightBits;

	unsigned* indices = (unsigned*)(((char*)surf) + surf->ofsIndices);

	int Bob = tess.numVertexes;
	unsigned* tessIndexes = tess.indexes + tess.numIndexes;
	for (int i = surf->numIndices - 1; i >= 0; i--)
	{
		tessIndexes[i] = indices[i] + Bob;
	}

	tess.numIndexes += surf->numIndices;

	float* v = surf->points[0];

	int numPoints = surf->numPoints;

	if (tess.shader->needsNormal)
	{
		float* normal = surf->plane.normal;
		for (int i = 0, ndx = tess.numVertexes; i < numPoints; i++, ndx++)
		{
			VectorCopy(normal, tess.normal[ndx]);
		}
	}

	v = surf->points[0];
	for (int i = 0, ndx = tess.numVertexes; i < numPoints; i++, v += BRUSH46_VERTEXSIZE, ndx++)
	{
		VectorCopy(v, tess.xyz[ndx]);
		tess.texCoords[ndx][0][0] = v[3];
		tess.texCoords[ndx][0][1] = v[4];
		tess.texCoords[ndx][1][0] = v[5];
		tess.texCoords[ndx][1][1] = v[6];
		*(unsigned int*)&tess.vertexColors[ndx] = *(unsigned int*)&v[7];
		tess.vertexDlightBits[ndx] = dlightBits;
	}

	tess.numVertexes += surf->numPoints;
}

//==========================================================================
//
//	LodErrorForVolume
//
//==========================================================================

static float LodErrorForVolume(vec3_t local, float radius)
{
	// never let it go negative
	if (r_lodCurveError->value < 0)
	{
		return 0;
	}

	vec3_t world;
	world[0] = local[0] * backEnd.orient.axis[0][0] + local[1] * backEnd.orient.axis[1][0] +
			   local[2] * backEnd.orient.axis[2][0] + backEnd.orient.origin[0];
	world[1] = local[0] * backEnd.orient.axis[0][1] + local[1] * backEnd.orient.axis[1][1] +
			   local[2] * backEnd.orient.axis[2][1] + backEnd.orient.origin[1];
	world[2] = local[0] * backEnd.orient.axis[0][2] + local[1] * backEnd.orient.axis[1][2] +
			   local[2] * backEnd.orient.axis[2][2] + backEnd.orient.origin[2];

	VectorSubtract(world, backEnd.viewParms.orient.origin, world);
	float d = DotProduct(world, backEnd.viewParms.orient.axis[0]);

	if (d < 0)
	{
		d = -d;
	}
	d -= radius;
	if (d < 1)
	{
		d = 1;
	}

	return r_lodCurveError->value / d;
}

//==========================================================================
//
//	RB_SurfaceGrid
//
//	Just copy the grid of points and triangulate
//
//==========================================================================

static void RB_SurfaceGrid(srfGridMesh_t* cv)
{
	int dlightBits = cv->dlightBits[backEnd.smpFrame];
	tess.dlightBits |= dlightBits;

	// determine the allowable discrepance
	float lodError = LodErrorForVolume(cv->lodOrigin, cv->lodRadius);

	// determine which rows and columns of the subdivision
	// we are actually going to use
	int widthTable[MAX_GRID_SIZE];
	widthTable[0] = 0;
	int lodWidth = 1;
	for (int i = 1; i < cv->width - 1; i++)
	{
		if (cv->widthLodError[i] <= lodError)
		{
			widthTable[lodWidth] = i;
			lodWidth++;
		}
	}
	widthTable[lodWidth] = cv->width - 1;
	lodWidth++;

	int heightTable[MAX_GRID_SIZE];
	heightTable[0] = 0;
	int lodHeight = 1;
	for (int i = 1; i < cv->height - 1; i++)
	{
		if (cv->heightLodError[i] <= lodError)
		{
			heightTable[lodHeight] = i;
			lodHeight++;
		}
	}
	heightTable[lodHeight] = cv->height - 1;
	lodHeight++;

	// very large grids may have more points or indexes than can be fit
	// in the tess structure, so we may have to issue it in multiple passes

	int used = 0;
	while (used < lodHeight - 1)
	{
		// see how many rows of both verts and indexes we can add without overflowing
		int irows, vrows;
		do
		{
			vrows = (SHADER_MAX_VERTEXES - tess.numVertexes) / lodWidth;
			irows = (SHADER_MAX_INDEXES - tess.numIndexes) / (lodWidth * 6);

			// if we don't have enough space for at least one strip, flush the buffer
			if (vrows < 2 || irows < 1)
			{
				RB_EndSurface();
				RB_BeginSurface(tess.shader, tess.fogNum);
				tess.dlightBits |= dlightBits;	// ydnar: for proper dlighting
			}
			else
			{
				break;
			}
		}
		while (1);

		int rows = irows;
		if (vrows < irows + 1)
		{
			rows = vrows - 1;
		}
		if (used + rows > lodHeight)
		{
			rows = lodHeight - used;
		}

		int numVertexes = tess.numVertexes;

		float* xyz = tess.xyz[numVertexes];
		float* normal = tess.normal[numVertexes];
		float* texCoords = tess.texCoords[numVertexes][0];
		byte* color = tess.vertexColors[numVertexes];
		int* vDlightBits = &tess.vertexDlightBits[numVertexes];
		bool needsNormal = tess.shader->needsNormal;

		for (int i = 0; i < rows; i++)
		{
			for (int j = 0; j < lodWidth; j++)
			{
				bsp46_drawVert_t* dv = cv->verts + heightTable[used + i] * cv->width + widthTable[j];

				xyz[0] = dv->xyz[0];
				xyz[1] = dv->xyz[1];
				xyz[2] = dv->xyz[2];
				texCoords[0] = dv->st[0];
				texCoords[1] = dv->st[1];
				texCoords[2] = dv->lightmap[0];
				texCoords[3] = dv->lightmap[1];
				if (needsNormal)
				{
					normal[0] = dv->normal[0];
					normal[1] = dv->normal[1];
					normal[2] = dv->normal[2];
				}
				*(unsigned int*)color = *(unsigned int*)dv->color;
				*vDlightBits++ = dlightBits;
				xyz += 4;
				normal += 4;
				texCoords += 4;
				color += 4;
			}
		}

		// add the indexes
		int h = rows - 1;
		int w = lodWidth - 1;
		int numIndexes = tess.numIndexes;
		for (int i = 0; i < h; i++)
		{
			for (int j = 0; j < w; j++)
			{
				// vertex order to be reckognized as tristrips
				int v1 = numVertexes + i * lodWidth + j + 1;
				int v2 = v1 - 1;
				int v3 = v2 + lodWidth;
				int v4 = v3 + 1;

				tess.indexes[numIndexes] = v2;
				tess.indexes[numIndexes + 1] = v3;
				tess.indexes[numIndexes + 2] = v1;

				tess.indexes[numIndexes + 3] = v1;
				tess.indexes[numIndexes + 4] = v3;
				tess.indexes[numIndexes + 5] = v4;
				numIndexes += 6;
			}
		}

		tess.numIndexes = numIndexes;

		tess.numVertexes += rows * lodWidth;

		used += rows - 1;
	}
}

//==========================================================================
//
//	RB_SurfaceTriangles
//
//==========================================================================

static void RB_SurfaceTriangles(srfTriangles_t* srf)
{
	// ydnar: moved before overflow so dlights work properly
	RB_CHECKOVERFLOW(srf->numVerts, srf->numIndexes);

	int dlightBits = srf->dlightBits[backEnd.smpFrame];
	tess.dlightBits |= dlightBits;

	for (int i = 0; i < srf->numIndexes; i += 3)
	{
		tess.indexes[tess.numIndexes + i + 0] = tess.numVertexes + srf->indexes[i + 0];
		tess.indexes[tess.numIndexes + i + 1] = tess.numVertexes + srf->indexes[i + 1];
		tess.indexes[tess.numIndexes + i + 2] = tess.numVertexes + srf->indexes[i + 2];
	}
	tess.numIndexes += srf->numIndexes;

	bsp46_drawVert_t* dv = srf->verts;
	float* xyz = tess.xyz[tess.numVertexes];
	float* normal = tess.normal[tess.numVertexes];
	float* texCoords = tess.texCoords[tess.numVertexes][0];
	byte* color = tess.vertexColors[tess.numVertexes];
	bool needsNormal = tess.shader->needsNormal;

	for (int i = 0; i < srf->numVerts; i++, dv++, xyz += 4, normal += 4, texCoords += 4, color += 4)
	{
		xyz[0] = dv->xyz[0];
		xyz[1] = dv->xyz[1];
		xyz[2] = dv->xyz[2];

		if (needsNormal)
		{
			normal[0] = dv->normal[0];
			normal[1] = dv->normal[1];
			normal[2] = dv->normal[2];
		}

		texCoords[0] = dv->st[0];
		texCoords[1] = dv->st[1];

		texCoords[2] = dv->lightmap[0];
		texCoords[3] = dv->lightmap[1];

		*(int*)color = *(int*)dv->color;
	}

	for (int i = 0; i < srf->numVerts; i++)
	{
		tess.vertexDlightBits[tess.numVertexes + i] = dlightBits;
	}

	tess.numVertexes += srf->numVerts;
}

static void RB_SurfaceFoliage(srfFoliage_t* srf)
{
	// basic setup
	int numVerts = srf->numVerts;
	int numIndexes = srf->numIndexes;
	vec3_t viewOrigin;
	VectorCopy(backEnd.orient.viewOrigin, viewOrigin);

	// set fov scale
	float fovScale = backEnd.viewParms.fovX * (1.0 / 90.0);

	// calculate distance vector
	vec3_t local;
	VectorSubtract(backEnd.orient.origin, backEnd.viewParms.orient.origin, local);
	vec4_t distanceVector;
	distanceVector[0] = -backEnd.orient.modelMatrix[2];
	distanceVector[1] = -backEnd.orient.modelMatrix[6];
	distanceVector[2] = -backEnd.orient.modelMatrix[10];
	distanceVector[3] = DotProduct(local, backEnd.viewParms.orient.axis[0]);

	// attempt distance cull
	vec4_t distanceCull;
	VectorCopy(tess.shader->distanceCull, distanceCull);
	distanceCull[3] = tess.shader->distanceCull[3];
	if (distanceCull[1] > 0)
	{
		float z = fovScale * (DotProduct(srf->localOrigin, distanceVector) + distanceVector[3] - srf->radius);
		float alpha = (distanceCull[1] - z) * distanceCull[3];
		if (alpha < distanceCull[2])
		{
			return;
		}
	}

	// set dlight bits
	int dlightBits = srf->dlightBits[backEnd.smpFrame];
	tess.dlightBits |= dlightBits;

	// iterate through origin list
	foliageInstance_t* instance = srf->instances;
	for (int o = 0; o < srf->numInstances; o++, instance++)
	{
		// fade alpha based on distance between inner and outer radii
		int srcColor = *(int*)instance->color;
		if (distanceCull[1] > 0.0f)
		{
			// calculate z distance
			float z = fovScale * (DotProduct(instance->origin, distanceVector) + distanceVector[3]);
			if (z < -64.0f)
			{
				// epsilon so close-by foliage doesn't pop in and out
				continue;
			}

			// check against frustum planes
			int i;
			for (i = 0; i < 5; i++)
			{
				float dist = DotProduct(instance->origin, backEnd.viewParms.frustum[i].normal) - backEnd.viewParms.frustum[i].dist;
				if (dist < -64.0)
				{
					break;
				}
			}
			if (i != 5)
			{
				continue;
			}

			// radix
			if (o & 1)
			{
				z *= 1.25;
				if (o & 2)
				{
					z *= 1.25;
				}
			}

			// calculate alpha
			float alpha = (distanceCull[1] - z) * distanceCull[3];
			if (alpha < distanceCull[2])
			{
				continue;
			}

			// set color
			int a = alpha > 1.0f ? 255 : alpha * 255;
			((byte*)&srcColor)[3] = a;
		}

		RB_CHECKOVERFLOW(numVerts, numIndexes);

		// ydnar: set after overflow check so dlights work properly
		tess.dlightBits |= dlightBits;

		// copy indexes
		Com_Memcpy(&tess.indexes[tess.numIndexes], srf->indexes, numIndexes * sizeof(srf->indexes[0]));
		for (int i = 0; i < numIndexes; i++)
		{
			tess.indexes[tess.numIndexes + i] += tess.numVertexes;
		}

		// copy xyz, normal and st
		vec_t* xyz = tess.xyz[tess.numVertexes];
		Com_Memcpy(xyz, srf->xyz, numVerts * sizeof(srf->xyz[0]));
		if (tess.shader->needsNormal)
		{
			Com_Memcpy(&tess.normal[tess.numVertexes], srf->normal, numVerts * sizeof(srf->xyz[0]));
		}
		for (int i = 0; i < numVerts; i++)
		{
			tess.texCoords[tess.numVertexes + i][0][0] = srf->texCoords[i][0];
			tess.texCoords[tess.numVertexes + i][0][1] = srf->texCoords[i][1];
			tess.texCoords[tess.numVertexes + i][1][0] = srf->lmTexCoords[i][0];
			tess.texCoords[tess.numVertexes + i][1][1] = srf->lmTexCoords[i][1];
		}

		// offset xyz
		for (int i = 0; i < numVerts; i++, xyz += 4)
		{
			VectorAdd(xyz, instance->origin, xyz);
		}

		// copy color
		int* color = (int*)tess.vertexColors[tess.numVertexes];
		for (int i = 0; i < numVerts; i++)
		{
			color[i] = srcColor;
		}

		// increment
		tess.numIndexes += numIndexes;
		tess.numVertexes += numVerts;
	}
}

//==========================================================================
//
//	RB_SurfacePolychain
//
//==========================================================================

static void RB_SurfacePolychain(srfPoly_t* p)
{
	RB_CHECKOVERFLOW(p->numVerts, 3 * (p->numVerts - 2));

	// fan triangles into the tess array
	int numv = tess.numVertexes;
	for (int i = 0; i < p->numVerts; i++)
	{
		VectorCopy(p->verts[i].xyz, tess.xyz[numv]);
		tess.texCoords[numv][0][0] = p->verts[i].st[0];
		tess.texCoords[numv][0][1] = p->verts[i].st[1];
		*(int*)&tess.vertexColors[numv] = *(int*)p->verts[i].modulate;

		numv++;
	}

	// generate fan indexes into the tess array
	for (int i = 0; i < p->numVerts - 2; i++)
	{
		tess.indexes[tess.numIndexes + 0] = tess.numVertexes;
		tess.indexes[tess.numIndexes + 1] = tess.numVertexes + i + 1;
		tess.indexes[tess.numIndexes + 2] = tess.numVertexes + i + 2;
		tess.numIndexes += 3;
	}

	tess.numVertexes = numv;
}

//==========================================================================
//
//	RB_SurfaceFlare
//
//==========================================================================

static void RB_SurfaceFlare(srfFlare_t* surf)
{
#if 0
	// calculate the xyz locations for the four corners
	float radius = 30;
	vec3_t left, up;
	VectorScale(backEnd.viewParms.orient.axis[1], radius, left);
	VectorScale(backEnd.viewParms.orient.axis[2], radius, up);
	if (backEnd.viewParms.isMirror)
	{
		VectorSubtract(vec3_origin, left, left);
	}

	byte color[4];
	color[0] = color[1] = color[2] = color[3] = 255;

	vec3_t origin;
	VectorMA(surf->origin, 3, surf->normal, origin);
	vec3_t dir;
	VectorSubtract(origin, backEnd.viewParms.orient.origin, dir);
	VectorNormalize(dir);
	VectorMA(origin, r_ignore->value, dir, origin);

	float d = -DotProduct(dir, surf->normal);
	if (d < 0)
	{
		return;
	}
#if 0
	color[0] *= d;
	color[1] *= d;
	color[2] *= d;
#endif

	RB_AddQuadStamp(origin, left, up, color);
#elif 0
	byte color[4];
	color[0] = surf->color[0] * 255;
	color[1] = surf->color[1] * 255;
	color[2] = surf->color[2] * 255;
	color[3] = 255;

	vec3_t left, up;
	VectorClear(left);
	VectorClear(up);

	left[0] = r_ignore->value;

	up[1] = r_ignore->value;

	RB_AddQuadStampExt(surf->origin, left, up, color, 0, 0, 1, 1);
#endif
}

//==========================================================================
//
//	RB_SurfaceSprite
//
//==========================================================================

static void RB_SurfaceSprite()
{
	// calculate the xyz locations for the four corners
	float radius = backEnd.currentEntity->e.radius;
	vec3_t left, up;
	if (backEnd.currentEntity->e.rotation == 0)
	{
		VectorScale(backEnd.viewParms.orient.axis[1], radius, left);
		VectorScale(backEnd.viewParms.orient.axis[2], radius, up);
	}
	else
	{
		float ang = M_PI * backEnd.currentEntity->e.rotation / 180;
		float s = sin(ang);
		float c = cos(ang);

		VectorScale(backEnd.viewParms.orient.axis[1], c * radius, left);
		VectorMA(left, -s * radius, backEnd.viewParms.orient.axis[2], left);

		VectorScale(backEnd.viewParms.orient.axis[2], c * radius, up);
		VectorMA(up, s * radius, backEnd.viewParms.orient.axis[1], up);
	}
	if (backEnd.viewParms.isMirror)
	{
		VectorSubtract(vec3_origin, left, left);
	}

	RB_AddQuadStamp(backEnd.currentEntity->e.origin, left, up, backEnd.currentEntity->e.shaderRGBA);
}

//==========================================================================
//
//	RB_SurfaceBeam
//
//==========================================================================

static void RB_SurfaceBeam()
{
	refEntity_t* e = &backEnd.currentEntity->e;

	vec3_t oldorigin;
	oldorigin[0] = e->oldorigin[0];
	oldorigin[1] = e->oldorigin[1];
	oldorigin[2] = e->oldorigin[2];

	vec3_t origin;
	origin[0] = e->origin[0];
	origin[1] = e->origin[1];
	origin[2] = e->origin[2];

	vec3_t direction, normalized_direction;
	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if (VectorNormalize(normalized_direction) == 0)
	{
		return;
	}

	vec3_t perpvec;
	PerpendicularVector(perpvec, normalized_direction);

	VectorScale(perpvec, 4, perpvec);

	enum { NUM_BEAM_SEGS = 6 };
	vec3_t start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	for (int i = 0; i < NUM_BEAM_SEGS; i++)
	{
		RotatePointAroundVector(start_points[i], normalized_direction, perpvec, (360.0 / NUM_BEAM_SEGS) * i);
		VectorAdd(start_points[i], direction, end_points[i]);
	}

	GL_Bind(tr.whiteImage);

	GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);

	qglColor3f(1, 0, 0);

	qglBegin(GL_TRIANGLE_STRIP);
	for (int i = 0; i <= NUM_BEAM_SEGS; i++)
	{
		qglVertex3fv(start_points[i % NUM_BEAM_SEGS]);
		qglVertex3fv(end_points[i % NUM_BEAM_SEGS]);
	}
	qglEnd();
}

//==========================================================================
//
//	DoRailCore
//
//==========================================================================

static void DoRailCore(const vec3_t start, const vec3_t end, const vec3_t up, float len, float spanWidth)
{
	// Gordon: configurable tile
	float t = len / (GGameType & GAME_ET && backEnd.currentEntity->e.radius > 0 ? backEnd.currentEntity->e.radius : 256.f);

	int vbase = tess.numVertexes;

	float spanWidth2 = -spanWidth;

	// FIXME: use quad stamp?
	VectorMA(start, spanWidth, up, tess.xyz[tess.numVertexes]);
	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = 0;
	if (GGameType & GAME_ET)
	{
		tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
		tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
		tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
		tess.vertexColors[tess.numVertexes][3] = backEnd.currentEntity->e.shaderRGBA[3];
	}
	else
	{
		tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0] * 0.25;
		tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1] * 0.25;
		tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2] * 0.25;
	}
	tess.numVertexes++;

	VectorMA(start, spanWidth2, up, tess.xyz[tess.numVertexes]);
	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = 1;
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	if (GGameType & GAME_ET)
	{
		tess.vertexColors[tess.numVertexes][3] = backEnd.currentEntity->e.shaderRGBA[3];
	}
	tess.numVertexes++;

	VectorMA(end, spanWidth, up, tess.xyz[tess.numVertexes]);

	tess.texCoords[tess.numVertexes][0][0] = t;
	tess.texCoords[tess.numVertexes][0][1] = 0;
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	if (GGameType & GAME_ET)
	{
		tess.vertexColors[tess.numVertexes][3] = backEnd.currentEntity->e.shaderRGBA[3];
	}
	tess.numVertexes++;

	VectorMA(end, spanWidth2, up, tess.xyz[tess.numVertexes]);
	tess.texCoords[tess.numVertexes][0][0] = t;
	tess.texCoords[tess.numVertexes][0][1] = 1;
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	if (GGameType & GAME_ET)
	{
		tess.vertexColors[tess.numVertexes][3] = backEnd.currentEntity->e.shaderRGBA[3];
	}
	tess.numVertexes++;

	tess.indexes[tess.numIndexes++] = vbase;
	tess.indexes[tess.numIndexes++] = vbase + 1;
	tess.indexes[tess.numIndexes++] = vbase + 2;

	tess.indexes[tess.numIndexes++] = vbase + 2;
	tess.indexes[tess.numIndexes++] = vbase + 1;
	tess.indexes[tess.numIndexes++] = vbase + 3;
}

//==========================================================================
//
//	RB_SurfaceBeam
//
//==========================================================================

static void RB_SurfaceRailCore()
{
	refEntity_t* e = &backEnd.currentEntity->e;

	vec3_t start, end;
	VectorCopy(e->oldorigin, start);
	VectorCopy(e->origin, end);

	vec3_t vec;
	VectorSubtract(end, start, vec);
	int len = VectorNormalize(vec);

	// compute side vector
	vec3_t v1;
	VectorSubtract(start, backEnd.viewParms.orient.origin, v1);
	VectorNormalize(v1);
	vec3_t v2;
	VectorSubtract(end, backEnd.viewParms.orient.origin, v2);
	VectorNormalize(v2);
	vec3_t right;
	CrossProduct(v1, v2, right);
	VectorNormalize(right);

	if (GGameType & GAME_ET)
	{
		DoRailCore(start, end, right, len, e->frame > 0 ? e->frame : 1);
	}
	else
	{
		DoRailCore(start, end, right, len, r_railCoreWidth->integer);
	}
}

//==========================================================================
//
//	RB_SurfaceBeam
//
//==========================================================================

static void DoRailDiscs(int numSegs, const vec3_t start, const vec3_t dir, const vec3_t right, const vec3_t up)
{
	if (numSegs > 1)
	{
		numSegs--;
	}
	if (!numSegs)
	{
		return;
	}

	float scale = 0.25;

	int spanWidth = r_railWidth->integer;
	vec3_t pos[4];
	for (int i = 0; i < 4; i++)
	{
		float c = cos(DEG2RAD(45 + i * 90));
		float s = sin(DEG2RAD(45 + i * 90));
		vec3_t v;
		v[0] = (right[0] * c + up[0] * s) * scale * spanWidth;
		v[1] = (right[1] * c + up[1] * s) * scale * spanWidth;
		v[2] = (right[2] * c + up[2] * s) * scale * spanWidth;
		VectorAdd(start, v, pos[i]);

		if (numSegs > 1)
		{
			// offset by 1 segment if we're doing a long distance shot
			VectorAdd(pos[i], dir, pos[i]);
		}
	}

	for (int i = 0; i < numSegs; i++)
	{
		RB_CHECKOVERFLOW(4, 6);

		for (int j = 0; j < 4; j++)
		{
			VectorCopy(pos[j], tess.xyz[tess.numVertexes]);
			tess.texCoords[tess.numVertexes][0][0] = (j < 2);
			tess.texCoords[tess.numVertexes][0][1] = (j && j != 3);
			tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
			tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
			tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
			tess.numVertexes++;

			VectorAdd(pos[j], dir, pos[j]);
		}

		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 0;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 1;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 3;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 3;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 1;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 2;
	}
}

//==========================================================================
//
//	RB_SurfaceBeam
//
//==========================================================================

static void RB_SurfaceRailRings()
{
	refEntity_t* e = &backEnd.currentEntity->e;

	vec3_t start, end;
	VectorCopy(e->oldorigin, start);
	VectorCopy(e->origin, end);

	// compute variables
	vec3_t vec;
	VectorSubtract(end, start, vec);
	int len = VectorNormalize(vec);
	vec3_t right, up;
	MakeNormalVectors(vec, right, up);
	int numSegs = len / r_railSegmentLength->value;
	if (numSegs <= 0)
	{
		numSegs = 1;
	}

	VectorScale(vec, r_railSegmentLength->value, vec);

	DoRailDiscs(numSegs, start, vec, right, up);
}

//==========================================================================
//
//	RB_SurfaceLightningBolt
//
//==========================================================================

static void RB_SurfaceLightningBolt()
{
	refEntity_t* e = &backEnd.currentEntity->e;

	vec3_t start, end;
	VectorCopy(e->oldorigin, end);
	VectorCopy(e->origin, start);

	// compute variables
	vec3_t vec;
	VectorSubtract(end, start, vec);
	int len = VectorNormalize(vec);

	// compute side vector
	vec3_t v1;
	VectorSubtract(start, backEnd.viewParms.orient.origin, v1);
	VectorNormalize(v1);
	vec3_t v2;
	VectorSubtract(end, backEnd.viewParms.orient.origin, v2);
	VectorNormalize(v2);
	vec3_t right;
	CrossProduct(v1, v2, right);
	VectorNormalize(right);

	for (int i = 0; i < 4; i++)
	{
		DoRailCore(start, end, right, len, 8);
		vec3_t temp;
		RotatePointAroundVector(temp, vec, right, 45);
		VectorCopy(temp, right);
	}
}

static void RB_SurfaceSplash()
{
	vec3_t left, up;
	float radius;

	// calculate the xyz locations for the four corners
	radius = backEnd.currentEntity->e.radius;

	VectorSet(left, -radius, 0, 0);
	VectorSet(up, 0, radius, 0);
	if (backEnd.viewParms.isMirror)
	{
		VectorSubtract(vec3_origin, left, left);
	}

	RB_AddQuadStamp(backEnd.currentEntity->e.origin, left, up, backEnd.currentEntity->e.shaderRGBA);
}

//==========================================================================
//
//	RB_SurfaceAxis
//
//	Draws x/y/z lines from the origin for orientation debugging
//
//==========================================================================

static void RB_SurfaceAxis()
{
	GL_Bind(tr.whiteImage);
	qglLineWidth(3);
	qglBegin(GL_LINES);
	qglColor3f(1, 0, 0);
	qglVertex3f(0, 0, 0);
	qglVertex3f(16, 0, 0);
	qglColor3f(0, 1, 0);
	qglVertex3f(0, 0, 0);
	qglVertex3f(0, 16, 0);
	qglColor3f(0, 0, 1);
	qglVertex3f(0, 0, 0);
	qglVertex3f(0, 0, 16);
	qglEnd();
	qglLineWidth(1);
}

//==========================================================================
//
//	RB_SurfaceEntity
//
//	Entities that have a single procedurally generated surface
//
//==========================================================================

static void RB_SurfaceEntity(surfaceType_t* surfType)
{
	switch (backEnd.currentEntity->e.reType)
	{
	case RT_SPRITE:
		RB_SurfaceSprite();
		break;

	case RT_BEAM:
		RB_SurfaceBeam();
		break;

	case RT_RAIL_CORE:
		RB_SurfaceRailCore();
		break;

	case RT_RAIL_RINGS:
		RB_SurfaceRailRings();
		break;

	case RT_LIGHTNING:
		RB_SurfaceLightningBolt();
		break;

	case RT_SPLASH:
		RB_SurfaceSplash();
		break;

	default:
		RB_SurfaceAxis();
		break;
	}
}

//==========================================================================
//
//	RB_SurfaceDisplayList
//
//==========================================================================

static void RB_SurfaceDisplayList(srfDisplayList_t* surf)
{
	// all apropriate state must be set in RB_BeginSurface
	// this isn't implemented yet...
	qglCallList(surf->listNum);
}

static void RB_SurfacePolyBuffer(srfPolyBuffer_t* surf)
{
	RB_EndSurface();

	RB_BeginSurface(tess.shader, tess.fogNum);

	// ===================================================
	//	Originally tess was pointed to different arrays.
	tess.numIndexes =   surf->pPolyBuffer->numIndicies;
	tess.numVertexes =  surf->pPolyBuffer->numVerts;

	Com_Memcpy(tess.xyz, surf->pPolyBuffer->xyz, tess.numVertexes * sizeof(vec4_t));
	for (int i = 0; i < tess.numVertexes; i++)
	{
		tess.texCoords[i][0][0] = surf->pPolyBuffer->st[i][0];
		tess.texCoords[i][0][1] = surf->pPolyBuffer->st[i][1];
	}
	Com_Memcpy(tess.indexes, surf->pPolyBuffer->indicies, tess.numIndexes * sizeof(glIndex_t));
	Com_Memcpy(tess.vertexColors, surf->pPolyBuffer->color, tess.numVertexes * sizeof(color4ub_t));
	// ===================================================

	RB_EndSurface();
}

static void RB_SurfaceDecal(srfDecal_t* srf)
{
	RB_CHECKOVERFLOW(srf->numVerts, 3 * (srf->numVerts - 2));

	// fan triangles into the tess array
	int numv = tess.numVertexes;
	for (int i = 0; i < srf->numVerts; i++)
	{
		VectorCopy(srf->verts[i].xyz, tess.xyz[numv]);
		tess.texCoords[numv][0][0] = srf->verts[i].st[0];
		tess.texCoords[numv][0][1] = srf->verts[i].st[1];
		*(int*)&tess.vertexColors[numv] = *(int*)srf->verts[i].modulate;
		numv++;
	}

	//	generate fan indexes into the tess array
	for (int i = 0; i < srf->numVerts - 2; i++)
	{
		tess.indexes[tess.numIndexes + 0] = tess.numVertexes;
		tess.indexes[tess.numIndexes + 1] = tess.numVertexes + i + 1;
		tess.indexes[tess.numIndexes + 2] = tess.numVertexes + i + 2;
		tess.numIndexes += 3;
	}

	tess.numVertexes = numv;
}

void(*rb_surfaceTable[SF_NUM_SURFACE_TYPES]) (void*) =
{
	(void (*)(void*))RB_SurfaceBad,			// SF_BAD,
	(void (*)(void*))RB_SurfaceSkip,		// SF_SKIP,
	(void (*)(void*))RB_SurfaceFace,		// SF_FACE,
	(void (*)(void*))RB_SurfaceGrid,		// SF_GRID,
	(void (*)(void*))RB_SurfaceTriangles,	// SF_TRIANGLES,
	(void (*)(void*))RB_SurfaceFoliage,		// SF_FOLIAGE,
	(void (*)(void*))RB_SurfacePolychain,	// SF_POLY,
	(void (*)(void*))RB_SurfaceMesh,		// SF_MD3,
	(void (*)(void*))RB_SurfaceAnim,		// SF_MD4,
	(void (*)(void*))RB_SurfaceCMesh,		// SF_MDC,
	(void (*)(void*))RB_SurfaceAnimMds,		// SF_MDS,
	(void (*)(void*))RB_MDM_SurfaceAnim,	// SF_MDM,
	(void (*)(void*))RB_SurfaceFlare,		// SF_FLARE,
	(void (*)(void*))RB_SurfaceEntity,		// SF_ENTITY
	(void (*)(void*))RB_SurfaceDisplayList,	// SF_DISPLAY_LIST
	(void (*)(void*))RB_SurfacePolyBuffer,	// SF_POLYBUFFER
	(void (*)(void*))RB_SurfaceDecal,		// SF_DECAL
};
