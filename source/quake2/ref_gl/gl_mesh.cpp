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
// gl_mesh.c: triangle model functions

#include "gl_local.h"

/*
=============================================================

  ALIAS MODELS

=============================================================
*/

typedef float vec4_t[4];

static	vec4_t	s_lerped[MAX_MD2_VERTS];
//static	vec3_t	lerped[MAX_MD2_VERTS];

vec3_t	shadevector;
float	shadelight[3];

// precalculated dot products for quantized angles
#define SHADEDOT_QUANT 16
float	r_avertexnormal_dots[SHADEDOT_QUANT][256] =
#include "anormtab.h"
;

float	*shadedots = r_avertexnormal_dots[0];

void GL_LerpVerts( int nverts, dmd2_trivertx_t *v, dmd2_trivertx_t *ov, dmd2_trivertx_t *verts, float *lerp, float move[3], float frontv[3], float backv[3] )
{
	int i;

	if (tr.currentEntity->e.renderfx & RF_COLOUR_SHELL)
	{
		for (i=0 ; i < nverts; i++, v++, ov++, lerp+=4 )
		{
			float *normal = bytedirs[verts[i].lightnormalindex];

			lerp[0] = move[0] + ov->v[0]*backv[0] + v->v[0]*frontv[0] + normal[0] * POWERSUIT_SCALE;
			lerp[1] = move[1] + ov->v[1]*backv[1] + v->v[1]*frontv[1] + normal[1] * POWERSUIT_SCALE;
			lerp[2] = move[2] + ov->v[2]*backv[2] + v->v[2]*frontv[2] + normal[2] * POWERSUIT_SCALE; 
		}
	}
	else
	{
		for (i=0 ; i < nverts; i++, v++, ov++, lerp+=4)
		{
			lerp[0] = move[0] + ov->v[0]*backv[0] + v->v[0]*frontv[0];
			lerp[1] = move[1] + ov->v[1]*backv[1] + v->v[1]*frontv[1];
			lerp[2] = move[2] + ov->v[2]*backv[2] + v->v[2]*frontv[2];
		}
	}

}

