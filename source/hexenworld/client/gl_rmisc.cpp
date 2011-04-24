// r_misc.c

#include "quakedef.h"
#include "glquake.h"

byte *playerTranslation;

byte globalcolormap[VID_GRADES*256];

unsigned	d_8to24table[256];
unsigned	d_8to24TranslucentTable[256];

float RTint[256],GTint[256],BTint[256];

int ColorIndex[16] =
{
	0, 31, 47, 63, 79, 95, 111, 127, 143, 159, 175, 191, 199, 207, 223, 231
};

unsigned ColorPercent[16] =
{
	25, 51, 76, 102, 114, 127, 140, 153, 165, 178, 191, 204, 216, 229, 237, 247
};

//int		texture_mode = GL_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_LINEAR;
int		texture_mode = GL_LINEAR;
//int		texture_mode = GL_LINEAR_MIPMAP_NEAREST;
//int		texture_mode = GL_LINEAR_MIPMAP_LINEAR;

int		texture_extension_number = 1;

float		gldepthmin, gldepthmax;

qboolean gl_mtexable = false;

extern void R_InitBubble();

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
	r_notexture_mip = (texture_t*)Hunk_AllocName (sizeof(texture_t) + 16*16+8*8+4*4+2*2, "notexture");
	
	r_notexture_mip->width = r_notexture_mip->height = 16;
	r_notexture_mip->offsets[0] = sizeof(texture_t);
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
	particletexture = texture_extension_number++;
    GL_Bind(particletexture);

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
	qglTexImage2D (GL_TEXTURE_2D, 0, gl_alpha_format, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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

	r_refdef.vrect.x = 0;
	r_refdef.vrect.y = 0;
	r_refdef.vrect.width = 256;
	r_refdef.vrect.height = 256;

	r_refdef.viewangles[0] = 0;
	r_refdef.viewangles[1] = 0;
	r_refdef.viewangles[2] = 0;
	GL_BeginRendering (&glx, &gly, &glwidth, &glheight);
	R_RenderView ();
	qglReadPixels (0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	FS_WriteFile("env0.rgb", buffer, sizeof(buffer));		

	r_refdef.viewangles[1] = 90;
	GL_BeginRendering (&glx, &gly, &glwidth, &glheight);
	R_RenderView ();
	qglReadPixels (0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	FS_WriteFile("env1.rgb", buffer, sizeof(buffer));		

	r_refdef.viewangles[1] = 180;
	GL_BeginRendering (&glx, &gly, &glwidth, &glheight);
	R_RenderView ();
	qglReadPixels (0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	FS_WriteFile("env2.rgb", buffer, sizeof(buffer));		

	r_refdef.viewangles[1] = 270;
	GL_BeginRendering (&glx, &gly, &glwidth, &glheight);
	R_RenderView ();
	qglReadPixels (0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	FS_WriteFile("env3.rgb", buffer, sizeof(buffer));		

	r_refdef.viewangles[0] = -90;
	r_refdef.viewangles[1] = 0;
	GL_BeginRendering (&glx, &gly, &glwidth, &glheight);
	R_RenderView ();
	qglReadPixels (0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	FS_WriteFile("env4.rgb", buffer, sizeof(buffer));		

	r_refdef.viewangles[0] = 90;
	r_refdef.viewangles[1] = 0;
	GL_BeginRendering (&glx, &gly, &glwidth, &glheight);
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
	int			counter;

	Cmd_AddCommand ("timerefresh", R_TimeRefresh_f);	
	Cmd_AddCommand ("envmap", R_Envmap_f);	
	//Cmd_AddCommand ("pointfile", R_ReadPointFile_f);	

	r_norefresh = Cvar_Get("r_norefresh", "0", 0);
	r_drawentities = Cvar_Get("r_drawentities", "1", 0);
	r_drawviewmodel = Cvar_Get("r_drawviewmodel", "1", 0);
	r_speeds = Cvar_Get("r_speeds", "0", 0);
	r_fullbright = Cvar_Get("r_fullbright", "0", 0);
	r_lightmap = Cvar_Get("r_lightmap", "0", 0);
	r_shadows = Cvar_Get("r_shadows", "0", 0);
	r_mirroralpha = Cvar_Get("r_mirroralpha", "1", 0);
	r_wateralpha = Cvar_Get("r_wateralpha","0.4", CVAR_ARCHIVE);
	r_dynamic = Cvar_Get("r_dynamic", "1", 0);
	r_novis = Cvar_Get("r_novis", "0", 0);
	r_netgraph = Cvar_Get("r_netgraph","0", 0);
	r_entdistance = Cvar_Get("r_entdistance", "0", CVAR_ARCHIVE);

	gl_clear = Cvar_Get("gl_clear", "0", 0);
	gl_texsort = Cvar_Get("gl_texsort", "1", 0);

	if (gl_mtexable)
		Cvar_SetValue ("gl_texsort", 0.0);

	gl_cull = Cvar_Get("gl_cull", "1", 0);
	gl_smoothmodels = Cvar_Get("gl_smoothmodels", "1", 0);
	gl_affinemodels = Cvar_Get("gl_affinemodels", "0", 0);
	gl_polyblend = Cvar_Get("gl_polyblend", "1", 0);
	gl_flashblend = Cvar_Get("gl_flashblend", "0", 0);
	gl_playermip = Cvar_Get("gl_playermip", "0", 0);
	gl_nocolors = Cvar_Get("gl_nocolors", "0", 0);

	gl_keeptjunctions = Cvar_Get("gl_keeptjunctions", "1", CVAR_ARCHIVE);
	gl_reporttjunctions = Cvar_Get("gl_reporttjunctions", "0", 0);

	r_teamcolor = Cvar_Get("r_teamcolor", "187", CVAR_ARCHIVE);

    gl_ztrick = Cvar_Get("gl_ztrick", "1", 0);

	R_InitBubble();
	
	R_InitParticles ();
	R_InitParticleTexture ();

	netgraphtexture = texture_extension_number;
	texture_extension_number++;

	playertextures = texture_extension_number;
	texture_extension_number += MAX_CLIENTS;

	for(counter=0;counter<MAX_EXTRA_TEXTURES;counter++)
		gl_extra_textures[counter] = -1;

	playerTranslation = (byte *)COM_LoadHunkFile ("gfx/player.lmp");
	if (!playerTranslation)
		Sys_Error ("Couldn't load gfx/player.lmp");
}

/*
===============
R_TranslatePlayerSkin

Translates a skin texture by the per-player color lookup
===============
*/
/*void R_TranslatePlayerSkin (int playernum)
{
	int		top, bottom;
	byte	translate[256];
	unsigned	translate32[256];
	int		i, j;
	model_t	*model;
	aliashdr_t *paliashdr;
	byte	*original;
	unsigned	pixels[512*256], *out;
	unsigned	scaled_width, scaled_height;
	int			inwidth, inheight;
	int			tinwidth, tinheight;
	byte		*inrow;
	unsigned	frac, fracstep;
	player_info_t *player;
	extern	byte		player_8bit_texels[320*200];

	GL_DisableMultitexture();

	player = &cl.players[playernum];
	if (!player->name[0])
		return;

	top = player->topcolor;
	if (top > 13 || top < 0)
		top = 13;
	top *= 16;
	bottom = player->bottomcolor;
	if (bottom > 13 || bottom < 0)
		bottom = 13;
	bottom *= 16;

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
	// real model width
	tinwidth = 296;
	tinheight = 194;

	if (!player->skin)
		Skin_Find(player);
	if ((original = Skin_Cache(player->skin)) != NULL) {
		//skin data width
		inwidth = 320;
		inheight = 200;
	} else {
		original = player_8bit_texels;
		inwidth = 296;
		inheight = 194;
	}


	// because this happens during gameplay, do it fast
	// instead of sending it through gl_upload 8
    GL_Bind(playertextures + playernum);

#if 0
	s = 320*200;
	byte	translated[320*200];

	for (i=0 ; i<s ; i+=4)
	{
		translated[i] = translate[original[i]];
		translated[i+1] = translate[original[i+1]];
		translated[i+2] = translate[original[i+2]];
		translated[i+3] = translate[original[i+3]];
	}


	// don't mipmap these, because it takes too long
	GL_Upload8 (translated, paliashdr->skinwidth, paliashdr->skinheight, 
		false, false, true);
#endif

	scaled_width = gl_max_size->value < 512 ? gl_max_size->value : 512;
	scaled_height = gl_max_size->value < 256 ? gl_max_size->value : 256;
	// allow users to crunch sizes down even more if they want
	scaled_width >>= (int)gl_playermip.value;
	scaled_height >>= (int)gl_playermip.value;

	for (i=0 ; i<256 ; i++)
		translate32[i] = d_8to24table[translate[i]];

	out = pixels;
	Com_Memset(pixels, 0, sizeof(pixels));
	fracstep = tinwidth*0x10000/scaled_width;
	for (i=0 ; i<scaled_height ; i++, out += scaled_width)
	{
		inrow = original + inwidth*(i*tinheight/scaled_height);
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

	qglTexImage2D (GL_TEXTURE_2D, 0, gl_solid_format, 
		scaled_width, scaled_height, 0, GL_RGBA, 
		GL_UNSIGNED_BYTE, pixels);

	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
*/

extern int color_offsets[MAX_PLAYER_CLASS];
extern model_t *player_models[MAX_PLAYER_CLASS];

/*
===============
R_TranslatePlayerSkin

Translates a skin texture by the per-player color lookup
===============
*/
void R_TranslatePlayerSkin (int playernum)
{
	int				top, bottom;
	byte			translate[256];
	unsigned		translate32[256];
	int				i, j, s;
	model_t			*model;
	aliashdr_t		*paliashdr;
	byte			*original;
	unsigned		pixels[512*256], *out;
	unsigned		scaled_width, scaled_height;
	int				inwidth, inheight;
	byte			*inrow;
	unsigned		frac, fracstep;
	extern	byte	player_8bit_texels[MAX_PLAYER_CLASS][620*245];
	byte			*sourceA, *sourceB, *colorA, *colorB;
	player_info_t	*player;


 	for (i=0 ; i<256 ; i++)
		translate[i] = i;

	player = &cl.players[playernum];
	if (!player->name[0])
		return;
	if (!player->playerclass)
		return;

	top = player->topcolor;
	bottom = player->bottomcolor;

	if (top > 10) top = 0;
	if (bottom > 10) bottom = 0;

	top -= 1;
	bottom -= 1;

	colorA = playerTranslation + 256 + color_offsets[(int)player->playerclass-1];
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
	if (cl.players[playernum].modelindex <= 0)
		return;


	model = player_models[cl.players[playernum].playerclass-1];
	if (!model)
		return;
	// player doesn't have a model yet
	paliashdr = (aliashdr_t *)Mod_Extradata (model);
	s = paliashdr->skinwidth * paliashdr->skinheight;

	if (cl.players[playernum].playerclass >= 1 && 
		cl.players[playernum].playerclass <= MAX_PLAYER_CLASS)
	{
		original = player_8bit_texels[(int)cl.players[playernum].playerclass-1];
		cl.players[playernum].Translated = true;
	}
	else
		original = player_8bit_texels[0];

//	if (s & 3)
//		Sys_Error ("R_TranslateSkin: s&3");

	// because this happens during gameplay, do it fast
	// instead of sending it through gl_upload 8
    GL_Bind(playertextures + playernum);

#if 0
	byte	translated[320*200];

	for (i=0 ; i<s ; i+=4)
	{
		translated[i] = translate[original[i]];
		translated[i+1] = translate[original[i+1]];
		translated[i+2] = translate[original[i+2]];
		translated[i+3] = translate[original[i+3]];
	}


	// don't mipmap these, because it takes too long
	GL_Upload8 (translated, paliashdr->skinwidth, paliashdr->skinheight, false, false, true);
#else
	for (i=0 ; i<256 ; i++)
		translate32[i] = d_8to24table[translate[i]];
	scaled_width = gl_max_size->value < 512 ? gl_max_size->value : 512;
	scaled_height = gl_max_size->value < 256 ? gl_max_size->value : 256;

	// allow users to crunch sizes down even more if they want
	scaled_width >>= (int)gl_playermip->value;
	scaled_height >>= (int)gl_playermip->value;

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
	qglTexImage2D (GL_TEXTURE_2D, 0, gl_solid_format, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif
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

	Com_Memset(&r_worldentity, 0, sizeof(r_worldentity));
	r_worldentity.model = cl.worldmodel;

// clear out efrags in case the level hasn't been reloaded
// FIXME: is this one short?
	for (i=0 ; i<cl.worldmodel->numleafs ; i++)
		cl.worldmodel->leafs[i].efrags = NULL;
		 	
	r_viewleaf = NULL;
	R_ClearParticles ();

	GL_BuildLightmaps ();

	// identify sky texture
	skytexturenum = -1;
	mirrortexturenum = -1;
	for (i=0 ; i<cl.worldmodel->numtextures ; i++)
	{
		if (!cl.worldmodel->textures[i])
			continue;
		if (!QStr::NCmp(cl.worldmodel->textures[i]->name,"sky",3) )
			skytexturenum = i;
		if (!QStr::NCmp(cl.worldmodel->textures[i]->name,"window02_1",10) )
			mirrortexturenum = i;
 		cl.worldmodel->textures[i]->texturechain = NULL;
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

	if (cls.state != ca_active)
	{
		Con_Printf("Not connected to a server\n");
		return;
	}

	qglDrawBuffer  (GL_FRONT);
	qglFinish ();

	start = Sys_DoubleTime ();
	for (i=0 ; i<128 ; i++)
	{
		r_refdef.viewangles[1] = i/128.0*360.0;
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
	unsigned short r,g,b;
	int     v;
	int     r1,g1,b1;
	int		j,k,l,m;
	unsigned short i, p, c;

	//
	// 8 8 8 encoding
	//
	byte* pal = host_basepal;
	unsigned* table = d_8to24table;
	for (int i = 0; i < 256; i++)
	{
		unsigned r = pal[0];
		unsigned g = pal[1];
		unsigned b = pal[2];
		pal += 3;
		
		unsigned v = (255 << 24) + (r << 0) + (g << 8) + (b << 16);
		*table++ = v;
	}
	d_8to24table[255] &= 0xffffff;	// 255 is transparent

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

static void CheckMultiTextureExtensions()
{
	if (strstr(glConfig.extensions_string, "GL_SGIS_multitexture ") && !COM_CheckParm("-nomtex")) {
		Con_Printf("Multitexture extensions found.\n");
		glMTexCoord2fSGIS = (lpMTexFUNC) GLimp_GetProcAddress("glMTexCoord2fSGIS");
		glSelectTextureSGIS = (lpSelTexFUNC) GLimp_GetProcAddress("glSelectTextureSGIS");
		gl_mtexable = true;
	}
}

/*
===============
GL_Init
===============
*/
void GL_Init()
{
	Con_Printf ("GL_VENDOR: %s\n", glConfig.vendor_string);
	Con_Printf ("GL_RENDERER: %s\n", glConfig.renderer_string);

	Con_Printf ("GL_VERSION: %s\n", glConfig.version_string);
	Con_Printf ("GL_EXTENSIONS: %s\n", glConfig.extensions_string);

	CheckMultiTextureExtensions ();

	qglClearColor (1,0,0,0);
	qglCullFace(GL_FRONT);
	qglEnable(GL_TEXTURE_2D);

	qglEnable(GL_ALPHA_TEST);
	qglAlphaFunc(GL_GREATER, 0.666);

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglShadeModel (GL_FLAT);

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

//	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

/*
=================
GL_BeginRendering

=================
*/
void GL_BeginRendering (int *x, int *y, int *width, int *height)
{
	*x = *y = 0;
	*width = glConfig.vidWidth;
	*height = glConfig.vidHeight;
}

/*
===================
VID_Init
===================
*/
void VID_Init()
{
	R_SharedRegister();

	R_CommonInit();

	VID_SetPalette();

	GL_Init ();

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
	vid.conheight = vid.conwidth*3 / 4;

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
}

void VID_Shutdown(void)
{
	GLimp_Shutdown();

	// shutdown QGL subsystem
	QGL_Shutdown();
}

void GL_EndRendering (void)
{
	GLimp_SwapBuffers();
}
