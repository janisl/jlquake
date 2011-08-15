
//**************************************************************************
//**
//** cl_effect.c
//**
//** Client side effects.
//**
//** $Header: /HexenWorld/Client/cl_effect.c 89    5/25/98 1:29p Mgummelt $
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "quakedef.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

#define MAX_EFFECT_ENTITIES		256

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------
extern void CreateStream(int type, int ent, int flags, int tag, float duration, int skin, vec3_t source, vec3_t dest);
extern void CLTENT_XbowImpact(vec3_t pos, vec3_t vel, int chType, int damage, int arrowType);//so xbow effect can use tents
extern void CLTENT_SpawnDeathBubble(vec3_t pos);
entity_state_t *FindState(int EntNum);
int	TempSoundChannel();
void setseed(unsigned int seed);
float seedrand(void);

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static int NewEffectEntity(void);
static void FreeEffectEntity(int index);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static entity_t EffectEntities[MAX_EFFECT_ENTITIES];
static qboolean EntityUsed[MAX_EFFECT_ENTITIES];
static int EffectEntityCount;

// CODE --------------------------------------------------------------------

static void vectoangles(vec3_t vec, vec3_t ang)
{
	float	forward;
	float	yaw, pitch;
	
	if (vec[1] == 0 && vec[0] == 0)
	{
		yaw = 0;
		if (vec[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		yaw = (int) (atan2(vec[1], vec[0]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;

		forward = sqrt (vec[0]*vec[0] + vec[1]*vec[1]);
		pitch = (int) (atan2(vec[2], forward) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}

	ang[0] = pitch;
	ang[1] = yaw;
	ang[2] = 0;
}


//==========================================================================
//
// CL_InitTEnts
//
//==========================================================================

static sfxHandle_t	cl_fxsfx_bone;
static sfxHandle_t	cl_fxsfx_bonefpow;
static sfxHandle_t	cl_fxsfx_xbowshoot;
static sfxHandle_t	cl_fxsfx_xbowfshoot;
static sfxHandle_t	cl_fxsfx_explode;
static sfxHandle_t	cl_fxsfx_mmfire;
static sfxHandle_t	cl_fxsfx_eidolon;
static sfxHandle_t	cl_fxsfx_scarabwhoosh;
static sfxHandle_t	cl_fxsfx_scarabgrab;
static sfxHandle_t	cl_fxsfx_scarabhome;
static sfxHandle_t	cl_fxsfx_scarabrip;
static sfxHandle_t	cl_fxsfx_scarabbyebye;
static sfxHandle_t	cl_fxsfx_ravensplit;
static sfxHandle_t	cl_fxsfx_ravenfire;
static sfxHandle_t	cl_fxsfx_ravengo;
static sfxHandle_t	cl_fxsfx_drillashoot;
static sfxHandle_t	cl_fxsfx_drillaspin;
static sfxHandle_t	cl_fxsfx_drillameat;


static sfxHandle_t	cl_fxsfx_arr2flsh;
static sfxHandle_t	cl_fxsfx_arr2wood;
static sfxHandle_t	cl_fxsfx_met2stn;

static sfxHandle_t	cl_fxsfx_ripple;
static sfxHandle_t	cl_fxsfx_splash;

void CL_InitEffects(void)
{
	cl_fxsfx_bone = S_RegisterSound("necro/bonefnrm.wav");
	cl_fxsfx_bonefpow = S_RegisterSound("necro/bonefpow.wav");
	cl_fxsfx_xbowshoot = S_RegisterSound("assassin/firebolt.wav");
	cl_fxsfx_xbowfshoot = S_RegisterSound("assassin/firefblt.wav");
	cl_fxsfx_explode = S_RegisterSound("weapons/explode.wav");
	cl_fxsfx_mmfire = S_RegisterSound("necro/mmfire.wav");
	cl_fxsfx_eidolon = S_RegisterSound("eidolon/spell.wav");
	cl_fxsfx_scarabwhoosh = S_RegisterSound("misc/whoosh.wav");
	cl_fxsfx_scarabgrab = S_RegisterSound("assassin/chn2flsh.wav");
	cl_fxsfx_scarabhome = S_RegisterSound("assassin/chain.wav");
	cl_fxsfx_scarabrip = S_RegisterSound("assassin/chntear.wav");
	cl_fxsfx_scarabbyebye = S_RegisterSound("items/itmspawn.wav");
	cl_fxsfx_ravensplit = S_RegisterSound("raven/split.wav");
	cl_fxsfx_ravenfire = S_RegisterSound("raven/rfire1.wav");
	cl_fxsfx_ravengo = S_RegisterSound("raven/ravengo.wav");
	cl_fxsfx_drillashoot = S_RegisterSound("assassin/pincer.wav");
	cl_fxsfx_drillaspin = S_RegisterSound("assassin/spin.wav");
	cl_fxsfx_drillameat = S_RegisterSound("assassin/core.wav");

	cl_fxsfx_arr2flsh = S_RegisterSound("assassin/arr2flsh.wav");
	cl_fxsfx_arr2wood = S_RegisterSound("assassin/arr2wood.wav");
	cl_fxsfx_met2stn = S_RegisterSound("weapons/met2stn.wav");

	cl_fxsfx_ripple = S_RegisterSound("misc/drip.wav");
	cl_fxsfx_splash = S_RegisterSound("raven/outwater.wav");
}

void CL_ClearEffects(void)
{
	Com_Memset(cl.Effects,0,sizeof(cl.Effects));
	Com_Memset(EntityUsed,0,sizeof(EntityUsed));
	EffectEntityCount = 0;
}

void CL_FreeEffect(int index)
{	
	int i;

	switch(cl.Effects[index].type)
	{
		case CE_RAIN:
			break;

		case CE_FOUNTAIN:
			break;

		case CE_QUAKE:
			break;

		case CE_TELESMK1:
			FreeEffectEntity(cl.Effects[index].Smoke.entity_index2);
			//no break wanted here
		case CE_WHITE_SMOKE:
		case CE_GREEN_SMOKE:
		case CE_GREY_SMOKE:
		case CE_RED_SMOKE:
		case CE_SLOW_WHITE_SMOKE:
		case CE_TELESMK2:
		case CE_GHOST:
		case CE_REDCLOUD:
		case CE_ACID_MUZZFL:
		case CE_FLAMESTREAM:
		case CE_FLAMEWALL:
		case CE_FLAMEWALL2:
		case CE_ONFIRE:
		case CE_RIPPLE:
			FreeEffectEntity(cl.Effects[index].Smoke.entity_index);
			break;

		case CE_DEATHBUBBLES:
			break;
		// Just go through animation and then remove
		case CE_SM_WHITE_FLASH:
		case CE_YELLOWRED_FLASH:
		case CE_BLUESPARK:
		case CE_YELLOWSPARK:
		case CE_SM_CIRCLE_EXP:
		case CE_BG_CIRCLE_EXP:
		case CE_SM_EXPLOSION:
		case CE_SM_EXPLOSION2:
		case CE_BG_EXPLOSION:
		case CE_FLOOR_EXPLOSION:
		case CE_BLUE_EXPLOSION:
		case CE_REDSPARK:
		case CE_GREENSPARK:
		case CE_ICEHIT:
		case CE_MEDUSA_HIT:
		case CE_MEZZO_REFLECT:
		case CE_FLOOR_EXPLOSION2:
		case CE_XBOW_EXPLOSION:
		case CE_NEW_EXPLOSION:
		case CE_MAGIC_MISSILE_EXPLOSION:
		case CE_BONE_EXPLOSION:
		case CE_BLDRN_EXPL:
		case CE_BRN_BOUNCE:
		case CE_LSHOCK:
		case CE_ACID_HIT:
		case CE_ACID_SPLAT:
		case CE_ACID_EXPL:
		case CE_LBALL_EXPL:
		case CE_FBOOM:
		case CE_BOMB:
		case CE_FIREWALL_SMALL:
		case CE_FIREWALL_MEDIUM:
		case CE_FIREWALL_LARGE:
			FreeEffectEntity(cl.Effects[index].Smoke.entity_index);
			break;

		// Go forward then backward through animation then remove
		case CE_WHITE_FLASH:
		case CE_BLUE_FLASH:
		case CE_SM_BLUE_FLASH:
		case CE_HWSPLITFLASH:
		case CE_RED_FLASH:
			FreeEffectEntity(cl.Effects[index].Flash.entity_index);
			break;

		case CE_RIDER_DEATH:
			break;

		case CE_TELEPORTERPUFFS:
			for (i=0;i<8;++i)
				FreeEffectEntity(cl.Effects[index].Teleporter.entity_index[i]);
			break;

		case CE_HWSHEEPINATOR:
			for (i=0;i<5;++i)
				FreeEffectEntity(cl.Effects[index].Xbow.ent[i]);
			break;

		case CE_HWXBOWSHOOT:
			for (i=0;i<cl.Effects[index].Xbow.bolts;++i)
				FreeEffectEntity(cl.Effects[index].Xbow.ent[i]);
			break;

		case CE_TELEPORTERBODY:
			FreeEffectEntity(cl.Effects[index].Teleporter.entity_index[0]);
			break;

		case CE_HWDRILLA:
		case CE_BONESHARD:
		case CE_BONESHRAPNEL:
		case CE_HWBONEBALL:
		case CE_HWRAVENSTAFF:
		case CE_HWRAVENPOWER:
			FreeEffectEntity(cl.Effects[index].Missile.entity_index);
			break;
		case CE_TRIPMINESTILL:
//			Con_DPrintf("Ditching chain\n");
			FreeEffectEntity(cl.Effects[index].Chain.ent1);
			break;
		case CE_SCARABCHAIN:
		case CE_TRIPMINE:
			FreeEffectEntity(cl.Effects[index].Chain.ent1);
			break;
		case CE_HWMISSILESTAR:
			FreeEffectEntity(cl.Effects[index].Star.ent2);
			//no break wanted here
		case CE_HWEIDOLONSTAR:
			FreeEffectEntity(cl.Effects[index].Star.ent1);
			FreeEffectEntity(cl.Effects[index].Star.entity_index);

			break;
		default:
//			Con_Printf("Freeing unknown effect type\n");
			break;
	}

	Com_Memset(&cl.Effects[index],0,sizeof(struct EffectT));
}

//==========================================================================
//
// CL_ParseEffect
//
//==========================================================================

// All changes need to be in SV_SendEffect(), SV_ParseEffect(),
// CL_ParseEffect()
void CL_ParseEffect(void)
{
	int index,i;
	qboolean ImmediateFree;
	entity_t *ent;
	int dir;
	float	angleval, sinval, cosval;
	float skinnum;
	vec3_t forward, right, up, vtemp;
	vec3_t forward2, right2, up2;
	vec3_t origin;

	ImmediateFree = false;

	index = net_message.ReadByte();
	if (cl.Effects[index].type)
		CL_FreeEffect(index);

	Com_Memset(&cl.Effects[index],0,sizeof(struct EffectT));

	cl.Effects[index].type = net_message.ReadByte();

	switch(cl.Effects[index].type)
	{
		case CE_RAIN:
			cl.Effects[index].Rain.min_org[0] = net_message.ReadCoord();
			cl.Effects[index].Rain.min_org[1] = net_message.ReadCoord();
			cl.Effects[index].Rain.min_org[2] = net_message.ReadCoord();
			cl.Effects[index].Rain.max_org[0] = net_message.ReadCoord();
			cl.Effects[index].Rain.max_org[1] = net_message.ReadCoord();
			cl.Effects[index].Rain.max_org[2] = net_message.ReadCoord();
			cl.Effects[index].Rain.e_size[0] = net_message.ReadCoord();
			cl.Effects[index].Rain.e_size[1] = net_message.ReadCoord();
			cl.Effects[index].Rain.e_size[2] = net_message.ReadCoord();
			cl.Effects[index].Rain.dir[0] = net_message.ReadCoord();
			cl.Effects[index].Rain.dir[1] = net_message.ReadCoord();
			cl.Effects[index].Rain.dir[2] = net_message.ReadCoord();
			cl.Effects[index].Rain.color = net_message.ReadShort();
			cl.Effects[index].Rain.count = net_message.ReadShort();
			cl.Effects[index].Rain.wait = net_message.ReadFloat();
			break;

		case CE_FOUNTAIN:
			cl.Effects[index].Fountain.pos[0] = net_message.ReadCoord ();
			cl.Effects[index].Fountain.pos[1] = net_message.ReadCoord ();
			cl.Effects[index].Fountain.pos[2] = net_message.ReadCoord ();
			cl.Effects[index].Fountain.angle[0] = net_message.ReadAngle ();
			cl.Effects[index].Fountain.angle[1] = net_message.ReadAngle ();
			cl.Effects[index].Fountain.angle[2] = net_message.ReadAngle ();
			cl.Effects[index].Fountain.movedir[0] = net_message.ReadCoord ();
			cl.Effects[index].Fountain.movedir[1] = net_message.ReadCoord ();
			cl.Effects[index].Fountain.movedir[2] = net_message.ReadCoord ();
			cl.Effects[index].Fountain.color = net_message.ReadShort ();
			cl.Effects[index].Fountain.cnt = net_message.ReadByte ();
			AngleVectors (cl.Effects[index].Fountain.angle, 
						  cl.Effects[index].Fountain.vforward,
						  cl.Effects[index].Fountain.vright,
						  cl.Effects[index].Fountain.vup);
			break;

		case CE_QUAKE:
			cl.Effects[index].Quake.origin[0] = net_message.ReadCoord ();
			cl.Effects[index].Quake.origin[1] = net_message.ReadCoord ();
			cl.Effects[index].Quake.origin[2] = net_message.ReadCoord ();
			cl.Effects[index].Quake.radius = net_message.ReadFloat ();
			break;

		case CE_WHITE_SMOKE:
		case CE_GREEN_SMOKE:
		case CE_GREY_SMOKE:
		case CE_RED_SMOKE:
		case CE_SLOW_WHITE_SMOKE:
		case CE_TELESMK1:
		case CE_TELESMK2:
		case CE_GHOST:
		case CE_REDCLOUD:
		case CE_ACID_MUZZFL:
		case CE_FLAMESTREAM:
		case CE_FLAMEWALL:
		case CE_FLAMEWALL2:
		case CE_ONFIRE:
		case CE_RIPPLE:
			cl.Effects[index].Smoke.origin[0] = net_message.ReadCoord ();
			cl.Effects[index].Smoke.origin[1] = net_message.ReadCoord ();
			cl.Effects[index].Smoke.origin[2] = net_message.ReadCoord ();

			cl.Effects[index].Smoke.velocity[0] = net_message.ReadFloat ();
			cl.Effects[index].Smoke.velocity[1] = net_message.ReadFloat ();
			cl.Effects[index].Smoke.velocity[2] = net_message.ReadFloat ();

			cl.Effects[index].Smoke.framelength = net_message.ReadFloat ();

			if ((cl.Effects[index].Smoke.entity_index = NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.Effects[index].Smoke.entity_index];
				VectorCopy(cl.Effects[index].Smoke.origin, ent->origin);

				if ((cl.Effects[index].type == CE_WHITE_SMOKE) || 
					(cl.Effects[index].type == CE_SLOW_WHITE_SMOKE))
					ent->model = R_RegisterModel("models/whtsmk1.spr");
				else if (cl.Effects[index].type == CE_GREEN_SMOKE)
					ent->model = R_RegisterModel("models/grnsmk1.spr");
				else if (cl.Effects[index].type == CE_GREY_SMOKE)
					ent->model = R_RegisterModel("models/grysmk1.spr");
				else if (cl.Effects[index].type == CE_RED_SMOKE)
					ent->model = R_RegisterModel("models/redsmk1.spr");
				else if (cl.Effects[index].type == CE_TELESMK1)
					ent->model = R_RegisterModel("models/telesmk1.spr");
				else if (cl.Effects[index].type == CE_TELESMK2)
					ent->model = R_RegisterModel("models/telesmk2.spr");
				else if (cl.Effects[index].type == CE_REDCLOUD)
					ent->model = R_RegisterModel("models/rcloud.spr");
				else if (cl.Effects[index].type == CE_FLAMESTREAM)
					ent->model = R_RegisterModel("models/flamestr.spr");
				else if (cl.Effects[index].type == CE_ACID_MUZZFL)
				{
					ent->model = R_RegisterModel("models/muzzle1.spr");
					ent->drawflags=DRF_TRANSLUCENT|MLS_ABSLIGHT;
					ent->abslight=0.2;
				}
				else if (cl.Effects[index].type == CE_FLAMEWALL)
					ent->model = R_RegisterModel("models/firewal1.spr");
				else if (cl.Effects[index].type == CE_FLAMEWALL2)
					ent->model = R_RegisterModel("models/firewal2.spr");
				else if (cl.Effects[index].type == CE_ONFIRE)
				{
					float rdm = rand() & 3;

					if (rdm < 1)
						ent->model = R_RegisterModel("models/firewal1.spr");
					else if (rdm < 2)
						ent->model = R_RegisterModel("models/firewal2.spr");
					else
						ent->model = R_RegisterModel("models/firewal3.spr");
					
					ent->drawflags = DRF_TRANSLUCENT;
					ent->abslight = 1;
					ent->frame = cl.Effects[index].Smoke.frame;
				}
				else if (cl.Effects[index].type == CE_RIPPLE)
				{
					if(cl.Effects[index].Smoke.framelength==2)
					{
						R_SplashParticleEffect (cl.Effects[index].Smoke.origin, 200, 406+rand()%8, pt_h2slowgrav, 40);//splash
						S_StartSound(cl.Effects[index].Smoke.origin, TempSoundChannel(), 1, cl_fxsfx_splash, 1, 1);
					}
					else if(cl.Effects[index].Smoke.framelength==1)
						R_SplashParticleEffect (cl.Effects[index].Smoke.origin, 100, 406+rand()%8, pt_h2slowgrav, 20);//splash
					else
						S_StartSound(cl.Effects[index].Smoke.origin, TempSoundChannel(), 1, cl_fxsfx_ripple, 1, 1);
					cl.Effects[index].Smoke.framelength=0.05;
					ent->model = R_RegisterModel("models/ripple.spr");
					ent->drawflags = DRF_TRANSLUCENT;//|SCALE_TYPE_XYONLY|SCALE_ORIGIN_CENTER;
					ent->angles[0]=90;
					//ent->scale = 1;
				}
				else
				{
					ImmediateFree = true;
					Con_Printf ("Bad effect type %d\n",(int)cl.Effects[index].type);
				}

				if (cl.Effects[index].type != CE_REDCLOUD&&cl.Effects[index].type != CE_ACID_MUZZFL&&cl.Effects[index].type != CE_FLAMEWALL&&cl.Effects[index].type != CE_RIPPLE)
					ent->drawflags = DRF_TRANSLUCENT;

				if (cl.Effects[index].type == CE_FLAMESTREAM)
				{
					ent->drawflags = DRF_TRANSLUCENT | MLS_ABSLIGHT;
					ent->abslight = 1;
					ent->frame = cl.Effects[index].Smoke.frame;
				}

				if (cl.Effects[index].type == CE_GHOST)
				{		
					ent->model = R_RegisterModel("models/ghost.spr");
					ent->drawflags = DRF_TRANSLUCENT | MLS_ABSLIGHT;
					ent->abslight = .5;
				}
				if (cl.Effects[index].type == CE_TELESMK1)
				{
					S_StartSound(cl.Effects[index].Smoke.origin, TempSoundChannel(), 1, cl_fxsfx_ravenfire, 1, 1);

					if ((cl.Effects[index].Smoke.entity_index2 = NewEffectEntity()) != -1)
					{
						ent = &EffectEntities[cl.Effects[index].Smoke.entity_index2];
						VectorCopy(cl.Effects[index].Smoke.origin, ent->origin);
						ent->model = R_RegisterModel("models/telesmk1.spr");
						ent->drawflags = DRF_TRANSLUCENT;
					}
				}
			}
			else
				ImmediateFree = true;
			break;

		case CE_SM_WHITE_FLASH:
		case CE_YELLOWRED_FLASH:
		case CE_BLUESPARK:
		case CE_YELLOWSPARK:
		case CE_SM_CIRCLE_EXP:
		case CE_BG_CIRCLE_EXP:
		case CE_SM_EXPLOSION:
		case CE_SM_EXPLOSION2:
		case CE_BG_EXPLOSION:
		case CE_FLOOR_EXPLOSION:
		case CE_BLUE_EXPLOSION:
		case CE_REDSPARK:
		case CE_GREENSPARK:
		case CE_ICEHIT:
		case CE_MEDUSA_HIT:
		case CE_MEZZO_REFLECT:
		case CE_FLOOR_EXPLOSION2:
		case CE_XBOW_EXPLOSION:
		case CE_NEW_EXPLOSION:
		case CE_MAGIC_MISSILE_EXPLOSION:
		case CE_BONE_EXPLOSION:
		case CE_BLDRN_EXPL:
		case CE_BRN_BOUNCE:
		case CE_LSHOCK:
		case CE_ACID_HIT:
		case CE_ACID_SPLAT:
		case CE_ACID_EXPL:
		case CE_LBALL_EXPL:
		case CE_FBOOM:
		case CE_BOMB:
		case CE_FIREWALL_SMALL:
		case CE_FIREWALL_MEDIUM:
		case CE_FIREWALL_LARGE:

			cl.Effects[index].Smoke.origin[0] = net_message.ReadCoord ();
			cl.Effects[index].Smoke.origin[1] = net_message.ReadCoord ();
			cl.Effects[index].Smoke.origin[2] = net_message.ReadCoord ();
			if ((cl.Effects[index].Smoke.entity_index = NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.Effects[index].Smoke.entity_index];
				VectorCopy(cl.Effects[index].Smoke.origin, ent->origin);

				if (cl.Effects[index].type == CE_BLUESPARK)
					ent->model = R_RegisterModel("models/bspark.spr");
				else if (cl.Effects[index].type == CE_YELLOWSPARK)
					ent->model = R_RegisterModel("models/spark.spr");
				else if (cl.Effects[index].type == CE_SM_CIRCLE_EXP)
					ent->model = R_RegisterModel("models/fcircle.spr");
				else if (cl.Effects[index].type == CE_BG_CIRCLE_EXP)
					ent->model = R_RegisterModel("models/xplod29.spr");
				else if (cl.Effects[index].type == CE_SM_WHITE_FLASH)
					ent->model = R_RegisterModel("models/sm_white.spr");
				else if (cl.Effects[index].type == CE_YELLOWRED_FLASH)
				{
					ent->model = R_RegisterModel("models/yr_flsh.spr");
					ent->drawflags = DRF_TRANSLUCENT;
				}
				else if (cl.Effects[index].type == CE_SM_EXPLOSION)
					ent->model = R_RegisterModel("models/sm_expld.spr");
				else if (cl.Effects[index].type == CE_SM_EXPLOSION2)
				{
					ent->model = R_RegisterModel("models/sm_expld.spr");
					S_StartSound(cl.Effects[index].Smoke.origin, TempSoundChannel(), 1, cl_fxsfx_explode, 1, 1);

				}
				else if (cl.Effects[index].type == CE_BG_EXPLOSION)
				{
					ent->model = R_RegisterModel("models/bg_expld.spr");
					S_StartSound(cl.Effects[index].Smoke.origin, TempSoundChannel(), 1, cl_fxsfx_explode, 1, 1);
				}
				else if (cl.Effects[index].type == CE_FLOOR_EXPLOSION)
					ent->model = R_RegisterModel("models/fl_expld.spr");
				else if (cl.Effects[index].type == CE_BLUE_EXPLOSION)
					ent->model = R_RegisterModel("models/xpspblue.spr");
				else if (cl.Effects[index].type == CE_REDSPARK)
					ent->model = R_RegisterModel("models/rspark.spr");
				else if (cl.Effects[index].type == CE_GREENSPARK)
					ent->model = R_RegisterModel("models/gspark.spr");
				else if (cl.Effects[index].type == CE_ICEHIT)
					ent->model = R_RegisterModel("models/icehit.spr");
				else if (cl.Effects[index].type == CE_MEDUSA_HIT)
					ent->model = R_RegisterModel("models/medhit.spr");
				else if (cl.Effects[index].type == CE_MEZZO_REFLECT)
					ent->model = R_RegisterModel("models/mezzoref.spr");
				else if (cl.Effects[index].type == CE_FLOOR_EXPLOSION2)
					ent->model = R_RegisterModel("models/flrexpl2.spr");
				else if (cl.Effects[index].type == CE_XBOW_EXPLOSION)
					ent->model = R_RegisterModel("models/xbowexpl.spr");
				else if (cl.Effects[index].type == CE_NEW_EXPLOSION)
					ent->model = R_RegisterModel("models/gen_expl.spr");
				else if (cl.Effects[index].type == CE_MAGIC_MISSILE_EXPLOSION)
				{
					ent->model = R_RegisterModel("models/mm_expld.spr");
					S_StartSound(cl.Effects[index].Smoke.origin, TempSoundChannel(), 1, cl_fxsfx_explode, 1, 1);
				}
				else if (cl.Effects[index].type == CE_BONE_EXPLOSION)
					ent->model = R_RegisterModel("models/bonexpld.spr");
				else if (cl.Effects[index].type == CE_BLDRN_EXPL)
					ent->model = R_RegisterModel("models/xplsn_1.spr");
				else if (cl.Effects[index].type == CE_ACID_HIT)
					ent->model = R_RegisterModel("models/axplsn_2.spr");
				else if (cl.Effects[index].type == CE_ACID_SPLAT)
					ent->model = R_RegisterModel("models/axplsn_1.spr");
				else if (cl.Effects[index].type == CE_ACID_EXPL)
				{
					ent->model = R_RegisterModel("models/axplsn_5.spr");
					ent->drawflags = MLS_ABSLIGHT;
					ent->abslight = 1;
				}
				else if (cl.Effects[index].type == CE_FBOOM)
					ent->model = R_RegisterModel("models/fboom.spr");
				else if (cl.Effects[index].type == CE_BOMB)
					ent->model = R_RegisterModel("models/pow.spr");
				else if (cl.Effects[index].type == CE_LBALL_EXPL)
					ent->model = R_RegisterModel("models/Bluexp3.spr");
				else if (cl.Effects[index].type == CE_FIREWALL_SMALL)
					ent->model = R_RegisterModel("models/firewal1.spr");
				else if (cl.Effects[index].type == CE_FIREWALL_MEDIUM)
					ent->model = R_RegisterModel("models/firewal5.spr");
				else if (cl.Effects[index].type == CE_FIREWALL_LARGE)
					ent->model = R_RegisterModel("models/firewal4.spr");
				else if (cl.Effects[index].type == CE_BRN_BOUNCE)
					ent->model = R_RegisterModel("models/spark.spr");
				else if (cl.Effects[index].type == CE_LSHOCK)
				{
					ent->model = R_RegisterModel("models/vorpshok.mdl");
					ent->drawflags=MLS_TORCH;
					ent->angles[2]=90;
					ent->scale=255;
				}
			}
			else
			{
				ImmediateFree = true;
			}
			break;

		case CE_WHITE_FLASH:
		case CE_BLUE_FLASH:
		case CE_SM_BLUE_FLASH:
		case CE_HWSPLITFLASH:
		case CE_RED_FLASH:
			cl.Effects[index].Flash.origin[0] = net_message.ReadCoord ();
			cl.Effects[index].Flash.origin[1] = net_message.ReadCoord ();
			cl.Effects[index].Flash.origin[2] = net_message.ReadCoord ();
			cl.Effects[index].Flash.reverse = 0;
			if ((cl.Effects[index].Flash.entity_index = NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.Effects[index].Flash.entity_index];
				VectorCopy(cl.Effects[index].Flash.origin, ent->origin);

				if (cl.Effects[index].type == CE_WHITE_FLASH)
					ent->model = R_RegisterModel("models/gryspt.spr");
				else if (cl.Effects[index].type == CE_BLUE_FLASH)
					ent->model = R_RegisterModel("models/bluflash.spr");
				else if (cl.Effects[index].type == CE_SM_BLUE_FLASH)
					ent->model = R_RegisterModel("models/sm_blue.spr");
				else if (cl.Effects[index].type == CE_RED_FLASH)
					ent->model = R_RegisterModel("models/redspt.spr");
				else if (cl.Effects[index].type == CE_HWSPLITFLASH)
				{
					ent->model = R_RegisterModel("models/sm_blue.spr");
					S_StartSound(cl.Effects[index].Flash.origin, TempSoundChannel(), 1, cl_fxsfx_ravensplit, 1, 1);
				}
				ent->drawflags = DRF_TRANSLUCENT;

			}
			else
			{
				ImmediateFree = true;
			}
			break;

		case CE_RIDER_DEATH:
			cl.Effects[index].RD.origin[0] = net_message.ReadCoord ();
			cl.Effects[index].RD.origin[1] = net_message.ReadCoord ();
			cl.Effects[index].RD.origin[2] = net_message.ReadCoord ();
			break;

		case CE_TELEPORTERPUFFS:
			cl.Effects[index].Teleporter.origin[0] = net_message.ReadCoord ();
			cl.Effects[index].Teleporter.origin[1] = net_message.ReadCoord ();
			cl.Effects[index].Teleporter.origin[2] = net_message.ReadCoord ();
				
			cl.Effects[index].Teleporter.framelength = .05;
			dir = 0;
			for (i=0;i<8;++i)
			{		
				if ((cl.Effects[index].Teleporter.entity_index[i] = NewEffectEntity()) != -1)
				{
					ent = &EffectEntities[cl.Effects[index].Teleporter.entity_index[i]];
					VectorCopy(cl.Effects[index].Teleporter.origin, ent->origin);

					angleval = dir * M_PI*2 / 360;

					sinval = sin(angleval);
					cosval = cos(angleval);

					cl.Effects[index].Teleporter.velocity[i][0] = 10*cosval;
					cl.Effects[index].Teleporter.velocity[i][1] = 10*sinval;
					cl.Effects[index].Teleporter.velocity[i][2] = 0;
					dir += 45;

					ent->model = R_RegisterModel("models/telesmk2.spr");
					ent->drawflags = DRF_TRANSLUCENT;
				}
			}
			break;

		case CE_TELEPORTERBODY:
			cl.Effects[index].Teleporter.origin[0] = net_message.ReadCoord ();
			cl.Effects[index].Teleporter.origin[1] = net_message.ReadCoord ();
			cl.Effects[index].Teleporter.origin[2] = net_message.ReadCoord ();

			cl.Effects[index].Teleporter.velocity[0][0] = net_message.ReadFloat ();
			cl.Effects[index].Teleporter.velocity[0][1] = net_message.ReadFloat ();
			cl.Effects[index].Teleporter.velocity[0][2] = net_message.ReadFloat ();

			skinnum = net_message.ReadFloat ();
			
			cl.Effects[index].Teleporter.framelength = .05;
			dir = 0;
			if ((cl.Effects[index].Teleporter.entity_index[0] = NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.Effects[index].Teleporter.entity_index[0]];
				VectorCopy(cl.Effects[index].Teleporter.origin, ent->origin);

				ent->model = R_RegisterModel("models/teleport.mdl");
				ent->drawflags = SCALE_TYPE_XYONLY | DRF_TRANSLUCENT;
				ent->scale = 100;
				ent->skinnum = skinnum;
			}
			break;

		case CE_BONESHRAPNEL:
		case CE_HWBONEBALL:
			cl.Effects[index].Missile.origin[0] = net_message.ReadCoord ();
			cl.Effects[index].Missile.origin[1] = net_message.ReadCoord ();
			cl.Effects[index].Missile.origin[2] = net_message.ReadCoord ();

			cl.Effects[index].Missile.velocity[0] = net_message.ReadFloat ();
			cl.Effects[index].Missile.velocity[1] = net_message.ReadFloat ();
			cl.Effects[index].Missile.velocity[2] = net_message.ReadFloat ();

			cl.Effects[index].Missile.angle[0] = net_message.ReadFloat ();
			cl.Effects[index].Missile.angle[1] = net_message.ReadFloat ();
			cl.Effects[index].Missile.angle[2] = net_message.ReadFloat ();

			cl.Effects[index].Missile.avelocity[0] = net_message.ReadFloat ();
			cl.Effects[index].Missile.avelocity[1] = net_message.ReadFloat ();
			cl.Effects[index].Missile.avelocity[2] = net_message.ReadFloat ();

			if ((cl.Effects[index].Missile.entity_index = NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.Effects[index].Missile.entity_index];
				VectorCopy(cl.Effects[index].Missile.origin, ent->origin);
				VectorCopy(cl.Effects[index].Missile.angle, ent->angles);
				if (cl.Effects[index].type == CE_BONESHRAPNEL)
					ent->model = R_RegisterModel("models/boneshrd.mdl");
				else if (cl.Effects[index].type == CE_HWBONEBALL)
				{
					ent->model = R_RegisterModel("models/bonelump.mdl");
					S_StartSound(cl.Effects[index].Missile.origin, TempSoundChannel(), 1, cl_fxsfx_bonefpow, 1, 1);
				}
			}
			else
				ImmediateFree = true;

			break;
		case CE_BONESHARD:
		case CE_HWRAVENSTAFF:
		case CE_HWRAVENPOWER:
			cl.Effects[index].Missile.origin[0] = net_message.ReadCoord ();
			cl.Effects[index].Missile.origin[1] = net_message.ReadCoord ();
			cl.Effects[index].Missile.origin[2] = net_message.ReadCoord ();

			cl.Effects[index].Missile.velocity[0] = net_message.ReadFloat ();
			cl.Effects[index].Missile.velocity[1] = net_message.ReadFloat ();
			cl.Effects[index].Missile.velocity[2] = net_message.ReadFloat ();
			vectoangles(cl.Effects[index].Missile.velocity, cl.Effects[index].Missile.angle);

			if ((cl.Effects[index].Missile.entity_index = NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.Effects[index].Missile.entity_index];
				VectorCopy(cl.Effects[index].Missile.origin, ent->origin);
				VectorCopy(cl.Effects[index].Missile.angle, ent->angles);
				if (cl.Effects[index].type == CE_HWRAVENSTAFF)
				{
					cl.Effects[index].Missile.avelocity[2] = 1000;
					ent->model = R_RegisterModel("models/vindsht1.mdl");
				}
				else if (cl.Effects[index].type == CE_BONESHARD)
				{
					cl.Effects[index].Missile.avelocity[0] = (rand() % 1554) - 777;
					ent->model = R_RegisterModel("models/boneshot.mdl");
					S_StartSound(cl.Effects[index].Missile.origin, TempSoundChannel(), 1, cl_fxsfx_bone, 1, 1);
				}
				else if (cl.Effects[index].type == CE_HWRAVENPOWER)
				{
					ent->model = R_RegisterModel("models/ravproj.mdl");
					S_StartSound(cl.Effects[index].Missile.origin, TempSoundChannel(), 1, cl_fxsfx_ravengo, 1, 1);
				}
			}
			else
				ImmediateFree = true;
			break;
		case CE_DEATHBUBBLES:
			cl.Effects[index].Bubble.owner = net_message.ReadShort();
			cl.Effects[index].Bubble.offset[0] = net_message.ReadByte();
			cl.Effects[index].Bubble.offset[1] = net_message.ReadByte();
			cl.Effects[index].Bubble.offset[2] = net_message.ReadByte();
			cl.Effects[index].Bubble.count = net_message.ReadByte();//num of bubbles
			cl.Effects[index].Bubble.time_amount = 0;
			break;
		case CE_HWXBOWSHOOT:
			origin[0] = net_message.ReadCoord ();
			origin[1] = net_message.ReadCoord ();
			origin[2] = net_message.ReadCoord ();

			cl.Effects[index].Xbow.angle[0] = net_message.ReadAngle ();
			cl.Effects[index].Xbow.angle[1] = net_message.ReadAngle ();
			cl.Effects[index].Xbow.angle[2] = 0;//net_message.ReadFloat ();

			cl.Effects[index].Xbow.bolts = net_message.ReadByte();
			cl.Effects[index].Xbow.randseed = net_message.ReadByte();

			cl.Effects[index].Xbow.turnedbolts = net_message.ReadByte();

			cl.Effects[index].Xbow.activebolts= net_message.ReadByte();

			setseed(cl.Effects[index].Xbow.randseed);

			AngleVectors (cl.Effects[index].Xbow.angle, forward, right, up);

			VectorNormalize(forward);
			VectorCopy(forward, cl.Effects[index].Xbow.velocity);
//			VectorScale(forward, 1000, cl.Effects[index].Xbow.velocity);

			if (cl.Effects[index].Xbow.bolts == 3)
			{
				S_StartSound(origin, TempSoundChannel(), 1, cl_fxsfx_xbowshoot, 1, 1);
			}
			else if (cl.Effects[index].Xbow.bolts == 5)
			{
				S_StartSound(origin, TempSoundChannel(), 1, cl_fxsfx_xbowfshoot, 1, 1);
			}

			for (i=0;i<cl.Effects[index].Xbow.bolts;i++)
			{
				cl.Effects[index].Xbow.gonetime[i] = 1 + seedrand()*2;
				cl.Effects[index].Xbow.state[i] = 0;

				if ((1<<i)&	cl.Effects[index].Xbow.turnedbolts)
				{
					cl.Effects[index].Xbow.origin[i][0]=net_message.ReadCoord();
					cl.Effects[index].Xbow.origin[i][1]=net_message.ReadCoord();
					cl.Effects[index].Xbow.origin[i][2]=net_message.ReadCoord();
					vtemp[0]=net_message.ReadAngle();
					vtemp[1]=net_message.ReadAngle();
					vtemp[2]=0;
					AngleVectors (vtemp, forward2, right2, up2);
					VectorScale(forward2, 800 + seedrand()*500, cl.Effects[index].Xbow.vel[i]);
				}
				else
				{
					VectorScale(forward, 800 + seedrand()*500, cl.Effects[index].Xbow.vel[i]);

					VectorScale(right,i*100-(cl.Effects[index].Xbow.bolts-1)*50,vtemp);

					//this should only be done for deathmatch:
					VectorScale(vtemp,0.333,vtemp);

					VectorAdd(cl.Effects[index].Xbow.vel[i],vtemp,cl.Effects[index].Xbow.vel[i]);

					//start me off a bit out
					VectorScale(vtemp,0.05,cl.Effects[index].Xbow.origin[i]);
					VectorAdd(origin,cl.Effects[index].Xbow.origin[i],cl.Effects[index].Xbow.origin[i]);
				}

				if ((cl.Effects[index].Xbow.ent[i] = NewEffectEntity()) != -1)
				{
					ent = &EffectEntities[cl.Effects[index].Xbow.ent[i]];
					VectorCopy(cl.Effects[index].Xbow.origin[i], ent->origin);
					vectoangles(cl.Effects[index].Xbow.vel[i],ent->angles);
					if (cl.Effects[index].Xbow.bolts == 5)
						ent->model = R_RegisterModel("models/flaming.mdl");
					else
						ent->model = R_RegisterModel("models/arrow.mdl");
				}
			}

			break;
		case CE_HWSHEEPINATOR:
			origin[0] = net_message.ReadCoord ();
			origin[1] = net_message.ReadCoord ();
			origin[2] = net_message.ReadCoord ();

			cl.Effects[index].Xbow.angle[0] = net_message.ReadAngle ();
			cl.Effects[index].Xbow.angle[1] = net_message.ReadAngle ();
			cl.Effects[index].Xbow.angle[2] = 0;//net_message.ReadFloat ();

			cl.Effects[index].Xbow.turnedbolts = net_message.ReadByte();

			cl.Effects[index].Xbow.activebolts= net_message.ReadByte();

			cl.Effects[index].Xbow.bolts = 5;
			cl.Effects[index].Xbow.randseed = 0;

			AngleVectors (cl.Effects[index].Xbow.angle, forward, right, up);

			VectorNormalize(forward);
			VectorCopy(forward, cl.Effects[index].Xbow.velocity);

//			S_StartSound(origin, TempSoundChannel(), 1, cl_fxsfx_xbowshoot, 1, 1);

			for (i=0;i<cl.Effects[index].Xbow.bolts;i++)
			{
				cl.Effects[index].Xbow.gonetime[i] = 0;
				cl.Effects[index].Xbow.state[i] = 0;


				if ((1<<i)&	cl.Effects[index].Xbow.turnedbolts)
				{
					cl.Effects[index].Xbow.origin[i][0]=net_message.ReadCoord();
					cl.Effects[index].Xbow.origin[i][1]=net_message.ReadCoord();
					cl.Effects[index].Xbow.origin[i][2]=net_message.ReadCoord();
					vtemp[0]=net_message.ReadAngle();
					vtemp[1]=net_message.ReadAngle();
					vtemp[2]=0;
					AngleVectors (vtemp, forward2, right2, up2);
					VectorScale(forward2, 700, cl.Effects[index].Xbow.vel[i]);
				}
				else
				{
					VectorCopy(origin,cl.Effects[index].Xbow.origin[i]);
					VectorScale(forward, 700, cl.Effects[index].Xbow.vel[i]);
					VectorScale(right,i*75-150,vtemp);
					VectorAdd(cl.Effects[index].Xbow.vel[i],vtemp,cl.Effects[index].Xbow.vel[i]);
				}

				if ((cl.Effects[index].Xbow.ent[i] = NewEffectEntity()) != -1)
				{
					ent = &EffectEntities[cl.Effects[index].Xbow.ent[i]];
					VectorCopy(cl.Effects[index].Xbow.origin[i], ent->origin);
					vectoangles(cl.Effects[index].Xbow.vel[i],ent->angles);
					ent->model = R_RegisterModel("models/polymrph.spr");
				}
			}

			break;
		case CE_HWDRILLA:
			cl.Effects[index].Missile.origin[0] = net_message.ReadCoord ();
			cl.Effects[index].Missile.origin[1] = net_message.ReadCoord ();
			cl.Effects[index].Missile.origin[2] = net_message.ReadCoord ();

			cl.Effects[index].Missile.angle[0] = net_message.ReadAngle ();
			cl.Effects[index].Missile.angle[1] = net_message.ReadAngle ();
			cl.Effects[index].Missile.angle[2] = 0;

			cl.Effects[index].Missile.speed = net_message.ReadShort();

			S_StartSound(cl.Effects[index].Missile.origin, TempSoundChannel(), 1, cl_fxsfx_drillashoot, 1, 1);

			AngleVectors(cl.Effects[index].Missile.angle,cl.Effects[index].Missile.velocity,right,up);

			VectorScale(cl.Effects[index].Missile.velocity,cl.Effects[index].Missile.speed,cl.Effects[index].Missile.velocity);

			if ((cl.Effects[index].Missile.entity_index = NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.Effects[index].Missile.entity_index];
				VectorCopy(cl.Effects[index].Missile.origin, ent->origin);
				VectorCopy(cl.Effects[index].Missile.angle, ent->angles);
				ent->model = R_RegisterModel("models/scrbstp1.mdl");
			}
			else
				ImmediateFree = true;
			break;
		case CE_SCARABCHAIN:
			cl.Effects[index].Chain.origin[0] = net_message.ReadCoord ();
			cl.Effects[index].Chain.origin[1] = net_message.ReadCoord ();
			cl.Effects[index].Chain.origin[2] = net_message.ReadCoord ();

			cl.Effects[index].Chain.owner = net_message.ReadShort ();
			cl.Effects[index].Chain.tag = net_message.ReadByte();
			cl.Effects[index].Chain.material = cl.Effects[index].Chain.owner>>12;
			cl.Effects[index].Chain.owner &= 0x0fff;
			cl.Effects[index].Chain.height = 16;//net_message.ReadByte ();

			cl.Effects[index].Chain.sound_time = cl.serverTimeFloat;

			cl.Effects[index].Chain.state = 0;//state 0: move slowly toward owner

			S_StartSound(cl.Effects[index].Chain.origin, TempSoundChannel(), 1, cl_fxsfx_scarabwhoosh, 1, 1);

			if ((cl.Effects[index].Chain.ent1 = NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.Effects[index].Chain.ent1];
				VectorCopy(cl.Effects[index].Chain.origin, ent->origin);
				ent->model = R_RegisterModel("models/scrbpbdy.mdl");
			}
			else
				ImmediateFree = true;
			break;
		case CE_TRIPMINE:
			cl.Effects[index].Chain.origin[0] = net_message.ReadCoord ();
			cl.Effects[index].Chain.origin[1] = net_message.ReadCoord ();
			cl.Effects[index].Chain.origin[2] = net_message.ReadCoord ();

			cl.Effects[index].Chain.velocity[0] = net_message.ReadFloat ();
			cl.Effects[index].Chain.velocity[1] = net_message.ReadFloat ();
			cl.Effects[index].Chain.velocity[2] = net_message.ReadFloat ();

			if ((cl.Effects[index].Chain.ent1 = NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.Effects[index].Chain.ent1];
				VectorCopy(cl.Effects[index].Chain.origin, ent->origin);
				ent->model = R_RegisterModel("models/twspike.mdl");
			}
			else
				ImmediateFree = true;
			break;
		case CE_TRIPMINESTILL:
//			Con_DPrintf("Allocating chain effect...\n");
			cl.Effects[index].Chain.origin[0] = net_message.ReadCoord ();
			cl.Effects[index].Chain.origin[1] = net_message.ReadCoord ();
			cl.Effects[index].Chain.origin[2] = net_message.ReadCoord ();

			cl.Effects[index].Chain.velocity[0] = net_message.ReadFloat ();
			cl.Effects[index].Chain.velocity[1] = net_message.ReadFloat ();
			cl.Effects[index].Chain.velocity[2] = net_message.ReadFloat ();

			if ((cl.Effects[index].Chain.ent1 = NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.Effects[index].Chain.ent1];
				VectorCopy(cl.Effects[index].Chain.velocity, ent->origin);
				ent->model = R_RegisterModel("models/twspike.mdl");
			}
			else
			{
//				Con_DPrintf("ERROR: Couldn't allocate chain effect!\n");
				ImmediateFree = true;
			}
			break;
		case CE_HWMISSILESTAR:
		case CE_HWEIDOLONSTAR:
			cl.Effects[index].Star.origin[0] = net_message.ReadCoord ();
			cl.Effects[index].Star.origin[1] = net_message.ReadCoord ();
			cl.Effects[index].Star.origin[2] = net_message.ReadCoord ();

			cl.Effects[index].Star.velocity[0] = net_message.ReadFloat ();
			cl.Effects[index].Star.velocity[1] = net_message.ReadFloat ();
			cl.Effects[index].Star.velocity[2] = net_message.ReadFloat ();
			vectoangles(cl.Effects[index].Star.velocity, cl.Effects[index].Star.angle);
			cl.Effects[index].Missile.avelocity[2] = 300 + rand()%300;

			if ((cl.Effects[index].Star.entity_index = NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.Effects[index].Star.entity_index];
				VectorCopy(cl.Effects[index].Star.origin, ent->origin);
				VectorCopy(cl.Effects[index].Star.angle, ent->angles);
				ent->model = R_RegisterModel("models/ball.mdl");
			}
			else
				ImmediateFree = true;

			cl.Effects[index].Star.scaleDir = 1;
			cl.Effects[index].Star.scale = 0.3;
			
			if ((cl.Effects[index].Star.ent1 = NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.Effects[index].Star.ent1];
				VectorCopy(cl.Effects[index].Star.origin, ent->origin);
				ent->drawflags |= MLS_ABSLIGHT;
				ent->abslight = 0.5;
				ent->angles[2] = 90;
				if(cl.Effects[index].type == CE_HWMISSILESTAR)
				{
					ent->model = R_RegisterModel("models/star.mdl");
					ent->scale = 0.3;
					S_StartSound(ent->origin, TempSoundChannel(), 1, cl_fxsfx_mmfire, 1, 1);

				}
				else
				{
					ent->model = R_RegisterModel("models/glowball.mdl");
					S_StartSound(ent->origin, TempSoundChannel(), 1, cl_fxsfx_eidolon, 1, 1);
				}
			}
			if(cl.Effects[index].type == CE_HWMISSILESTAR)
			{
				if ((cl.Effects[index].Star.ent2 = NewEffectEntity()) != -1)
				{
					ent = &EffectEntities[cl.Effects[index].Star.ent2];
					VectorCopy(cl.Effects[index].Star.origin, ent->origin);
					ent->model = R_RegisterModel("models/star.mdl");
					ent->drawflags |= MLS_ABSLIGHT;
					ent->abslight = 0.5;
					ent->scale = 0.3;
				}
			}
			break;
		default:
			Sys_Error ("CL_ParseEffect: bad type");
	}

	if (ImmediateFree)
	{
		cl.Effects[index].type = CE_NONE;
	}
}

// these are in cl_tent.c
void CreateRavenDeath(vec3_t pos);
void CreateExplosionWithSound(vec3_t pos);

void CL_EndEffect(void)
{
	int index;
	entity_t *ent;

	index = net_message.ReadByte();

	switch(cl.Effects[index].type )
	{
	case CE_HWRAVENPOWER:
		if(cl.Effects[index].Missile.entity_index > -1)
		{
			ent = &EffectEntities[cl.Effects[index].Missile.entity_index];
			CreateRavenDeath(ent->origin);
		}
		break;
	case CE_HWRAVENSTAFF:
		if(cl.Effects[index].Missile.entity_index > -1)
		{
			ent = &EffectEntities[cl.Effects[index].Missile.entity_index];
			CreateExplosionWithSound(ent->origin);
		}
		break;
	}
	CL_FreeEffect(index);
}

void XbowImpactPuff(vec3_t origin, int material)//hopefully can use this with xbow & chain both
{
	int		part_color;

	switch(material)
	{
	case XBOW_IMPACT_REDFLESH:
		part_color = 256 + 8 * 16 + rand()%9;				//Blood red
		break;
	case XBOW_IMPACT_STONE:
		part_color = 256 + 20 + rand()%8;			// Gray
		break;
	case XBOW_IMPACT_METAL:
		part_color = 256 + (8 * 15);			// Sparks
		break;
	case XBOW_IMPACT_WOOD:
		part_color = 256 + (5 * 16) + rand()%8;			// Wood chunks
		break;
	case XBOW_IMPACT_ICE:
		part_color = 406+rand()%8;				// Ice particles
		break;
	case XBOW_IMPACT_GREENFLESH:
		part_color = 256 + 183 + rand()%8;		// Spider's have green blood
		break;
	default:
		part_color = 256 + (3 * 16) + 4;		// Dust Brown
		break;
	}

	R_RunParticleEffect4 (origin, 24, part_color, pt_h2fastgrav, 20);
}

void CL_ReviseEffect(void)	// be sure to read, in the switch statement, everything
							// in this message, even if the effect is not the right kind or invalid,
							// or else client is sure to crash.	
{
	int index,type,revisionCode;
	int curEnt,material,takedamage;
	entity_t	*ent;
	vec3_t	forward,right,up,pos;
	float	dist,speed;
	entity_state_t	*es;


	index = net_message.ReadByte ();
	type = net_message.ReadByte ();

	if (cl.Effects[index].type==type)
		switch(type)
		{
		case CE_SCARABCHAIN://attach to new guy or retract if new guy is world
			curEnt = net_message.ReadShort();
			if (cl.Effects[index].type==type)
			{
				cl.Effects[index].Chain.material = curEnt>>12;
				curEnt &= 0x0fff;

				if (curEnt)
				{
					cl.Effects[index].Chain.state = 1;
					cl.Effects[index].Chain.owner = curEnt;
					es = FindState(cl.Effects[index].Chain.owner);
					if (es)
					{
						ent = &EffectEntities[cl.Effects[index].Chain.ent1];
						XbowImpactPuff(ent->origin, cl.Effects[index].Chain.material);
					}
				}
				else
					cl.Effects[index].Chain.state = 2;
			}
			break;
		case CE_HWXBOWSHOOT:
			revisionCode = net_message.ReadByte();
			//this is one packed byte!
			//highest bit: for impact revision, indicates whether damage is done
			//				for redirect revision, indicates whether new origin was sent
			//next 3 high bits: for all revisions, indicates which bolt is to be revised
			//highest 3 of the low 4 bits: for impact revision, indicates the material that was hit
			//lowest bit: indicates whether revision is of impact or redirect variety


			curEnt = (revisionCode>>4)&7;
			if (revisionCode & 1)//impact effect: 
			{
				cl.Effects[index].Xbow.activebolts &= ~(1<<curEnt);
				dist = net_message.ReadCoord();
				if (cl.Effects[index].Xbow.ent[curEnt]!= -1)
				{
					ent = &EffectEntities[cl.Effects[index].Xbow.ent[curEnt]];

					//make sure bolt is in correct position
					VectorCopy(cl.Effects[index].Xbow.vel[curEnt],forward);
					VectorNormalize(forward);
					VectorScale(forward,dist,forward);
					VectorAdd(cl.Effects[index].Xbow.origin[curEnt],forward,ent->origin);

					material = (revisionCode & 14);
					takedamage = (revisionCode & 128);

					if (takedamage)
					{
						cl.Effects[index].Xbow.gonetime[curEnt] = cl.serverTimeFloat;
					}
					else
					{
						cl.Effects[index].Xbow.gonetime[curEnt] += cl.serverTimeFloat;
					}
					
					VectorCopy(cl.Effects[index].Xbow.vel[curEnt],forward);
					VectorNormalize(forward);
					VectorScale(forward,8,forward);

					// do a particle effect here, with the color depending on chType
					XbowImpactPuff(ent->origin,material);

					// impact sound:
					switch (material)
					{
					case XBOW_IMPACT_GREENFLESH:
					case XBOW_IMPACT_REDFLESH:
					case XBOW_IMPACT_MUMMY:
						S_StartSound(ent->origin, TempSoundChannel(), 0, cl_fxsfx_arr2flsh, 1, 1);
						break;
					case XBOW_IMPACT_WOOD:
						S_StartSound(ent->origin, TempSoundChannel(), 0, cl_fxsfx_arr2wood, 1, 1);
						break;
					default:
						S_StartSound(ent->origin, TempSoundChannel(), 0, cl_fxsfx_met2stn, 1, 1);
						break;
					}

					CLTENT_XbowImpact(ent->origin, forward, material, takedamage, cl.Effects[index].Xbow.bolts);
				}
			}
			else
			{
				if (cl.Effects[index].Xbow.ent[curEnt]!=-1)
				{
					ent = &EffectEntities[cl.Effects[index].Xbow.ent[curEnt]];
					ent->angles[0] = net_message.ReadAngle();
					if (ent->angles[0] < 0)
						ent->angles[0] += 360;
					ent->angles[0]*=-1;
					ent->angles[1] = net_message.ReadAngle();
					if (ent->angles[1] < 0)
						ent->angles[1] += 360;
					ent->angles[2] = 0;

					if (revisionCode &128)//new origin
					{
						cl.Effects[index].Xbow.origin[curEnt][0]=net_message.ReadCoord();
						cl.Effects[index].Xbow.origin[curEnt][1]=net_message.ReadCoord();
						cl.Effects[index].Xbow.origin[curEnt][2]=net_message.ReadCoord();
					}

					AngleVectors(ent->angles,forward,right,up);
					speed = VectorLength(cl.Effects[index].Xbow.vel[curEnt]);
					VectorScale(forward,speed,cl.Effects[index].Xbow.vel[curEnt]);
					VectorCopy(cl.Effects[index].Xbow.origin[curEnt],ent->origin);
				}
				else
				{
					pos[0] = net_message.ReadAngle();
					if (pos[0] < 0)
						pos[0] += 360;
					pos[0]*=-1;
					pos[1] = net_message.ReadAngle();
					if (pos[1] < 0)
						pos[1] += 360;
					pos[2] = 0;

					if (revisionCode &128)//new origin
					{
						cl.Effects[index].Xbow.origin[curEnt][0]=net_message.ReadCoord();
						cl.Effects[index].Xbow.origin[curEnt][1]=net_message.ReadCoord();
						cl.Effects[index].Xbow.origin[curEnt][2]=net_message.ReadCoord();
					}

					AngleVectors(pos,forward,right,up);
					speed = VectorLength(cl.Effects[index].Xbow.vel[curEnt]);
					VectorScale(forward,speed,cl.Effects[index].Xbow.vel[curEnt]);
				}
			}
			break;

		case CE_HWSHEEPINATOR:
			revisionCode = net_message.ReadByte();
			curEnt = (revisionCode>>4)&7;
			if (revisionCode & 1)//impact
			{
				dist = net_message.ReadCoord();
				cl.Effects[index].Xbow.activebolts &= ~(1<<curEnt);
				if (cl.Effects[index].Xbow.ent[curEnt] != -1)
				{
					ent = &EffectEntities[cl.Effects[index].Xbow.ent[curEnt]];

					//make sure bolt is in correct position
					VectorCopy(cl.Effects[index].Xbow.vel[curEnt],forward);
					VectorNormalize(forward);
					VectorScale(forward,dist,forward);
					VectorAdd(cl.Effects[index].Xbow.origin[curEnt],forward,ent->origin);
					R_ColoredParticleExplosion(ent->origin,(rand()%16)+144/*(144,159)*/,20,30);
				}
			}
			else//direction change
			{
				if (cl.Effects[index].Xbow.ent[curEnt] != -1)
				{
					ent = &EffectEntities[cl.Effects[index].Xbow.ent[curEnt]];
					ent->angles[0] = net_message.ReadAngle();
					if (ent->angles[0] < 0)
						ent->angles[0] += 360;
					ent->angles[0]*=-1;
					ent->angles[1] = net_message.ReadAngle();
					if (ent->angles[1] < 0)
						ent->angles[1] += 360;
					ent->angles[2] = 0;

					if (revisionCode &128)//new origin
					{
						cl.Effects[index].Xbow.origin[curEnt][0]=net_message.ReadCoord();
						cl.Effects[index].Xbow.origin[curEnt][1]=net_message.ReadCoord();
						cl.Effects[index].Xbow.origin[curEnt][2]=net_message.ReadCoord();
					}

					AngleVectors(ent->angles,forward,right,up);
					speed = VectorLength(cl.Effects[index].Xbow.vel[curEnt]);
					VectorScale(forward,speed,cl.Effects[index].Xbow.vel[curEnt]);
					VectorCopy(cl.Effects[index].Xbow.origin[curEnt],ent->origin);
				}
				else
				{
					pos[0] = net_message.ReadAngle();
					if (pos[0] < 0)
						pos[0] += 360;
					pos[0]*=-1;
					pos[1] = net_message.ReadAngle();
					if (pos[1] < 0)
						pos[1] += 360;
					pos[2] = 0;

					if (revisionCode &128)//new origin
					{
						cl.Effects[index].Xbow.origin[curEnt][0]=net_message.ReadCoord();
						cl.Effects[index].Xbow.origin[curEnt][1]=net_message.ReadCoord();
						cl.Effects[index].Xbow.origin[curEnt][2]=net_message.ReadCoord();
					}

					AngleVectors(pos,forward,right,up);
					speed = VectorLength(cl.Effects[index].Xbow.vel[curEnt]);
					VectorScale(forward,speed,cl.Effects[index].Xbow.vel[curEnt]);
				}
			}
			break;


		case CE_HWDRILLA:
			revisionCode = net_message.ReadByte();
			if (revisionCode == 0)//impact
			{
				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();
				material = net_message.ReadByte();

				//throw lil bits of victim at entry
				XbowImpactPuff(pos,material);

				if ((material == XBOW_IMPACT_GREENFLESH) || (material == XBOW_IMPACT_GREENFLESH))
				{//meaty sound and some chunks too
					S_StartSound(pos, TempSoundChannel(), 0, cl_fxsfx_drillameat, 1, 1);
					
					//todo: the chunks
				}

				//lil bits at exit
				VectorCopy(cl.Effects[index].Missile.velocity,forward);
				VectorNormalize(forward);
				VectorScale(forward,36,forward);
				VectorAdd(forward,pos,pos);
				XbowImpactPuff(pos,material);
			}
			else//turn
			{
				if (cl.Effects[index].Missile.entity_index!=-1)
				{
					ent = &EffectEntities[cl.Effects[index].Missile.entity_index];
					ent->angles[0] = net_message.ReadAngle();
					if (ent->angles[0] < 0)
						ent->angles[0] += 360;
					ent->angles[0]*=-1;
					ent->angles[1] = net_message.ReadAngle();
					if (ent->angles[1] < 0)
						ent->angles[1] += 360;
					ent->angles[2] = 0;

					cl.Effects[index].Missile.origin[0]=net_message.ReadCoord();
					cl.Effects[index].Missile.origin[1]=net_message.ReadCoord();
					cl.Effects[index].Missile.origin[2]=net_message.ReadCoord();

					AngleVectors(ent->angles,forward,right,up);
					speed = VectorLength(cl.Effects[index].Missile.velocity);
					VectorScale(forward,speed,cl.Effects[index].Missile.velocity);
					VectorCopy(cl.Effects[index].Missile.origin,ent->origin);
				}
				else
				{
					pos[0] = net_message.ReadAngle();
					if (pos[0] < 0)
						pos[0] += 360;
					pos[0]*=-1;
					pos[1] = net_message.ReadAngle();
					if (pos[1] < 0)
						pos[1] += 360;
					pos[2] = 0;

					cl.Effects[index].Missile.origin[0]=net_message.ReadCoord();
					cl.Effects[index].Missile.origin[1]=net_message.ReadCoord();
					cl.Effects[index].Missile.origin[2]=net_message.ReadCoord();

					AngleVectors(pos,forward,right,up);
					speed = VectorLength(cl.Effects[index].Missile.velocity);
					VectorScale(forward,speed,cl.Effects[index].Missile.velocity);
				}
			}
			break;
		}
	else
	{
//		Con_DPrintf("Received Unrecognized Effect Update!\n");
		switch(type)
		{
		case CE_SCARABCHAIN://attach to new guy or retract if new guy is world
			curEnt = net_message.ReadShort();
			break;
		case CE_HWXBOWSHOOT:
			revisionCode = net_message.ReadByte();

			curEnt = (revisionCode>>4)&7;
			if (revisionCode & 1)//impact effect: 
			{
				net_message.ReadCoord();
			}
			else
			{
				net_message.ReadAngle();
				net_message.ReadAngle();
				if (revisionCode &128)//new origin
				{
					net_message.ReadCoord();
					net_message.ReadCoord();
					net_message.ReadCoord();

					// create a clc message to retrieve effect information
//					cls.netchan.message.WriteByte(clc_get_effect);
//					cls.netchan.message.WriteByte(index);
				}
			}
			break;
		case CE_HWSHEEPINATOR:
			revisionCode = net_message.ReadByte();
			curEnt = (revisionCode>>4)&7;
			if (revisionCode & 1)//impact
			{
				net_message.ReadCoord();
			}
			else//direction change
			{
				net_message.ReadAngle();
				net_message.ReadAngle();
				if (revisionCode &128)//new origin
				{
					net_message.ReadCoord();
					net_message.ReadCoord();
					net_message.ReadCoord();

					// create a clc message to retrieve effect information
//					cls.netchan.message.WriteByte(clc_get_effect);
//					cls.netchan.message.WriteByte(index);
				}
			}
			break;
		case CE_HWDRILLA:
			revisionCode = net_message.ReadByte();
			if (revisionCode == 0)//impact
			{
				net_message.ReadCoord();
				net_message.ReadCoord();
				net_message.ReadCoord();
				net_message.ReadByte();
			}
			else//turn
			{
				net_message.ReadAngle();
				net_message.ReadAngle();

				net_message.ReadCoord();
				net_message.ReadCoord();
				net_message.ReadCoord();

				// create a clc message to retrieve effect information
//				cls.netchan.message.WriteByte(clc_get_effect);
//				cls.netchan.message.WriteByte(index);

			}
			break;
		}
	}
}

void UpdateMissilePath(vec3_t oldorg, vec3_t neworg, vec3_t newvel, float time)
{
	vec3_t endpos;	//the position it should be at currently
	float delta;

	delta = cl.serverTimeFloat - time;
	
	VectorMA(neworg, delta, newvel, endpos); 
	VectorCopy(neworg, oldorg);	//set orig, maybe vel too
}


void CL_TurnEffect(void)
{
	int index;
	entity_t *ent;
	vec3_t pos, vel;
	float time;

	index = net_message.ReadByte ();
	time = net_message.ReadFloat ();
	pos[0] = net_message.ReadCoord();
	pos[1] = net_message.ReadCoord();
	pos[2] = net_message.ReadCoord();
	vel[0] = net_message.ReadCoord();
	vel[1] = net_message.ReadCoord();
	vel[2] = net_message.ReadCoord();
	switch(cl.Effects[index].type)
	{
	case CE_HWRAVENSTAFF:
	case CE_HWRAVENPOWER:
	case CE_BONESHARD:
	case CE_BONESHRAPNEL:
	case CE_HWBONEBALL:
		if(cl.Effects[index].Missile.entity_index > -1)
		{
			ent = &EffectEntities[cl.Effects[index].Missile.entity_index];
			UpdateMissilePath(ent->origin, pos, vel, time);
			VectorCopy(vel, cl.Effects[index].Missile.velocity);
			vectoangles(cl.Effects[index].Missile.velocity, cl.Effects[index].Missile.angle);
		}
		break;
	case CE_HWMISSILESTAR:
	case CE_HWEIDOLONSTAR:
		if(cl.Effects[index].Star.entity_index > -1)
		{
			ent = &EffectEntities[cl.Effects[index].Star.entity_index];
			UpdateMissilePath(ent->origin, pos, vel, time);
			VectorCopy(vel, cl.Effects[index].Star.velocity);
		}
		break;
	case 0:
		// create a clc message to retrieve effect information
//		cls.netchan.message.WriteByte(clc_get_effect);
//		cls.netchan.message.WriteByte(index);
//		Con_Printf("CL_TurnEffect: null effect %d\n", index);
		break;
	default:
		Con_Printf ("CL_TurnEffect: bad type %d\n", cl.Effects[index].type);
		break;
	}

}

void CL_LinkEntity(entity_t *ent)
{
	//debug: if visedicts getting messed up, it should appear here.
//	if (ent->_model < 10)
//	{
//		ent->_model = 3;
//	}

	refEntity_t rent;
	Com_Memset(&rent, 0, sizeof(rent));
	rent.reType = RT_MODEL;
	VectorCopy(ent->origin, rent.origin);
	rent.hModel = ent->model;
	rent.frame = ent->frame;
	rent.skinNum = ent->skinnum;
	rent.shaderTime = ent->syncbase;
	CL_SetRefEntAxis(&rent, ent->angles, ent->angleAdd, ent->scale, ent->colorshade, ent->abslight, ent->drawflags);
	R_HandleCustomSkin(&rent, -1);
	R_AddRefEntityToScene(&rent);
}

void R_RunQuakeEffect (vec3_t org, float distance);
void RiderParticle(int count, vec3_t origin);

void CL_UpdateEffects(void)
{
	int			index,cur_frame;
	vec3_t		mymin,mymax;
	float		frametime;
//	edict_t		test;
//	trace_t		trace;
	vec3_t		org,org2,old_origin;
	int			x_dir,y_dir;
	entity_t	*ent, *ent2;
	float		smoketime;
	int			i;
	entity_state_t	*es;

	frametime = host_frametime;
	if (!frametime) return;
//	Con_Printf("Here at %f\n",cl.time);

	for(index=0;index<MAX_EFFECTS;index++)
	{
		if (!cl.Effects[index].type) 
			continue;

		switch(cl.Effects[index].type)
		{
			case CE_RAIN:
				org[0] = cl.Effects[index].Rain.min_org[0];
				org[1] = cl.Effects[index].Rain.min_org[1];
				org[2] = cl.Effects[index].Rain.max_org[2];

				org2[0] = cl.Effects[index].Rain.e_size[0];
				org2[1] = cl.Effects[index].Rain.e_size[1];
				org2[2] = cl.Effects[index].Rain.e_size[2];

				x_dir = cl.Effects[index].Rain.dir[0];
				y_dir = cl.Effects[index].Rain.dir[1];
				
				cl.Effects[index].Rain.next_time += frametime;
				if (cl.Effects[index].Rain.next_time >= cl.Effects[index].Rain.wait)
				{		
					R_RainEffect(org,org2,x_dir,y_dir,cl.Effects[index].Rain.color,
						cl.Effects[index].Rain.count);
					cl.Effects[index].Rain.next_time = 0;
				}
				break;

			case CE_FOUNTAIN:
				mymin[0] = (-3 * cl.Effects[index].Fountain.vright[0] * cl.Effects[index].Fountain.movedir[0]) +
						   (-3 * cl.Effects[index].Fountain.vforward[0] * cl.Effects[index].Fountain.movedir[1]) +
						   (2 * cl.Effects[index].Fountain.vup[0] * cl.Effects[index].Fountain.movedir[2]);
				mymin[1] = (-3 * cl.Effects[index].Fountain.vright[1] * cl.Effects[index].Fountain.movedir[0]) +
						   (-3 * cl.Effects[index].Fountain.vforward[1] * cl.Effects[index].Fountain.movedir[1]) +
						   (2 * cl.Effects[index].Fountain.vup[1] * cl.Effects[index].Fountain.movedir[2]);
				mymin[2] = (-3 * cl.Effects[index].Fountain.vright[2] * cl.Effects[index].Fountain.movedir[0]) +
						   (-3 * cl.Effects[index].Fountain.vforward[2] * cl.Effects[index].Fountain.movedir[1]) +
						   (2 * cl.Effects[index].Fountain.vup[2] * cl.Effects[index].Fountain.movedir[2]);
				mymin[0] *= 15;
				mymin[1] *= 15;
				mymin[2] *= 15;

				mymax[0] = (3 * cl.Effects[index].Fountain.vright[0] * cl.Effects[index].Fountain.movedir[0]) +
						   (3 * cl.Effects[index].Fountain.vforward[0] * cl.Effects[index].Fountain.movedir[1]) +
						   (10 * cl.Effects[index].Fountain.vup[0] * cl.Effects[index].Fountain.movedir[2]);
				mymax[1] = (3 * cl.Effects[index].Fountain.vright[1] * cl.Effects[index].Fountain.movedir[0]) +
						   (3 * cl.Effects[index].Fountain.vforward[1] * cl.Effects[index].Fountain.movedir[1]) +
						   (10 * cl.Effects[index].Fountain.vup[1] * cl.Effects[index].Fountain.movedir[2]);
				mymax[2] = (3 * cl.Effects[index].Fountain.vright[2] * cl.Effects[index].Fountain.movedir[0]) +
						   (3 * cl.Effects[index].Fountain.vforward[2] * cl.Effects[index].Fountain.movedir[1]) +
						   (10 * cl.Effects[index].Fountain.vup[2] * cl.Effects[index].Fountain.movedir[2]);
				mymax[0] *= 15;
				mymax[1] *= 15;
				mymax[2] *= 15;

				R_RunParticleEffect2 (cl.Effects[index].Fountain.pos,mymin,mymax,
					                  cl.Effects[index].Fountain.color,pt_h2fastgrav,cl.Effects[index].Fountain.cnt);

/*				Com_Memset(&test,0,sizeof(test));
				trace = SV_Move (cl.Effects[index].Fountain.pos, mymin, mymax, mymin, false, &test);
				Con_Printf("Fraction is %f\n",trace.fraction);*/
				break;

			case CE_QUAKE:
				R_RunQuakeEffect (cl.Effects[index].Quake.origin,cl.Effects[index].Quake.radius);
				break;

			case CE_RIPPLE:
				cl.Effects[index].Smoke.time_amount += frametime;
				ent = &EffectEntities[cl.Effects[index].Smoke.entity_index];

				smoketime = cl.Effects[index].Smoke.framelength;
				if (!smoketime)
					smoketime = HX_FRAME_TIME*2;

				while(cl.Effects[index].Smoke.time_amount >= smoketime&&ent->scale<250)
				{
					ent->frame++;
					ent->angles[1]+=1;
					cl.Effects[index].Smoke.time_amount -= smoketime;
				}

				if (ent->frame >= 10)
				{
					CL_FreeEffect(index);
				}
				else
					CL_LinkEntity(ent);
				break;


			case CE_WHITE_SMOKE:
			case CE_GREEN_SMOKE:
			case CE_GREY_SMOKE:
			case CE_RED_SMOKE:
			case CE_SLOW_WHITE_SMOKE:
			case CE_TELESMK1:
			case CE_TELESMK2:
			case CE_GHOST:
			case CE_REDCLOUD:
			case CE_FLAMESTREAM:
			case CE_ACID_MUZZFL:
			case CE_FLAMEWALL:
			case CE_FLAMEWALL2:
			case CE_ONFIRE:
				cl.Effects[index].Smoke.time_amount += frametime;
				ent = &EffectEntities[cl.Effects[index].Smoke.entity_index];

				smoketime = cl.Effects[index].Smoke.framelength;
				if (!smoketime)
					smoketime = HX_FRAME_TIME;

				ent->origin[0] += (frametime/smoketime) * cl.Effects[index].Smoke.velocity[0];
				ent->origin[1] += (frametime/smoketime) * cl.Effects[index].Smoke.velocity[1];
				ent->origin[2] += (frametime/smoketime) * cl.Effects[index].Smoke.velocity[2];
	
				i=0;
				while(cl.Effects[index].Smoke.time_amount >= smoketime)
				{
					ent->frame++;
					i++;
					cl.Effects[index].Smoke.time_amount -= smoketime;
				}

				if (ent->frame >= R_ModelNumFrames(ent->model))
				{
					CL_FreeEffect(index);
				}
				else
					CL_LinkEntity(ent);

				
				if(cl.Effects[index].type == CE_TELESMK1)
				{
					ent = &EffectEntities[cl.Effects[index].Smoke.entity_index2];

					ent->origin[0] -= (frametime/smoketime) * cl.Effects[index].Smoke.velocity[0];
					ent->origin[1] -= (frametime/smoketime) * cl.Effects[index].Smoke.velocity[1];
					ent->origin[2] -= (frametime/smoketime) * cl.Effects[index].Smoke.velocity[2];

					ent->frame += i;
					if (ent->frame < R_ModelNumFrames(ent->model))
					{
						CL_LinkEntity(ent);
					}
				}
				break;

			// Just go through animation and then remove
			case CE_SM_WHITE_FLASH:
			case CE_YELLOWRED_FLASH:
			case CE_BLUESPARK:
			case CE_YELLOWSPARK:
			case CE_SM_CIRCLE_EXP:
			case CE_BG_CIRCLE_EXP:
			case CE_SM_EXPLOSION:
			case CE_SM_EXPLOSION2:
			case CE_BG_EXPLOSION:
			case CE_FLOOR_EXPLOSION:
			case CE_BLUE_EXPLOSION:
			case CE_REDSPARK:
			case CE_GREENSPARK:
			case CE_ICEHIT:
			case CE_MEDUSA_HIT:
			case CE_MEZZO_REFLECT:
			case CE_FLOOR_EXPLOSION2:
			case CE_XBOW_EXPLOSION:
			case CE_NEW_EXPLOSION:
			case CE_MAGIC_MISSILE_EXPLOSION:
			case CE_BONE_EXPLOSION:
			case CE_BLDRN_EXPL:
			case CE_BRN_BOUNCE:
			case CE_ACID_HIT:
			case CE_ACID_SPLAT:
			case CE_ACID_EXPL:
			case CE_LBALL_EXPL:
			case CE_FBOOM:
			case CE_BOMB:
			case CE_FIREWALL_SMALL:
			case CE_FIREWALL_MEDIUM:
			case CE_FIREWALL_LARGE:

				cl.Effects[index].Smoke.time_amount += frametime;
				ent = &EffectEntities[cl.Effects[index].Smoke.entity_index];

				if (cl.Effects[index].type != CE_BG_CIRCLE_EXP)
				{
					while(cl.Effects[index].Smoke.time_amount >= HX_FRAME_TIME)
					{
						ent->frame++;
						cl.Effects[index].Smoke.time_amount -= HX_FRAME_TIME;
					}
				}
				else
				{
					while(cl.Effects[index].Smoke.time_amount >= HX_FRAME_TIME * 2)
					{
						ent->frame++;
						cl.Effects[index].Smoke.time_amount -= HX_FRAME_TIME * 2;
					}
				}


				if (ent->frame >= R_ModelNumFrames(ent->model))
				{
					CL_FreeEffect(index);
				}
				else
					CL_LinkEntity(ent);

				break;

			// Go forward then backward through animation then remove
			case CE_WHITE_FLASH:
			case CE_BLUE_FLASH:
			case CE_SM_BLUE_FLASH:
			case CE_HWSPLITFLASH:
			case CE_RED_FLASH:
				cl.Effects[index].Flash.time_amount += frametime;
				ent = &EffectEntities[cl.Effects[index].Flash.entity_index];

				while(cl.Effects[index].Flash.time_amount >= HX_FRAME_TIME)
				{
					if (!cl.Effects[index].Flash.reverse)
					{
						if (ent->frame >= R_ModelNumFrames(ent->model) - 1)  // Ran through forward animation
						{
							cl.Effects[index].Flash.reverse = 1;
							ent->frame--;
						}
						else
							ent->frame++;

					}	
					else
						ent->frame--;

					cl.Effects[index].Flash.time_amount -= HX_FRAME_TIME;
				}

				if ((ent->frame <= 0) && (cl.Effects[index].Flash.reverse))
				{
					CL_FreeEffect(index);
				}
				else
					CL_LinkEntity(ent);
				break;

			case CE_RIDER_DEATH:
				cl.Effects[index].RD.time_amount += frametime;
				if (cl.Effects[index].RD.time_amount >= 1)
				{
					cl.Effects[index].RD.stage++;
					cl.Effects[index].RD.time_amount -= 1;
				}

				VectorCopy(cl.Effects[index].RD.origin,org);
				org[0] += sin(cl.Effects[index].RD.time_amount * 2 * M_PI) * 30;
				org[1] += cos(cl.Effects[index].RD.time_amount * 2 * M_PI) * 30;

				if (cl.Effects[index].RD.stage <= 6)
				{
					RiderParticle(cl.Effects[index].RD.stage+1,org);
				}
				else
				{
					// To set the rider's origin point for the particles
					RiderParticle(0,org);
					if (cl.Effects[index].RD.stage == 7) 
					{
						cl.cshifts[CSHIFT_BONUS].destcolor[0] = 255;
						cl.cshifts[CSHIFT_BONUS].destcolor[1] = 255;
						cl.cshifts[CSHIFT_BONUS].destcolor[2] = 255;
						cl.cshifts[CSHIFT_BONUS].percent = 256;
					}
					else if (cl.Effects[index].RD.stage > 13) 
					{
//						cl.Effects[index].RD.stage = 0;
						CL_FreeEffect(index);
					}
				}
				break;

			case CE_TELEPORTERPUFFS:
				cl.Effects[index].Teleporter.time_amount += frametime;
				smoketime = cl.Effects[index].Teleporter.framelength;

				ent = &EffectEntities[cl.Effects[index].Teleporter.entity_index[0]];
				while(cl.Effects[index].Teleporter.time_amount >= HX_FRAME_TIME)
				{
					ent->frame++;
					cl.Effects[index].Teleporter.time_amount -= HX_FRAME_TIME;
				}
				cur_frame = ent->frame;

				if (cur_frame >= R_ModelNumFrames(ent->model))
				{
					CL_FreeEffect(index);
					break;
				}

				for (i=0;i<8;++i)
				{
					ent = &EffectEntities[cl.Effects[index].Teleporter.entity_index[i]];

					ent->origin[0] += (frametime/smoketime) * cl.Effects[index].Teleporter.velocity[i][0];
					ent->origin[1] += (frametime/smoketime) * cl.Effects[index].Teleporter.velocity[i][1];
					ent->origin[2] += (frametime/smoketime) * cl.Effects[index].Teleporter.velocity[i][2];
					ent->frame = cur_frame;

					CL_LinkEntity(ent);
				}
				break;
			case CE_TELEPORTERBODY:
				cl.Effects[index].Teleporter.time_amount += frametime;
				smoketime = cl.Effects[index].Teleporter.framelength;

				ent = &EffectEntities[cl.Effects[index].Teleporter.entity_index[0]];
				while(cl.Effects[index].Teleporter.time_amount >= HX_FRAME_TIME)
				{
					ent->scale -= 15;
					cl.Effects[index].Teleporter.time_amount -= HX_FRAME_TIME;
				}

				ent = &EffectEntities[cl.Effects[index].Teleporter.entity_index[0]];
				ent->angles[1] += 45;

				if (ent->scale <= 10)
				{
					CL_FreeEffect(index);
				}
				else
				{
					CL_LinkEntity(ent);
				}
				break;

			case CE_HWDRILLA:
				cl.Effects[index].Missile.time_amount += frametime;
				ent = &EffectEntities[cl.Effects[index].Missile.entity_index];

				if ((int)(cl.serverTimeFloat) != (int)(cl.serverTimeFloat - host_frametime))
				{
					S_StartSound(ent->origin, TempSoundChannel(), 1, cl_fxsfx_drillaspin, 1, 1);
				}

				ent->angles[0] += frametime * cl.Effects[index].Missile.avelocity[0];
				ent->angles[1] += frametime * cl.Effects[index].Missile.avelocity[1];
				ent->angles[2] += frametime * cl.Effects[index].Missile.avelocity[2];

				VectorCopy(ent->origin,old_origin);

				ent->origin[0] += frametime * cl.Effects[index].Missile.velocity[0];
				ent->origin[1] += frametime * cl.Effects[index].Missile.velocity[1];
				ent->origin[2] += frametime * cl.Effects[index].Missile.velocity[2];

				R_RocketTrail (old_origin, ent->origin, rt_setstaff);

				CL_LinkEntity(ent);
				break;
			case CE_HWXBOWSHOOT:
				cl.Effects[index].Xbow.time_amount += frametime;
				for (i=0;i<cl.Effects[index].Xbow.bolts;i++)
				{
					if (cl.Effects[index].Xbow.ent[i] != -1)//only update valid effect ents
					{
						if (cl.Effects[index].Xbow.activebolts & (1<<i))//bolt in air, simply update position
						{
							ent = &EffectEntities[cl.Effects[index].Xbow.ent[i]];

							ent->origin[0] += frametime * cl.Effects[index].Xbow.vel[i][0];
							ent->origin[1] += frametime * cl.Effects[index].Xbow.vel[i][1];
							ent->origin[2] += frametime * cl.Effects[index].Xbow.vel[i][2];

							CL_LinkEntity(ent);
						}
						else if (cl.Effects[index].Xbow.bolts == 5)//fiery bolts don't just go away
						{
							if (cl.Effects[index].Xbow.state[i] == 0)//waiting to explode state
							{
								if (cl.Effects[index].Xbow.gonetime[i] > cl.serverTimeFloat)//fiery bolts stick around for a while
								{
									ent = &EffectEntities[cl.Effects[index].Xbow.ent[i]];
									CL_LinkEntity(ent);
								}
								else//when time's up on fiery guys, they explode
								{
									//set state to exploding
									cl.Effects[index].Xbow.state[i] = 1;

									ent = &EffectEntities[cl.Effects[index].Xbow.ent[i]];

									//move bolt back a little to make explosion look better
									VectorNormalize(cl.Effects[index].Xbow.vel[i]);
									VectorScale(cl.Effects[index].Xbow.vel[i],-8,cl.Effects[index].Xbow.vel[i]);
									VectorAdd(ent->origin,cl.Effects[index].Xbow.vel[i],ent->origin);

									//turn bolt entity into an explosion
									ent->model = R_RegisterModel("models/xbowexpl.spr");
									ent->frame = 0;

									//set frame change counter
									cl.Effects[index].Xbow.gonetime[i] = cl.serverTimeFloat + HX_FRAME_TIME * 2;

									//play explosion sound
									S_StartSound(ent->origin, TempSoundChannel(), 1, cl_fxsfx_explode, 1, 1);

									CL_LinkEntity(ent);
								}
							}
							else if (cl.Effects[index].Xbow.state[i] == 1)//fiery bolt exploding state
							{
								ent = &EffectEntities[cl.Effects[index].Xbow.ent[i]];

								//increment frame if it's time
								while(cl.Effects[index].Xbow.gonetime[i] <= cl.serverTimeFloat)
								{
									ent->frame++;
									cl.Effects[index].Xbow.gonetime[i] += HX_FRAME_TIME * 0.75;
								}


								if (ent->frame >= R_ModelNumFrames(ent->model))
								{
									cl.Effects[index].Xbow.state[i] = 2;//if anim is over, set me to inactive state
								}
								else
									CL_LinkEntity(ent);
							}
						}
					}
				}
				break;
			case CE_HWSHEEPINATOR:
				cl.Effects[index].Xbow.time_amount += frametime;
				for (i=0;i<cl.Effects[index].Xbow.bolts;i++)
				{
					if (cl.Effects[index].Xbow.ent[i] != -1)//only update valid effect ents
					{
						if (cl.Effects[index].Xbow.activebolts & (1<<i))//bolt in air, simply update position
						{
							ent = &EffectEntities[cl.Effects[index].Xbow.ent[i]];

							ent->origin[0] += frametime * cl.Effects[index].Xbow.vel[i][0];
							ent->origin[1] += frametime * cl.Effects[index].Xbow.vel[i][1];
							ent->origin[2] += frametime * cl.Effects[index].Xbow.vel[i][2];

							R_RunParticleEffect4(ent->origin,7,(rand()%15)+144,pt_h2explode2,(rand()%5)+1);

							CL_LinkEntity(ent);
						}
					}
				}
				break;
			case CE_DEATHBUBBLES:
				cl.Effects[index].Bubble.time_amount += frametime;
				if (cl.Effects[index].Bubble.time_amount > 0.1)//10 bubbles a sec
				{
					cl.Effects[index].Bubble.time_amount = 0;
					cl.Effects[index].Bubble.count--;
					es = FindState(cl.Effects[index].Bubble.owner);
					if (es)
					{
						VectorCopy(es->origin,org);
						VectorAdd(org,cl.Effects[index].Bubble.offset,org);

						if (CM_PointContentsQ1(org, 0) != BSP29CONTENTS_WATER) 
						{
							//not in water anymore
							CL_FreeEffect(index);
							break;
						}
						else
						{
							CLTENT_SpawnDeathBubble(org);
						}
					}
				}
				if (cl.Effects[index].Bubble.count <= 0)
					CL_FreeEffect(index);
				break;
			case CE_SCARABCHAIN:
				cl.Effects[index].Chain.time_amount += frametime;
				ent = &EffectEntities[cl.Effects[index].Chain.ent1];

				switch (cl.Effects[index].Chain.state)
				{
				case 0://zooming in toward owner
					es = FindState(cl.Effects[index].Chain.owner);
					if (cl.Effects[index].Chain.sound_time <= cl.serverTimeFloat)
					{
						cl.Effects[index].Chain.sound_time = cl.serverTimeFloat + 0.5;
						S_StartSound(ent->origin, TempSoundChannel(), 1, cl_fxsfx_scarabhome, 1, 1);
					}
					if (es)
					{
						VectorCopy(es->origin,org);
						org[2]+=cl.Effects[index].Chain.height;
						VectorSubtract(org,ent->origin,org);
						if (fabs(VectorNormalize(org))<500*frametime)
						{
							S_StartSound(ent->origin, TempSoundChannel(), 1, cl_fxsfx_scarabgrab, 1, 1);
							cl.Effects[index].Chain.state = 1;
							VectorCopy(es->origin, ent->origin);
							ent->origin[2] += cl.Effects[index].Chain.height;
							XbowImpactPuff(ent->origin, cl.Effects[index].Chain.material);
						}
						else
						{
							VectorScale(org,500*frametime,org);
							VectorAdd(ent->origin,org,ent->origin);
						}
					}
					break;
				case 1://attached--snap to owner's pos
					es = FindState(cl.Effects[index].Chain.owner);
					if (es)
					{
						VectorCopy(es->origin, ent->origin);
						ent->origin[2] += cl.Effects[index].Chain.height;
					}
					break;
				case 2://unattaching, server needs to set this state
					VectorCopy(ent->origin,org);
					VectorSubtract(cl.Effects[index].Chain.origin,org,org);
					if (fabs(VectorNormalize(org))>350*frametime)//closer than 30 is too close?
					{
						VectorScale(org,350*frametime,org);
						VectorAdd(ent->origin,org,ent->origin);
					}
					else//done--flash & git outa here (change type to redflash)
					{
						S_StartSound(ent->origin, TempSoundChannel(), 1, cl_fxsfx_scarabbyebye, 1, 1);
						cl.Effects[index].Flash.entity_index = cl.Effects[index].Chain.ent1;
						cl.Effects[index].type = CE_RED_FLASH;
						VectorCopy(ent->origin,cl.Effects[index].Flash.origin);
						cl.Effects[index].Flash.reverse = 0;
						ent->model = R_RegisterModel("models/redspt.spr");
						ent->frame = 0;
						ent->drawflags = DRF_TRANSLUCENT;
					}
					break;
				}

				CL_LinkEntity(ent);

				//damndamndamn--add stream stuff here!
				VectorCopy(cl.Effects[index].Chain.origin, org);
				VectorCopy(ent->origin, org2);
				CreateStream(TE_STREAM_CHAIN, cl.Effects[index].Chain.ent1, 1, cl.Effects[index].Chain.tag, 0.1, 0, org, org2);

				break;
			case CE_TRIPMINESTILL:
				cl.Effects[index].Chain.time_amount += frametime;
				ent = &EffectEntities[cl.Effects[index].Chain.ent1];

//				if (cl.Effects[index].Chain.ent1 < 0)//fixme: remove this!!!
//					Con_DPrintf("OHSHITOHSHIT--bad chain ent\n");

				CL_LinkEntity(ent);
//				Con_DPrintf("Chain Ent at: %d %d %d\n",(int)cl.Effects[index].Chain.origin[0],(int)cl.Effects[index].Chain.origin[1],(int)cl.Effects[index].Chain.origin[2]);

				//damndamndamn--add stream stuff here!
				VectorCopy(cl.Effects[index].Chain.origin, org);
				VectorCopy(ent->origin, org2);
				CreateStream(TE_STREAM_CHAIN, cl.Effects[index].Chain.ent1, 1, 1, 0.1, 0, org, org2);

				break;
			case CE_TRIPMINE:
				cl.Effects[index].Chain.time_amount += frametime;
				ent = &EffectEntities[cl.Effects[index].Chain.ent1];

				ent->origin[0] += frametime * cl.Effects[index].Chain.velocity[0];
				ent->origin[1] += frametime * cl.Effects[index].Chain.velocity[1];
				ent->origin[2] += frametime * cl.Effects[index].Chain.velocity[2];

				CL_LinkEntity(ent);

				//damndamndamn--add stream stuff here!
				VectorCopy(cl.Effects[index].Chain.origin, org);
				VectorCopy(ent->origin, org2);
				CreateStream(TE_STREAM_CHAIN, cl.Effects[index].Chain.ent1, 1, 1, 0.1, 0, org, org2);

				break;
			case CE_BONESHARD:
			case CE_BONESHRAPNEL:
			case CE_HWBONEBALL:
			case CE_HWRAVENSTAFF:
			case CE_HWRAVENPOWER:
				cl.Effects[index].Missile.time_amount += frametime;
				ent = &EffectEntities[cl.Effects[index].Missile.entity_index];

//		ent->angles[0] = cl.Effects[index].Missile.angle[0];
//		ent->angles[1] = cl.Effects[index].Missile.angle[1];
//		ent->angles[2] = cl.Effects[index].Missile.angle[2];

				ent->angles[0] += frametime * cl.Effects[index].Missile.avelocity[0];
				ent->angles[1] += frametime * cl.Effects[index].Missile.avelocity[1];
				ent->angles[2] += frametime * cl.Effects[index].Missile.avelocity[2];

				ent->origin[0] += frametime * cl.Effects[index].Missile.velocity[0];
				ent->origin[1] += frametime * cl.Effects[index].Missile.velocity[1];
				ent->origin[2] += frametime * cl.Effects[index].Missile.velocity[2];
				if(cl.Effects[index].type == CE_HWRAVENPOWER)
				{
					while(cl.Effects[index].Missile.time_amount >= HX_FRAME_TIME)
					{
						ent->frame++;
						cl.Effects[index].Missile.time_amount -= HX_FRAME_TIME;
					}

					if (ent->frame > 7)
					{
						ent->frame = 0;
						break;
					}
				}
				CL_LinkEntity(ent);
				if(cl.Effects[index].type == CE_HWBONEBALL)
				{
					R_RunParticleEffect4 (ent->origin, 10, 368 + rand() % 16, pt_h2slowgrav, 3);

				}
				break;
			case CE_HWMISSILESTAR:
			case CE_HWEIDOLONSTAR:
				// update scale
				if(cl.Effects[index].Star.scaleDir)
				{
					cl.Effects[index].Star.scale += 0.05;
					if(cl.Effects[index].Star.scale >= 1)
					{
						cl.Effects[index].Star.scaleDir = 0;
					}
				}
				else
				{
					cl.Effects[index].Star.scale -= 0.05;
					if(cl.Effects[index].Star.scale <= 0.01)
					{
						cl.Effects[index].Star.scaleDir = 1;
					}
				}
				
				cl.Effects[index].Star.time_amount += frametime;
				ent = &EffectEntities[cl.Effects[index].Star.entity_index];

				ent->angles[0] += frametime * cl.Effects[index].Star.avelocity[0];
				ent->angles[1] += frametime * cl.Effects[index].Star.avelocity[1];
				ent->angles[2] += frametime * cl.Effects[index].Star.avelocity[2];

				ent->origin[0] += frametime * cl.Effects[index].Star.velocity[0];
				ent->origin[1] += frametime * cl.Effects[index].Star.velocity[1];
				ent->origin[2] += frametime * cl.Effects[index].Star.velocity[2];

				CL_LinkEntity(ent);
				
				if (cl.Effects[index].Star.ent1 != -1)
				{
					ent2= &EffectEntities[cl.Effects[index].Star.ent1];
					VectorCopy(ent->origin, ent2->origin);
					ent2->scale = cl.Effects[index].Star.scale;
					ent2->angles[1] += frametime * 300;
					ent2->angles[2] += frametime * 400;
					CL_LinkEntity(ent2);
				}
				if(cl.Effects[index].type == CE_HWMISSILESTAR)
				{
					if (cl.Effects[index].Star.ent2 != -1)
					{
						ent2 = &EffectEntities[cl.Effects[index].Star.ent2];
						VectorCopy(ent->origin, ent2->origin);
						ent2->scale = cl.Effects[index].Star.scale;
						ent2->angles[1] += frametime * -300;
						ent2->angles[2] += frametime * -400;
						CL_LinkEntity(ent2);
					}
				}					
				if(rand() % 10 < 3)		
				{
					R_RunParticleEffect4 (ent->origin, 7, 148 + rand() % 11, pt_h2grav, 10 + rand() % 10);
				}
				break;
		}
	}
//	Con_DPrintf("Effect Ents: %d\n",EffectEntityCount);
}

// this creates multi effects from one packet
void CreateRavenExplosions(vec3_t pos);
void CL_ParseMultiEffect(void)
{
	int type, index, count;
	vec3_t	orig, vel;
	entity_t *ent;

	type = net_message.ReadByte();
	switch(type)
	{
	case CE_HWRAVENPOWER:
		orig[0] = net_message.ReadCoord();
		orig[1] = net_message.ReadCoord();
		orig[2] = net_message.ReadCoord();
		vel[0] = net_message.ReadCoord();
		vel[1] = net_message.ReadCoord();
		vel[2] = net_message.ReadCoord();
		for(count=0;count<3;count++)
		{
			index = net_message.ReadByte();
			// create the effect
			cl.Effects[index].type = type;
			VectorCopy(orig, cl.Effects[index].Missile.origin);
			VectorCopy(vel, cl.Effects[index].Missile.velocity);
			vectoangles(cl.Effects[index].Missile.velocity, cl.Effects[index].Missile.angle);
			if ((cl.Effects[index].Missile.entity_index = NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.Effects[index].Missile.entity_index];
				VectorCopy(cl.Effects[index].Missile.origin, ent->origin);
				VectorCopy(cl.Effects[index].Missile.angle, ent->angles);
				ent->model = R_RegisterModel("models/ravproj.mdl");
				S_StartSound(cl.Effects[index].Missile.origin, TempSoundChannel(), 1, cl_fxsfx_ravengo, 1, 1);
			}
		}
		CreateRavenExplosions(orig);
		break;
	default:
		Sys_Error ("CL_ParseMultiEffect: bad type");

	}	
	
}


//==========================================================================
//
// NewEffectEntity
//
//==========================================================================

static int NewEffectEntity(void)
{
	entity_t	*ent;
	int counter;

	if(EffectEntityCount == MAX_EFFECT_ENTITIES)
	{
		return -1;
	}

	for(counter=0;counter<MAX_EFFECT_ENTITIES;counter++)
		if (!EntityUsed[counter]) 
			break;

	EntityUsed[counter] = true;
	EffectEntityCount++;
	ent = &EffectEntities[counter];
	Com_Memset(ent, 0, sizeof(*ent));

	return counter;
}

static void FreeEffectEntity(int index)
{
	if (index != -1 && EntityUsed[index])
	{
		EntityUsed[index] = false;
		EffectEntityCount--;
	}
	else if (index != -1)
	{
		EffectEntityCount--;//still decrement counter; since value is -1, counter was incremented 
//		Con_DPrintf("ERROR: Redeleting Effect Entity: %d!\n",index);//fixme: this should not be in final version
	}
	else
	{
//		Con_DPrintf("ERROR: Deleting Invalid Effect Entity: %d!\n",index);//fixme: this should not be in final version
	}
}
