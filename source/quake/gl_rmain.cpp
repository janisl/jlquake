/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// r_main.c

#include "quakedef.h"
#include "glquake.h"

qboolean	r_cache_thrash;		// compatability

int			c_brush_polys, c_alias_polys;

qboolean	envmap;				// true during envmap command capture 

image_t*	particletexture;	// little dot for particles
image_t*	playertextures[16];		// up to 16 color translated skins

bool		r_third_person;

float	r_world_matrix[16];
float	r_base_world_matrix[16];

//
// screen size info
//
refdef_t	r_refdef;

mbrush29_leaf_t		*r_viewleaf, *r_oldviewleaf;

int		d_lightstylevalue[256];	// 8.8 fraction of base light value

int				cl_numvisedicts;
trRefEntity_t	cl_visedicts[MAX_VISEDICTS];

void R_MarkLeaves (void);

QCvar*	r_norefresh;
QCvar*	r_drawentities;
QCvar*	r_drawviewmodel;
QCvar*	r_speeds;
QCvar*	r_shadows;
QCvar*	r_dynamic;
QCvar*	r_novis;

QCvar*	gl_finish;
QCvar*	gl_clear;
QCvar*	gl_cull;
QCvar*	gl_texsort;
QCvar*	gl_smoothmodels;
QCvar*	gl_affinemodels;
QCvar*	gl_polyblend;
QCvar*	gl_nocolors;
QCvar*	gl_keeptjunctions;
QCvar*	gl_reporttjunctions;
QCvar*	gl_doubleeyes;

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
		if (BoxOnPlaneSide (mins, maxs, &tr.viewParms.frustum[i]) == 2)
			return true;
	return false;
}


