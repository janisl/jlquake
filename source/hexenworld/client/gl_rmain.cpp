// r_main.c

#include "quakedef.h"
#include "glquake.h"

image_t*	playertextures[MAX_CLIENTS];		// up to 16 color translated skins
image_t*	gl_extra_textures[MAX_EXTRA_TEXTURES];   // generic textures for models

extern qhandle_t	player_models[MAX_PLAYER_CLASS];

//
// screen size info
//
refdef_t	r_refdef;

QCvar*	gl_nocolors;
QCvar*	gl_reporttjunctions;
QCvar*	r_teamcolor;

extern	QCvar*	scr_fov;

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
			Sys_Error("skinnum > 255");
		}

		if (!gl_extra_textures[Ent->skinNum - 100])  // Need to load it in
		{
			char temp[80];
			QStr::Sprintf(temp, sizeof(temp), "gfx/skin%d.lmp", Ent->skinNum);
			gl_extra_textures[Ent->skinNum - 100] = R_CachePic(temp);
		}

		Ent->customSkin = R_GetImageHandle(gl_extra_textures[Ent->skinNum - 100]);
	}
	else if (PlayerNum >= 0 && !gl_nocolors->value)
	{
		// we can't dynamically colormap textures, so they are cached
		// seperately for the players.  Heads are just uncolored.
		//FIXME? What about Demoness and Dwarf?
		if (Ent->hModel == player_models[0] ||
			Ent->hModel == player_models[1] ||
			Ent->hModel == player_models[2] ||
			Ent->hModel == player_models[3])
		{
			if (!cl.players[PlayerNum].Translated)
			{
				R_TranslatePlayerSkin(PlayerNum);
			}

			Ent->customSkin = R_GetImageHandle(playertextures[PlayerNum]);
		}
	}
}

void R_DrawName(vec3_t origin, char *Name, int Red)
{
	if (!Name)
	{
		return;
	}

	vec4_t eye;
	vec4_t Out;
	R_TransformModelToClip(origin, tr.viewParms.world.modelMatrix, tr.viewParms.projectionMatrix, eye, Out);

	if (eye[2] > -r_znear->value)
	{
		return;
	}

	vec4_t normalized;
	vec4_t window;
	R_TransformClipToWindow(Out, &tr.viewParms, normalized, window);
	int u = tr.refdef.x + (int)window[0];
	int v = tr.refdef.y + tr.refdef.height - (int)window[1];

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
