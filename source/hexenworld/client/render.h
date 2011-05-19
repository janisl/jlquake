
// refresh.h -- public interface to refresh functions

#define	TOP_RANGE		16			// soldier uniform colors
#define	BOTTOM_RANGE	96

// EF_ changes must also be made in model.h

#define	EF_ROCKET		       1			// leave a trail
#define	EF_GRENADE		       2			// leave a trail
#define	EF_GIB			       4			// leave a trail
#define	EF_ROTATE		       8			// rotate (bonus items)
#define	EF_TRACER		      16			// green split trail
#define	EF_ZOMGIB			  32			// small blood trail
#define	EF_TRACER2			  64			// orange split trail + rotate
#define	EF_TRACER3			 128			// purple trail
#define  EF_FIREBALL		 256			// Yellow transparent trail in all directions
#define  EF_ICE				 512			// Blue-white transparent trail, with gravity
#define  EF_MIP_MAP			1024			// This model has mip-maps
#define  EF_SPIT			2048			// Black transparent trail with negative light
#define  EF_TRANSPARENT		4096		    // Transparent sprite
#define  EF_SPELL			8192			// Vertical spray of particles
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

//=============================================================================

struct entity_t
{
	vec3_t					origin;
	vec3_t					angles;
	vec3_t					angleAdd;		// For clientside rotation stuff
	qhandle_t				model;			// 0 = no model
	int						frame;
	byte					*colormap;
	byte					colorshade;
	int						skinnum;		// for Alias models
	int						scale;			// for Alias models
	int						drawflags;		// for Alias models
	int						abslight;		// for Alias models

	struct player_info_s	*scoreboard;	// identify player

	float					syncbase;
};

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	int			x,y,width,height;	// subwindow in video for refresh
	vec3_t		vieworg;
	vec3_t		viewangles;
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

extern	entity_t	r_worldentity;

extern	QCvar*	r_teamcolor;

extern float RTint[256],GTint[256],BTint[256];

void R_Init (void);
void R_InitTextures (void);
void R_InitEfrags (void);
void R_RenderView (void);		// must set r_refdef first
void R_ViewChanged (vrect_t *pvrect, int lineadj, float aspect);
								// called whenever r_refdef or vid change
void R_InitSky (struct texture_s *mt);	// called at level load

void R_NewMap (void);


void R_ParseParticleEffect (void);
void R_ParticleExplosion (vec3_t org);
void R_ParticleExplosion2 (vec3_t org, int colorStart, int colorLength);
void R_BlobExplosion (vec3_t org);
void R_RunParticleEffect (vec3_t org, vec3_t dir, int color, int count);
void R_RunParticleEffect2 (vec3_t org, vec3_t dmin, vec3_t dmax, int color, int effect, int count);
void R_RunParticleEffect3 (vec3_t org, vec3_t box, int color, int effect, int count);
void R_RunParticleEffect4 (vec3_t org, float radius, int color, int effect, int count);
void R_LavaSplash (vec3_t org);
void R_TeleportSplash (vec3_t org);
void R_RunQuakeEffect (vec3_t org, float distance);
void R_SunStaffTrail(vec3_t source, vec3_t dest);
void RiderParticle(int count, vec3_t origin);
void R_RocketTrail (vec3_t start, vec3_t end, int type);
void R_RainEffect (vec3_t org,vec3_t e_size,int x_dir, int y_dir,int color,int count);
void R_RainEffect2 (vec3_t org,vec3_t e_size,int x_dir, int y_dir,int color,int count);
void R_ColoredParticleExplosion (vec3_t org,int color,int radius,int counter);

void R_EntityParticles (entity_t *ent);
void R_SuccubusInvincibleParticles (refEntity_t *ent);
void R_BlobExplosion (vec3_t org);
void R_ParticleExplosion (vec3_t org);
void R_LavaSplash (vec3_t org);
void R_TeleportSplash (vec3_t org);
void R_TargetBallEffect (vec3_t org);
void R_BrightFieldSource (vec3_t org);

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

void Mod_CalcScaleOffset(qhandle_t Handle, float ScaleX, float ScaleY, float ScaleZ, float ScaleZOrigin, vec3_t Out);
void R_HandleCustomSkin(refEntity_t* Ent, int PlayerNum);
float R_CalcEntityLight(refEntity_t* e);
void R_ClearScene();
void R_AddRefEntToScene(refEntity_t* Ent);
void R_TranslatePlayerSkin (int playernum);
void R_SplashParticleEffect (vec3_t org, float radius, int color, int effect, int count);
void R_DarkFieldParticles (refEntity_t *ent);
void R_ParseParticleEffect2 (void);
void R_ParseParticleEffect3 (void);
void R_ParseParticleEffect4 (void);
void R_ParseRainEffect(void);
void R_DrawName(vec3_t origin, char *Name, int Red);
int M_DrawBigCharacter (int x, int y, int num, int numNext);

void	Mod_Init (void);
void	Mod_ClearAll (void);
qhandle_t Mod_ForName (char *name, qboolean crash);
int Mod_GetNumFrames(qhandle_t Handle);
int Mod_GetFlags(qhandle_t Handle);
bool Mod_IsAliasModel(qhandle_t Handle);
const char* Mod_GetName(qhandle_t Handle);
