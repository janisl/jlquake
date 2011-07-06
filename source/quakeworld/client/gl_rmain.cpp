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

int			c_brush_polys;

qboolean	envmap;				// true during envmap command capture 

image_t*	particletexture;	// little dot for particles
image_t*	playertextures[MAX_CLIENTS];		// up to 16 color translated skins

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
QCvar*	r_dynamic;
QCvar*	r_novis;
QCvar*	r_netgraph;

QCvar*	gl_cull;
QCvar*	gl_polyblend;
QCvar*	gl_nocolors;
QCvar*	gl_reporttjunctions;
QCvar*	gl_finish;

extern	QCvar*	scr_fov;

/*
=============================================================

  ALIAS MODELS

=============================================================
*/

void R_HandlePlayerSkin(refEntity_t* Ent, int PlayerNum)
{
	// we can't dynamically colormap textures, so they are cached
	// seperately for the players.  Heads are just uncolored.
	if (!gl_nocolors->value)
	{
		if (!cl.players[PlayerNum].skin)
		{
			Skin_Find(&cl.players[PlayerNum]);
			R_TranslatePlayerSkin(PlayerNum);
		}
		Ent->customSkin = R_GetImageHandle(playertextures[PlayerNum]);
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
	for (i=0 ; i<tr.refdef.num_entities; i++)
	{
		tr.currentEntity = &tr.refdef.entities[i];

		if (tr.currentEntity->e.renderfx & RF_FIRST_PERSON)
		{
			if (!r_drawviewmodel->value || envmap)
			{
				continue;
			}
		}
		if (tr.currentEntity->e.renderfx & RF_THIRD_PERSON)
		{
			continue;
		}

		switch (R_GetModelByHandle(tr.currentEntity->e.hModel)->type)
		{
		case MOD_MESH1:
			R_DrawMdlModel(tr.currentEntity);
			break;

		case MOD_BRUSH29:
			R_DrawBrushModel(tr.currentEntity);
			break;

		default:
			break;
		}
	}

	for (i=0 ; i<tr.refdef.num_entities; i++)
	{
		tr.currentEntity = &tr.refdef.entities[i];

		switch (R_GetModelByHandle(tr.currentEntity->e.hModel)->type)
		{
		case MOD_SPRITE:
			R_DrawSprModel(tr.currentEntity);
			break;

		default :
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

//Con_Printf("R_PolyBlend(): %4.2f %4.2f %4.2f %4.2f\n",v_blend[0], v_blend[1],	v_blend[2],	v_blend[3]);

	GL_State(GLS_DEFAULT | GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	qglDisable (GL_TEXTURE_2D);

    qglLoadIdentity ();

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
}

/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame (void)
{
// don't allow cheats in multiplayer
	r_fullbright->value = 0;
	r_lightmap->value = 0;
	if (!QStr::Atoi(Info_ValueForKey(cl.serverinfo, "watervis")))
		r_wateralpha->value = 1;

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
	r_viewleaf = Mod_PointInLeafQ1 (tr.viewParms.orient.origin, tr.worldModel);

	V_SetContentsColor (r_viewleaf->contents);
	V_CalcBlend ();

	r_cache_thrash = false;

	c_brush_polys = 0;
	c_alias_polys = 0;

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

	qglMatrixMode(GL_MODELVIEW);

	R_RotateForViewer();
	qglLoadMatrixf(tr.viewParms.world.modelMatrix);

	//
	// set drawing parms
	//
	glState.faceCulling = -1;
	if (gl_cull->value)
	{
		GL_Cull(CT_FRONT_SIDED);
	}
	else
	{
		GL_Cull(CT_TWO_SIDED);
	}

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

	R_DrawParticles ();
}


/*
=============
R_Clear
=============
*/
void R_Clear (void)
{
	if (r_clear->value)
		qglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	else
		qglClear (GL_DEPTH_BUFFER_BIT);

	qglDepthRange(0, 1);
}

/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
void R_RenderView (void)
{
	double	time1 = 0, time2;

	if (r_norefresh->value)
		return;

	for (int i = 0; i < MAX_LIGHTSTYLES_Q1; i++)
	{
		float Val = d_lightstylevalue[i] / 256.0;
		R_AddLightStyleToScene(i, Val, Val, Val);
	}

	tr.frameCount++;
	tr.frameSceneNum = 0;

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

	R_PushDlights ();

	R_Clear ();

	// render normal view
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

void R_SyncRenderThread()
{
}
