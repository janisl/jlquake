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

// HEADER FILES ------------------------------------------------------------

#include "../../client.h"
#include "../local.h"

// MACROS ------------------------------------------------------------------

#if 0
#define LIGHTMAP_SIZE		128

#define MAX_FACE_POINTS		64

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

world_t		s_worldData;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static byte*		fileBase;
#endif

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	HSVtoRGB
//
//==========================================================================

static void HSVtoRGB(float h, float s, float v, float rgb[3])
{
	h *= 5;

	int i = (int)floor(h);
	float f = h - i;

	float p = v * (1 - s);
	float q = v * (1 - s * f);
	float t = v * (1 - s * (1 - f));

	switch (i)
	{
	case 0:
		rgb[0] = v;
		rgb[1] = t;
		rgb[2] = p;
		break;
	case 1:
		rgb[0] = q;
		rgb[1] = v;
		rgb[2] = p;
		break;
	case 2:
		rgb[0] = p;
		rgb[1] = v;
		rgb[2] = t;
		break;
	case 3:
		rgb[0] = p;
		rgb[1] = q;
		rgb[2] = v;
		break;
	case 4:
		rgb[0] = t;
		rgb[1] = p;
		rgb[2] = v;
		break;
	case 5:
		rgb[0] = v;
		rgb[1] = p;
		rgb[2] = q;
		break;
	}
}

//==========================================================================
//
//	R_ColorShiftLightingBytes
//
//==========================================================================

//static 
void R_ColorShiftLightingBytes(byte in[4], byte out[4])
{
	// shift the color data based on overbright range
	int shift = r_mapOverBrightBits->integer - tr.overbrightBits;

	// shift the data based on overbright range
	int r = in[0] << shift;
	int g = in[1] << shift;
	int b = in[2] << shift;
	
	// normalize by color instead of saturating to white
	if ((r | g | b) > 255)
	{
		int max = r > g ? r : g;
		max = max > b ? max : b;
		r = r * 255 / max;
		g = g * 255 / max;
		b = b * 255 / max;
	}

	out[0] = r;
	out[1] = g;
	out[2] = b;
	out[3] = in[3];
}

float R_ProcessLightmap(byte* buf_p, int in_padding, int width, int height, byte* image)
{
	float maxIntensity = 0;
	if (r_lightmap->integer > 1)
	{
		// color code by intensity as development tool	(FIXME: check range)
		for (int j = 0; j < width * height; j++)
		{
			float r = buf_p[j * in_padding + 0];
			float g = buf_p[j * in_padding + 1];
			float b = buf_p[j * in_padding + 2];
			float intensity;
			float out[3];

			intensity = 0.33f * r + 0.685f * g + 0.063f * b;

			if (intensity > 255)
			{
				intensity = 1.0f;
			}
			else
			{
				intensity /= 255.0f;
			}

			if (intensity > maxIntensity)
			{
				maxIntensity = intensity;
			}

			HSVtoRGB(intensity, 1.00, 0.50, out);

			if (r_lightmap->integer == 3)
			{
				// Arnout: artists wanted the colours to be inversed
				image[j * 4 + 0] = out[2] * 255;
				image[j * 4 + 1] = out[1] * 255;
				image[j * 4 + 2] = out[0] * 255;
			}
			else
			{
				image[j * 4 + 0] = out[0] * 255;
				image[j * 4 + 1] = out[1] * 255;
				image[j * 4 + 2] = out[2] * 255;
				image[j * 4 + 3] = 255;
			}
		}
	}
	else
	{
		for (int j = 0; j < width * height; j++)
		{
			R_ColorShiftLightingBytes(&buf_p[j * in_padding], &image[j * 4]);
			image[j * 4 + 3] = 255;
		}
	}
	return maxIntensity;
}

#if 0
//==========================================================================
//
//	R_ColorShiftLightingBytes
//
//==========================================================================

static void R_LoadLightmaps(bsp46_lump_t* l)
{
    int len = l->filelen;
	if (!len)
	{
		return;
	}
	byte* buf = fileBase + l->fileofs;

	// we are about to upload textures
	R_SyncRenderThread();

	// create all the lightmaps
	tr.numLightmaps = len / (LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3);
	if (tr.numLightmaps == 1)
	{
		//FIXME: HACK: maps with only one lightmap turn up fullbright for some reason.
		//this avoids this, but isn't the correct solution.
		tr.numLightmaps++;
	}

	// if we are in r_vertexLight mode, we don't need the lightmaps at all
	if (r_vertexLight->integer)
	{
		return;
	}

	float maxIntensity = 0;
	for (int i = 0; i < tr.numLightmaps; i++)
	{
		// expand the 24 bit on-disk to 32 bit
		byte* buf_p = buf + i * LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3;

		byte image[LIGHTMAP_SIZE * LIGHTMAP_SIZE * 4];
		float intensity = R_ProcessLightmap(buf_p, 3, LIGHTMAP_SIZE, LIGHTMAP_SIZE, image);
		if (intensity > maxIntensity)
		{
			maxIntensity = intensity;
		}
		tr.lightmaps[i] = R_CreateImage(va("*lightmap%d",i), image, 
			LIGHTMAP_SIZE, LIGHTMAP_SIZE, false, false, GL_CLAMP, false);
	}

	if (r_lightmap->integer == 2)
	{
		Log::write("Brightest lightmap value: %d\n", (int)(maxIntensity * 255));
	}
}

//==========================================================================
//
//	R_LoadVisibility
//
//==========================================================================

static void R_LoadVisibility(bsp46_lump_t* l)
{
	int len = (s_worldData.numClusters + 63) & ~63;
	s_worldData.novis = new byte[len];
	Com_Memset(s_worldData.novis, 0xff, len);

    len = l->filelen;
	if (!len)
	{
		return;
	}
	byte* buf = fileBase + l->fileofs;

	s_worldData.numClusters = LittleLong(((int*)buf)[0]);
	s_worldData.clusterBytes = LittleLong(((int*)buf)[1]);

	byte* dest = new byte[len - 8];
	Com_Memcpy(dest, buf + 8, len - 8);
	s_worldData.vis = dest;
}

//==========================================================================
//
//	ShaderForShaderNum
//
//==========================================================================

static shader_t* ShaderForShaderNum(int shaderNum, int lightmapNum)
{
	shaderNum = LittleLong(shaderNum);
	if (shaderNum < 0 || shaderNum >= s_worldData.numShaders)
	{
		throw DropException(va("ShaderForShaderNum: bad num %i", shaderNum));
	}
	bsp46_dshader_t* dsh = &s_worldData.shaders[shaderNum];

	if (r_vertexLight->integer)
	{
		lightmapNum = LIGHTMAP_BY_VERTEX;
	}

	if (r_fullbright->integer)
	{
		lightmapNum = LIGHTMAP_WHITEIMAGE;
	}

	shader_t* shader = R_FindShader(dsh->shader, lightmapNum, true);

	// if the shader had errors, just use default shader
	if (shader->defaultShader)
	{
		return tr.defaultShader;
	}

	return shader;
}

//==========================================================================
//
//	ParseFace
//
//==========================================================================

