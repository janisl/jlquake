//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

enum
{
	MAX_PARTICLES = 7000,
	PARTICLE_GRAVITY = 40,
};

#define INSTANT_PARTICLE	-10000.0

enum ptype_t
{
	pt_q2static,

	pt_q1static,
	pt_q1grav,
	pt_q1slowgrav,
	pt_q1fire,
	pt_q1explode,
	pt_q1explode2,
	pt_q1blob,
	pt_q1blob2,

	pt_h2static,
	pt_h2grav,
	pt_h2fastgrav,
	pt_h2slowgrav,
	pt_h2fire,
	pt_h2explode,
	pt_h2explode2,
	pt_h2blob,
	pt_h2blob2,
	pt_h2rain,
	pt_h2c_explode,
	pt_h2c_explode2,
	pt_h2spit,
	pt_h2fireball,
	pt_h2ice,
	pt_h2spell,
	pt_h2test,
	pt_h2quake,
	pt_h2rd,			// rider's death
	pt_h2vorpal,
	pt_h2setstaff,
	pt_h2magicmissile,
	pt_h2boneshard,
	pt_h2scarab,
	pt_h2acidball,
	pt_h2darken,
	pt_h2snow,
	pt_h2gravwell,
	pt_h2redfire,
	pt_h2grensmoke,
	pt_h2bluestep,
};

//	Hexen 2 snow flags.
enum
{
	SFL_FLUFFY = 1,			// All largish flakes
	SFL_MIXED = 2,			// Mixed flakes
	SFL_HALF_BRIGHT = 4,	// All flakes start darker
	SFL_NO_MELT = 8,		// Flakes don't melt when his surface, just go away
	SFL_IN_BOUNDS = 16,		// Flakes cannot leave the bounds of their box
	SFL_NO_TRANS = 32,		// All flakes start non-translucent
};

enum rt_type_t
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
	rt_grensmoke,
	rt_purify,
};

struct cparticle_t
{
	cparticle_t* next;
	vec3_t org;
	vec3_t vel;
	int color;

	ptype_t type;
	float ramp;
	int die;

	vec3_t minOrg;
	vec3_t maxOrg;
	byte flags;
	byte count;

	int time;
	vec3_t accel;
	float alpha;
	float alphavel;
};

extern cparticle_t particles[MAX_PARTICLES];
extern int cl_numparticles;
extern cparticle_t* active_particles;
extern cparticle_t* free_particles;
extern Cvar* snow_flurry;
extern Cvar* snow_active;
extern vec3_t avelocities[NUMVERTEXNORMALS];

extern int q1ramp1[8];
extern int q1ramp2[8];
extern int q1ramp3[8];

extern int h2ramp1[8];
extern int h2ramp2[8];
extern int h2ramp3[8];
extern int h2ramp4[16];
extern int h2ramp5[16];
extern int h2ramp6[16];
extern int h2ramp7[16];
extern int h2ramp8[16];
extern int h2ramp9[16];
extern int h2ramp10[16];
extern int h2ramp11[8];
extern int h2ramp12[8];
extern int h2ramp13[16];
extern vec3_t rider_origin;

void CL_ClearParticles();
cparticle_t* CL_AllocParticle();

void CLQ1_ParticleExplosion(const vec3_t origin);
void CLQ1_ParticleExplosion2(const vec3_t origin, int colorStart, int colorLength);
void CLQ1_BlobExplosion(const vec3_t origin);
void CLQ1_RunParticleEffect(const vec3_t origin, const vec3_t direction, int colour, int count);
void CLQ1_LavaSplash(const vec3_t origin);
void CLQ1_TeleportSplash(const vec3_t origin);
void CLQ1_BrightFieldParticles(const vec3_t origin);
void CLQ1_TrailParticles(vec3_t start, const vec3_t end, int type);

