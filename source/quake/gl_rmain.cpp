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

image_t*	playertextures[16];		// up to 16 color translated skins

//
// screen size info
//
refdef_t	r_refdef;

int		d_lightstylevalue[256];	// 8.8 fraction of base light value

QCvar*	r_norefresh;
QCvar*	r_drawviewmodel;

QCvar*	gl_cull;
QCvar*	gl_polyblend;
QCvar*	gl_nocolors;
QCvar*	gl_reporttjunctions;
QCvar*	gl_doubleeyes;

/*
=============================================================

  ALIAS MODELS

=============================================================
*/

void R_HandleRefEntColormap(refEntity_t* Ent, int ColorMap)
{
	// we can't dynamically colormap textures, so they are cached
	// seperately for the players.  Heads are just uncolored.
	if (ColorMap && !gl_nocolors->value && !QStr::Cmp(R_GetModelByHandle(Ent->hModel)->name, "progs/player.mdl"))
	{
	    Ent->customSkin = R_GetImageHandle(playertextures[ColorMap - 1]);
	}
}

//==================================================================================

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

	qglVertex3f(10, 100, 100);
	qglVertex3f(10, -100, 100);
	qglVertex3f(10, -100, -100);
	qglVertex3f(10, 100, -100);
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

	tr.viewParms.viewportX = x;
	tr.viewParms.viewportY = y2; 
	tr.viewParms.viewportWidth = w;
	tr.viewParms.viewportHeight = h;
	R_SetupProjection();
	R_RotateForViewer();

	backEnd.viewParms = tr.viewParms;
	RB_BeginDrawingView();

	qglLoadMatrixf(tr.viewParms.world.modelMatrix);

	//
	// set drawing parms
	//
	if (gl_cull->value)
	{
		GL_Cull(CT_FRONT_SIDED);
	}
	else
	{
		GL_Cull(CT_TWO_SIDED);
	}
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

	R_GenerateDrawSurfs();
}


/*
=============
R_Clear
=============
*/
void R_Clear (void)
{
	if (r_clear->value)
	{
		qglClearColor(1, 0, 0, 0);
		qglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
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

	for (int i = 0; i < MAX_LIGHTSTYLES_Q1; i++)
	{
		float Val = d_lightstylevalue[i] / 256.0;
		R_AddLightStyleToScene(i, Val, Val, Val);
	}
	CL_AddParticles();

	tr.frameCount++;
	tr.frameSceneNum = 0;

	glState.finishCalled = false;

	if (!tr.worldModel)
		Sys_Error ("R_RenderView: NULL worldmodel");

	if (r_speeds->value)
	{
		time1 = Sys_DoubleTime ();
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	QGL_EnableLogging(!!r_logFile->integer);

	r_refdef.time = (int)(cl.time * 1000);
	R_CommonRenderScene(&r_refdef);

	R_PushDlightsQ1 ();

	R_Clear ();

	// render normal view

	R_RenderScene ();

	R_PolyBlend ();

	if (r_speeds->value)
	{
		time2 = Sys_DoubleTime ();
		Con_Printf ("%3i ms  %4i wpoly %4i epoly\n", (int)((time2-time1)*1000), c_brush_polys, c_alias_polys); 
	}
}