static void ParseFace(bsp46_dsurface_t* ds, bsp46_drawVert_t* verts, mbrush46_surface_t* surf, int* indexes)
{
	int lightmapNum = LittleLong(ds->lightmapNum);

	// get fog volume
	surf->fogIndex = LittleLong(ds->fogNum) + 1;

	// get shader value
	surf->shader = ShaderForShaderNum(ds->shaderNum, lightmapNum);
	if (r_singleShader->integer && !surf->shader->isSky)
	{
		surf->shader = tr.defaultShader;
	}

	int numPoints = LittleLong(ds->numVerts);
	if (numPoints > MAX_FACE_POINTS)
	{
		Log::write(S_COLOR_YELLOW "WARNING: MAX_FACE_POINTS exceeded: %i\n", numPoints);
		numPoints = MAX_FACE_POINTS;
		surf->shader = tr.defaultShader;
	}

	int numIndexes = LittleLong(ds->numIndexes);

	// create the srfSurfaceFace_t
	int sfaceSize = (qintptr)&((srfSurfaceFace_t *)0)->points[numPoints];
	int ofsIndexes = sfaceSize;
	sfaceSize += sizeof(int) * numIndexes;

	srfSurfaceFace_t* cv = (srfSurfaceFace_t*)Mem_Alloc(sfaceSize);
	cv->surfaceType = SF_FACE;
	cv->numPoints = numPoints;
	cv->numIndices = numIndexes;
	cv->ofsIndices = ofsIndexes;

	verts += LittleLong(ds->firstVert);
	for (int i = 0; i < numPoints; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			cv->points[i][j] = LittleFloat(verts[i].xyz[j]);
		}
		for (int j = 0; j < 2; j++)
		{
			cv->points[i][3 + j] = LittleFloat(verts[i].st[j]);
			cv->points[i][5 + j] = LittleFloat(verts[i].lightmap[j]);
		}
		R_ColorShiftLightingBytes(verts[i].color, (byte*)&cv->points[i][7]);
	}

	indexes += LittleLong(ds->firstIndex);
	for (int i = 0; i < numIndexes; i++)
	{
		((int*)((byte*)cv + cv->ofsIndices))[i] = LittleLong(indexes[i]);
	}

	// take the plane information from the lightmap vector
	for (int i = 0; i < 3; i++)
	{
		cv->plane.normal[i] = LittleFloat(ds->lightmapVecs[2][i]);
	}
	cv->plane.dist = DotProduct(cv->points[0], cv->plane.normal);
	SetPlaneSignbits(&cv->plane);
	cv->plane.type = PlaneTypeForNormal(cv->plane.normal);

	surf->data = (surfaceType_t*)cv;
}

//==========================================================================
//
//	ParseMesh
//
//==========================================================================

static void ParseMesh(bsp46_dsurface_t* ds, bsp46_drawVert_t* verts, mbrush46_surface_t* surf)
{
	int lightmapNum = LittleLong(ds->lightmapNum);

	// get fog volume
	surf->fogIndex = LittleLong(ds->fogNum) + 1;

	// get shader value
	surf->shader = ShaderForShaderNum(ds->shaderNum, lightmapNum);
	if (r_singleShader->integer && !surf->shader->isSky)
	{
		surf->shader = tr.defaultShader;
	}

	// we may have a nodraw surface, because they might still need to
	// be around for movement clipping
	if (s_worldData.shaders[LittleLong(ds->shaderNum)].surfaceFlags & BSP46SURF_NODRAW)
	{
		static surfaceType_t skipData = SF_SKIP;

		surf->data = &skipData;
		return;
	}

	int width = LittleLong(ds->patchWidth);
	int height = LittleLong(ds->patchHeight);

	verts += LittleLong(ds->firstVert);
	int numPoints = width * height;
	bsp46_drawVert_t points[MAX_PATCH_SIZE * MAX_PATCH_SIZE];
	for (int i = 0; i < numPoints; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			points[i].xyz[j] = LittleFloat(verts[i].xyz[j]);
			points[i].normal[j] = LittleFloat(verts[i].normal[j]);
		}
		for (int j = 0; j < 2; j++)
		{
			points[i].st[j] = LittleFloat(verts[i].st[j]);
			points[i].lightmap[j] = LittleFloat(verts[i].lightmap[j]);
		}
		R_ColorShiftLightingBytes(verts[i].color, points[i].color);
	}

	// pre-tesseleate
	srfGridMesh_t* grid = R_SubdividePatchToGrid(width, height, points);
	surf->data = (surfaceType_t*)grid;

	// copy the level of detail origin, which is the center
	// of the group of all curves that must subdivide the same
	// to avoid cracking
	vec3_t bounds[2];
	for (int i = 0; i < 3; i++)
	{
		bounds[0][i] = LittleFloat(ds->lightmapVecs[0][i]);
		bounds[1][i] = LittleFloat(ds->lightmapVecs[1][i]);
	}
	VectorAdd(bounds[0], bounds[1], bounds[1]);
	VectorScale(bounds[1], 0.5f, grid->lodOrigin);
	vec3_t tmpVec;
	VectorSubtract(bounds[0], grid->lodOrigin, tmpVec);
	grid->lodRadius = VectorLength(tmpVec);
}

//==========================================================================
//
//	ParseTriSurf
//
//==========================================================================

static void ParseTriSurf(bsp46_dsurface_t* ds, bsp46_drawVert_t* verts, mbrush46_surface_t* surf, int* indexes)
{
	// get fog volume
	surf->fogIndex = LittleLong(ds->fogNum) + 1;

	// get shader
	surf->shader = ShaderForShaderNum(ds->shaderNum, LIGHTMAP_BY_VERTEX);
	if (r_singleShader->integer && !surf->shader->isSky)
	{
		surf->shader = tr.defaultShader;
	}

	int numVerts = LittleLong(ds->numVerts);
	int numIndexes = LittleLong(ds->numIndexes);

	srfTriangles_t* tri = (srfTriangles_t*)Mem_Alloc(sizeof(*tri) + numVerts * sizeof(tri->verts[0]) +
		numIndexes * sizeof(tri->indexes[0]));
	tri->surfaceType = SF_TRIANGLES;
	tri->numVerts = numVerts;
	tri->numIndexes = numIndexes;
	tri->verts = (bsp46_drawVert_t*)(tri + 1);
	tri->indexes = (int*)(tri->verts + tri->numVerts);

	surf->data = (surfaceType_t *)tri;

	// copy vertexes
	ClearBounds(tri->bounds[0], tri->bounds[1]);
	verts += LittleLong(ds->firstVert);
	for (int i = 0; i < numVerts; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			tri->verts[i].xyz[j] = LittleFloat(verts[i].xyz[j]);
			tri->verts[i].normal[j] = LittleFloat(verts[i].normal[j]);
		}
		AddPointToBounds(tri->verts[i].xyz, tri->bounds[0], tri->bounds[1]);
		for (int j = 0; j < 2; j++)
		{
			tri->verts[i].st[j] = LittleFloat(verts[i].st[j]);
			tri->verts[i].lightmap[j] = LittleFloat(verts[i].lightmap[j]);
		}

		R_ColorShiftLightingBytes(verts[i].color, tri->verts[i].color);
	}

	// copy indexes
	indexes += LittleLong(ds->firstIndex);
	for (int i = 0; i < numIndexes; i++)
	{
		tri->indexes[i] = LittleLong( indexes[i] );
		if (tri->indexes[i] < 0 || tri->indexes[i] >= numVerts)
		{
			throw DropException("Bad index in triangle surface");
		}
	}
}

//==========================================================================
//
//	ParseFlare
//
//==========================================================================

static void ParseFlare(bsp46_dsurface_t* ds, bsp46_drawVert_t* verts, mbrush46_surface_t* surf, int* indexes)
{
	// get fog volume
	surf->fogIndex = LittleLong(ds->fogNum) + 1;

	// get shader
	surf->shader = ShaderForShaderNum(ds->shaderNum, LIGHTMAP_BY_VERTEX);
	if (r_singleShader->integer && !surf->shader->isSky)
	{
		surf->shader = tr.defaultShader;
	}

	srfFlare_t* flare = new srfFlare_t();
	flare->surfaceType = SF_FLARE;

	surf->data = (surfaceType_t*)flare;

	for (int i = 0; i < 3 ; i++)
	{
		flare->origin[i] = LittleFloat(ds->lightmapOrigin[i]);
		flare->color[i] = LittleFloat(ds->lightmapVecs[0][i]);
		flare->normal[i] = LittleFloat(ds->lightmapVecs[2][i]);
	}
}

//==========================================================================
//
//	R_MergedWidthPoints
//
//	returns true if there are grid points merged on a width edge
//
//==========================================================================

