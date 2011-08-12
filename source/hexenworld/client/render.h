
// refresh.h -- public interface to refresh functions

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
	byte					colorshade;
	int						skinnum;		// for Alias models
	int						scale;			// for Alias models
	int						drawflags;		// for Alias models
	int						abslight;		// for Alias models

	struct player_info_s	*scoreboard;	// identify player

	float					syncbase;
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
void R_ParticleExplosion (vec3_t org);
void R_ParticleExplosion2 (vec3_t org, int colorStart, int colorLength);
void R_BlobExplosion (vec3_t org);
void R_RunParticleEffect (vec3_t org, vec3_t dir, int color, int count);
void R_RunParticleEffect2 (vec3_t org, vec3_t dmin, vec3_t dmax, int color, ptype_t effect, int count);
void R_RunParticleEffect3 (vec3_t org, vec3_t box, int color, ptype_t effect, int count);
void R_RunParticleEffect4 (vec3_t org, float radius, int color, ptype_t effect, int count);
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

void R_SplashParticleEffect (vec3_t org, float radius, int color, int effect, int count);
void R_DarkFieldParticles (refEntity_t *ent);
void R_ParseParticleEffect2 (void);
void R_ParseParticleEffect3 (void);
void R_ParseParticleEffect4 (void);
void R_ParseRainEffect(void);
void R_DrawName(vec3_t origin, char *Name, int Red);
int M_DrawBigCharacter (int x, int y, int num, int numNext);
void R_InitParticles (void);
void R_ClearParticles (void);
