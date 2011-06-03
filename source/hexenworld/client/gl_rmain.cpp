// r_main.c

#include "quakedef.h"
#include "glquake.h"

entity_t	r_worldentity;

qboolean	r_cache_thrash;		// compatability

vec3_t		modelorg, r_entorigin;
trRefEntity_t	*currententity;

int			r_visframecount;	// bumped when going to a new PVS

cplane_t	frustum[4];

int			c_brush_polys, c_alias_polys;

qboolean	envmap;				// true during envmap command capture 

image_t*	particletexture;	// little dot for particles
image_t*	playertextures[MAX_CLIENTS];		// up to 16 color translated skins
image_t*	gl_extra_textures[MAX_EXTRA_TEXTURES];   // generic textures for models

int			mirrortexturenum;	// quake texturenum, not gltexturenum
qboolean	mirror;
cplane_t	*mirror_plane;

float		model_constant_alpha;

float		r_time1;
float		r_lasttime1 = 0;

// refresh list
int				cl_numvisedicts;
trRefEntity_t		cl_visedicts[MAX_VISEDICTS];

extern qhandle_t	player_models[MAX_PLAYER_CLASS];

float	r_world_matrix[16];
float	r_base_world_matrix[16];

//
// screen size info
//
refdef_t	r_refdef;

mbrush29_leaf_t		*r_viewleaf, *r_oldviewleaf;

int		d_lightstylevalue[256];	// 8.8 fraction of base light value


void R_MarkLeaves (void);

QCvar*	r_norefresh;
QCvar*	r_drawentities;
QCvar*	r_drawviewmodel;
QCvar*	r_speeds;
QCvar*	r_shadows;
QCvar*	r_mirroralpha;
QCvar*	r_dynamic;
QCvar*	r_novis;
QCvar*	r_netgraph;
QCvar*	r_entdistance;
QCvar*	gl_clear;
QCvar*	gl_cull;
QCvar*	gl_texsort;
QCvar*	gl_smoothmodels;
QCvar*	gl_affinemodels;
QCvar*	gl_polyblend;
QCvar*	gl_flashblend;
QCvar*	gl_nocolors;
QCvar*	gl_keeptjunctions;
QCvar*	gl_reporttjunctions;
QCvar*	r_teamcolor;

extern	QCvar*	scr_fov;

/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
qboolean R_CullBox (vec3_t mins, vec3_t maxs)
{
	int		i;

	for (i=0 ; i<4 ; i++)
		if (BoxOnPlaneSide (mins, maxs, &frustum[i]) == 2)
			return true;
	return false;
}


//==========================================================================
//
// R_RotateForEntity
//
// Same as R_RotateForEntity(), but checks for EF_ROTATE and modifies
// yaw appropriately.
//
//==========================================================================

void R_RotateForEntity(trRefEntity_t *e)
{
	GLfloat glmat[16];

	if (Mod_GetModel(e->e.hModel)->q1_flags & EF_FACE_VIEW)
	{
		float fvaxis[3][3];

		VectorSubtract(tr.refdef.vieworg, e->e.origin, fvaxis[0]);
		VectorNormalize(fvaxis[0]);

		if (fvaxis[0][1] == 0 && fvaxis[0][0] == 0)
		{
			fvaxis[1][0] = 0;
			fvaxis[1][1] = 1;
			fvaxis[1][2] = 0;
			fvaxis[2][1] = 0;
			fvaxis[2][2] = 0;
			if (fvaxis[0][2] > 0)
			{
				fvaxis[2][0] = -1;
			}
			else
			{
				fvaxis[2][0] = 1;
			}
		}
		else
		{
			fvaxis[2][0] = 0;
			fvaxis[2][1] = 0;
			fvaxis[2][2] = 1;
			CrossProduct(fvaxis[2], fvaxis[0], fvaxis[1]);
			VectorNormalize(fvaxis[1]);
			CrossProduct(fvaxis[0], fvaxis[1], fvaxis[2]);
			VectorNormalize(fvaxis[2]);
		}

		float CombAxis[3][3];
		MatrixMultiply(e->e.axis, fvaxis, CombAxis);

		glmat[0] = CombAxis[0][0];
		glmat[1] = CombAxis[0][1];
		glmat[2] = CombAxis[0][2];
		glmat[4] = CombAxis[1][0];
		glmat[5] = CombAxis[1][1];
		glmat[6] = CombAxis[1][2];
		glmat[8] = CombAxis[2][0];
		glmat[9] = CombAxis[2][1];
		glmat[10] = CombAxis[2][2];
	}
	else
	{
		glmat[0] = e->e.axis[0][0];
		glmat[1] = e->e.axis[0][1];
		glmat[2] = e->e.axis[0][2];
		glmat[4] = e->e.axis[1][0];
		glmat[5] = e->e.axis[1][1];
		glmat[6] = e->e.axis[1][2];
		glmat[8] = e->e.axis[2][0];
		glmat[9] = e->e.axis[2][1];
		glmat[10] = e->e.axis[2][2];
	}

	glmat[3] = 0;
	glmat[7] = 0;
	glmat[11] = 0;
	glmat[12] = e->e.origin[0];
	glmat[13] = e->e.origin[1];
	glmat[14] = e->e.origin[2];
	glmat[15] = 1;

	qglMultMatrixf(glmat);
}

