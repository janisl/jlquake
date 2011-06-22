/*
Copyright (C) 1997-2001 Id Software, Inc.

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
#include <stdio.h>

#include <math.h>

#include "../../client/client.h"
#include "../client/ref.h"

#include "qgl.h"

#include <GL/glu.h>

#ifndef GL_COLOR_INDEX8_EXT
#define GL_COLOR_INDEX8_EXT GL_COLOR_INDEX
#endif

#define	REF_VERSION	"GL 0.01"

//===================================================================

#include "gl_model.h"

void GL_EndRendering (void);

void GL_SetDefaultState( void );
void GL_UpdateSwapInterval( void );

extern	float	gldepthmin, gldepthmax;

#define BACKFACE_EPSILON	0.01


//====================================================


extern	image_t		*r_particletexture;
extern	int			c_brush_polys, c_alias_polys;


//
// screen size info
//
extern	refdef_t	r_newrefdef;
extern	int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

extern	QCvar	*r_norefresh;
extern	QCvar	*r_drawentities;
extern	QCvar	*r_drawworld;
extern	QCvar	*r_speeds;
extern	QCvar	*r_novis;
extern	QCvar	*r_lerpmodels;

extern	QCvar	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level

extern QCvar	*gl_particle_min_size;
extern QCvar	*gl_particle_max_size;
extern QCvar	*gl_particle_size;
extern QCvar	*gl_particle_att_a;
extern QCvar	*gl_particle_att_b;
extern QCvar	*gl_particle_att_c;

extern	QCvar	*gl_nosubimage;
extern	QCvar	*gl_shadows;
extern	QCvar	*gl_dynamic;
extern	QCvar	*gl_skymip;
extern	QCvar	*gl_showtris;
extern	QCvar	*gl_finish;
extern	QCvar	*gl_clear;
extern	QCvar	*gl_cull;
extern	QCvar	*gl_poly;
extern	QCvar	*gl_texsort;
extern	QCvar	*gl_polyblend;
extern	QCvar	*gl_lightmaptype;
extern	QCvar	*gl_modulate;
extern	QCvar	*gl_drawbuffer;
extern  QCvar  *gl_driver;
extern  QCvar  *gl_saturatelighting;
extern  QCvar  *gl_lockpvs;

extern	int		c_visible_lightmaps;
extern	int		c_visible_textures;

void R_TranslatePlayerSkin (int playernum);

void R_LightPoint (vec3_t p, vec3_t color);
void R_PushDlights (void);

//====================================================================


void V_AddBlend (float r, float g, float b, float a, float *v_blend);

int 	R_Init();
void	R_Shutdown( void );

void R_RenderView (refdef_t *fd);
void GL_ScreenShot_f (void);
void R_DrawAliasModel (trRefEntity_t *e);
void R_DrawBrushModel (trRefEntity_t *e);
void R_DrawBeam( trRefEntity_t *e );
void R_DrawWorld (void);
void R_DrawAlphaSurfaces (void);
void R_RenderBrushPoly (mbrush38_surface_t *fa);
void R_InitParticleTexture (void);
void Draw_InitLocal (void);
void R_MarkLeaves (void);

mbrush38_glpoly_t *WaterWarpPolyVerts (mbrush38_glpoly_t *p);
void EmitWaterPolys (mbrush38_surface_t *fa);
void R_AddSkySurface (mbrush38_surface_t *fa);
void R_ClearSkyBox (void);
void R_DrawSkyBox (void);
void R_MarkLights (dlight_t *light, int bit, mbrush38_node_t *node);

void COM_StripExtension (char *in, char *out);

void	Draw_GetPicSize (int *w, int *h, char *name);
void	Draw_Pic (int x, int y, char *name);
void	Draw_StretchPic (int x, int y, int w, int h, char *name);
void	Draw_Char (int x, int y, int c);
void	Draw_TileClear (int x, int y, int w, int h, char *name);
void	Draw_Fill (int x, int y, int w, int h, int c);
void	Draw_FadeScreen (void);
void	Draw_StretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data);

void	R_BeginFrame( float camera_separation );
void	R_SwapBuffers( int );

image_t *R_RegisterSkinQ2 (char *name);

/*
** GL extension emulation functions
*/
void GL_DrawParticles( int n, const particle_t particles[], const unsigned colortable[768] );

typedef struct
{
	float camera_separation;
} glstate2_t;

extern glstate2_t   gl_state;

/*
====================================================================

IMPORTED FUNCTIONS

====================================================================
*/

extern	refimport_t	ri;

extern QCvar		*vid_ref;
extern qboolean		reflib_active;

refexport_t GetRefAPI (refimport_t rimp);

void VID_Restart_f (void);
void VID_NewWindow ( int width, int height);
void VID_FreeReflib (void);
qboolean VID_LoadRefresh();


/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void		GLimp_BeginFrame( float camera_separation );
void		GLimp_EndFrame( void );
