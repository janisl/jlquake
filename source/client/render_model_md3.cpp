//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "render_local.h"

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

//==========================================================================
//
//	R_LoadMd3Lod
//
//==========================================================================

static bool R_LoadMd3Lod(model_t* mod, int lod, const void* buffer, const char* mod_name)
{
	md3Header_t* pinmodel = (md3Header_t*)buffer;

	int version = LittleLong(pinmodel->version);
	if (version != MD3_VERSION)
	{
		Log::write(S_COLOR_YELLOW "R_LoadMD3: %s has wrong version (%i should be %i)\n",
			mod_name, version, MD3_VERSION);
		return false;
	}

	mod->type = MOD_MESH3;
	int size = LittleLong(pinmodel->ofsEnd);
	mod->q3_dataSize += size;
	mod->q3_md3[lod] = (md3Header_t*)Mem_Alloc(size);

	Com_Memcpy(mod->q3_md3[lod], buffer, LittleLong(pinmodel->ofsEnd));

	LL(mod->q3_md3[lod]->ident);
	LL(mod->q3_md3[lod]->version);
	LL(mod->q3_md3[lod]->numFrames);
	LL(mod->q3_md3[lod]->numTags);
	LL(mod->q3_md3[lod]->numSurfaces);
	LL(mod->q3_md3[lod]->ofsFrames);
	LL(mod->q3_md3[lod]->ofsTags);
	LL(mod->q3_md3[lod]->ofsSurfaces);
	LL(mod->q3_md3[lod]->ofsEnd);

	if (mod->q3_md3[lod]->numFrames < 1)
	{
		Log::write(S_COLOR_YELLOW "R_LoadMD3: %s has no frames\n", mod_name);
		return false;
	}

	// swap all the frames
	md3Frame_t* frame = (md3Frame_t*)((byte*)mod->q3_md3[lod] + mod->q3_md3[lod]->ofsFrames);
	for (int i = 0; i < mod->q3_md3[lod]->numFrames; i++, frame++)
	{
		frame->radius = LittleFloat(frame->radius);
		for (int j = 0; j < 3; j++)
		{
			frame->bounds[0][j] = LittleFloat(frame->bounds[0][j]);
			frame->bounds[1][j] = LittleFloat(frame->bounds[1][j]);
			frame->localOrigin[j] = LittleFloat(frame->localOrigin[j]);
		}
	}

	// swap all the tags
	md3Tag_t* tag = (md3Tag_t*)((byte*)mod->q3_md3[lod] + mod->q3_md3[lod]->ofsTags);
	for (int i = 0; i < mod->q3_md3[lod]->numTags * mod->q3_md3[lod]->numFrames; i++, tag++)
	{
		for (int j = 0; j < 3; j++)
		{
			tag->origin[j] = LittleFloat(tag->origin[j]);
			tag->axis[0][j] = LittleFloat(tag->axis[0][j]);
			tag->axis[1][j] = LittleFloat(tag->axis[1][j]);
			tag->axis[2][j] = LittleFloat(tag->axis[2][j]);
		}
	}

	// swap all the surfaces
	md3Surface_t* surf = (md3Surface_t*)((byte*)mod->q3_md3[lod] + mod->q3_md3[lod]->ofsSurfaces);
	for (int i = 0; i < mod->q3_md3[lod]->numSurfaces; i++)
	{
		LL(surf->ident);
		LL(surf->flags);
		LL(surf->numFrames);
		LL(surf->numShaders);
		LL(surf->numTriangles);
		LL(surf->ofsTriangles);
		LL(surf->numVerts);
		LL(surf->ofsShaders);
		LL(surf->ofsSt);
		LL(surf->ofsXyzNormals);
		LL(surf->ofsEnd);
		
		if (surf->numVerts > SHADER_MAX_VERTEXES)
		{
			throw QDropException(va("R_LoadMD3: %s has more than %i verts on a surface (%i)",
				mod_name, SHADER_MAX_VERTEXES, surf->numVerts));
		}
		if (surf->numTriangles * 3 > SHADER_MAX_INDEXES)
		{
			throw QDropException(va("R_LoadMD3: %s has more than %i triangles on a surface (%i)",
				mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles));
		}

		// change to surface identifier
		surf->ident = SF_MD3;

		// lowercase the surface name so skin compares are faster
		String::ToLower(surf->name);

		// strip off a trailing _1 or _2
		// this is a crutch for q3data being a mess
		int Len = String::Length(surf->name);
		if (Len > 2 && surf->name[Len - 2] == '_')
		{
			surf->name[Len - 2] = 0;
		}

		// register the shaders
		md3Shader_t* shader = (md3Shader_t*)((byte*)surf + surf->ofsShaders);
		for (int j = 0; j < surf->numShaders; j++, shader++)
		{
			shader_t* sh = R_FindShader(shader->name, LIGHTMAP_NONE, true);
			if (sh->defaultShader)
			{
				shader->shaderIndex = 0;
			}
			else
			{
				shader->shaderIndex = sh->index;
			}
		}

		// swap all the triangles
		md3Triangle_t* tri = (md3Triangle_t*)((byte*)surf + surf->ofsTriangles);
		for (int j = 0; j < surf->numTriangles; j++, tri++)
		{
			LL(tri->indexes[0]);
			LL(tri->indexes[1]);
			LL(tri->indexes[2]);
		}

		// swap all the ST
		md3St_t* st = (md3St_t*)((byte*)surf + surf->ofsSt);
		for (int j = 0; j < surf->numVerts; j++, st++)
		{
			st->st[0] = LittleFloat(st->st[0]);
			st->st[1] = LittleFloat(st->st[1]);
		}

		// swap all the XyzNormals
		md3XyzNormal_t* xyz = (md3XyzNormal_t*)((byte*)surf + surf->ofsXyzNormals);
		for (int j = 0; j < surf->numVerts * surf->numFrames; j++, xyz++) 
		{
			xyz->xyz[0] = LittleShort(xyz->xyz[0]);
			xyz->xyz[1] = LittleShort(xyz->xyz[1]);
			xyz->xyz[2] = LittleShort(xyz->xyz[2]);

			xyz->normal = LittleShort(xyz->normal);
		}

		// find the next surface
		surf = (md3Surface_t*)((byte*)surf + surf->ofsEnd);
	}

	return true;
}