static bool R_MergedWidthPoints(srfGridMesh_t* grid, int offset)
{
	for (int i = 1; i < grid->width - 1; i++)
	{
		for (int j = i + 1; j < grid->width - 1; j++)
		{
			if (fabs(grid->verts[i + offset].xyz[0] - grid->verts[j + offset].xyz[0]) > .1)
			{
				continue;
			}
			if (fabs(grid->verts[i + offset].xyz[1] - grid->verts[j + offset].xyz[1]) > .1)
			{
				continue;
			}
			if (fabs(grid->verts[i + offset].xyz[2] - grid->verts[j + offset].xyz[2]) > .1)
			{
				continue;
			}
			return true;
		}
	}
	return false;
}

//==========================================================================
//
//	R_MergedHeightPoints
//
//	returns true if there are grid points merged on a height edge
//
//==========================================================================

static bool R_MergedHeightPoints(srfGridMesh_t* grid, int offset)
{
	for (int i = 1; i < grid->height - 1; i++)
	{
		for (int j = i + 1; j < grid->height - 1; j++)
		{
			if (fabs(grid->verts[grid->width * i + offset].xyz[0] - grid->verts[grid->width * j + offset].xyz[0]) > .1)
			{
				continue;
			}
			if (fabs(grid->verts[grid->width * i + offset].xyz[1] - grid->verts[grid->width * j + offset].xyz[1]) > .1)
			{
				continue;
			}
			if (fabs(grid->verts[grid->width * i + offset].xyz[2] - grid->verts[grid->width * j + offset].xyz[2]) > .1)
			{
				continue;
			}
			return true;
		}
	}
	return false;
}

//==========================================================================
//
//	R_FixSharedVertexLodError_r
//
//	NOTE: never sync LoD through grid edges with merged points!
//
//	FIXME: write generalized version that also avoids cracks between a patch and one that meets half way?
//
//==========================================================================

static void R_FixSharedVertexLodError_r(int start, srfGridMesh_t* grid1)
{
	int k, l, m, n, offset1, offset2;

	for (int j = start; j < s_worldData.numsurfaces; j++)
	{
		//
		srfGridMesh_t* grid2 = (srfGridMesh_t*)s_worldData.surfaces[j].data;
		// if this surface is not a grid
		if (grid2->surfaceType != SF_GRID)
		{
			continue;
		}
		// if the LOD errors are already fixed for this patch
		if (grid2->lodFixed == 2)
		{
			continue;
		}
		// grids in the same LOD group should have the exact same lod radius
		if (grid1->lodRadius != grid2->lodRadius)
		{
			continue;
		}
		// grids in the same LOD group should have the exact same lod origin
		if (grid1->lodOrigin[0] != grid2->lodOrigin[0])
		{
			continue;
		}
		if (grid1->lodOrigin[1] != grid2->lodOrigin[1])
		{
			continue;
		}
		if (grid1->lodOrigin[2] != grid2->lodOrigin[2])
		{
			continue;
		}
		//
		bool touch = false;
		for (n = 0; n < 2; n++)
		{
			//
			if (n)
			{
				offset1 = (grid1->height - 1) * grid1->width;
			}
			else
			{
				offset1 = 0;
			}
			if (R_MergedWidthPoints(grid1, offset1))
			{
				continue;
			}
			for (k = 1; k < grid1->width-1; k++)
			{
				for (m = 0; m < 2; m++)
				{
					if (m)
					{
						offset2 = (grid2->height-1) * grid2->width;
					}
					else
					{
						offset2 = 0;
					}
					if (R_MergedWidthPoints(grid2, offset2))
					{
						continue;
					}
					for (l = 1; l < grid2->width-1; l++)
					{
						if (fabs(grid1->verts[k + offset1].xyz[0] - grid2->verts[l + offset2].xyz[0]) > .1)
						{
							continue;
						}
						if (fabs(grid1->verts[k + offset1].xyz[1] - grid2->verts[l + offset2].xyz[1]) > .1)
						{
							continue;
						}
						if (fabs(grid1->verts[k + offset1].xyz[2] - grid2->verts[l + offset2].xyz[2]) > .1)
						{
							continue;
						}
						// ok the points are equal and should have the same lod error
						grid2->widthLodError[l] = grid1->widthLodError[k];
						touch = true;
					}
				}
				for (m = 0; m < 2; m++)
				{
					if (m)
					{
						offset2 = grid2->width - 1;
					}
					else
					{
						offset2 = 0;
					}
					if (R_MergedHeightPoints(grid2, offset2))
					{
						continue;
					}
					for (l = 1; l < grid2->height - 1; l++)
					{
						if (fabs(grid1->verts[k + offset1].xyz[0] - grid2->verts[grid2->width * l + offset2].xyz[0]) > .1)
						{
							continue;
						}
						if (fabs(grid1->verts[k + offset1].xyz[1] - grid2->verts[grid2->width * l + offset2].xyz[1]) > .1)
						{
							continue;
						}
						if (fabs(grid1->verts[k + offset1].xyz[2] - grid2->verts[grid2->width * l + offset2].xyz[2]) > .1)
						{
							continue;
						}
						// ok the points are equal and should have the same lod error
						grid2->heightLodError[l] = grid1->widthLodError[k];
						touch = true;
					}
				}
			}
		}
		for (n = 0; n < 2; n++)
		{
			if (n)
			{
				offset1 = grid1->width - 1;
			}
			else
			{
				offset1 = 0;
			}
			if (R_MergedHeightPoints(grid1, offset1))
			{
				continue;
			}
			for (k = 1; k < grid1->height-1; k++)
			{
				for (m = 0; m < 2; m++)
				{
					if (m)
					{
						offset2 = (grid2->height-1) * grid2->width;
					}
					else
					{
						offset2 = 0;
					}
					if (R_MergedWidthPoints(grid2, offset2))
					{
						continue;
					}
					for (l = 1; l < grid2->width - 1; l++)
					{
						if (fabs(grid1->verts[grid1->width * k + offset1].xyz[0] - grid2->verts[l + offset2].xyz[0]) > .1)
						{
							continue;
						}
						if (fabs(grid1->verts[grid1->width * k + offset1].xyz[1] - grid2->verts[l + offset2].xyz[1]) > .1)
						{
							continue;
						}
						if (fabs(grid1->verts[grid1->width * k + offset1].xyz[2] - grid2->verts[l + offset2].xyz[2]) > .1)
						{
							continue;
						}
						// ok the points are equal and should have the same lod error
						grid2->widthLodError[l] = grid1->heightLodError[k];
						touch = true;
					}
				}
				for (m = 0; m < 2; m++)
				{
					if (m)
					{
						offset2 = grid2->width - 1;
					}
					else
					{
						offset2 = 0;
					}
					if (R_MergedHeightPoints(grid2, offset2))
					{
						continue;
					}
					for (l = 1; l < grid2->height-1; l++)
					{
						if (fabs(grid1->verts[grid1->width * k + offset1].xyz[0] - grid2->verts[grid2->width * l + offset2].xyz[0]) > .1)
						{
							continue;
						}
						if (fabs(grid1->verts[grid1->width * k + offset1].xyz[1] - grid2->verts[grid2->width * l + offset2].xyz[1]) > .1)
						{
							continue;
						}
						if (fabs(grid1->verts[grid1->width * k + offset1].xyz[2] - grid2->verts[grid2->width * l + offset2].xyz[2]) > .1)
						{
							continue;
						}
						// ok the points are equal and should have the same lod error
						grid2->heightLodError[l] = grid1->heightLodError[k];
						touch = true;
					}
				}
			}
		}
		if (touch)
		{
			grid2->lodFixed = 2;
			R_FixSharedVertexLodError_r( start, grid2);
			//NOTE: this would be correct but makes things really slow
			//grid2->lodFixed = 1;
		}
	}
}

//==========================================================================
//
//	R_FixSharedVertexLodError
//
//	This function assumes that all patches in one group are nicely stitched
// together for the highest LoD. If this is not the case this function will
// still do its job but won't fix the highest LoD cracks.
//
//==========================================================================

