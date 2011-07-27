// gl_main.c

/*
 * $Header: /H2 Mission Pack/gl_rmain.c 4     3/30/98 10:57a Jmonroe $
 */

#include "quakedef.h"
#include "glquake.h"

image_t*	playertextures[16];		// up to 16 color translated skins
image_t*	gl_extra_textures[MAX_EXTRA_TEXTURES];   // generic textures for models

extern qhandle_t	player_models[NUM_CLASSES];

//
// screen size info
//
refdef_t	r_refdef;

int		d_lightstylevalue[256];	// 8.8 fraction of base light value


QCvar*	r_drawviewmodel;

QCvar*	gl_polyblend;
QCvar*	gl_nocolors;
QCvar*	gl_reporttjunctions;

/*
=============================================================

  ALIAS MODELS

=============================================================
*/

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
			gl_extra_textures[Ent->skinNum - 100] = R_CachePic(temp);
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
	if (cl.maxclients > 1)
		Cvar_Set ("r_fullbright", "0");

	R_AnimateLight ();

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

	R_BeginFrame(STEREO_CENTER);

	R_SetupFrame ();

	r_refdef.time = (int)(cl.time * 1000);

	R_RenderScene(&r_refdef);

	R_PolyBlend ();
}
