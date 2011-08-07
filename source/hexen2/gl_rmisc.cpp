// r_misc.c

#include "quakedef.h"
#include "glquake.h"

unsigned	d_8to24TranslucentTable[256];

float RTint[256],GTint[256],BTint[256];

qboolean	vid_initialized = false;

//
// screen size info
//
refdef_t	r_refdef;

QCvar*	gl_reporttjunctions;

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
	gl_reporttjunctions = Cvar_Get("gl_reporttjunctions", "0", 0);

	R_InitParticles ();

	playerTranslation = (byte *)COM_LoadHunkFile ("gfx/player.lmp");
	if (!playerTranslation)
		Sys_Error ("Couldn't load gfx/player.lmp");
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
	R_BeginRegistration(&cls.glconfig);

	VID_SetPalette();

	Sys_ShowConsole(0, false);

	if (COM_CheckParm("-scale2d"))
	{
		viddef.height = 200;
		viddef.width = 320;
	}
	else
	{
		viddef.height = glConfig.vidHeight;
		viddef.width = glConfig.vidWidth;
	}

	vid_initialized = true;
}
