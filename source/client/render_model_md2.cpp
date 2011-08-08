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

#define POWERSUIT_SCALE			4.0F

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static float	md2_shadelight[3];
static vec4_t	s_lerped[MAX_MD2_VERTS];

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	Mod_LoadMd2Model
//
//==========================================================================

void Mod_LoadMd2Model(model_t* mod, const void* buffer)
{
	const dmd2_t* pinmodel = (const dmd2_t*)buffer;

	int version = LittleLong(pinmodel->version);
	if (version != MESH2_VERSION)
	{
		throw QDropException(va("%s has wrong version number (%i should be %i)",
			mod->name, version, MESH2_VERSION));
	}

	dmd2_t* pheader = (dmd2_t*)Mem_Alloc(LittleLong(pinmodel->ofs_end));
	
	// byte swap the header fields and sanity check
	for (int i = 0; i < (int)sizeof(dmd2_t) / 4; i++)
	{
		((int*)pheader)[i] = LittleLong(((int*)buffer)[i]);
	}

	if (pheader->num_xyz <= 0)
	{
		throw QDropException(va("model %s has no vertices", mod->name));
	}

	if (pheader->num_xyz > MAX_MD2_VERTS)
	{
		throw QDropException(va("model %s has too many vertices", mod->name));
	}

	if (pheader->num_st <= 0)
	{
		throw QDropException(va("model %s has no st vertices", mod->name));
	}

	if (pheader->num_tris <= 0)
	{
		throw QDropException(va("model %s has no triangles", mod->name));
	}

	if (pheader->num_frames <= 0)
	{
		throw QDropException(va("model %s has no frames", mod->name));
	}

	//
	// load base s and t vertices (not used in gl version)
	//
	const dmd2_stvert_t* pinst = (const dmd2_stvert_t*)((byte*)pinmodel + pheader->ofs_st);
	dmd2_stvert_t* poutst = (dmd2_stvert_t*)((byte*)pheader + pheader->ofs_st);

	for (int i = 0; i < pheader->num_st; i++)
	{
		poutst[i].s = LittleShort(pinst[i].s);
		poutst[i].t = LittleShort(pinst[i].t);
	}

	//
	// load triangle lists
	//
	const dmd2_triangle_t* pintri = (const dmd2_triangle_t*)((byte*)pinmodel + pheader->ofs_tris);
	dmd2_triangle_t* pouttri = (dmd2_triangle_t*)((byte*)pheader + pheader->ofs_tris);

	for (int i = 0; i < pheader->num_tris; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			pouttri[i].index_xyz[j] = LittleShort(pintri[i].index_xyz[j]);
			pouttri[i].index_st[j] = LittleShort(pintri[i].index_st[j]);
		}
	}

	//
	// load the frames
	//
	for (int i = 0; i < pheader->num_frames; i++)
	{
		const dmd2_frame_t* pinframe = (const dmd2_frame_t*)((byte*)pinmodel +
			pheader->ofs_frames + i * pheader->framesize);
		dmd2_frame_t* poutframe = (dmd2_frame_t*)((byte*)pheader +
			pheader->ofs_frames + i * pheader->framesize);

		Com_Memcpy(poutframe->name, pinframe->name, sizeof(poutframe->name));
		for (int j = 0; j < 3; j++)
		{
			poutframe->scale[j] = LittleFloat(pinframe->scale[j]);
			poutframe->translate[j] = LittleFloat(pinframe->translate[j]);
		}
		// verts are all 8 bit, so no swapping needed
		Com_Memcpy(poutframe->verts, pinframe->verts, pheader->num_xyz * sizeof(dmd2_trivertx_t));
	}

	mod->type = MOD_MESH2;
	mod->q2_extradata = pheader;
	mod->q2_extradatasize = pheader->ofs_end;
	mod->q2_numframes = pheader->num_frames;

	//
	// load the glcmds
	//
	const int* pincmd = (const int*)((byte*)pinmodel + pheader->ofs_glcmds);
	int* poutcmd = (int*)((byte*)pheader + pheader->ofs_glcmds);
	for (int i = 0; i < pheader->num_glcmds; i++)
	{
		poutcmd[i] = LittleLong(pincmd[i]);
	}

	// register all skins
	Com_Memcpy((char*)pheader + pheader->ofs_skins, (char*)pinmodel + pheader->ofs_skins,
		pheader->num_skins*MAX_MD2_SKINNAME);
	for (int i = 0; i < pheader->num_skins; i++)
	{
		mod->q2_skins[i] = R_FindImageFile((char*)pheader + pheader->ofs_skins + i * MAX_MD2_SKINNAME,
			true, true, GL_CLAMP, false, IMG8MODE_Skin);
	}

	mod->q2_mins[0] = -32;
	mod->q2_mins[1] = -32;
	mod->q2_mins[2] = -32;
	mod->q2_maxs[0] = 32;
	mod->q2_maxs[1] = 32;
	mod->q2_maxs[2] = 32;
}

