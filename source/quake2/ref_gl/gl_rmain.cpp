/*
Copyright (C) 1997-2001 Id Software, Inc.

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
#include "gl_local.h"

void R_Clear (void);

extern int				r_numdlights;
extern dlight_t			r_dlights[MAX_DLIGHTS];
extern int				r_numentities;
extern trRefEntity_t	r_entities[MAX_ENTITIES];
extern lightstyle_t		r_lightstyles[MAX_LIGHTSTYLES];
extern int				r_numparticles;
extern particle_t		r_particles[MAX_PARTICLES];

refimport_t	ri;

float		gldepthmin, gldepthmax;

glstate2_t  gl_state;

image_t		*r_particletexture;	// little dot for particles

trRefEntity_t	*currententity;

cplane_t	frustum[4];

int			c_brush_polys, c_alias_polys;

float		v_blend[4];			// final blending color

float	r_world_matrix[16];
float	r_base_world_matrix[16];

//
// screen size info
//
refdef_t	r_newrefdef;

int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

QCvar	*r_norefresh;
QCvar	*r_drawentities;
QCvar	*r_drawworld;
QCvar	*r_speeds;
QCvar	*r_novis;
QCvar	*r_nocull;
QCvar	*r_lerpmodels;
QCvar	*r_lefthand;

QCvar	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level

QCvar	*gl_nosubimage;

QCvar	*gl_vertex_arrays;

QCvar	*gl_particle_min_size;
QCvar	*gl_particle_max_size;
QCvar	*gl_particle_size;
QCvar	*gl_particle_att_a;
QCvar	*gl_particle_att_b;
QCvar	*gl_particle_att_c;

QCvar	*gl_drawbuffer;
QCvar  *gl_driver;
QCvar	*gl_shadows;
QCvar	*gl_dynamic;
QCvar	*gl_modulate;
QCvar	*gl_skymip;
QCvar	*gl_showtris;
QCvar	*gl_finish;
QCvar	*gl_clear;
QCvar	*gl_cull;
QCvar	*gl_polyblend;
QCvar  *gl_saturatelighting;
QCvar	*gl_lockpvs;

QCvar	*vid_ref;

/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
qboolean R_CullBox (vec3_t mins, vec3_t maxs)
{
	int		i;

	if (r_nocull->value)
		return false;

	for (i=0 ; i<4 ; i++)
		if ( BOX_ON_PLANE_SIDE(mins, maxs, &frustum[i]) == 2)
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
=================
R_DrawSpriteModel

=================
*/
void R_DrawSpriteModel (trRefEntity_t *e)
{
	float alpha = 1.0F;
	vec3_t	point;
	dsp2_frame_t	*frame;
	float		*up, *left;
	dsprite2_t		*psprite;

	// don't even bother culling, because it's just a single
	// polygon without a surface cache

	psprite = (dsprite2_t *)currentmodel->q2_extradata;

#if 0
	if (e->frame < 0 || e->frame >= psprite->numframes)
	{
		ri.Con_Printf (PRINT_ALL, "no such sprite frame %i\n", e->frame);
		e->frame = 0;
	}
#endif
	e->e.frame %= psprite->numframes;

	frame = &psprite->frames[e->e.frame];

#if 0
	if (psprite->type == SPR_ORIENTED)
	{	// bullet marks on walls
	vec3_t		v_forward, v_right, v_up;

	AngleVectors (currententity->angles, v_forward, v_right, v_up);
		up = v_up;
		right = v_right;
	}
	else
#endif
	{	// normal sprite
		up = tr.refdef.viewaxis[2];
		left = tr.refdef.viewaxis[1];
	}

	if ( e->e.renderfx & RF_TRANSLUCENT )
		alpha = e->e.shaderRGBA[3] / 255.0;

	if ( alpha != 1.0F )
	{
		GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	}
	else
	{
		GL_State(GLS_DEFAULT | GLS_ATEST_GE_80);
	}

	qglColor4f( 1, 1, 1, alpha );

    GL_Bind(currentmodel->q2_skins[e->e.frame]);

	GL_TexEnv( GL_MODULATE );

	qglBegin (GL_QUADS);

	qglTexCoord2f (0, 1);
	VectorMA (e->e.origin, -frame->origin_y, up, point);
	VectorMA (point, frame->origin_x, left, point);
	qglVertex3fv (point);

	qglTexCoord2f (0, 0);
	VectorMA (e->e.origin, frame->height - frame->origin_y, up, point);
	VectorMA (point, frame->origin_x, left, point);
	qglVertex3fv (point);

	qglTexCoord2f (1, 0);
	VectorMA (e->e.origin, frame->height - frame->origin_y, up, point);
	VectorMA (point, -(frame->width - frame->origin_x), left, point);
	qglVertex3fv (point);

	qglTexCoord2f (1, 1);
	VectorMA (e->e.origin, -frame->origin_y, up, point);
	VectorMA (point, -(frame->width - frame->origin_x), left, point);
	qglVertex3fv (point);
	
	qglEnd ();

	GL_State(GLS_DEFAULT);
	GL_TexEnv(GL_REPLACE);

	if ( alpha != 1.0F )
		GL_State(GLS_DEFAULT);

	qglColor4f( 1, 1, 1, 1 );
}