static void R_FixSharedVertexLodError()
{
	for (int i = 0; i < s_worldData.numsurfaces; i++)
	{
		srfGridMesh_t* grid1 = (srfGridMesh_t*) s_worldData.surfaces[i].data;
		// if this surface is not a grid
		if (grid1->surfaceType != SF_GRID)
		{
			continue;
		}
		if (grid1->lodFixed)
		{
			continue;
		}
		//
		grid1->lodFixed = 2;
		// recursively fix other patches in the same LOD group
		R_FixSharedVertexLodError_r(i + 1, grid1);
	}
}

//==========================================================================
//
//	R_StitchPatches
//
//==========================================================================

static bool R_StitchPatches(int grid1num, int grid2num)
{
	srfGridMesh_t* grid1 = (srfGridMesh_t*)s_worldData.surfaces[grid1num].data;
	srfGridMesh_t* grid2 = (srfGridMesh_t*)s_worldData.surfaces[grid2num].data;
	for (int n = 0; n < 2; n++)
	{
		int offset1;
		if (n)
		{
			offset1 = (grid1->height-1) * grid1->width;
		}
		else
		{
			offset1 = 0;
		}
		if (R_MergedWidthPoints(grid1, offset1))
		{
			continue;
		}
		for (int k = 0; k < grid1->width-2; k += 2)
		{
			for (int m = 0; m < 2; m++)
			{
				if (grid2->width >= MAX_GRID_SIZE)
				{
					break;
				}
				int offset2;
				if (m)
				{
					offset2 = (grid2->height-1) * grid2->width;
				}
				else
				{
					offset2 = 0;
				}
				for (int l = 0; l < grid2->width-1; l++)
				{
					float *v1 = grid1->verts[k + offset1].xyz;
					float *v2 = grid2->verts[l + offset2].xyz;
					if (fabs(v1[0] - v2[0]) > .1)
					{
						continue;
					}
					if (fabs(v1[1] - v2[1]) > .1)
					{
						continue;
					}
					if (fabs(v1[2] - v2[2]) > .1)
					{
						continue;
					}

					v1 = grid1->verts[k + 2 + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if (fabs(v1[0] - v2[0]) > .1)
					{
						continue;
					}
					if (fabs(v1[1] - v2[1]) > .1)
					{
						continue;
					}
					if (fabs(v1[2] - v2[2]) > .1)
					{
						continue;
					}
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if (fabs(v1[0] - v2[0]) < .01 &&
						fabs(v1[1] - v2[1]) < .01 &&
						fabs(v1[2] - v2[2]) < .01)
					{
						continue;
					}
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					int row;
					if (m)
					{
						row = grid2->height - 1;
					}
					else
					{
						row = 0;
					}
					grid2 = R_GridInsertColumn(grid2, l + 1, row,
						grid1->verts[k + 1 + offset1].xyz, grid1->widthLodError[k + 1]);
					grid2->lodStitched = false;
					s_worldData.surfaces[grid2num].data = (surfaceType_t*)grid2;
					return true;
				}
			}
			for (int m = 0; m < 2; m++)
			{
				if (grid2->height >= MAX_GRID_SIZE)
				{
					break;
				}
				int offset2;
				if (m)
				{
					offset2 = grid2->width - 1;
				}
				else
				{
					offset2 = 0;
				}
				for (int l = 0; l < grid2->height - 1; l++)
				{
					float *v1 = grid1->verts[k + offset1].xyz;
					float *v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if (fabs(v1[0] - v2[0]) > .1)
					{
						continue;
					}
					if (fabs(v1[1] - v2[1]) > .1)
					{
						continue;
					}
					if (fabs(v1[2] - v2[2]) > .1)
					{
						continue;
					}

					v1 = grid1->verts[k + 2 + offset1].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if (fabs(v1[0] - v2[0]) > .1)
					{
						continue;
					}
					if (fabs(v1[1] - v2[1]) > .1)
					{
						continue;
					}
					if (fabs(v1[2] - v2[2]) > .1)
					{
						continue;
					}
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if (fabs(v1[0] - v2[0]) < .01 &&
						fabs(v1[1] - v2[1]) < .01 &&
						fabs(v1[2] - v2[2]) < .01)
					{
						continue;
					}
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					int column;
					if (m)
					{
						column = grid2->width - 1;
					}
					else
					{
						column = 0;
					}
					grid2 = R_GridInsertRow(grid2, l + 1, column,
						grid1->verts[k + 1 + offset1].xyz, grid1->widthLodError[k + 1]);
					grid2->lodStitched = false;
					s_worldData.surfaces[grid2num].data = (surfaceType_t*)grid2;
					return true;
				}
			}
		}
	}
	for (int n = 0; n < 2; n++)
	{
		int offset1;
		if (n)
		{
			offset1 = grid1->width - 1;
		}
		else
		{
			offset1 = 0;
		}
		if (R_MergedHeightPoints(grid1, offset1))
		{
			continue;
		}
		for (int k = 0; k < grid1->height - 2; k += 2)
		{
			for (int m = 0; m < 2; m++)
			{
				if (grid2->width >= MAX_GRID_SIZE)
				{
					break;
				}
				int offset2;
				if (m)
				{
					offset2 = (grid2->height - 1) * grid2->width;
				}
				else
				{
					offset2 = 0;
				}
				for (int l = 0; l < grid2->width - 1; l++)
				{
					float *v1 = grid1->verts[grid1->width * k + offset1].xyz;
					float *v2 = grid2->verts[l + offset2].xyz;
					if (fabs(v1[0] - v2[0]) > .1)
					{
						continue;
					}
					if (fabs(v1[1] - v2[1]) > .1)
					{
						continue;
					}
					if (fabs(v1[2] - v2[2]) > .1)
					{
						continue;
					}

					v1 = grid1->verts[grid1->width * (k + 2) + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if (fabs(v1[0] - v2[0]) > .1)
					{
						continue;
					}
					if (fabs(v1[1] - v2[1]) > .1)
					{
						continue;
					}
					if (fabs(v1[2] - v2[2]) > .1)
					{
						continue;
					}
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[(l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
						fabs(v1[1] - v2[1]) < .01 &&
						fabs(v1[2] - v2[2]) < .01)
					{
						continue;
					}
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					int row;
					if (m)
					{
						row = grid2->height - 1;
					}
					else
					{
						row = 0;
					}
					grid2 = R_GridInsertColumn(grid2, l + 1, row,
						grid1->verts[grid1->width * (k + 1) + offset1].xyz, grid1->heightLodError[k + 1]);
					grid2->lodStitched = false;
					s_worldData.surfaces[grid2num].data = (surfaceType_t*)grid2;
					return true;
				}
			}
			for (int m = 0; m < 2; m++)
			{
				if (grid2->height >= MAX_GRID_SIZE)
				{
					break;
				}
				int offset2;
				if (m)
				{
					offset2 = grid2->width - 1;
				}
				else
				{
					offset2 = 0;
				}
				for (int l = 0; l < grid2->height - 1; l++)
				{
					float *v1 = grid1->verts[grid1->width * k + offset1].xyz;
					float *v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if (fabs(v1[0] - v2[0]) > .1)
					{
						continue;
					}
					if (fabs(v1[1] - v2[1]) > .1)
					{
						continue;
					}
					if (fabs(v1[2] - v2[2]) > .1)
					{
						continue;
					}

					v1 = grid1->verts[grid1->width * (k + 2) + offset1].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if (fabs(v1[0] - v2[0]) > .1)
					{
						continue;
					}
					if (fabs(v1[1] - v2[1]) > .1)
					{
						continue;
					}
					if (fabs(v1[2] - v2[2]) > .1)
					{
						continue;
					}
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if (fabs(v1[0] - v2[0]) < .01 &&
						fabs(v1[1] - v2[1]) < .01 &&
						fabs(v1[2] - v2[2]) < .01)
					{
						continue;
					}
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					int column;
					if (m)
					{
						column = grid2->width - 1;
					}
					else
					{
						column = 0;
					}
					grid2 = R_GridInsertRow(grid2, l + 1, column,
						grid1->verts[grid1->width * (k + 1) + offset1].xyz, grid1->heightLodError[k + 1]);
					grid2->lodStitched = false;
					s_worldData.surfaces[grid2num].data = (surfaceType_t*)grid2;
					return true;
				}
			}
		}
	}
	for (int n = 0; n < 2; n++)
	{
		int offset1;
		if (n)
		{
			offset1 = (grid1->height - 1) * grid1->width;
		}
		else
		{
			offset1 = 0;
		}
		if (R_MergedWidthPoints(grid1, offset1))
		{
			continue;
		}
		for (int k = grid1->width - 1; k > 1; k -= 2)
		{
			for (int m = 0; m < 2; m++)
			{
				if (grid2->width >= MAX_GRID_SIZE)
				{
					break;
				}
				int offset2;
				if (m)
				{
					offset2 = (grid2->height - 1) * grid2->width;
				}
				else
				{
					offset2 = 0;
				}
				for (int l = 0; l < grid2->width - 1; l++)
				{
					float *v1 = grid1->verts[k + offset1].xyz;
					float *v2 = grid2->verts[l + offset2].xyz;
					if (fabs(v1[0] - v2[0]) > .1)
					{
						continue;
					}
					if (fabs(v1[1] - v2[1]) > .1)
					{
						continue;
					}
					if (fabs(v1[2] - v2[2]) > .1)
					{
						continue;
					}

					v1 = grid1->verts[k - 2 + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if (fabs(v1[0] - v2[0]) > .1)
					{
						continue;
					}
					if (fabs(v1[1] - v2[1]) > .1)
					{
						continue;
					}
					if (fabs(v1[2] - v2[2]) > .1)
					{
						continue;
					}
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[(l + 1) + offset2].xyz;
					if (fabs(v1[0] - v2[0]) < .01 &&
						fabs(v1[1] - v2[1]) < .01 &&
						fabs(v1[2] - v2[2]) < .01)
					{
						continue;
					}
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					int row;
					if (m)
					{
						row = grid2->height - 1;
					}
					else
					{
						row = 0;
					}
					grid2 = R_GridInsertColumn(grid2, l + 1, row,
						grid1->verts[k - 1 + offset1].xyz, grid1->widthLodError[k + 1]);
					grid2->lodStitched = false;
					s_worldData.surfaces[grid2num].data = (surfaceType_t*)grid2;
					return true;
				}
			}
			for (int m = 0; m < 2; m++)
			{
				if (grid2->height >= MAX_GRID_SIZE)
				{
					break;
				}
				int offset2;
				if (m)
				{
					offset2 = grid2->width - 1;
				}
				else
				{
					offset2 = 0;
				}
				for (int l = 0; l < grid2->height - 1; l++)
				{
					float *v1 = grid1->verts[k + offset1].xyz;
					float *v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if (fabs(v1[0] - v2[0]) > .1)
					{
						continue;
					}
					if (fabs(v1[1] - v2[1]) > .1)
					{
						continue;
					}
					if (fabs(v1[2] - v2[2]) > .1)
					{
						continue;
					}

					v1 = grid1->verts[k - 2 + offset1].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if (fabs(v1[0] - v2[0]) > .1)
					{
						continue;
					}
					if (fabs(v1[1] - v2[1]) > .1)
					{
						continue;
					}
					if (fabs(v1[2] - v2[2]) > .1)
					{
						continue;
					}
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if (fabs(v1[0] - v2[0]) < .01 &&
						fabs(v1[1] - v2[1]) < .01 &&
						fabs(v1[2] - v2[2]) < .01)
					{
						continue;
					}
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					int column;
					if (m)
					{
						column = grid2->width - 1;
					}
					else
					{
						column = 0;
					}
					grid2 = R_GridInsertRow(grid2, l + 1, column,
						grid1->verts[k - 1 + offset1].xyz, grid1->widthLodError[k + 1]);
					if (!grid2)
					{
						break;
					}
					grid2->lodStitched = false;
					s_worldData.surfaces[grid2num].data = (surfaceType_t*)grid2;
					return true;
				}
			}
		}
	}
	for (int n = 0; n < 2; n++)
	{
		int offset1;
		if (n)
		{
			offset1 = grid1->width - 1;
		}
		else
		{
			offset1 = 0;
		}
		if (R_MergedHeightPoints(grid1, offset1))
		{
			continue;
		}
		for (int k = grid1->height - 1; k > 1; k -= 2)
		{
			for (int m = 0; m < 2; m++)
			{
				if (grid2->width >= MAX_GRID_SIZE)
				{
					break;
				}
				int offset2;
				if (m)
				{
					offset2 = (grid2->height-1) * grid2->width;
				}
				else
				{
					offset2 = 0;
				}
				for (int l = 0; l < grid2->width - 1; l++)
				{
					float *v1 = grid1->verts[grid1->width * k + offset1].xyz;
					float *v2 = grid2->verts[l + offset2].xyz;
					if (fabs(v1[0] - v2[0]) > .1)
					{
						continue;
					}
					if (fabs(v1[1] - v2[1]) > .1)
					{
						continue;
					}
					if (fabs(v1[2] - v2[2]) > .1)
					{
						continue;
					}

					v1 = grid1->verts[grid1->width * (k - 2) + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if (fabs(v1[0] - v2[0]) > .1)
					{
						continue;
					}
					if (fabs(v1[1] - v2[1]) > .1)
					{
						continue;
					}
					if (fabs(v1[2] - v2[2]) > .1)
					{
						continue;
					}
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[(l + 1) + offset2].xyz;
					if (fabs(v1[0] - v2[0]) < .01 &&
						fabs(v1[1] - v2[1]) < .01 &&
						fabs(v1[2] - v2[2]) < .01)
					{
						continue;
					}
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					int row;
					if (m)
					{
						row = grid2->height - 1;
					}
					else
					{
						row = 0;
					}
					grid2 = R_GridInsertColumn(grid2, l + 1, row,
						grid1->verts[grid1->width * (k - 1) + offset1].xyz, grid1->heightLodError[k + 1]);
					grid2->lodStitched = false;
					s_worldData.surfaces[grid2num].data = (surfaceType_t*)grid2;
					return true;
				}
			}
			for (int m = 0; m < 2; m++)
			{
				if (grid2->height >= MAX_GRID_SIZE)
				{
					break;
				}
				int offset2;
				if (m)
				{
					offset2 = grid2->width - 1;
				}
				else
				{
					offset2 = 0;
				}
				for (int l = 0; l < grid2->height - 1; l++)
				{
					float *v1 = grid1->verts[grid1->width * k + offset1].xyz;
					float *v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if (fabs(v1[0] - v2[0]) > .1)
					{
						continue;
					}
					if (fabs(v1[1] - v2[1]) > .1)
					{
						continue;
					}
					if (fabs(v1[2] - v2[2]) > .1)
					{
						continue;
					}

					v1 = grid1->verts[grid1->width * (k - 2) + offset1].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if (fabs(v1[0] - v2[0]) > .1)
					{
						continue;
					}
					if (fabs(v1[1] - v2[1]) > .1)
					{
						continue;
					}
					if (fabs(v1[2] - v2[2]) > .1)
					{
						continue;
					}
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if (fabs(v1[0] - v2[0]) < .01 &&
						fabs(v1[1] - v2[1]) < .01 &&
						fabs(v1[2] - v2[2]) < .01)
					{
						continue;
					}
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					int column;
					if (m)
					{
						column = grid2->width - 1;
					}
					else
					{
						column = 0;
					}
					grid2 = R_GridInsertRow(grid2, l + 1, column,
						grid1->verts[grid1->width * (k - 1) + offset1].xyz, grid1->heightLodError[k + 1]);
					grid2->lodStitched = false;
					s_worldData.surfaces[grid2num].data = (surfaceType_t*)grid2;
					return true;
				}
			}
		}
	}
	return false;
}

//==========================================================================
//
//	R_TryStitchPatch
//
//	This function will try to stitch patches in the same LoD group together
// for the highest LoD.
//
//	Only single missing vertice cracks will be fixed.
//
//	Vertices will be joined at the patch side a crack is first found, at the
// other side of the patch (on the same row or column) the vertices will not
// be joined and cracks might still appear at that side.
//
//==========================================================================

static int R_TryStitchingPatch(int grid1num)
{
	int numstitches = 0;
	srfGridMesh_t* grid1 = (srfGridMesh_t*)s_worldData.surfaces[grid1num].data;
	for (int j = 0; j < s_worldData.numsurfaces; j++)
	{
		srfGridMesh_t* grid2 = (srfGridMesh_t*)s_worldData.surfaces[j].data;
		// if this surface is not a grid
		if (grid2->surfaceType != SF_GRID)
		{
			continue;
		}
		// grids in the same LOD group should have the exact same lod radius
		if (grid1->lodRadius != grid2->lodRadius)
		{
			continue;
		}
		// grids in the same LOD group should have the exact same lod origin
		if (grid1->lodOrigin[0] != grid2->lodOrigin[0])
		{
			continue;
		}
		if (grid1->lodOrigin[1] != grid2->lodOrigin[1])
		{
			continue;
		}
		if (grid1->lodOrigin[2] != grid2->lodOrigin[2])
		{
			continue;
		}
		while (R_StitchPatches(grid1num, j))
		{
			numstitches++;
		}
	}
	return numstitches;
}

//==========================================================================
//
//	R_StitchAllPatches
//
//==========================================================================

static void R_StitchAllPatches()
{
	int numstitches = 0;
	bool stitched;
	do
	{
		stitched = false;
		for (int i = 0; i < s_worldData.numsurfaces; i++)
		{
			srfGridMesh_t* grid1 = (srfGridMesh_t*)s_worldData.surfaces[i].data;
			// if this surface is not a grid
			if (grid1->surfaceType != SF_GRID)
			{
				continue;
			}
			if (grid1->lodStitched)
			{
				continue;
			}
			grid1->lodStitched = true;
			stitched = true;
			//
			numstitches += R_TryStitchingPatch(i);
		}
	}
	while (stitched);
	Log::write("stitched %d LoD cracks\n", numstitches);
}

//==========================================================================
//
//	R_LoadSurfaces
//
//==========================================================================

static void R_LoadSurfaces(bsp46_lump_t* surfs, bsp46_lump_t* verts, bsp46_lump_t* indexLump)
{
	bsp46_dsurface_t* in = (bsp46_dsurface_t*)(fileBase + surfs->fileofs);
	if (surfs->filelen % sizeof(*in))
	{
		throw DropException(va("LoadMap: funny lump size in %s", s_worldData.name));
	}
	int count = surfs->filelen / sizeof(*in);

	bsp46_drawVert_t* dv = (bsp46_drawVert_t *)(fileBase + verts->fileofs);
	if (verts->filelen % sizeof(*dv))
	{
		throw DropException(va("LoadMap: funny lump size in %s", s_worldData.name));
	}

	int* indexes = (int*)(fileBase + indexLump->fileofs);
	if ( indexLump->filelen % sizeof(*indexes))
	{
		throw DropException(va("LoadMap: funny lump size in %s", s_worldData.name));
	}

	mbrush46_surface_t* out = new mbrush46_surface_t[count];
	Com_Memset(out, 0, sizeof(mbrush46_surface_t) * count);

	s_worldData.surfaces = out;
	s_worldData.numsurfaces = count;

	int numFaces = 0;
	int numMeshes = 0;
	int numTriSurfs = 0;
	int numFlares = 0;

	for (int i = 0; i < count; i++, in++, out++)
	{
		switch (LittleLong(in->surfaceType))
		{
		case BSP46MST_PATCH:
			ParseMesh(in, dv, out);
			numMeshes++;
			break;
		case BSP46MST_TRIANGLE_SOUP:
			ParseTriSurf(in, dv, out, indexes);
			numTriSurfs++;
			break;
		case BSP46MST_PLANAR:
			ParseFace(in, dv, out, indexes);
			numFaces++;
			break;
		case BSP46MST_FLARE:
			ParseFlare(in, dv, out, indexes);
			numFlares++;
			break;
		default:
			throw DropException("Bad surfaceType");
		}
	}

	R_StitchAllPatches();

	R_FixSharedVertexLodError();

	Log::write("...loaded %d faces, %i meshes, %i trisurfs, %i flares\n", 
		numFaces, numMeshes, numTriSurfs, numFlares);
}

//==========================================================================
//
//	R_SetParent
//
//==========================================================================

static void R_SetParent(mbrush46_node_t* node, mbrush46_node_t* parent)
{
	node->parent = parent;
	if (node->contents != -1)
	{
		return;
	}
	R_SetParent(node->children[0], node);
	R_SetParent(node->children[1], node);
}

//==========================================================================
//
//	R_LoadNodesAndLeafs
//
//==========================================================================

static void R_LoadNodesAndLeafs(bsp46_lump_t* nodeLump, bsp46_lump_t* leafLump)
{
	bsp46_dnode_t* in = (bsp46_dnode_t*)(fileBase + nodeLump->fileofs);
	if (nodeLump->filelen % sizeof(bsp46_dnode_t) ||
		leafLump->filelen % sizeof(bsp46_dleaf_t))
	{
		throw DropException(va("LoadMap: funny lump size in %s", s_worldData.name));
	}
	int numNodes = nodeLump->filelen / sizeof(bsp46_dnode_t);
	int numLeafs = leafLump->filelen / sizeof(bsp46_dleaf_t);

	mbrush46_node_t* out = new mbrush46_node_t[numNodes + numLeafs];
	Com_Memset(out, 0, sizeof(mbrush46_node_t) * (numNodes + numLeafs));

	s_worldData.nodes = out;
	s_worldData.numnodes = numNodes + numLeafs;
	s_worldData.numDecisionNodes = numNodes;

	// load nodes
	for (int i = 0; i < numNodes; i++, in++, out++)
	{
		for (int j = 0; j < 3; j++)
		{
			out->mins[j] = LittleLong(in->mins[j]);
			out->maxs[j] = LittleLong(in->maxs[j]);
		}

		int p = LittleLong(in->planeNum);
		out->plane = s_worldData.planes + p;

		out->contents = BRUSH46_CONTENTS_NODE;	// differentiate from leafs

		for (int j = 0; j < 2; j++)
		{
			p = LittleLong(in->children[j]);
			if (p >= 0)
			{
				out->children[j] = s_worldData.nodes + p;
			}
			else
			{
				out->children[j] = s_worldData.nodes + numNodes + (-1 - p);
			}
		}
	}

	// load leafs
	bsp46_dleaf_t* inLeaf = (bsp46_dleaf_t*)(fileBase + leafLump->fileofs);
	for (int i = 0; i < numLeafs; i++, inLeaf++, out++)
	{
		for (int j = 0; j < 3; j++)
		{
			out->mins[j] = LittleLong(inLeaf->mins[j]);
			out->maxs[j] = LittleLong(inLeaf->maxs[j]);
		}

		out->cluster = LittleLong(inLeaf->cluster);
		out->area = LittleLong(inLeaf->area);

		if (out->cluster >= s_worldData.numClusters)
		{
			s_worldData.numClusters = out->cluster + 1;
		}

		out->firstmarksurface = s_worldData.marksurfaces +
			LittleLong(inLeaf->firstLeafSurface);
		out->nummarksurfaces = LittleLong(inLeaf->numLeafSurfaces);
	}	

	// chain decendants
	R_SetParent(s_worldData.nodes, NULL);
}

//==========================================================================
//
//	R_LoadShaders
//
//==========================================================================

static void R_LoadShaders(bsp46_lump_t* l)
{	
	bsp46_dshader_t* in = (bsp46_dshader_t*)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw DropException(va("LoadMap: funny lump size in %s", s_worldData.name));
	}
	int count = l->filelen / sizeof(*in);
	bsp46_dshader_t* out = new bsp46_dshader_t[count];

	s_worldData.shaders = out;
	s_worldData.numShaders = count;

	Com_Memcpy(out, in, count * sizeof(*out));

	for (int i = 0; i < count; i++)
	{
		out[i].surfaceFlags = LittleLong(out[i].surfaceFlags);
		out[i].contentFlags = LittleLong(out[i].contentFlags);
	}
}

//==========================================================================
//
//	R_LoadMarksurfaces
//
//==========================================================================

static void R_LoadMarksurfaces(bsp46_lump_t* l)
{	
	int* in = (int*)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw DropException(va("LoadMap: funny lump size in %s", s_worldData.name));
	}
	int count = l->filelen / sizeof(*in);
	mbrush46_surface_t** out = new mbrush46_surface_t*[count];

	s_worldData.marksurfaces = out;
	s_worldData.nummarksurfaces = count;

	for (int i = 0; i < count; i++)
	{
		int j = LittleLong(in[i]);
		out[i] = s_worldData.surfaces + j;
	}
}

//==========================================================================
//
//	R_LoadPlanes
//
//==========================================================================

static void R_LoadPlanes(bsp46_lump_t* l)
{
	bsp46_dplane_t* in = (bsp46_dplane_t*)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw DropException(va("LoadMap: funny lump size in %s", s_worldData.name));
	}
	int count = l->filelen / sizeof(*in);
	//JL Why * 2?
	cplane_t* out = new cplane_t[count * 2];
	Com_Memset(out, 0, sizeof(cplane_t) * count * 2);

	s_worldData.planes = out;
	s_worldData.numplanes = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		for (int j = 0; j < 3; j++)
		{
			out->normal[j] = LittleFloat(in->normal[j]);
		}

		out->dist = LittleFloat(in->dist);
		out->type = PlaneTypeForNormal(out->normal);
		SetPlaneSignbits(out);
	}
}
#endif

//==========================================================================
//
//	ColorBytes4
//
//==========================================================================

unsigned ColorBytes4(float r, float g, float b, float a)
{
	unsigned	i;

	((byte*)&i)[0] = r * 255;
	((byte*)&i)[1] = g * 255;
	((byte*)&i)[2] = b * 255;
	((byte*)&i)[3] = a * 255;

	return i;
}

#if 0
//==========================================================================
//
//	R_LoadFogs
//
//==========================================================================

static void R_LoadFogs(bsp46_lump_t* l, bsp46_lump_t* brushesLump, bsp46_lump_t* sidesLump)
{
	bsp46_dfog_t* fogs = (bsp46_dfog_t*)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*fogs))
	{
		throw DropException(va("LoadMap: funny lump size in %s", s_worldData.name));
	}
	int count = l->filelen / sizeof(*fogs);

	// create fog strucutres for them
	s_worldData.numfogs = count + 1;
	s_worldData.fogs = new mbrush46_fog_t[s_worldData.numfogs];
	Com_Memset(s_worldData.fogs, 0, sizeof(mbrush46_fog_t) * s_worldData.numfogs);
	mbrush46_fog_t* out = s_worldData.fogs + 1;

	if (!count)
	{
		return;
	}

	bsp46_dbrush_t* brushes = (bsp46_dbrush_t*)(fileBase + brushesLump->fileofs);
	if (brushesLump->filelen % sizeof(*brushes))
	{
		throw DropException(va("LoadMap: funny lump size in %s", s_worldData.name));
	}
	int brushesCount = brushesLump->filelen / sizeof(*brushes);

	bsp46_dbrushside_t* sides = (bsp46_dbrushside_t*)(fileBase + sidesLump->fileofs);
	if (sidesLump->filelen % sizeof(*sides))
	{
		throw DropException(va("LoadMap: funny lump size in %s", s_worldData.name));
	}
	int sidesCount = sidesLump->filelen / sizeof(*sides);

	for (int i = 0; i < count; i++, fogs++, out++)
	{
		out->originalBrushNumber = LittleLong(fogs->brushNum);

		if ((unsigned)out->originalBrushNumber >= (unsigned)brushesCount)
		{
			throw DropException("fog brushNumber out of range");
		}
		bsp46_dbrush_t* brush = brushes + out->originalBrushNumber;

		int firstSide = LittleLong(brush->firstSide);
		if ((unsigned)firstSide > (unsigned)(sidesCount - 6))
		{
			throw DropException("fog brush sideNumber out of range");
		}

		// brushes are always sorted with the axial sides first
		int sideNum = firstSide + 0;
		int planeNum = LittleLong(sides[ sideNum ].planeNum);
		out->bounds[0][0] = -s_worldData.planes[planeNum].dist;

		sideNum = firstSide + 1;
		planeNum = LittleLong(sides[sideNum].planeNum);
		out->bounds[1][0] = s_worldData.planes[planeNum].dist;

		sideNum = firstSide + 2;
		planeNum = LittleLong(sides[sideNum].planeNum);
		out->bounds[0][1] = -s_worldData.planes[planeNum].dist;

		sideNum = firstSide + 3;
		planeNum = LittleLong(sides[sideNum].planeNum);
		out->bounds[1][1] = s_worldData.planes[planeNum].dist;

		sideNum = firstSide + 4;
		planeNum = LittleLong(sides[sideNum].planeNum);
		out->bounds[0][2] = -s_worldData.planes[planeNum].dist;

		sideNum = firstSide + 5;
		planeNum = LittleLong(sides[sideNum].planeNum);
		out->bounds[1][2] = s_worldData.planes[planeNum].dist;

		// get information from the shader for fog parameters
		shader_t* shader = R_FindShader(fogs->shader, LIGHTMAP_NONE, true);

		out->parms = shader->fogParms;

		out->colorInt = ColorBytes4(shader->fogParms.color[0] * tr.identityLight, 
			shader->fogParms.color[1] * tr.identityLight, 
			shader->fogParms.color[2] * tr.identityLight, 1.0);

		float d = shader->fogParms.depthForOpaque < 1 ? 1 : shader->fogParms.depthForOpaque;
		out->tcScale = 1.0f / (d * 8);

		// set the gradient vector
		sideNum = LittleLong(fogs->visibleSide);

		if (sideNum == -1)
		{
			out->hasSurface = false;
		}
		else
		{
			out->hasSurface = true;
			planeNum = LittleLong(sides[firstSide + sideNum].planeNum);
			VectorSubtract(vec3_origin, s_worldData.planes[planeNum].normal, out->surface);
			out->surface[3] = -s_worldData.planes[planeNum].dist;
		}
	}
}

