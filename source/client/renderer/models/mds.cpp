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
#include "../../../common/common_defs.h"
#include "../../../common/strings.h"
#include "../../../common/endian.h"

#define LL(x) x = LittleLong(x)

/*

All bones should be an identity orientation to display the mesh exactly
as it is specified.

For all other frames, the bones represent the transformation from the
orientation of the bone in the base frame to the orientation in this
frame.

*/

//#define HIGH_PRECISION_BONES	// enable this for 32bit precision bones
//#define DBG_PROFILE_BONES

#define ANGLES_SHORT_TO_FLOAT(pf, sh)     { *(pf++) = SHORT2ANGLE(*(sh++)); *(pf++) = SHORT2ANGLE(*(sh++)); *(pf++) = SHORT2ANGLE(*(sh++)); }

#ifdef DBG_PROFILE_BONES
#define DBG_SHOWTIME    common->Printf("%i: %i, ", di++, (dt = Sys_Milliseconds()) - ldt); ldt = dt;
#else
#define DBG_SHOWTIME    ;
#endif

//-----------------------------------------------------------------------------
// Static Vars, ugly but easiest (and fastest) means of seperating RB_SurfaceAnimMds
// and R_CalcBones

struct mdsBoneFrame_t
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
static mdsVertex_t* v;
static mdsBoneFrame_t bones[MDS_MAX_BONES], rawBones[MDS_MAX_BONES], oldBones[MDS_MAX_BONES];
static char validBones[MDS_MAX_BONES];
static char newBones[MDS_MAX_BONES];
static mdsBoneFrame_t* bonePtr, * bone, * parentBone;
static mdsBoneFrameCompressed_t* cBonePtr, * cTBonePtr, * cOldBonePtr, * cOldTBonePtr, * cBoneList, * cOldBoneList, * cBoneListTorso, * cOldBoneListTorso;
static mdsBoneInfo_t* boneInfo, * thisBoneInfo, * parentBoneInfo;
static mdsFrame_t* frame, * torsoFrame;
static mdsFrame_t* oldFrame, * oldTorsoFrame;
static int frameSize;
static short* sh, * sh2;
static float* pf;
static vec3_t angles, tangles, torsoParentOffset, torsoAxis[3], tmpAxis[3];
static float* tempVert, * tempNormal;
static vec3_t vec, v2, dir;
static float diff, a1, a2;
static int render_count;
static float lodRadius, lodScale;
static int* collapse_map, * pCollapseMap;
static int collapse[MDS_MAX_VERTS], * pCollapse;
static int p0, p1, p2;
static bool isTorso, fullTorso;
static vec4_t m1[4], m2[4];
static vec3_t t;
static refEntity_t lastBoneEntity;

static int totalrv, totalrt, totalv, totalt;	//----(SA)