//==========================================================================
//
//	Mod_FreeMd2Model
//
//==========================================================================

void Mod_FreeMd2Model(model_t* mod)
{
	Mem_Free(mod->q2_extradata);
}

//==========================================================================
//
//	GL_LerpVerts
//
//==========================================================================

static void GL_LerpVerts(int nverts, dmd2_trivertx_t* v, dmd2_trivertx_t* ov, dmd2_trivertx_t* verts,
	float* lerp, float move[3], float frontv[3], float backv[3])
{
	if (tr.currentEntity->e.renderfx & RF_COLOUR_SHELL)
	{
		for (int i = 0; i < nverts; i++, v++, ov++, lerp += 4)
		{
			float* normal = bytedirs[verts[i].lightnormalindex];

			lerp[0] = move[0] + ov->v[0] * backv[0] + v->v[0] * frontv[0] + normal[0] * POWERSUIT_SCALE;
			lerp[1] = move[1] + ov->v[1] * backv[1] + v->v[1] * frontv[1] + normal[1] * POWERSUIT_SCALE;
			lerp[2] = move[2] + ov->v[2] * backv[2] + v->v[2] * frontv[2] + normal[2] * POWERSUIT_SCALE; 
		}
	}
	else
	{
		for (int i = 0; i < nverts; i++, v++, ov++, lerp += 4)
		{
			lerp[0] = move[0] + ov->v[0] * backv[0] + v->v[0] * frontv[0];
			lerp[1] = move[1] + ov->v[1] * backv[1] + v->v[1] * frontv[1];
			lerp[2] = move[2] + ov->v[2] * backv[2] + v->v[2] * frontv[2];
		}
	}
}

//==========================================================================
//
//	GL_DrawMd2FrameLerp
//
//	interpolates between two frames and origins
//	FIXME: batch lerp all vertexes
//
//==========================================================================

