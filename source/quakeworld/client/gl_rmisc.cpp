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
// r_misc.c

#include "quakedef.h"
#include "glquake.h"

/*
==================
R_InitTextures
==================
*/
void	R_InitTextures (void)
{
	int		x,y, m;
	byte	*dest;

// create a simple checkerboard texture for the default
	r_notexture_mip = (mbrush29_texture_t*)Hunk_AllocName (sizeof(mbrush29_texture_t) + 16*16+8*8+4*4+2*2, "notexture");
	
	r_notexture_mip->width = r_notexture_mip->height = 16;
	r_notexture_mip->offsets[0] = sizeof(mbrush29_texture_t);
	r_notexture_mip->offsets[1] = r_notexture_mip->offsets[0] + 16*16;
	r_notexture_mip->offsets[2] = r_notexture_mip->offsets[1] + 8*8;
	r_notexture_mip->offsets[3] = r_notexture_mip->offsets[2] + 4*4;
	
	for (m=0 ; m<4 ; m++)
	{
		dest = (byte *)r_notexture_mip + r_notexture_mip->offsets[m];
		for (y=0 ; y< (16>>m) ; y++)
			for (x=0 ; x< (16>>m) ; x++)
			{
				if (  (y< (8>>m) ) ^ (x< (8>>m) ) )
					*dest++ = 0;
				else
					*dest++ = 0xff;
			}
	}	
}

/*
===============
CL_InitRenderStuff
===============
*/
void CL_InitRenderStuff (void)
{
	Cmd_AddCommand ("timerefresh", R_TimeRefresh_f);	

	if (qglActiveTextureARB)
		Cvar_SetValue ("r_texsort", 0.0);

	gl_nocolors = Cvar_Get("gl_nocolors", "0", 0);

	gl_reporttjunctions = Cvar_Get("gl_reporttjunctions", "0", 0);

	R_InitParticles ();
}

/*
===============
R_TranslatePlayerSkin

Translates a skin texture by the per-player color lookup
===============
*/
void R_TranslatePlayerSkin (int playernum)
{
	byte	translate[256];
	byte	*original;
	player_info_t *player;
	char s[512];

	player = &cl.players[playernum];
	if (!player->name[0])
		return;

	QStr::Cpy(s, Info_ValueForKey(player->userinfo, "skin"));
	QStr::StripExtension(s, s);
	if (player->skin && !QStr::ICmp(s, player->skin->name))
		player->skin = NULL;

	if (player->_topcolor != player->topcolor ||
		player->_bottomcolor != player->bottomcolor || !player->skin)
	{
		player->_topcolor = player->topcolor;
		player->_bottomcolor = player->bottomcolor;

		int top = player->topcolor;
		int bottom = player->bottomcolor;
		top = (top < 0) ? 0 : ((top > 13) ? 13 : top);
		bottom = (bottom < 0) ? 0 : ((bottom > 13) ? 13 : bottom);
		top *= 16;
		bottom *= 16;

		for (int i = 0; i < 256; i++)
		{
			translate[i] = i;
		}

		for (int i = 0; i < 16; i++)
		{
			if (top < 128)	// the artists made some backwards ranges.  sigh.
			{
				translate[TOP_RANGE + i] = top + i;
			}
			else
			{
				translate[TOP_RANGE + i] = top + 15 - i;
			}

			if (bottom < 128)
			{
				translate[BOTTOM_RANGE + i] = bottom + i;
			}
			else
			{
				translate[BOTTOM_RANGE + i] = bottom + 15 - i;
			}
		}

		//
		// locate the original skin pixels
		//

		if (!player->skin)
		{
			Skin_Find(player);
		}
		original = Skin_Cache(player->skin);
		if (original != NULL)
		{
			//skin data width
			R_CreateOrUpdateTranslatedSkin(playertextures[playernum], va("*player%d", playernum), original, translate, 320, 200);
		}
		else
		{
			R_CreateOrUpdateTranslatedModelSkinQ1(playertextures[playernum], va("*player%d", playernum),
				cl.model_precache[cl_playerindex], translate);
		}
	}
}

/*
===============
R_NewMap
===============
*/
void R_NewMap (void)
{
	int		i;
	
	for (i=0 ; i<256 ; i++)
		cl_lightstylevalue[i] = 264;		// normal light value

	R_ClearParticles ();

	R_EndRegistration();
}


/*
====================
R_TimeRefresh_f

For program optimization
====================
*/
void R_TimeRefresh_f (void)
{
	int			i;
	float		start, stop, time;

	start = Sys_DoubleTime ();
	vec3_t viewangles;
	viewangles[0] = 0;
	viewangles[1] = 0;
	viewangles[2] = 0;
	for (i=0 ; i<128 ; i++)
	{
		viewangles[1] = i/128.0*360.0;
		AnglesToAxis(viewangles, r_refdef.viewaxis);
		R_BeginFrame(STEREO_CENTER);
		V_RenderScene ();
		R_EndFrame(NULL, NULL);
	}

	stop = Sys_DoubleTime ();
	time = stop-start;
	Con_Printf ("%f seconds (%f fps)\n", time, 128/time);
}

void VID_Init()
{
	R_BeginRegistration(&cls.glconfig);

	Sys_ShowConsole(0, false);

	int i;
	if ((i = COM_CheckParm("-conwidth")) != 0)
		viddef.width = QStr::Atoi(COM_Argv(i+1));
	else
		viddef.width = 640;

	viddef.width &= 0xfff8; // make it a multiple of eight

	if (viddef.width < 320)
		viddef.width = 320;

	// pick a conheight that matches with correct aspect
	viddef.height = viddef.width / glConfig.windowAspect;

	if ((i = COM_CheckParm("-conheight")) != 0)
		viddef.height = QStr::Atoi(COM_Argv(i+1));
	if (viddef.height < 200)
		viddef.height = 200;

	if (viddef.height > glConfig.vidHeight)
		viddef.height = glConfig.vidHeight;
	if (viddef.width > glConfig.vidWidth)
		viddef.width = glConfig.vidWidth;
}