bool R_LoadMds(model_t* mod, const void* buffer)
{
	mdsHeader_t* pinmodel = (mdsHeader_t*)buffer;

	int version = LittleLong(pinmodel->version);
	if (version != MDS_VERSION)
	{
		common->Printf(S_COLOR_YELLOW "R_LoadMds: %s has wrong version (%i should be %i)\n",
			mod->name, version, MDS_VERSION);
		return false;
	}

	mod->type = MOD_MDS;
	int size = LittleLong(pinmodel->ofsEnd);
	mod->q3_dataSize += size;
	mdsHeader_t* mds = (mdsHeader_t*)Mem_Alloc(size);
	mod->q3_mds = mds;

	memcpy(mds, buffer, LittleLong(pinmodel->ofsEnd));

	LL(mds->ident);
	LL(mds->version);
	LL(mds->numFrames);
	LL(mds->numBones);
	LL(mds->numTags);
	LL(mds->numSurfaces);
	LL(mds->ofsFrames);
	LL(mds->ofsBones);
	LL(mds->ofsTags);
	LL(mds->ofsEnd);
	LL(mds->ofsSurfaces);
	mds->lodBias = LittleFloat(mds->lodBias);
	mds->lodScale = LittleFloat(mds->lodScale);
	LL(mds->torsoParent);

	if (mds->numFrames < 1)
	{
		common->Printf(S_COLOR_YELLOW "R_LoadMds: %s has no frames\n", mod->name);
		return false;
	}

	if (LittleLong(1) != 1)
	{
		// swap all the frames
		//frameSize = (int)( &((mdsFrame_t *)0)->bones[ mds->numBones ] );
		int frameSize = (int)(sizeof(mdsFrame_t) - sizeof(mdsBoneFrameCompressed_t) + mds->numBones * sizeof(mdsBoneFrameCompressed_t));
		for (int i = 0; i < mds->numFrames; i++)
		{
			mdsFrame_t* frame = (mdsFrame_t*)((byte*)mds + mds->ofsFrames + i * frameSize);
			frame->radius = LittleFloat(frame->radius);
			for (int j = 0; j < 3; j++)
			{
				frame->bounds[0][j] = LittleFloat(frame->bounds[0][j]);
				frame->bounds[1][j] = LittleFloat(frame->bounds[1][j]);
				frame->localOrigin[j] = LittleFloat(frame->localOrigin[j]);
				frame->parentOffset[j] = LittleFloat(frame->parentOffset[j]);
			}
			for (int j = 0; j < mds->numBones * (int)(sizeof(mdsBoneFrameCompressed_t) / sizeof(short)); j++)
			{
				((short*)frame->bones)[j] = LittleShort(((short*)frame->bones)[j]);
			}
		}

		// swap all the tags
		mdsTag_t* tag = (mdsTag_t*)((byte*)mds + mds->ofsTags);
		for (int i = 0; i < mds->numTags; i++, tag++)
		{
			LL(tag->boneIndex);
			tag->torsoWeight = LittleFloat(tag->torsoWeight);
		}

		// swap all the bones
		for (int i = 0; i < mds->numBones; i++)
		{
			mdsBoneInfo_t* bi = (mdsBoneInfo_t*)((byte*)mds + mds->ofsBones + i * sizeof(mdsBoneInfo_t));
			LL(bi->parent);
			bi->torsoWeight = LittleFloat(bi->torsoWeight);
			bi->parentDist = LittleFloat(bi->parentDist);
			LL(bi->flags);
		}
	}

	// swap all the surfaces
	mdsSurface_t* surf = (mdsSurface_t*)((byte*)mds + mds->ofsSurfaces);
	for (int i = 0; i < mds->numSurfaces; i++)
	{
		if (LittleLong(1) != 1)
		{
			LL(surf->ident);
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
		surf->ident = SF_MDS;

		if (surf->numVerts > SHADER_MAX_VERTEXES)
		{
			common->Error("R_LoadMds: %s has more than %i verts on a surface (%i)",
				mod->name, SHADER_MAX_VERTEXES, surf->numVerts);
		}
		if (surf->numTriangles * 3 > SHADER_MAX_INDEXES)
		{
			common->Error("R_LoadMds: %s has more than %i triangles on a surface (%i)",
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
			mdsTriangle_t* tri = (mdsTriangle_t*)((byte*)surf + surf->ofsTriangles);
			for (int j = 0; j < surf->numTriangles; j++, tri++)
			{
				LL(tri->indexes[0]);
				LL(tri->indexes[1]);
				LL(tri->indexes[2]);
			}

			// swap all the vertexes
			mdsVertex_t* v = (mdsVertex_t*)((byte*)surf + surf->ofsVerts);
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

				v = (mdsVertex_t*)&v->weights[v->numWeights];
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
		surf = (mdsSurface_t*)((byte*)surf + surf->ofsEnd);
	}

	return true;
}

void R_FreeMds(model_t* mod)
{
	Mem_Free(mod->q3_mds);
}

static int R_CullModel(mdsHeader_t* header, trRefEntity_t* ent)
{
	int frameSize = (int)(sizeof(mdsFrame_t) - sizeof(mdsBoneFrameCompressed_t) + header->numBones * sizeof(mdsBoneFrameCompressed_t));

	// compute frame pointers
	mdsFrame_t* newFrame = (mdsFrame_t*)((byte*)header + header->ofsFrames + ent->e.frame * frameSize);
	mdsFrame_t* oldFrame = (mdsFrame_t*)((byte*)header + header->ofsFrames + ent->e.oldframe * frameSize);

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

static int R_ComputeFogNum(mdsHeader_t* header, trRefEntity_t* ent)
{
	if (tr.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return 0;
	}

	// FIXME: non-normalized axis issues
	mdsFrame_t* mdsFrame = (mdsFrame_t*)((byte*)header + header->ofsFrames + (sizeof(mdsFrame_t) + sizeof(mdsBoneFrameCompressed_t) * (header->numBones - 1)) * ent->e.frame);
	vec3_t localOrigin;
	VectorAdd(ent->e.origin, mdsFrame->localOrigin, localOrigin);
	for (int i = 1; i < tr.world->numfogs; i++)
	{
		mbrush46_fog_t* fog = &tr.world->fogs[i];
		int j;
		for (j = 0; j < 3; j++)
		{
			if (localOrigin[j] - mdsFrame->radius >= fog->bounds[1][j])
			{
				break;
			}
			if (localOrigin[j] + mdsFrame->radius <= fog->bounds[0][j])
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

void R_AddMdsAnimSurfaces(trRefEntity_t* ent)
{
	mdsHeader_t* header;
	mdsSurface_t* surface;
	shader_t* shader = 0;
	int i, fogNum, cull;
	qboolean personalModel;

	// don't add third_person objects if not in a portal
	personalModel = (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal;

	header = tr.currentModel->q3_mds;

	//
	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum.
	//
	cull = R_CullModel(header, ent);
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
	fogNum = R_ComputeFogNum(header, ent);

	surface = (mdsSurface_t*)((byte*)header + header->ofsSurfaces);
	for (i = 0; i < header->numSurfaces; i++)
	{
		//----(SA)	blink will change to be an overlay rather than replacing the head texture.
		//		think of it like batman's mask.  the polygons that have eye texture are duplicated
		//		and the 'lids' rendered with polygonoffset over the top of the open eyes.  this gives
		//		minimal overdraw/alpha blending/texture use without breaking the model and causing seams
		if (GGameType & GAME_WolfSP && !String::ICmp(surface->name, "h_blink"))
		{
			if (!(ent->e.renderfx & RF_BLINK))
			{
				surface = (mdsSurface_t*)((byte*)surface + surface->ofsEnd);
				continue;
			}
		}

		if (ent->e.customShader)
		{
			shader = R_GetShaderByHandle(ent->e.customShader);
		}
		else if (ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins)
		{
			skin_t* skin;
			int j;

			skin = R_GetSkinByHandle(ent->e.customSkin);

			// match the surface name to something in the skin file
			shader = tr.defaultShader;

			if (GGameType & (GAME_WolfMP | GAME_ET) && ent->e.renderfx & RF_BLINK)
			{
				const char* s = va("%s_b", surface->name);	// append '_b' for 'blink'
				int hash = Com_HashKey(s, String::Length(s));
				for (j = 0; j < skin->numSurfaces; j++)
				{
					if (GGameType & GAME_ET && hash != skin->surfaces[j]->hash)
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

			if (shader == tr.defaultShader)			// blink reference in skin was not found
			{
				int hash = Com_HashKey(surface->name, sizeof(surface->name));
				for (j = 0; j < skin->numSurfaces; j++)
				{
					// the names have both been lowercased
					if (GGameType & GAME_ET && hash != skin->surfaces[j]->hash)
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
			// GR - always tessellate these objects
			R_AddDrawSurf((surfaceType_t*)surface, shader, fogNum, false, 0, ATI_TESS_TRUFORM);
		}

		surface = (mdsSurface_t*)((byte*)surface + surface->ofsEnd);
	}
}

static void R_CalcBone(mdsHeader_t* header, const refEntity_t* refent, int boneNum)
{
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

#ifdef HIGH_PRECISION_BONES
	// rotation
	if (fullTorso)
	{
		VectorCopy(cTBonePtr->angles, angles);
	}
	else
	{
		VectorCopy(cBonePtr->angles, angles);
		if (isTorso)
		{
			VectorCopy(cTBonePtr->angles, tangles);
			// blend the angles together
			for (int j = 0; j < 3; j++)
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
#else
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
			for (int j = 0; j < 3; j++)
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
#endif
	AnglesToAxis(angles, bonePtr->matrix);

	// translation
	if (parentBone)
	{
#ifdef HIGH_PRECISION_BONES
		if (fullTorso)
		{
			angles[0] = cTBonePtr->ofsAngles[0];
			angles[1] = cTBonePtr->ofsAngles[1];
			angles[2] = 0;
			LocalAngleVector(angles, vec);
			LocalVectorMA(parentBone->translation, thisBoneInfo->parentDist, vec, bonePtr->translation);
		}
		else
		{
			angles[0] = cBonePtr->ofsAngles[0];
			angles[1] = cBonePtr->ofsAngles[1];
			angles[2] = 0;
			LocalAngleVector(angles, vec);

			if (isTorso)
			{
				tangles[0] = cTBonePtr->ofsAngles[0];
				tangles[1] = cTBonePtr->ofsAngles[1];
				tangles[2] = 0;
				LocalAngleVector(tangles, v2);

				// blend the angles together
				SLerp_Normal(vec, v2, thisBoneInfo->torsoWeight, vec);
				LocalVectorMA(parentBone->translation, thisBoneInfo->parentDist, vec, bonePtr->translation);

			}
			else
			{	// legs bone
				LocalVectorMA(parentBone->translation, thisBoneInfo->parentDist, vec, bonePtr->translation);
			}
		}
#else
		if (fullTorso)
		{
			sh = (short*)cTBonePtr->ofsAngles; pf = angles;
			*(pf++) = SHORT2ANGLE(*(sh++)); *(pf++) = SHORT2ANGLE(*(sh++)); *(pf++) = 0;
			LocalAngleVector(angles, vec);
			LocalVectorMA(parentBone->translation, thisBoneInfo->parentDist, vec, bonePtr->translation);
		}
		else
		{
			sh = (short*)cBonePtr->ofsAngles; pf = angles;
			*(pf++) = SHORT2ANGLE(*(sh++)); *(pf++) = SHORT2ANGLE(*(sh++)); *(pf++) = 0;
			LocalAngleVector(angles, vec);

			if (isTorso)
			{
				sh = (short*)cTBonePtr->ofsAngles;
				pf = tangles;
				*(pf++) = SHORT2ANGLE(*(sh++)); *(pf++) = SHORT2ANGLE(*(sh++)); *(pf++) = 0;
				LocalAngleVector(tangles, v2);

				// blend the angles together
				SLerp_Normal(vec, v2, thisBoneInfo->torsoWeight, vec);
				LocalVectorMA(parentBone->translation, thisBoneInfo->parentDist, vec, bonePtr->translation);
			}
			else
			{	// legs bone
				LocalVectorMA(parentBone->translation, thisBoneInfo->parentDist, vec, bonePtr->translation);
			}
		}
#endif
	}
	else
	{	// just use the frame position
		bonePtr->translation[0] = frame->parentOffset[0];
		bonePtr->translation[1] = frame->parentOffset[1];
		bonePtr->translation[2] = frame->parentOffset[2];
	}
	//
	if (boneNum == header->torsoParent)
	{
		// this is the torsoParent
		VectorCopy(bonePtr->translation, torsoParentOffset);
	}
	//
	validBones[boneNum] = 1;
	//
	rawBones[boneNum] = *bonePtr;
	newBones[boneNum] = 1;
}

static void R_CalcBoneLerp(mdsHeader_t* header, const refEntity_t* refent, int boneNum)
{
	if (!refent || !header || boneNum < 0 || boneNum >= MDS_MAX_BONES)
	{
		return;
	}

	thisBoneInfo = &boneInfo[boneNum];

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
			for (int j = 0; j < 3; j++)
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

		pf = angles;
		*(pf++) = SHORT2ANGLE(*(sh++));
		*(pf++) = SHORT2ANGLE(*(sh++));
		*(pf++) = 0;
		LocalAngleVector(angles, v2);		// new

		pf = angles;
		*(pf++) = SHORT2ANGLE(*(sh2++));
		*(pf++) = SHORT2ANGLE(*(sh2++));
		*(pf++) = 0;
		LocalAngleVector(angles, vec);		// old

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
		if (!fullTorso && isTorso)
		{
			// partial legs/torso, need to lerp according to torsoWeight
			// calc the torso frame
			sh = (short*)cTBonePtr->ofsAngles;
			sh2 = (short*)cOldTBonePtr->ofsAngles;

			pf = angles;
			*(pf++) = SHORT2ANGLE(*(sh++));
			*(pf++) = SHORT2ANGLE(*(sh++));
			*(pf++) = 0;
			LocalAngleVector(angles, v2);		// new

			pf = angles;
			*(pf++) = SHORT2ANGLE(*(sh2++));
			*(pf++) = SHORT2ANGLE(*(sh2++));
			*(pf++) = 0;
			LocalAngleVector(angles, vec);		// old

			// blend the angles together
			SLerp_Normal(vec, v2, torsoFrontlerp, v2);

			// blend the torso/legs together
			SLerp_Normal(dir, v2, thisBoneInfo->torsoWeight, dir);
		}

		LocalVectorMA(parentBone->translation, thisBoneInfo->parentDist, dir, bonePtr->translation);
	}
	else
	{	// just interpolate the frame positions
		bonePtr->translation[0] = frontlerp * frame->parentOffset[0] + backlerp * oldFrame->parentOffset[0];
		bonePtr->translation[1] = frontlerp * frame->parentOffset[1] + backlerp * oldFrame->parentOffset[1];
		bonePtr->translation[2] = frontlerp * frame->parentOffset[2] + backlerp * oldFrame->parentOffset[2];
	}
	//
	if (boneNum == header->torsoParent)
	{
		// this is the torsoParent
		VectorCopy(bonePtr->translation, torsoParentOffset);
	}
	validBones[boneNum] = 1;
	//
	rawBones[boneNum] = *bonePtr;
	newBones[boneNum] = 1;

}

//	The list of bones[] should only be built and modified from within here
static void R_CalcBones(mdsHeader_t* header, const refEntity_t* refent, int* boneList, int numBones)
{
	//
	// if the entity has changed since the last time the bones were built, reset them
	//
	if (memcmp(&lastBoneEntity, refent, sizeof(refEntity_t)))
	{
		// different, cached bones are not valid
		Com_Memset(validBones, 0, header->numBones);
		lastBoneEntity = *refent;

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
		totalrv = totalrt = totalv = totalt = 0;
	}

	Com_Memset(newBones, 0, header->numBones);

	if (refent->oldframe == refent->frame)
	{
		backlerp = 0;
		frontlerp = 1;
	}
	else
	{
		backlerp = refent->backlerp;
		frontlerp = 1.0f - backlerp;
	}

	if (refent->oldTorsoFrame == refent->torsoFrame)
	{
		torsoBacklerp = 0;
		torsoFrontlerp = 1;
	}
	else
	{
		torsoBacklerp = refent->torsoBacklerp;
		torsoFrontlerp = 1.0f - torsoBacklerp;
	}

	frameSize = (int)(sizeof(mdsFrame_t) + (header->numBones - 1) * sizeof(mdsBoneFrameCompressed_t));

	frame = (mdsFrame_t*)((byte*)header + header->ofsFrames + refent->frame * frameSize);
	torsoFrame = (mdsFrame_t*)((byte*)header + header->ofsFrames + refent->torsoFrame * frameSize);
	oldFrame = (mdsFrame_t*)((byte*)header + header->ofsFrames + refent->oldframe * frameSize);
	oldTorsoFrame = (mdsFrame_t*)((byte*)header + header->ofsFrames + refent->oldTorsoFrame * frameSize);

	//
	// lerp all the needed bones (torsoParent is always the first bone in the list)
	//
	cBoneList = frame->bones;
	cBoneListTorso = torsoFrame->bones;

	boneInfo = (mdsBoneInfo_t*)((byte*)header + header->ofsBones);
	boneRefs = boneList;
	//
	Matrix3Transpose(refent->torsoAxis, torsoAxis);

#ifdef HIGH_PRECISION_BONES
	if (true)
#else
	if (!backlerp && !torsoBacklerp)
#endif
	{
		for (int i = 0; i < numBones; i++, boneRefs++)
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
				R_CalcBone(header, refent, boneInfo[*boneRefs].parent);
			}

			R_CalcBone(header, refent, *boneRefs);
		}
	}
	else
	{
		// interpolated
		cOldBoneList = oldFrame->bones;
		cOldBoneListTorso = oldTorsoFrame->bones;

		for (int i = 0; i < numBones; i++, boneRefs++)
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
				R_CalcBoneLerp(header, refent, boneInfo[*boneRefs].parent);
			}

			R_CalcBoneLerp(header, refent, *boneRefs);
		}
	}

	// adjust for torso rotations
	float torsoWeight = 0;
	int* boneRefs = boneList;
	for (int i = 0; i < numBones; i++, boneRefs++)
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

			if (!(thisBoneInfo->flags & MDS_BONEFLAG_TAG))
			{
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
			}
			else
			{	// tag's require special handling
				// rotate each of the axis by the torsoAngles
				LocalScaledMatrixTransformVector(bonePtr->matrix[0], thisBoneInfo->torsoWeight, torsoAxis, tmpAxis[0]);
				LocalScaledMatrixTransformVector(bonePtr->matrix[1], thisBoneInfo->torsoWeight, torsoAxis, tmpAxis[1]);
				LocalScaledMatrixTransformVector(bonePtr->matrix[2], thisBoneInfo->torsoWeight, torsoAxis, tmpAxis[2]);
				Com_Memcpy(bonePtr->matrix, tmpAxis, sizeof(tmpAxis));

				// rotate the translation around the torsoParent
				VectorSubtract(bonePtr->translation, torsoParentOffset, t);
				LocalScaledMatrixTransformVector(t, thisBoneInfo->torsoWeight, torsoAxis, bonePtr->translation);
				VectorAdd(bonePtr->translation, torsoParentOffset, bonePtr->translation);
			}
		}
	}

	// backup the final bones
	Com_Memcpy(oldBones, bones, sizeof(bones[0]) * header->numBones);
}

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
	p[1] = Q_fabs(r);
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

static float R_CalcMDSLod(refEntity_t* refent, vec3_t origin, float radius, float modelBias, float modelScale)
{
	if (GGameType & GAME_WolfSP && refent->reFlags & REFLAG_FULL_LOD)
	{
		return 1.0f;
	}

	// compute projected bounding sphere and use that as a criteria for selecting LOD
	float projectedRadius = ProjectRadius(radius, origin);
	float flod;
	if (projectedRadius != 0)
	{
		float lodScale = r_lodscale->value;		// fudge factor since MDS uses a much smoother method of LOD
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

void RB_SurfaceAnimMds(mdsSurface_t* surface)
{
#ifdef DBG_PROFILE_BONES
	int di = 0;
	int dt = Sys_Milliseconds();
	int ldt = dt;
#endif

	refEntity_t* refent = &backEnd.currentEntity->e;
	int* boneList = (int*)((byte*)surface + surface->ofsBoneReferences);
	mdsHeader_t* header = (mdsHeader_t*)((byte*)surface + surface->ofsHeader);

	R_CalcBones(header, (const refEntity_t*)refent, boneList, surface->numBoneReferences);

	DBG_SHOWTIME

	//
	// calculate LOD
	//
	// TODO: lerp the radius and origin
	VectorAdd(refent->origin, frame->localOrigin, vec);
	lodRadius = frame->radius;
	lodScale = R_CalcMDSLod(refent, vec, lodRadius, header->lodBias, header->lodScale);


//DBG_SHOWTIME

//----(SA)	modification to allow dead skeletal bodies to go below minlod (experiment)
	if (refent->reFlags & REFLAG_DEAD_LOD)
	{
		if (lodScale < 0.35)
		{
			// allow dead to lod down to 35% (even if below surf->minLod) (%35 is arbitrary and probably not good generally.  worked for the blackguard/infantry as a test though)
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

	RB_CheckOverflow(render_count, surface->numTriangles);

//DBG_SHOWTIME

	//
	// setup triangle list
	//
	RB_CheckOverflow(surface->numVerts, surface->numTriangles * 3);

//DBG_SHOWTIME

	collapse_map = (int*)((byte*)surface + surface->ofsCollapseMap);
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
		Com_Memcpy(pIndexes, triangles, sizeof(triangles[0]) * indexes);
		if (baseVertex)
		{
			for (glIndex_t* indexesEnd = pIndexes + indexes; pIndexes < indexesEnd; pIndexes++)
			{
				*pIndexes += baseVertex;
			}
		}
		tess.numIndexes += indexes;
	}
	else
	{
		pCollapse = collapse;
		for (int j = 0; j < render_count; pCollapse++, j++)
		{
			*pCollapse = j;
		}

		pCollapseMap = &collapse_map[render_count];
		for (int* collapseEnd = collapse + surface->numVerts; pCollapse < collapseEnd; pCollapse++, pCollapseMap++)
		{
			*pCollapse = collapse[*pCollapseMap];
		}

		for (int j = 0; j < indexes; j += 3)
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
	v = (mdsVertex_t*)((byte*)surface + surface->ofsVerts);
	tempVert = (float*)(tess.xyz + baseVertex);
	tempNormal = (float*)(tess.normal + baseVertex);
	for (int j = 0; j < render_count; j++, tempVert += 4, tempNormal += 4)
	{
		VectorClear(tempVert);

		mdsWeight_t* w = v->weights;
		for (int k = 0; k < v->numWeights; k++, w++)
		{
			bone = &bones[w->boneIndex];
			LocalAddScaledMatrixTransformVectorTranslate(w->offset, w->boneWeight, bone->matrix, bone->translation, tempVert);
		}

		LocalMatrixTransformVector(v->normal, bones[v->weights[0].boneIndex].matrix, tempNormal);

		tess.texCoords[baseVertex + j][0][0] = v->texCoords[0];
		tess.texCoords[baseVertex + j][0][1] = v->texCoords[1];

		v = (mdsVertex_t*)&v->weights[v->numWeights];
	}

	DBG_SHOWTIME

	if (r_bonesDebug->integer)
	{
		if (r_bonesDebug->integer < 3)
		{
			// DEBUG: show the bones as a stick figure with axis at each bone
			boneRefs = (int*)((byte*)surface + surface->ofsBoneReferences);
			for (int i = 0; i < surface->numBoneReferences; i++, boneRefs++)
			{
				bonePtr = &bones[*boneRefs];

				GL_Bind(tr.whiteImage);
				qglLineWidth(1);
				qglBegin(GL_LINES);
				for (int j = 0; j < 3; j++)
				{
					VectorClear(vec);
					vec[j] = 1;
					qglColor3fv(vec);
					qglVertex3fv(bonePtr->translation);
					VectorMA(bonePtr->translation, 5, bonePtr->matrix[j], vec);
					qglVertex3fv(vec);
				}
				qglEnd();

				// connect to our parent if it's valid
				if (validBones[boneInfo[*boneRefs].parent])
				{
					qglLineWidth(2);
					qglBegin(GL_LINES);
					qglColor3f(.6,.6,.6);
					qglVertex3fv(bonePtr->translation);
					qglVertex3fv(bones[boneInfo[*boneRefs].parent].translation);
					qglEnd();
				}

				qglLineWidth(1);
			}
		}

		if (r_bonesDebug->integer == 3 || r_bonesDebug->integer == 4)
		{
			int render_indexes = (tess.numIndexes - oldIndexes);

			// show mesh edges
			tempVert = (float*)(tess.xyz + baseVertex);
			tempNormal = (float*)(tess.normal + baseVertex);

			GL_Bind(tr.whiteImage);
			qglLineWidth(1);
			qglBegin(GL_LINES);
			qglColor3f(.0, .0, .8);

			pIndexes = &tess.indexes[oldIndexes];
			for (int j = 0; j < render_indexes / 3; j++, pIndexes += 3)
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
				common->Printf("Lod %.2f  verts %4d/%4d  tris %4d/%4d  (%.2f%%)\n",
					lodScale, render_count, surface->numVerts, render_indexes / 3, surface->numTriangles,
					(float)(100.0 * render_indexes / 3) / (float)surface->numTriangles);
			}
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

static void R_RecursiveBoneListAdd(int bi, int* boneList, int* numBones, mdsBoneInfo_t* boneInfoList)
{
	if (boneInfoList[bi].parent >= 0)
	{
		R_RecursiveBoneListAdd(boneInfoList[bi].parent, boneList, numBones, boneInfoList);
	}

	boneList[(*numBones)++] = bi;
}

int R_GetBoneTagMds(orientation_t* outTag, mdsHeader_t* mds, int startTagIndex, const refEntity_t* refent, const char* tagName)
{
	if (startTagIndex > mds->numTags)
	{
		Com_Memset(outTag, 0, sizeof(*outTag));
		return -1;
	}

	// find the correct tag

	mdsTag_t* pTag = (mdsTag_t*)((byte*)mds + mds->ofsTags);

	pTag += startTagIndex;

	int i;
	for (i = startTagIndex; i < mds->numTags; i++, pTag++)
	{
		if (!String::Cmp(pTag->name, tagName))
		{
			break;
		}
	}

	if (i >= mds->numTags)
	{
		Com_Memset(outTag, 0, sizeof(*outTag));
		return -1;
	}

	// now build the list of bones we need to calc to get this tag's bone information

	mdsBoneInfo_t* boneInfoList = (mdsBoneInfo_t*)((byte*)mds + mds->ofsBones);
	int numBones = 0;

	int boneList[MDS_MAX_BONES];
	R_RecursiveBoneListAdd(pTag->boneIndex, boneList, &numBones, boneInfoList);

	// calc the bones

	R_CalcBones((mdsHeader_t*)mds, refent, boneList, numBones);

	// now extract the orientation for the bone that represents our tag

	Com_Memcpy(outTag->axis, bones[pTag->boneIndex].matrix, sizeof(outTag->axis));
	VectorCopy(bones[pTag->boneIndex].translation, outTag->origin);

	return i;
}
