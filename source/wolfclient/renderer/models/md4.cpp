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

All bones should be an identity orientation to display the mesh exactly
as it is specified.

For all other frames, the bones represent the transformation from the 
orientation of the bone in the base frame to the orientation in this
frame.

*/

// HEADER FILES ------------------------------------------------------------

#include "../../client.h"
#include "../local.h"

// MACROS ------------------------------------------------------------------

#define	LL(x) x=LittleLong(x)

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

#if 0
//==========================================================================
//
//	R_LoadMD4
//
//==========================================================================

bool R_LoadMD4(model_t* mod, void* buffer, const char* mod_name)
{
	md4Header_t* pinmodel = (md4Header_t*)buffer;

	int version = LittleLong(pinmodel->version);
	if (version != MD4_VERSION)
	{
		Log::write(S_COLOR_YELLOW "R_LoadMD4: %s has wrong version (%i should be %i)\n",
			mod_name, version, MD4_VERSION);
		return false;
	}

	mod->type = MOD_MD4;
	int size = LittleLong(pinmodel->ofsEnd);
	mod->q3_dataSize += size;
	md4Header_t* md4 = mod->q3_md4 = (md4Header_t*)Mem_Alloc(size);

	Com_Memcpy(md4, buffer, LittleLong(pinmodel->ofsEnd));

	LL(md4->ident);
	LL(md4->version);
	LL(md4->numFrames);
	LL(md4->numBones);
	LL(md4->numLODs);
	LL(md4->ofsFrames);
	LL(md4->ofsLODs);
	LL(md4->ofsEnd);

	if (md4->numFrames < 1)
	{
		Log::write(S_COLOR_YELLOW "R_LoadMD4: %s has no frames\n", mod_name);
		return false;
	}

	// we don't need to swap tags in the renderer, they aren't used

	// swap all the frames
	int frameSize = (qintptr)(&((md4Frame_t*)0)->bones[md4->numBones]);
	for (int i = 0; i < md4->numFrames; i++)
	{
		md4Frame_t* frame = (md4Frame_t*)((byte*)md4 + md4->ofsFrames + i * frameSize);
		frame->radius = LittleFloat(frame->radius);
		for (int j = 0; j < 3; j++)
		{
			frame->bounds[0][j] = LittleFloat(frame->bounds[0][j]);
			frame->bounds[1][j] = LittleFloat(frame->bounds[1][j]);
			frame->localOrigin[j] = LittleFloat(frame->localOrigin[j]);
		}
		for (int j = 0; j < md4->numBones * (int)sizeof(md4Bone_t) / 4; j++)
		{
			((float*)frame->bones)[j] = LittleFloat(((float*)frame->bones)[j]);
		}
	}

	// swap all the LOD's
	md4LOD_t* lod = (md4LOD_t*)((byte*)md4 + md4->ofsLODs);
	for (int lodindex = 0; lodindex < md4->numLODs; lodindex++)
	{
		// swap all the surfaces
		md4Surface_t* surf = (md4Surface_t*)((byte*)lod + lod->ofsSurfaces);
		for (int i = 0; i < lod->numSurfaces; i++)
		{
			LL(surf->ident);
			LL(surf->numTriangles);
			LL(surf->ofsTriangles);
			LL(surf->numVerts);
			LL(surf->ofsVerts);
			LL(surf->ofsEnd);

			if (surf->numVerts > SHADER_MAX_VERTEXES)
			{
				throw DropException(va("R_LoadMD3: %s has more than %i verts on a surface (%i)",
					mod_name, SHADER_MAX_VERTEXES, surf->numVerts));
			}
			if (surf->numTriangles*3 > SHADER_MAX_INDEXES)
			{
				throw DropException(va("R_LoadMD3: %s has more than %i triangles on a surface (%i)",
					mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles));
			}

			// change to surface identifier
			surf->ident = SF_MD4;

			// lowercase the surface name so skin compares are faster
			String::ToLower(surf->name);

			// register the shaders
			shader_t* sh = R_FindShader(surf->shader, LIGHTMAP_NONE, true);
			if (sh->defaultShader)
			{
				surf->shaderIndex = 0;
			}
			else
			{
				surf->shaderIndex = sh->index;
			}

			// swap all the triangles
			md4Triangle_t* tri = (md4Triangle_t*)((byte*)surf + surf->ofsTriangles);
			for (int j = 0; j < surf->numTriangles; j++, tri++)
			{
				LL(tri->indexes[0]);
				LL(tri->indexes[1]);
				LL(tri->indexes[2]);
			}

			// swap all the vertexes
			// FIXME
			// This makes TFC's skeletons work.  Shouldn't be necessary anymore, but left
			// in for reference.
			//v = (md4Vertex_t *) ( (byte *)surf + surf->ofsVerts + 12);
			md4Vertex_t* v = (md4Vertex_t*)((byte*)surf + surf->ofsVerts);
			for (int j = 0; j < surf->numVerts; j++)
			{
				v->normal[0] = LittleFloat(v->normal[0]);
				v->normal[1] = LittleFloat(v->normal[1]);
				v->normal[2] = LittleFloat(v->normal[2]);

				v->texCoords[0] = LittleFloat(v->texCoords[0]);
				v->texCoords[1] = LittleFloat(v->texCoords[1]);

				v->numWeights = LittleLong(v->numWeights);

				for (int k = 0; k < v->numWeights; k++)
				{
					v->weights[k].boneIndex = LittleLong(v->weights[k].boneIndex);
					v->weights[k].boneWeight = LittleFloat(v->weights[k].boneWeight);
					v->weights[k].offset[0] = LittleFloat(v->weights[k].offset[0]);
					v->weights[k].offset[1] = LittleFloat(v->weights[k].offset[1]);
					v->weights[k].offset[2] = LittleFloat(v->weights[k].offset[2]);
				}
				// FIXME
				// This makes TFC's skeletons work.  Shouldn't be necessary anymore, but left
				// in for reference.
				//v = (md4Vertex_t *)( ( byte * )&v->weights[v->numWeights] + 12 );
				v = (md4Vertex_t*)((byte*)&v->weights[v->numWeights]);
			}

			// find the next surface
			surf = (md4Surface_t*)((byte*)surf + surf->ofsEnd);
		}

		// find the next LOD
		lod = (md4LOD_t*)((byte*)lod + lod->ofsEnd);
	}

	return true;
}

