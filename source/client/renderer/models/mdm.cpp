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

#include "../local.h"
#include "skeletal_model_inlines.h"

#define LL(x) x = LittleLong(x)

/*

All bones should be an identity orientation to display the mesh exactly
as it is specified.

For all other frames, the bones represent the transformation from the
orientation of the bone in the base frame to the orientation in this
frame.

*/

// undef to use floating-point lerping with explicit trig funcs
#define YD_INGLES

//#define DBG_PROFILE_BONES

#define ANGLES_SHORT_TO_FLOAT(pf, sh)     { *(pf++) = SHORT2ANGLE(*(sh++)); *(pf++) = SHORT2ANGLE(*(sh++)); *(pf++) = SHORT2ANGLE(*(sh++)); }

// ydnar

#define FUNCTABLE_SHIFT     (16 - FUNCTABLE_SIZE2)
#define SIN_TABLE(i)      tr.sinTable[(i) >> FUNCTABLE_SHIFT];
#define COS_TABLE(i)      tr.sinTable[(((i) >> FUNCTABLE_SHIFT) + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK];

#ifdef DBG_PROFILE_BONES
#define DBG_SHOWTIME    common->Printf("%i: %i, ", di++, (dt = ri.Milliseconds()) - ldt); ldt = dt;
#else
#define DBG_SHOWTIME    ;
#endif

//-----------------------------------------------------------------------------
// Static Vars, ugly but easiest (and fastest) means of seperating RB_SurfaceAnim
// and R_CalcBones

// NOTE: this only used at run-time
// FIXME: do we really need this?
struct mdxBoneFrame_t
{
	float matrix[3][3];				// 3x3 rotation
	vec3_t translation;				// translation vector
};

static float frontlerp, backlerp;
static float torsoFrontlerp, torsoBacklerp;
static int* triangles, * boneRefs;
static glIndex_t* pIndexes;
static int indexes;
static int baseIndex, baseVertex, oldIndexes;
static int numVerts;
static mdmVertex_t* v;
static mdxBoneFrame_t bones[MDX_MAX_BONES], rawBones[MDX_MAX_BONES], oldBones[MDX_MAX_BONES];
static char validBones[MDX_MAX_BONES];
static char newBones[MDX_MAX_BONES];
static mdxBoneFrame_t* bonePtr, * bone, * parentBone;
static mdxBoneFrameCompressed_t* cBonePtr, * cTBonePtr, * cOldBonePtr, * cOldTBonePtr, * cBoneList, * cOldBoneList, * cBoneListTorso, * cOldBoneListTorso;
static mdxBoneInfo_t* boneInfo, * thisBoneInfo, * parentBoneInfo;
static mdxFrame_t* frame, * torsoFrame;
static mdxFrame_t* oldFrame, * oldTorsoFrame;
static int frameSize;
static short* sh, * sh2;
static float* pf;
static int ingles[3], tingles[3];					// ydnar
static vec3_t angles, tangles, torsoParentOffset, torsoAxis[3];				//, tmpAxis[3];	// rain - unused
static float* tempVert, * tempNormal;
static vec3_t vec, v2, dir;
static float diff;				//, a1, a2;	// rain - unused
static int render_count;
static float lodRadius, lodScale;
static int* collapse_map, * pCollapseMap;
static int collapse[MDM_MAX_VERTS], * pCollapse;
static int p0, p1, p2;
static qboolean isTorso, fullTorso;
static vec4_t m1[4], m2[4];
static vec3_t t;
static refEntity_t lastBoneEntity;

static int totalrv, totalrt, totalv, totalt;				//----(SA)

//-----------------------------------------------------------------------------

bool R_LoadMdm(model_t* mod, const void* buffer)
{
	mdmHeader_t* pinmodel = (mdmHeader_t*)buffer;

	int version = LittleLong(pinmodel->version);
	if (version != MDM_VERSION)
	{
		common->Printf(S_COLOR_YELLOW "R_LoadMdm: %s has wrong version (%i should be %i)\n",
			mod->name, version, MDM_VERSION);
		return false;
	}

	mod->type = MOD_MDM;
	int size = LittleLong(pinmodel->ofsEnd);
	mod->q3_dataSize += size;
	mdmHeader_t* mdm = mod->q3_mdm = (mdmHeader_t*)Mem_Alloc(size);

	memcpy(mdm, buffer, LittleLong(pinmodel->ofsEnd));

	LL(mdm->ident);
	LL(mdm->version);
	LL(mdm->numTags);
	LL(mdm->numSurfaces);
	LL(mdm->ofsTags);
	LL(mdm->ofsEnd);
	LL(mdm->ofsSurfaces);
	mdm->lodBias = LittleFloat(mdm->lodBias);
	mdm->lodScale = LittleFloat(mdm->lodScale);

	if (LittleLong(1) != 1)
	{
		// swap all the tags
		mdmTag_t* tag = (mdmTag_t*)((byte*)mdm + mdm->ofsTags);
		for (int i = 0; i < mdm->numTags; i++)
		{
			for (int ii = 0; ii < 3; ii++)
			{
				tag->axis[ii][0] = LittleFloat(tag->axis[ii][0]);
				tag->axis[ii][1] = LittleFloat(tag->axis[ii][1]);
				tag->axis[ii][2] = LittleFloat(tag->axis[ii][2]);
			}

			LL(tag->boneIndex);
			tag->offset[0] = LittleFloat(tag->offset[0]);
			tag->offset[1] = LittleFloat(tag->offset[1]);
			tag->offset[2] = LittleFloat(tag->offset[2]);

			LL(tag->numBoneReferences);
			LL(tag->ofsBoneReferences);
			LL(tag->ofsEnd);

			// swap the bone references
			int* boneref = (int*)((byte*)tag + tag->ofsBoneReferences);
			for (int j = 0; j < tag->numBoneReferences; j++, boneref++)
			{
				*boneref = LittleLong(*boneref);
			}

			// find the next tag
			tag = (mdmTag_t*)((byte*)tag + tag->ofsEnd);
		}
	}

	// swap all the surfaces
	mdmSurface_t* surf = (mdmSurface_t*)((byte*)mdm + mdm->ofsSurfaces);
	for (int i = 0; i < mdm->numSurfaces; i++)
	{
		if (LittleLong(1) != 1)
		{
			//LL(surf->ident);
			LL(surf->shaderIndex);
			LL(surf->minLod);
			LL(surf->ofsHeader);
			LL(surf->ofsCollapseMap);
			LL(surf->numTriangles);
			LL(surf->ofsTriangles);
			LL(surf->numVerts);
			LL(surf->ofsVerts);
			LL(surf->numBoneReferences);
			LL(surf->ofsBoneReferences);
			LL(surf->ofsEnd);
		}

		// change to surface identifier
		surf->ident = SF_MDM;

		if (surf->numVerts > SHADER_MAX_VERTEXES)
		{
			common->Error("R_LoadMdm: %s has more than %i verts on a surface (%i)",
				mod->name, SHADER_MAX_VERTEXES, surf->numVerts);
		}
		if (surf->numTriangles * 3 > SHADER_MAX_INDEXES)
		{
			common->Error("R_LoadMdm: %s has more than %i triangles on a surface (%i)",
				mod->name, SHADER_MAX_INDEXES / 3, surf->numTriangles);
		}

		// register the shaders
		if (surf->shader[0])
		{
			shader_t* sh = R_FindShader(surf->shader, LIGHTMAP_NONE, true);
			if (sh->defaultShader)
			{
				surf->shaderIndex = 0;
			}
			else
			{
				surf->shaderIndex = sh->index;
			}
		}
		else
		{
			surf->shaderIndex = 0;
		}

		if (LittleLong(1) != 1)
		{
			// swap all the triangles
			mdmTriangle_t* tri = (mdmTriangle_t*)((byte*)surf + surf->ofsTriangles);
			for (int j = 0; j < surf->numTriangles; j++, tri++)
			{
				LL(tri->indexes[0]);
				LL(tri->indexes[1]);
				LL(tri->indexes[2]);
			}

			// swap all the vertexes
			mdmVertex_t* v = (mdmVertex_t*)((byte*)surf + surf->ofsVerts);
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

				v = (mdmVertex_t*)&v->weights[v->numWeights];
			}

			// swap the collapse map
			int* collapseMap = (int*)((byte*)surf + surf->ofsCollapseMap);
			for (int j = 0; j < surf->numVerts; j++, collapseMap++)
			{
				*collapseMap = LittleLong(*collapseMap);
			}

			// swap the bone references
			int* boneref = (int*)((byte*)surf + surf->ofsBoneReferences);
			for (int j = 0; j < surf->numBoneReferences; j++, boneref++)
			{
				*boneref = LittleLong(*boneref);
			}
		}

		// find the next surface
		surf = (mdmSurface_t*)((byte*)surf + surf->ofsEnd);
	}

	return true;
}