//==========================================================================
//
//	R_LoadLightGrid
//
//==========================================================================

static void R_LoadLightGrid(bsp46_lump_t* l)
{
	world_t* w = &s_worldData;

	w->lightGridInverseSize[0] = 1.0f / w->lightGridSize[0];
	w->lightGridInverseSize[1] = 1.0f / w->lightGridSize[1];
	w->lightGridInverseSize[2] = 1.0f / w->lightGridSize[2];

	float* wMins = w->bmodels[0].bounds[0];
	float* wMaxs = w->bmodels[0].bounds[1];

	vec3_t maxs;
	for (int i = 0; i < 3; i++)
	{
		w->lightGridOrigin[i] = w->lightGridSize[i] * ceil(wMins[i] / w->lightGridSize[i]);
		maxs[i] = w->lightGridSize[i] * floor(wMaxs[i] / w->lightGridSize[i]);
		w->lightGridBounds[i] = (maxs[i] - w->lightGridOrigin[i]) / w->lightGridSize[i] + 1;
	}

	int numGridPoints = w->lightGridBounds[0] * w->lightGridBounds[1] * w->lightGridBounds[2];

	if (l->filelen != numGridPoints * 8)
	{
		Log::write(S_COLOR_YELLOW "WARNING: light grid mismatch\n");
		w->lightGridData = NULL;
		return;
	}

	w->lightGridData = new byte[l->filelen];
	Com_Memcpy(w->lightGridData, fileBase + l->fileofs, l->filelen);

	// deal with overbright bits
	for (int i = 0; i < numGridPoints; i++)
	{
		R_ColorShiftLightingBytes(&w->lightGridData[i * 8], &w->lightGridData[i * 8]);
		R_ColorShiftLightingBytes(&w->lightGridData[i * 8 + 3], &w->lightGridData[i * 8 + 3]);
	}
}

