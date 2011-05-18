// disable data conversion warnings

#ifdef _WIN32
#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86
#pragma warning(disable : 4051)     // ALPHA
#else
#define PROC void*
#endif

#include "../../client/render_local.h"

#include <GL/glu.h>

void GL_BeginRendering (int *x, int *y, int *width, int *height);
void GL_EndRendering (void);


extern	float	gldepthmin, gldepthmax;

#define MAX_EXTRA_TEXTURES 156   // 255-100+1
extern image_t*		gl_extra_textures[MAX_EXTRA_TEXTURES];   // generic textures for models

image_t* GL_LoadTexture(char *identifier, int width, int height, byte *data, qboolean mipmap);

extern	int glx, gly, glwidth, glheight;


// r_local.h -- private refresh defs

#define ALIAS_BASE_SIZE_RATIO		(1.0 / 11.0)
					// normalizing factor so player model works out to about
					//  1 pixel per triangle
#define	MAX_LBM_HEIGHT		480

#define TILE_SIZE		128		// size of textures generated by R_GenTiledSurf

#define SKYSHIFT		7
#define	SKYSIZE			(1 << SKYSHIFT)
#define SKYMASK			(SKYSIZE - 1)

#define BACKFACE_EPSILON	0.01


void R_TimeRefresh_f (void);
void R_ReadPointFile_f (void);
texture_t *R_TextureAnimation (texture_t *base);

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
	pt_darken,
	pt_grensmoke,
	pt_redfire,
	pt_acidball,
	pt_bluestep
} ptype_t;

// Changes to rtype_t must also be made in glquake.h
typedef enum
{
   rt_rocket_trail = 0,
	rt_smoke,
	rt_blood,
	rt_tracer,
	rt_slight_blood,
	rt_tracer2,
	rt_voor_trail,
	rt_fireball,
	rt_ice,
	rt_spit,
	rt_spell,
	rt_vorpal,
	rt_setstaff,
	rt_magicmissile,
	rt_boneshard,
	rt_scarab,
	rt_grensmoke,
	rt_purify,
	rt_darken,
	rt_acidball,
	rt_bloodshot
} rt_type_t;

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct particle_s
{
// driver-usable fields
	vec3_t		org;
	float		color;
// drivers never touch the following fields
	struct particle_s	*next;
	vec3_t		vel;
	float		ramp;
	float		die;
	ptype_t		type;
} particle_t;


//====================================================


extern	entity_t	r_worldentity;
extern	qboolean	r_cache_thrash;		// compatability
extern	vec3_t		modelorg, r_entorigin;
extern	refEntity_t	*currententity;
extern	int			r_visframecount;	// ??? what difs?
extern	cplane_t	frustum[4];
extern	int		c_brush_polys, c_alias_polys;


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
extern	mleaf_t		*r_viewleaf, *r_oldviewleaf;
extern	texture_t	*r_notexture_mip;
extern	int		d_lightstylevalue[256];	// 8.8 fraction of base light value

extern	qboolean	envmap;
extern	image_t*	particletexture;
extern	image_t*	playertextures[MAX_CLIENTS];

extern	int	skytexturenum;		// index in cl.loadmodel, not gl texture object

extern	QCvar*	r_norefresh;
extern	QCvar*	r_drawentities;
extern	QCvar*	r_drawviewmodel;
extern	QCvar*	r_speeds;
extern	QCvar*	r_fullbright;
extern	QCvar*	r_lightmap;
extern	QCvar*	r_shadows;
extern	QCvar*	r_mirroralpha;
extern	QCvar*	r_dynamic;
extern	QCvar*	r_novis;
extern	QCvar*	r_netgraph;
extern	QCvar*	r_entdistance;
extern	QCvar*	r_teamcolor;

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

#define			MAX_VISEDICTS	512
extern	int				cl_numvisedicts;
extern	refEntity_t		cl_visedicts[MAX_VISEDICTS];

void R_TranslatePlayerSkin (int playernum);

void GL_DisableMultitexture(void);
void GL_EnableMultitexture(void);

extern byte *playerTranslation;

void R_DrawName(vec3_t origin, char *Name, int Red);
void GL_Set2D (void);
void Draw_Crosshair(void);
void EmitWaterPolys (msurface_t *fa);
void EmitSkyPolys (msurface_t *fa);
void EmitBothSkyLayers (msurface_t *fa);
void R_DrawSkyChain (msurface_t *s);
qboolean R_CullBox (vec3_t mins, vec3_t maxs);
void R_MarkLights (dlight_t *light, int bit, mnode_t *node);
void R_RotateForEntity (refEntity_t *e);
void R_InitParticles (void);
void R_ClearParticles (void);
void GL_BuildLightmaps (void);
int R_LightPoint (vec3_t p);
void R_DrawBrushModel (refEntity_t *e, qboolean Translucent);
void R_AnimateLight(void);
void V_CalcBlend (void);
void R_DrawWorld (void);
void R_RenderDlights (void);
void R_DrawParticles (void);
void R_DrawWaterSurfaces (void);
int M_DrawBigCharacter (int x, int y, int num, int numNext);
void Draw_RedString (int x, int y, char *str);
void GL_SubdivideSurface (msurface_t *fa);
void R_ParseParticleEffect2 (void);
void R_ParseParticleEffect3 (void);
void R_ParseParticleEffect4 (void);
void R_ParseRainEffect(void);
void R_NetGraph (void);
model_t *Mod_FindName (char *name);
void R_DarkFieldParticles (refEntity_t *ent);
void R_SplashParticleEffect (vec3_t org, float radius, int color, int effect, int count);
void GL_MakeAliasModelDisplayLists (model_t *m, aliashdr_t *hdr);

extern float RTint[256],GTint[256],BTint[256];