/*
=============
GL_DrawAliasFrameLerp

interpolates between two frames and origins
FIXME: batch lerp all vertexes
=============
*/
void GL_DrawAliasFrameLerp (dmd2_t *paliashdr, float backlerp)
{
	float 	l;
	dmd2_frame_t	*frame, *oldframe;
	dmd2_trivertx_t	*v, *ov, *verts;
	int		*order;
	int		count;
	float	frontlerp;
	float	alpha;
	vec3_t	move, delta;
	vec3_t	frontv, backv;
	int		i;
	int		index_xyz;
	float	*lerp;

	frame = (dmd2_frame_t *)((byte *)paliashdr + paliashdr->ofs_frames 
		+ tr.currentEntity->e.frame * paliashdr->framesize);
	verts = v = frame->verts;

	oldframe = (dmd2_frame_t *)((byte *)paliashdr + paliashdr->ofs_frames 
		+ tr.currentEntity->e.oldframe * paliashdr->framesize);
	ov = oldframe->verts;

	order = (int *)((byte *)paliashdr + paliashdr->ofs_glcmds);

//	qglTranslatef (frame->translate[0], frame->translate[1], frame->translate[2]);
//	qglScalef (frame->scale[0], frame->scale[1], frame->scale[2]);

	if (tr.currentEntity->e.renderfx & RF_TRANSLUCENT)
		alpha = tr.currentEntity->e.shaderRGBA[3] / 255.0;
	else
		alpha = 1.0;

	if (tr.currentEntity->e.renderfx & RF_COLOUR_SHELL)
		qglDisable( GL_TEXTURE_2D );

	frontlerp = 1.0 - backlerp;

	// move should be the delta back to the previous frame * backlerp
	VectorSubtract (tr.currentEntity->e.oldorigin, tr.currentEntity->e.origin, delta);

	move[0] = DotProduct(delta, tr.currentEntity->e.axis[0]);	// forward
	move[1] = DotProduct(delta, tr.currentEntity->e.axis[1]);	// left
	move[2] = DotProduct(delta, tr.currentEntity->e.axis[2]);	// up

	VectorAdd (move, oldframe->translate, move);

	for (i=0 ; i<3 ; i++)
	{
		move[i] = backlerp*move[i] + frontlerp*frame->translate[i];
	}

	for (i=0 ; i<3 ; i++)
	{
		frontv[i] = frontlerp*frame->scale[i];
		backv[i] = backlerp*oldframe->scale[i];
	}

	lerp = s_lerped[0];

	GL_LerpVerts( paliashdr->num_xyz, v, ov, verts, lerp, move, frontv, backv );

	if ( gl_vertex_arrays->value )
	{
		float colorArray[MAX_MD2_VERTS*4];

		qglEnableClientState( GL_VERTEX_ARRAY );
		qglVertexPointer( 3, GL_FLOAT, 16, s_lerped );	// padded for SIMD

		if (tr.currentEntity->e.renderfx & RF_COLOUR_SHELL)
		{
			qglColor4f( shadelight[0], shadelight[1], shadelight[2], alpha );
		}
		else
		{
			qglEnableClientState( GL_COLOR_ARRAY );
			qglColorPointer( 3, GL_FLOAT, 0, colorArray );

			//
			// pre light everything
			//
			for ( i = 0; i < paliashdr->num_xyz; i++ )
			{
				float l = shadedots[verts[i].lightnormalindex];

				colorArray[i*3+0] = l * shadelight[0];
				colorArray[i*3+1] = l * shadelight[1];
				colorArray[i*3+2] = l * shadelight[2];
			}
		}

		if ( qglLockArraysEXT != 0 )
			qglLockArraysEXT( 0, paliashdr->num_xyz );

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
			{
				qglBegin (GL_TRIANGLE_STRIP);
			}

			if (tr.currentEntity->e.renderfx & RF_COLOUR_SHELL)
			{
				do
				{
					index_xyz = order[2];
					order += 3;

					qglVertex3fv( s_lerped[index_xyz] );

				} while (--count);
			}
			else
			{
				do
				{
					// texture coordinates come from the draw list
					qglTexCoord2f (((float *)order)[0], ((float *)order)[1]);
					index_xyz = order[2];

					order += 3;

					// normals and vertexes come from the frame list
//					l = shadedots[verts[index_xyz].lightnormalindex];
					
//					qglColor4f (l* shadelight[0], l*shadelight[1], l*shadelight[2], alpha);
					qglArrayElement( index_xyz );

				} while (--count);
			}
			qglEnd ();
		}

		if ( qglUnlockArraysEXT != 0 )
			qglUnlockArraysEXT();
	}
	else
	{
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
			{
				qglBegin (GL_TRIANGLE_STRIP);
			}

			if (tr.currentEntity->e.renderfx & RF_COLOUR_SHELL)
			{
				do
				{
					index_xyz = order[2];
					order += 3;

					qglColor4f( shadelight[0], shadelight[1], shadelight[2], alpha);
					qglVertex3fv (s_lerped[index_xyz]);

				} while (--count);
			}
			else
			{
				do
				{
					// texture coordinates come from the draw list
					qglTexCoord2f (((float *)order)[0], ((float *)order)[1]);
					index_xyz = order[2];
					order += 3;

					// normals and vertexes come from the frame list
					l = shadedots[verts[index_xyz].lightnormalindex];
					
					qglColor4f (l* shadelight[0], l*shadelight[1], l*shadelight[2], alpha);
					qglVertex3fv (s_lerped[index_xyz]);
				} while (--count);
			}

			qglEnd ();
		}
	}

	if (tr.currentEntity->e.renderfx & RF_COLOUR_SHELL)
		qglEnable( GL_TEXTURE_2D );
}


#if 1
/*
=============
GL_DrawAliasShadow
=============
*/
extern	vec3_t			lightspot;

void GL_DrawAliasShadow (dmd2_t *paliashdr, int posenum)
{
	dmd2_trivertx_t	*verts;
	int		*order;
	vec3_t	point;
	float	height, lheight;
	int		count;
	dmd2_frame_t	*frame;

	lheight = tr.currentEntity->e.origin[2] - lightspot[2];

	frame = (dmd2_frame_t *)((byte *)paliashdr + paliashdr->ofs_frames 
		+ tr.currentEntity->e.frame * paliashdr->framesize);
	verts = frame->verts;

	height = 0;

	order = (int *)((byte *)paliashdr + paliashdr->ofs_glcmds);

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
			// normals and vertexes come from the frame list
/*
			point[0] = verts[order[2]].v[0] * frame->scale[0] + frame->translate[0];
			point[1] = verts[order[2]].v[1] * frame->scale[1] + frame->translate[1];
			point[2] = verts[order[2]].v[2] * frame->scale[2] + frame->translate[2];
*/

			Com_Memcpy( point, s_lerped[order[2]], sizeof( point )  );

			point[0] -= shadevector[0]*(point[2]+lheight);
			point[1] -= shadevector[1]*(point[2]+lheight);
			point[2] = height;
//			height -= 0.001;
			qglVertex3fv (point);

			order += 3;

//			verts++;

		} while (--count);

		qglEnd ();
	}	
}