//==========================================================================
//
//	R_LoadEntities
//
//==========================================================================

static void R_LoadEntities(bsp46_lump_t* l)
{
	world_t* w = &s_worldData;
	w->lightGridSize[0] = 64;
	w->lightGridSize[1] = 64;
	w->lightGridSize[2] = 128;

	const char* p = (const char*)(fileBase + l->fileofs);

	// store for reference by the cgame
	w->entityString = new char[l->filelen + 1];
	String::Cpy(w->entityString, p);
	w->entityString[l->filelen] = 0;
	w->entityParsePoint = w->entityString;

	char* token = String::ParseExt(&p, true);
	if (!*token || *token != '{')
	{
		return;
	}

	// only parse the world spawn
	while (1)
	{	
		// parse key
		token = String::ParseExt(&p, true);

		if (!*token || *token == '}')
		{
			break;
		}
		char keyname[MAX_TOKEN_CHARS_Q3];
		String::NCpyZ(keyname, token, sizeof(keyname));

		// parse value
		token = String::ParseExt(&p, true);

		if (!*token || *token == '}')
		{
			break;
		}
		char value[MAX_TOKEN_CHARS_Q3];
		String::NCpyZ(value, token, sizeof(value));

		// check for remapping of shaders for vertex lighting
		const char* keybase = "vertexremapshader";
		if (!String::NCmp(keyname, keybase, String::Length(keybase)))
		{
			char* s = strchr(value, ';');
			if (!s)
			{
				Log::write(S_COLOR_YELLOW "WARNING: no semi colon in vertexshaderremap '%s'\n", value);
				break;
			}
			*s++ = 0;
			if (r_vertexLight->integer)
			{
				R_RemapShader(value, s, "0");
			}
			continue;
		}

		// check for remapping of shaders
		keybase = "remapshader";
		if (!String::NCmp(keyname, keybase, String::Length(keybase)))
		{
			char* s = strchr(value, ';');
			if (!s)
			{
				Log::write(S_COLOR_YELLOW "WARNING: no semi colon in shaderremap '%s'\n", value);
				break;
			}
			*s++ = 0;
			R_RemapShader(value, s, "0");
			continue;
		}

		// check for a different grid size
		if (!String::ICmp(keyname, "gridsize"))
		{
			sscanf(value, "%f %f %f", &w->lightGridSize[0], &w->lightGridSize[1], &w->lightGridSize[2]);
			continue;
		}
	}
}