//==========================================================================
//
//	R_FreeMd4
//
//==========================================================================

void R_FreeMd4(model_t* mod)
{
	Mem_Free(mod->q3_md4);
}

//==========================================================================
//
//	R_AddAnimSurfaces
//
//==========================================================================

void R_AddAnimSurfaces(trRefEntity_t* ent)
{
	md4Header_t* header = tr.currentModel->q3_md4;
	md4LOD_t* lod = (md4LOD_t*)((byte*)header + header->ofsLODs);

	md4Surface_t* surface = (md4Surface_t*)((byte*)lod + lod->ofsSurfaces);
	for (int i = 0; i < lod->numSurfaces; i++)
	{
		shader_t* shader = R_GetShaderByHandle(surface->shaderIndex);
		R_AddDrawSurf((surfaceType_t*)surface, shader, 0 /*fogNum*/, false);
		surface = (md4Surface_t*)((byte*)surface + surface->ofsEnd);
	}
}
#endif

//==========================================================================
//
//	RB_SurfaceAnim
//
//==========================================================================

void RB_SurfaceAnim(md4Surface_t* surface)
{
	float frontlerp, backlerp;
	if (backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame)
	{
		backlerp = 0;
		frontlerp = 1;
	}
	else 
	{
		backlerp = backEnd.currentEntity->e.backlerp;
		frontlerp = 1.0f - backlerp;
	}
	md4Header_t* header = (md4Header_t*)((byte*)surface + surface->ofsHeader);

	int frameSize = (qintptr)(&((md4Frame_t*)0)->bones[header->numBones]);

	md4Frame_t* frame = (md4Frame_t*)((byte*)header + header->ofsFrames + 
		backEnd.currentEntity->e.frame * frameSize);
	md4Frame_t* oldFrame = (md4Frame_t*)((byte*)header + header->ofsFrames + 
		backEnd.currentEntity->e.oldframe * frameSize);

	RB_CheckOverflow(surface->numVerts, surface->numTriangles * 3);

	int* triangles = (int*)((byte*)surface + surface->ofsTriangles);
	int indexes = surface->numTriangles * 3;
	int baseIndex = tess.numIndexes;
	int baseVertex = tess.numVertexes;
	for (int j = 0; j < indexes; j++)
	{
		tess.indexes[baseIndex + j] = baseIndex + triangles[j];
	}
	tess.numIndexes += indexes;

	//
	// lerp all the needed bones
	//
	md4Bone_t* bonePtr;
	md4Bone_t bones[MD4_MAX_BONES];
	if (!backlerp)
	{
		// no lerping needed
		bonePtr = frame->bones;
	}
	else
	{
		bonePtr = bones;
		for (int i = 0; i < header->numBones * 12; i++)
		{
			((float*)bonePtr)[i] = frontlerp * ((float*)frame->bones)[i] +
				backlerp * ((float*)oldFrame->bones)[i];
		}
	}

	//
	// deform the vertexes by the lerped bones
	//
	int numVerts = surface->numVerts;
	// FIXME
	// This makes TFC's skeletons work.  Shouldn't be necessary anymore, but left
	// in for reference.
	//v = (md4Vertex_t *) ((byte *)surface + surface->ofsVerts + 12);
	md4Vertex_t* v = (md4Vertex_t*) ((byte*)surface + surface->ofsVerts);
	for (int j = 0; j < numVerts; j++)
	{
		vec3_t tempVert, tempNormal;
		VectorClear(tempVert);
		VectorClear(tempNormal);
		md4Weight_t* w = v->weights;
		for (int k = 0; k < v->numWeights; k++, w++)
		{
			md4Bone_t* bone = bonePtr + w->boneIndex;

			tempVert[0] += w->boneWeight * (DotProduct(bone->matrix[0], w->offset) + bone->matrix[0][3]);
			tempVert[1] += w->boneWeight * (DotProduct(bone->matrix[1], w->offset) + bone->matrix[1][3]);
			tempVert[2] += w->boneWeight * (DotProduct(bone->matrix[2], w->offset) + bone->matrix[2][3]);

			tempNormal[0] += w->boneWeight * DotProduct(bone->matrix[0], v->normal);
			tempNormal[1] += w->boneWeight * DotProduct(bone->matrix[1], v->normal);
			tempNormal[2] += w->boneWeight * DotProduct(bone->matrix[2], v->normal);
		}

		tess.xyz[baseVertex + j][0] = tempVert[0];
		tess.xyz[baseVertex + j][1] = tempVert[1];
		tess.xyz[baseVertex + j][2] = tempVert[2];

		tess.normal[baseVertex + j][0] = tempNormal[0];
		tess.normal[baseVertex + j][1] = tempNormal[1];
		tess.normal[baseVertex + j][2] = tempNormal[2];

		tess.texCoords[baseVertex + j][0][0] = v->texCoords[0];
		tess.texCoords[baseVertex + j][0][1] = v->texCoords[1];

		// FIXME
		// This makes TFC's skeletons work.  Shouldn't be necessary anymore, but left
		// in for reference.
		//v = (md4Vertex_t *)( ( byte * )&v->weights[v->numWeights] + 12 );
		v = (md4Vertex_t*)&v->weights[v->numWeights];
	}

	tess.numVertexes += surface->numVerts;
}
