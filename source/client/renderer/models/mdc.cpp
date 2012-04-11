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

#define NUMMDCVERTEXNORMALS	256

#define MDC_DIST_SCALE		0.05    // lower for more accuracy, but less range

// note: we are locked in at 8 or less bits since changing to byte-encoded normals
#define MDC_BITS_PER_AXIS	8
#define MDC_MAX_OFS			127.0   // to be safe

#define R_MDC_DecodeXyzCompressed(ofsVec, out, normal) \
	(out)[0] = ((float)((ofsVec) & 255) - MDC_MAX_OFS) * MDC_DIST_SCALE; \
	(out)[1] = ((float)((ofsVec >> 8) & 255) - MDC_MAX_OFS) * MDC_DIST_SCALE; \
	(out)[2] = ((float)((ofsVec >> 16) & 255) - MDC_MAX_OFS) * MDC_DIST_SCALE; \
	VectorCopy((r_anormals)[(ofsVec >> 24)], normal);

#define LL(x) x = LittleLong(x)

static float r_anormals[NUMMDCVERTEXNORMALS][3] =
{
#include "anorms256.h"
};

static bool R_LoadMdcLod(model_t* mod, int lod, void* buffer, const char* mod_name)
{
	mdcHeader_t* pinmodel = (mdcHeader_t*)buffer;

	int version = LittleLong(pinmodel->version);
	if (version != MDC_VERSION)
	{
		common->Printf(S_COLOR_YELLOW "R_LoadMdcLod: %s has wrong version (%i should be %i)\n",
			mod_name, version, MDC_VERSION);
		return false;
	}

	mod->type = MOD_MDC;
	int size = LittleLong(pinmodel->ofsEnd);
	mod->q3_dataSize += size;
	mod->q3_mdc[lod] = (mdcHeader_t*)Mem_Alloc(size);

	memcpy(mod->q3_mdc[lod], buffer, LittleLong(pinmodel->ofsEnd));

	LL(mod->q3_mdc[lod]->ident);
	LL(mod->q3_mdc[lod]->version);
	LL(mod->q3_mdc[lod]->numFrames);
	LL(mod->q3_mdc[lod]->numTags);
	LL(mod->q3_mdc[lod]->numSurfaces);
	LL(mod->q3_mdc[lod]->ofsFrames);
	LL(mod->q3_mdc[lod]->ofsTagNames);
	LL(mod->q3_mdc[lod]->ofsTags);
	LL(mod->q3_mdc[lod]->ofsSurfaces);
	LL(mod->q3_mdc[lod]->ofsEnd);
	LL(mod->q3_mdc[lod]->flags);
	LL(mod->q3_mdc[lod]->numSkins);


	if (mod->q3_mdc[lod]->numFrames < 1)
	{
		common->Printf(S_COLOR_YELLOW "R_LoadMdcLod: %s has no frames\n", mod_name);
		return false;
	}

	// swap all the frames
	md3Frame_t* frame = (md3Frame_t*)((byte*)mod->q3_mdc[lod] + mod->q3_mdc[lod]->ofsFrames);
	for (int i = 0; i < mod->q3_mdc[lod]->numFrames; i++, frame++)
	{
		frame->radius = LittleFloat(frame->radius);
		if (strstr(mod->name, "sherman") || strstr(mod->name, "mg42"))
		{
			frame->radius = 256;
			for (int j = 0; j < 3; j++)
			{
				frame->bounds[0][j] = 128;
				frame->bounds[1][j] = -128;
				frame->localOrigin[j] = LittleFloat(frame->localOrigin[j]);
			}
		}
		else
		{
			for (int j = 0; j < 3; j++)
			{
				frame->bounds[0][j] = LittleFloat(frame->bounds[0][j]);
				frame->bounds[1][j] = LittleFloat(frame->bounds[1][j]);
				frame->localOrigin[j] = LittleFloat(frame->localOrigin[j]);
			}
		}
	}

	// swap all the tags
	mdcTag_t* tag = (mdcTag_t*)((byte*)mod->q3_mdc[lod] + mod->q3_mdc[lod]->ofsTags);
	if (LittleLong(1) != 1)
	{
		for (int i = 0; i < mod->q3_mdc[lod]->numTags * mod->q3_mdc[lod]->numFrames; i++, tag++)
		{
			for (int j = 0; j < 3; j++)
			{
				tag->xyz[j] = LittleShort(tag->xyz[j]);
				tag->angles[j] = LittleShort(tag->angles[j]);
			}
		}
	}

	// swap all the surfaces
	mdcSurface_t* surf = (mdcSurface_t*)((byte*)mod->q3_mdc[lod] + mod->q3_mdc[lod]->ofsSurfaces);
	for (int i = 0; i < mod->q3_mdc[lod]->numSurfaces; i++)
	{

		LL(surf->ident);
		LL(surf->flags);
		LL(surf->numBaseFrames);
		LL(surf->numCompFrames);
		LL(surf->numShaders);
		LL(surf->numTriangles);
		LL(surf->ofsTriangles);
		LL(surf->numVerts);
		LL(surf->ofsShaders);
		LL(surf->ofsSt);
		LL(surf->ofsXyzNormals);
		LL(surf->ofsXyzCompressed);
		LL(surf->ofsFrameBaseFrames);
		LL(surf->ofsFrameCompFrames);
		LL(surf->ofsEnd);

		if (surf->numVerts > SHADER_MAX_VERTEXES)
		{
			common->Error("R_LoadMdcLod: %s has more than %i verts on a surface (%i)",
						  mod_name, SHADER_MAX_VERTEXES, surf->numVerts);
		}
		if (surf->numTriangles * 3 > SHADER_MAX_INDEXES)
		{
			common->Error("R_LoadMdcLod: %s has more than %i triangles on a surface (%i)",
						  mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles);
		}

		// change to surface identifier
		surf->ident = SF_MDC;

		// lowercase the surface name so skin compares are faster
		String::ToLower(surf->name);

		// strip off a trailing _1 or _2
		// this is a crutch for q3data being a mess
		int j = String::Length(surf->name);
		if (j > 2 && surf->name[j - 2] == '_')
		{
			surf->name[j - 2] = 0;
		}

		// register the shaders
		md3Shader_t* shader = (md3Shader_t*)((byte*)surf + surf->ofsShaders);
		for (j = 0; j < surf->numShaders; j++, shader++)
		{
			shader_t* sh;

			sh = R_FindShader(shader->name, LIGHTMAP_NONE, true);
			if (sh->defaultShader)
			{
				shader->shaderIndex = 0;
			}
			else
			{
				shader->shaderIndex = sh->index;
			}
		}

		// Ridah, optimization, only do the swapping if we really need to
		if (LittleShort(1) != 1)
		{

			// swap all the triangles
			md3Triangle_t* tri = (md3Triangle_t*)((byte*)surf + surf->ofsTriangles);
			for (j = 0; j < surf->numTriangles; j++, tri++)
			{
				LL(tri->indexes[0]);
				LL(tri->indexes[1]);
				LL(tri->indexes[2]);
			}

			// swap all the ST
			md3St_t* st = (md3St_t*)((byte*)surf + surf->ofsSt);
			for (j = 0; j < surf->numVerts; j++, st++)
			{
				st->st[0] = LittleFloat(st->st[0]);
				st->st[1] = LittleFloat(st->st[1]);
			}

			// swap all the XyzNormals
			md3XyzNormal_t* xyz = (md3XyzNormal_t*)((byte*)surf + surf->ofsXyzNormals);
			for (j = 0; j < surf->numVerts * surf->numBaseFrames; j++, xyz++)
			{
				xyz->xyz[0] = LittleShort(xyz->xyz[0]);
				xyz->xyz[1] = LittleShort(xyz->xyz[1]);
				xyz->xyz[2] = LittleShort(xyz->xyz[2]);

				xyz->normal = LittleShort(xyz->normal);
			}

			// swap all the XyzCompressed
			mdcXyzCompressed_t* xyzComp = (mdcXyzCompressed_t*)((byte*)surf + surf->ofsXyzCompressed);
			for (j = 0; j < surf->numVerts * surf->numCompFrames; j++, xyzComp++)
			{
				LL(xyzComp->ofsVec);
			}

			// swap the frameBaseFrames
			short* ps = (short*)((byte*)surf + surf->ofsFrameBaseFrames);
			for (j = 0; j < mod->q3_mdc[lod]->numFrames; j++, ps++)
			{
				*ps = LittleShort(*ps);
			}

			// swap the frameCompFrames
			ps = (short*)((byte*)surf + surf->ofsFrameCompFrames);
			for (j = 0; j < mod->q3_mdc[lod]->numFrames; j++, ps++)
			{
				*ps = LittleShort(*ps);
			}
		}
		// done.

		// find the next surface
		surf = (mdcSurface_t*)((byte*)surf + surf->ofsEnd);
	}

	return true;
}