void R_FreeMdm(model_t* mod)
{
	Mem_Free(mod->q3_mdm);
}

bool R_LoadMdx(model_t* mod, const void* buffer)
{
	mdxHeader_t* pinmodel = (mdxHeader_t*)buffer;

	int version = LittleLong(pinmodel->version);
	if (version != MDX_VERSION)
	{
		common->Printf(S_COLOR_YELLOW "R_LoadMdx: %s has wrong version (%i should be %i)\n",
			mod->name, version, MDX_VERSION);
		return false;
	}

	mod->type = MOD_MDX;
	int size = LittleLong(pinmodel->ofsEnd);
	mod->q3_dataSize += size;
	mdxHeader_t* mdx = mod->q3_mdx = (mdxHeader_t*)Mem_Alloc(size);

	memcpy(mdx, buffer, LittleLong(pinmodel->ofsEnd));

	LL(mdx->ident);
	LL(mdx->version);
	LL(mdx->numFrames);
	LL(mdx->numBones);
	LL(mdx->ofsFrames);
	LL(mdx->ofsBones);
	LL(mdx->ofsEnd);
	LL(mdx->torsoParent);

	if (LittleLong(1) != 1)
	{
		// swap all the frames
		int frameSize = (int)(sizeof(mdxBoneFrameCompressed_t)) * mdx->numBones;
		for (int i = 0; i < mdx->numFrames; i++)
		{
			mdxFrame_t* frame = (mdxFrame_t*)((byte*)mdx + mdx->ofsFrames + i * frameSize + i * sizeof(mdxFrame_t));
			frame->radius = LittleFloat(frame->radius);
			for (int j = 0; j < 3; j++)
			{
				frame->bounds[0][j] = LittleFloat(frame->bounds[0][j]);
				frame->bounds[1][j] = LittleFloat(frame->bounds[1][j]);
				frame->localOrigin[j] = LittleFloat(frame->localOrigin[j]);
				frame->parentOffset[j] = LittleFloat(frame->parentOffset[j]);
			}

			short* bframe = (short*)((byte*)mdx + mdx->ofsFrames + i * frameSize + ((i + 1) * sizeof(mdxFrame_t)));
			for (int j = 0; j < mdx->numBones * (int)sizeof(mdxBoneFrameCompressed_t) / (int)sizeof(short); j++)
			{
				bframe[j] = LittleShort(bframe[j]);
			}
		}

		// swap all the bones
		for (int i = 0; i < mdx->numBones; i++)
		{
			mdxBoneInfo_t* bi = (mdxBoneInfo_t*)((byte*)mdx + mdx->ofsBones + i * sizeof(mdxBoneInfo_t));
			LL(bi->parent);
			bi->torsoWeight = LittleFloat(bi->torsoWeight);
			bi->parentDist = LittleFloat(bi->parentDist);
			LL(bi->flags);
		}
	}

	return true;
}

void R_FreeMdx(model_t* mod)
{
	Mem_Free(mod->q3_mdx);
}