static void GL_DrawMd2FrameLerp(dmd2_t* paliashdr, float backlerp)
{
	dmd2_frame_t* frame = (dmd2_frame_t*)((byte*)paliashdr + paliashdr->ofs_frames +
		tr.currentEntity->e.frame * paliashdr->framesize);
	dmd2_trivertx_t* verts = frame->verts;
	dmd2_trivertx_t* v = verts;

	dmd2_frame_t* oldframe = (dmd2_frame_t*)((byte*)paliashdr + paliashdr->ofs_frames +
		tr.currentEntity->e.oldframe * paliashdr->framesize);
	dmd2_trivertx_t* ov = oldframe->verts;

	int* order = (int*)((byte*)paliashdr + paliashdr->ofs_glcmds);

	float alpha;
	if (tr.currentEntity->e.renderfx & RF_TRANSLUCENT)
	{
		alpha = tr.currentEntity->e.shaderRGBA[3] / 255.0;
	}
	else
	{
		alpha = 1.0;
	}

	if (tr.currentEntity->e.renderfx & RF_COLOUR_SHELL)
	{
		qglDisable(GL_TEXTURE_2D);
	}

	float frontlerp = 1.0 - backlerp;

	// move should be the delta back to the previous frame * backlerp
	vec3_t delta;
	VectorSubtract(tr.currentEntity->e.oldorigin, tr.currentEntity->e.origin, delta);

	vec3_t move;
	move[0] = DotProduct(delta, tr.currentEntity->e.axis[0]);	// forward
	move[1] = DotProduct(delta, tr.currentEntity->e.axis[1]);	// left
	move[2] = DotProduct(delta, tr.currentEntity->e.axis[2]);	// up

	VectorAdd(move, oldframe->translate, move);

	for (int i = 0; i < 3; i++)
	{
		move[i] = backlerp * move[i] + frontlerp * frame->translate[i];
	}

	vec3_t frontv, backv;
	for (int i = 0; i < 3; i++)
	{
		frontv[i] = frontlerp * frame->scale[i];
		backv[i] = backlerp * oldframe->scale[i];
	}

	float* lerp = s_lerped[0];

	GL_LerpVerts(paliashdr->num_xyz, v, ov, verts, lerp, move, frontv, backv);

	if (r_vertex_arrays->value)
	{
		float colorArray[MAX_MD2_VERTS * 4];

		qglVertexPointer(3, GL_FLOAT, 16, s_lerped);	// padded for SIMD

		if (tr.currentEntity->e.renderfx & RF_COLOUR_SHELL)
		{
			qglColor4f(md2_shadelight[0], md2_shadelight[1], md2_shadelight[2], alpha);
		}
		else
		{
			qglEnableClientState(GL_COLOR_ARRAY);
			qglColorPointer(3, GL_FLOAT, 0, colorArray);

			//
			// pre light everything
			//
			for (int i = 0; i < paliashdr->num_xyz; i++)
			{
				float l = shadedots[verts[i].lightnormalindex];

				colorArray[i * 3 + 0] = l * md2_shadelight[0];
				colorArray[i * 3 + 1] = l * md2_shadelight[1];
				colorArray[i * 3 + 2] = l * md2_shadelight[2];
			}
		}

		if (qglLockArraysEXT)
		{
			qglLockArraysEXT(0, paliashdr->num_xyz);
		}

		while (1)
		{
			// get the vertex count and primitive type
			int count = *order++;
			if (!count)
			{
				break;		// done
			}
			if (count < 0)
			{
				count = -count;
				qglBegin(GL_TRIANGLE_FAN);
			}
			else
			{
				qglBegin(GL_TRIANGLE_STRIP);
			}

			if (tr.currentEntity->e.renderfx & RF_COLOUR_SHELL)
			{
				do
				{
					int index_xyz = order[2];
					order += 3;

					qglVertex3fv(s_lerped[index_xyz]);
				}
				while (--count);
			}
			else
			{
				do
				{
					// texture coordinates come from the draw list
					qglTexCoord2f(((float*)order)[0], ((float*)order)[1]);
					int index_xyz = order[2];

					order += 3;

					// normals and vertexes come from the frame list
					qglArrayElement(index_xyz);
				}
				while (--count);
			}
			qglEnd();
		}

		if (qglUnlockArraysEXT)
		{
			qglUnlockArraysEXT();
		}
	}
	else
	{
		while (1)
		{
			// get the vertex count and primitive type
			int count = *order++;
			if (!count)
			{
				break;		// done
			}
			if (count < 0)
			{
				count = -count;
				qglBegin(GL_TRIANGLE_FAN);
			}
			else
			{
				qglBegin(GL_TRIANGLE_STRIP);
			}

			if (tr.currentEntity->e.renderfx & RF_COLOUR_SHELL)
			{
				do
				{
					int index_xyz = order[2];
					order += 3;

					qglColor4f(md2_shadelight[0], md2_shadelight[1], md2_shadelight[2], alpha);
					qglVertex3fv(s_lerped[index_xyz]);
				}
				while (--count);
			}
			else
			{
				do
				{
					// texture coordinates come from the draw list
					qglTexCoord2f(((float*)order)[0], ((float*)order)[1]);
					int index_xyz = order[2];
					order += 3;

					// normals and vertexes come from the frame list
					float l = shadedots[verts[index_xyz].lightnormalindex];
					
					qglColor4f(l * md2_shadelight[0], l * md2_shadelight[1], l * md2_shadelight[2], alpha);
					qglVertex3fv(s_lerped[index_xyz]);
				}
				while (--count);
			}
			qglEnd();
		}
	}

	if (tr.currentEntity->e.renderfx & RF_COLOUR_SHELL)
	{
		qglEnable(GL_TEXTURE_2D);
	}
}