//==========================================================================
//
//	R_LoadSubmodels
//
//==========================================================================

static void R_LoadSubmodels(bsp46_lump_t* l)
{
	bsp46_dmodel_t* in = (bsp46_dmodel_t*)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw DropException(va("LoadMap: funny lump size in %s", s_worldData.name));
	}
	int count = l->filelen / sizeof(*in);

	mbrush46_model_t* out = new mbrush46_model_t[count];
	s_worldData.bmodels = out;

	for (int i = 0; i < count; i++, in++, out++)
	{
		model_t* model = R_AllocModel();

		qassert(model != NULL);			// this should never happen

		model->type = MOD_BRUSH46;
		model->q3_bmodel = out;
		String::Sprintf(model->name, sizeof(model->name), "*%d", i);

		for (int j = 0; j < 3; j++)
		{
			out->bounds[0][j] = LittleFloat(in->mins[j]);
			out->bounds[1][j] = LittleFloat(in->maxs[j]);
		}

		out->firstSurface = s_worldData.surfaces + LittleLong(in->firstSurface);
		out->numSurfaces = LittleLong(in->numSurfaces);
	}
}

//==========================================================================
//
//	R_LoadBrush46Model
//
//==========================================================================

void R_LoadBrush46Model(void* buffer)
{
	bsp46_dheader_t* header = (bsp46_dheader_t*)buffer;
	fileBase = (byte*)buffer;

	int version = LittleLong(header->version);
	if (version != BSP46_VERSION)
	{
		throw DropException(va("RE_LoadWorldMap: %s has wrong version number (%i should be %i)", 
			s_worldData.name, version, BSP46_VERSION));
	}

	// swap all the lumps
	for (int i = 0; i < (int)sizeof(bsp46_dheader_t) / 4; i++)
	{
		((int*)header)[i] = LittleLong(((int*)header)[i]);
	}

	// load into heap
	R_LoadShaders(&header->lumps[BSP46LUMP_SHADERS]);
	R_LoadLightmaps(&header->lumps[BSP46LUMP_LIGHTMAPS]);
	R_LoadPlanes(&header->lumps[BSP46LUMP_PLANES]);
	R_LoadFogs(&header->lumps[BSP46LUMP_FOGS], &header->lumps[BSP46LUMP_BRUSHES], &header->lumps[BSP46LUMP_BRUSHSIDES]);
	R_LoadSurfaces(&header->lumps[BSP46LUMP_SURFACES], &header->lumps[BSP46LUMP_DRAWVERTS], &header->lumps[BSP46LUMP_DRAWINDEXES]);
	R_LoadMarksurfaces(&header->lumps[BSP46LUMP_LEAFSURFACES]);
	R_LoadNodesAndLeafs(&header->lumps[BSP46LUMP_NODES], &header->lumps[BSP46LUMP_LEAFS]);
	R_LoadSubmodels(&header->lumps[BSP46LUMP_MODELS]);
	R_LoadVisibility(&header->lumps[BSP46LUMP_VISIBILITY]);
	R_LoadEntities(&header->lumps[BSP46LUMP_ENTITIES]);
	R_LoadLightGrid(&header->lumps[BSP46LUMP_LIGHTGRID]);
}