static int R_CullModel(trRefEntity_t* ent)
{
	mdxHeader_t* newFrameHeader = R_GetModelByHandle(ent->e.frameModel)->q3_mdx;
	mdxHeader_t* oldFrameHeader = R_GetModelByHandle(ent->e.oldframeModel)->q3_mdx;

	if (!newFrameHeader || !oldFrameHeader)
	{
		return CULL_OUT;
	}

	// compute frame pointers
	mdxFrame_t* newFrame = (mdxFrame_t*)((byte*)newFrameHeader + newFrameHeader->ofsFrames +
										 ent->e.frame * (int)(sizeof(mdxBoneFrameCompressed_t)) * newFrameHeader->numBones +
										 ent->e.frame * sizeof(mdxFrame_t));
	mdxFrame_t* oldFrame = (mdxFrame_t*)((byte*)oldFrameHeader + oldFrameHeader->ofsFrames +
										 ent->e.oldframe * (int)(sizeof(mdxBoneFrameCompressed_t)) * oldFrameHeader->numBones +
										 ent->e.oldframe * sizeof(mdxFrame_t));

	// cull bounding sphere ONLY if this is not an upscaled entity
	if (!ent->e.nonNormalizedAxes)
	{
		if (ent->e.frame == ent->e.oldframe && ent->e.frameModel == ent->e.oldframeModel)
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
			int sphereCull, sphereCullB;

			sphereCull  = R_CullLocalPointAndRadius(newFrame->localOrigin, newFrame->radius);
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
	for (int i = 0; i < 3; i++)
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

static int R_ComputeFogNum(trRefEntity_t* ent)
{
	if (tr.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return 0;
	}

	mdxHeader_t* header = R_GetModelByHandle(ent->e.frameModel)->q3_mdx;

	// compute frame pointers
	mdxFrame_t* mdxFrame = (mdxFrame_t*)((byte*)header + header->ofsFrames +
										 ent->e.frame * (int)(sizeof(mdxBoneFrameCompressed_t)) * header->numBones +
										 ent->e.frame * sizeof(mdxFrame_t));

	// FIXME: non-normalized axis issues
	vec3_t localOrigin;
	VectorAdd(ent->e.origin, mdxFrame->localOrigin, localOrigin);
	for (int i = 1; i < tr.world->numfogs; i++)
	{
		mbrush46_fog_t* fog = &tr.world->fogs[i];
		int j;
		for (j = 0; j < 3; j++)
		{
			if (localOrigin[j] - mdxFrame->radius >= fog->bounds[1][j])
			{
				break;
			}
			if (localOrigin[j] + mdxFrame->radius <= fog->bounds[0][j])
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

void R_MDM_AddAnimSurfaces(trRefEntity_t* ent)
{
	// don't add third_person objects if not in a portal
	bool personalModel = (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal;

	mdmHeader_t* header = tr.currentModel->q3_mdm;

	//
	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum.
	//
	int cull = R_CullModel(ent);
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
	int fogNum = R_ComputeFogNum(ent);

	mdmSurface_t* surface = (mdmSurface_t*)((byte*)header + header->ofsSurfaces);
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

			if (ent->e.renderfx & RF_BLINK)
			{
				char* s = va("%s_b", surface->name);	// append '_b' for 'blink'
				int hash = Com_HashKey(s, String::Length(s));
				for (int j = 0; j < skin->numSurfaces; j++)
				{
					if (hash != skin->surfaces[j]->hash)
					{
						continue;
					}
					if (!String::Cmp(skin->surfaces[j]->name, s))
					{
						shader = skin->surfaces[j]->shader;
						break;
					}
				}
			}

			if (shader == tr.defaultShader)		// blink reference in skin was not found
			{
				int hash = Com_HashKey(surface->name, sizeof(surface->name));
				for (int j = 0; j < skin->numSurfaces; j++)
				{
					// the names have both been lowercased
					if (hash != skin->surfaces[j]->hash)
					{
						continue;
					}
					if (!String::Cmp(skin->surfaces[j]->name, surface->name))
					{
						shader = skin->surfaces[j]->shader;
						break;
					}
				}
			}

			if (shader == tr.defaultShader)
			{
				common->DPrintf(S_COLOR_RED "WARNING: no shader for surface %s in skin %s\n", surface->name, skin->name);
			}
			else if (shader->defaultShader)
			{
				common->DPrintf(S_COLOR_RED "WARNING: shader %s in skin %s not found\n", shader->name, skin->name);
			}
		}
		else
		{
			shader = R_GetShaderByHandle(surface->shaderIndex);
		}

		// don't add third_person objects if not viewing through a portal
		if (!personalModel)
		{
			R_AddDrawSurf((surfaceType_t*)surface, shader, fogNum, 0, 0, 0);
		}

		surface = (mdmSurface_t*)((byte*)surface + surface->ofsEnd);
	}
}

static float ProjectRadius(float r, vec3_t location)
{
	float pr;
	float dist;
	float c;
	vec3_t p;
	float projected[4];

	c = DotProduct(tr.viewParms.orient.axis[0], tr.viewParms.orient.origin);
	dist = DotProduct(tr.viewParms.orient.axis[0], location) - c;

	if (dist <= 0)
	{
		return 0;
	}

	p[0] = 0;
	p[1] = Q_fabs(r);
	p[2] = -dist;

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


	pr = projected[1] / projected[3];

	if (pr > 1.0f)
	{
		pr = 1.0f;
	}

	return pr;
}

static inline void LocalIngleVector(int ingles[3], vec3_t forward)
{
	float sy = SIN_TABLE(ingles[YAW] & 65535);
	float cy = COS_TABLE(ingles[YAW] & 65535);
	float sp = SIN_TABLE(ingles[PITCH] & 65535);
	float cp = COS_TABLE(ingles[PITCH] & 65535);

	//%	sy = sin( SHORT2ANGLE( ingles[ YAW ] ) * (M_PI*2 / 360) );
	//%	cy = cos( SHORT2ANGLE( ingles[ YAW ] ) * (M_PI*2 / 360) );
	//%	sp = sin( SHORT2ANGLE( ingles[ PITCH ] ) * (M_PI*2 / 360) );
	//%	cp = cos( SHORT2ANGLE( ingles[ PITCH ] ) *  (M_PI*2 / 360) );

	forward[0] = cp * cy;
	forward[1] = cp * sy;
	forward[2] = -sp;
}

static void InglesToAxis(int ingles[3], vec3_t axis[3])
{
	// get sine/cosines for angles
	float sy = SIN_TABLE(ingles[YAW] & 65535);
	float cy = COS_TABLE(ingles[YAW] & 65535);
	float sp = SIN_TABLE(ingles[PITCH] & 65535);
	float cp = COS_TABLE(ingles[PITCH] & 65535);
	float sr = SIN_TABLE(ingles[ROLL] & 65535);
	float cr = COS_TABLE(ingles[ROLL] & 65535);

	// calculate axis vecs
	axis[0][0] = cp * cy;
	axis[0][1] = cp * sy;
	axis[0][2] = -sp;

	axis[1][0] = sr * sp * cy + cr * -sy;
	axis[1][1] = sr * sp * sy + cr * cy;
	axis[1][2] = sr * cp;

	axis[2][0] = cr * sp * cy + -sr * -sy;
	axis[2][1] = cr * sp * sy + -sr * cy;
	axis[2][2] = cr * cp;
}

/*
==============
R_CalcBone
==============
*/
static void R_CalcBone(const int torsoParent, const refEntity_t* refent, int boneNum)
{
	int j;

	thisBoneInfo = &boneInfo[boneNum];
	if (thisBoneInfo->torsoWeight)
	{
		cTBonePtr = &cBoneListTorso[boneNum];
		isTorso = true;
		if (thisBoneInfo->torsoWeight == 1.0f)
		{
			fullTorso = true;
		}
	}
	else
	{
		isTorso = false;
		fullTorso = false;
	}
	cBonePtr = &cBoneList[boneNum];

	bonePtr = &bones[boneNum];

	// we can assume the parent has already been uncompressed for this frame + lerp
	if (thisBoneInfo->parent >= 0)
	{
		parentBone = &bones[thisBoneInfo->parent];
		parentBoneInfo = &boneInfo[thisBoneInfo->parent];
	}
	else
	{
		parentBone = NULL;
		parentBoneInfo = NULL;
	}

	// rotation
	if (fullTorso)
	{
		sh = (short*)cTBonePtr->angles;
		pf = angles;
		ANGLES_SHORT_TO_FLOAT(pf, sh);
	}
	else
	{
		sh = (short*)cBonePtr->angles;
		pf = angles;
		ANGLES_SHORT_TO_FLOAT(pf, sh);
		if (isTorso)
		{
			sh = (short*)cTBonePtr->angles;
			pf = tangles;
			ANGLES_SHORT_TO_FLOAT(pf, sh);
			// blend the angles together
			for (j = 0; j < 3; j++)
			{
				diff = tangles[j] - angles[j];
				if (Q_fabs(diff) > 180)
				{
					diff = AngleNormalize180(diff);
				}
				angles[j] = angles[j] + thisBoneInfo->torsoWeight * diff;
			}
		}
	}
	AnglesToAxis(angles, bonePtr->matrix);

	// translation
	if (parentBone)
	{
		if (fullTorso)
		{
			#ifndef YD_INGLES
			sh = (short*)cTBonePtr->ofsAngles; pf = angles;
			*(pf++) = SHORT2ANGLE(*(sh++)); *(pf++) = SHORT2ANGLE(*(sh++)); *(pf++) = 0;
			LocalAngleVector(angles, vec);
			#else
			sh = (short*)cTBonePtr->ofsAngles;
			ingles[0] = sh[0];
			ingles[1] = sh[1];
			ingles[2] = 0;
			LocalIngleVector(ingles, vec);
			#endif

			VectorMA(parentBone->translation, thisBoneInfo->parentDist, vec, bonePtr->translation);
		}
		else
		{
			#ifndef YD_INGLES
			sh = (short*)cBonePtr->ofsAngles; pf = angles;
			*(pf++) = SHORT2ANGLE(*(sh++)); *(pf++) = SHORT2ANGLE(*(sh++)); *(pf++) = 0;
			LocalAngleVector(angles, vec);
			#else
			sh = (short*)cBonePtr->ofsAngles;
			ingles[0] = sh[0];
			ingles[1] = sh[1];
			ingles[2] = 0;
			LocalIngleVector(ingles, vec);
			#endif

			if (isTorso)
			{
				#ifndef YD_INGLES
				sh = (short*)cTBonePtr->ofsAngles;
				pf = tangles;
				*(pf++) = SHORT2ANGLE(*(sh++)); *(pf++) = SHORT2ANGLE(*(sh++)); *(pf++) = 0;
				LocalAngleVector(tangles, v2);
				#else
				sh = (short*)cTBonePtr->ofsAngles;
				tingles[0] = sh[0];
				tingles[1] = sh[1];
				tingles[2] = 0;
				LocalIngleVector(tingles, v2);
				#endif

				// blend the angles together
				SLerp_Normal(vec, v2, thisBoneInfo->torsoWeight, vec);
				VectorMA(parentBone->translation, thisBoneInfo->parentDist, vec, bonePtr->translation);

			}
			else		// legs bone
			{
				VectorMA(parentBone->translation, thisBoneInfo->parentDist, vec, bonePtr->translation);
			}
		}
	}
	else		// just use the frame position
	{
		bonePtr->translation[0] = frame->parentOffset[0];
		bonePtr->translation[1] = frame->parentOffset[1];
		bonePtr->translation[2] = frame->parentOffset[2];
	}
	//
	if (boneNum == torsoParent)		// this is the torsoParent
	{
		VectorCopy(bonePtr->translation, torsoParentOffset);
	}
	//
	validBones[boneNum] = 1;
	//
	rawBones[boneNum] = *bonePtr;
	newBones[boneNum] = 1;

}

/*
==============
R_CalcBoneLerp
==============
*/
static void R_CalcBoneLerp(const int torsoParent, const refEntity_t* refent, int boneNum)
{
	int j;

	if (!refent || boneNum < 0 || boneNum >= MDX_MAX_BONES)
	{
		return;
	}


	thisBoneInfo = &boneInfo[boneNum];

	if (!thisBoneInfo)
	{
		return;
	}

	if (thisBoneInfo->parent >= 0)
	{
		parentBone = &bones[thisBoneInfo->parent];
		parentBoneInfo = &boneInfo[thisBoneInfo->parent];
	}
	else
	{
		parentBone = NULL;
		parentBoneInfo = NULL;
	}

	if (thisBoneInfo->torsoWeight)
	{
		cTBonePtr = &cBoneListTorso[boneNum];
		cOldTBonePtr = &cOldBoneListTorso[boneNum];
		isTorso = true;
		if (thisBoneInfo->torsoWeight == 1.0f)
		{
			fullTorso = true;
		}
	}
	else
	{
		isTorso = false;
		fullTorso = false;
	}
	cBonePtr = &cBoneList[boneNum];
	cOldBonePtr = &cOldBoneList[boneNum];

	bonePtr = &bones[boneNum];

	newBones[boneNum] = 1;

	// rotation (take into account 170 to -170 lerps, which need to take the shortest route)
	#ifndef YD_INGLES

	if (fullTorso)
	{
		sh = (short*)cTBonePtr->angles;
		sh2 = (short*)cOldTBonePtr->angles;
		pf = angles;

		a1 = SHORT2ANGLE(*(sh++)); a2 = SHORT2ANGLE(*(sh2++)); diff = AngleNormalize180(a1 - a2);
		*(pf++) = a1 - torsoBacklerp * diff;
		a1 = SHORT2ANGLE(*(sh++)); a2 = SHORT2ANGLE(*(sh2++)); diff = AngleNormalize180(a1 - a2);
		*(pf++) = a1 - torsoBacklerp * diff;
		a1 = SHORT2ANGLE(*(sh++)); a2 = SHORT2ANGLE(*(sh2++)); diff = AngleNormalize180(a1 - a2);
		*(pf++) = a1 - torsoBacklerp * diff;
	}
	else
	{
		sh = (short*)cBonePtr->angles;
		sh2 = (short*)cOldBonePtr->angles;
		pf = angles;

		a1 = SHORT2ANGLE(*(sh++)); a2 = SHORT2ANGLE(*(sh2++)); diff = AngleNormalize180(a1 - a2);
		*(pf++) = a1 - backlerp * diff;
		a1 = SHORT2ANGLE(*(sh++)); a2 = SHORT2ANGLE(*(sh2++)); diff = AngleNormalize180(a1 - a2);
		*(pf++) = a1 - backlerp * diff;
		a1 = SHORT2ANGLE(*(sh++)); a2 = SHORT2ANGLE(*(sh2++)); diff = AngleNormalize180(a1 - a2);
		*(pf++) = a1 - backlerp * diff;

		if (isTorso)
		{
			sh = (short*)cTBonePtr->angles;
			sh2 = (short*)cOldTBonePtr->angles;
			pf = tangles;

			a1 = SHORT2ANGLE(*(sh++)); a2 = SHORT2ANGLE(*(sh2++)); diff = AngleNormalize180(a1 - a2);
			*(pf++) = a1 - torsoBacklerp * diff;
			a1 = SHORT2ANGLE(*(sh++)); a2 = SHORT2ANGLE(*(sh2++)); diff = AngleNormalize180(a1 - a2);
			*(pf++) = a1 - torsoBacklerp * diff;
			a1 = SHORT2ANGLE(*(sh++)); a2 = SHORT2ANGLE(*(sh2++)); diff = AngleNormalize180(a1 - a2);
			*(pf++) = a1 - torsoBacklerp * diff;

			// blend the angles together
			for (j = 0; j < 3; j++)
			{
				diff = tangles[j] - angles[j];
				if (Q_fabs(diff) > 180)
				{
					diff = AngleNormalize180(diff);
				}
				angles[j] = angles[j] + thisBoneInfo->torsoWeight * diff;
			}

		}

	}

	AnglesToAxis(angles, bonePtr->matrix);

	#else

	// ydnar: ingles-based bone code
	if (fullTorso)
	{
		sh = (short*)cTBonePtr->angles;
		sh2 = (short*)cOldTBonePtr->angles;
		for (j = 0; j < 3; j++)
		{
			ingles[j] = (sh[j] - sh2[j]) & 65535;
			if (ingles[j] > 32767)
			{
				ingles[j] -= 65536;
			}
			ingles[j] = sh[j] - torsoBacklerp * ingles[j];
		}
	}
	else
	{
		sh = (short*)cBonePtr->angles;
		sh2 = (short*)cOldBonePtr->angles;
		for (j = 0; j < 3; j++)
		{
			ingles[j] = (sh[j] - sh2[j]) & 65535;
			if (ingles[j] > 32767)
			{
				ingles[j] -= 65536;
			}
			ingles[j] = sh[j] - backlerp * ingles[j];
		}

		if (isTorso)
		{
			sh = (short*)cTBonePtr->angles;
			sh2 = (short*)cOldTBonePtr->angles;
			for (j = 0; j < 3; j++)
			{
				tingles[j] = (sh[j] - sh2[j]) & 65535;
				if (tingles[j] > 32767)
				{
					tingles[j] -= 65536;
				}
				tingles[j] = sh[j] - torsoBacklerp * tingles[j];

				// blend torso and angles
				tingles[j] = (tingles[j] - ingles[j]) & 65535;
				if (tingles[j] > 32767)
				{
					tingles[j] -= 65536;
				}
				ingles[j] += thisBoneInfo->torsoWeight * tingles[j];
			}
		}

	}

	InglesToAxis(ingles, bonePtr->matrix);

	#endif





	if (parentBone)
	{

		if (fullTorso)
		{
			sh = (short*)cTBonePtr->ofsAngles;
			sh2 = (short*)cOldTBonePtr->ofsAngles;
		}
		else
		{
			sh = (short*)cBonePtr->ofsAngles;
			sh2 = (short*)cOldBonePtr->ofsAngles;
		}

		#ifndef YD_INGLES
		pf = angles;
		*(pf++) = SHORT2ANGLE(*(sh++));
		*(pf++) = SHORT2ANGLE(*(sh++));
		*(pf++) = 0;
		LocalAngleVector(angles, v2);			// new

		pf = angles;
		*(pf++) = SHORT2ANGLE(*(sh2++));
		*(pf++) = SHORT2ANGLE(*(sh2++));
		*(pf++) = 0;
		LocalAngleVector(angles, vec);			// old
		#else
		ingles[0] = sh[0];
		ingles[1] = sh[1];
		ingles[2] = 0;
		LocalIngleVector(ingles, v2);			// new

		ingles[0] = sh2[0];
		ingles[1] = sh2[1];
		ingles[2] = 0;
		LocalIngleVector(ingles, vec);			// old
		#endif

		// blend the angles together
		if (fullTorso)
		{
			SLerp_Normal(vec, v2, torsoFrontlerp, dir);
		}
		else
		{
			SLerp_Normal(vec, v2, frontlerp, dir);
		}

		// translation
		if (!fullTorso && isTorso)			// partial legs/torso, need to lerp according to torsoWeight

		{	// calc the torso frame
			sh = (short*)cTBonePtr->ofsAngles;
			sh2 = (short*)cOldTBonePtr->ofsAngles;

			#ifndef YD_INGLES
			pf = angles;
			*(pf++) = SHORT2ANGLE(*(sh++));
			*(pf++) = SHORT2ANGLE(*(sh++));
			*(pf++) = 0;
			LocalAngleVector(angles, v2);			// new

			pf = angles;
			*(pf++) = SHORT2ANGLE(*(sh2++));
			*(pf++) = SHORT2ANGLE(*(sh2++));
			*(pf++) = 0;
			LocalAngleVector(angles, vec);			// old
			#else
			ingles[0] = sh[0];
			ingles[1] = sh[1];
			ingles[2] = 0;
			LocalIngleVector(ingles, v2);			// new

			ingles[0] = sh[0];
			ingles[1] = sh[1];
			ingles[2] = 0;
			LocalIngleVector(ingles, vec);			// old
			#endif

			// blend the angles together
			SLerp_Normal(vec, v2, torsoFrontlerp, v2);

			// blend the torso/legs together
			SLerp_Normal(dir, v2, thisBoneInfo->torsoWeight, dir);

		}

		VectorMA(parentBone->translation, thisBoneInfo->parentDist, dir, bonePtr->translation);

	}
	else		// just interpolate the frame positions

	{
		bonePtr->translation[0] = frontlerp * frame->parentOffset[0] + backlerp * oldFrame->parentOffset[0];
		bonePtr->translation[1] = frontlerp * frame->parentOffset[1] + backlerp * oldFrame->parentOffset[1];
		bonePtr->translation[2] = frontlerp * frame->parentOffset[2] + backlerp * oldFrame->parentOffset[2];

	}
	//
	if (boneNum == torsoParent)		// this is the torsoParent
	{
		VectorCopy(bonePtr->translation, torsoParentOffset);
	}
	validBones[boneNum] = 1;
	//
	rawBones[boneNum] = *bonePtr;
	newBones[boneNum] = 1;

}

/*
==============
R_BonesStillValid

    FIXME: optimization opportunity here, profile which values change most often and check for those first to get early outs

    Other way we could do this is doing a random memory probe, which in worst case scenario ends up being the memcmp? - BAD as only a few values are used

    Another solution: bones cache on an entity basis?
==============
*/
static qboolean R_BonesStillValid(const refEntity_t* refent)
{
	if (lastBoneEntity.hModel != refent->hModel)
	{
		return false;
	}
	else if (lastBoneEntity.frame != refent->frame)
	{
		return false;
	}
	else if (lastBoneEntity.oldframe != refent->oldframe)
	{
		return false;
	}
	else if (lastBoneEntity.frameModel != refent->frameModel)
	{
		return false;
	}
	else if (lastBoneEntity.oldframeModel != refent->oldframeModel)
	{
		return false;
	}
	else if (lastBoneEntity.backlerp != refent->backlerp)
	{
		return false;
	}
	else if (lastBoneEntity.torsoFrame != refent->torsoFrame)
	{
		return false;
	}
	else if (lastBoneEntity.oldTorsoFrame != refent->oldTorsoFrame)
	{
		return false;
	}
	else if (lastBoneEntity.torsoFrameModel != refent->torsoFrameModel)
	{
		return false;
	}
	else if (lastBoneEntity.oldTorsoFrameModel != refent->oldTorsoFrameModel)
	{
		return false;
	}
	else if (lastBoneEntity.torsoBacklerp != refent->torsoBacklerp)
	{
		return false;
	}
	else if (lastBoneEntity.reFlags != refent->reFlags)
	{
		return false;
	}
	else if (!VectorCompare(lastBoneEntity.torsoAxis[0], refent->torsoAxis[0]) ||
			 !VectorCompare(lastBoneEntity.torsoAxis[1], refent->torsoAxis[1]) ||
			 !VectorCompare(lastBoneEntity.torsoAxis[2], refent->torsoAxis[2]))
	{
		return false;
	}

	return true;
}

/*
==============
R_CalcBones

    The list of bones[] should only be built and modified from within here
==============
*/
static void R_CalcBones(const refEntity_t* refent, int* boneList, int numBones)
{

	int i;
	int* boneRefs;
	float torsoWeight;
	mdxHeader_t* mdxFrameHeader = R_GetModelByHandle(refent->frameModel)->q3_mdx;
	mdxHeader_t* mdxOldFrameHeader = R_GetModelByHandle(refent->oldframeModel)->q3_mdx;
	mdxHeader_t* mdxTorsoFrameHeader = R_GetModelByHandle(refent->torsoFrameModel)->q3_mdx;
	mdxHeader_t* mdxOldTorsoFrameHeader = R_GetModelByHandle(refent->oldTorsoFrameModel)->q3_mdx;

	if (!mdxFrameHeader || !mdxOldFrameHeader || !mdxTorsoFrameHeader || !mdxOldTorsoFrameHeader)
	{
		return;
	}

	//
	// if the entity has changed since the last time the bones were built, reset them
	//
	if (!R_BonesStillValid(refent))
	{
		// different, cached bones are not valid
		memset(validBones, 0, mdxFrameHeader->numBones);
		lastBoneEntity = *refent;

		// (SA) also reset these counter statics
//----(SA)	print stats for the complete model (not per-surface)
		if (r_bonesDebug->integer == 4 && totalrt)
		{
			common->Printf("Lod %.2f  verts %4d/%4d  tris %4d/%4d  (%.2f%%)\n",
				lodScale,
				totalrv,
				totalv,
				totalrt,
				totalt,
				(float)(100.0 * totalrt) / (float)totalt);
		}
//----(SA)	end
		totalrv = totalrt = totalv = totalt = 0;
	}

	memset(newBones, 0, mdxFrameHeader->numBones);

	if (refent->oldframe == refent->frame && refent->oldframeModel == refent->frameModel)
	{
		backlerp = 0;
		frontlerp = 1;
	}
	else
	{
		backlerp = refent->backlerp;
		frontlerp = 1.0f - backlerp;
	}

	if (refent->oldTorsoFrame == refent->torsoFrame && refent->oldTorsoFrameModel == refent->oldframeModel)
	{
		torsoBacklerp = 0;
		torsoFrontlerp = 1;
	}
	else
	{
		torsoBacklerp = refent->torsoBacklerp;
		torsoFrontlerp = 1.0f - torsoBacklerp;
	}

	frame = (mdxFrame_t*)((byte*)mdxFrameHeader + mdxFrameHeader->ofsFrames +
						  refent->frame * (int)(sizeof(mdxBoneFrameCompressed_t)) * mdxFrameHeader->numBones +
						  refent->frame * sizeof(mdxFrame_t));
	torsoFrame = (mdxFrame_t*)((byte*)mdxTorsoFrameHeader + mdxTorsoFrameHeader->ofsFrames +
							   refent->torsoFrame * (int)(sizeof(mdxBoneFrameCompressed_t)) * mdxTorsoFrameHeader->numBones +
							   refent->torsoFrame * sizeof(mdxFrame_t));
	oldFrame = (mdxFrame_t*)((byte*)mdxOldFrameHeader + mdxOldFrameHeader->ofsFrames +
							 refent->oldframe * (int)(sizeof(mdxBoneFrameCompressed_t)) * mdxOldFrameHeader->numBones +
							 refent->oldframe * sizeof(mdxFrame_t));
	oldTorsoFrame = (mdxFrame_t*)((byte*)mdxOldTorsoFrameHeader + mdxOldTorsoFrameHeader->ofsFrames +
								  refent->oldTorsoFrame * (int)(sizeof(mdxBoneFrameCompressed_t)) * mdxOldTorsoFrameHeader->numBones +
								  refent->oldTorsoFrame * sizeof(mdxFrame_t));

	//
	// lerp all the needed bones (torsoParent is always the first bone in the list)
	//
	frameSize = (int)sizeof(mdxBoneFrameCompressed_t) * mdxFrameHeader->numBones;

	cBoneList = (mdxBoneFrameCompressed_t*)((byte*)mdxFrameHeader + mdxFrameHeader->ofsFrames +
											(refent->frame + 1) * sizeof(mdxFrame_t) +
											refent->frame * frameSize);
	cBoneListTorso = (mdxBoneFrameCompressed_t*)((byte*)mdxTorsoFrameHeader + mdxTorsoFrameHeader->ofsFrames +
												 (refent->torsoFrame + 1) * sizeof(mdxFrame_t) +
												 refent->torsoFrame * frameSize);

	boneInfo = (mdxBoneInfo_t*)((byte*)mdxFrameHeader + mdxFrameHeader->ofsBones);
	boneRefs = boneList;
	//
	Matrix3Transpose(refent->torsoAxis, torsoAxis);

	if (!backlerp && !torsoBacklerp)
	{

		for (i = 0; i < numBones; i++, boneRefs++)
		{

			if (validBones[*boneRefs])
			{
				// this bone is still in the cache
				bones[*boneRefs] = rawBones[*boneRefs];
				continue;
			}

			// find our parent, and make sure it has been calculated
			if ((boneInfo[*boneRefs].parent >= 0) && (!validBones[boneInfo[*boneRefs].parent] && !newBones[boneInfo[*boneRefs].parent]))
			{
				R_CalcBone(mdxFrameHeader->torsoParent, refent, boneInfo[*boneRefs].parent);
			}

			R_CalcBone(mdxFrameHeader->torsoParent, refent, *boneRefs);

		}

	}
	else		// interpolated

	{
		cOldBoneList = (mdxBoneFrameCompressed_t*)((byte*)mdxOldFrameHeader + mdxOldFrameHeader->ofsFrames +
												   (refent->oldframe + 1) * sizeof(mdxFrame_t) +
												   refent->oldframe * frameSize);
		cOldBoneListTorso = (mdxBoneFrameCompressed_t*)((byte*)mdxOldTorsoFrameHeader + mdxOldTorsoFrameHeader->ofsFrames +
														(refent->oldTorsoFrame + 1) * sizeof(mdxFrame_t) +
														refent->oldTorsoFrame * frameSize);

		for (i = 0; i < numBones; i++, boneRefs++)
		{

			if (validBones[*boneRefs])
			{
				// this bone is still in the cache
				bones[*boneRefs] = rawBones[*boneRefs];
				continue;
			}

			// find our parent, and make sure it has been calculated
			if ((boneInfo[*boneRefs].parent >= 0) && (!validBones[boneInfo[*boneRefs].parent] && !newBones[boneInfo[*boneRefs].parent]))
			{
				R_CalcBoneLerp(mdxFrameHeader->torsoParent, refent, boneInfo[*boneRefs].parent);
			}

			R_CalcBoneLerp(mdxFrameHeader->torsoParent, refent, *boneRefs);

		}

	}

	// adjust for torso rotations
	torsoWeight = 0;
	boneRefs = boneList;
	for (i = 0; i < numBones; i++, boneRefs++)
	{

		thisBoneInfo = &boneInfo[*boneRefs];
		bonePtr = &bones[*boneRefs];
		// add torso rotation
		if (thisBoneInfo->torsoWeight > 0)
		{

			if (!newBones[*boneRefs])
			{
				// just copy it back from the previous calc
				bones[*boneRefs] = oldBones[*boneRefs];
				continue;
			}

			//if ( !(thisBoneInfo->flags & MDS_BONEFLAG_TAG) ) {

			// 1st multiply with the bone->matrix
			// 2nd translation for rotation relative to bone around torso parent offset
			VectorSubtract(bonePtr->translation, torsoParentOffset, t);
			Matrix4FromAxisPlusTranslation(bonePtr->matrix, t, m1);
			// 3rd scaled rotation
			// 4th translate back to torso parent offset
			// use previously created matrix if available for the same weight
			if (torsoWeight != thisBoneInfo->torsoWeight)
			{
				Matrix4FromScaledAxisPlusTranslation(torsoAxis, thisBoneInfo->torsoWeight, torsoParentOffset, m2);
				torsoWeight = thisBoneInfo->torsoWeight;
			}
			// multiply matrices to create one matrix to do all calculations
			Matrix4MultiplyInto3x3AndTranslation(m2, m1, bonePtr->matrix, bonePtr->translation);

			/*} else {	// tag's require special handling

			    // rotate each of the axis by the torsoAngles
			    LocalScaledMatrixTransformVector( bonePtr->matrix[0], thisBoneInfo->torsoWeight, torsoAxis, tmpAxis[0] );
			    LocalScaledMatrixTransformVector( bonePtr->matrix[1], thisBoneInfo->torsoWeight, torsoAxis, tmpAxis[1] );
			    LocalScaledMatrixTransformVector( bonePtr->matrix[2], thisBoneInfo->torsoWeight, torsoAxis, tmpAxis[2] );
			    memcpy( bonePtr->matrix, tmpAxis, sizeof(tmpAxis) );

			    // rotate the translation around the torsoParent
			    VectorSubtract( bonePtr->translation, torsoParentOffset, t );
			    LocalScaledMatrixTransformVector( t, thisBoneInfo->torsoWeight, torsoAxis, bonePtr->translation );
			    VectorAdd( bonePtr->translation, torsoParentOffset, bonePtr->translation );

			}*/
		}
	}

	// backup the final bones
	memcpy(oldBones, bones, sizeof(bones[0]) * mdxFrameHeader->numBones);
}

/*
=================
R_CalcMDMLod

=================
*/
static float R_CalcMDMLod(refEntity_t* refent, vec3_t origin, float radius, float modelBias, float modelScale)
{
	float flod, lodScale;
	float projectedRadius;

	// compute projected bounding sphere and use that as a criteria for selecting LOD

	projectedRadius = ProjectRadius(radius, origin);
	if (projectedRadius != 0)
	{

//		ri.Printf (PRINT_ALL, "projected radius: %f\n", projectedRadius);

		lodScale = r_lodscale->value;	// fudge factor since MDS uses a much smoother method of LOD
		flod = projectedRadius * lodScale * modelScale;
	}
	else
	{
		// object intersects near view plane, e.g. view weapon
		flod = 1.0f;
	}

	if (refent->reFlags & REFLAG_FORCE_LOD)
	{
		flod *= 0.5;
	}
//----(SA)	like reflag_force_lod, but separate for the moment
	if (refent->reFlags & REFLAG_DEAD_LOD)
	{
		flod *= 0.8;
	}

	flod -= 0.25 * (r_lodbias->value) + modelBias;

	if (flod < 0.0)
	{
		flod = 0.0;
	}
	else if (flod > 1.0f)
	{
		flod = 1.0f;
	}

	return flod;
}

/*
==============
RB_MDM_SurfaceAnim
==============
*/
void RB_MDM_SurfaceAnim(mdmSurface_t* surface)
{
	int i, j, k;
	refEntity_t* refent;
	int* boneList;
	mdmHeader_t* header;

#ifdef DBG_PROFILE_BONES
	int di = 0, dt, ldt;

	dt = ri.Milliseconds();
	ldt = dt;
#endif

	refent = &backEnd.currentEntity->e;
	boneList = (int*)((byte*)surface + surface->ofsBoneReferences);
	header = (mdmHeader_t*)((byte*)surface + surface->ofsHeader);

	R_CalcBones((const refEntity_t*)refent, boneList, surface->numBoneReferences);

	DBG_SHOWTIME

	//
	// calculate LOD
	//
	// TODO: lerp the radius and origin
	VectorAdd(refent->origin, frame->localOrigin, vec);
	lodRadius = frame->radius;
	lodScale = R_CalcMDMLod(refent, vec, lodRadius, header->lodBias, header->lodScale);

	// ydnar: debug code
	//%	lodScale = 0.15;

//DBG_SHOWTIME

//----(SA)	modification to allow dead skeletal bodies to go below minlod (experiment)
	if (refent->reFlags & REFLAG_DEAD_LOD)
	{
		if (lodScale < 0.35)		// allow dead to lod down to 35% (even if below surf->minLod) (%35 is arbitrary and probably not good generally.  worked for the blackguard/infantry as a test though)
		{
			lodScale = 0.35;
		}
		render_count = (int)((float)surface->numVerts * lodScale);

	}
	else
	{
		render_count = (int)((float)surface->numVerts * lodScale);
		if (render_count < surface->minLod)
		{
			if (!(refent->reFlags & REFLAG_DEAD_LOD))
			{
				render_count = surface->minLod;
			}
		}
	}
//----(SA)	end


	if (render_count > surface->numVerts)
	{
		render_count = surface->numVerts;
	}

	// ydnar: to profile bone transform performance only
	if (r_bonesDebug->integer == 10)
	{
		return;
	}


	RB_CheckOverflow(render_count, surface->numTriangles * 3);

//DBG_SHOWTIME

	//
	// setup triangle list
	//
	// ydnar: no need to do this twice
	//%	RB_CheckOverflow( surface->numVerts, surface->numTriangles * 3);

//DBG_SHOWTIME

	collapse_map   = (int*)((byte*)surface + surface->ofsCollapseMap);
	triangles = (int*)((byte*)surface + surface->ofsTriangles);
	indexes = surface->numTriangles * 3;
	baseIndex = tess.numIndexes;
	baseVertex = tess.numVertexes;
	oldIndexes = baseIndex;

	tess.numVertexes += render_count;

	pIndexes = &tess.indexes[baseIndex];

//DBG_SHOWTIME

	if (render_count == surface->numVerts)
	{
		memcpy(pIndexes, triangles, sizeof(triangles[0]) * indexes);
		if (baseVertex)
		{
			glIndex_t* indexesEnd;
			for (indexesEnd = pIndexes + indexes; pIndexes < indexesEnd; pIndexes++)
			{
				*pIndexes += baseVertex;
			}
		}
		tess.numIndexes += indexes;
	}
	else
	{
		int* collapseEnd;

		pCollapse = collapse;
		for (j = 0; j < render_count; pCollapse++, j++)
		{
			*pCollapse = j;
		}

		pCollapseMap = &collapse_map[render_count];
		for (collapseEnd = collapse + surface->numVerts; pCollapse < collapseEnd; pCollapse++, pCollapseMap++)
		{
			*pCollapse = collapse[*pCollapseMap];
		}

		for (j = 0; j < indexes; j += 3)
		{
			p0 = collapse[*(triangles++)];
			p1 = collapse[*(triangles++)];
			p2 = collapse[*(triangles++)];

			// FIXME
			// note:  serious optimization opportunity here,
			//  by sorting the triangles the following "continue"
			//  could have been made into a "break" statement.
			if (p0 == p1 || p1 == p2 || p2 == p0)
			{
				continue;
			}

			*(pIndexes++) = baseVertex + p0;
			*(pIndexes++) = baseVertex + p1;
			*(pIndexes++) = baseVertex + p2;
			tess.numIndexes += 3;
		}

		baseIndex = tess.numIndexes;
	}

//DBG_SHOWTIME

	//
	// deform the vertexes by the lerped bones
	//
	numVerts = surface->numVerts;
	v = (mdmVertex_t*)((byte*)surface + surface->ofsVerts);
	tempVert = (float*)(tess.xyz + baseVertex);
	tempNormal = (float*)(tess.normal + baseVertex);
	for (j = 0; j < render_count; j++, tempVert += 4, tempNormal += 4)
	{
		mdmWeight_t* w;

		VectorClear(tempVert);

		w = v->weights;
		for (k = 0; k < v->numWeights; k++, w++)
		{
			bone = &bones[w->boneIndex];
			LocalAddScaledMatrixTransformVectorTranslate(w->offset, w->boneWeight, bone->matrix, bone->translation, tempVert);
		}

		LocalMatrixTransformVector(v->normal, bones[v->weights[0].boneIndex].matrix, tempNormal);

		tess.texCoords[baseVertex + j][0][0] = v->texCoords[0];
		tess.texCoords[baseVertex + j][0][1] = v->texCoords[1];

		v = (mdmVertex_t*)&v->weights[v->numWeights];
	}

	DBG_SHOWTIME

	if (r_bonesDebug->integer)
	{
		GL_State(GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE);
		if (r_bonesDebug->integer < 3 || r_bonesDebug->integer == 5 || r_bonesDebug->integer == 8 || r_bonesDebug->integer == 9)
		{
			// DEBUG: show the bones as a stick figure with axis at each bone
			boneRefs = (int*)((byte*)surface + surface->ofsBoneReferences);
			for (i = 0; i < surface->numBoneReferences; i++, boneRefs++)
			{
				bonePtr = &bones[*boneRefs];

				GL_Bind(tr.whiteImage);
				if (r_bonesDebug->integer != 9)
				{
					qglLineWidth(1);
					qglBegin(GL_LINES);
					for (j = 0; j < 3; j++)
					{
						VectorClear(vec);
						vec[j] = 1;
						qglColor3fv(vec);
						qglVertex3fv(bonePtr->translation);
						VectorMA(bonePtr->translation, (r_bonesDebug->integer == 8 ? 1.5 : 5), bonePtr->matrix[j], vec);
						qglVertex3fv(vec);
					}
					qglEnd();
				}

				// connect to our parent if it's valid
				if (validBones[boneInfo[*boneRefs].parent])
				{
					qglLineWidth(r_bonesDebug->integer == 8 ? 4 : 2);
					qglBegin(GL_LINES);
					qglColor3f(.6,.6,.6);
					qglVertex3fv(bonePtr->translation);
					qglVertex3fv(bones[boneInfo[*boneRefs].parent].translation);
					qglEnd();
				}

				qglLineWidth(1);
			}

			if (r_bonesDebug->integer == 8)
			{
				// FIXME: Actually draw the whole skeleton
				//if( surface == (mdmSurface_t *)((byte *)header + header->ofsSurfaces) ) {
				mdxHeader_t* mdxHeader = R_GetModelByHandle(refent->frameModel)->q3_mdx;
				boneRefs = (int*)((byte*)surface + surface->ofsBoneReferences);

				qglDepthRange(0, 0);		// never occluded
				qglBlendFunc(GL_SRC_ALPHA, GL_ONE);

				for (i = 0; i < surface->numBoneReferences; i++, boneRefs++)
				{
					vec3_t diff;
					mdxBoneInfo_t* mdxBoneInfo = (mdxBoneInfo_t*)((byte*)mdxHeader + mdxHeader->ofsBones + *boneRefs * sizeof(mdxBoneInfo_t));
					bonePtr = &bones[*boneRefs];

					VectorSet(vec, 0.f, 0.f, 32.f);
					VectorSubtract(bonePtr->translation, vec, diff);
					vec[0] = vec[0] + diff[0] * 6;
					vec[1] = vec[1] + diff[1] * 6;
					vec[2] = vec[2] + diff[2] * 3;

					qglEnable(GL_BLEND);
					qglBegin(GL_LINES);
					qglColor4f(1.f, .4f, .05f, .35f);
					qglVertex3fv(bonePtr->translation);
					qglVertex3fv(vec);
					qglEnd();
					qglDisable(GL_BLEND);

					R_DebugText(vec, 1.f, 1.f, 1.f, mdxBoneInfo->name, false);			// false, as there is no reason to set depthrange again
				}

				qglDepthRange(0, 1);
				//}
			}
			else if (r_bonesDebug->integer == 9)
			{
				if (surface == (mdmSurface_t*)((byte*)header + header->ofsSurfaces))
				{
					mdmTag_t* pTag = (mdmTag_t*)((byte*)header + header->ofsTags);

					qglDepthRange(0, 0);	// never occluded
					qglBlendFunc(GL_SRC_ALPHA, GL_ONE);

					for (i = 0; i < header->numTags; i++)
					{
						mdxBoneFrame_t* tagBone;
						orientation_t outTag;
						vec3_t diff;

						// now extract the orientation for the bone that represents our tag
						tagBone = &bones[pTag->boneIndex];
						VectorClear(outTag.origin);
						LocalAddScaledMatrixTransformVectorTranslate(pTag->offset, 1.f, tagBone->matrix, tagBone->translation, outTag.origin);
						for (j = 0; j < 3; j++)
						{
							LocalMatrixTransformVector(pTag->axis[j], bone->matrix, outTag.axis[j]);
						}

						GL_Bind(tr.whiteImage);
						qglLineWidth(2);
						qglBegin(GL_LINES);
						for (j = 0; j < 3; j++)
						{
							VectorClear(vec);
							vec[j] = 1;
							qglColor3fv(vec);
							qglVertex3fv(outTag.origin);
							VectorMA(outTag.origin, 5, outTag.axis[j], vec);
							qglVertex3fv(vec);
						}
						qglEnd();

						VectorSet(vec, 0.f, 0.f, 32.f);
						VectorSubtract(outTag.origin, vec, diff);
						vec[0] = vec[0] + diff[0] * 2;
						vec[1] = vec[1] + diff[1] * 2;
						vec[2] = vec[2] + diff[2] * 1.5;

						qglLineWidth(1);
						qglEnable(GL_BLEND);
						qglBegin(GL_LINES);
						qglColor4f(1.f, .4f, .05f, .35f);
						qglVertex3fv(outTag.origin);
						qglVertex3fv(vec);
						qglEnd();
						qglDisable(GL_BLEND);

						R_DebugText(vec, 1.f, 1.f, 1.f, pTag->name, false);		// false, as there is no reason to set depthrange again

						pTag = (mdmTag_t*)((byte*)pTag + pTag->ofsEnd);
					}
					qglDepthRange(0, 1);
				}
			}
		}

		if (r_bonesDebug->integer >= 3 && r_bonesDebug->integer <= 6)
		{
			int render_indexes = (tess.numIndexes - oldIndexes);

			// show mesh edges
			tempVert = (float*)(tess.xyz + baseVertex);
			tempNormal = (float*)(tess.normal + baseVertex);

			GL_Bind(tr.whiteImage);
			qglLineWidth(1);
			qglBegin(GL_LINES);
			qglColor3f(.0,.0,.8);

			pIndexes = &tess.indexes[oldIndexes];
			for (j = 0; j < render_indexes / 3; j++, pIndexes += 3)
			{
				qglVertex3fv(tempVert + 4 * pIndexes[0]);
				qglVertex3fv(tempVert + 4 * pIndexes[1]);

				qglVertex3fv(tempVert + 4 * pIndexes[1]);
				qglVertex3fv(tempVert + 4 * pIndexes[2]);

				qglVertex3fv(tempVert + 4 * pIndexes[2]);
				qglVertex3fv(tempVert + 4 * pIndexes[0]);
			}

			qglEnd();

//----(SA)	track debug stats
			if (r_bonesDebug->integer == 4)
			{
				totalrv += render_count;
				totalrt += render_indexes / 3;
				totalv += surface->numVerts;
				totalt += surface->numTriangles;
			}
//----(SA)	end

			if (r_bonesDebug->integer == 3)
			{
				common->Printf("Lod %.2f  verts %4d/%4d  tris %4d/%4d  (%.2f%%)\n", lodScale, render_count, surface->numVerts, render_indexes / 3, surface->numTriangles,
					(float)(100.0 * render_indexes / 3) / (float)surface->numTriangles);
			}
		}

		if (r_bonesDebug->integer == 6 || r_bonesDebug->integer == 7)
		{
			v = (mdmVertex_t*)((byte*)surface + surface->ofsVerts);
			tempVert = (float*)(tess.xyz + baseVertex);
			GL_Bind(tr.whiteImage);
			qglPointSize(5);
			qglBegin(GL_POINTS);
			for (j = 0; j < render_count; j++, tempVert += 4)
			{
				if (v->numWeights > 1)
				{
					if (v->numWeights == 2)
					{
						qglColor3f(.4f, .4f, 0.f);
					}
					else if (v->numWeights == 3)
					{
						qglColor3f(.8f, .4f, 0.f);
					}
					else
					{
						qglColor3f(1.f, .4f, 0.f);
					}
					qglVertex3fv(tempVert);
				}
				v = (mdmVertex_t*)&v->weights[v->numWeights];
			}
			qglEnd();
		}
	}

	if (r_bonesDebug->integer > 1)
	{
		// dont draw the actual surface
		tess.numIndexes = oldIndexes;
		tess.numVertexes = baseVertex;
		return;
	}

#ifdef DBG_PROFILE_BONES
	common->Printf("\n");
#endif

}

/*
===============
R_GetBoneTag
===============
*/
int R_MDM_GetBoneTag(orientation_t* outTag, mdmHeader_t* mdm, int startTagIndex, const refEntity_t* refent, const char* tagName)
{
	int i, j;
	mdmTag_t* pTag;
	int* boneList;

	if (startTagIndex > mdm->numTags)
	{
		memset(outTag, 0, sizeof(*outTag));
		return -1;
	}

	// find the correct tag

	pTag = (mdmTag_t*)((byte*)mdm + mdm->ofsTags);

	if (startTagIndex)
	{
		for (i = 0; i < startTagIndex; i++)
		{
			pTag = (mdmTag_t*)((byte*)pTag + pTag->ofsEnd);
		}
	}

	for (i = startTagIndex; i < mdm->numTags; i++)
	{
		if (!String::Cmp(pTag->name, tagName))
		{
			break;
		}
		pTag = (mdmTag_t*)((byte*)pTag + pTag->ofsEnd);
	}

	if (i >= mdm->numTags)
	{
		memset(outTag, 0, sizeof(*outTag));
		return -1;
	}

	// calc the bones

	boneList = (int*)((byte*)pTag + pTag->ofsBoneReferences);
	R_CalcBones(refent, boneList, pTag->numBoneReferences);

	// now extract the orientation for the bone that represents our tag
	bone = &bones[pTag->boneIndex];
	VectorClear(outTag->origin);
	LocalAddScaledMatrixTransformVectorTranslate(pTag->offset, 1.f, bone->matrix, bone->translation, outTag->origin);
	for (j = 0; j < 3; j++)
	{
		LocalMatrixTransformVector(pTag->axis[j], bone->matrix, outTag->axis[j]);
	}
	return i;
}
