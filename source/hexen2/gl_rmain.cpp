// gl_main.c

/*
 * $Header: /H2 Mission Pack/gl_rmain.c 4     3/30/98 10:57a Jmonroe $
 */

#include "quakedef.h"
#include "glquake.h"

qboolean	r_cache_thrash;		// compatability

int			c_brush_polys, c_alias_polys, c_sky_polys;

qboolean	envmap;				// true during envmap command capture 

image_t*	particletexture;	// little dot for particles
image_t*	playertextures[16];		// up to 16 color translated skins
image_t*	gl_extra_textures[MAX_EXTRA_TEXTURES];   // generic textures for models

float		model_constant_alpha;

float		r_time1;
float		r_lasttime1 = 0;

extern qhandle_t	player_models[NUM_CLASSES];

bool		r_third_person;

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
QCvar*	r_dynamic;
QCvar*	r_novis;
QCvar*	r_wholeframe;

QCvar*	gl_clear;
QCvar*	gl_cull;
QCvar*	gl_texsort;
QCvar*	gl_smoothmodels;
QCvar*	gl_affinemodels;
QCvar*	gl_polyblend;
QCvar*	gl_nocolors;
QCvar*	gl_keeptjunctions;
QCvar*	gl_reporttjunctions;

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
msprite1frame_t *R_GetSpriteFrame (msprite1_t *psprite)
{
	msprite1group_t	*pspritegroup;
	msprite1frame_t	*pspriteframe;
	int				i, numframes, frame;
	float			*pintervals, fullinterval, targettime, time;

	frame = tr.currentEntity->e.frame;

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

		time = cl.time + tr.currentEntity->e.shaderTime;

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
typedef struct
{
	vec3_t			vup, vright, vpn;	// in worldspace
} spritedesc_t;

void R_DrawSpriteModel (trRefEntity_t *e)
{
	vec3_t	point;
	msprite1frame_t	*frame;
	msprite1_t		*psprite;
	
	vec3_t			tvec;
	float			dot, angle, sr, cr;
	spritedesc_t			r_spritedesc;
	int i;

	psprite = (msprite1_t*)R_GetModelByHandle(tr.currentEntity->e.hModel)->q1_cache;

	frame = R_GetSpriteFrame (psprite);

	if (tr.currentEntity->e.renderfx & RF_WATERTRANS)
	{
		// rjr
		qglColor4f (1,1,1,r_wateralpha->value);
	}
	else if (R_GetModelByHandle(tr.currentEntity->e.hModel)->q1_flags & EF_TRANSPARENT)
	{
		// rjr
		qglColor3f(1,1,1);
	}
	else
	{
		// rjr
		qglColor3f(1,1,1);
	}

	GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);

	if (psprite->type == SPR_FACING_UPRIGHT)
	{
	// generate the sprite's axes, with vup straight up in worldspace, and
	// r_spritedesc.vright perpendicular to tr.refdef.vieworg.
	// This will not work if the view direction is very close to straight up or
	// down, because the cross product will be between two nearly parallel
	// vectors and starts to approach an undefined state, so we don't draw if
	// the two vectors are less than 1 degree apart
		tvec[0] = -tr.refdef.vieworg[0];
		tvec[1] = -tr.refdef.vieworg[1];
		tvec[2] = -tr.refdef.vieworg[2];
		VectorNormalize (tvec);
		dot = tvec[2];	// same as DotProduct (tvec, r_spritedesc.vup) because
						//  r_spritedesc.vup is 0, 0, 1
		if ((dot > 0.999848) || (dot < -0.999848))	// cos(1 degree) = 0.999848
			return;
		r_spritedesc.vup[0] = 0;
		r_spritedesc.vup[1] = 0;
		r_spritedesc.vup[2] = 1;
		r_spritedesc.vright[0] = tvec[1];
								// CrossProduct(r_spritedesc.vup, -tr.refdef.vieworg,
		r_spritedesc.vright[1] = -tvec[0];
								//              r_spritedesc.vright)
		r_spritedesc.vright[2] = 0;
		VectorNormalize (r_spritedesc.vright);
		r_spritedesc.vpn[0] = -r_spritedesc.vright[1];
		r_spritedesc.vpn[1] = r_spritedesc.vright[0];
		r_spritedesc.vpn[2] = 0;
					// CrossProduct (r_spritedesc.vright, r_spritedesc.vup,
					//  r_spritedesc.vpn)
	}
	else if (psprite->type == SPR_VP_PARALLEL)
	{
	// generate the sprite's axes, completely parallel to the viewplane. There
	// are no problem situations, because the sprite is always in the same
	// position relative to the viewer
		for (i=0 ; i<3 ; i++)
		{
			r_spritedesc.vup[i] = tr.refdef.viewaxis[2][i];
			r_spritedesc.vright[i] = -tr.refdef.viewaxis[1][i];
			r_spritedesc.vpn[i] = tr.refdef.viewaxis[0][i];
		}
	}
	else if (psprite->type == SPR_VP_PARALLEL_UPRIGHT)
	{
	// generate the sprite's axes, with vup straight up in worldspace, and
	// r_spritedesc.vright parallel to the viewplane.
	// This will not work if the view direction is very close to straight up or
	// down, because the cross product will be between two nearly parallel
	// vectors and starts to approach an undefined state, so we don't draw if
	// the two vectors are less than 1 degree apart
		dot = tr.refdef.viewaxis[0][2];	// same as DotProduct (vpn, r_spritedesc.vup) because
						//  r_spritedesc.vup is 0, 0, 1
		if ((dot > 0.999848) || (dot < -0.999848))	// cos(1 degree) = 0.999848
			return;
		r_spritedesc.vup[0] = 0;
		r_spritedesc.vup[1] = 0;
		r_spritedesc.vup[2] = 1;
		r_spritedesc.vright[0] = tr.refdef.viewaxis[0][1];
										// CrossProduct (r_spritedesc.vup, vpn,
		r_spritedesc.vright[1] = -tr.refdef.viewaxis[0][0];	//  r_spritedesc.vright)
		r_spritedesc.vright[2] = 0;
		VectorNormalize (r_spritedesc.vright);
		r_spritedesc.vpn[0] = -r_spritedesc.vright[1];
		r_spritedesc.vpn[1] = r_spritedesc.vright[0];
		r_spritedesc.vpn[2] = 0;
					// CrossProduct (r_spritedesc.vright, r_spritedesc.vup,
					//  r_spritedesc.vpn)
	}
	else if (psprite->type == SPR_ORIENTED)
	{
	// generate the sprite's axes, according to the sprite's world orientation
		VectorCopy(tr.currentEntity->e.axis[0], r_spritedesc.vpn);
		VectorSubtract(vec3_origin,	tr.currentEntity->e.axis[1], r_spritedesc.vright);
		VectorCopy(tr.currentEntity->e.axis[2], r_spritedesc.vup);
	}
	else if (psprite->type == SPR_VP_PARALLEL_ORIENTED)
	{
	// generate the sprite's axes, parallel to the viewplane, but rotated in
	// that plane around the center according to the sprite entity's roll
	// angle. So vpn stays the same, but vright and vup rotate
		vec3_t tmp_angles;
		VecToAngles(e->e.axis[0], tmp_angles);
		angle = tmp_angles[ROLL] * (M_PI*2 / 360);
		sr = sin(angle);
		cr = cos(angle);

		for (i=0 ; i<3 ; i++)
		{
			r_spritedesc.vpn[i] = tr.refdef.viewaxis[0][i];
			r_spritedesc.vright[i] = -tr.refdef.viewaxis[1][i] * cr + tr.refdef.viewaxis[2][i] * sr;
			r_spritedesc.vup[i] = tr.refdef.viewaxis[1][i] * sr + tr.refdef.viewaxis[2][i] * cr;
		}
	}
	else
	{
		Sys_Error ("R_DrawSprite: Bad sprite type %d", psprite->type);
	}

//	R_RotateSprite (psprite->beamlength);

    GL_Bind(frame->gl_texture);

	qglBegin (GL_QUADS);

	qglTexCoord2f (0, 1);
	VectorMA (e->e.origin, frame->down, r_spritedesc.vup, point);
	VectorMA (point, frame->left, r_spritedesc.vright, point);
	qglVertex3fv (point);

	qglTexCoord2f (0, 0);
	VectorMA (e->e.origin, frame->up, r_spritedesc.vup, point);
	VectorMA (point, frame->left, r_spritedesc.vright, point);
	qglVertex3fv (point);

	qglTexCoord2f (1, 0);
	VectorMA (e->e.origin, frame->up, r_spritedesc.vup, point);
	VectorMA (point, frame->right, r_spritedesc.vright, point);
	qglVertex3fv (point);

	qglTexCoord2f (1, 1);
	VectorMA (e->e.origin, frame->down, r_spritedesc.vup, point);
	VectorMA (point, frame->right, r_spritedesc.vright, point);
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
	float	s, t;
	float 	l;
	int		i, j;
	int		index;
	dmdl_trivertx_t	*v, *verts;
	int		list;
	int		*order;
	vec3_t	point;
	float	*normal;
	int		count;
	float	r,g,b,p;

lastposenum = posenum;

	verts = paliashdr->posedata;
	verts += posenum * paliashdr->poseverts;
	order = paliashdr->commands;

	if (tr.currentEntity->e.renderfx & RF_COLORSHADE)
	{
		r = tr.currentEntity->e.shaderRGBA[0] / 255.0;
		g = tr.currentEntity->e.shaderRGBA[1] / 255.0;
		b = tr.currentEntity->e.shaderRGBA[2] / 255.0;
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

	lheight = tr.currentEntity->e.origin[2] - lightspot[2];

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
			Sys_Error ("skinnum > 255");
		}

		if (!gl_extra_textures[Ent->skinNum - 100])  // Need to load it in
		{
			char temp[40];
			QStr::Sprintf(temp, sizeof(temp), "gfx/skin%d.lmp", Ent->skinNum);
			gl_extra_textures[Ent->skinNum - 100] = Draw_CachePic(temp);
		}

		Ent->customSkin = R_GetImageHandle(gl_extra_textures[Ent->skinNum - 100]);
	}
	else if (PlayerNum >= 0 && !gl_nocolors->value && Ent->hModel)
	{
		// we can't dynamically colormap textures, so they are cached
		// seperately for the players.  Heads are just uncolored.
		if (Ent->hModel == player_models[0] ||
			Ent->hModel == player_models[1] ||
			Ent->hModel == player_models[2] ||
			Ent->hModel == player_models[3] ||
			Ent->hModel == player_models[4])
		{
			Ent->customSkin = R_GetImageHandle(playertextures[PlayerNum]);
		}
	}
}