//==================================================================================

/*
=============
R_DrawNullModel
=============
*/
void R_DrawNullModel (void)
{
	vec3_t	shadelight;
	int		i;

	if ( currententity->e.renderfx & RF_ABSOLUTE_LIGHT)
		shadelight[0] = shadelight[1] = shadelight[2] = currententity->e.radius;
	else
		R_LightPoint (currententity->e.origin, shadelight);

    qglPushMatrix ();
	R_RotateForEntity (currententity);

	qglDisable (GL_TEXTURE_2D);
	qglColor3fv (shadelight);

	qglBegin (GL_TRIANGLE_FAN);
	qglVertex3f (0, 0, -16);
	for (i=0 ; i<=4 ; i++)
		qglVertex3f (16*cos(i*M_PI/2), 16*sin(i*M_PI/2), 0);
	qglEnd ();

	qglBegin (GL_TRIANGLE_FAN);
	qglVertex3f (0, 0, 16);
	for (i=4 ; i>=0 ; i--)
		qglVertex3f (16*cos(i*M_PI/2), 16*sin(i*M_PI/2), 0);
	qglEnd ();

	qglColor3f (1,1,1);
	qglPopMatrix ();
	qglEnable (GL_TEXTURE_2D);
}

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

	// draw non-transparent first
	for (i=0 ; i<tr.refdef.num_entities ; i++)
	{
		currententity = &tr.refdef.entities[i];
		if (currententity->e.renderfx & RF_TRANSLUCENT)
			continue;	// solid

		if (currententity->e.reType == RT_BEAM)
		{
			R_DrawBeam( currententity );
		}
		else
		{
			currentmodel = R_GetModelByHandle(currententity->e.hModel);
			switch (currentmodel->type)
			{
			case MOD_BAD:
				R_DrawNullModel();
				break;
			case MOD_MESH2:
				R_DrawAliasModel(currententity);
				break;
			case MOD_BRUSH38:
				R_DrawBrushModel(currententity);
				break;
			case MOD_SPRITE2:
				R_DrawSpriteModel(currententity);
				break;
			default:
				ri.Sys_Error (ERR_DROP, "Bad modeltype");
				break;
			}
		}
	}

	// draw transparent entities
	// we could sort these if it ever becomes a problem...
	for (i=0 ; i<tr.refdef.num_entities ; i++)
	{
		currententity = &tr.refdef.entities[i];
		if (!(currententity->e.renderfx & RF_TRANSLUCENT))
			continue;	// solid

		if (currententity->e.reType == RT_BEAM)
		{
			R_DrawBeam( currententity );
		}
		else
		{
			currentmodel = R_GetModelByHandle(currententity->e.hModel);
			switch (currentmodel->type)
			{
			case MOD_BAD:
				R_DrawNullModel();
				break;
			case MOD_MESH2:
				R_DrawAliasModel(currententity);
				break;
			case MOD_BRUSH38:
				R_DrawBrushModel(currententity);
				break;
			case MOD_SPRITE2:
				R_DrawSpriteModel(currententity);
				break;
			default:
				ri.Sys_Error (ERR_DROP, "Bad modeltype");
				break;
			}
		}
	}
	GL_State(GLS_DEPTHMASK_TRUE);		// back to writing

}