//==========================================================================
//
//	R_LoadMd3
//
//==========================================================================

bool R_LoadMd3(model_t* mod, void* buffer)
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
			sprintf(namebuf, "_%d.md3", lod);
			String::Cat(filename, sizeof(filename), namebuf);

			FS_ReadFile(filename, (void**)&buf);
			if (!buf)
			{
				continue;
			}
		}

		int ident = LittleLong(*(unsigned*)buf);
		if (ident != MD3_IDENT)
		{
			Log::write(S_COLOR_YELLOW "R_LoadMd3: unknown fileid for %s\n", filename);
			return false;
		}

		bool loaded = R_LoadMd3Lod(mod, lod, buf, filename);

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
//			if ( lod <= r_lodbias->integer ) {
//				break;
//			}
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
		mod->q3_md3[lod] = mod->q3_md3[lod + 1];
	}

	return true;
}

//==========================================================================
//
//	R_FreeMd3
//
//==========================================================================

void R_FreeMd3(model_t* mod)
{
	Mem_Free(mod->q3_md3[0]);
	for (int lod = 1; lod < MD3_MAX_LODS; lod++)
	{
		if (mod->q3_md3[lod] != mod->q3_md3[lod - 1])
		{
			Mem_Free(mod->q3_md3[lod]);
		}
	}
}

//==========================================================================
//
//	ProjectRadius
//
//==========================================================================

static float ProjectRadius(float r, vec3_t location)
{
	float c = DotProduct(tr.viewParms.orient.axis[0], tr.viewParms.orient.origin);
	float dist = DotProduct(tr.viewParms.orient.axis[0], location) - c;

	if (dist <= 0)
	{
		return 0;
	}

	vec3_t p;
	p[0] = 0;
	p[1] = fabs(r);
	p[2] = -dist;

	float projected[4];
	projected[0] = p[0] * tr.viewParms.projectionMatrix[0] + 
		           p[1] * tr.viewParms.projectionMatrix[4] +
				   p[2] * tr.viewParms.projectionMatrix[8] +
				   tr.viewParms.projectionMatrix[12];

	projected[1] = p[0] * tr.viewParms.projectionMatrix[1] + 
		           p[1] * tr.viewParms.projectionMatrix[5] +
				   p[2] * tr.viewParms.projectionMatrix[9] +
				   tr.viewParms.projectionMatrix[13];

	projected[2] = p[0] * tr.viewParms.projectionMatrix[2] + 
		           p[1] * tr.viewParms.projectionMatrix[6] +
				   p[2] * tr.viewParms.projectionMatrix[10] +
				   tr.viewParms.projectionMatrix[14];

	projected[3] = p[0] * tr.viewParms.projectionMatrix[3] + 
		           p[1] * tr.viewParms.projectionMatrix[7] +
				   p[2] * tr.viewParms.projectionMatrix[11] +
				   tr.viewParms.projectionMatrix[15];


	float pr = projected[1] / projected[3];

	if (pr > 1.0f)
	{
		pr = 1.0f;
	}

	return pr;
}

