// r_main.c

#include "quakedef.h"
#include "glquake.h"

entity_t	r_worldentity;

qboolean	r_cache_thrash;		// compatability

vec3_t		modelorg, r_entorigin;
entity_t	*currententity;

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

extern model_t *player_models[MAX_PLAYER_CLASS];

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

float	r_world_matrix[16];
float	r_base_world_matrix[16];

//
// screen size info
//
refdef_t	r_refdef;

mleaf_t		*r_viewleaf, *r_oldviewleaf;

texture_t	*r_notexture_mip;

int		d_lightstylevalue[256];	// 8.8 fraction of base light value


void R_MarkLeaves (void);

QCvar*	r_norefresh;
QCvar*	r_drawentities;
QCvar*	r_drawviewmodel;
QCvar*	r_speeds;
QCvar*	r_fullbright;
QCvar*	r_lightmap;
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

QCvar*	gl_ztrick;
extern	QCvar*	scr_fov;
static qboolean AlwaysDrawModel;

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


void R_RotateForEntity (entity_t *e)
{
    qglTranslatef (e->origin[0],  e->origin[1],  e->origin[2]);

    qglRotatef (e->angles[1],  0, 0, 1);
    qglRotatef (-e->angles[0],  0, 1, 0);
    qglRotatef (e->angles[2],  1, 0, 0);	//RDM: switched sign so it matches software
}

//==========================================================================
//
// R_RotateForEntity2
//
// Same as R_RotateForEntity(), but checks for EF_ROTATE and modifies
// yaw appropriately.
//
//==========================================================================

