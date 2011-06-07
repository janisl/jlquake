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

float		gldepthmin, gldepthmax;

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

byte	dottexture[8][8] =
{
	{0,1,1,0,0,0,0,0},
	{1,1,1,1,0,0,0,0},
	{1,1,1,1,0,0,0,0},
	{0,1,1,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
};
void R_InitParticleTexture (void)
{
	int		x,y;
	byte	data[8][8][4];

	//
	// particle texture
	//
	for (x=0 ; x<8 ; x++)
	{
		for (y=0 ; y<8 ; y++)
		{
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = dottexture[x][y]*255;
		}
	}
	particletexture = R_CreateImage("*particle", (byte*)data, 8, 8, false, false, GL_CLAMP, false);

	GL_TexEnv(GL_MODULATE);
}

/*
===============
R_Envmap_f

Grab six views for environment mapping tests
===============
*/
void R_Envmap_f (void)
{
	byte	buffer[256*256*4];
	char	name[1024];

	qglDrawBuffer  (GL_FRONT);
	qglReadBuffer  (GL_FRONT);
	envmap = true;

	r_refdef.x = 0;
	r_refdef.y = 0;
	r_refdef.width = 256;
	r_refdef.height = 256;

	vec3_t viewangles;
	viewangles[0] = 0;
	viewangles[1] = 0;
	viewangles[2] = 0;
	AnglesToAxis(viewangles, r_refdef.viewaxis);
	R_RenderView ();
	qglReadPixels (0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	FS_WriteFile("env0.rgb", buffer, sizeof(buffer));		

	viewangles[1] = 90;
	AnglesToAxis(viewangles, r_refdef.viewaxis);
	R_RenderView ();
	qglReadPixels (0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	FS_WriteFile("env1.rgb", buffer, sizeof(buffer));		

	viewangles[1] = 180;
	AnglesToAxis(viewangles, r_refdef.viewaxis);
	R_RenderView ();
	qglReadPixels (0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	FS_WriteFile("env2.rgb", buffer, sizeof(buffer));		

	viewangles[1] = 270;
	AnglesToAxis(viewangles, r_refdef.viewaxis);
	R_RenderView ();
	qglReadPixels (0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	FS_WriteFile("env3.rgb", buffer, sizeof(buffer));		

	viewangles[0] = -90;
	viewangles[1] = 0;
	AnglesToAxis(viewangles, r_refdef.viewaxis);
	R_RenderView ();
	qglReadPixels (0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	FS_WriteFile("env4.rgb", buffer, sizeof(buffer));		

	viewangles[0] = 90;
	viewangles[1] = 0;
	AnglesToAxis(viewangles, r_refdef.viewaxis);
	R_RenderView ();
	qglReadPixels (0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	FS_WriteFile("env5.rgb", buffer, sizeof(buffer));		

	envmap = false;
	qglDrawBuffer  (GL_BACK);
	qglReadBuffer  (GL_BACK);
	GL_EndRendering ();
}

/*
===============
R_Init
===============
*/
void R_Init (void)
{	
	extern byte *hunk_base;
	extern QCvar* gl_finish;

	Cmd_AddCommand ("timerefresh", R_TimeRefresh_f);	
	Cmd_AddCommand ("envmap", R_Envmap_f);	
	//Cmd_AddCommand ("pointfile", R_ReadPointFile_f);	

	r_norefresh = Cvar_Get("r_norefresh", "0", 0);
	r_drawentities = Cvar_Get("r_drawentities", "1", 0);
	r_drawviewmodel = Cvar_Get("r_drawviewmodel", "1", 0);
	r_speeds = Cvar_Get("r_speeds", "0", 0);
	r_shadows = Cvar_Get("r_shadows", "0", 0);
	r_dynamic = Cvar_Get("r_dynamic", "1", 0);
	r_novis = Cvar_Get("r_novis", "0", 0);

	gl_finish = Cvar_Get("gl_finish","0", 0);
	gl_clear = Cvar_Get("gl_clear", "0", 0);
	gl_texsort = Cvar_Get("gl_texsort", "1", 0);

	if (qglActiveTextureARB)
		Cvar_SetValue ("gl_texsort", 0.0);

	gl_cull = Cvar_Get("gl_cull", "1", 0);
	gl_smoothmodels = Cvar_Get("gl_smoothmodels", "1", 0);
	gl_affinemodels = Cvar_Get("gl_affinemodels", "0", 0);
	gl_polyblend = Cvar_Get("gl_polyblend", "1", 0);
	gl_nocolors = Cvar_Get("gl_nocolors", "0", 0);

	gl_keeptjunctions = Cvar_Get("gl_keeptjunctions", "1", CVAR_ARCHIVE);
	gl_reporttjunctions = Cvar_Get("gl_reporttjunctions", "0", 0);

	gl_doubleeyes = Cvar_Get("gl_doubleeys", "1", 0);

	R_InitParticles ();
	R_InitParticleTexture ();
}

/*
===============
R_TranslatePlayerSkin

Translates a skin texture by the per-player color lookup
===============
*/
void R_TranslatePlayerSkin (int playernum)
{
	int		top, bottom;
	byte	translate[256];
	unsigned	translate32[256];
	int		i, j, s;
	model_t	*model;
	mesh1hdr_t *paliashdr;
	byte	*original;
	unsigned	pixels[512*256], *out;
	unsigned	scaled_width, scaled_height;
	int			inwidth, inheight;
	byte		*inrow;
	unsigned	frac, fracstep;

	GL_DisableMultitexture();

	top = cl.scores[playernum].colors & 0xf0;
	bottom = (cl.scores[playernum].colors &15)<<4;

	for (i=0 ; i<256 ; i++)
		translate[i] = i;

	for (i=0 ; i<16 ; i++)
	{
		if (top < 128)	// the artists made some backwards ranges.  sigh.
			translate[TOP_RANGE+i] = top+i;
		else
			translate[TOP_RANGE+i] = top+15-i;
				
		if (bottom < 128)
			translate[BOTTOM_RANGE+i] = bottom+i;
		else
			translate[BOTTOM_RANGE+i] = bottom+15-i;
	}

	//
	// locate the original skin pixels
	//
	entity_t* ent = &cl_entities[1+playernum];
	model = R_GetModelByHandle(ent->model);
	if (!model)
		return;		// player doesn't have a model yet
	if (model->type != MOD_MESH1)
		return; // only translate skins on alias models

	paliashdr = (mesh1hdr_t *)Mod_Extradata (model);
	s = paliashdr->skinwidth * paliashdr->skinheight;
	original = q1_player_8bit_texels;
	if (s & 3)
		Sys_Error ("R_TranslateSkin: s&3");

	inwidth = paliashdr->skinwidth;
	inheight = paliashdr->skinheight;

	scaled_width = 512;
	scaled_height = 256;

	for (i=0 ; i<256 ; i++)
		translate32[i] = d_8to24table[translate[i]];

	out = pixels;
	fracstep = inwidth*0x10000/scaled_width;
	for (i=0 ; i<scaled_height ; i++, out += scaled_width)
	{
		inrow = original + inwidth*(i*inheight/scaled_height);
		frac = fracstep >> 1;
		for (j=0 ; j<scaled_width ; j+=4)
		{
			out[j] = translate32[inrow[frac>>16]];
			frac += fracstep;
			out[j+1] = translate32[inrow[frac>>16]];
			frac += fracstep;
			out[j+2] = translate32[inrow[frac>>16]];
			frac += fracstep;
			out[j+3] = translate32[inrow[frac>>16]];
			frac += fracstep;
		}
	}

	// because this happens during gameplay, do it fast
	// instead of sending it through gl_upload 8
	if (!playertextures[playernum])
	{
		playertextures[playernum] = R_CreateImage(va("*player%d", playernum), (byte*)pixels, scaled_width, scaled_height, false, true, GL_CLAMP, false);
	}
	else
	{
		R_ReUploadImage(playertextures[playernum], (byte*)pixels);
	}

	GL_Bind(playertextures[playernum]);
	GL_TexEnv(GL_MODULATE);
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
		d_lightstylevalue[i] = 264;		// normal light value

	r_viewleaf = NULL;
	R_ClearParticles ();

	GL_BuildLightmaps ();

	// identify sky texture
	skytexturenum = -1;
	for (i=0 ; i<tr.worldModel->brush29_numtextures ; i++)
	{
		if (!tr.worldModel->brush29_textures[i])
			continue;
		if (!QStr::NCmp(tr.worldModel->brush29_textures[i]->name,"sky",3) )
			skytexturenum = i;
 		tr.worldModel->brush29_textures[i]->texturechain = NULL;
	}
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
	int			startangle;
	vrect_t		vr;

	qglDrawBuffer  (GL_FRONT);
	qglFinish ();

	start = Sys_DoubleTime();
	vec3_t viewangles;
	viewangles[0] = 0;
	viewangles[1] = 0;
	viewangles[2] = 0;
	for (i=0 ; i<128 ; i++)
	{
		viewangles[1] = i/128.0*360.0;
		AnglesToAxis(viewangles, r_refdef.viewaxis);
		R_RenderView ();
	}

	qglFinish ();
	stop = Sys_DoubleTime ();
	time = stop-start;
	Con_Printf ("%f seconds (%f fps)\n", time, 128/time);

	qglDrawBuffer  (GL_BACK);
	GL_EndRendering ();
}

/*
===============
GL_Init
===============
*/
static void GL_Init()
{
	CommonGfxInfo_f();

	qglClearColor (1,0,0,0);
	qglCullFace(GL_FRONT);
	qglEnable(GL_TEXTURE_2D);

	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80);

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglShadeModel (GL_FLAT);

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	GL_TexEnv(GL_REPLACE);
	qglDepthFunc (GL_LEQUAL);
}

void VID_Init()
{
	R_SharedRegister();

	R_CommonInit();

	GL_Init();

	Sys_ShowConsole(0, false);

	int i;
	if ((i = COM_CheckParm("-conwidth")) != 0)
		vid.conwidth = QStr::Atoi(COM_Argv(i+1));
	else
		vid.conwidth = 640;

	vid.conwidth &= 0xfff8; // make it a multiple of eight

	if (vid.conwidth < 320)
		vid.conwidth = 320;

	// pick a conheight that matches with correct aspect
	vid.conheight = vid.conwidth / glConfig.windowAspect;

	if ((i = COM_CheckParm("-conheight")) != 0)
		vid.conheight = QStr::Atoi(COM_Argv(i+1));
	if (vid.conheight < 200)
		vid.conheight = 200;

	if (vid.conheight > glConfig.vidHeight)
		vid.conheight = glConfig.vidHeight;
	if (vid.conwidth > glConfig.vidWidth)
		vid.conwidth = glConfig.vidWidth;
	vid.width = vid.conwidth;
	vid.height = vid.conheight;

	vid.numpages = 2;

	vid.recalc_refdef = 1;

	vid.colormap = host_colormap;

	tr.registered = true;
}

void VID_Shutdown(void)
{
	R_FreeModels();
	R_FreeShaders();
	R_DeleteTextures();
	R_DoneFreeType();
	GLimp_Shutdown();

	// shutdown QGL subsystem
	QGL_Shutdown();

	tr.registered = false;
}

void GL_EndRendering()
{
	//qglFlush();
	GLimp_SwapBuffers();
}