/*
=============================================================

  SPRITE MODELS

=============================================================
*/

/*
================
R_GetSpriteFrame
================
*/
msprite1frame_t *R_GetSpriteFrame (trRefEntity_t *currententity)
{
	msprite1_t		*psprite;
	msprite1group_t	*pspritegroup;
	msprite1frame_t	*pspriteframe;
	int				i, numframes, frame;
	float			*pintervals, fullinterval, targettime, time;

	psprite = (msprite1_t*)Mod_GetModel(currententity->e.hModel)->q1_cache;
	frame = currententity->e.frame;

	if ((frame >= psprite->numframes) || (frame < 0))
	{
		Con_Printf ("R_DrawSprite: no such frame %d\n", frame);
		frame = 0;
	}

	if (psprite->frames[frame].type == SPR_SINGLE)
	{
		pspriteframe = psprite->frames[frame].frameptr;
	}
	else
	{
		pspritegroup = (msprite1group_t *)psprite->frames[frame].frameptr;
		pintervals = pspritegroup->intervals;
		numframes = pspritegroup->numframes;
		fullinterval = pintervals[numframes-1];

		time = cl.time + currententity->e.shaderTime;

	// when loading in Mod_LoadSpriteGroup, we guaranteed all interval values
	// are positive, so we don't have to worry about division by 0
		targettime = time - ((int)(time / fullinterval)) * fullinterval;

		for (i=0 ; i<(numframes-1) ; i++)
		{
			if (pintervals[i] > targettime)
				break;
		}

		pspriteframe = pspritegroup->frames[i];
	}

	return pspriteframe;
}


/*
=================
R_DrawSpriteModel

=================
*/
void R_DrawSpriteModel (trRefEntity_t *e)
{
	vec3_t	point;
	msprite1frame_t	*frame;
	float		*up, *right;
	vec3_t		v_right;
	msprite1_t		*psprite;

	// don't even bother culling, because it's just a single
	// polygon without a surface cache

	if (currententity->e.renderfx & RF_WATERTRANS)
	{
		qglColor4f (1,1,1,r_wateralpha->value);
	}
	else if (Mod_GetModel(currententity->e.hModel)->q1_flags & EF_TRANSPARENT)
	{
		qglColor3f(1,1,1);
	}
	else
	{
//		qglColor3f(1,1,1);
		qglColor3f(1,1,1);
	}

	GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);

	frame = R_GetSpriteFrame (e);
	psprite = (msprite1_t*)Mod_GetModel(currententity->e.hModel)->q1_cache;

	if (psprite->type == SPR_ORIENTED)
	{
		// bullet marks on walls
		up = e->e.axis[2];
		VectorSubtract(vec3_origin, currententity->e.axis[1], v_right);
		right = v_right;
	}
	else
	{	// normal sprite
		up = tr.refdef.viewaxis[2];
		VectorSubtract(vec3_origin, tr.refdef.viewaxis[1], v_right);
		right = v_right;
	}

	GL_DisableMultitexture();

    GL_Bind(frame->gl_texture);

	qglBegin (GL_QUADS);

	qglTexCoord2f (0, 1);
	VectorMA (e->e.origin, frame->down, up, point);
	VectorMA (point, frame->left, right, point);
	qglVertex3fv (point);

	qglTexCoord2f (0, 0);
	VectorMA (e->e.origin, frame->up, up, point);
	VectorMA (point, frame->left, right, point);
	qglVertex3fv (point);

	qglTexCoord2f (1, 0);
	VectorMA (e->e.origin, frame->up, up, point);
	VectorMA (point, frame->right, right, point);
	qglVertex3fv (point);

	qglTexCoord2f (1, 1);
	VectorMA (e->e.origin, frame->down, up, point);
	VectorMA (point, frame->right, right, point);
	qglVertex3fv (point);
	
	qglEnd ();

	GL_State(GLS_DEFAULT);
}

/*
=============================================================

  ALIAS MODELS

=============================================================
*/


#define NUMVERTEXNORMALS	162

float	r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "anorms.h"
};

vec3_t	shadevector;
float	shadelight, ambientlight;

// precalculated dot products for quantized angles
#define SHADEDOT_QUANT 16
float	r_avertexnormal_dots[SHADEDOT_QUANT][256] =
#include "anorm_dots.h"
;