/*
** GL_DrawParticles
**
*/
void GL_DrawParticles( int num_particles, const particle_t particles[], const unsigned colortable[768] )
{
	const particle_t *p;
	int				i;
	vec3_t			up, right;
	float			scale;
	byte			color[4];

    GL_Bind(r_particletexture);
	GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);		// no z buffering
	GL_TexEnv( GL_MODULATE );
	qglBegin( GL_TRIANGLES );

	VectorScale(tr.refdef.viewaxis[2], 1.5, up);
	VectorScale(tr.refdef.viewaxis[1], -1.5, right);

	for ( p = particles, i=0 ; i < num_particles ; i++,p++)
	{
		// hack a scale up to keep particles from disapearing
		scale = ( p->origin[0] - tr.refdef.vieworg[0] ) * tr.refdef.viewaxis[0][0] + 
			    ( p->origin[1] - tr.refdef.vieworg[1] ) * tr.refdef.viewaxis[0][1] +
			    ( p->origin[2] - tr.refdef.vieworg[2] ) * tr.refdef.viewaxis[0][2];

		if (scale < 20)
			scale = 1;
		else
			scale = 1 + scale * 0.004;

		*(int *)color = colortable[p->color];
		color[3] = p->alpha*255;

		qglColor4ubv( color );

		qglTexCoord2f( 0.0625, 0.0625 );
		qglVertex3fv( p->origin );

		qglTexCoord2f( 1.0625, 0.0625 );
		qglVertex3f( p->origin[0] + up[0]*scale, 
			         p->origin[1] + up[1]*scale, 
					 p->origin[2] + up[2]*scale);

		qglTexCoord2f( 0.0625, 1.0625 );
		qglVertex3f( p->origin[0] + right[0]*scale, 
			         p->origin[1] + right[1]*scale, 
					 p->origin[2] + right[2]*scale);
	}

	qglEnd ();
	qglColor4f( 1,1,1,1 );
	GL_State(GLS_DEPTHMASK_TRUE);		// back to normal Z buffering
	GL_TexEnv( GL_REPLACE );
}

