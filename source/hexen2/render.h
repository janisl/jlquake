
// refresh.h -- public interface to refresh functions

/*
 * $Header: /H2 Mission Pack/RENDER.H 4     3/05/98 7:54p Jmonroe $
 */

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
	byte					colorshade;
	int						effects;		// light, particals, etc
	int						skinnum;		// for Alias models
	int						scale;			// for Alias models
	int						drawflags;		// for Alias models
	int						abslight;		// for Alias models
};


//
// refresh
//
extern	refdef_t	r_refdef;

extern float RTint[256],GTint[256],BTint[256];

void V_RenderScene (void);		// must set r_refdef first
void R_ViewChanged (vrect_t *pvrect, int lineadj, float aspect);
								// called whenever r_refdef or vid change

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

void R_RunParticleEffect2 (vec3_t org, vec3_t dmin, vec3_t dmax, int color, ptype_t effect, int count);
void R_InitParticles (void);
void R_ClearParticles (void);

int M_DrawBigCharacter (int x, int y, int num, int numNext);