//==========================================================================
//
//	R_CullModel
//
//==========================================================================

static int R_CullModel(md3Header_t* header, trRefEntity_t* ent)
{
	// compute frame pointers
	md3Frame_t* newFrame = (md3Frame_t*)((byte*)header + header->ofsFrames) + ent->e.frame;
	md3Frame_t* oldFrame = (md3Frame_t*)((byte*)header + header->ofsFrames) + ent->e.oldframe;

	// cull bounding sphere ONLY if this is not an upscaled entity
	if (!ent->e.nonNormalizedAxes)
	{
		if (ent->e.frame == ent->e.oldframe)
		{
			switch (R_CullLocalPointAndRadius(newFrame->localOrigin, newFrame->radius))
			{
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
		}
		else
		{
			int sphereCull = R_CullLocalPointAndRadius(newFrame->localOrigin, newFrame->radius);
			int sphereCullB;
			if (newFrame == oldFrame)
			{
				sphereCullB = sphereCull;
			}
			else
			{
				sphereCullB = R_CullLocalPointAndRadius(oldFrame->localOrigin, oldFrame->radius);
			}

			if (sphereCull == sphereCullB)
			{
				if (sphereCull == CULL_OUT)
				{
					tr.pc.c_sphere_cull_md3_out++;
					return CULL_OUT;
				}
				else if (sphereCull == CULL_IN)
				{
					tr.pc.c_sphere_cull_md3_in++;
					return CULL_IN;
				}
				else
				{
					tr.pc.c_sphere_cull_md3_clip++;
				}
			}
		}
	}

	// calculate a bounding box in the current coordinate system
	vec3_t bounds[2];
	for (int i = 0 ; i < 3 ; i++)
	{
		bounds[0][i] = oldFrame->bounds[0][i] < newFrame->bounds[0][i] ? oldFrame->bounds[0][i] : newFrame->bounds[0][i];
		bounds[1][i] = oldFrame->bounds[1][i] > newFrame->bounds[1][i] ? oldFrame->bounds[1][i] : newFrame->bounds[1][i];
	}

	switch (R_CullLocalBox(bounds))
	{
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

//==========================================================================
//
//	R_ComputeLOD
//
//==========================================================================

static int R_ComputeLOD(trRefEntity_t* ent)
{
	int lod;
	if (tr.currentModel->q3_numLods < 2)
	{
		// model has only 1 LOD level, skip computations and bias
		lod = 0;
	}
	else
	{
		// multiple LODs exist, so compute projected bounding sphere
		// and use that as a criteria for selecting LOD

		md3Frame_t* frame = (md3Frame_t*)(((byte*)tr.currentModel->q3_md3[0]) + tr.currentModel->q3_md3[0]->ofsFrames);

		frame += ent->e.frame;

		float radius = RadiusFromBounds(frame->bounds[0], frame->bounds[1]);

		float projectedRadius = ProjectRadius(radius, ent->e.origin);
		float flod;
		if (projectedRadius != 0)
		{
			float lodscale = r_lodscale->value;
			if (lodscale > 20)
			{
				lodscale = 20;
			}
			flod = 1.0f - projectedRadius * lodscale;
		}
		else
		{
			// object intersects near view plane, e.g. view weapon
			flod = 0;
		}

		flod *= tr.currentModel->q3_numLods;
		lod = myftol(flod);

		if (lod < 0)
		{
			lod = 0;
		}
		else if (lod >= tr.currentModel->q3_numLods)
		{
			lod = tr.currentModel->q3_numLods - 1;
		}
	}

	lod += r_lodbias->integer;

	if (lod >= tr.currentModel->q3_numLods)
	{
		lod = tr.currentModel->q3_numLods - 1;
	}
	if (lod < 0)
	{
		lod = 0;
	}

	return lod;
}

//==========================================================================
//
//	R_ComputeFogNum
//
//==========================================================================

static int R_ComputeFogNum(md3Header_t* header, trRefEntity_t* ent)
{
	if (tr.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return 0;
	}

	// FIXME: non-normalized axis issues
	md3Frame_t* md3Frame = (md3Frame_t*)((byte*)header + header->ofsFrames) + ent->e.frame;
	vec3_t localOrigin;
	VectorAdd(ent->e.origin, md3Frame->localOrigin, localOrigin);
	for (int i = 1; i < tr.world->numfogs; i++)
	{
		mbrush46_fog_t* fog = &tr.world->fogs[i];
		int j;
		for (j = 0; j < 3; j++)
		{
			if (localOrigin[j] - md3Frame->radius >= fog->bounds[1][j])
			{
				break;
			}
			if (localOrigin[j] + md3Frame->radius <= fog->bounds[0][j])
			{
				break;
			}
		}
		if (j == 3)
		{
			return i;
		}
	}

	return 0;
}

//==========================================================================
//
//	R_AddMD3Surfaces
//
//==========================================================================

void R_AddMD3Surfaces(trRefEntity_t* ent)
{
	// don't add third_person objects if not in a portal
	bool personalModel = (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal;

	if (ent->e.renderfx & RF_WRAP_FRAMES)
	{
		ent->e.frame %= tr.currentModel->q3_md3[0]->numFrames;
		ent->e.oldframe %= tr.currentModel->q3_md3[0]->numFrames;
	}

	//
	// Validate the frames so there is no chance of a crash.
	// This will write directly into the entity structure, so
	// when the surfaces are rendered, they don't need to be
	// range checked again.
	//
	if ((ent->e.frame >= tr.currentModel->q3_md3[0]->numFrames) ||
		(ent->e.frame < 0) ||
		(ent->e.oldframe >= tr.currentModel->q3_md3[0]->numFrames) ||
		(ent->e.oldframe < 0))
	{
		Log::develWrite(S_COLOR_RED "R_AddMD3Surfaces: no such frame %d to %d for '%s'\n",
			ent->e.oldframe, ent->e.frame, tr.currentModel->name);
		ent->e.frame = 0;
		ent->e.oldframe = 0;
	}

	//
	// compute LOD
	//
	int lod = R_ComputeLOD(ent);

	md3Header_t* header = tr.currentModel->q3_md3[lod];

	//
	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum.
	//
	int cull = R_CullModel(header, ent);
	if (cull == CULL_OUT)
	{
		return;
	}

	//
	// set up lighting now that we know we aren't culled
	//
	if (!personalModel || r_shadows->integer > 1)
	{
		R_SetupEntityLighting(&tr.refdef, ent);
	}

	//
	// see if we are in a fog volume
	//
	int fogNum = R_ComputeFogNum(header, ent);

	//
	// draw all surfaces
	//
	md3Surface_t* surface = (md3Surface_t*)((byte*)header + header->ofsSurfaces);
	for (int i = 0; i < header->numSurfaces; i++)
	{
		shader_t* shader;
		if (ent->e.customShader)
		{
			shader = R_GetShaderByHandle(ent->e.customShader);
		}
		else if (ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins)
		{
			skin_t* skin = R_GetSkinByHandle(ent->e.customSkin);

			// match the surface name to something in the skin file
			shader = tr.defaultShader;
			for (int j = 0; j < skin->numSurfaces; j++)
			{
				// the names have both been lowercased
				if (!String::Cmp(skin->surfaces[j]->name, surface->name))
				{
					shader = skin->surfaces[j]->shader;
					break;
				}
			}
			if (shader == tr.defaultShader)
			{
				Log::develWrite(S_COLOR_RED "WARNING: no shader for surface %s in skin %s\n", surface->name, skin->name);
			}
			else if (shader->defaultShader)
			{
				Log::develWrite(S_COLOR_RED "WARNING: shader %s in skin %s not found\n", shader->name, skin->name);
			}
		}
		else if (surface->numShaders <= 0)
		{
			shader = tr.defaultShader;
		}
		else
		{
			md3Shader_t* md3Shader = (md3Shader_t*)((byte*)surface + surface->ofsShaders);
			md3Shader += ent->e.skinNum % surface->numShaders;
			shader = tr.shaders[md3Shader->shaderIndex];
		}

		// we will add shadows even if the main object isn't visible in the view

		// stencil shadows can't do personal models unless I polyhedron clip
		if (!personalModel &&
			r_shadows->integer == 2 &&
			fogNum == 0 &&
			!(ent->e.renderfx & (RF_NOSHADOW | RF_DEPTHHACK)) &&
			shader->sort == SS_OPAQUE)
		{
			R_AddDrawSurf((surfaceType_t*)surface, tr.shadowShader, 0, false);
		}

		// projection shadows work fine with personal models
		if (r_shadows->integer == 3 &&
			fogNum == 0 &&
			(ent->e.renderfx & RF_SHADOW_PLANE) &&
			shader->sort == SS_OPAQUE)
		{
			R_AddDrawSurf((surfaceType_t*)surface, tr.projectionShadowShader, 0, false);
		}

		// don't add third_person objects if not viewing through a portal
		if (!personalModel)
		{
			R_AddDrawSurf((surfaceType_t*)surface, shader, fogNum, false);
		}

		surface = (md3Surface_t*)((byte*)surface + surface->ofsEnd);
	}
}

//==========================================================================
//
//	VectorArrayNormalize
//
//	The inputs to this routing seem to always be close to length = 1.0 (about 0.6 to 2.0)
//	This means that we don't have to worry about zero length or enormously long vectors.
//
//==========================================================================

static void VectorArrayNormalize(vec4_t* normals, unsigned int count)
{
	// given the input, it's safe to call VectorNormalizeFast
	while (count--)
	{
		VectorNormalizeFast(normals[0]);
		normals++;
	}
}

//==========================================================================
//
//	LerpMeshVertexes
//
//==========================================================================

static void LerpMeshVertexes(md3Surface_t* surf, float backlerp)
{
	float* outXyz = tess.xyz[tess.numVertexes];
	float* outNormal = tess.normal[tess.numVertexes];

	short* newXyz = (short*)((byte*)surf + surf->ofsXyzNormals) +
		(backEnd.currentEntity->e.frame * surf->numVerts * 4);
	short* newNormals = newXyz + 3;

	float newXyzScale = MD3_XYZ_SCALE * (1.0 - backlerp);
	float newNormalScale = 1.0 - backlerp;

	int numVerts = surf->numVerts;

	if (backlerp == 0)
	{
		//
		// just copy the vertexes
		//
		for (int vertNum = 0; vertNum < numVerts; vertNum++,
			newXyz += 4, newNormals += 4,
			outXyz += 4, outNormal += 4) 
		{
			outXyz[0] = newXyz[0] * newXyzScale;
			outXyz[1] = newXyz[1] * newXyzScale;
			outXyz[2] = newXyz[2] * newXyzScale;

			unsigned lat = (newNormals[0] >> 8) & 0xff;
			unsigned lng = (newNormals[0] & 0xff);
			lat *= (FUNCTABLE_SIZE / 256);
			lng *= (FUNCTABLE_SIZE / 256);

			// decode X as cos( lat ) * sin( long )
			// decode Y as sin( lat ) * sin( long )
			// decode Z as cos( long )

			outNormal[0] = tr.sinTable[(lat + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK] * tr.sinTable[lng];
			outNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			outNormal[2] = tr.sinTable[(lng + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK];
		}
	}
	else
	{
		//
		// interpolate and copy the vertex and normal
		//
		short* oldXyz = (short*)((byte*)surf + surf->ofsXyzNormals) +
			(backEnd.currentEntity->e.oldframe * surf->numVerts * 4);
		short* oldNormals = oldXyz + 3;

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

			// FIXME: interpolate lat/long instead?
			unsigned lat = (newNormals[0] >> 8) & 0xff;
			unsigned lng = (newNormals[0] & 0xff);
			lat *= 4;
			lng *= 4;
			vec3_t uncompressedNewNormal;
			uncompressedNewNormal[0] = tr.sinTable[(lat + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK] * tr.sinTable[lng];
			uncompressedNewNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			uncompressedNewNormal[2] = tr.sinTable[(lng + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK];

			lat = (oldNormals[0] >> 8) & 0xff;
			lng = (oldNormals[0] & 0xff);
			lat *= 4;
			lng *= 4;

			vec3_t uncompressedOldNormal;
			uncompressedOldNormal[0] = tr.sinTable[(lat + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK] * tr.sinTable[lng];
			uncompressedOldNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			uncompressedOldNormal[2] = tr.sinTable[(lng + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK];

			outNormal[0] = uncompressedOldNormal[0] * oldNormalScale + uncompressedNewNormal[0] * newNormalScale;
			outNormal[1] = uncompressedOldNormal[1] * oldNormalScale + uncompressedNewNormal[1] * newNormalScale;
			outNormal[2] = uncompressedOldNormal[2] * oldNormalScale + uncompressedNewNormal[2] * newNormalScale;
		}
		VectorArrayNormalize((vec4_t*)tess.normal[tess.numVertexes], numVerts);
	}
}

//==========================================================================
//
//	RB_SurfaceMesh
//
//==========================================================================

void RB_SurfaceMesh(md3Surface_t* surface)
{
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

	LerpMeshVertexes(surface, backlerp);

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
