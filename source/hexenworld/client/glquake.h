// disable data conversion warnings

#ifdef _WIN32
#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86
#pragma warning(disable : 4051)     // ALPHA
#else
#define PROC void*
#endif

#include "../../client/render_local.h"
#include "gl_model.h"

#include <GL/glu.h>

void GL_EndRendering (void);


#define MAX_EXTRA_TEXTURES 156   // 255-100+1
extern image_t*		gl_extra_textures[MAX_EXTRA_TEXTURES];   // generic textures for models

// r_local.h -- private refresh defs

#define TILE_SIZE		128		// size of textures generated by R_GenTiledSurf

#define SKYSHIFT		7
#define	SKYSIZE			(1 << SKYSHIFT)
#define SKYMASK			(SKYSIZE - 1)

#define BACKFACE_EPSILON	0.01


void R_TimeRefresh_f (void);
void R_ReadPointFile_f (void);
mbrush29_texture_t *R_TextureAnimation (mbrush29_texture_t *base);

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
} cparticle_t;


//====================================================


extern	qboolean	r_cache_thrash;		// compatability
extern	int		c_brush_polys;


//
// screen size info
//
extern	mbrush29_leaf_t		*r_viewleaf, *r_oldviewleaf;
extern	int		d_lightstylevalue[256];	// 8.8 fraction of base light value

extern	qboolean	envmap;
extern	image_t*	particletexture;
extern	image_t*	playertextures[MAX_CLIENTS];

extern	int	skytexturenum;		// index in cl.loadmodel, not gl texture object

extern	QCvar*	r_norefresh;
extern	QCvar*	r_drawentities;
extern	QCvar*	r_drawviewmodel;
extern	QCvar*	r_speeds;
extern	QCvar*	r_dynamic;
extern	QCvar*	r_novis;
extern	QCvar*	r_netgraph;
extern	QCvar*	r_entdistance;

extern	QCvar*	gl_cull;
extern	QCvar*	gl_polyblend;
extern	QCvar*	gl_nocolors;
extern	QCvar*	gl_reporttjunctions;

extern byte *playerTranslation;

void GL_Set2D (void);
void Draw_Crosshair(void);
void EmitWaterPolys (mbrush29_surface_t *fa);
void R_MarkLights (dlight_t *light, int bit, mbrush29_node_t *node);
void R_InitParticles (void);
void R_ClearParticles (void);
void R_DrawBrushModel (trRefEntity_t *e, qboolean Translucent);
void R_AnimateLight(void);
void V_CalcBlend (void);
void R_DrawWorld (void);
void R_DrawParticles (void);
void R_DrawWaterSurfaces (void);
void Draw_RedString (int x, int y, const char *str);
void R_NetGraph (void);
void R_PushDlights (void);
