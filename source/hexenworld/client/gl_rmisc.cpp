// r_misc.c

#include "quakedef.h"
#include "glquake.h"

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
CL_InitRenderStuff
===============
*/
void CL_InitRenderStuff (void)
{
	Cmd_AddCommand ("timerefresh", R_TimeRefresh_f);	

	if (qglActiveTextureARB)
		Cvar_SetValue ("r_texsort", 0.0);

	gl_reporttjunctions = Cvar_Get("gl_reporttjunctions", "0", 0);

	r_teamcolor = Cvar_Get("r_teamcolor", "187", CVAR_ARCHIVE);

	R_InitParticles ();

	playerTranslation = (byte *)COM_LoadHunkFile ("gfx/player.lmp");
	if (!playerTranslation)
		Sys_Error ("Couldn't load gfx/player.lmp");
}

extern qhandle_t	player_models[MAX_PLAYER_CLASS];

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

	if (cls.state != ca_active)
	{
		Con_Printf("Not connected to a server\n");
		return;
	}

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
	R_BeginRegistration(&cls.glconfig);

	VID_SetPalette();

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
	viddef.height = viddef.width*3 / 4;

	if ((i = COM_CheckParm("-conheight")) != 0)
		viddef.height = QStr::Atoi(COM_Argv(i+1));
	if (viddef.height < 200)
		viddef.height = 200;

	if (viddef.height > glConfig.vidHeight)
		viddef.height = glConfig.vidHeight;
	if (viddef.width > glConfig.vidWidth)
		viddef.width = glConfig.vidWidth;
}