void CLH2_DarkFieldParticles(const vec3_t origin);
void CLH2_ParticleExplosion(const vec3_t origin);
void CLH2_BlobExplosion(const vec3_t origin);
void CLH2_RunParticleEffect(const vec3_t origin, const vec3_t direction, int count);
void CLH2_RunParticleEffect2(const vec3_t origin, const vec3_t directionMin, const vec3_t directionMax, int colour, ptype_t effect, int count);
void CLH2_RunParticleEffect3(const vec3_t origin, const vec3_t box, int colour, ptype_t effect, int count);
void CLH2_RunParticleEffect4(const vec3_t origin, float radius, int colour, ptype_t effect, int count);
void CLH2_SplashParticleEffect(const vec3_t origin, float radius, int colour, ptype_t effect, int count);
void CLH2_LavaSplash(const vec3_t origin);
void CLH2_TeleportSplash(const vec3_t origin);
void CLH2_RunQuakeEffect(const vec3_t origin, float distance);
void CLH2_SunStaffTrail(vec3_t source, const vec3_t destination);
void CLH2_RiderParticles(int count, const vec3_t origin);
void CLH2_GravityWellParticle(int count, const vec3_t origin, int colour);
void CLH2_RainEffect(const vec3_t origin, const vec3_t size, int xDirection, int yDirection, int colour, int count);
void CLH2_RainEffect2(const vec3_t origin, const vec3_t size, int xDirection, int yDirection, int colour, int count);
void CLH2_SnowEffect(const vec3_t minOrigin, const vec3_t maxOrigin, int flags, const vec3_t direction, int count);
void CLH2_ColouredParticleExplosion(const vec3_t origin, int colour, int radius, int count);
void CLH2_TrailParticles(vec3_t start, const vec3_t end, int type);
void CLHW_BrightFieldParticles(vec3_t origin);
void CLHW_SuccubusInvincibleParticles(vec3_t origin);
void CLHW_TargetBallEffectParticles(vec3_t origin, float targetDistance);

void CLQ2_ParticleEffect(vec3_t origin, vec3_t direction, int colour, int count);
void CLQ2_ParticleEffect2(vec3_t origin, vec3_t direction, int colour, int count);
void CLQ2_ParticleEffect3(vec3_t origin, vec3_t direction, int colour, int count);
void CLQ2_TeleporterParticles(vec3_t origin);
void CLQ2_PlayerSpawnParticles(vec3_t origin, int baseColour);
void CLQ2_ItemRespawnParticles(vec3_t origin);
void CLQ2_ExplosionParticles(vec3_t origin);
void CLQ2_BigTeleportParticles(vec3_t origin);
void CLQ2_BlasterParticles(vec3_t origin, vec3_t direction);
void CLQ2_BlasterTrail(vec3_t start, vec3_t end);
void CLQ2_FlagTrail(vec3_t start, vec3_t end, float colour);
void CLQ2_DiminishingTrail(vec3_t start, vec3_t end, q2centity_t* old, int flags);
void CLQ2_RocketTrail(vec3_t start, vec3_t end, q2centity_t* old);
void CLQ2_RailTrail(vec3_t start, vec3_t end);
void CLQ2_IonripperTrail(vec3_t start, vec3_t end);
void CLQ2_BubbleTrail(vec3_t start, vec3_t end);
void CLQ2_FlyParticles(vec3_t origin, int count);
void CLQ2_BfgParticles(vec3_t origin);
void CLQ2_BFGExplosionParticles(vec3_t origin);
void CLQ2_TeleportParticles(vec3_t origin);
void CLQ2_DebugTrail(vec3_t start, vec3_t end);
void CLQ2_ForceWall(vec3_t start, vec3_t end, int colour);
void CLQ2_BubbleTrail2(vec3_t start, vec3_t end, int distance);
void CLQ2_HeatbeamPaticles(vec3_t start, vec3_t forward);
void CLQ2_ParticleSteamEffect(vec3_t origin, vec3_t direction, int colour, int count, int magnitude);
void CLQ2_ParticleSteamEffect2(q2cl_sustain_t* self);
void CLQ2_TrackerTrail(vec3_t start, vec3_t end, int particleColour);
void CLQ2_Tracker_Shell(vec3_t origin);
void CLQ2_MonsterPlasma_Shell(vec3_t origin);
void CLQ2_Widowbeamout(q2cl_sustain_t* self);
void CLQ2_Nukeblast(q2cl_sustain_t* self);
void CLQ2_WidowSplash(vec3_t origin);
void CLQ2_Tracker_Explode(vec3_t origin);
void CLQ2_TagTrail(vec3_t start, vec3_t end, int colour);
void CLQ2_ColorExplosionParticles(vec3_t origin, int colour, int run);
void CLQ2_ParticleSmokeEffect(vec3_t origin, vec3_t direction, int colour, int count, int magnitude);
void CLQ2_BlasterParticles2(vec3_t origin, vec3_t direction, unsigned int colour);
void CLQ2_BlasterTrail2(vec3_t start, vec3_t end);