/*
===============
R_DrawParticles
===============
*/
void R_DrawParticles (void)
{
	if (qglPointParameterfEXT )
	{
		int i;
		unsigned char color[4];
		const particle_t *p;

		GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		qglDisable( GL_TEXTURE_2D );

		qglPointSize( gl_particle_size->value );

		qglBegin( GL_POINTS );
		for ( i = 0, p = tr.refdef.particles; i < tr.refdef.num_particles; i++, p++ )
		{
			*(int *)color = d_8to24table[p->color];
			color[3] = p->alpha*255;

			qglColor4ubv( color );

			qglVertex3fv( p->origin );
		}
		qglEnd();

		qglColor4f( 1.0F, 1.0F, 1.0F, 1.0F );
		GL_State(GLS_DEPTHMASK_TRUE);
		qglEnable( GL_TEXTURE_2D );

	}
	else
	{
		GL_DrawParticles(tr.refdef.num_particles, tr.refdef.particles, d_8to24table);
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

	GL_State(GLS_DEFAULT | GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	qglDisable (GL_TEXTURE_2D);

    qglLoadIdentity ();

	// FIXME: get rid of these
    qglRotatef (-90,  1, 0, 0);	    // put Z going up
    qglRotatef (90,  0, 0, 1);	    // put Z going up

	qglColor4fv (v_blend);

	qglBegin (GL_QUADS);

	qglVertex3f (10, 100, 100);
	qglVertex3f (10, -100, 100);
	qglVertex3f (10, -100, -100);
	qglVertex3f (10, 100, -100);
	qglEnd ();

	qglEnable (GL_TEXTURE_2D);
	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80);

	qglColor4f(1,1,1,1);
}

//=======================================================================

void R_SetFrustum (void)
{
	int		i;

#if 0
	/*
	** this code is wrong, since it presume a 90 degree FOV both in the
	** horizontal and vertical plane
	*/
	// front side is visible
	VectorAdd (vpn, vright, frustum[0].normal);
	VectorSubtract (vpn, vright, frustum[1].normal);
	VectorAdd (vpn, vup, frustum[2].normal);
	VectorSubtract (vpn, vup, frustum[3].normal);

	// we theoretically don't need to normalize these vectors, but I do it
	// anyway so that debugging is a little easier
	VectorNormalize( frustum[0].normal );
	VectorNormalize( frustum[1].normal );
	VectorNormalize( frustum[2].normal );
	VectorNormalize( frustum[3].normal );
#else
	// rotate VPN right by FOV_X/2 degrees
	RotatePointAroundVector(frustum[0].normal, tr.refdef.viewaxis[2], tr.refdef.viewaxis[0], -(90 - tr.refdef.fov_x / 2));
	// rotate VPN left by FOV_X/2 degrees
	RotatePointAroundVector(frustum[1].normal, tr.refdef.viewaxis[2], tr.refdef.viewaxis[0], 90 - tr.refdef.fov_x / 2);
	// rotate VPN up by FOV_X/2 degrees
	RotatePointAroundVector(frustum[2].normal, tr.refdef.viewaxis[1], tr.refdef.viewaxis[0], -(90 - tr.refdef.fov_y / 2));
	// rotate VPN down by FOV_X/2 degrees
	RotatePointAroundVector(frustum[3].normal, tr.refdef.viewaxis[1], tr.refdef.viewaxis[0], 90 - tr.refdef.fov_y / 2);
#endif

	for (i=0 ; i<4 ; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct(tr.refdef.vieworg, frustum[i].normal);
		SetPlaneSignbits(&frustum[i]);
	}
}

//=======================================================================

/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame (void)
{
	int i;
	mbrush38_leaf_t	*leaf;

	tr.viewCount++;

// current viewcluster
	if (!(tr.refdef.rdflags & RDF_NOWORLDMODEL))
	{
		r_oldviewcluster = r_viewcluster;
		r_oldviewcluster2 = r_viewcluster2;
		leaf = Mod_PointInLeaf (tr.refdef.vieworg, tr.worldModel);
		r_viewcluster = r_viewcluster2 = leaf->cluster;

		// check above and below so crossing solid water doesn't draw wrong
		if (!leaf->contents)
		{	// look down a bit
			vec3_t	temp;

			VectorCopy (tr.refdef.vieworg, temp);
			temp[2] -= 16;
			leaf = Mod_PointInLeaf (temp, tr.worldModel);
			if ( !(leaf->contents & BSP38CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
		else
		{	// look up a bit
			vec3_t	temp;

			VectorCopy (tr.refdef.vieworg, temp);
			temp[2] += 16;
			leaf = Mod_PointInLeaf (temp, tr.worldModel);
			if ( !(leaf->contents & BSP38CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
	}

	c_brush_polys = 0;
	c_alias_polys = 0;

	// clear out the portion of the screen that the NOWORLDMODEL defines
	if (tr.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		qglEnable( GL_SCISSOR_TEST );
		qglClearColor( 0.3, 0.3, 0.3, 1 );
		qglScissor( tr.refdef.x, glConfig.vidHeight - tr.refdef.height - tr.refdef.y, tr.refdef.width, tr.refdef.height );
		qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		qglClearColor( 1, 0, 0.5, 0.5 );
		qglDisable( GL_SCISSOR_TEST );
	}
}


void MYgluPerspective( GLdouble fovy, GLdouble aspect,
		     GLdouble zNear, GLdouble zFar )
{
   GLdouble xmin, xmax, ymin, ymax;

   ymax = zNear * tan( fovy * M_PI / 360.0 );
   ymin = -ymax;

   xmin = ymin * aspect;
   xmax = ymax * aspect;

   xmin += -( 2 * gl_state.camera_separation ) / zNear;
   xmax += -( 2 * gl_state.camera_separation ) / zNear;

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
//	float	yfov;
	int		x, x2, y2, y, w, h;

	//
	// set up viewport
	//
	x = floor((double)tr.refdef.x);
	x2 = ceil((double)(tr.refdef.x + tr.refdef.width));
	y = floor((double)glConfig.vidHeight - tr.refdef.y);
	y2 = ceil((double)glConfig.vidHeight - (tr.refdef.y + tr.refdef.height));

	w = x2 - x;
	h = y - y2;

	qglViewport (x, y2, w, h);

	//
	// set up projection matrix
	//
    screenaspect = (float)tr.refdef.width/tr.refdef.height;
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity ();
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
    qglTranslatef(-tr.refdef.vieworg[0],  -tr.refdef.vieworg[1],  -tr.refdef.vieworg[2]);

//	if ( gl_state.camera_separation != 0 && glConfig.stereoEnabled )
//		qglTranslatef ( gl_state.camera_separation, 0, 0 );

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

void R_Flash( void )
{
	R_PolyBlend ();
}

/*
================
R_RenderView

r_newrefdef must be set before the first call
================
*/
void R_RenderView (refdef_t *fd)
{
	if (r_norefresh->value)
		return;

	tr.sceneCount++;
	tr.frameSceneNum = 0;
	tr.frameSceneNum++;

	R_CommonRenderScene(fd);

	tr.refdef.num_entities = r_numentities;
	tr.refdef.entities = r_entities;
	tr.refdef.num_dlights = r_numdlights;
	tr.refdef.dlights = r_dlights;
	tr.refdef.lightstyles = r_lightstyles;
	tr.refdef.num_particles = r_numparticles;
	tr.refdef.particles = r_particles;
	r_newrefdef = *fd;

	if (!tr.worldModel && !(tr.refdef.rdflags & RDF_NOWORLDMODEL))
		ri.Sys_Error (ERR_DROP, "R_RenderView: NULL worldmodel");

	if (r_speeds->value)
	{
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	R_PushDlights ();

	if (gl_finish->value)
		qglFinish ();

	R_SetupFrame ();

	R_SetFrustum ();

	R_SetupGL ();

	R_MarkLeaves ();	// done here so we know if we're in water

	R_DrawWorld ();

	R_DrawEntitiesOnList ();

	R_DrawParticles ();

	R_DrawAlphaSurfaces ();

	R_Flash();

	if (r_speeds->value)
	{
		ri.Con_Printf (PRINT_ALL, "%4i wpoly %4i epoly %i tex %i lmaps\n",
			c_brush_polys, 
			c_alias_polys, 
			c_visible_textures, 
			c_visible_lightmaps); 
	}
}


void	R_SetGL2D (void)
{
	// set 2D virtual screen size
	qglViewport (0,0, glConfig.vidWidth, glConfig.vidHeight);
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity ();
	qglOrtho  (0, glConfig.vidWidth, glConfig.vidHeight, 0, -99999, 99999);
	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity ();
	qglDisable (GL_CULL_FACE);
	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);
	qglColor4f (1,1,1,1);
}

/*
====================
R_SetLightLevel

====================
*/
void R_SetLightLevel (void)
{
	vec3_t		shadelight;

	if (tr.refdef.rdflags & RDF_NOWORLDMODEL)
		return;

	// save off light value for server to look at (BIG HACK!)

	R_LightPoint (tr.refdef.vieworg, shadelight);

	// pick the greatest component, which should be the same
	// as the mono value returned by software
	if (shadelight[0] > shadelight[1])
	{
		if (shadelight[0] > shadelight[2])
			r_lightlevel->value = 150*shadelight[0];
		else
			r_lightlevel->value = 150*shadelight[2];
	}
	else
	{
		if (shadelight[1] > shadelight[2])
			r_lightlevel->value = 150*shadelight[1];
		else
			r_lightlevel->value = 150*shadelight[2];
	}

}

/*
@@@@@@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@@@@@@
*/
void R_RenderFrame (refdef_t *fd)
{
	R_RenderView( fd );
	R_SetLightLevel ();
	R_SetGL2D ();
}


void R_Register( void )
{
	R_SharedRegister();
	r_lefthand = Cvar_Get( "hand", "0", CVAR_USERINFO | CVAR_ARCHIVE );
	r_norefresh = Cvar_Get ("r_norefresh", "0", 0);
	r_drawentities = Cvar_Get ("r_drawentities", "1", 0);
	r_drawworld = Cvar_Get ("r_drawworld", "1", 0);
	r_novis = Cvar_Get ("r_novis", "0", 0);
	r_nocull = Cvar_Get ("r_nocull", "0", 0);
	r_lerpmodels = Cvar_Get ("r_lerpmodels", "1", 0);
	r_speeds = Cvar_Get ("r_speeds", "0", 0);

	r_lightlevel = Cvar_Get ("r_lightlevel", "0", 0);

	gl_nosubimage = Cvar_Get( "gl_nosubimage", "0", 0 );

	gl_particle_min_size = Cvar_Get( "gl_particle_min_size", "2", CVAR_ARCHIVE );
	gl_particle_max_size = Cvar_Get( "gl_particle_max_size", "40", CVAR_ARCHIVE );
	gl_particle_size = Cvar_Get( "gl_particle_size", "40", CVAR_ARCHIVE );
	gl_particle_att_a = Cvar_Get( "gl_particle_att_a", "0.01", CVAR_ARCHIVE );
	gl_particle_att_b = Cvar_Get( "gl_particle_att_b", "0.0", CVAR_ARCHIVE );
	gl_particle_att_c = Cvar_Get( "gl_particle_att_c", "0.01", CVAR_ARCHIVE );

	gl_modulate = Cvar_Get ("gl_modulate", "1", CVAR_ARCHIVE );
	gl_shadows = Cvar_Get ("gl_shadows", "0", CVAR_ARCHIVE );
	gl_dynamic = Cvar_Get ("gl_dynamic", "1", 0);
	gl_skymip = Cvar_Get ("gl_skymip", "0", 0);
	gl_showtris = Cvar_Get ("gl_showtris", "0", 0);
	gl_finish = Cvar_Get ("gl_finish", "0", CVAR_ARCHIVE);
	gl_clear = Cvar_Get ("gl_clear", "0", 0);
	gl_cull = Cvar_Get ("gl_cull", "1", 0);
	gl_polyblend = Cvar_Get ("gl_polyblend", "1", 0);
	gl_driver = Cvar_Get( "gl_driver", "opengl32", CVAR_ARCHIVE );
	gl_lockpvs = Cvar_Get( "gl_lockpvs", "0", 0 );

	gl_vertex_arrays = Cvar_Get( "gl_vertex_arrays", "0", CVAR_ARCHIVE );

	gl_drawbuffer = Cvar_Get( "gl_drawbuffer", "GL_BACK", 0 );

	gl_saturatelighting = Cvar_Get( "gl_saturatelighting", "0", 0 );

	vid_ref = Cvar_Get( "vid_ref", "soft", CVAR_ARCHIVE );

	Cmd_AddCommand( "screenshot", GL_ScreenShot_f );
	Cmd_AddCommand( "modellist", Mod_Modellist_f );
	Cmd_AddCommand( "gfxinfo", CommonGfxInfo_f);
}

trGlobals_base_t	_tr;

/*
===============
R_Init
===============
*/
int R_Init()
{	
	int		err;
	int		j;
	extern float r_turbsin[256];

	for ( j = 0; j < 256; j++ )
	{
		r_turbsin[j] *= 0.5;
	}

	ri.Con_Printf (PRINT_ALL, "ref_gl version: "REF_VERSION"\n");

	tr_shared = &_tr;
	R_Register();

	// destroy the existing window
	GLimp_Shutdown ();

	r_fullscreen->modified = false;
	r_mode->modified = false;

	R_CommonInit();

	// let the sound and input subsystems know about the new window
	VID_NewWindow(glConfig.vidWidth, glConfig.vidHeight);

	CommonGfxInfo_f();

	Cvar_SetLatched( "scr_drawall", "0" );

	GL_SetDefaultState();

	R_InitFunctionTables();
	R_InitFogTable();
	R_NoiseInit();
	R_InitImages();
	Mod_Init ();
	R_InitParticleTexture ();
	Draw_InitLocal ();

	err = qglGetError();
	if ( err != GL_NO_ERROR )
		ri.Con_Printf (PRINT_ALL, "glGetError() = 0x%x\n", err);
	return 0;
}

/*
===============
R_Shutdown
===============
*/
void R_Shutdown (void)
{	
	Cmd_RemoveCommand ("modellist");
	Cmd_RemoveCommand ("screenshot");
	Cmd_RemoveCommand ("imagelist");
	Cmd_RemoveCommand ("gfxinfo");

	R_FreeModels();

	R_DeleteTextures();

	/*
	** shut down OS specific OpenGL stuff like contexts, etc.
	*/
	GLimp_Shutdown();

	/*
	** shutdown our QGL subsystem
	*/
	QGL_Shutdown();
}



/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginFrame
@@@@@@@@@@@@@@@@@@@@@
*/
void R_BeginFrame( float camera_separation )
{
	tr.frameCount++;

	gl_state.camera_separation = camera_separation;

	/*
	** change modes if necessary
	*/
	if ( r_mode->modified || r_fullscreen->modified )
	{	// FIXME: only restart if CDS is required
		QCvar	*ref;

		ref = Cvar_Get ("vid_ref", "gl", 0);
		ref->modified = true;
	}

	QGL_EnableLogging(!!r_logFile->integer);

	QGL_LogComment("*** R_BeginFrame ***\n");

	GLimp_BeginFrame( camera_separation );

	/*
	** go into 2D mode
	*/
	qglViewport (0,0, glConfig.vidWidth, glConfig.vidHeight);
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity ();
	qglOrtho  (0, glConfig.vidWidth, glConfig.vidHeight, 0, -99999, 99999);
	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity ();
	qglDisable (GL_CULL_FACE);
	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);
	qglColor4f (1,1,1,1);

	/*
	** draw buffer stuff
	*/
	if ( gl_drawbuffer->modified )
	{
		gl_drawbuffer->modified = false;

		if ( gl_state.camera_separation == 0 || !glConfig.stereoEnabled )
		{
			if ( QStr::ICmp( gl_drawbuffer->string, "GL_FRONT" ) == 0 )
				qglDrawBuffer( GL_FRONT );
			else
				qglDrawBuffer( GL_BACK );
		}
	}

	/*
	** texturemode stuff
	*/
	if ( r_textureMode->modified )
	{
		GL_TextureMode( r_textureMode->string );
		r_textureMode->modified = false;
	}

	/*
	** swapinterval stuff
	*/
	GL_UpdateSwapInterval();

	//
	// clear screen if desired
	//
	R_Clear ();
}

/*
** R_DrawBeam
*/
void R_DrawBeam( trRefEntity_t *e )
{
#define NUM_BEAM_SEGS 6

	int	i;
	float r, g, b;

	vec3_t perpvec;
	vec3_t direction, normalized_direction;
	vec3_t	start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	vec3_t oldorigin, origin;

	oldorigin[0] = e->e.oldorigin[0];
	oldorigin[1] = e->e.oldorigin[1];
	oldorigin[2] = e->e.oldorigin[2];

	origin[0] = e->e.origin[0];
	origin[1] = e->e.origin[1];
	origin[2] = e->e.origin[2];

	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if ( VectorNormalize( normalized_direction ) == 0 )
		return;

	PerpendicularVector( perpvec, normalized_direction );
	VectorScale( perpvec, e->e.frame / 2, perpvec );

	for ( i = 0; i < 6; i++ )
	{
		RotatePointAroundVector( start_points[i], normalized_direction, perpvec, (360.0/NUM_BEAM_SEGS)*i );
		VectorAdd( start_points[i], origin, start_points[i] );
		VectorAdd( start_points[i], direction, end_points[i] );
	}

	qglDisable( GL_TEXTURE_2D );
	GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);

	r = ( d_8to24table[e->e.skinNum & 0xFF] ) & 0xFF;
	g = ( d_8to24table[e->e.skinNum & 0xFF] >> 8 ) & 0xFF;
	b = ( d_8to24table[e->e.skinNum & 0xFF] >> 16 ) & 0xFF;

	r *= 1/255.0F;
	g *= 1/255.0F;
	b *= 1/255.0F;

	qglColor4f( r, g, b, e->e.shaderRGBA[3] / 255.0 );

	qglBegin( GL_TRIANGLE_STRIP );
	for ( i = 0; i < NUM_BEAM_SEGS; i++ )
	{
		qglVertex3fv( start_points[i] );
		qglVertex3fv( end_points[i] );
		qglVertex3fv( start_points[(i+1)%NUM_BEAM_SEGS] );
		qglVertex3fv( end_points[(i+1)%NUM_BEAM_SEGS] );
	}
	qglEnd();

	qglEnable( GL_TEXTURE_2D );
	GL_State(GLS_DEPTHMASK_TRUE);
}

//===================================================================


void	R_BeginRegistration (char *map);
image_t	*R_RegisterSkin (char *name);
void R_SetSky (char *name, float rotate, vec3_t axis);
void	R_EndRegistration (void);

void	R_RenderFrame (refdef_t *fd);

image_t	*Draw_FindPic (char *name);

void	Draw_Pic (int x, int y, char *name);
void	Draw_Char (int x, int y, int c);
void	Draw_TileClear (int x, int y, int w, int h, char *name);
void	Draw_Fill (int x, int y, int w, int h, int c);
void	Draw_FadeScreen (void);

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
refexport_t GetRefAPI (refimport_t rimp )
{
	refexport_t	re;

	ri = rimp;

	re.BeginRegistration = R_BeginRegistration;
	re.RegisterSkin = R_RegisterSkin;
	re.RegisterPic = Draw_FindPic;
	re.SetSky = R_SetSky;
	re.EndRegistration = R_EndRegistration;

	re.RenderFrame = R_RenderFrame;

	re.DrawGetPicSize = Draw_GetPicSize;
	re.DrawPic = Draw_Pic;
	re.DrawStretchPic = Draw_StretchPic;
	re.DrawChar = Draw_Char;
	re.DrawTileClear = Draw_TileClear;
	re.DrawFill = Draw_Fill;
	re.DrawFadeScreen= Draw_FadeScreen;

	re.DrawStretchRaw = Draw_StretchRaw;

	re.Shutdown = R_Shutdown;

	re.BeginFrame = R_BeginFrame;
	re.EndFrame = GLimp_EndFrame;

	return re;
}

void R_ClearScreen()
{
	qglClearColor(0, 0, 0, 0);
	qglClear(GL_COLOR_BUFFER_BIT);
	qglClearColor(1,0, 0.5, 0.5);
}

void R_InitSkyTexCoords( float cloudLayerHeight )
{
}
shader_t* R_FindShader( const char *name, int lightmapIndex, qboolean mipRawImage )
{
	return NULL;
}
void R_InitSky(mbrush29_texture_t *mt)
{
}
void R_SyncRenderThread()
{
}
void    R_RemapShader(const char *oldShader, const char *newShader, const char *timeOffset)
{
}