bool R_LoadMdc(model_t* mod, void* buffer)
{
	mod->q3_numLods = 0;

	//
	// load the files
	//
	int numLoaded = 0;

	int lod = MD3_MAX_LODS - 1;
	for (; lod >= 0; lod--)
	{
		char filename[1024];

		String::Cpy(filename, mod->name);

		void* buf;
		if (lod == 0)
		{
			buf = buffer;
		}
		else
		{
			char namebuf[80];

			if (String::RChr(filename, '.'))
			{
				*String::RChr(filename, '.') = 0;
			}
			sprintf(namebuf, "_%d.mdc", lod);
			String::Cat(filename, sizeof(filename), namebuf);

			FS_ReadFile(filename, &buf);
			if (!buf)
			{
				continue;
			}
		}

		int ident = LittleLong(*(unsigned*)buf);
		if (ident != MDC_IDENT)
		{
			common->Printf(S_COLOR_YELLOW "R_LoadMdc: unknown fileid for %s\n", filename);
			return false;
		}

		bool loaded = R_LoadMdcLod(mod, lod, buf, filename);

		if (lod != 0)
		{
			FS_FreeFile(buf);
		}

		if (!loaded)
		{
			if (lod == 0)
			{
				return false;
			}
			else
			{
				break;
			}
		}
		else
		{
			mod->q3_numLods++;
			numLoaded++;
			// if we have a valid model and are biased
			// so that we won't see any higher detail ones,
			// stop loading them
			if (lod <= r_lodbias->integer)
			{
				break;
			}
		}
	}

	if (!numLoaded)
	{
		return false;
	}

	// duplicate into higher lod spots that weren't
	// loaded, in case the user changes r_lodbias on the fly
	for (lod--; lod >= 0; lod--)
	{
		mod->q3_numLods++;
		mod->q3_mdc[lod] = mod->q3_mdc[lod + 1];
	}

	return true;
}