/*
=================
R_DrawAliasModel

=================
*/
void R_DrawAliasModel (trRefEntity_t *e)
{
	int			i, j;
	int			lnum;
	vec3_t		dist;
	float		add;
	model_t		*clmodel;
	vec3_t		mins, maxs;
	mesh1hdr_t	*paliashdr;
	dmdl_trivertx_t	*verts, *v;
	int			index;
	float		s, t, an;
	vec3_t		adjust_origin;

	clmodel = R_GetModelByHandle(tr.currentEntity->e.hModel);

	VectorAdd (tr.currentEntity->e.origin, clmodel->q1_mins, mins);
	VectorAdd (tr.currentEntity->e.origin, clmodel->q1_maxs, maxs);

	if (R_CullLocalBox(&clmodel->q1_mins) == CULL_OUT)
	{
		return;
	}

	// hack the depth range to prevent view model from poking into walls
	if (e->e.renderfx & RF_DEPTHHACK)
	{
		qglDepthRange (gldepthmin, gldepthmin + 0.3*(gldepthmax-gldepthmin));
	}

	//
	// get lighting information
	//

	float* lorg = e->e.origin;
	if (e->e.renderfx & RF_LIGHTING_ORIGIN)
	{
		lorg = e->e.lightingOrigin;
	}
	VectorCopy(lorg, adjust_origin);
	adjust_origin[2] += (clmodel->q1_mins[2] + clmodel->q1_maxs[2]) / 2;
	ambientlight = shadelight = R_LightPoint (adjust_origin);

	// allways give the gun some light
	if ((e->e.renderfx & RF_MINLIGHT) && ambientlight < 24)
		ambientlight = shadelight = 24;

	for (lnum=0 ; lnum<tr.refdef.num_dlights; lnum++)
	{
		VectorSubtract(lorg,
						tr.refdef.dlights[lnum].origin,
						dist);
		add = tr.refdef.dlights[lnum].radius - VectorLength(dist);

		if (add > 0)
			ambientlight += add;
	}

	if (e->e.renderfx & RF_FIRST_PERSON)
		cl.light_level = ambientlight;

	// clamp lighting so it doesn't overbright as much
	if (ambientlight > 128)
		ambientlight = 128;
	if (ambientlight + shadelight > 192)
		shadelight = 192 - ambientlight;

	if (e->e.renderfx & RF_ABSOLUTE_LIGHT)
	{
		ambientlight = shadelight = tr.currentEntity->e.radius * 256.0;
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

    qglPushMatrix ();
	R_RotateForEntity(e, &tr.viewParms, &tr.orient);

	qglLoadMatrixf(tr.orient.modelMatrix);

	qglTranslatef(paliashdr->scale_origin[0], paliashdr->scale_origin[1], paliashdr->scale_origin[2]);
	qglScalef(paliashdr->scale[0], paliashdr->scale[1], paliashdr->scale[2]);

	if ((clmodel->q1_flags & EF_SPECIAL_TRANS))
	{
		// rjr
//		qglColor3f( 1,1,1);
		model_constant_alpha = 1.0f;
		qglDisable( GL_CULL_FACE );
		GL_State(GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA | GLS_DSTBLEND_SRC_ALPHA);
	}
	else if (tr.currentEntity->e.renderfx & RF_WATERTRANS)
	{
		// rjr
//		qglColor4f( 1,1,1,r_wateralpha.value);
		model_constant_alpha = r_wateralpha->value;
		GL_State(GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	}
	else if ((clmodel->q1_flags & EF_TRANSPARENT))
	{
		// rjr
//		qglColor3f( 1,1,1);
		model_constant_alpha = 1.0f;
		GL_State(GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	}
	else if ((clmodel->q1_flags & EF_HOLEY))
	{
		// rjr
//		qglColor3f( 1,1,1);
		model_constant_alpha = 1.0f;
		GL_State(GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	}
	else
	{
		// rjr
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
		GL_Bind(paliashdr->gl_texture[tr.currentEntity->e.skinNum][0]);
	}

	if (gl_smoothmodels->value)
		qglShadeModel (GL_SMOOTH);
	GL_TexEnv(GL_MODULATE);

	if (gl_affinemodels->value)
		qglHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

	R_SetupAliasFrame (tr.currentEntity->e.frame, paliashdr);

	GL_State(GLS_DEFAULT);

	if ((clmodel->q1_flags & EF_SPECIAL_TRANS))
	{
		// rjr
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
		qglLoadMatrixf(tr.orient.modelMatrix);
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

sortedent_t     cl_transvisedicts[MAX_ENTITIES];
sortedent_t		cl_transwateredicts[MAX_ENTITIES];

int				cl_numtransvisedicts;
int				cl_numtranswateredicts;

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList (void)
{
	int		i;
	qboolean item_trans;
	mbrush29_leaf_t *pLeaf;

	cl_numtransvisedicts=0;
	cl_numtranswateredicts=0;

	if (!r_drawentities->value)
		return;

	// draw sprites seperately, because of alpha blending
	for (i=0 ; i<tr.refdef.num_entities; i++)
	{
		tr.currentEntity = &tr.refdef.entities[i];

		if (tr.currentEntity->e.renderfx & RF_FIRST_PERSON)
		{
			if (r_third_person || !r_drawviewmodel->value || envmap)
			{
				continue;
			}
		}
		if (tr.currentEntity->e.renderfx & RF_THIRD_PERSON)
		{
			if (!r_third_person)
			{
				continue;
			}
		}

		switch (R_GetModelByHandle(tr.currentEntity->e.hModel)->type)
		{
		case MOD_MESH1:
			item_trans = ((tr.currentEntity->e.renderfx & RF_WATERTRANS) ||
						  (R_GetModelByHandle(tr.currentEntity->e.hModel)->q1_flags & (EF_TRANSPARENT|EF_HOLEY|EF_SPECIAL_TRANS))) != 0;
			if (!item_trans)
				R_DrawAliasModel (tr.currentEntity);

			break;

		case MOD_BRUSH29:
			item_trans = ((tr.currentEntity->e.renderfx & RF_WATERTRANS)) != 0;
			if (!item_trans)
				R_DrawBrushModel (tr.currentEntity,false);

			break;


		case MOD_SPRITE:
			item_trans = true;

			break;

		default:
			item_trans = false;
			break;
		}
		
		if (item_trans) {
			pLeaf = Mod_PointInLeafQ1 (tr.currentEntity->e.origin, tr.worldModel);
//			if (pLeaf->contents == CONTENTS_EMPTY)
			if (pLeaf->contents != BSP29CONTENTS_WATER)
				cl_transvisedicts[cl_numtransvisedicts++].ent = tr.currentEntity;
			else
				cl_transwateredicts[cl_numtranswateredicts++].ent = tr.currentEntity;
		}
	}
}

/*
================
R_DrawTransEntitiesOnList
Implemented by: jack
================
*/

int transCompare(const void *arg1, const void *arg2 ) {
	const sortedent_t *a1, *a2;
	a1 = (sortedent_t *) arg1;
	a2 = (sortedent_t *) arg2;
	return (a2->len - a1->len); // Sorted in reverse order.  Neat, huh?
}

void R_DrawTransEntitiesOnList ( qboolean inwater) {
	int i;
	int numents;
	sortedent_t *theents;
	vec3_t result;

	theents = (inwater) ? cl_transwateredicts : cl_transvisedicts;
	numents = (inwater) ? cl_numtranswateredicts : cl_numtransvisedicts;

	for (i=0; i<numents; i++) {
		VectorSubtract(
			theents[i].ent->e.origin, 
			tr.refdef.vieworg, 
			result);
//		theents[i].len = Length(result);
		theents[i].len = (result[0] * result[0]) + (result[1] * result[1]) + (result[2] * result[2]);
	}

	qsort((void *) theents, numents, sizeof(sortedent_t), transCompare);
	// Add in BETTER sorting here

	for (i=0;i<numents;i++) {
		tr.currentEntity = theents[i].ent;

		switch (R_GetModelByHandle(tr.currentEntity->e.hModel)->type)
		{
		case MOD_MESH1:
			R_DrawAliasModel (tr.currentEntity);
			break;
		case MOD_BRUSH29:
			R_DrawBrushModel (tr.currentEntity,true);
			break;
		case MOD_SPRITE:
			R_DrawSpriteModel (tr.currentEntity);
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
	if (cl.maxclients > 1)
		Cvar_Set ("r_fullbright", "0");

	R_AnimateLight ();

	tr.viewCount++;

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
	r_viewleaf = Mod_PointInLeafQ1(tr.refdef.vieworg, tr.worldModel);

	V_SetContentsColor (r_viewleaf->contents);
	V_CalcBlend ();

	r_cache_thrash = false;

	c_brush_polys = c_alias_polys = c_sky_polys = 0;

}


/*
=============
R_SetupGL
=============
*/
void R_SetupGL (void)
{
	int		x, x2, y2, y, w, h;

	//
	// set up viewpoint
	//
	// JACK: Changes for non-scaled
	x = tr.refdef.x;
	x2 = tr.refdef.x + tr.refdef.width;
	y = glConfig.vidHeight - tr.refdef.y;
	y2 = glConfig.vidHeight - (tr.refdef.y + tr.refdef.height);

	// fudge around because of frac screen scale
	if (x > 0)
		x--;
	if (x2 < glConfig.vidWidth)
		x2++;
	if (y2 < 0)
		y2--;
	if (y < glConfig.vidHeight)
		y++;

	w = x2 - x;
	h = y - y2;

	if (envmap)
	{
		x = y2 = 0;
		w = h = 256;
	}

	qglViewport (x, y2, w, h);
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity ();
	R_SetupProjection();
	qglMultMatrixf(tr.viewParms.projectionMatrix);

	qglCullFace(GL_FRONT);

	qglMatrixMode(GL_MODELVIEW);

	R_RotateForViewer();
	qglLoadMatrixf(tr.viewParms.world.modelMatrix);

	//
	// set drawing parms
	//
	if (gl_cull->value)
		qglEnable(GL_CULL_FACE);
	else
		qglDisable(GL_CULL_FACE);

	GL_State(GLS_DEFAULT);
}

/*
================
R_RenderScene

r_refdef must be set before the first call
================
*/
void R_RenderScene ()
{
	R_SetupFrame ();

	VectorCopy(tr.refdef.vieworg, tr.viewParms.orient.origin);
	VectorCopy(tr.refdef.viewaxis[0], tr.viewParms.orient.axis[0]);
	VectorCopy(tr.refdef.viewaxis[1], tr.viewParms.orient.axis[1]);
	VectorCopy(tr.refdef.viewaxis[2], tr.viewParms.orient.axis[2]);
	tr.viewParms.fovX = tr.refdef.fov_x;
	tr.viewParms.fovY = tr.refdef.fov_y;
	R_SetupFrustum();

	R_SetupGL ();

	R_MarkLeaves ();	// done here so we know if we're in water

	R_DrawWorld ();		// adds static entities to the list

	S_ExtraUpdate ();	// don't let sound get messed up if going slow
	
	R_DrawEntitiesOnList ();
}


/*
=============
R_Clear
=============
*/
void R_Clear (void)
{
	if (gl_clear->value)
		qglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	else
		qglClear (GL_DEPTH_BUFFER_BIT);
	gldepthmin = 0;
	gldepthmax = 1;

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

	Con_Printf("%3.1f fps %5.0f ms\n%4i wpoly  %4i epoly %4i spoly\n",
		fps, ms, c_brush_polys, c_alias_polys, c_sky_polys);
}

/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
void R_RenderView (void)
{
	double	time1, time2;

	if (r_norefresh->value)
		return;

	tr.frameCount++;
	tr.frameSceneNum = 0;

	if (!tr.worldModel)
		Sys_Error ("R_RenderView: NULL worldmodel");

	if (r_speeds->value)
	{
		qglFinish();
		if (r_wholeframe->value)
		   r_time1 = r_lasttime1;
	   else
		   r_time1 = Sys_DoubleTime ();
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	QGL_EnableLogging(!!r_logFile->integer);

	r_refdef.time = (int)(cl.time * 1000);
	R_CommonRenderScene(&r_refdef);

	R_PushDlights ();

//	qglFinish ();

	R_Clear ();

	r_third_person = !!chase_active->value;

	// render normal view
	R_RenderScene ();

	R_DrawParticles ();

	R_DrawTransEntitiesOnList( r_viewleaf->contents == BSP29CONTENTS_EMPTY ); // This restores the depth mask

	R_DrawWaterSurfaces ();

	R_DrawTransEntitiesOnList( r_viewleaf->contents != BSP29CONTENTS_EMPTY );

	R_PolyBlend ();

	if (r_speeds->value)
		R_PrintTimes ();
}

void R_InitSkyTexCoords( float cloudLayerHeight )
{
}
void R_SyncRenderThread()
{
}
void GL_CreateSurfaceLightmap (mbrush38_surface_t *surf)
{
}
void GL_EndBuildingLightmaps (void)
{
}
void GL_BeginBuildingLightmaps (model_t *m)
{
}
void RB_StageIteratorGeneric( void )
{
}
void RB_StageIteratorSky( void )
{
}
void RB_StageIteratorVertexLitTexture( void )
{
}
void RB_StageIteratorLightmappedMultitexture( void )
{
}
