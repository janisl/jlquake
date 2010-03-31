// d_iface.h: interface header file for rasterization driver modules

#define WARP_WIDTH		320
#define WARP_HEIGHT		200

#define MAX_LBM_HEIGHT	480

typedef struct
{
	float	u, v;
	float	s, t;
	float	zi;
} emitpoint_t;

// Changes to ptype_t must also be made in glquake.h
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

#define PARTICLE_Z_CLIP	8.0

typedef struct polyvert_s {
	float	u, v, zi, s, t;
} polyvert_t;

typedef struct polydesc_s {
	int			numverts;
	float		nearzi;
	msurface_t	*pcurrentface;
	polyvert_t	*pverts;
} polydesc_t;

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct finalvert_s {
	int		v[6];		// u, v, s, t, l, 1/z
	int		flags;
	float	reserved;
} finalvert_t;

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct
{
	void				*pskin;
	maliasskindesc_t	*pskindesc;
	int					skinwidth;
	int					skinheight;
	mtriangle_t			*ptriangles;
	finalvert_t			*pfinalverts;
	int					numtriangles;
	int					drawtype;
	int					seamfixupX16;
} affinetridesc_t;

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct {
	float	u, v, zi, color;
} screenpart_t;

typedef struct
{
	int			nump;
	emitpoint_t	*pverts;	// there's room for an extra element at [nump], 
							//  if the driver wants to duplicate element [0] at
							//  element [nump] to avoid dealing with wrapping
	mspriteframe_t	*pspriteframe;
	vec3_t			vup, vright, vpn;	// in worldspace
	float			nearzi;
} spritedesc_t;

typedef struct
{
	int		u, v;
	float	zi;
	int		color;
} zpointdesc_t;

extern cvar_t	r_drawflat;
extern int		d_spanpixcount;
extern int		r_framecount;		// sequence # of current frame since Quake
									//  started
extern qboolean	r_drawpolys;		// 1 if driver wants clipped polygons
									//  rather than a span list
extern qboolean	r_drawculledpolys;	// 1 if driver wants clipped polygons that
									//  have been culled by the edge list
extern qboolean	r_worldpolysbacktofront;	// 1 if driver wants polygons
											//  delivered back to front rather
											//  than front to back
extern qboolean	r_recursiveaffinetriangles;	// true if a driver wants to use
											//  recursive triangular subdivison
											//  and vertex drawing via
											//  D_PolysetDrawFinalVerts() past
											//  a certain distance (normally 
											//  only used by the software
											//  driver)
extern float	r_aliasuvscale;		// scale-up factor for screen u and v
									//  on Alias vertices passed to driver
extern int		r_pixbytes;
extern qboolean	r_dowarp;

extern affinetridesc_t	r_affinetridesc;
extern spritedesc_t		r_spritedesc;
extern zpointdesc_t		r_zpointdesc;
extern polydesc_t		r_polydesc;

extern int		d_con_indirect;	// if 0, Quake will draw console directly
								//  to vid.buffer; if 1, Quake will
								//  draw console via D_DrawRect. Must be
								//  defined by driver

extern vec3_t	r_pright, r_pup, r_ppn;

void D_PolysetDrawT (void);
void D_PolysetDrawFinalVertsT (finalvert_t *p1, finalvert_t *p2, finalvert_t *p3);
void D_PolysetDrawT2 (void);
void D_PolysetDrawFinalVertsT2 (finalvert_t *p1, finalvert_t *p2, finalvert_t *p3);

void D_Aff8Patch (void *pcolormap);
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height);
void D_DisableBackBufferAccess (void);
void D_EndDirectRect (int x, int y, int width, int height);
void D_PolysetDraw (void);
void D_PolysetDrawFinalVerts (finalvert_t *p1, finalvert_t *p2, finalvert_t *p3);
void D_DrawParticle (particle_t *pparticle);
void D_DrawParticle1x1b (particle_t *pparticle);
void D_DrawPoly (void);
void D_DrawSprite (void);
void D_DrawSurfaces (qboolean Translucent);
void D_DrawZPoint (void);
void D_EnableBackBufferAccess (void);
void D_EndParticles (void);
void D_Init (void);
void D_ViewChanged (void);
void D_SetupFrame (void);
void D_StartParticles (void);
void D_TurnZOn (void);
void D_WarpScreen (void);

void D_FillRect (vrect_t *vrect, int color);
void D_DrawRect (void);
void D_UpdateRects (vrect_t *prect);

// currently for internal use only, and should be a do-nothing function in
// hardware drivers
// FIXME: this should go away
void D_PolysetUpdateTables (void);

// these are currently for internal use only, and should not be used by drivers
extern int				r_skydirect;
extern byte				*r_skysource;

// transparency types for D_DrawRect ()
#define DR_SOLID		0
#define DR_TRANSPARENT	1

// !!! must be kept the same as in quakeasm.h !!!
#define TRANSPARENT_COLOR	0xFF

extern void *acolormap;	// FIXME: should go away

//=======================================================================//

// callbacks to Quake

typedef struct
{
	pixel_t		*surfdat;	// destination for generated surface
	int			rowbytes;	// destination logical width in bytes
	msurface_t	*surf;		// description for surface to generate
	fixed8_t	lightadj[MAXLIGHTMAPS];
							// adjust for lightmap levels for dynamic lighting
	texture_t	*texture;	// corrected for animating textures
	int			surfmip;	// mipmapped ratio of surface texels / world pixels
	int			surfwidth;	// in mipmapped texels
	int			surfheight;	// in mipmapped texels
} drawsurf_t;

extern drawsurf_t	r_drawsurf;

void R_DrawSurface (void);
void R_GenTile (msurface_t *psurf, void *pdest);


// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define TURB_TEX_SIZE	64		// base turbulent texture size

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define	CYCLE			128		// turbulent cycle size

#define TILE_SIZE		128		// size of textures generated by R_GenTiledSurf

#define SKYSHIFT		7
#define	SKYSIZE			(1 << SKYSHIFT)
#define SKYMASK			(SKYSIZE - 1)

extern float	skyspeed, skyspeed2;
extern float	skytime;

extern int		c_surf;
extern vrect_t	scr_vrect;

extern byte		*r_warpbuffer;