float	*shadedots = r_avertexnormal_dots[0];

int	lastposenum;

/*
=============
GL_DrawAliasFrame
=============
*/
void GL_DrawAliasFrame (mesh1hdr_t *paliashdr, int posenum)
{
	float		s, t;
	float 		l;
	int			index;
	dmdl_trivertx_t	*v, *verts;
	int			list;
	int			*order;
	vec3_t		point;
	float		*normal;
	int			count;
	float		r,g,b,p;

	lastposenum = posenum;

	verts = paliashdr->posedata;
	verts += posenum * paliashdr->poseverts;
	order = paliashdr->commands;

	if (currententity->e.renderfx & RF_COLORSHADE)
	{
		r = currententity->e.shaderRGBA[0] / 255.0;
		g = currententity->e.shaderRGBA[1] / 255.0;
		b = currententity->e.shaderRGBA[2] / 255.0;
	}
	else
		r = g = b = 1;

	while (1)
	{
		// get the vertex count and primitive type
		count = *order++;
		if (!count)
			break;		// done
		if (count < 0)
		{
			count = -count;
			qglBegin (GL_TRIANGLE_FAN);
		}
		else
			qglBegin (GL_TRIANGLE_STRIP);

		do
		{
			// texture coordinates come from the draw list
			qglTexCoord2f (((float *)order)[0], ((float *)order)[1]);
			order += 2;

			// normals and vertexes come from the frame list
			l = shadedots[verts->lightnormalindex] * shadelight;
			qglColor4f (r*l, g*l, b*l, model_constant_alpha);
			qglVertex3f (verts->v[0], verts->v[1], verts->v[2]);
			verts++;
		} while (--count);

		qglEnd ();
	}
}


/*
=============
GL_DrawAliasShadow
=============
*/
extern	vec3_t			lightspot;

void GL_DrawAliasShadow (mesh1hdr_t *paliashdr, int posenum)
{
	float	s, t, l;
	int		i, j;
	int		index;
	dmdl_trivertx_t	*v, *verts;
	int		list;
	int		*order;
	vec3_t	point;
	float	*normal;
	float	height, lheight;
	int		count;

	lheight = currententity->e.origin[2] - lightspot[2];

	height = 0;
	verts = paliashdr->posedata;
	verts += posenum * paliashdr->poseverts;
	order = paliashdr->commands;

	height = -lheight + 1.0;

	while (1)
	{
		// get the vertex count and primitive type
		count = *order++;
		if (!count)
			break;		// done
		if (count < 0)
		{
			count = -count;
			qglBegin (GL_TRIANGLE_FAN);
		}
		else
			qglBegin (GL_TRIANGLE_STRIP);

		do
		{
			// texture coordinates come from the draw list
			// (skipped for shadows) qqglTexCoord2fv ((float *)order);
			order += 2;

			// normals and vertexes come from the frame list
			point[0] = verts->v[0] * paliashdr->scale[0] + paliashdr->scale_origin[0];
			point[1] = verts->v[1] * paliashdr->scale[1] + paliashdr->scale_origin[1];
			point[2] = verts->v[2] * paliashdr->scale[2] + paliashdr->scale_origin[2];

			point[0] -= shadevector[0]*(point[2]+lheight);
			point[1] -= shadevector[1]*(point[2]+lheight);
			point[2] = height;
//			height -= 0.001;
			qglVertex3fv (point);

			verts++;
		} while (--count);

		qglEnd ();
	}	
}



/*
=================
R_SetupAliasFrame

=================
*/
void R_SetupAliasFrame (int frame, mesh1hdr_t *paliashdr)
{
	int				pose, numposes;
	float			interval;

	if ((frame >= paliashdr->numframes) || (frame < 0))
	{
		Con_DPrintf ("R_AliasSetupFrame: no such frame %d\n", frame);
		frame = 0;
	}

	pose = paliashdr->frames[frame].firstpose;
	numposes = paliashdr->frames[frame].numposes;

	if (numposes > 1)
	{
		interval = paliashdr->frames[frame].interval;
		pose += (int)(cl.time / interval) % numposes;
	}

	GL_DrawAliasFrame (paliashdr, pose);
}

void R_HandleCustomSkin(refEntity_t* Ent, int PlayerNum)
{
	if (Ent->skinNum >= 100)
	{
		if (Ent->skinNum > 255) 
		{
			Sys_Error("skinnum > 255");
		}

		if (!gl_extra_textures[Ent->skinNum - 100])  // Need to load it in
		{
			char temp[80];
			QStr::Sprintf(temp, sizeof(temp), "gfx/skin%d.lmp", Ent->skinNum);
			gl_extra_textures[Ent->skinNum - 100] = Draw_CachePic(temp);
		}

		Ent->customSkin = R_GetImageHandle(gl_extra_textures[Ent->skinNum - 100]);
	}
	else if (PlayerNum >= 0 && !gl_nocolors->value)
	{
		// we can't dynamically colormap textures, so they are cached
		// seperately for the players.  Heads are just uncolored.
		//FIXME? What about Demoness and Dwarf?
		if (Ent->hModel == player_models[0] ||
			Ent->hModel == player_models[1] ||
			Ent->hModel == player_models[2] ||
			Ent->hModel == player_models[3])
		{
			if (!cl.players[PlayerNum].Translated)
			{
				R_TranslatePlayerSkin(PlayerNum);
			}

			Ent->customSkin = R_GetImageHandle(playertextures[PlayerNum]);
		}
	}
}