static void R_RotateForEntity2(entity_t *e)
{
	float	forward;
	float	yaw, pitch;
	vec3_t			angles;

	qglTranslatef(e->origin[0], e->origin[1], e->origin[2]);

	if (e->model->flags & EF_FACE_VIEW)
	{
		VectorSubtract(e->origin,r_origin,angles);
		VectorSubtract(r_origin,e->origin,angles);
		VectorNormalize(angles);

		if (angles[1] == 0 && angles[0] == 0)
		{
			yaw = 0;
			if (angles[2] > 0)
				pitch = 90;
			else
				pitch = 270;
		}
		else
		{
			yaw = (int) (atan2(angles[1], angles[0]) * 180 / M_PI);
			if (yaw < 0)
				yaw += 360;

			forward = sqrt (angles[0]*angles[0] + angles[1]*angles[1]);
			pitch = (int) (atan2(angles[2], forward) * 180 / M_PI);
			if (pitch < 0)
				pitch += 360;
		}

		angles[0] = pitch;
		angles[1] = yaw;
		angles[2] = 0;

		qglRotatef(-angles[0], 0, 1, 0);
		qglRotatef(angles[1], 0, 0, 1);
//		qglRotatef(-angles[2], 1, 0, 0);
		qglRotatef(-e->angles[2], 1, 0, 0);
	}
	else 
	{
		if (e->model->flags & EF_ROTATE)
		{
			qglRotatef(AngleMod((e->origin[0]+e->origin[1])*0.8
				+(108*cl.time)), 0, 0, 1);
		}
		else
		{
			qglRotatef(e->angles[1], 0, 0, 1);
		}

		qglRotatef(-e->angles[0], 0, 1, 0);
		qglRotatef(-e->angles[2], 1, 0, 0);

		// For clientside rotation stuff
		qglRotatef(e->angleAdd[0], 0, 1, 0);
		qglRotatef(e->angleAdd[1], 0, 0, 1);
		qglRotatef(e->angleAdd[2], 1, 0, 0);
	}
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
mspriteframe_t *R_GetSpriteFrame (entity_t *currententity)
{
	msprite_t		*psprite;
	mspritegroup_t	*pspritegroup;
	mspriteframe_t	*pspriteframe;
	int				i, numframes, frame;
	float			*pintervals, fullinterval, targettime, time;

	psprite = (msprite_t*)currententity->model->cache.data;
	frame = currententity->frame;

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
		pspritegroup = (mspritegroup_t *)psprite->frames[frame].frameptr;
		pintervals = pspritegroup->intervals;
		numframes = pspritegroup->numframes;
		fullinterval = pintervals[numframes-1];

		time = cl.time + currententity->syncbase;

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
void R_DrawSpriteModel (entity_t *e)
{
	vec3_t	point;
	mspriteframe_t	*frame;
	float		*up, *right;
	vec3_t		v_forward, v_right, v_up;
	msprite_t		*psprite;

	// don't even bother culling, because it's just a single
	// polygon without a surface cache

	if (currententity->drawflags & DRF_TRANSLUCENT)
	{
		qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		qglEnable( GL_BLEND );
		qglColor4f (1,1,1,r_wateralpha->value);
	}
	else if (currententity->model->flags & EF_TRANSPARENT)
	{
		qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		qglEnable( GL_BLEND );
		qglColor3f(1,1,1);
	}
	else
	{
//		qglColor3f(1,1,1);
		qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		qglEnable( GL_BLEND );
		qglColor3f(1,1,1);
	}

	frame = R_GetSpriteFrame (e);
	psprite = (msprite_t*)currententity->model->cache.data;

	if (psprite->type == SPR_ORIENTED)
	{	// bullet marks on walls
		AngleVectors (currententity->angles, v_forward, v_right, v_up);
		up = v_up;
		right = v_right;
	}
	else
	{	// normal sprite
		up = vup;
		right = vright;
	}

	GL_DisableMultitexture();

    GL_Bind(frame->gl_texture);

	qglBegin (GL_QUADS);

	qglTexCoord2f (0, 1);
	VectorMA (e->origin, frame->down, up, point);
	VectorMA (point, frame->left, right, point);
	qglVertex3fv (point);

	qglTexCoord2f (0, 0);
	VectorMA (e->origin, frame->up, up, point);
	VectorMA (point, frame->left, right, point);
	qglVertex3fv (point);

	qglTexCoord2f (1, 0);
	VectorMA (e->origin, frame->up, up, point);
	VectorMA (point, frame->right, right, point);
	qglVertex3fv (point);

	qglTexCoord2f (1, 1);
	VectorMA (e->origin, frame->down, up, point);
	VectorMA (point, frame->right, right, point);
	qglVertex3fv (point);
	
	qglEnd ();

	qglDisable( GL_BLEND );
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
void GL_DrawAliasFrame (aliashdr_t *paliashdr, int posenum)
{
	float		s, t;
	float 		l;
	int			i, j;
	int			index;
	trivertx_t	*v, *verts;
	int			list;
	int			*order;
	vec3_t		point;
	float		*normal;
	int			count;
	float		r,g,b,p;
	char		client_team[16], this_team[16];
	qboolean	OnTeam = false;
	byte		ColorShade;
	int			my_team, ve_team;

	lastposenum = posenum;

	verts = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
	verts += posenum * paliashdr->poseverts;
	order = (int *)((byte *)paliashdr + paliashdr->commands);

	ColorShade = currententity->colorshade;

	i = currententity->scoreboard - cl.players;
	if (i >= 0 && i<MAX_CLIENTS)
	{
		my_team = cl.players[cl.playernum].siege_team;
		ve_team = cl.players[i].siege_team;
		if((ambientlight+shadelight)>50||(cl_siege&&my_team==ve_team))
			cl.players[i].shownames_off = false;
		else
			cl.players[i].shownames_off = true;
		if(cl_siege)
		{
			if(cl.players[cl.playernum].playerclass==CLASS_DWARF&&currententity->skinnum==101)
			{
				ColorShade = 133;
				if(ambientlight<128)
					shadelight += (128 - ambientlight);
				cl.players[i].shownames_off = false;
			}
			else if(cl.players[cl.playernum].playerclass==CLASS_DWARF&&(ambientlight+shadelight)<51)//OOps, use darkmaps in GL
			{
				ColorShade = 128 + (int)((ambientlight+shadelight)/5);
				shadelight += (51 - ambientlight);
				cl.players[i].shownames_off = false;
			}
			else if(ve_team==ST_DEFENDER)
			{//tint gold since we can't have seperate skins
				OnTeam = true;
				ColorShade = 165;
			}
		}
		else
		{
			QStr::NCpy(client_team, Info_ValueForKey(cl.players[cl.playernum].userinfo, "team"), 16);
			client_team[15] = 0;
			if (client_team[0])
			{
				QStr::NCpy(this_team, Info_ValueForKey(cl.players[i].userinfo, "team"), 16);
				this_team[15] = 0;
				if (QStr::ICmp(client_team, this_team) == 0)
				{
					OnTeam = true;
					ColorShade = r_teamcolor->value;
				}
			}
		}
	}

	if (ColorShade)
	{
		r = RTint[ColorShade];
		g = GTint[ColorShade];
		b = BTint[ColorShade];
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

void GL_DrawAliasShadow (aliashdr_t *paliashdr, int posenum)
{
	float	s, t, l;
	int		i, j;
	int		index;
	trivertx_t	*v, *verts;
	int		list;
	int		*order;
	vec3_t	point;
	float	*normal;
	float	height, lheight;
	int		count;

	lheight = currententity->origin[2] - lightspot[2];

	height = 0;
	verts = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
	verts += posenum * paliashdr->poseverts;
	order = (int *)((byte *)paliashdr + paliashdr->commands);

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
void R_SetupAliasFrame (int frame, aliashdr_t *paliashdr)
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



/*
=================
R_DrawAliasModel

=================
*/
void R_DrawAliasModel (entity_t *e)
{
	int			i, j;
	int			lnum;
	vec3_t		dist;
	float		add;
	model_t		*clmodel;
	vec3_t		mins, maxs;
	aliashdr_t	*paliashdr;
	trivertx_t	*verts, *v;
	int			index;
	float		s, t, an;
	int			anim;
	static float	tmatrix[3][4];
	float entScale;
	float xyfact;
	float zfact;
	char		temp[80];
	int mls;
	vec3_t		adjust_origin;

	clmodel = currententity->model;

	VectorAdd (currententity->origin, clmodel->mins, mins);
	VectorAdd (currententity->origin, clmodel->maxs, maxs);

	if (!AlwaysDrawModel && R_CullBox (mins, maxs))
		return;


	VectorCopy (currententity->origin, r_entorigin);
	VectorSubtract (r_origin, r_entorigin, modelorg);

	//
	// get lighting information
	//

	VectorCopy(currententity->origin, adjust_origin);
	adjust_origin[2] += (currententity->model->mins[2] + currententity->model->maxs[2]) / 2;
	ambientlight = shadelight = R_LightPoint (adjust_origin);

	// allways give the gun some light
	if (e == &cl.viewent && ambientlight < 24)
		ambientlight = shadelight = 24;

	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		if (cl_dlights[lnum].die >= cl.time)
		{
			VectorSubtract (currententity->origin,
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

	mls = currententity->drawflags&MLS_MASKIN;
	if(currententity->model->flags&EF_ROTATE)
	{
		ambientlight = shadelight =
			60+34+sin(currententity->origin[0]+currententity->origin[1]
				+(cl.time*3.8))*34;
	}
	else if (mls == MLS_ABSLIGHT)

	{
		ambientlight = shadelight = currententity->abslight;
	}
	else if (mls != MLS_NONE)
	{ // Use a model light style (25-30)
		ambientlight = shadelight = d_lightstylevalue[24+mls]/2;
	}

	shadedots = r_avertexnormal_dots[((int)(e->angles[1] * (SHADEDOT_QUANT / 360.0))) & (SHADEDOT_QUANT - 1)];
	shadelight = shadelight / 200.0;
	
	an = e->angles[1]/180*M_PI;
	shadevector[0] = cos(-an);
	shadevector[1] = sin(-an);
	shadevector[2] = 1;
	VectorNormalize (shadevector);

	//
	// locate the proper data
	//
	paliashdr = (aliashdr_t *)Mod_Extradata (currententity->model);

	c_alias_polys += paliashdr->numtris;

	//
	// draw all the triangles
	//

	GL_DisableMultitexture();

    qglPushMatrix ();
	R_RotateForEntity2(e);

	if(currententity->scale != 0 && currententity->scale != 100)
	{
		entScale = (float)currententity->scale/100.0;
		switch(currententity->drawflags&SCALE_TYPE_MASKIN)
		{
		case SCALE_TYPE_UNIFORM:
			tmatrix[0][0] = paliashdr->scale[0]*entScale;
			tmatrix[1][1] = paliashdr->scale[1]*entScale;
			tmatrix[2][2] = paliashdr->scale[2]*entScale;
			xyfact = zfact = (entScale-1.0)*127.95;
			break;
		case SCALE_TYPE_XYONLY:
			tmatrix[0][0] = paliashdr->scale[0]*entScale;
			tmatrix[1][1] = paliashdr->scale[1]*entScale;
			tmatrix[2][2] = paliashdr->scale[2];
			xyfact = (entScale-1.0)*127.95;
			zfact = 1.0;
			break;
		case SCALE_TYPE_ZONLY:
			tmatrix[0][0] = paliashdr->scale[0];
			tmatrix[1][1] = paliashdr->scale[1];
			tmatrix[2][2] = paliashdr->scale[2]*entScale;
			xyfact = 1.0;
			zfact = (entScale-1.0)*127.95;
			break;
		}
		switch(currententity->drawflags&SCALE_ORIGIN_MASKIN)
		{
		case SCALE_ORIGIN_CENTER:
			tmatrix[0][3] = paliashdr->scale_origin[0]-paliashdr->scale[0]*xyfact;
			tmatrix[1][3] = paliashdr->scale_origin[1]-paliashdr->scale[1]*xyfact;
			tmatrix[2][3] = paliashdr->scale_origin[2]-paliashdr->scale[2]*zfact;
			break;
		case SCALE_ORIGIN_BOTTOM:
			tmatrix[0][3] = paliashdr->scale_origin[0]-paliashdr->scale[0]*xyfact;
			tmatrix[1][3] = paliashdr->scale_origin[1]-paliashdr->scale[1]*xyfact;
			tmatrix[2][3] = paliashdr->scale_origin[2];
			break;
		case SCALE_ORIGIN_TOP:
			tmatrix[0][3] = paliashdr->scale_origin[0]-paliashdr->scale[0]*xyfact;
			tmatrix[1][3] = paliashdr->scale_origin[1]-paliashdr->scale[1]*xyfact;
			tmatrix[2][3] = paliashdr->scale_origin[2]-paliashdr->scale[2]*zfact*2.0;
			break;
		}
	}
	else
	{
		tmatrix[0][0] = paliashdr->scale[0];
		tmatrix[1][1] = paliashdr->scale[1];
		tmatrix[2][2] = paliashdr->scale[2];
		tmatrix[0][3] = paliashdr->scale_origin[0];
		tmatrix[1][3] = paliashdr->scale_origin[1];
		tmatrix[2][3] = paliashdr->scale_origin[2];
	}

	if(clmodel->flags&EF_ROTATE)
	{ // Floating motion
		tmatrix[2][3] += sin(currententity->origin[0]
			+currententity->origin[1]+(cl.time*3))*5.5;
	}

// [0][3] [1][3] [2][3]
//	qglTranslatef (paliashdr->scale_origin[0], paliashdr->scale_origin[1], paliashdr->scale_origin[2]);
	qglTranslatef (tmatrix[0][3],tmatrix[1][3],tmatrix[2][3]);
// [0][0] [1][1] [2][2]
//	qglScalef (paliashdr->scale[0], paliashdr->scale[1], paliashdr->scale[2]);
	qglScalef (tmatrix[0][0],tmatrix[1][1],tmatrix[2][2]);

	if ((currententity->model->flags & EF_SPECIAL_TRANS))
	{
		qglEnable (GL_BLEND);
		qglBlendFunc (GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
//		qglColor3f( 1,1,1);
		model_constant_alpha = 1.0f;
		qglDisable( GL_CULL_FACE );
	}
	else if (currententity->drawflags & DRF_TRANSLUCENT)
	{
		qglEnable (GL_BLEND);
		qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//		qglColor4f( 1,1,1,r_wateralpha.value);
		model_constant_alpha = r_wateralpha->value;
	}
	else if ((currententity->model->flags & EF_TRANSPARENT))
	{
		qglEnable (GL_BLEND);
		qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//		qglColor3f( 1,1,1);
		model_constant_alpha = 1.0f;
	}
	else if ((currententity->model->flags & EF_HOLEY))
	{
		qglEnable (GL_BLEND);
		qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

//		qglColor3f( 1,1,1);
		model_constant_alpha = 1.0f;
	}
	else
	{
		qglColor3f( 1,1,1);
		model_constant_alpha = 1.0f;
	}

//	if(cl.players[currententity->scoreboard - cl.players].siege_team==ST_DEFENDER)
//		currententity->skinnum = cl.players[currententity->scoreboard - cl.players].playerclass+110;

	if (currententity->skinnum >= 100)
	{
		if (currententity->skinnum > 255) 
		{
			Sys_Error ("skinnum > 255");
		}

		if (!gl_extra_textures[currententity->skinnum-100])  // Need to load it in
		{
			sprintf(temp,"gfx/skin%d.lmp",currententity->skinnum);
			gl_extra_textures[currententity->skinnum-100] = Draw_CachePic(temp);
		}

		GL_Bind(gl_extra_textures[currententity->skinnum-100]);
	}
	else
	{
		anim = (int)(cl.time*10) & 3;
		GL_Bind(paliashdr->gl_texture[currententity->skinnum][anim]);

		// we can't dynamically colormap textures, so they are cached
		// seperately for the players.  Heads are just uncolored.
	
		if (currententity->colormap != vid.colormap && !gl_nocolors->value)
		{//FIXME? What about Demoness and Dwarf?
			if (currententity->model == player_models[0] ||
			    currententity->model == player_models[1] ||
			    currententity->model == player_models[2] ||
			    currententity->model == player_models[3])
			{
				i = currententity->scoreboard - cl.players;
//				if (currententity->scoreboard && !currententity->scoreboard->skin)
//				{
//					//Skin_Find(currententity->scoreboard);
//					R_TranslatePlayerSkin(i);
//				}
				if (i >= 0 && i<MAX_CLIENTS)
				{
					if (!cl.players[i].Translated)
					{
						R_TranslatePlayerSkin(i);
					}

					GL_Bind(playertextures[i]);
				}
			}
		}
	}

	if (gl_smoothmodels->value)
		qglShadeModel (GL_SMOOTH);
	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	if (gl_affinemodels->value)
		qglHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

	R_SetupAliasFrame (currententity->frame, paliashdr);
	if ((currententity->drawflags & DRF_TRANSLUCENT) ||
		(currententity->model->flags & EF_SPECIAL_TRANS))
		qglDisable (GL_BLEND);

	if ((currententity->model->flags & EF_TRANSPARENT))
		qglDisable (GL_BLEND);

	if ((currententity->model->flags & EF_HOLEY))
		qglDisable (GL_BLEND);

	if ((currententity->model->flags & EF_SPECIAL_TRANS))
	{
		qglEnable( GL_CULL_FACE );
	}


	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	qglShadeModel (GL_FLAT);
	if (gl_affinemodels->value)
		qglHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	qglPopMatrix ();

	if (r_shadows->value)
	{
		qglPushMatrix ();
		R_RotateForEntity2 (e);
		qglDisable (GL_TEXTURE_2D);
		qglEnable (GL_BLEND);
		qglColor4f (0,0,0,0.5);
		GL_DrawAliasShadow (paliashdr, lastposenum);
		qglEnable (GL_TEXTURE_2D);
		qglDisable (GL_BLEND);
		qglColor4f (1,1,1,1);
		qglPopMatrix ();
	}

}

//==================================================================================

typedef struct sortedent_s {
	entity_t *ent;
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
	mleaf_t		*pLeaf;
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

//		if(currententity->drawflags&5)//MLS_INVIS - but dwarf can see
//			if(cl.v.playerclass!=CLASS_DWARF)
//				continue;

		switch (currententity->model->type)
		{
		case mod_alias:
			VectorSubtract(currententity->origin, r_origin, diff);
			calc_length = (diff[0]*diff[0]) + (diff[1]*diff[1]) + (diff[2]*diff[2]);
			if (calc_length > test_length)
			{
				continue;
			}

			item_trans = ((currententity->drawflags & DRF_TRANSLUCENT) ||
						  (currententity->model->flags & (EF_TRANSPARENT|EF_HOLEY|EF_SPECIAL_TRANS))) != 0;
			if (!item_trans)
				R_DrawAliasModel (currententity);
			break;

		case mod_brush:
			item_trans = ((currententity->drawflags & DRF_TRANSLUCENT)) != 0;
			if (!item_trans)
				R_DrawBrushModel (currententity,false);
			break;

		case mod_sprite:
			VectorSubtract(currententity->origin, r_origin, diff);
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
			pLeaf = Mod_PointInLeaf (currententity->origin, cl.worldmodel);
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
	int depthMaskWrite = 0;
	vec3_t result;

	theents = (inwater) ? cl_transwateredicts : cl_transvisedicts;
	numents = (inwater) ? cl_numtranswateredicts : cl_numtransvisedicts;

	for (i=0; i<numents; i++)
	{
		VectorSubtract(
			theents[i].ent->origin, 
			r_origin, 
			result);
//		theents[i].len = Length(result);
		theents[i].len = (result[0] * result[0]) + (result[1] * result[1]) + (result[2] * result[2]);
	}

	qsort((void *) theents, numents, sizeof(sortedent_t), transCompare);
	// Add in BETTER sorting here

	qglDepthMask(0);
	for (i=0;i<numents;i++)
	{
		currententity = theents[i].ent;

		switch (currententity->model->type)
		{
		case mod_alias:
			if (!depthMaskWrite)
			{
				depthMaskWrite = 1;
				qglDepthMask(1);
			}
			R_DrawAliasModel (currententity);
			break;
		case mod_brush:
			if (!depthMaskWrite)
			{
				depthMaskWrite = 1;
				qglDepthMask(1);
			}
			R_DrawBrushModel (currententity,true);
			break;
		case mod_sprite:
			if (depthMaskWrite)
			{
				depthMaskWrite = 0;
				qglDepthMask(0);
			}
			R_DrawSpriteModel (currententity);
			break;
		}
	}
	if (!depthMaskWrite) 
		qglDepthMask(1);
}

/*
=============
R_DrawViewModel
=============
*/
void R_DrawViewModel (void)
{
	float		ambient[4], diffuse[4];
	int			j;
	int			lnum;
	vec3_t		dist;
	float		add;
	dlight_t	*dl;
	int			ambientlight, shadelight;

	if (!r_drawviewmodel->value || cl.spectator)
		return;

	if (envmap)
		return;

	if (!r_drawentities->value)
		return;

//rjr	if (cl.items & IT_INVISIBILITY)
//rjr		return;

	if (cl.v.health <= 0)
		return;

	currententity = &cl.viewent;
	if (!currententity->model)
		return;

	j = R_LightPoint (currententity->origin);

	if (j < 24)
		j = 24;		// allways give some light on gun
	ambientlight = j;
	shadelight = j;

// add dynamic lights		
	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		dl = &cl_dlights[lnum];
		if (!dl->radius)
			continue;
		if (!dl->radius)
			continue;
		if (dl->die < cl.time)
			continue;

		VectorSubtract (currententity->origin, dl->origin, dist);
		add = dl->radius - VectorLength(dist);
		if (add > 0)
			ambientlight += add;
	}

	cl.light_level = ambientlight;

	ambient[0] = ambient[1] = ambient[2] = ambient[3] = (float)ambientlight / 128;
	diffuse[0] = diffuse[1] = diffuse[2] = diffuse[3] = (float)shadelight / 128;

	// hack the depth range to prevent view model from poking into walls
	qglDepthRange (gldepthmin, gldepthmin + 0.3*(gldepthmax-gldepthmin));
	AlwaysDrawModel = true;
	R_DrawAliasModel (currententity);
	AlwaysDrawModel = false;
	qglDepthRange (gldepthmin, gldepthmax);
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

	qglDisable (GL_ALPHA_TEST);
	qglEnable (GL_BLEND);
	qglDisable (GL_DEPTH_TEST);
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

	qglDisable (GL_BLEND);
	qglEnable (GL_TEXTURE_2D);
	qglEnable (GL_ALPHA_TEST);
}


void R_SetFrustum (void)
{
	int		i;

	// front side is visible

	VectorAdd (vpn, vright, frustum[0].normal);
	VectorSubtract (vpn, vright, frustum[1].normal);

	VectorAdd (vpn, vup, frustum[2].normal);
	VectorSubtract (vpn, vup, frustum[3].normal);

	for (i=0 ; i<4 ; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct (r_origin, frustum[i].normal);
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

// build the transformation matrix for the given view angles
	VectorCopy (r_refdef.vieworg, r_origin);

	AngleVectors (r_refdef.viewangles, vpn, vright, vup);

// current viewleaf
	r_oldviewleaf = r_viewleaf;
	r_viewleaf = Mod_PointInLeaf (r_origin, cl.worldmodel);

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
	x = r_refdef.vrect.x * glwidth/vid.width;
	x2 = (r_refdef.vrect.x + r_refdef.vrect.width) * glwidth/vid.width;
	y = (vid.height-r_refdef.vrect.y) * glheight/vid.height;
	y2 = (vid.height - (r_refdef.vrect.y + r_refdef.vrect.height)) * glheight/vid.height;

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
    screenaspect = (float)r_refdef.vrect.width/r_refdef.vrect.height;
//	yfov = 2*atan((float)r_refdef.vrect.height/r_refdef.vrect.width)*180/M_PI;
//	yfov = (2.0 * tan (scr_fov.value/360*M_PI)) / screenaspect;
	yfov = 2*atan((float)r_refdef.vrect.height/r_refdef.vrect.width)*(scr_fov->value*2)/M_PI;
    MYgluPerspective (yfov,  screenaspect,  4,  4096);

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
    qglRotatef (-r_refdef.viewangles[2],  1, 0, 0);
    qglRotatef (-r_refdef.viewangles[0],  0, 1, 0);
    qglRotatef (-r_refdef.viewangles[1],  0, 0, 1);
    qglTranslatef (-r_refdef.vieworg[0],  -r_refdef.vieworg[1],  -r_refdef.vieworg[2]);

	qglGetFloatv (GL_MODELVIEW_MATRIX, r_world_matrix);

	//
	// set drawing parms
	//
	if (gl_cull->value)
		qglEnable(GL_CULL_FACE);
	else
		qglDisable(GL_CULL_FACE);

	qglDisable(GL_BLEND);
	qglDisable(GL_ALPHA_TEST);
	qglEnable(GL_DEPTH_TEST);

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
		qglDepthFunc (GL_LEQUAL);
	}
	else if (gl_ztrick->value)
	{
		static int trickframe;

		if (gl_clear->value)
			qglClear (GL_COLOR_BUFFER_BIT);

		trickframe++;
		if (trickframe & 1)
		{
			gldepthmin = 0;
			gldepthmax = 0.49999;
			qglDepthFunc (GL_LEQUAL);
		}
		else
		{
			gldepthmin = 1;
			gldepthmax = 0.5;
			qglDepthFunc (GL_GEQUAL);
		}
	}
	else
	{
		if (gl_clear->value)
			qglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		else
			qglClear (GL_DEPTH_BUFFER_BIT);
		gldepthmin = 0;
		gldepthmax = 1;
		qglDepthFunc (GL_LEQUAL);
	}

	qglDepthRange (gldepthmin, gldepthmax);
}

#if 0 //!!! FIXME, Zoid, mirror is disabled for now
/*
=============
R_Mirror
=============
*/
void R_Mirror (void)
{
	float		d;
	msurface_t	*s;
	entity_t	*ent;

	if (!mirror)
		return;

	Com_Memcpy(r_base_world_matrix, r_world_matrix, sizeof(r_base_world_matrix));

	d = DotProduct (r_refdef.vieworg, mirror_plane->normal) - mirror_plane->dist;
	VectorMA (r_refdef.vieworg, -2*d, mirror_plane->normal, r_refdef.vieworg);

	d = DotProduct (vpn, mirror_plane->normal);
	VectorMA (vpn, -2*d, mirror_plane->normal, vpn);

	r_refdef.viewangles[0] = -asin (vpn[2])/M_PI*180;
	r_refdef.viewangles[1] = atan2 (vpn[1], vpn[0])/M_PI*180;
	r_refdef.viewangles[2] = -r_refdef.viewangles[2];

	ent = &cl_entities[cl.viewentity];
	if (cl_numvisedicts < MAX_VISEDICTS)
	{
		cl_visedicts[cl_numvisedicts] = ent;
		cl_numvisedicts++;
	}

	gldepthmin = 0.5;
	gldepthmax = 1;
	qglDepthRange (gldepthmin, gldepthmax);
	qglDepthFunc (GL_LEQUAL);

	qglDepthMask(0);

	R_DrawParticles ();
// THIS IS THE F*S*D(KCING MIRROR ROUTINE!  Go down!!!
	R_DrawTransEntitiesOnList( true ); // This restores the depth mask

	R_DrawWaterSurfaces ();

	R_DrawTransEntitiesOnList( false );


	gldepthmin = 0;
	gldepthmax = 0.5;
	qglDepthRange (gldepthmin, gldepthmax);
	qglDepthFunc (GL_LEQUAL);

	// blend on top
	qglEnable (GL_BLEND);
	qglMatrixMode(GL_PROJECTION);
	if (mirror_plane->normal[2])
		qglScalef (1,-1,1);
	else
		qglScalef (-1,1,1);
	qglCullFace(GL_FRONT);
	qglMatrixMode(GL_MODELVIEW);

	qglLoadMatrixf (r_base_world_matrix);

	qglColor4f (1,1,1,r_mirroralpha.value);
	s = cl.worldmodel->textures[mirrortexturenum]->texturechain;
	for ( ; s ; s=s->texturechain)
		R_RenderBrushPoly (s, true);
	cl.worldmodel->textures[mirrortexturenum]->texturechain = NULL;
	qglDisable (GL_BLEND);
	qglColor4f (1,1,1,1);
}
#endif


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

//	qglFinish ();

	R_Clear ();

	// render normal view
	R_RenderScene ();

	qglDepthMask(0);

	R_DrawParticles ();

	R_DrawTransEntitiesOnList( r_viewleaf->contents == BSP29CONTENTS_EMPTY ); // This restores the depth mask

	R_DrawWaterSurfaces ();

	R_DrawTransEntitiesOnList( r_viewleaf->contents != BSP29CONTENTS_EMPTY );

	R_DrawViewModel();

	// render mirror view
//	R_Mirror ();

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
	u = (int)(r_refdef.vrect.width / 2 * (zi * Out.x + 1) ) + r_refdef.vrect.x;
	v = (int)(r_refdef.vrect.height / 2 * (zi * (-Out.y) + 1) ) + r_refdef.vrect.y;

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