void R_RotateForEntity (trRefEntity_t *e)
{
	GLfloat glmat[16];

	glmat[0] = e->e.axis[0][0];
	glmat[1] = e->e.axis[0][1];
	glmat[2] = e->e.axis[0][2];
	glmat[3] = 0;
	glmat[4] = e->e.axis[1][0];
	glmat[5] = e->e.axis[1][1];
	glmat[6] = e->e.axis[1][2];
	glmat[7] = 0;
	glmat[8] = e->e.axis[2][0];
	glmat[9] = e->e.axis[2][1];
	glmat[10] = e->e.axis[2][2];
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

	psprite = (msprite1_t*)R_GetModelByHandle(tr.currentEntity->e.hModel)->q1_cache;
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
void R_DrawSpriteModel (trRefEntity_t *e)
{
	vec3_t	point;
	msprite1frame_t	*frame;
	float		*up, *right;
	vec3_t		v_right;
	msprite1_t		*psprite;

	// don't even bother culling, because it's just a single
	// polygon without a surface cache
	frame = R_GetSpriteFrame (e);
	psprite = (msprite1_t*)R_GetModelByHandle(tr.currentEntity->e.hModel)->q1_cache;

	if (psprite->type == SPR_ORIENTED)
	{
		// bullet marks on walls
		up = tr.currentEntity->e.axis[2];
		VectorSubtract(vec3_origin, tr.currentEntity->e.axis[1], v_right);
		right = v_right;
	}
	else
	{	// normal sprite
		up = tr.refdef.viewaxis[2];
		VectorSubtract(vec3_origin, tr.refdef.viewaxis[1], v_right);
		right = v_right;
	}

	qglColor3f (1,1,1);

	GL_DisableMultitexture();

    GL_Bind(frame->gl_texture);

	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80);
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

lastposenum = posenum;

	verts = paliashdr->posedata;
	verts += posenum * paliashdr->poseverts;
	order = paliashdr->commands;

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
			qglColor3f (l, l, l);
			qglVertex3f(verts->v[0], verts->v[1], verts->v[2]);
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

void R_HandleRefEntColormap(refEntity_t* Ent, int ColorMap)
{
	// we can't dynamically colormap textures, so they are cached
	// seperately for the players.  Heads are just uncolored.
	if (ColorMap && !gl_nocolors->value && !QStr::Cmp(R_GetModelByHandle(Ent->hModel)->name, "progs/player.mdl"))
	{
	    Ent->customSkin = R_GetImageHandle(playertextures[ColorMap - 1]);
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

	clmodel = R_GetModelByHandle(tr.currentEntity->e.hModel);

	VectorAdd (tr.currentEntity->e.origin, clmodel->q1_mins, mins);
	VectorAdd (tr.currentEntity->e.origin, clmodel->q1_maxs, maxs);

	if (R_CullBox (mins, maxs))
		return;

	// hack the depth range to prevent view model from poking into walls
	if (e->e.renderfx & RF_DEPTHHACK)
	{
		qglDepthRange (gldepthmin, gldepthmin + 0.3*(gldepthmax-gldepthmin));
	}

	VectorCopy (tr.currentEntity->e.origin, tr.orient.origin);
	VectorSubtract(tr.refdef.vieworg, tr.orient.origin, tr.orient.viewOrigin);

	//
	// get lighting information
	//

	ambientlight = shadelight = R_LightPoint (tr.currentEntity->e.origin);

	// allways give the gun some light
	if ((e->e.renderfx & RF_MINLIGHT) && ambientlight < 24)
		ambientlight = shadelight = 24;

	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		if (cl_dlights[lnum].die >= cl.time)
		{
			VectorSubtract (tr.currentEntity->e.origin,
							cl_dlights[lnum].origin,
							dist);
			add = cl_dlights[lnum].radius - VectorLength(dist);

			if (add > 0) {
				ambientlight += add;
				//ZOID models should be affected by dlights as well
				shadelight += add;
			}
		}
	}

	// clamp lighting so it doesn't overbright as much
	if (ambientlight > 128)
		ambientlight = 128;
	if (ambientlight + shadelight > 192)
		shadelight = 192 - ambientlight;

	// ZOID: never allow players to go totally black
	if (!QStr::Cmp(clmodel->name, "progs/player.mdl"))
		if (ambientlight < 8)
			ambientlight = shadelight = 8;

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
	VectorNormalize(shadevector);

	//
	// locate the proper data
	//
	paliashdr = (mesh1hdr_t *)Mod_Extradata(R_GetModelByHandle(tr.currentEntity->e.hModel));

	c_alias_polys += paliashdr->numtris;

	//
	// draw all the triangles
	//

	GL_DisableMultitexture();

    qglPushMatrix ();
	R_RotateForEntity (e);

	qglTranslatef (paliashdr->scale_origin[0], paliashdr->scale_origin[1], paliashdr->scale_origin[2]);
	qglScalef (paliashdr->scale[0], paliashdr->scale[1], paliashdr->scale[2]);

	if (e->e.customSkin)
	{
		GL_Bind(tr.images[e->e.customSkin]);
	}
	else
	{
		int anim = (int)(cl.time * 10) & 3;
		GL_Bind(paliashdr->gl_texture[e->e.skinNum][anim]);
	}

	if (gl_smoothmodels->value)
		qglShadeModel (GL_SMOOTH);
	GL_TexEnv(GL_MODULATE);

	if (gl_affinemodels->value)
		qglHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

	R_SetupAliasFrame(tr.currentEntity->e.frame, paliashdr);

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
		R_RotateForEntity (e);
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

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList (void)
{
	int		i;

	if (!r_drawentities->value)
		return;

	// draw sprites seperately, because of alpha blending
	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		tr.currentEntity = &cl_visedicts[i];

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
			R_DrawAliasModel(tr.currentEntity);
			break;

		case MOD_BRUSH29:
			R_DrawBrushModel(tr.currentEntity);
			break;

		default:
			break;
		}
	}

	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		tr.currentEntity = &cl_visedicts[i];

		switch (R_GetModelByHandle(tr.currentEntity->e.hModel)->type)
		{
		case MOD_SPRITE:
			R_DrawSpriteModel(tr.currentEntity);
			break;
		}
	}
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

	GL_DisableMultitexture();

	GL_State(GLS_DEFAULT | GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	qglDisable (GL_TEXTURE_2D);

    qglLoadIdentity ();

    qglRotatef (-90,  1, 0, 0);	    // put Z going up
    qglRotatef (90,  0, 0, 1);	    // put Z going up

	qglColor4fv (v_blend);

	qglBegin (GL_QUADS);

	qglVertex3f(10, 100, 100);
	qglVertex3f(10, -100, 100);
	qglVertex3f(10, -100, -100);
	qglVertex3f(10, 100, -100);
	qglEnd ();

	qglEnable (GL_TEXTURE_2D);
	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80);
}


