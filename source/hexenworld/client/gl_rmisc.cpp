// r_misc.c

#include "quakedef.h"
#include "../../client/render_local.h"

unsigned	d_8to24TranslucentTable[256];

float RTint[256],GTint[256],BTint[256];

QCvar*	r_teamcolor;

/*
=============================================================

  ALIAS MODELS

=============================================================
*/

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

/*
===============
CL_InitRenderStuff
===============
*/
void CL_InitRenderStuff (void)
{
	if (qglActiveTextureARB)
		Cvar_SetValue ("r_texsort", 0.0);

	r_teamcolor = Cvar_Get("r_teamcolor", "187", CVAR_ARCHIVE);

	R_InitParticles ();

	playerTranslation = (byte *)COM_LoadHunkFile ("gfx/player.lmp");
	if (!playerTranslation)
		Sys_Error ("Couldn't load gfx/player.lmp");
}