//==========================================================================
//
//	R_FreeBsp46
//
//==========================================================================

void R_FreeBsp46(world_t* mod)
{
	delete[] mod->novis;
	if (mod->vis)
	{
		delete[] mod->vis;
	}
	for (int i = 0; i < mod->numsurfaces; i++)
	{
		switch (*mod->surfaces[i].data)
		{
		case SF_GRID:
			R_FreeSurfaceGridMesh((srfGridMesh_t*)mod->surfaces[i].data);
			break;
		case SF_TRIANGLES:
			Mem_Free(mod->surfaces[i].data);
			break;
		case SF_FACE:
			Mem_Free(mod->surfaces[i].data);
			break;
		case SF_FLARE:
			Mem_Free(mod->surfaces[i].data);
			break;
		default:
			break;
		}
	}
	delete[] mod->surfaces;
	delete[] mod->nodes;
	delete[] mod->shaders;
	delete[] mod->marksurfaces;
	delete[] mod->planes;
	delete[] mod->fogs;
	delete[] mod->lightGridData;
	delete[] mod->entityString;
	delete[] mod->bmodels;
}

//==========================================================================
//
//	R_FreeBsp46Model
//
//==========================================================================

void R_FreeBsp46Model(model_t* mod)
{
}

//==========================================================================
//
//	R_ClusterPVS
//
//==========================================================================

const byte* R_ClusterPVS(int cluster)
{
	if (!tr.world || !tr.world->vis || cluster < 0 || cluster >= tr.world->numClusters)
	{
		return tr.world->novis;
	}

	return tr.world->vis + cluster * tr.world->clusterBytes;
}

//==========================================================================
//
//	R_PointInLeaf
//
//==========================================================================

mbrush46_node_t* R_PointInLeaf(const vec3_t p)
{
	if (!tr.world)
	{
		throw DropException("R_PointInLeaf: bad model");
	}

	mbrush46_node_t* node = tr.world->nodes;
	while (1)
	{
		if (node->contents != -1)
		{
			break;
		}
		cplane_t* plane = node->plane;
		float d = DotProduct(p, plane->normal) - plane->dist;
		if (d > 0)
		{
			node = node->children[0];
		}
		else
		{
			node = node->children[1];
		}
	}

	return node;
}
#endif