void R_SetFrustum (void)
{
	int		i;

	if (tr.refdef.fov_x == 90)
	{
		// front side is visible

		VectorSubtract(tr.refdef.viewaxis[0], tr.refdef.viewaxis[1], tr.viewParms.frustum[0].normal);
		VectorAdd(tr.refdef.viewaxis[0], tr.refdef.viewaxis[1], tr.viewParms.frustum[1].normal);

		VectorAdd(tr.refdef.viewaxis[0], tr.refdef.viewaxis[2], tr.viewParms.frustum[2].normal);
		VectorSubtract(tr.refdef.viewaxis[0], tr.refdef.viewaxis[2], tr.viewParms.frustum[3].normal);
	}
	else
	{
		// rotate VPN right by FOV_X/2 degrees
		RotatePointAroundVector( tr.viewParms.frustum[0].normal, tr.refdef.viewaxis[2], tr.refdef.viewaxis[0], -(90 - tr.refdef.fov_x / 2));
		// rotate VPN left by FOV_X/2 degrees
		RotatePointAroundVector( tr.viewParms.frustum[1].normal, tr.refdef.viewaxis[2], tr.refdef.viewaxis[0], 90 - tr.refdef.fov_x / 2);
		// rotate VPN up by FOV_X/2 degrees
		RotatePointAroundVector( tr.viewParms.frustum[2].normal, tr.refdef.viewaxis[1], tr.refdef.viewaxis[0], -(90 - tr.refdef.fov_y / 2));
		// rotate VPN down by FOV_X/2 degrees
		RotatePointAroundVector( tr.viewParms.frustum[3].normal, tr.refdef.viewaxis[1], tr.refdef.viewaxis[0], 90 - tr.refdef.fov_y / 2);
	}

	for (i=0 ; i<4 ; i++)
	{
		tr.viewParms.frustum[i].type = PLANE_ANYZ;
		tr.viewParms.frustum[i].dist = DotProduct(tr.refdef.vieworg, tr.viewParms.frustum[i].normal);
		SetPlaneSignbits(&tr.viewParms.frustum[i]);
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
	r_viewleaf = Mod_PointInLeaf (tr.refdef.vieworg, tr.worldModel);

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
	int		x, x2, y2, y, w, h;

	//
	// set up viewpoint
	//
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity ();
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
    screenaspect = (float)tr.refdef.width/tr.refdef.height;
//	yfov = 2*atan((float)r_refdef.height/r_refdef.width)*180/M_PI;
    MYgluPerspective (tr.refdef.fov_y,  screenaspect,  4,  4096);

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

	R_DrawParticles ();
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
================
R_RenderView

r_refdef must be set before the first call
================
*/
void R_RenderView (void)
{
	double	time1, time2;
	GLfloat colors[4] = {(GLfloat) 0.0, (GLfloat) 0.0, (GLfloat) 1, (GLfloat) 0.20};

	if (r_norefresh->value)
		return;

	tr.frameCount++;
	tr.sceneCount++;
	tr.frameSceneNum = 0;
	tr.frameSceneNum++;

	if (!tr.worldModel)
		Sys_Error ("R_RenderView: NULL worldmodel");

	if (r_speeds->value)
	{
		qglFinish ();
		time1 = Sys_DoubleTime ();
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	if (gl_finish->value)
		qglFinish ();

	QGL_EnableLogging(!!r_logFile->integer);

	r_refdef.time = (int)(cl.time * 1000);
	R_CommonRenderScene(&r_refdef);

	R_Clear ();

	// render normal view

	r_third_person = !!chase_active->value;

	R_RenderScene ();
	R_DrawWaterSurfaces ();

	R_PolyBlend ();

	if (r_speeds->value)
	{
//		qglFinish ();
		time2 = Sys_DoubleTime ();
		Con_Printf ("%3i ms  %4i wpoly %4i epoly\n", (int)((time2-time1)*1000), c_brush_polys, c_alias_polys); 
	}
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

void R_InitSkyTexCoords( float cloudLayerHeight )
{
}
shader_t* R_FindShader( const char *name, int lightmapIndex, qboolean mipRawImage )
{
	return NULL;
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
void    R_RemapShader(const char *oldShader, const char *newShader, const char *timeOffset)
{
}