float R_CalcEntityLight(refEntity_t* e)
{
	float* lorg = e->origin;
	if (e->renderfx & RF_LIGHTING_ORIGIN)
	{
		lorg = e->lightingOrigin;
	}

	vec3_t adjust_origin;
	VectorCopy(lorg, adjust_origin);
	model_t* clmodel = Mod_GetModel(e->hModel);
	adjust_origin[2] += (clmodel->q1_mins[2] + clmodel->q1_maxs[2]) / 2;
	float light = R_LightPoint(adjust_origin);

	// allways give the gun some light
	if ((e->renderfx & RF_MINLIGHT) && light < 24)
	{
		light = 24;
	}

	for (int lnum = 0; lnum < MAX_DLIGHTS; lnum++)
	{
		if (cl_dlights[lnum].die >= cl.time)
		{
			vec3_t dist;
			VectorSubtract(lorg, cl_dlights[lnum].origin, dist);
			float add = cl_dlights[lnum].radius - VectorLength(dist);

			if (add > 0)
			{
				light += add;
			}
		}
	}
	return light;
}

/*
=================
R_DrawAliasModel

=================
*/
void R_DrawAliasModel (trRefEntity_t *e)
{
	int			i, j;
	model_t		*clmodel;
	vec3_t		mins, maxs;
	mesh1hdr_t	*paliashdr;
	dmdl_trivertx_t	*verts, *v;
	int			index;
	float		s, t, an;

	clmodel = Mod_GetModel(currententity->e.hModel);

	VectorAdd (currententity->e.origin, clmodel->q1_mins, mins);
	VectorAdd (currententity->e.origin, clmodel->q1_maxs, maxs);

	if (!(e->e.renderfx & RF_FIRST_PERSON) && R_CullBox (mins, maxs))
		return;

	// hack the depth range to prevent view model from poking into walls
	if (e->e.renderfx & RF_DEPTHHACK)
	{
		qglDepthRange (gldepthmin, gldepthmin + 0.3*(gldepthmax-gldepthmin));
	}

	VectorCopy (currententity->e.origin, r_entorigin);
	VectorSubtract (tr.refdef.vieworg, r_entorigin, modelorg);

	//
	// get lighting information
	//

	ambientlight = shadelight = R_CalcEntityLight(&e->e);

	if (e->e.renderfx & RF_FIRST_PERSON)
		cl.light_level = ambientlight;

	// clamp lighting so it doesn't overbright as much
	if (ambientlight > 128)
		ambientlight = 128;
	if (ambientlight + shadelight > 192)
		shadelight = 192 - ambientlight;

	if (e->e.renderfx & RF_ABSOLUTE_LIGHT)
	{
		ambientlight = shadelight = currententity->e.radius * 256.0;
	}

	vec3_t tmp_angles;
	VecToAngles(e->e.axis[0], tmp_angles);
	shadedots = r_avertexnormal_dots[((int)(tmp_angles[1] * (SHADEDOT_QUANT / 360.0))) & (SHADEDOT_QUANT - 1)];
	shadelight = shadelight / 200.0;
	
	VectorCopy(e->e.axis[0], shadevector);
	shadevector[2] = 1;
	VectorNormalize (shadevector);

	//
	// locate the proper data
	//
	paliashdr = (mesh1hdr_t *)Mod_Extradata (clmodel);

	c_alias_polys += paliashdr->numtris;

	//
	// draw all the triangles
	//

	GL_DisableMultitexture();

    qglPushMatrix ();
	R_RotateForEntity(e);

	qglTranslatef(paliashdr->scale_origin[0], paliashdr->scale_origin[1], paliashdr->scale_origin[2]);
	qglScalef(paliashdr->scale[0], paliashdr->scale[1], paliashdr->scale[2]);

	if ((clmodel->q1_flags & EF_SPECIAL_TRANS))
	{
//		qglColor3f( 1,1,1);
		model_constant_alpha = 1.0f;
		qglDisable( GL_CULL_FACE );
		GL_State(GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA | GLS_DSTBLEND_SRC_ALPHA);
	}
	else if (currententity->e.renderfx & RF_WATERTRANS)
	{
//		qglColor4f( 1,1,1,r_wateralpha.value);
		model_constant_alpha = r_wateralpha->value;
		GL_State(GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	}
	else if ((clmodel->q1_flags & EF_TRANSPARENT))
	{
//		qglColor3f( 1,1,1);
		model_constant_alpha = 1.0f;
		GL_State(GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	}
	else if ((clmodel->q1_flags & EF_HOLEY))
	{
//		qglColor3f( 1,1,1);
		model_constant_alpha = 1.0f;
		GL_State(GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	}
	else
	{
		qglColor3f( 1,1,1);
		model_constant_alpha = 1.0f;
		GL_State(GLS_DEPTHMASK_TRUE);
	}

	if (e->e.customSkin)
	{
		GL_Bind(tr.images[e->e.customSkin]);
	}
	else
	{
		int anim = (int)(cl.time * 10) & 3;
		GL_Bind(paliashdr->gl_texture[currententity->e.skinNum][anim]);
	}

	if (gl_smoothmodels->value)
		qglShadeModel (GL_SMOOTH);
	GL_TexEnv(GL_MODULATE);

	if (gl_affinemodels->value)
		qglHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

	R_SetupAliasFrame (currententity->e.frame, paliashdr);

	GL_State(GLS_DEFAULT);

	if ((clmodel->q1_flags & EF_SPECIAL_TRANS))
	{
		qglEnable( GL_CULL_FACE );
	}


	GL_TexEnv(GL_REPLACE);

	qglShadeModel (GL_FLAT);
	if (gl_affinemodels->value)
		qglHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	qglPopMatrix ();

	if (e->e.renderfx & RF_DEPTHHACK)
	{
		qglDepthRange (gldepthmin, gldepthmax);
	}

	if (r_shadows->value)
	{
		qglPushMatrix ();
		R_RotateForEntity(e);
		qglDisable (GL_TEXTURE_2D);
		GL_State(GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		qglColor4f (0,0,0,0.5);
		GL_DrawAliasShadow (paliashdr, lastposenum);
		qglEnable (GL_TEXTURE_2D);
		GL_State(GLS_DEFAULT);
		qglColor4f (1,1,1,1);
		qglPopMatrix ();
	}

}

//==================================================================================

typedef struct sortedent_s {
	trRefEntity_t *ent;
	vec_t len;
} sortedent_t;

sortedent_t     cl_transvisedicts[MAX_VISEDICTS];
sortedent_t		cl_transwateredicts[MAX_VISEDICTS];

int				cl_numtransvisedicts;
int				cl_numtranswateredicts;

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList (void)
{
	int			i;
	qboolean	item_trans;
	mbrush29_leaf_t		*pLeaf;
	vec3_t		diff;
	int			test_length, calc_length;

	cl_numtransvisedicts=0;
	cl_numtranswateredicts=0;

	if (!r_drawentities->value)
		return;

	if (r_entdistance->value <= 0)
	{
		test_length = 9999*9999;
	}
	else
	{
		test_length = r_entdistance->value * r_entdistance->value;
	}

	// draw sprites seperately, because of alpha blending
	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		currententity = &cl_visedicts[i];

		if (currententity->e.renderfx & RF_FIRST_PERSON)
		{
			if (!r_drawviewmodel->value || envmap)
			{
				continue;
			}
		}
		if (currententity->e.renderfx & RF_THIRD_PERSON)
		{
			continue;
		}

		switch (Mod_GetModel(currententity->e.hModel)->type)
		{
		case MOD_MESH1:
			VectorSubtract(currententity->e.origin, tr.refdef.vieworg, diff);
			calc_length = (diff[0]*diff[0]) + (diff[1]*diff[1]) + (diff[2]*diff[2]);
			if (calc_length > test_length)
			{
				continue;
			}

			item_trans = ((currententity->e.renderfx & RF_WATERTRANS) ||
						  (Mod_GetModel(currententity->e.hModel)->q1_flags & (EF_TRANSPARENT|EF_HOLEY|EF_SPECIAL_TRANS))) != 0;
			if (!item_trans)
				R_DrawAliasModel (currententity);
			break;

		case MOD_BRUSH29:
			item_trans = ((currententity->e.renderfx & RF_WATERTRANS)) != 0;
			if (!item_trans)
				R_DrawBrushModel (currententity,false);
			break;

		case MOD_SPRITE:
			VectorSubtract(currententity->e.origin, tr.refdef.vieworg, diff);
			calc_length = (diff[0]*diff[0]) + (diff[1]*diff[1]) + (diff[2]*diff[2]);
			if (calc_length > test_length)
			{
				continue;
			}

			item_trans = true;
			break;

		default:
			item_trans = false;
			break;
		}

		if (item_trans)
		{
			pLeaf = Mod_PointInLeaf (currententity->e.origin, Mod_GetModel(cl.worldmodel));
//			if (pLeaf->contents == CONTENTS_EMPTY)
			if (pLeaf->contents != BSP29CONTENTS_WATER)
				cl_transvisedicts[cl_numtransvisedicts++].ent = currententity;
			else
				cl_transwateredicts[cl_numtranswateredicts++].ent = currententity;
		}
	}
}

/*
================
R_DrawTransEntitiesOnList
Implemented by: jack
================
*/

int transCompare(const void *arg1, const void *arg2 )
{
	const sortedent_t *a1, *a2;
	a1 = (sortedent_t *) arg1;
	a2 = (sortedent_t *) arg2;
	return (a2->len - a1->len); // Sorted in reverse order.  Neat, huh?
}

void R_DrawTransEntitiesOnList ( qboolean inwater)
{
	int i;
	int numents;
	sortedent_t *theents;
	vec3_t result;

	theents = (inwater) ? cl_transwateredicts : cl_transvisedicts;
	numents = (inwater) ? cl_numtranswateredicts : cl_numtransvisedicts;

	for (i=0; i<numents; i++)
	{
		VectorSubtract(
			theents[i].ent->e.origin, 
			tr.refdef.vieworg, 
			result);
//		theents[i].len = Length(result);
		theents[i].len = (result[0] * result[0]) + (result[1] * result[1]) + (result[2] * result[2]);
	}

	qsort((void *) theents, numents, sizeof(sortedent_t), transCompare);
	// Add in BETTER sorting here

	for (i=0;i<numents;i++)
	{
		currententity = theents[i].ent;

		switch (Mod_GetModel(currententity->e.hModel)->type)
		{
		case MOD_MESH1:
			R_DrawAliasModel (currententity);
			break;
		case MOD_BRUSH29:
			R_DrawBrushModel (currententity,true);
			break;
		case MOD_SPRITE:
			R_DrawSpriteModel (currententity);
			break;
		}
	}
	GL_State(GLS_DEPTHMASK_TRUE);
}

/*
============
R_PolyBlend
============
*/
void R_PolyBlend (void)
{
	if (!gl_polyblend->value)
		return;
	if (!v_blend[3])
		return;

//Con_Printf("R_PolyBlend(): %4.2f %4.2f %4.2f %4.2f\n",v_blend[0], v_blend[1],	v_blend[2],	v_blend[3]);

 	GL_DisableMultitexture();

	GL_State(GLS_DEFAULT | GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	qglDisable (GL_TEXTURE_2D);

    qglLoadIdentity ();

    qglRotatef (-90,  1, 0, 0);	    // put Z going up
    qglRotatef (90,  0, 0, 1);	    // put Z going up

	qglColor4fv (v_blend);

	qglBegin (GL_QUADS);

	qglVertex3f (10, 10, 10);
	qglVertex3f (10, -10, 10);
	qglVertex3f (10, -10, -10);
	qglVertex3f (10, 10, -10);
	qglEnd ();

	qglEnable (GL_TEXTURE_2D);
	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80);
}


void R_SetFrustum (void)
{
	int		i;

	// front side is visible

	VectorSubtract(tr.refdef.viewaxis[0], tr.refdef.viewaxis[1], frustum[0].normal);
	VectorAdd(tr.refdef.viewaxis[0], tr.refdef.viewaxis[1], frustum[1].normal);

	VectorAdd (tr.refdef.viewaxis[0], tr.refdef.viewaxis[2], frustum[2].normal);
	VectorSubtract (tr.refdef.viewaxis[0], tr.refdef.viewaxis[2], frustum[3].normal);

	for (i=0 ; i<4 ; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct(tr.refdef.vieworg, frustum[i].normal);
		SetPlaneSignbits(&frustum[i]);
	}
}



/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame (void)
{
	int				edgecount;
	vrect_t			vrect;
	float			w, h;

// don't allow cheats in multiplayer
	r_fullbright->value = 0;

	R_AnimateLight ();

	tr.frameCount++;

	//
	// texturemode stuff
	//
	if (r_textureMode->modified)
	{
		GL_TextureMode(r_textureMode->string);
		r_textureMode->modified = false;
	}

// current viewleaf
	r_oldviewleaf = r_viewleaf;
	r_viewleaf = Mod_PointInLeaf(tr.refdef.vieworg, Mod_GetModel(cl.worldmodel));

	V_SetContentsColor (r_viewleaf->contents);
	V_CalcBlend ();

	r_cache_thrash = false;

	c_brush_polys = 0;
	c_alias_polys = 0;

}


void MYgluPerspective( GLdouble fovy, GLdouble aspect,
		     GLdouble zNear, GLdouble zFar )
{
   GLdouble xmin, xmax, ymin, ymax;

   ymax = zNear * tan( fovy * M_PI / 360.0 );
   ymin = -ymax;

   xmin = ymin * aspect;
   xmax = ymax * aspect;

   qglFrustum( xmin, xmax, ymin, ymax, zNear, zFar );
}


typedef struct _MATRIX {
    GLfloat M[4][4];
} MATRIX;

typedef struct _point3d {
    GLfloat x;
    GLfloat y;
    GLfloat z;
} POINT3D;

static MATRIX	ModelviewMatrix, ProjectionMatrix, FinalMatrix;

void MultiplyMatrix( MATRIX *m1, MATRIX *m2, MATRIX *m3 )
{
    int i, j;

    for( j = 0; j < 4; j ++ )
	{
		for( i = 0; i < 4; i ++ )
		{
			m1->M[j][i] = m2->M[j][0] * m3->M[0][i] +
				m2->M[j][1] * m3->M[1][i] +
				m2->M[j][2] * m3->M[2][i] +
				m2->M[j][3] * m3->M[3][i];
		}
    }
}

void TransformPoint(POINT3D *ptOut, POINT3D *ptIn, MATRIX *mat)
{
    double x, y, z;

    x = (ptIn->x * mat->M[0][0]) + (ptIn->y * mat->M[0][1]) +
        (ptIn->z * mat->M[0][2]) + mat->M[0][3];

    y = (ptIn->x * mat->M[1][0]) + (ptIn->y * mat->M[1][1]) +
        (ptIn->z * mat->M[1][2]) + mat->M[1][3];

    z = (ptIn->x * mat->M[2][0]) + (ptIn->y * mat->M[2][1]) +
        (ptIn->z * mat->M[2][2]) + mat->M[2][3];

    ptOut->x = (float) x;
    ptOut->y = (float) y;
    ptOut->z = (float) z;
}


/*
=============
R_SetupGL
=============
*/
void R_SetupGL (void)
{
	float	screenaspect;
	float	yfov;
	int		i;
	extern	int glwidth, glheight;
	int		x, x2, y2, y, w, h;

	//
	// set up viewpoint
	//
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity ();
	x = tr.refdef.x;
	x2 = tr.refdef.x + tr.refdef.width;
	y = vid.height * glheight/vid.height-tr.refdef.y;
	y2 = vid.height * glheight/vid.height - (tr.refdef.y + tr.refdef.height);

	// fudge around because of frac screen scale
	if (x > 0)
		x--;
	if (x2 < glwidth)
		x2++;
	if (y2 < 0)
		y2--;
	if (y < glheight)
		y++;

	w = x2 - x;
	h = y - y2;

	if (envmap)
	{
		x = y2 = 0;
		w = h = 256;
	}

	qglViewport (glx + x, gly + y2, w, h);
    screenaspect = (float)tr.refdef.width/tr.refdef.height;
    MYgluPerspective(tr.refdef.fov_y,  screenaspect,  4,  4096);

	if (mirror)
	{
		if (mirror_plane->normal[2])
			qglScalef (1, -1, 1);
		else
			qglScalef (-1, 1, 1);
		qglCullFace(GL_BACK);
	}
	else
		qglCullFace(GL_FRONT);

	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity ();

    qglRotatef (-90,  1, 0, 0);	    // put Z going up
    qglRotatef (90,  0, 0, 1);	    // put Z going up

	GLfloat glmat[16];

	glmat[0] = tr.refdef.viewaxis[0][0];
	glmat[1] = tr.refdef.viewaxis[1][0];
	glmat[2] = tr.refdef.viewaxis[2][0];
	glmat[3] = 0;
	glmat[4] = tr.refdef.viewaxis[0][1];
	glmat[5] = tr.refdef.viewaxis[1][1];
	glmat[6] = tr.refdef.viewaxis[2][1];
	glmat[7] = 0;
	glmat[8] = tr.refdef.viewaxis[0][2];
	glmat[9] = tr.refdef.viewaxis[1][2];
	glmat[10] = tr.refdef.viewaxis[2][2];
	glmat[11] = 0;
	glmat[12] = 0;
	glmat[13] = 0;
	glmat[14] = 0;
	glmat[15] = 1;
	qglMultMatrixf(glmat);
    qglTranslatef (-tr.refdef.vieworg[0],  -tr.refdef.vieworg[1],  -tr.refdef.vieworg[2]);

	qglGetFloatv (GL_MODELVIEW_MATRIX, r_world_matrix);

	//
	// set drawing parms
	//
	if (gl_cull->value)
		qglEnable(GL_CULL_FACE);
	else
		qglDisable(GL_CULL_FACE);

	GL_State(GLS_DEFAULT);

	qglGetFloatv(GL_MODELVIEW_MATRIX, (float *)ModelviewMatrix.M);
//	ModelviewMatrix.M[0][3] = 0;
//	ModelviewMatrix.M[1][3] = 0;
//	ModelviewMatrix.M[2][3] = 0;
//	ModelviewMatrix.M[3][3] = 0;
	qglGetFloatv(GL_PROJECTION_MATRIX, (float *)ProjectionMatrix.M);
//	MultiplyMatrix(&FinalMatrix, &ModelviewMatrix, &ProjectionMatrix);
	MultiplyMatrix(&FinalMatrix, &ProjectionMatrix, &ModelviewMatrix);
}

/*
================
R_RenderScene

r_refdef must be set before the first call
================
*/
void R_RenderScene (void)
{
	R_SetupFrame ();

	R_SetFrustum ();

	R_SetupGL ();

	R_MarkLeaves ();	// done here so we know if we're in water

	R_DrawWorld ();		// adds static entities to the list

	S_ExtraUpdate ();	// don't let sound get messed up if going slow

	R_DrawEntitiesOnList ();

	GL_DisableMultitexture();

	R_RenderDlights ();
}


/*
=============
R_Clear
=============
*/
void R_Clear (void)
{
	if (r_mirroralpha->value != 1.0)
	{
		if (gl_clear->value)
			qglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		else
			qglClear (GL_DEPTH_BUFFER_BIT);
		gldepthmin = 0;
		gldepthmax = 0.5;
	}
	else
	{
		if (gl_clear->value)
			qglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		else
			qglClear (GL_DEPTH_BUFFER_BIT);
		gldepthmin = 0;
		gldepthmax = 1;
	}

	qglDepthRange (gldepthmin, gldepthmax);
}

/*
=============
R_PrintTimes
=============
*/
void R_PrintTimes(void)
{
	float r_time2;
	float ms, fps;

	r_lasttime1 = r_time2 = Sys_DoubleTime();

	ms = 1000*(r_time2-r_time1);
	fps = 1000/ms;

	Con_Printf("%3.1f fps %5.0f ms\n%4i wpoly  %4i epoly  %4i(%i) edicts\n",
		fps, ms, c_brush_polys, c_alias_polys, cl_numvisedicts, cl_numtransvisedicts+cl_numtranswateredicts);
}


/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
void R_RenderView (void)
{
	vec3_t t;

	if (r_norefresh->value)
		return;

	if (!r_worldentity.model || !cl.worldmodel)
		Sys_Error ("R_RenderView: NULL worldmodel");

	if (r_speeds->value)
	{
		qglFinish ();
		r_time1 = Sys_DoubleTime ();
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	mirror = false;

	QGL_EnableLogging(!!r_logFile->integer);

	r_refdef.time = (int)(cl.time * 1000);
	R_CommonRenderScene(&r_refdef);

//	qglFinish ();

	R_Clear ();

	// render normal view
	R_RenderScene ();

	R_DrawParticles ();

	R_DrawTransEntitiesOnList( r_viewleaf->contents == BSP29CONTENTS_EMPTY ); // This restores the depth mask

	R_DrawWaterSurfaces ();

	R_DrawTransEntitiesOnList( r_viewleaf->contents != BSP29CONTENTS_EMPTY );

	R_PolyBlend ();

	if (r_speeds->value)
	{
		R_PrintTimes ();
	}
}

void R_DrawName(vec3_t origin, char *Name, int Red)
{
	float	zi;
	int		u, v;
	POINT3D	In, Out;

	if (!Name)
	{
		return;
	}

	In.x = origin[0];
	In.y = origin[1];
	In.z = origin[2];
	TransformPoint(&Out, &In, &FinalMatrix);

	if (Out.z < 0)
	{
		return;
	}

	zi = 1.0 / (Out.z + 8);
	u = (int)(tr.refdef.width / 2 * (zi * Out.x + 1) ) + tr.refdef.x;
	v = (int)(tr.refdef.height / 2 * (zi * (-Out.y) + 1) ) + tr.refdef.y;

	u -= QStr::Length(Name) * 4;

	if(cl_siege)
	{
		if(Red>10)
		{
			Red-=10;
			Draw_Character (u, v, 145);//key
			u+=8;
		}
		if(Red>0&&Red<3)//def
		{
			if(Red==true)
				Draw_Character (u, v, 143);//shield
			else
				Draw_Character (u, v, 130);//crown
			Draw_RedString(u+8, v, Name);
		}
		else if(!Red)
		{
			Draw_Character (u, v, 144);//sword
			Draw_String (u+8, v, Name);
		}
		else
			Draw_String (u+8, v, Name);
	}
	else
		Draw_String (u, v, Name);
}

void R_ClearScene()
{
	cl_numvisedicts = 0;
}

void R_AddRefEntToScene(refEntity_t* Ent)
{
	if (cl_numvisedicts == MAX_VISEDICTS)
	{
		return;
	}
	cl_visedicts[cl_numvisedicts].e = *Ent;
	cl_numvisedicts++;
}
