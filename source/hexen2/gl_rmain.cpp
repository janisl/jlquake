// gl_main.c

/*
 * $Header: /H2 Mission Pack/gl_rmain.c 4     3/30/98 10:57a Jmonroe $
 */

#include "quakedef.h"
#include "glquake.h"

image_t*	gl_extra_textures[MAX_EXTRA_TEXTURES];   // generic textures for models

extern qhandle_t	player_models[NUM_CLASSES];

//
// screen size info
//
refdef_t	r_refdef;

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
	else if (PlayerNum >= 0 && Ent->hModel)
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