//==========================================================================
//
//	GL_DrawMd2Shadow
//
//==========================================================================

static void GL_DrawMd2Shadow(dmd2_t* paliashdr, int posenum)
{
	float lheight = tr.currentEntity->e.origin[2] - lightspot[2];

	int* order = (int*)((byte*)paliashdr + paliashdr->ofs_glcmds);

	float height = -lheight + 1.0;

	while (1)
	{
		// get the vertex count and primitive type
		int count = *order++;
		if (!count)
		{
			break;		// done
		}
		if (count < 0)
		{
			count = -count;
			qglBegin(GL_TRIANGLE_FAN);
		}
		else
		{
			qglBegin(GL_TRIANGLE_STRIP);
		}

		do
		{
			vec3_t point;
			Com_Memcpy(point, s_lerped[order[2]], sizeof(point));

			point[0] -= shadevector[0] * (point[2] + lheight);
			point[1] -= shadevector[1] * (point[2] + lheight);
			point[2] = height;
			qglVertex3fv(point);

			order += 3;
		}
		while (--count);

		qglEnd();
	}	
}

//==========================================================================
//
//	R_CullMd2Model
//
//==========================================================================

static bool R_CullMd2Model(trRefEntity_t* e)
{
	dmd2_t* paliashdr = (dmd2_t*)tr.currentModel->q2_extradata;

	if ((e->e.frame >= paliashdr->num_frames) || (e->e.frame < 0))
	{
		gLog.write("R_CullMd2Model %s: no such frame %d\n", 
			tr.currentModel->name, e->e.frame);
		e->e.frame = 0;
	}
	if ((e->e.oldframe >= paliashdr->num_frames) || (e->e.oldframe < 0))
	{
		gLog.write("R_CullMd2Model %s: no such oldframe %d\n", 
			tr.currentModel->name, e->e.oldframe);
		e->e.oldframe = 0;
	}

	dmd2_frame_t* pframe = (dmd2_frame_t*)((byte*)paliashdr + 
		paliashdr->ofs_frames + e->e.frame * paliashdr->framesize);

	dmd2_frame_t* poldframe = (dmd2_frame_t*)((byte*)paliashdr + 
		paliashdr->ofs_frames + e->e.oldframe * paliashdr->framesize);

	/*
	** compute axially aligned mins and maxs
	*/
	vec3_t bounds[2];
	if (pframe == poldframe)
	{
		for (int i = 0; i < 3; i++)
		{
			bounds[0][i] = pframe->translate[i];
			bounds[1][i] = bounds[0][i] + pframe->scale[i] * 255;
		}
	}
	else
	{
		for (int i = 0; i < 3; i++)
		{
			vec3_t thismins, thismaxs;
			thismins[i] = pframe->translate[i];
			thismaxs[i] = thismins[i] + pframe->scale[i] * 255;

			vec3_t oldmins, oldmaxs;
			oldmins[i]  = poldframe->translate[i];
			oldmaxs[i]  = oldmins[i] + poldframe->scale[i] * 255;

			if (thismins[i] < oldmins[i])
			{
				bounds[0][i] = thismins[i];
			}
			else
			{
				bounds[0][i] = oldmins[i];
			}

			if (thismaxs[i] > oldmaxs[i])
			{
				bounds[1][i] = thismaxs[i];
			}
			else
			{
				bounds[1][i] = oldmaxs[i];
			}
		}
	}

	return R_CullLocalBox(bounds) == CULL_OUT;
}

