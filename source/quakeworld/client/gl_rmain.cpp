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

image_t*	playertextures[MAX_CLIENTS];		// up to 16 color translated skins

//
// screen size info
//
refdef_t	r_refdef;

int		d_lightstylevalue[256];	// 8.8 fraction of base light value

QCvar*	r_drawviewmodel;
QCvar*	r_netgraph;

QCvar*	gl_polyblend;
QCvar*	gl_nocolors;
QCvar*	gl_reporttjunctions;

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

	//
	// texturemode stuff
	//
	if (r_textureMode->modified)
	{
		GL_TextureMode(r_textureMode->string);
		r_textureMode->modified = false;
	}

	V_SetContentsColor(CM_PointContentsQ1(r_refdef.vieworg, 0));
	V_CalcBlend ();

	r_cache_thrash = false;

	c_brush_polys = 0;
	c_alias_polys = 0;

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
	double	time1 = 0, time2;

	for (int i = 0; i < MAX_LIGHTSTYLES_Q1; i++)
	{
		float Val = d_lightstylevalue[i] / 256.0;
		R_AddLightStyleToScene(i, Val, Val, Val);
	}
	CL_AddParticles();

	tr.frameCount++;
	tr.frameSceneNum = 0;

	glState.finishCalled = false;

	if (r_speeds->value)
	{
		time1 = Sys_DoubleTime ();
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	QGL_EnableLogging(!!r_logFile->integer);

	R_Clear ();

	R_SetupFrame ();

	r_refdef.time = (int)(cl.time * 1000);

	R_RenderScene(&r_refdef);

	R_PolyBlend ();

	if (r_speeds->value)
	{
		time2 = Sys_DoubleTime ();
		Con_Printf ("%3i ms  %4i wpoly %4i epoly\n", (int)((time2-time1)*1000), c_brush_polys, c_alias_polys); 
	}
}
