// r_misc.c

#include "quakedef.h"
#include "glquake.h"

byte *playerTranslation;

unsigned	d_8to24TranslucentTable[256];

float RTint[256],GTint[256],BTint[256];

byte globalcolormap[VID_GRADES*256];

float		gldepthmin, gldepthmax;

qboolean	vid_initialized = false;

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
byte	dottexture[8][8] =
{
	{0,1,1,0,0,0,0,0},
	{1,1,1,1,0,0,0,0},
	{1,1,1,1,0,0,0,0},
	{0,1,1,0,0,0,0,0},
	{0,0,0,0,1,0,0,0},
	{0,0,0,0,1,0,0,1},
	{0,0,0,0,0,0,1,0},
	{0,0,0,1,0,0,0,1},
};
*/

byte	dottexture[16][16] =
{
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},//1
	{0,0,0,1,0,1,0,0,0,0,0,0,1,1,0,0},
	{0,0,0,0,1,0,0,0,0,0,0,1,1,1,1,0},
	{0,1,0,1,1,1,0,1,0,0,0,1,1,1,1,0},//4
	{0,0,1,1,1,1,1,0,0,0,0,0,1,1,0,0},
	{0,1,0,1,1,1,0,1,0,0,0,0,0,0,0,0},
	{0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0},//8
	{0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0},
	{0,0,0,0,0,0,0,0,0,1,1,1,0,1,0,0},
	{0,0,0,0,0,0,0,0,1,1,0,1,1,0,1,0},
	{0,0,0,1,0,0,0,0,1,1,1,1,1,0,1,0},//12
	{0,1,1,1,0,0,0,0,1,1,0,1,1,0,1,0},
	{0,0,1,1,1,0,0,0,0,1,1,1,0,1,0,0},
	{0,0,1,0,0,0,0,0,0,0,1,1,1,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},//16
};

void R_InitParticleTexture (void)
{
	int		x,y;
	byte	texsize = 16;//was 8
	byte	data[16][16][4];

	//
	// particle texture
	//
	for (x=0 ; x<texsize ; x++)
	{
		for (y=0 ; y<texsize ; y++)
		{
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = dottexture[x][y]*255;
		}
	}
	particletexture = R_CreateImage("*particle", (byte*)data, texsize, texsize, false, false, GL_CLAMP, false);

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
	int counter;

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
	r_wholeframe = Cvar_Get("r_wholeframe", "1", CVAR_ARCHIVE);

	gl_clear = Cvar_Get("gl_clear", "0", 0);
	gl_texsort = Cvar_Get("gl_texsort", "1", 0);
	gl_cull = Cvar_Get("gl_cull", "1", 0);
	gl_smoothmodels = Cvar_Get("gl_smoothmodels", "1", 0);
	gl_affinemodels = Cvar_Get("gl_affinemodels", "0", 0);
	gl_polyblend = Cvar_Get("gl_polyblend", "1", 0);
	gl_nocolors = Cvar_Get("gl_nocolors", "0", 0);

	gl_keeptjunctions = Cvar_Get("gl_keeptjunctions", "1", CVAR_ARCHIVE);
	gl_reporttjunctions = Cvar_Get("gl_reporttjunctions", "0", 0);

	R_InitParticles ();
	R_InitParticleTexture ();

	playerTranslation = (byte *)COM_LoadHunkFile ("gfx/player.lmp");
	if (!playerTranslation)
		Sys_Error ("Couldn't load gfx/player.lmp");
}

extern int color_offsets[NUM_CLASSES];

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
	byte	*sourceA, *sourceB, *colorA, *colorB;
	int		playerclass = (int)cl.scores[playernum].playerclass;

	for (i=0 ; i<256 ; i++)
		translate[i] = i;

	top = (cl.scores[playernum].colors & 0xf0) >> 4;
	bottom = (cl.scores[playernum].colors & 15);

	if (top > 10) top = 0;
	if (bottom > 10) bottom = 0;

	top -= 1;
	bottom -= 1;

	colorA = playerTranslation + 256 + color_offsets[playerclass-1];
	colorB = colorA + 256;
	sourceA = colorB + 256 + (top * 256);
	sourceB = colorB + 256 + (bottom * 256);
	for(i=0;i<256;i++,colorA++,colorB++,sourceA++,sourceB++)
	{
		if (top >= 0 && (*colorA != 255)) 
			translate[i] = *sourceA;
		if (bottom >= 0 && (*colorB != 255)) 
			translate[i] = *sourceB;
	}


	//
	// locate the original skin pixels
	//
	entity_t* ent = &cl_entities[1+playernum];
	model = R_GetModelByHandle(ent->model);
	if (model->type != MOD_MESH1)
		return;		// player doesn't have a model yet
	paliashdr = (mesh1hdr_t *)Mod_Extradata (model);
	s = paliashdr->skinwidth * paliashdr->skinheight;

	if (playerclass >= 1 && 
		playerclass <= NUM_CLASSES)
		original = h2_player_8bit_texels[playerclass-1];
	else
		original = h2_player_8bit_texels[0];

	if (s & 3)
		Sys_Error ("R_TranslateSkin: s&3");

	for (i=0 ; i<256 ; i++)
		translate32[i] = d_8to24table[translate[i]];
	scaled_width = 512;
	scaled_height = 256;

	inwidth = paliashdr->skinwidth;
	inheight = paliashdr->skinheight;
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

void VID_SetPalette()
{
	byte	*pal;
	int		r,g,b,v;
	int		i,c,p;
	unsigned	*table;

	pal = host_basepal;
	table = d_8to24TranslucentTable;

	for (i=0; i<16;i++)
	{
		c = ColorIndex[i]*3;

		r = pal[c];
		g = pal[c+1];
		b = pal[c+2];

		for(p=0;p<16;p++)
		{
			v = (ColorPercent[15-p]<<24) + (r<<0) + (g<<8) + (b<<16);
			//v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
			*table++ = v;

			RTint[i*16+p] = ((float)r) / ((float)ColorPercent[15-p]) ;
			GTint[i*16+p] = ((float)g) / ((float)ColorPercent[15-p]);
			BTint[i*16+p] = ((float)b) / ((float)ColorPercent[15-p]);
		}
	}
}

/*
===============
GL_Init
===============
*/
void GL_Init()
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

void D_ShowLoadingSize(void)
{
	if (!vid_initialized)
		return;

	qglDrawBuffer  (GL_FRONT);

	SCR_DrawLoading();

	qglDrawBuffer  (GL_BACK);
}

void VID_Init()
{
	R_SharedRegister();

	R_CommonInit();

	VID_SetPalette();

	GL_Init();

	Sys_ShowConsole(0, false);

	if (COM_CheckParm("-scale2d"))
	{
		vid.height = vid.conheight = 200;
		vid.width = vid.conwidth = 320;
	}
	else
	{
		vid.height = vid.conheight = glConfig.vidHeight;
		vid.width = vid.conwidth = glConfig.vidWidth;
	}
	vid.numpages = 2;

	vid.recalc_refdef = 1;

	vid.colormap = host_colormap;

	vid_initialized = true;

	tr.registered = true;
}

void VID_Shutdown(void)
{
	R_FreeModels();
	R_FreeShaders();
	R_DeleteTextures();
	R_FreeBackEndData();
	R_DoneFreeType();
	GLimp_Shutdown();

	// shutdown QGL subsystem
	QGL_Shutdown();

	tr.registered = false;
}

void GL_EndRendering (void)
{
	GLimp_SwapBuffers();
	R_CommonToggleSmpFrame();
}
