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

void GL_BeginRendering (int *x, int *y, int *width, int *height);
void GL_EndRendering (void);

void GL_SetDefaultState( void );
void GL_UpdateSwapInterval( void );

extern	float	gldepthmin, gldepthmax;

#define BACKFACE_EPSILON	0.01


//====================================================


extern	image_t		*r_particletexture;
extern	trRefEntity_t	*currententity;
extern	int			r_visframecount;
extern	cplane_t	frustum[4];
extern	int			c_brush_polys, c_alias_polys;


//
// view origin
//
extern	vec3_t	vup;
extern	vec3_t	vpn;
extern	vec3_t	vright;
extern	vec3_t	r_origin;

//
// screen size info
//
extern	refdef_t	r_newrefdef;
extern	int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

extern	QCvar	*r_norefresh;
extern	QCvar	*r_lefthand;
extern	QCvar	*r_drawentities;
extern	QCvar	*r_drawworld;
extern	QCvar	*r_speeds;
extern	QCvar	*r_novis;
extern	QCvar	*r_nocull;
extern	QCvar	*r_lerpmodels;

extern	QCvar	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level

extern QCvar	*gl_vertex_arrays;

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
extern	QCvar	*gl_flashblend;
extern	QCvar	*gl_lightmaptype;
extern	QCvar	*gl_modulate;
extern	QCvar	*gl_drawbuffer;
extern  QCvar  *gl_driver;
extern  QCvar  *gl_saturatelighting;
extern  QCvar  *gl_lockpvs;

extern	int		c_visible_lightmaps;
extern	int		c_visible_textures;

extern	float	r_world_matrix[16];

void R_TranslatePlayerSkin (int playernum);
void GL_MBind( int target, image_t* image);
void GL_EnableMultitexture( qboolean enable );

void R_LightPoint (vec3_t p, vec3_t color);
void R_PushDlights (void);

//====================================================================

extern	model_t	*r_worldmodel;

extern	int		registration_sequence;


void V_AddBlend (float r, float g, float b, float a, float *v_blend);

int 	R_Init();
void	R_Shutdown( void );

void R_RenderView (refdef_t *fd);
void GL_ScreenShot_f (void);
void R_DrawAliasModel (trRefEntity_t *e);
void R_DrawBrushModel (trRefEntity_t *e);
void R_DrawSpriteModel (trRefEntity_t *e);
void R_DrawBeam( trRefEntity_t *e );
void R_DrawWorld (void);
void R_RenderDlights (void);
void R_DrawAlphaSurfaces (void);
void R_RenderBrushPoly (mbrush38_surface_t *fa);
void R_InitParticleTexture (void);
void Draw_InitLocal (void);
void GL_SubdivideSurface (mbrush38_surface_t *fa);
qboolean R_CullBox (vec3_t mins, vec3_t maxs);
void R_RotateForEntity (trRefEntity_t *e);
void R_MarkLeaves (void);

mbrush38_glpoly_t *WaterWarpPolyVerts (mbrush38_glpoly_t *p);
void EmitWaterPolys (mbrush38_surface_t *fa);
void R_AddSkySurface (mbrush38_surface_t *fa);
void R_ClearSkyBox (void);
void R_DrawSkyBox (void);
void R_MarkLights (dlight_t *light, int bit, mbrush38_node_t *node);

#if 0
short LittleShort (short l);
short BigShort (short l);
int	LittleLong (int l);
float LittleFloat (float f);

char	*va(char *format, ...);
// does a varargs printf into a temp buffer
#endif

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

image_t *R_RegisterSkin (char *name);

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