#endif

/*
** R_CullAliasModel
*/
static qboolean R_CullAliasModel( vec3_t bbox[8], trRefEntity_t *e )
{
	dmd2_t* paliashdr = (dmd2_t*)tr.currentModel->q2_extradata;

	if ((e->e.frame >= paliashdr->num_frames) || (e->e.frame < 0))
	{
		GLog.Write("R_CullAliasModel %s: no such frame %d\n", 
			tr.currentModel->name, e->e.frame);
		e->e.frame = 0;
	}
	if ((e->e.oldframe >= paliashdr->num_frames) || (e->e.oldframe < 0))
	{
		GLog.Write("R_CullAliasModel %s: no such oldframe %d\n", 
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

/*
=================
R_DrawAliasModel

=================
*/
void R_DrawAliasModel (trRefEntity_t *e)
{
	int			i;
	dmd2_t		*paliashdr;
	vec3_t		bbox[8];
	image_t		*skin;

	R_RotateForEntity(tr.currentEntity, &tr.viewParms, &tr.orient);

	if ( !( e->e.renderfx & RF_FIRST_PERSON) )
	{
		if ( R_CullAliasModel( bbox, e ) )
			return;
	}

	if ( e->e.renderfx & RF_FIRST_PERSON)
	{
		if ( r_lefthand->value == 2 )
			return;
	}

	paliashdr = (dmd2_t *)tr.currentModel->q2_extradata;

	//
	// get lighting information
	//
	if (tr.currentEntity->e.renderfx & RF_COLOUR_SHELL)
	{
		for (i = 0; i < 3; i++)
			shadelight[i] = tr.currentEntity->e.shaderRGBA[i] / 255.0;
	}
	else if (tr.currentEntity->e.renderfx & RF_ABSOLUTE_LIGHT)
	{
		for (i=0 ; i<3 ; i++)
			shadelight[i] = e->e.radius;
	}
	else
	{
		R_LightPoint (tr.currentEntity->e.origin, shadelight);

		// player lighting hack for communication back to server
		// big hack!
		if ( tr.currentEntity->e.renderfx & RF_FIRST_PERSON)
		{
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
	}

	if ( tr.currentEntity->e.renderfx & RF_MINLIGHT )
	{
		for (i=0 ; i<3 ; i++)
			if (shadelight[i] > 0.1)
				break;
		if (i == 3)
		{
			shadelight[0] = 0.1;
			shadelight[1] = 0.1;
			shadelight[2] = 0.1;
		}
	}

	if (tr.currentEntity->e.renderfx & RF_GLOW)
	{
		// bonus items will pulse with time
		float	scale;
		float	min;

		scale = 0.1 * sin(tr.refdef.floatTime * 7);
		for (i=0 ; i<3 ; i++)
		{
			min = shadelight[i] * 0.8;
			shadelight[i] += scale;
			if (shadelight[i] < min)
				shadelight[i] = min;
		}
	}

// =================
// PGM	ir goggles color override
	if (tr.refdef.rdflags & RDF_IRGOGGLES && tr.currentEntity->e.renderfx & RF_IR_VISIBLE)
	{
		shadelight[0] = 1.0;
		shadelight[1] = 0.0;
		shadelight[2] = 0.0;
	}
// PGM	
// =================

	vec3_t tmp_angles;
	VecToAngles(e->e.axis[0], tmp_angles);
	shadedots = r_avertexnormal_dots[((int)(tmp_angles[1] * (SHADEDOT_QUANT / 360.0))) & (SHADEDOT_QUANT - 1)];
	
	VectorCopy(e->e.axis[0], shadevector);
	shadevector[2] = 1;
	VectorNormalize (shadevector);

	//
	// locate the proper data
	//

	c_alias_polys += paliashdr->num_tris;

	//
	// draw all the triangles
	//
	if (tr.currentEntity->e.renderfx & RF_DEPTHHACK) // hack the depth range to prevent view model from poking into walls
		qglDepthRange (gldepthmin, gldepthmin + 0.3*(gldepthmax-gldepthmin));

	if ( ( tr.currentEntity->e.renderfx & RF_FIRST_PERSON) && ( r_lefthand->value == 1.0F ) )
	{
		qglMatrixMode( GL_PROJECTION );
		qglPushMatrix();
		qglLoadIdentity();
		qglScalef( -1, 1, 1 );
		qglMultMatrixf(tr.viewParms.projectionMatrix);
		qglMatrixMode( GL_MODELVIEW );

		qglCullFace( GL_BACK );
	}

    qglPushMatrix ();
	qglLoadMatrixf(tr.orient.modelMatrix);

	// select skin
	if (tr.currentEntity->e.customSkin)
		skin = tr.images[tr.currentEntity->e.customSkin];	// custom player skin
	else
	{
		if (tr.currentEntity->e.skinNum >= MAX_MD2_SKINS)
			skin = tr.currentModel->q2_skins[0];
		else
		{
			skin = tr.currentModel->q2_skins[tr.currentEntity->e.skinNum];
			if (!skin)
				skin = tr.currentModel->q2_skins[0];
		}
	}
	if (!skin)
		skin = tr.defaultImage;	// fallback...
	GL_Bind(skin);

	// draw it

	qglShadeModel (GL_SMOOTH);

	GL_TexEnv( GL_MODULATE );
	if ( tr.currentEntity->e.renderfx & RF_TRANSLUCENT )
	{
		GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	}
	else
	{
		GL_State(GLS_DEFAULT);
	}


	if ( (tr.currentEntity->e.frame >= paliashdr->num_frames) 
		|| (tr.currentEntity->e.frame < 0) )
	{
		ri.Con_Printf (PRINT_ALL, "R_DrawAliasModel %s: no such frame %d\n",
			tr.currentModel->name, tr.currentEntity->e.frame);
		tr.currentEntity->e.frame = 0;
		tr.currentEntity->e.oldframe = 0;
	}

	if ( (tr.currentEntity->e.oldframe >= paliashdr->num_frames)
		|| (tr.currentEntity->e.oldframe < 0))
	{
		ri.Con_Printf (PRINT_ALL, "R_DrawAliasModel %s: no such oldframe %d\n",
			tr.currentModel->name, tr.currentEntity->e.oldframe);
		tr.currentEntity->e.frame = 0;
		tr.currentEntity->e.oldframe = 0;
	}

	if ( !r_lerpmodels->value )
		tr.currentEntity->e.backlerp = 0;
	GL_DrawAliasFrameLerp (paliashdr, tr.currentEntity->e.backlerp);

	GL_TexEnv( GL_REPLACE );
	qglShadeModel (GL_FLAT);

	qglPopMatrix ();

	if ( ( tr.currentEntity->e.renderfx & RF_FIRST_PERSON) && ( r_lefthand->value == 1.0F ) )
	{
		qglMatrixMode( GL_PROJECTION );
		qglPopMatrix();
		qglMatrixMode( GL_MODELVIEW );
		qglCullFace( GL_FRONT );
	}

	if ( tr.currentEntity->e.renderfx & RF_TRANSLUCENT )
	{
		GL_State(GLS_DEFAULT);
	}

	if (tr.currentEntity->e.renderfx & RF_DEPTHHACK)
		qglDepthRange (gldepthmin, gldepthmax);

#if 1
	if (gl_shadows->value && !(tr.currentEntity->e.renderfx & (RF_TRANSLUCENT | RF_FIRST_PERSON)))
	{
		qglPushMatrix ();
		qglLoadMatrixf(tr.orient.modelMatrix);
		qglDisable (GL_TEXTURE_2D);
		GL_State(GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		qglColor4f (0,0,0,0.5);
		GL_DrawAliasShadow (paliashdr, tr.currentEntity->e.frame );
		qglEnable (GL_TEXTURE_2D);
		GL_State(GLS_DEFAULT);
		qglPopMatrix ();
	}
#endif
	qglColor4f (1,1,1,1);
}


