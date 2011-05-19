
// refresh.h -- public interface to refresh functions

/*
 * $Header: /H2 Mission Pack/RENDER.H 4     3/05/98 7:54p Jmonroe $
 */

#define	MAXCLIPPLANES	11

#define	TOP_RANGE		16			// soldier uniform colors
#define	BOTTOM_RANGE	96

// EF_ changes must also be made in model.h

#define	EF_ROCKET		       1			// leave a trail
#define	EF_GRENADE		       2			// leave a trail
#define	EF_GIB			       4			// leave a trail
#define	EF_ROTATE		       8			// rotate (bonus items)
#define	EF_TRACER		      16			// green split trail
#define	EF_ZOMGIB		      32			// small blood trail
#define	EF_TRACER2			  64			// orange split trail + rotate
#define	EF_TRACER3			 128			// purple trail
#define  EF_FIREBALL		 256			// Yellow transparent trail in all directions
#define  EF_ICE				 512			// Blue-white transparent trail, with gravity
#define  EF_MIP_MAP			1024			// This model has mip-maps
#define  EF_SPIT			2048			// Black transparent trail with negative light
#define  EF_TRANSPARENT		4096			// Transparent sprite
#define  EF_SPELL           8192			// Vertical spray of particles
#define  EF_HOLEY		   16384			// Solid model with color 0
#define  EF_SPECIAL_TRANS  32768			// Translucency through the particle table
#define  EF_FACE_VIEW	   65536			// Poly Model always faces you
#define  EF_VORP_MISSILE  131072			// leave a trail at top and bottom of model
#define  EF_SET_STAFF     262144			// slowly move up and left/right
#define  EF_MAGICMISSILE  524288            // a trickle of blue/white particles with gravity
#define  EF_BONESHARD    1048576           // a trickle of brown particles with gravity
#define  EF_SCARAB       2097152           // white transparent particles with little gravity
#define  EF_ACIDBALL	 4194304			// Green drippy acid shit
#define  EF_BLOODSHOT	 8388608			// Blood rain shot trail

#define  EF_MIP_MAP_FAR	  0x1000000	// Set per frame, this model will use the far mip map

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
	rt_acidball,
	rt_bloodshot,
} rt_type_t;

//=============================================================================

struct entity_t
{
	qboolean				forcelink;		// model changed

	entity_state3_t			baseline;		// to fill in defaults in updates

	double					msgtime;		// time of last update
	vec3_t					msg_origins[2];	// last two updates (0 is newest)	
	vec3_t					origin;
	vec3_t					msg_angles[2];	// last two updates (0 is newest)
	vec3_t					angles;
	qhandle_t				model;			// 0 = no model
	int						frame;
	float					syncbase;		// for client-side animations
	byte					*colormap, *sourcecolormap;
	byte					colorshade;
	int						effects;		// light, particals, etc
	int						skinnum;		// for Alias models
	int						scale;			// for Alias models
	int						drawflags;		// for Alias models
	int						abslight;		// for Alias models
};

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	vrect_t		vrect;				// subwindow in video for refresh
									// FIXME: not need vrect next field here?
	vrect_t		aliasvrect;			// scaled Alias version
	int			vrectright, vrectbottom;	// right & bottom screen coords
	int			aliasvrectright, aliasvrectbottom;	// scaled Alias versions
	float		vrectrightedge;			// rightmost right edge we care about,
										//  for use in edge list
	float		fvrectx, fvrecty;		// for floating-point compares
	float		fvrectx_adj, fvrecty_adj; // left and top edges, for clamping
	int			vrect_x_adj_shift20;	// (vrect.x + 0.5 - epsilon) << 20
	int			vrectright_adj_shift20;	// (vrectright + 0.5 - epsilon) << 20
	float		fvrectright_adj, fvrectbottom_adj;
										// right and bottom edges, for clamping
	float		fvrectright;			// rightmost edge, for Alias clamping
	float		fvrectbottom;			// bottommost edge, for Alias clamping
	float		horizontalFieldOfView;	// at Z = 1.0, this many X is visible 
										// 2.0 = 90 degrees
	float		xOrigin;			// should probably allways be 0.5
	float		yOrigin;			// between be around 0.3 to 0.5

	vec3_t		vieworg;
	vec3_t		viewangles;
	
	int			ambientlight;
} refdef_t;


//
// refresh
//
extern	int		reinit_surfcache;


extern	refdef_t	r_refdef;
extern vec3_t	r_origin;
extern "C"
{
extern vec3_t	vpn, vright, vup;
}

extern	struct texture_s	*r_notexture_mip;

extern float RTint[256],GTint[256],BTint[256];

extern	int		d_lightstylevalue[256];	// 8.8 fraction of base light value

void R_Init (void);
void R_InitTextures (void);
void R_InitEfrags (void);
void R_RenderView (void);		// must set r_refdef first
void R_ViewChanged (vrect_t *pvrect, int lineadj, float aspect);
								// called whenever r_refdef or vid change
void R_InitSky (struct texture_s *mt);	// called at level load

void R_NewMap (void);


void R_ParseParticleEffect (void);
void R_ParseParticleEffect2 (void);
void R_ParseParticleEffect3 (void);
void R_ParseParticleEffect4 (void);
void R_RunParticleEffect (vec3_t org, vec3_t dir, int color, int count);
void R_RocketTrail (vec3_t start, vec3_t end, int type);
void R_SunStaffTrail(vec3_t source, vec3_t dest);

void R_DarkFieldParticles (entity_t *ent);
void R_RainEffect (vec3_t org,vec3_t e_size,int x_dir, int y_dir,int color,int count);
void R_SnowEffect (vec3_t org1,vec3_t org2,int flags,vec3_t alldir,int count);
void R_ColoredParticleExplosion (vec3_t org,int color,int radius,int counter);

void R_EntityParticles (entity_t *ent);
void R_BlobExplosion (vec3_t org);
void R_ParticleExplosion (vec3_t org);
void R_ParticleExplosion2 (vec3_t org, int colorStart, int colorLength);
void R_LavaSplash (vec3_t org);
void R_TeleportSplash (vec3_t org);

void R_PushDlights (void);


//
// surface cache related
//
extern	int		reinit_surfcache;	// if 1, surface cache is currently empty and
extern qboolean	r_cache_thrash;	// set if thrashing the surface cache

int	D_SurfaceCacheForRes (int width, int height);
void D_DeleteSurfaceCache (void);
void D_InitCaches (void *buffer, int size);
void R_SetVrect (vrect_t *pvrect, vrect_t *pvrectin, int lineadj);

void R_RunParticleEffect2 (vec3_t org, vec3_t dmin, vec3_t dmax, int color, int effect, int count);
void R_ClearParticles (void);

void Mod_CalcScaleOffset(qhandle_t Handle, float ScaleX, float ScaleY, float ScaleZ, float ScaleZOrigin, vec3_t Out);
void R_HandleCustomSkin(refEntity_t* Ent, int PlayerNum);
void R_ClearScene();
void R_AddRefEntToScene(refEntity_t* Ent);
void R_TranslatePlayerSkin (int playernum);
void D_ShowLoadingSize(void);

void	Mod_Init (void);
void	Mod_ClearAll (void);
qhandle_t Mod_ForName (char *name, qboolean crash);
int Mod_GetNumFrames(qhandle_t Handle);
int Mod_GetFlags(qhandle_t Handle);
void Mod_PrintFrameName(qhandle_t m, int frame);
bool Mod_IsAliasModel(qhandle_t Handle);
const char* Mod_GetName(qhandle_t Handle);
int Mod_GetSyncType(qhandle_t Handle);
