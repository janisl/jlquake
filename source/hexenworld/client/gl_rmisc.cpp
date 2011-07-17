// r_misc.c

#include "quakedef.h"
#include "glquake.h"

byte *playerTranslation;

byte globalcolormap[VID_GRADES*256];

unsigned	d_8to24TranslucentTable[256];

float RTint[256],GTint[256],BTint[256];

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
R_Init
===============
*/
void R_Init (void)
{
	extern byte *hunk_base;
	int			counter;

	Cmd_AddCommand ("timerefresh", R_TimeRefresh_f);	

	r_norefresh = Cvar_Get("r_norefresh", "0", 0);
	r_drawentities = Cvar_Get("r_drawentities", "1", 0);
	r_drawviewmodel = Cvar_Get("r_drawviewmodel", "1", 0);
	r_netgraph = Cvar_Get("r_netgraph","0", 0);


	if (qglActiveTextureARB)
		Cvar_SetValue ("r_texsort", 0.0);

	gl_cull = Cvar_Get("gl_cull", "1", 0);
	gl_polyblend = Cvar_Get("gl_polyblend", "1", 0);
	gl_nocolors = Cvar_Get("gl_nocolors", "0", 0);

	gl_reporttjunctions = Cvar_Get("gl_reporttjunctions", "0", 0);

	r_teamcolor = Cvar_Get("r_teamcolor", "187", CVAR_ARCHIVE);

	R_InitParticles ();

	playerTranslation = (byte *)COM_LoadHunkFile ("gfx/player.lmp");
	if (!playerTranslation)
		Sys_Error ("Couldn't load gfx/player.lmp");
}

extern int color_offsets[MAX_PLAYER_CLASS];
extern qhandle_t	player_models[MAX_PLAYER_CLASS];

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
	mesh1hdr_t		*paliashdr;
	byte			*original;
	unsigned		pixels[512*256], *out;
	unsigned		scaled_width, scaled_height;
	int				inwidth, inheight;
	byte			*inrow;
	unsigned		frac, fracstep;
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


	model = R_GetModelByHandle(player_models[cl.players[playernum].playerclass-1]);
	if (!model)
		return;
	// player doesn't have a model yet
	paliashdr = (mesh1hdr_t *)model->q1_cache;
	s = paliashdr->skinwidth * paliashdr->skinheight;

	if (cl.players[playernum].playerclass >= 1 && 
		cl.players[playernum].playerclass <= MAX_PLAYER_CLASS)
	{
		original = h2_player_8bit_texels[(int)cl.players[playernum].playerclass-1];
		cl.players[playernum].Translated = true;
	}
	else
		original = h2_player_8bit_texels[0];

//	if (s & 3)
//		Sys_Error ("R_TranslateSkin: s&3");

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

	if (cls.state != ca_active)
	{
		Con_Printf("Not connected to a server\n");
		return;
	}

	qglDrawBuffer  (GL_FRONT);
	qglFinish ();

	start = Sys_DoubleTime ();
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
	unsigned short r,g,b;
	int     v;
	unsigned short i, p, c;

	byte* pal = host_basepal;
	unsigned* table = d_8to24TranslucentTable;

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
===================
VID_Init
===================
*/

void VID_Init()
{
	R_CommonInit1();

	R_SharedRegister();

	R_CommonInit2();

	VID_SetPalette();

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

	tr.registered = true;
}

void VID_Shutdown(void)
{
	R_CommonShutdown(true);
}

void GL_EndRendering (void)
{
	GLimp_SwapBuffers();
	R_ToggleSmpFrame();
}