//==========================================================================
//
//	R_DrawMd2Model
//
//==========================================================================

void R_DrawMd2Model(trRefEntity_t* e)
{
	if ((tr.currentEntity->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal)
	{
		return;
	}

	if (!(e->e.renderfx & RF_FIRST_PERSON))
	{
		if (R_CullMd2Model(e))
		{
			return;
		}
	}

	dmd2_t* paliashdr = (dmd2_t*)tr.currentModel->q2_extradata;

	//
	// get lighting information
	//
	if (tr.currentEntity->e.renderfx & RF_COLOUR_SHELL)
	{
		for (int i = 0; i < 3; i++)
		{
			md2_shadelight[i] = tr.currentEntity->e.shaderRGBA[i] / 255.0;
		}
	}
	else if (tr.currentEntity->e.renderfx & RF_ABSOLUTE_LIGHT)
	{
		for (int i = 0; i < 3; i++)
		{
			md2_shadelight[i] = e->e.radius;
		}
	}
	else
	{
		R_LightPointQ2(tr.currentEntity->e.origin, md2_shadelight);

		// player lighting hack for communication back to server
		// big hack!
		if (tr.currentEntity->e.renderfx & RF_FIRST_PERSON)
		{
			// pick the greatest component, which should be the same
			// as the mono value returned by software
			if (md2_shadelight[0] > md2_shadelight[1])
			{
				if (md2_shadelight[0] > md2_shadelight[2])
				{
					r_lightlevel->value = 150 * md2_shadelight[0];
				}
				else
				{
					r_lightlevel->value = 150 * md2_shadelight[2];
				}
			}
			else
			{
				if (md2_shadelight[1] > md2_shadelight[2])
				{
					r_lightlevel->value = 150 * md2_shadelight[1];
				}
				else
				{
					r_lightlevel->value = 150 * md2_shadelight[2];
				}
			}
		}
	}

	if (tr.currentEntity->e.renderfx & RF_MINLIGHT)
	{
		int i;
		for (i = 0; i < 3; i++)
		{
			if (md2_shadelight[i] > 0.1)
			{
				break;
			}
		}
		if (i == 3)
		{
			md2_shadelight[0] = 0.1;
			md2_shadelight[1] = 0.1;
			md2_shadelight[2] = 0.1;
		}
	}

	if (tr.currentEntity->e.renderfx & RF_GLOW)
	{
		// bonus items will pulse with time
		float scale = 0.1 * sin(tr.refdef.floatTime * 7);
		for (int i = 0; i < 3; i++)
		{
			float min = md2_shadelight[i] * 0.8;
			md2_shadelight[i] += scale;
			if (md2_shadelight[i] < min)
			{
				md2_shadelight[i] = min;
			}
		}
	}

	// =================
	// PGM	ir goggles color override
	if (tr.refdef.rdflags & RDF_IRGOGGLES && tr.currentEntity->e.renderfx & RF_IR_VISIBLE)
	{
		md2_shadelight[0] = 1.0;
		md2_shadelight[1] = 0.0;
		md2_shadelight[2] = 0.0;
	}
	// PGM	
	// =================

	vec3_t tmp_angles;
	VecToAngles(e->e.axis[0], tmp_angles);
	shadedots = r_avertexnormal_dots[((int)(tmp_angles[1] * (SHADEDOT_QUANT / 360.0))) & (SHADEDOT_QUANT - 1)];
	
	VectorCopy(e->e.axis[0], shadevector);
	shadevector[2] = 1;
	VectorNormalize(shadevector);

	//
	// locate the proper data
	//

	c_alias_polys += paliashdr->num_tris;

	//
	// draw all the triangles
	//
	if (tr.currentEntity->e.renderfx & RF_DEPTHHACK) // hack the depth range to prevent view model from poking into walls
	{
		qglDepthRange(0, 0.3);
	}

	if (tr.currentEntity->e.renderfx & RF_LEFTHAND)
	{
		qglMatrixMode(GL_PROJECTION);
		qglPushMatrix();
		qglLoadIdentity();
		qglScalef(-1, 1, 1);
		qglMultMatrixf(tr.viewParms.projectionMatrix);
		qglMatrixMode(GL_MODELVIEW);

		GL_Cull(CT_BACK_SIDED);
	}

	qglPushMatrix();
	qglLoadMatrixf(tr.orient.modelMatrix);

	// select skin
	image_t* skin;
	if (tr.currentEntity->e.customSkin)
	{
		skin = tr.images[tr.currentEntity->e.customSkin];	// custom player skin
	}
	else
	{
		if (tr.currentEntity->e.skinNum >= MAX_MD2_SKINS)
		{
			skin = tr.currentModel->q2_skins[0];
		}
		else
		{
			skin = tr.currentModel->q2_skins[tr.currentEntity->e.skinNum];
			if (!skin)
			{
				skin = tr.currentModel->q2_skins[0];
			}
		}
	}
	if (!skin)
	{
		skin = tr.defaultImage;	// fallback...
	}
	GL_Bind(skin);

	// draw it

	GL_TexEnv(GL_MODULATE);
	if (tr.currentEntity->e.renderfx & RF_TRANSLUCENT)
	{
		GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	}
	else
	{
		GL_State(GLS_DEFAULT);
	}


	if ((tr.currentEntity->e.frame >= paliashdr->num_frames) || (tr.currentEntity->e.frame < 0))
	{
		gLog.write("R_DrawMd2Model %s: no such frame %d\n",
			tr.currentModel->name, tr.currentEntity->e.frame);
		tr.currentEntity->e.frame = 0;
		tr.currentEntity->e.oldframe = 0;
	}

	if ((tr.currentEntity->e.oldframe >= paliashdr->num_frames) || (tr.currentEntity->e.oldframe < 0))
	{
		gLog.write("R_DrawMd2Model %s: no such oldframe %d\n",
			tr.currentModel->name, tr.currentEntity->e.oldframe);
		tr.currentEntity->e.frame = 0;
		tr.currentEntity->e.oldframe = 0;
	}

	if (!r_lerpmodels->value)
	{
		tr.currentEntity->e.backlerp = 0;
	}
	GL_DrawMd2FrameLerp(paliashdr, tr.currentEntity->e.backlerp);

	GL_TexEnv(GL_REPLACE);

	qglPopMatrix();

	if (tr.currentEntity->e.renderfx & RF_LEFTHAND)
	{
		qglMatrixMode(GL_PROJECTION);
		qglPopMatrix();
		qglMatrixMode(GL_MODELVIEW);
		GL_Cull(CT_FRONT_SIDED);
	}

	if (tr.currentEntity->e.renderfx & RF_TRANSLUCENT)
	{
		GL_State(GLS_DEFAULT);
	}

	if (tr.currentEntity->e.renderfx & RF_DEPTHHACK)
	{
		qglDepthRange(0, 1);
	}

	if (r_shadows->value && !(tr.currentEntity->e.renderfx & (RF_TRANSLUCENT | RF_FIRST_PERSON)))
	{
		qglPushMatrix();
		qglLoadMatrixf(tr.orient.modelMatrix);
		qglDisable(GL_TEXTURE_2D);
		GL_State(GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		qglColor4f(0, 0, 0, 0.5);
		GL_DrawMd2Shadow(paliashdr, tr.currentEntity->e.frame);
		qglEnable(GL_TEXTURE_2D);
		GL_State(GLS_DEFAULT);
		qglPopMatrix();
	}
	qglColor4f(1, 1, 1, 1);
}
