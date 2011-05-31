/*
 * $Header: /H2 Mission Pack/glquake.h 10    3/09/98 11:24p Mgummelt $
 */

// disable data conversion warnings

#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86
#pragma warning(disable : 4051)     // ALPHA
  
#ifndef _WIN32
#define PROC void*
#endif

#include "../client/render_local.h"
#include "gl_model.h"

#include <GL/glu.h>

void GL_BeginRendering (int *x, int *y, int *width, int *height);
void GL_EndRendering (void);

extern	float	gldepthmin, gldepthmax;

#define MAX_EXTRA_TEXTURES 156   // 255-100+1
extern image_t*		gl_extra_textures[MAX_EXTRA_TEXTURES];   // generic textures for models

image_t* GL_LoadTexture(char *identifier, int width, int height, byte *data, qboolean mipmap);

extern	int glx, gly, glwidth, glheight;


// r_local.h -- private refresh defs

#define TILE_SIZE		128		// size of textures generated by R_GenTiledSurf

#define SKYSHIFT		7
#define	SKYSIZE			(1 << SKYSHIFT)
#define SKYMASK			(SKYSIZE - 1)

#define BACKFACE_EPSILON	0.01


void R_TimeRefresh_f (void);
void R_ReadPointFile_f (void);
mbrush29_texture_t *R_TextureAnimation (mbrush29_texture_t* base);

// Changes to ptype_t must also be made in d_iface.h
typedef enum {
	pt_static,
	pt_grav,
	pt_fastgrav,
	pt_slowgrav,
	pt_fire,
	pt_explode,
	pt_explode2,
	pt_blob,
	pt_blob2,
	pt_rain,
	pt_c_explode,
	pt_c_explode2,
	pt_spit,
	pt_fireball,
	pt_ice,
	pt_spell,
	pt_test,
	pt_quake,
	pt_rd,			// rider's death
	pt_vorpal,
	pt_setstaff,
	pt_magicmissile,
	pt_boneshard,
	pt_scarab,
	pt_acidball,
	pt_darken,
	pt_snow,
	pt_gravwell,
	pt_redfire
} ptype_t;

// !!! if this is changed, it must be changed in d_iface.h too !!!
typedef struct particle_s
{
// driver-usable fields
	vec3_t		org;
	float		color;
// drivers never touch the following fields
	struct particle_s	*next;
	vec3_t		vel;
	vec3_t		min_org;
	vec3_t		max_org;
	float		ramp;
	float		die;
	byte		type;
	byte		flags;
	byte		count;
} particle_t;


//====================================================


extern	entity_t	r_worldentity;
extern	qboolean	r_cache_thrash;		// compatability
extern	vec3_t		modelorg, r_entorigin;
extern	trRefEntity_t	*currententity;
extern	int			r_visframecount;	// ??? what difs?
extern	cplane_t	frustum[4];
extern	int		c_brush_polys, c_alias_polys, c_sky_polys;


//
// view origin
//
extern "C"
{
extern	vec3_t	vup;
extern	vec3_t	vpn;
extern	vec3_t	vright;
}
extern	vec3_t	r_origin;

//
// screen size info
//
extern	refdef_t	r_refdef;
extern	mbrush29_leaf_t		*r_viewleaf, *r_oldviewleaf;

extern	qboolean	envmap;
extern	image_t*	particletexture;
extern	image_t*	playertextures[16];

extern	int	skytexturenum;		// index in cl.loadmodel, not gl texture object

extern	QCvar*	r_norefresh;
extern	QCvar*	r_drawentities;
extern	QCvar*	r_drawviewmodel;
extern	QCvar*	r_speeds;
extern	QCvar*	r_shadows;
extern	QCvar*	r_mirroralpha;
extern	QCvar*	r_dynamic;
extern	QCvar*	r_novis;
extern	QCvar*	r_wholeframe;

extern	QCvar*	gl_clear;
extern	QCvar*	gl_cull;
extern	QCvar*	gl_texsort;
extern	QCvar*	gl_smoothmodels;
extern	QCvar*	gl_affinemodels;
extern	QCvar*	gl_polyblend;
extern	QCvar*	gl_flashblend;
extern	QCvar*	gl_nocolors;
extern	QCvar*	gl_keeptjunctions;
extern	QCvar*	gl_reporttjunctions;

extern	int			mirrortexturenum;	// quake texturenum, not gltexturenum
extern	qboolean	mirror;
extern	cplane_t	*mirror_plane;

extern	float	r_world_matrix[16];

#define			MAX_VISEDICTS	256
extern	int				cl_numvisedicts;
extern	trRefEntity_t		cl_visedicts[MAX_VISEDICTS];

extern byte *playerTranslation;

void GL_SubdivideSurface (mbrush29_surface_t *fa);
int R_LightPoint (vec3_t p);
void R_DrawBrushModel (trRefEntity_t *e, qboolean Translucent);
void R_AnimateLight(void);
void V_CalcBlend (void);
void R_DrawWorld (void);
void R_RenderDlights (void);
void R_DrawParticles (void);
void R_DrawWaterSurfaces (void);
void R_RenderBrushPoly (mbrush29_surface_t *fa, qboolean override);
void R_InitParticles (void);
void GL_BuildLightmaps (void);
void EmitWaterPolys (mbrush29_surface_t *fa);
void EmitSkyPolys (mbrush29_surface_t *fa, qboolean save);
void EmitBothSkyLayers (mbrush29_surface_t *fa);
void R_DrawSkyChain (mbrush29_surface_t *s);
qboolean R_CullBox (vec3_t mins, vec3_t maxs);
void R_MarkLights (dlight_t *light, int bit, mbrush29_node_t *node);
void R_RotateForEntity (trRefEntity_t *e);
void GL_Set2D (void);
void SCR_DrawLoading (void);
void R_InitSky (mbrush29_texture_t *mt);	// called at level load

extern qboolean	vid_initialized;
