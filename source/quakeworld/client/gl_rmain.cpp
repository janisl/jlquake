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

	R_Draw2DQuad(r_refdef.x, r_refdef.y, r_refdef.width, r_refdef.height, NULL, 0, 0, 0, 0, v_blend[0], v_blend[1], v_blend[2], v_blend[3]);
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
}

/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
void R_RenderView (void)
{
	for (int i = 0; i < MAX_LIGHTSTYLES_Q1; i++)
	{
		float Val = d_lightstylevalue[i] / 256.0;
		R_AddLightStyleToScene(i, Val, Val, Val);
	}
	CL_AddParticles();

	tr.frameCount++;
	tr.frameSceneNum = 0;

	glState.finishCalled = false;

	R_BeginFrameCommon(STEREO_CENTER);

	R_SetupFrame ();

	r_refdef.time = (int)(cl.time * 1000);

	R_RenderScene(&r_refdef);

	R_PolyBlend ();
}
