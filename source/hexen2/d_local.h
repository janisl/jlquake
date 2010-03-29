// d_local.h:  private rasterization driver defs

/*
 * $Header: /H3/game/D_LOCAL.H 13    8/30/97 6:17p Rjohnson $
 */

#include "r_shared.h"

//
// TODO: fine-tune this; it's based on providing some overage even if there
// is a 2k-wide scan, with subdivision every 8, for 256 spans of 12 bytes each
//
#define SCANBUFFERPAD		0x1000

#define R_SKY_SMASK	0x007F0000
#define R_SKY_TMASK	0x007F0000

#define DS_SPAN_LIST_END	-128

//old surface cache - rj     #define SURFCACHE_SIZE_AT_320X200	600*1024
#define SURFCACHE_SIZE_AT_320X200	768*1024


typedef struct surfcache_s
{
	struct surfcache_s	*next;
	struct surfcache_s 	**owner;		// NULL is an empty chunk of memory
	int					lightadj[MAXLIGHTMAPS]; // checked for strobe flush
	int					dlight;
	int					size;		// including header
	unsigned			width;
	unsigned			height;		// DEBUG only needed for debug
	float				mipscale;
	struct texture_s	*texture;	// checked for animating textures
	int					drawflags;
	int					abslight;
	byte				data[4];	// width*height elements
} surfcache_t;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct sspan_s
{
	int				u, v, count;
} sspan_t;

extern cvar_t	d_subdiv16;

extern float	scale_for_mip;

extern qboolean		d_roverwrapped;
extern surfcache_t	*sc_rover;
extern surfcache_t	*d_initial_rover;

extern float	d_sdivzstepu, d_tdivzstepu, d_zistepu;
extern float	d_sdivzstepv, d_tdivzstepv, d_zistepv;
extern float	d_sdivzorigin, d_tdivzorigin, d_ziorigin;

fixed16_t	sadjust, tadjust;
fixed16_t	bbextents, bbextentt;


void D_DrawSpans8 (espan_t *pspans);
void D_DrawSpans16 (espan_t *pspans);
void D_DrawSpans16T (espan_t *pspans);
void D_DrawZSpans (espan_t *pspans);
void D_DrawSingleZSpans (espan_t *pspans);
void Turbulent8 (surf_t *s);
void D_SpriteDrawSpans (sspan_t *pspan);
void D_SpriteDrawSpansT (sspan_t *pspan);
void D_SpriteDrawSpansT2 (sspan_t *pspan);

void D_DrawSkyScans8 (espan_t *pspan);
void D_DrawSkyScans16 (espan_t *pspan);

void R_ShowSubDiv (void);
void (*prealspandrawer)(void);
surfcache_t	*D_CacheSurface (msurface_t *surface, int miplevel);

extern int D_MipLevelForScale (float scale);

#if id386
extern void D_PolysetAff8Start (void);
extern void D_PolysetAff8End (void);
extern void D_PolysetAff8StartT (void);
extern void D_PolysetAff8EndT (void);
extern void D_PolysetAff8StartT2 (void);
extern void D_PolysetAff8EndT2 (void);
extern void D_PolysetAff8StartT3 (void);
extern void D_PolysetAff8EndT3 (void);
extern void D_PolysetAff8StartT5 (void);
extern void D_PolysetAff8EndT5 (void);
extern void D_Draw16StartT (void);
extern void D_Draw16EndT (void);
extern void D_SpriteSpansStartT (void);
extern void D_SpriteSpansEndT (void);
extern void D_SpriteSpansStartT2 (void);
extern void D_SpriteSpansEndT2 (void);
extern void D_DrawTurbulent8TSpan (void);
extern void D_DrawTurbulent8TQuickSpan (void);
extern void D_DrawTurbulent8TSpanEnd (void);
#endif

extern short *d_pzbuffer;
extern unsigned int d_zrowbytes, d_zwidth;

extern int	*d_pscantable;
extern int	d_scantable[MAXHEIGHT];

extern int	d_vrectx, d_vrecty, d_vrectright_particle, d_vrectbottom_particle;

extern int	d_y_aspect_shift, d_pix_min, d_pix_max, d_pix_shift;

extern pixel_t	*d_viewbuffer;

extern short	*zspantable[MAXHEIGHT];

extern int		d_minmip;
extern float	d_scalemip[3];

extern void (*d_drawspans) (espan_t *pspan);


#define SCAN_SIZE 2048

extern byte			scanList[SCAN_SIZE];


/*
 * $Log: /H3/game/D_LOCAL.H $
 * 
 * 13    8/30/97 6:17p Rjohnson
 * Reduced texture cache
 * 
 * 12    6/12/97 11:08a Rjohnson
 * Only water is translucent now
 * 
 * 11    6/03/97 5:50p Rjohnson
 * Added translucent water
 * 
 * 10    5/22/97 5:56p Rjohnson
 * New translucency effect
 * 
 * 9     5/18/97 1:37p Rjohnson
 * Added new mixed mode sprite assembly
 * 
 * 8     5/18/97 12:50p Rjohnson
 * Sprite translucency is now in assembly
 * 
 * 7     5/15/97 4:42p Rjohnson
 * Minor assembly optimization
 * 
 * 6     4/24/97 11:21p Rjohnson
 * You can now set the overall light level of a bmodel
 * 
 * 5     4/21/97 11:35a Rjohnson
 * Translucency update (drawn in right order) and translucent bmodels
 * 
 * 4     3/28/97 5:28p Rjohnson
 * Updates to the transparency for the models
 * 
 * 3     3/28/97 10:08a Rjohnson
 * Added transparent models
 * 
 * 2     1/07/97 10:03a Rjohnson
 * Increased surface cache size
 */