void R_FreeMdc(model_t* mod)
{
	Mem_Free(mod->q3_mdc[0]);
	for (int lod = 1; lod < MD3_MAX_LODS; lod++)
	{
		if (mod->q3_mdc[lod] != mod->q3_mdc[lod - 1])
		{
			Mem_Free(mod->q3_mdc[lod]);
		}
	}
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

		for (int vertNum = 0; vertNum < numVerts; vertNum++,
			oldXyz += 4, newXyz += 4, oldNormals += 4, newNormals += 4,
			outXyz += 4, outNormal += 4)
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

void RB_SurfaceCMesh(mdcSurface_t* surface)
{
	// RF, check for REFLAG_HANDONLY
	if (backEnd.currentEntity->e.reFlags & REFLAG_ONLYHAND)
	{
		if (!strstr(surface->name, "hand"))
		{
			return;
		}
	}

	float backlerp;
	if (backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame)
	{
		backlerp = 0;
	}
	else
	{
		backlerp = backEnd.currentEntity->e.backlerp;
	}

	RB_CHECKOVERFLOW(surface->numVerts, surface->numTriangles * 3);

	LerpCMeshVertexes(surface, backlerp);

	int* triangles = (int*)((byte*)surface + surface->ofsTriangles);
	int indexes = surface->numTriangles * 3;
	int Bob = tess.numIndexes;
	int Doug = tess.numVertexes;
	for (int j = 0; j < indexes; j++)
	{
		tess.indexes[Bob + j] = Doug + triangles[j];
	}
	tess.numIndexes += indexes;

	float* texCoords = (float*)((byte*)surface + surface->ofsSt);

	int numVerts = surface->numVerts;
	for (int j = 0; j < numVerts; j++)
	{
		tess.texCoords[Doug + j][0][0] = texCoords[j * 2 + 0];
		tess.texCoords[Doug + j][0][1] = texCoords[j * 2 + 1];
		// FIXME: fill in lightmapST for completeness?
	}

	tess.numVertexes += surface->numVerts;
}
