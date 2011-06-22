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
		if ( R_CullMd2Model( bbox, e ) )
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
			md2_shadelight[i] = tr.currentEntity->e.shaderRGBA[i] / 255.0;
	}
	else if (tr.currentEntity->e.renderfx & RF_ABSOLUTE_LIGHT)
	{
		for (i=0 ; i<3 ; i++)
			md2_shadelight[i] = e->e.radius;
	}
	else
	{
		R_LightPoint (tr.currentEntity->e.origin, md2_shadelight);

		// player lighting hack for communication back to server
		// big hack!
		if ( tr.currentEntity->e.renderfx & RF_FIRST_PERSON)
		{
			// pick the greatest component, which should be the same
			// as the mono value returned by software
			if (md2_shadelight[0] > md2_shadelight[1])
			{
				if (md2_shadelight[0] > md2_shadelight[2])
					r_lightlevel->value = 150*md2_shadelight[0];
				else
					r_lightlevel->value = 150*md2_shadelight[2];
			}
			else
			{
				if (md2_shadelight[1] > md2_shadelight[2])
					r_lightlevel->value = 150*md2_shadelight[1];
				else
					r_lightlevel->value = 150*md2_shadelight[2];
			}

		}
	}

	if ( tr.currentEntity->e.renderfx & RF_MINLIGHT )
	{
		for (i=0 ; i<3 ; i++)
			if (md2_shadelight[i] > 0.1)
				break;
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
		float	scale;
		float	min;

		scale = 0.1 * sin(tr.refdef.floatTime * 7);
		for (i=0 ; i<3 ; i++)
		{
			min = md2_shadelight[i] * 0.8;
			md2_shadelight[i] += scale;
			if (md2_shadelight[i] < min)
				md2_shadelight[i] = min;
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
	GL_DrawMd2FrameLerp (paliashdr, tr.currentEntity->e.backlerp);

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
		GL_DrawMd2Shadow (paliashdr, tr.currentEntity->e.frame );
		qglEnable (GL_TEXTURE_2D);
		GL_State(GLS_DEFAULT);
		qglPopMatrix ();
	}
#endif
	qglColor4f (1,1,1,1);
}


