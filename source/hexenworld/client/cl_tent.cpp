// cl_tent.c -- client side temporary entities

// HEADER FILES ------------------------------------------------------------

#include "quakedef.h"

// MACROS ------------------------------------------------------------------

#define	MAX_EXPLOSIONS			128

#define FRANDOM() (rand()*(1.0/RAND_MAX))

static unsigned int randomseed;
void setseed(unsigned int seed)
{
	randomseed = seed;
}

//unsigned int seedrand(int max)
float seedrand(void)
{
	randomseed = (randomseed * 877 + 573) % 9968;
	return (float)randomseed / 9968;
}

// TYPES -------------------------------------------------------------------

typedef enum
{
	EXFLAG_ROTATE = 1,
	EXFLAG_COLLIDE = 2,
	EXFLAG_STILL_FRAME = 4
} exflags_t;


struct explosion_t
{
	vec3_t		origin;
	vec3_t		oldorg;// holds position from last frame
	float		startTime;
	float		endTime;
	vec3_t		velocity;
	vec3_t		accel;
	vec3_t		angles;
	vec3_t		avel;	// angular velocity
	int			flags;
	int			abslight;
	int	        exflags;
	int			skin;
	int			scale;
	qhandle_t	model;
	void (*frameFunc)(explosion_t *ex);
	void (*removeFunc)(explosion_t *ex);
	float		data; //for easy transition of script code that relied on counters of some sort
};
// PUBLIC FUNCTION DEFINITIONS ---------------------------------------------

int	TempSoundChannel()
{
	static int last = -1;
	last--;
	if(last<-20) last=-1;
	return last;
}



// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void ParseStream(int type);
static h2stream_t *NewStream(int ent, int tag, int *isNew);

void MultiGrenadeThink (explosion_t *ex);
void MultiGrenadePieceThink (explosion_t *ex);
void MultiGrenadePiece2Think (explosion_t *ex);
void ChunkThink(explosion_t *ex);
void BubbleThink(explosion_t *ex);
void MissileFlashThink(explosion_t *ex);
void TeleportFlashThink(explosion_t *ex);
void CheckSpaceThink(explosion_t *ex);
void SwordFrameFunc(explosion_t *ex);
void zapFrameFunc(explosion_t *ex);
void fireBallUpdate(explosion_t *ex);
void sunBallUpdate(explosion_t *ex);
void sunPowerUpdate(explosion_t *ex);
void purify1Update(explosion_t *ex);
void telEffectUpdate (explosion_t *ex);
void CL_UpdateTargetBall(void);
void updateBloodRain(explosion_t *ex);
void updatePurify2(explosion_t *ex);
void updateSwordShot(explosion_t *ex);
void updateIceShot(explosion_t *ex);
void updateMeteor(explosion_t *ex);
void SmokeRingFrameFunc(explosion_t *ex);
void updateAcidBlob(explosion_t *ex);
void updateAcidBall(explosion_t *ex);
void MeteorCrushSpawnThink(explosion_t *ex);

// PRIVATE DATA DEFINITIONS ------------------------------------------------

explosion_t		cl_explosions[MAX_EXPLOSIONS];

static int		MultiGrenadeCurrentChannel;

//static sfxHandle_t		cl_sfx_wizhit;
//static sfxHandle_t		cl_sfx_knighthit;
static sfxHandle_t		cl_sfx_tink1;
static sfxHandle_t		cl_sfx_ric1;
static sfxHandle_t		cl_sfx_ric2;
static sfxHandle_t		cl_sfx_ric3;
static sfxHandle_t		cl_sfx_r_exp3;
static sfxHandle_t		cl_sfx_explode;
static sfxHandle_t		cl_sfx_bonehit;
static sfxHandle_t		cl_sfx_bonewal;
static sfxHandle_t		cl_sfx_bonephit;
static sfxHandle_t		cl_sfx_ravendie;
static sfxHandle_t		cl_sfx_buzzbee;

static sfxHandle_t		cl_sfx_iceflesh;
static sfxHandle_t		cl_sfx_icewall;
static sfxHandle_t		cl_sfx_iceshatter;
static sfxHandle_t		cl_sfx_icestorm;
static sfxHandle_t		cl_sfx_sunstaff;
static sfxHandle_t		cl_sfx_sunhit;
static sfxHandle_t		cl_sfx_lightning1;
static sfxHandle_t		cl_sfx_lightning2;
static sfxHandle_t		cl_sfx_hammersound;
static sfxHandle_t		cl_sfx_tornado;
static sfxHandle_t		cl_sfx_swordExplode;
static sfxHandle_t		cl_sfx_axeBounce;
static sfxHandle_t		cl_sfx_axeExplode;
static sfxHandle_t		cl_sfx_fireBall;
static sfxHandle_t		cl_sfx_purify2;
static sfxHandle_t		cl_sfx_telefrag;
static sfxHandle_t		cl_sfx_big_gib;
static sfxHandle_t		cl_sfx_gib1;
static sfxHandle_t		cl_sfx_gib2;
static sfxHandle_t		cl_sfx_purify1_fire;
static sfxHandle_t		cl_sfx_purify1_hit;
static sfxHandle_t		cl_sfx_acidhit;
static sfxHandle_t		cl_sfx_dropfizzle;
static sfxHandle_t		cl_sfx_flameend;

static sfxHandle_t		cl_sfx_teleport[5];
static sfxHandle_t		cl_sfx_ravengo;



/*
=================
CL_ParseTEnts
=================
*/
void CL_InitTEnts (void)
{
//	cl_sfx_wizhit = S_RegisterSound("wizard/hit.wav");
//	cl_sfx_knighthit = S_RegisterSound("hknight/hit.wav");
	cl_sfx_tink1 = S_RegisterSound("weapons/tink1.wav");
	cl_sfx_ric1 = S_RegisterSound("weapons/ric1.wav");
	cl_sfx_ric2 = S_RegisterSound("weapons/ric2.wav");
	cl_sfx_ric3 = S_RegisterSound("weapons/ric3.wav");
	cl_sfx_r_exp3 = S_RegisterSound("weapons/r_exp3.wav");
	cl_sfx_explode = S_RegisterSound("weapons/explode.wav");
	cl_sfx_bonephit = S_RegisterSound("necro/bonephit.wav");
	cl_sfx_bonehit = S_RegisterSound("necro/bonenhit.wav");
	cl_sfx_bonewal = S_RegisterSound("necro/bonenwal.wav");
	cl_sfx_ravendie = S_RegisterSound("raven/death.wav");
	cl_sfx_buzzbee = S_RegisterSound("assassin/scrbfly.wav");

	cl_sfx_iceflesh = S_RegisterSound("crusader/icehit.wav");
	cl_sfx_icewall = S_RegisterSound("crusader/icewall.wav");
	cl_sfx_iceshatter = S_RegisterSound("misc/icestatx.wav");
	cl_sfx_icestorm = S_RegisterSound("crusader/blizzard.wav");
	cl_sfx_sunstaff = S_RegisterSound("crusader/sunhum.wav");
	cl_sfx_sunhit = S_RegisterSound("crusader/sunhit.wav");
	cl_sfx_lightning1 = S_RegisterSound("crusader/lghtn1.wav");
	cl_sfx_lightning2 = S_RegisterSound("crusader/lghtn2.wav");
	cl_sfx_hammersound = S_RegisterSound("paladin/axblade.wav");
	cl_sfx_tornado = S_RegisterSound("crusader/tornado.wav");
	cl_sfx_swordExplode = S_RegisterSound("weapons/explode.wav");
	cl_sfx_axeBounce = S_RegisterSound("paladin/axric1.wav");
	cl_sfx_axeExplode = S_RegisterSound("weapons/explode.wav");
	cl_sfx_fireBall = S_RegisterSound("weapons/fbfire.wav");
	cl_sfx_purify2 = S_RegisterSound("weapons/exphuge.wav");
	cl_sfx_telefrag = S_RegisterSound("player/telefrag.wav");
	cl_sfx_big_gib = S_RegisterSound("player/megagib.wav");
	cl_sfx_gib1 = S_RegisterSound("player/gib1.wav");
	cl_sfx_gib2 = S_RegisterSound("player/gib2.wav");
	cl_sfx_purify1_fire = S_RegisterSound("paladin/purfire.wav");
	cl_sfx_purify1_hit = S_RegisterSound("weapons/expsmall.wav");
	cl_sfx_acidhit = S_RegisterSound("succubus/acidhit.wav");
	cl_sfx_dropfizzle = S_RegisterSound("succubus/dropfizz.wav");
	cl_sfx_flameend = S_RegisterSound("succubus/flamend.wav");


	cl_sfx_teleport[0] = S_RegisterSound("misc/teleprt1.wav");
	cl_sfx_teleport[1] = S_RegisterSound("misc/teleprt2.wav");
	cl_sfx_teleport[2] = S_RegisterSound("misc/teleprt3.wav");
	cl_sfx_teleport[3] = S_RegisterSound("misc/teleprt4.wav");
	cl_sfx_teleport[4] = S_RegisterSound("misc/teleprt5.wav");

	cl_sfx_ravengo = S_RegisterSound("raven/ravengo.wav");
}


static void vectoangles(vec3_t vec, vec3_t ang)
{
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
		VecToAnglesCommon(vec, ang, yaw, pitch);
		if (pitch < 0)
			pitch += 360;
	}

	ang[0] = pitch;
	ang[1] = yaw;
	ang[2] = 0;
}



/*
=================
CL_ClearTEnts
=================
*/
void CL_ClearTEnts (void)
{
	Com_Memset(cl_explosions, 0, sizeof(cl_explosions));
	CLH2_ClearStreams();
}

/*
=================
CL_AllocExplosion
**** CAREFUL!!! This may overwrite an explosion!!!!!
=================
*/
explosion_t *CL_AllocExplosion (void)
{
	int		i,freeSlot;
	float	time;
	int		index;
	
	index = 0;
	freeSlot = false;

	for (i=0 ; i<MAX_EXPLOSIONS ; i++)
	{
		if (!cl_explosions[i].model)
		{
			index = i;
			freeSlot = true;
			break;
		}
	}


// find the oldest explosion
	time = cl.serverTimeFloat;

	if (!freeSlot)
	{
		for (i=0 ; i<MAX_EXPLOSIONS ; i++)
		{
			if (cl_explosions[i].startTime < time)
			{
				time = cl_explosions[i].startTime;
				index = i;
			}
		}
	}

	//zero out velocity and acceleration, funcs
	Com_Memset(&cl_explosions[index], 0, sizeof(explosion_t));

	return &cl_explosions[index];
}

/*
=================
CL_ParseBeam
=================
*/
void CL_ParseBeam ()
{
	net_message.ReadShort();
	
	net_message.ReadCoord();
	net_message.ReadCoord();
	net_message.ReadCoord();
	
	net_message.ReadCoord();
	net_message.ReadCoord();
	net_message.ReadCoord();
}

entity_state_t *FindState(int EntNum)
{
	packet_entities_t		*pack;
	static entity_state_t	pretend_player;
	int						pnum;

	if (EntNum >= 1 && EntNum <= MAX_CLIENTS)
	{
		EntNum--;

		if (EntNum == cl.playernum)
		{
			VectorCopy(cl.simorg, pretend_player.origin);
		}
		else
		{
			VectorCopy(cl.frames[cls.netchan.incoming_sequence&UPDATE_MASK].playerstate[EntNum].origin, pretend_player.origin);
		}

		return &pretend_player;
	}

	pack = &cl.frames[cls.netchan.incoming_sequence&UPDATE_MASK].packet_entities;
	for (pnum=0 ; pnum<pack->num_entities ; pnum++)
	{
		if (pack->entities[pnum].number == EntNum)
		{
			return &pack->entities[pnum];
		}
	}

	return NULL;
}

//==========================================================================
//
// CreateStream--for use mainly external to cl_tent (i.e. cl_effect)
//
// generally pass in 0 for skin
//
//==========================================================================

void CreateStream(int type, int ent, int flags, int tag, float duration, int skin, vec3_t source, vec3_t dest)
{
	h2stream_t		*stream;
	qhandle_t		models[4];
	entity_state_t	*state;

	models[1] = models[2] = models[3] = 0;
	switch(type)
	{
	case TE_STREAM_CHAIN:
		models[0] = R_RegisterModel("models/stchain.mdl");
		break;
	case TE_STREAM_SUNSTAFF1:
		models[0] = R_RegisterModel("models/stsunsf1.mdl");
		models[1] = R_RegisterModel("models/stsunsf2.mdl");
		models[2] = R_RegisterModel("models/stsunsf3.mdl");
		models[3] = R_RegisterModel("models/stsunsf4.mdl");
		break;
	case TE_STREAM_SUNSTAFF2:
		models[0] = R_RegisterModel("models/stsunsf5.mdl");
		models[2] = R_RegisterModel("models/stsunsf3.mdl");
		models[3] = R_RegisterModel("models/stsunsf4.mdl");
		break;
	case TE_STREAM_LIGHTNING:
		models[0] = R_RegisterModel("models/stlghtng.mdl");
		break;
	case TE_STREAM_LIGHTNING_SMALL:
		models[0] = R_RegisterModel("models/stltng2.mdl");
		break;
	case TE_STREAM_FAMINE:
		models[0] = R_RegisterModel("models/fambeam.mdl");
		break;
	case TE_STREAM_COLORBEAM:
		models[0] = R_RegisterModel("models/stclrbm.mdl");
		break;
	case TE_STREAM_ICECHUNKS:
		models[0] = R_RegisterModel("models/stice.mdl");
		break;
	case TE_STREAM_GAZE:
		models[0] = R_RegisterModel("models/stmedgaz.mdl");
		break;
	default:
		Sys_Error("CreateStream: bad type");
	}

	if((stream = NewStream(ent, tag, NULL)) == NULL)
	{
		Con_Printf("stream list overflow\n");
		return;
	}
	stream->type = type;
	stream->tag = tag;
	stream->flags = flags;
	stream->entity = ent;
	stream->skin = skin;
	stream->models[0] = models[0];
	stream->models[1] = models[1];
	stream->models[2] = models[2];
	stream->models[3] = models[3];
	stream->endTime = cl.serverTimeFloat+duration;
	stream->lastTrailTime = 0;
	VectorCopy(source, stream->source);
	VectorCopy(dest, stream->dest);
	if(flags&H2STREAM_ATTACHED)
	{
		VectorCopy(vec3_origin, stream->offset);

		state = FindState(ent);
		if (state)
		{	// rjr - potential problem if this doesn't ever get set - origin might have to be set properly in script code?
			VectorSubtract(source, state->origin, stream->offset);
		}
	}
}

void CLTENT_SpawnDeathBubble(vec3_t pos)
{
	explosion_t	*ex;

	//generic spinny impact image
	ex=CL_AllocExplosion();
	VectorCopy(pos,ex->origin);
	VectorSet(ex->velocity,0,0,17);
	ex->data=cl.serverTimeFloat;
	ex->scale=128;
	ex->frameFunc = BubbleThink;
	ex->startTime=cl.serverTimeFloat;
	ex->endTime=cl.serverTimeFloat+15;
	ex->model=R_RegisterModel ("models/s_bubble.spr");
	ex->flags = DRF_TRANSLUCENT | MLS_ABSLIGHT;
	ex->abslight=175;
}

void CLTENT_XbowImpact(vec3_t pos, vec3_t vel, int chType, int damage, int arrowType)//arrowType is total # of arrows in effect
{
	explosion_t	*ex;
	float cnt;
	int i;

	//generic spinny impact image
	ex=CL_AllocExplosion();
	ex->origin[0]=pos[0]-vel[0];
	ex->origin[1]=pos[1]-vel[1];
	ex->origin[2]=pos[2]-vel[2];
	vectoangles(vel,ex->angles);
	ex->avel[2]=(rand()%500)+200;
	ex->scale=10;
	ex->frameFunc = MissileFlashThink;
	ex->startTime=cl.serverTimeFloat;
	ex->endTime=cl.serverTimeFloat+0.3;
	ex->model=R_RegisterModel ("models/arrowhit.mdl");
	ex->exflags = EXFLAG_ROTATE;
	ex->flags = DRF_TRANSLUCENT | MLS_ABSLIGHT;
	ex->abslight=175;

	//white smoke if invulnerable impact
	if (!damage)
	{
		ex=CL_AllocExplosion();
		ex->origin[0]=pos[0]-vel[0]*2;
		ex->origin[1]=pos[1]-vel[1]*2;
		ex->origin[2]=pos[2]-vel[2]*2;
		ex->velocity[0] = 0.0;
		ex->velocity[1] = 0.0;
		ex->velocity[2] = 80.0;
		vectoangles(vel,ex->angles);
		ex->startTime=cl.serverTimeFloat;
		ex->endTime=cl.serverTimeFloat+0.35;
		ex->model=R_RegisterModel ("models/whtsmk1.spr");
		ex->flags = DRF_TRANSLUCENT;


		if (arrowType == 3)//little arrows go away
		{

			if (rand()&3)//chunky go
			{
				cnt	= rand()%2+1;

				for(i = 0; i < cnt; i++)
				{
					float final;

					ex = CL_AllocExplosion();
					ex->frameFunc = ChunkThink;

					VectorSubtract(pos,vel,ex->origin);
					// temp modify them...
					ex->velocity[0] = (rand()%140)-70;
					ex->velocity[1] = (rand()%140)-70;
					ex->velocity[2] = (rand()%140)-70;

					// are these in degrees or radians?
					ex->angles[0] = rand()%360;
					ex->angles[1] = rand()%360;
					ex->angles[2] = rand()%360;
					ex->exflags = EXFLAG_ROTATE;

					ex->avel[0] = rand()%850 - 425;
					ex->avel[1] = rand()%850 - 425;
					ex->avel[2] = rand()%850 - 425;

					ex->scale = 30 + 100 * (cnt / 40.0) + rand()%40;

					ex->data = THINGTYPE_WOOD;

					final = (rand()%100)*.01;

					if (final < 0.25)
						ex->model = R_RegisterModel ("models/splnter1.mdl");
					else if (final < 0.50)
						ex->model = R_RegisterModel ("models/splnter2.mdl");
					else if (final < 0.75)
						ex->model = R_RegisterModel ("models/splnter3.mdl");
					else 
						ex->model = R_RegisterModel ("models/splnter4.mdl");

					ex->startTime = cl.serverTimeFloat;
					ex->endTime = ex->startTime + 4.0;
				}
			}
			else if (rand()&1)//whole go
			{
					ex = CL_AllocExplosion();
					ex->frameFunc = ChunkThink;

					VectorSubtract(pos,vel,ex->origin);
					// temp modify them...
					ex->velocity[0] = (rand()%140)-70;
					ex->velocity[1] = (rand()%140)-70;
					ex->velocity[2] = (rand()%140)-70;

					// are these in degrees or radians?
					ex->angles[0] = rand()%360;
					ex->angles[1] = rand()%360;
					ex->angles[2] = rand()%360;
					ex->exflags = EXFLAG_ROTATE;

					ex->avel[0] = rand()%850 - 425;
					ex->avel[1] = rand()%850 - 425;
					ex->avel[2] = rand()%850 - 425;

					ex->scale = 128;

					ex->data = THINGTYPE_WOOD;

					ex->model = R_RegisterModel ("models/arrow.mdl");

					ex->startTime = cl.serverTimeFloat;
					ex->endTime = ex->startTime + 4.0;
			}
		}

	}

}
//==========================================================================
//
// ParseStream
//
//==========================================================================

static void ParseStream(int type)
{
	int				ent;
	int				tag;
	int				flags;
	int				skin;
	vec3_t			source;
	vec3_t			dest;
	h2stream_t		*stream;
	float			duration;
	qhandle_t		models[4];
	entity_state_t	*state;

	ent = net_message.ReadShort();
	flags = net_message.ReadByte();
	tag = flags&15;
	duration = (float)net_message.ReadByte()*0.05;
	skin = 0;
	if(type == TE_STREAM_COLORBEAM)
	{
		skin = net_message.ReadByte();
	}
	source[0] = net_message.ReadCoord();
	source[1] = net_message.ReadCoord();
	source[2] = net_message.ReadCoord();
	dest[0] = net_message.ReadCoord();
	dest[1] = net_message.ReadCoord();
	dest[2] = net_message.ReadCoord();

	models[1] = models[2] = models[3] = 0;
	switch(type)
	{
	case TE_STREAM_CHAIN:
		models[0] = R_RegisterModel("models/stchain.mdl");
		break;
	case TE_STREAM_SUNSTAFF1:
		models[0] = R_RegisterModel("models/stsunsf1.mdl");
		models[1] = R_RegisterModel("models/stsunsf2.mdl");
		models[2] = R_RegisterModel("models/stsunsf3.mdl");
		models[3] = R_RegisterModel("models/stsunsf4.mdl");
		break;
	case TE_STREAM_SUNSTAFF2:
		models[0] = R_RegisterModel("models/stsunsf5.mdl");
		models[2] = R_RegisterModel("models/stsunsf3.mdl");
		models[3] = R_RegisterModel("models/stsunsf4.mdl");
		break;
	case TE_STREAM_LIGHTNING:
		models[0] = R_RegisterModel("models/stlghtng.mdl");
		break;
	case TE_STREAM_LIGHTNING_SMALL:
		models[0] = R_RegisterModel("models/stltng2.mdl");
		break;
	case TE_STREAM_FAMINE:
		models[0] = R_RegisterModel("models/fambeam.mdl");
		break;
	case TE_STREAM_COLORBEAM:
		models[0] = R_RegisterModel("models/stclrbm.mdl");
		break;
	case TE_STREAM_ICECHUNKS:
		models[0] = R_RegisterModel("models/stice.mdl");
		break;
	case TE_STREAM_GAZE:
		models[0] = R_RegisterModel("models/stmedgaz.mdl");
		break;
	default:
		Sys_Error("ParseStream: bad type");
	}

	if((stream = NewStream(ent, tag, NULL)) == NULL)
	{
		Con_Printf("stream list overflow\n");
		return;
	}
	stream->type = type;
	stream->tag = tag;
	stream->flags = flags;
	stream->entity = ent;
	stream->skin = skin;
	stream->models[0] = models[0];
	stream->models[1] = models[1];
	stream->models[2] = models[2];
	stream->models[3] = models[3];
	stream->endTime = cl.serverTimeFloat+duration;
	stream->lastTrailTime = 0;
	VectorCopy(source, stream->source);
	VectorCopy(dest, stream->dest);
	if(flags&H2STREAM_ATTACHED)
	{
		VectorCopy(vec3_origin, stream->offset);

		state = FindState(ent);
		if (state)
		{	// rjr - potential problem if this doesn't ever get set - origin might have to be set properly in script code?
			VectorSubtract(source, state->origin, stream->offset);
		}
	}
}

//==========================================================================
//
// NewStream
//
//==========================================================================

static h2stream_t *NewStream(int ent, int tag, int *isNew)
{
	int i;
	h2stream_t *stream;

	// Search for a stream with matching entity and tag
	for(i = 0, stream = clh2_Streams; i < MAX_STREAMS_H2; i++, stream++)
	{
		if(stream->entity == ent && stream->tag == tag)
		{
			if(isNew)
			{
				if(stream->endTime > cl.serverTimeFloat)
				{
					*isNew = 0;
				}
				else
				{	// this stream was already used up
					*isNew = 1;
				}
			}
			return stream;
		}
	}
	// Search for a free stream
	for(i = 0, stream = clh2_Streams; i < MAX_STREAMS_H2; i++, stream++)
	{
		if(!stream->models[0] || stream->endTime < cl.serverTimeFloat)
		{
			if(isNew)*isNew = 1;
			return stream;
		}
	}
	return NULL;
}

/*
=================
CL_ParseTEnt
=================
*/
void CL_ParseTEnt (void)
{
	int		type;
	vec3_t	pos, vel, movedir, offset;
	int		rnd;
	explosion_t	*ex;
	int		cnt, cnt2, i, chType;
	int damage;
	float	volume, scale;
	float dir, cosval, sinval;

	type = net_message.ReadByte ();
	switch (type)
	{
		case TE_WIZSPIKE:			// spike hitting wall
			pos[0] = net_message.ReadCoord ();
			pos[1] = net_message.ReadCoord ();
			pos[2] = net_message.ReadCoord ();
			CLH2_RunParticleEffect (pos, vec3_origin, 30);
			break;
			
		case TE_KNIGHTSPIKE:			// spike hitting wall
			pos[0] = net_message.ReadCoord ();
			pos[1] = net_message.ReadCoord ();
			pos[2] = net_message.ReadCoord ();
			CLH2_RunParticleEffect (pos, vec3_origin, 20);
			break;
			
		case TE_SPIKE:			// spike hitting wall
			pos[0] = net_message.ReadCoord ();
			pos[1] = net_message.ReadCoord ();
			pos[2] = net_message.ReadCoord ();
			CLH2_RunParticleEffect (pos, vec3_origin, 10);

			if ( rand() % 5 )
				S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_tink1, 1, 1);
			else
			{
				rnd = rand() & 3;
				if (rnd == 1)
					S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_ric1, 1, 1);
				else if (rnd == 2)
					S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_ric2, 1, 1);
				else
					S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_ric3, 1, 1);
			}
			break;
		case TE_SUPERSPIKE:			// super spike hitting wall
			pos[0] = net_message.ReadCoord ();
			pos[1] = net_message.ReadCoord ();
			pos[2] = net_message.ReadCoord ();
			CLH2_RunParticleEffect (pos, vec3_origin, 20);

			if ( rand() % 5 )
				S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_tink1, 1, 1);
			else
			{
				rnd = rand() & 3;
				if (rnd == 1)
					S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_ric1, 1, 1);
				else if (rnd == 2)
					S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_ric2, 1, 1);
				else
					S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_ric3, 1, 1);
			}
			break;

		case TE_DRILLA_EXPLODE:
		// particles
			pos[0] = net_message.ReadCoord ();
			pos[1] = net_message.ReadCoord ();
			pos[2] = net_message.ReadCoord ();
			ex = CL_AllocExplosion();
			VectorCopy(pos,ex->origin);

			ex->model = R_RegisterModel("models/gen_expl.spr");

			ex->startTime = cl.serverTimeFloat;
			ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.1;
		
		// sound
			S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_explode, 1, 1);
			break;
			
		case TE_EXPLOSION:			// rocket explosion
		// particles
			pos[0] = net_message.ReadCoord ();
			pos[1] = net_message.ReadCoord ();
			pos[2] = net_message.ReadCoord ();
			CLH2_ParticleExplosion (pos);
			
		// light
			CLH2_ExplosionLight(pos);
		
		// sound
			S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_r_exp3, 1, 1);
			break;
			
		case TE_TAREXPLOSION:			// tarbaby explosion
			pos[0] = net_message.ReadCoord ();
			pos[1] = net_message.ReadCoord ();
			pos[2] = net_message.ReadCoord ();
			CLH2_BlobExplosion (pos);

			S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_r_exp3, 1, 1);
			break;

		case TE_LIGHTNING1:				// lightning bolts
		case TE_LIGHTNING2:
		case TE_LIGHTNING3:
			CL_ParseBeam();
			break;

		case TE_STREAM_CHAIN:
		case TE_STREAM_SUNSTAFF1:
		case TE_STREAM_SUNSTAFF2:
		case TE_STREAM_LIGHTNING:
		case TE_STREAM_LIGHTNING_SMALL:
		case TE_STREAM_COLORBEAM:
		case TE_STREAM_ICECHUNKS:
		case TE_STREAM_GAZE:
		case TE_STREAM_FAMINE:
			ParseStream(type);
			break;

		case TE_LAVASPLASH:	
			pos[0] = net_message.ReadCoord ();
			pos[1] = net_message.ReadCoord ();
			pos[2] = net_message.ReadCoord ();
			CLH2_LavaSplash (pos);
			break;
		
		case TE_TELEPORT:
			pos[0] = net_message.ReadCoord ();
			pos[1] = net_message.ReadCoord ();
			pos[2] = net_message.ReadCoord ();
			CLH2_TeleportSplash (pos);
			break;

		case TE_GUNSHOT:			// bullet hitting wall
			cnt = net_message.ReadByte ();
			pos[0] = net_message.ReadCoord ();
			pos[1] = net_message.ReadCoord ();
			pos[2] = net_message.ReadCoord ();
			CLH2_RunParticleEffect (pos, vec3_origin, 20*cnt);
			break;
			
		case TE_BLOOD:				// bullets hitting body
			cnt = net_message.ReadByte ();
			pos[0] = net_message.ReadCoord ();
			pos[1] = net_message.ReadCoord ();
			pos[2] = net_message.ReadCoord ();
			CLH2_RunParticleEffect (pos, vec3_origin, 20*cnt);
			break;

		case TE_LIGHTNINGBLOOD:		// lightning hitting body
			pos[0] = net_message.ReadCoord ();
			pos[1] = net_message.ReadCoord ();
			pos[2] = net_message.ReadCoord ();
			CLH2_RunParticleEffect (pos, vec3_origin, 50);
			break;

		case TE_BIGGRENADE:	// effect for big grenade
			pos[0] = net_message.ReadCoord ();
			pos[1] = net_message.ReadCoord ();
			pos[2] = net_message.ReadCoord ();
			CLH2_ParticleExplosion(pos);
			ex = CL_AllocExplosion();
			VectorCopy(pos,ex->origin);
			ex->frameFunc = MultiGrenadeThink;

			ex->data=250;

			ex->model = R_RegisterModel("models/sm_expld.spr");

			ex->startTime = cl.serverTimeFloat;
			ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.1;
			break;

		case TE_CHUNK:		//directed chunks
		case TE_CHUNK2:		//volume based chunks

			pos[0] = net_message.ReadCoord();
			pos[1] = net_message.ReadCoord();
			pos[2] = net_message.ReadCoord();
			vel[0] = net_message.ReadCoord();	//vel for CHUNK, volume for CHUNK2
			vel[1] = net_message.ReadCoord();
			vel[2] = net_message.ReadCoord();
			chType = net_message.ReadByte();

			if(type == TE_CHUNK)
			{
				cnt	= net_message.ReadByte();
			}
			else
			{
				volume = vel[0] * vel[1] * vel[2];
				cnt = volume / 8192;
				if (volume < 5000)
				{
					scale = .20;
					cnt *= 3;	// Because so few pieces come out of a small object
				}
				else if (volume < 50000)
				{
					scale = .45;
					cnt *= 3;	// Because so few pieces come out of a small object
				}
				else if (volume < 500000)
				{
					scale = .50;
				}
				else if (volume < 1000000)
				{
					scale = .75;
				}
				else
				{
					scale = 1;
				}
				if(cnt > 30)
				{
					cnt = 30;
				}
			}

			for(i = 0; i < cnt; i++)
			{
				float final;

				ex = CL_AllocExplosion();
				ex->frameFunc = ChunkThink;

				if(type == TE_CHUNK)
				{
					VectorCopy(pos,ex->origin);
					VectorCopy(vel, ex->velocity);
					VectorScale(ex->velocity, .80 + ((rand()%4)/10.0), ex->velocity);
					// temp modify them...
					ex->velocity[0] += (rand()%140)-70;
					ex->velocity[1] += (rand()%140)-70;
					ex->velocity[2] += (rand()%140)-70;

					// are these in degrees or radians?
					ex->angles[0] = rand()%360;
					ex->angles[1] = rand()%360;
					ex->angles[2] = rand()%360;
					ex->exflags = EXFLAG_ROTATE;

					ex->avel[0] = rand()%850 - 425;
					ex->avel[1] = rand()%850 - 425;
					ex->avel[2] = rand()%850 - 425;

					ex->scale = 30 + 100 * (cnt / 40.0) + rand()%40;
				}
				else
				{
					// set origin (origin is really absmin here)
					VectorCopy(pos, ex->origin);
					ex->origin[0] += FRANDOM() * vel[0];
					ex->origin[1] += FRANDOM() * vel[1];
					ex->origin[2] += FRANDOM() * vel[2];
					// set velocity
					ex->velocity[0] = -210 + FRANDOM() * 420;
					ex->velocity[1] = -210 + FRANDOM() * 420;
					ex->velocity[2] = -210 + FRANDOM() * 490;
					// set scale
					ex->scale = scale*100;
					// set angles, avel
					ex->angles[0] = rand()%360;
					ex->angles[1] = rand()%360;
					ex->angles[2] = rand()%360;

					ex->avel[0] = rand()%1200;
					ex->avel[1] = rand()%1200;
					ex->avel[2] = rand()%1200;
				}
				ex->data = chType;

				final = (rand()%100)*.01;
				if ((chType==THINGTYPE_GLASS) || (chType==THINGTYPE_REDGLASS) || 
						(chType==THINGTYPE_CLEARGLASS) || (chType==THINGTYPE_WEBS))
				{
					if (final<0.20)
						ex->model = R_RegisterModel ("models/shard1.mdl");
					else if (final<0.40)
						ex->model = R_RegisterModel ("models/shard2.mdl");
					else if (final<0.60)
						ex->model = R_RegisterModel ("models/shard3.mdl");
					else if (final<0.80)
						ex->model = R_RegisterModel ("models/shard4.mdl");
					else 
						ex->model = R_RegisterModel ("models/shard5.mdl");

					if (chType==THINGTYPE_CLEARGLASS)
					{
						ex->skin = 1;
						ex->flags |= DRF_TRANSLUCENT;
					}
					else if (chType==THINGTYPE_REDGLASS)
					{
						ex->skin = 2;
					}
					else if (chType==THINGTYPE_WEBS)
					{
						ex->skin = 3;
						ex->flags |= DRF_TRANSLUCENT;
					}
				}
				else if (chType==THINGTYPE_WOOD)
				{
					if (final < 0.25)
						ex->model = R_RegisterModel ("models/splnter1.mdl");
					else if (final < 0.50)
						ex->model = R_RegisterModel ("models/splnter2.mdl");
					else if (final < 0.75)
						ex->model = R_RegisterModel ("models/splnter3.mdl");
					else 
						ex->model = R_RegisterModel ("models/splnter4.mdl");
				}
				else if (chType==THINGTYPE_METAL)
				{
					if (final < 0.25)
						ex->model = R_RegisterModel ("models/metlchk1.mdl");
					else if (final < 0.50)
						ex->model = R_RegisterModel ("models/metlchk2.mdl");
					else if (final < 0.75)
						ex->model = R_RegisterModel ("models/metlchk3.mdl");
					else 
						ex->model = R_RegisterModel ("models/metlchk4.mdl");
				}
				else if (chType==THINGTYPE_FLESH)
				{
					if (final < 0.33)
						ex->model = R_RegisterModel ("models/flesh1.mdl");
					else if (final < 0.66)
						ex->model = R_RegisterModel ("models/flesh2.mdl");
					else
						ex->model = R_RegisterModel ("models/flesh3.mdl");
				}
				else if (chType==THINGTYPE_BROWNSTONE||chType==THINGTYPE_DIRT)
				{
					if (final < 0.25)
						ex->model = R_RegisterModel ("models/schunk1.mdl");
					else if (final < 0.50)
						ex->model = R_RegisterModel ("models/schunk2.mdl");
					else if (final < 0.75)
						ex->model = R_RegisterModel ("models/schunk3.mdl");
					else 
						ex->model = R_RegisterModel ("models/schunk4.mdl");
					ex->skin = 1;
				}
				else if (chType==THINGTYPE_CLAY)
				{
					if (final < 0.25)
						ex->model = R_RegisterModel ("models/clshard1.mdl");
					else if (final < 0.50)
						ex->model = R_RegisterModel ("models/clshard2.mdl");
					else if (final < 0.75)
						ex->model = R_RegisterModel ("models/clshard3.mdl");
					else 
						ex->model = R_RegisterModel ("models/clshard4.mdl");
				}
				else if (chType==THINGTYPE_LEAVES)
				{
					if (final < 0.33)
						ex->model = R_RegisterModel ("models/leafchk1.mdl");
					else if (final < 0.66)
						ex->model = R_RegisterModel ("models/leafchk2.mdl");
					else 
						ex->model = R_RegisterModel ("models/leafchk3.mdl");
				}
				else if (chType==THINGTYPE_HAY)
				{
					if (final < 0.33)
						ex->model = R_RegisterModel ("models/hay1.mdl");
					else if (final < 0.66)
						ex->model = R_RegisterModel ("models/hay2.mdl");
					else 
						ex->model = R_RegisterModel ("models/hay3.mdl");
				}
				else if (chType==THINGTYPE_CLOTH)
				{
					if (final < 0.33)
						ex->model = R_RegisterModel ("models/clthchk1.mdl");
					else if (final < 0.66)
						ex->model = R_RegisterModel ("models/clthchk2.mdl");
					else 
						ex->model = R_RegisterModel ("models/clthchk3.mdl");
				}
				else if (chType==THINGTYPE_WOOD_LEAF)
				{
					if (final < 0.14)
						ex->model = R_RegisterModel ("models/splnter1.mdl");
					else if (final < 0.28)
						ex->model = R_RegisterModel ("models/leafchk1.mdl");
					else if (final < 0.42)
						ex->model = R_RegisterModel ("models/splnter2.mdl");
					else if (final < 0.56)
						ex->model = R_RegisterModel ("models/leafchk2.mdl");
					else if (final < 0.70)
						ex->model = R_RegisterModel ("models/splnter3.mdl");
					else if (final < 0.84)
						ex->model = R_RegisterModel ("models/leafchk3.mdl");
					else 
						ex->model = R_RegisterModel ("models/splnter4.mdl");
				}
				else if (chType==THINGTYPE_WOOD_METAL)
				{
					if (final < 0.125)
						ex->model = R_RegisterModel ("models/splnter1.mdl");
					else if (final < 0.25)
						ex->model = R_RegisterModel ("models/metlchk1.mdl");
					else if (final < 0.375)
						ex->model = R_RegisterModel ("models/splnter2.mdl");
					else if (final < 0.50)
						ex->model = R_RegisterModel ("models/metlchk2.mdl");
					else if (final < 0.625)
						ex->model = R_RegisterModel ("models/splnter3.mdl");
					else if (final < 0.75)
						ex->model = R_RegisterModel ("models/metlchk3.mdl");
					else if (final < 0.875)
						ex->model = R_RegisterModel ("models/splnter4.mdl");
					else 
						ex->model = R_RegisterModel ("models/metlchk4.mdl");
				}
				else if (chType==THINGTYPE_WOOD_STONE)
				{
					if (final < 0.125)
						ex->model = R_RegisterModel ("models/splnter1.mdl");
					else if (final < 0.25)
						ex->model = R_RegisterModel ("models/schunk1.mdl");
					else if (final < 0.375)
						ex->model = R_RegisterModel ("models/splnter2.mdl");
					else if (final < 0.50)
						ex->model = R_RegisterModel ("models/schunk2.mdl");
					else if (final < 0.625)
						ex->model = R_RegisterModel ("models/splnter3.mdl");
					else if (final < 0.75)
						ex->model = R_RegisterModel ("models/schunk3.mdl");
					else if (final < 0.875)
						ex->model = R_RegisterModel ("models/splnter4.mdl");
					else 
						ex->model = R_RegisterModel ("models/schunk4.mdl");
				}
				else if (chType==THINGTYPE_METAL_STONE)
				{
					if (final < 0.125)
						ex->model = R_RegisterModel ("models/metlchk1.mdl");
					else if (final < 0.25)
						ex->model = R_RegisterModel ("models/schunk1.mdl");
					else if (final < 0.375)
						ex->model = R_RegisterModel ("models/metlchk2.mdl");
					else if (final < 0.50)
						ex->model = R_RegisterModel ("models/schunk2.mdl");
					else if (final < 0.625)
						ex->model = R_RegisterModel ("models/metlchk3.mdl");
					else if (final < 0.75)
						ex->model = R_RegisterModel ("models/schunk3.mdl");
					else if (final < 0.875)
						ex->model = R_RegisterModel ("models/metlchk4.mdl");
					else 
						ex->model = R_RegisterModel ("models/schunk4.mdl");
				}
				else if (chType==THINGTYPE_METAL_CLOTH)
				{
					if (final < 0.14)
						ex->model = R_RegisterModel ("models/metlchk1.mdl");
					else if (final < 0.28)
						ex->model = R_RegisterModel ("models/clthchk1.mdl");
					else if (final < 0.42)
						ex->model = R_RegisterModel ("models/metlchk2.mdl");
					else if (final < 0.56)
						ex->model = R_RegisterModel ("models/clthchk2.mdl");
					else if (final < 0.70)
						ex->model = R_RegisterModel ("models/metlchk3.mdl");
					else if (final < 0.84)
						ex->model = R_RegisterModel ("models/clthchk3.mdl");
					else 
						ex->model = R_RegisterModel ("models/metlchk4.mdl");
				}
				else if (chType==THINGTYPE_ICE)
				{
					ex->model = R_RegisterModel("models/shard.mdl");
					ex->skin = 0;
					//ent->frame = rand()%2;
					ex->flags |= DRF_TRANSLUCENT|MLS_ABSLIGHT;
					//ent->abslight = 0.5;
				}
				else if (chType==THINGTYPE_METEOR)
				{
					ex->model = R_RegisterModel("models/tempmetr.mdl");
					ex->skin = 0;
					//ex->scale *= .6;
					VectorScale(ex->avel, 4.0, ex->avel);
				}
				else if (chType==THINGTYPE_ACID)
				{	// no spinning if possible...
					ex->model = R_RegisterModel("models/sucwp2p.mdl");
					ex->skin = 0;
				}
				else if (chType==THINGTYPE_GREENFLESH)
				{	// spider guts
					if (final < 0.33)
						ex->model = R_RegisterModel ("models/sflesh1.mdl");
					else if (final < 0.66)
						ex->model = R_RegisterModel ("models/sflesh2.mdl");
					else
						ex->model = R_RegisterModel ("models/sflesh3.mdl");

					ex->skin = 0;
				}
				else// if (chType==THINGTYPE_GREYSTONE)
				{
					if (final < 0.25)
						ex->model = R_RegisterModel ("models/schunk1.mdl");
					else if (final < 0.50)
						ex->model = R_RegisterModel ("models/schunk2.mdl");
					else if (final < 0.75)
						ex->model = R_RegisterModel ("models/schunk3.mdl");
					else 
						ex->model = R_RegisterModel ("models/schunk4.mdl");
					ex->skin = 0;
				}

				ex->startTime = cl.serverTimeFloat;
				ex->endTime = ex->startTime + 4.0;
			}
			break;

		case TE_XBOWHIT:
			pos[0] = net_message.ReadCoord();
			pos[1] = net_message.ReadCoord();
			pos[2] = net_message.ReadCoord();
			vel[0] = net_message.ReadCoord();
			vel[1] = net_message.ReadCoord();
			vel[2] = net_message.ReadCoord();
			chType = net_message.ReadByte();
			damage = net_message.ReadByte();

			// do a particle effect here, with the color depending on chType

			// impact sound:
			switch (chType)
			{
			case THINGTYPE_FLESH:
//				S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_arr2flsh, 1, 1);
				break;
			case THINGTYPE_WOOD:
//				S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_arr2wood, 1, 1);
				break;
			default:
//				S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_met2stn, 1, 1);
				break;
			}

			//generic spinny impact image
			ex=CL_AllocExplosion();
			ex->origin[0]=pos[0]-vel[0];
			ex->origin[1]=pos[1]-vel[1];
			ex->origin[2]=pos[2]-vel[2];
			vectoangles(vel,ex->angles);
			ex->avel[2]=(rand()%500)+200;
			ex->scale=10;
			ex->startTime=cl.serverTimeFloat;
			ex->endTime=cl.serverTimeFloat+0.3;
			ex->model=R_RegisterModel ("models/arrowhit.mdl");
			ex->exflags = EXFLAG_ROTATE;
			ex->flags = DRF_TRANSLUCENT | MLS_ABSLIGHT;
			ex->abslight=128;

			//white smoke if invulnerable impact
			if (!damage)
			{
				ex=CL_AllocExplosion();
				ex->origin[0]=pos[0]-vel[0]*2;
				ex->origin[1]=pos[1]-vel[1]*2;
				ex->origin[2]=pos[2]-vel[2]*2;
				ex->velocity[0] = 0.0;
				ex->velocity[1] = 0.0;
				ex->velocity[2] = 80.0;
				vectoangles(vel,ex->angles);
				ex->startTime=cl.serverTimeFloat;
				ex->endTime=cl.serverTimeFloat+0.35;
				ex->model=R_RegisterModel ("models/whtsmk1.spr");
				ex->flags = DRF_TRANSLUCENT;
			}

			break;

		case TE_METEORHIT:
			pos[0] = net_message.ReadCoord();
			pos[1] = net_message.ReadCoord();
			pos[2] = net_message.ReadCoord();

			// always make 8 meteors
			i = (host_frametime < .07) ? 0 : 4;	// based on framerate
			for( ; i < 8; i++)
			{
				float final;

				ex = CL_AllocExplosion();
				VectorCopy(pos,ex->origin);
				ex->frameFunc = ChunkThink;

				// temp modify them...
				ex->velocity[0] += (rand()%400)-200;
				ex->velocity[1] += (rand()%400)-200;
				ex->velocity[2] += (rand()%200)+150;

				// are these in degrees or radians?
				ex->angles[0] = rand()%360;
				ex->angles[1] = rand()%360;
				ex->angles[2] = rand()%360;
				ex->exflags = EXFLAG_ROTATE;

				ex->avel[0] = rand()%850 - 425;
				ex->avel[1] = rand()%850 - 425;
				ex->avel[2] = rand()%850 - 425;

				ex->scale = 45 + rand()%10;
				ex->data = THINGTYPE_METEOR;

				final = (rand()%100)*.01;
				ex->model = R_RegisterModel("models/tempmetr.mdl");
				ex->skin = 0;
				VectorScale(ex->avel, 4.0, ex->avel);

				ex->startTime = cl.serverTimeFloat;
				ex->endTime = ex->startTime + 4.0;
			}
			// make the actual explosion
			//for(i = 0; i < 7; i++)
			i = (host_frametime < .07) ? 0 : 8;	// based on framerate
			for( ; i < 11; i++)
			{
				ex = CL_AllocExplosion();
				VectorCopy(pos, ex->origin);
				ex->origin[0] += (rand()%10) - 5;
				ex->origin[1] += (rand()%10) - 5;
				ex->origin[2] += (rand()%10) - 5;

				ex->velocity[0] = (ex->origin[0] - pos[0])*12;
				ex->velocity[1] = (ex->origin[1] - pos[1])*12;
				ex->velocity[2] = (ex->origin[2] - pos[2])*12;

/*				ex->origin[0] += rand()%10 - 5;
				ex->origin[1] += rand()%10 - 5;
				ex->origin[2] += rand()%10 - 5;

				ex->velocity[0] = (ex->origin[0] - pos[0])*10;
				ex->velocity[1] = (ex->origin[1] - pos[1])*10;
				ex->velocity[2] = (ex->origin[2] - pos[2])*10 + 200;*/

				switch(rand()%4)
				{
				case 0:
				case 1:
					ex->model = R_RegisterModel("models/sm_expld.spr");
					break;
				case 2:
					ex->model = R_RegisterModel("models/bg_expld.spr");
					break;
				case 3:
					ex->model = R_RegisterModel("models/gen_expl.spr");
					break;
				}
				if(host_frametime < .07)
				{
					ex->flags |= MLS_ABSLIGHT|DRF_TRANSLUCENT;
				}
				ex->abslight = 160 + rand()%64;
				ex->skin = 0;
				ex->scale = 80 + rand()%40;
				ex->startTime = cl.serverTimeFloat + (rand()%50 / 200.0);
				ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.04;
			}
			S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_axeExplode, 1, 1);
			break;

		case TE_HWBONEPOWER:
			cnt = net_message.ReadByte ();
			pos[0] = net_message.ReadCoord ();
			pos[1] = net_message.ReadCoord ();
			pos[2] = net_message.ReadCoord ();
			movedir[0] = net_message.ReadCoord ();
			movedir[1] = net_message.ReadCoord ();
			movedir[2] = net_message.ReadCoord ();
			CLH2_RunParticleEffect4 (pos, 50, 368 + rand() % 16, pt_h2grav, 10);

			// particle4 (50, rand(368-384), grav, 10);
			ex = CL_AllocExplosion ();
			VectorCopy(pos, ex->origin);
			VectorMA(ex->origin, -6, movedir, ex->origin);
			ex->data=250;
			ex->model = R_RegisterModel("models/sm_expld.spr");
			ex->startTime = cl.serverTimeFloat;
			ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.1;
			for(cnt2=0; cnt2<cnt; cnt2++)
			{
				offset[0] = rand() % 40 - 20;
				offset[1] = rand() % 40 - 20;
				offset[2] = rand() % 40 - 20;
				ex = CL_AllocExplosion ();
				VectorAdd(pos, offset, ex->origin); 
				VectorMA(ex->origin, -8, movedir, ex->origin);
				VectorCopy(offset, ex->velocity);
				ex->velocity[2] += 30;
				ex->data=250;
				ex->model = R_RegisterModel("models/ghost.spr");
				ex->abslight = 128;
				ex->flags = DRF_TRANSLUCENT | MLS_ABSLIGHT;
				ex->startTime = cl.serverTimeFloat;
				ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.1;
			}			
			for(cnt2=0; cnt2<20; cnt2++)
			{
				// want faster velocity to hide the fact that these 
				// aren't in the real location
				offset[0] = rand() % 400 + 300;
				if(rand() % 2)
					offset[0] = -offset[0];
				offset[1] = rand() % 400 + 300;
				if(rand() % 2)
					offset[1] = -offset[1];
				offset[2] = rand() % 400 + 300;
				if(rand() % 2)
					offset[2] = -offset[2];
				ex = CL_AllocExplosion ();
				VectorMA(pos, 1/700, offset, ex->origin); 
				VectorCopy(offset, ex->velocity);
				ex->data=250;
				ex->model = R_RegisterModel("models/boneshrd.mdl");
				ex->startTime = cl.serverTimeFloat;
				ex->endTime = ex->startTime + rand() * 50 / 100;
				ex->flags |= EXFLAG_ROTATE|EXFLAG_COLLIDE;
				ex->angles[0] = rand() % 700;
				ex->angles[1] = rand() % 700;
				ex->angles[2] = rand() % 700;
				ex->avel[0] = rand() % 700;
				ex->avel[1] = rand() % 700;
				ex->avel[2] = rand() % 700;
				ex->frameFunc = CheckSpaceThink;
				
			}
			S_StartSound(pos, TempSoundChannel(), 1, cl_sfx_bonephit, 1, 1);

			break;
		case TE_HWBONEPOWER2:
			cnt2 = net_message.ReadByte ();		//did it hit? changes sound
			pos[0] = net_message.ReadCoord ();
			pos[1] = net_message.ReadCoord ();
			pos[2] = net_message.ReadCoord ();

			// white smoke
			ex = CL_AllocExplosion ();
			VectorCopy(pos, ex->origin);
			ex->model = R_RegisterModel("models/whtsmk1.spr");
			ex->startTime = cl.serverTimeFloat;
			ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.1;
	
			//sound
			if(cnt2)
			{
				S_StartSound(pos, TempSoundChannel(), 1, cl_sfx_bonehit, 1, 1);
			}
			else
			{
				S_StartSound(pos, TempSoundChannel(), 1, cl_sfx_bonewal, 1, 1);
			}

			CLH2_RunParticleEffect4 (pos, 3, 368 + rand() % 16, pt_h2grav, 7);
//			particle4(self.origin,3,random(368,384),PARTICLETYPE_GRAV,self.dmg/2);
			break;
		case TE_HWRAVENDIE:
			pos[0] = net_message.ReadCoord ();
			pos[1] = net_message.ReadCoord ();
			pos[2] = net_message.ReadCoord ();

			ex = CL_AllocExplosion ();
			VectorCopy(pos, ex->origin);
			ex->velocity[1] = 8;
			ex->velocity[2] = -10;
			ex->model = R_RegisterModel("models/whtsmk1.spr");
			ex->startTime = cl.serverTimeFloat;
			ex->endTime = ex->startTime + HX_FRAME_TIME * 10;

			ex = CL_AllocExplosion ();
			VectorCopy(pos, ex->origin);
			ex->velocity[2] = -10;
			ex->model = R_RegisterModel("models/redsmk1.spr");
			ex->startTime = cl.serverTimeFloat;
			ex->endTime = ex->startTime + HX_FRAME_TIME * 10;

			ex = CL_AllocExplosion ();
			VectorCopy(pos, ex->origin);
			ex->velocity[1] = -8;
			ex->velocity[2] = -10;
			ex->model = R_RegisterModel("models/whtsmk1.spr");
			ex->startTime = cl.serverTimeFloat;
			ex->endTime = ex->startTime + HX_FRAME_TIME * 10;
			
			S_StartSound(pos, TempSoundChannel(), 1, cl_sfx_ravendie, 1, 1);
			break;
		case TE_HWRAVENEXPLODE:
			pos[0] = net_message.ReadCoord ();
			pos[1] = net_message.ReadCoord ();
			pos[2] = net_message.ReadCoord ();

			ex = CL_AllocExplosion ();
			VectorCopy(pos, ex->origin);
			ex->velocity[2] = 8;
			ex->model = R_RegisterModel("models/whtsmk1.spr");
			ex->startTime = cl.serverTimeFloat;
			ex->endTime = ex->startTime + HX_FRAME_TIME * 10;

			ex = CL_AllocExplosion ();
			VectorCopy(pos, ex->origin);
			ex->origin[2] -= 5;
			ex->velocity[2] = 8;
			ex->model = R_RegisterModel("models/whtsmk1.spr");
			ex->startTime = cl.serverTimeFloat;
			ex->endTime = ex->startTime + HX_FRAME_TIME * 10;

			ex = CL_AllocExplosion ();
			VectorCopy(pos, ex->origin);
			ex->origin[2] -= 10;
			ex->velocity[2] = 8;
			ex->model = R_RegisterModel("models/whtsmk1.spr");
			ex->startTime = cl.serverTimeFloat;
			ex->endTime = ex->startTime + HX_FRAME_TIME * 10;

			S_StartSound(pos, TempSoundChannel(), 1, cl_sfx_ravengo, 1, 1);
			break;

		case TE_ICEHIT:
			pos[0] = net_message.ReadCoord();
			pos[1] = net_message.ReadCoord();
			pos[2] = net_message.ReadCoord();

			cnt2 = net_message.ReadByte();	// 0 for person, 1 for wall

			if(cnt2)
			{
				i = (host_frametime < .07) ? 0 : 5;	// based on framerate
				for( ; i < 9; i++)
				{
					ex = CL_AllocExplosion();
					VectorCopy(pos,ex->origin);
					ex->frameFunc = ChunkThink;

					ex->velocity[0] += (rand()%1000)-500;
					ex->velocity[1] += (rand()%1000)-500;
					ex->velocity[2] += (rand()%200)-50;

					// are these in degrees or radians?
					ex->angles[0] = rand()%360;
					ex->angles[1] = rand()%360;
					ex->angles[2] = rand()%360;
					ex->exflags = EXFLAG_ROTATE;

					ex->avel[0] = rand()%850 - 425;
					ex->avel[1] = rand()%850 - 425;
					ex->avel[2] = rand()%850 - 425;

					if(cnt2 == 2)
					{
						ex->scale = 65 + rand()%10;
					}
					else
					{
						ex->scale = 35 + rand()%10;
					}
					ex->data = THINGTYPE_ICE;

					ex->model = R_RegisterModel("models/shard.mdl");
					ex->skin = 0;
					//ent->frame = rand()%2;
					ex->flags |= DRF_TRANSLUCENT|MLS_ABSLIGHT;
					ex->abslight = 128;

					ex->startTime = cl.serverTimeFloat;
					ex->endTime = ex->startTime + 2.0;
					if(cnt2 == 2)
					{
						ex->endTime += 3.0;
					}
				}
			}
			else
			{
				vec3_t dmin = {-10, -10, -10};
				vec3_t dmax = {10, 10, 10};
				CLH2_ColouredParticleExplosion(pos,14,10,10);
				CLH2_RunParticleEffect2 (pos, dmin, dmax, 145, pt_h2explode, 14);
			}
			// make the actual explosion
			ex = CL_AllocExplosion();
			VectorCopy(pos, ex->origin);
			ex->model = R_RegisterModel("models/icehit.spr");
			ex->startTime = cl.serverTimeFloat;
			ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.1;

			// Add in the sound
			if(cnt2 == 1)
			{	// hit a wall
				S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_icewall, 1, 1);
			}
			else if (cnt2 == 2)
			{
				S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_iceshatter, 1, 1);
			}
			else
			{	// hit a person
				S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_iceflesh, 1, 1);
			}
			break;

		case TE_ICESTORM:
			{
				int				ent;
				vec3_t			center;
				h2stream_t		*stream;
				qhandle_t		models[2];
				entity_state_t	*state;
				static float	playIceSound = .6;

				ent = net_message.ReadShort();

				state = FindState(ent);
				if (state)
				{
					VectorCopy(state->origin, center);

					playIceSound+=host_frametime;
					if(playIceSound >= .6)
					{
						S_StartSound(center, TempSoundChannel(), 0, cl_sfx_icestorm, 1, 1);
						playIceSound -= .6;
					}

					for (i = 0; i < 5; i++)
					{	// make some ice beams...
						models[0] = R_RegisterModel("models/stice.mdl");

						if((stream = NewStream(ent, i, NULL)) == NULL)
						{
							Con_Printf("stream list overflow\n");
							return;
						}
						stream->type = TE_STREAM_ICECHUNKS;
						stream->tag = (i)&15;// FIXME
						stream->flags = (i+H2STREAM_ATTACHED);
						stream->entity = ent;
						stream->skin = 0;
						stream->models[0] = models[0];
						stream->endTime = cl.serverTimeFloat+0.3;
						stream->lastTrailTime = 0;

						VectorCopy(center, stream->source);
						stream->source[0] += rand()%100 - 50;
						stream->source[1] += rand()%100 - 50;
						VectorCopy(stream->source, stream->dest);
						stream->dest[2] += 128;

						VectorCopy(vec3_origin, stream->offset);

						VectorSubtract(stream->source, state->origin, stream->offset);
					}

				}
			}

			break;
		case TE_HWMISSILEFLASH:
			pos[0] = net_message.ReadCoord ();
			pos[1] = net_message.ReadCoord ();
			pos[2] = net_message.ReadCoord ();
			vel[0] = net_message.ReadCoord ();	//angles
			vel[1] = net_message.ReadCoord ();
			vel[2] = rand() % 360;

			ex = CL_AllocExplosion ();
			VectorCopy(pos, ex->origin);
			VectorCopy(vel, ex->angles);
			ex->frameFunc = MissileFlashThink;
			ex->model = R_RegisterModel("models/handfx.mdl");
			ex->startTime = cl.serverTimeFloat;
			ex->endTime = ex->startTime + 2;
			ex->exflags = EXFLAG_ROTATE;
			ex->avel[2] = rand() * 360 + 360;
			ex->flags = MLS_ABSLIGHT|DRF_TRANSLUCENT;
			ex->abslight = 128;
			//ex->scale = .8 + (rand()%100) * 0.004;  // .8 to 1.2
			ex->scale = 80 + rand()%40;
			break;

		case TE_SUNSTAFF_CHEAP:
			{
				int				ent;
				vec3_t			points[4];
				int				reflect_count;
				short int		tempVal;
				entity_state_t	*state;
				int				i, j;

				h2stream_t		*stream;
				qhandle_t		models[4];


				ent = net_message.ReadShort();
				reflect_count = net_message.ReadByte();

				state = FindState(ent);
				if (state)
				{
					// read in up to 4 points for up to 3 beams
					for(i = 0; i < 3; i++)
					{
						tempVal = net_message.ReadCoord();
						points[0][i] = tempVal;
					}
					for(i = 1; i < 2 + reflect_count; i++)
					{
						for(j = 0; j < 3; j++)
						{
							tempVal = net_message.ReadCoord();
							points[i][j] = tempVal;
						}
					}
					
					// actually create the sun model pieces
					for ( i = 0; i < reflect_count + 1; i++)
					{
						models[0] = R_RegisterModel("models/stsunsf1.mdl");
						models[1] = R_RegisterModel("models/stsunsf2.mdl");
						models[2] = R_RegisterModel("models/stsunsf3.mdl");
						models[3] = R_RegisterModel("models/stsunsf4.mdl");

						//if((stream = NewStream(ent, i, NULL)) == NULL)
						if((stream = NewStream(ent, i, NULL)) == NULL)
						{
							Con_Printf("stream list overflow\n");
							return;
						}
						stream->type = TE_STREAM_SUNSTAFF1;
						stream->tag = i;
						if(!i)
						{
							stream->flags = (i+H2STREAM_ATTACHED);
						}
						else
						{
							stream->flags = i;
						}
						stream->entity = ent;
						stream->skin = 0;
						stream->models[0] = models[0];
						stream->models[1] = models[1];
						stream->models[2] = models[2];
						stream->models[3] = models[3];
						stream->endTime = cl.serverTimeFloat+0.5;	// FIXME
						stream->lastTrailTime = 0;

						VectorCopy(points[i], stream->source);
						VectorCopy(points[i+1], stream->dest);

						if(!i)
						{
							VectorCopy(vec3_origin, stream->offset);
							VectorSubtract(stream->source, state->origin, stream->offset);
						}
					}
				}
				else
				{	// read in everything to keep everything in sync
					for(i = 0; i < (2 + reflect_count)*3; i++)
					{
						tempVal = net_message.ReadShort();
					}
				}
			}
			break;

		case TE_LIGHTNING_HAMMER:
			{
				int				ent;
				h2stream_t		*stream;
				qhandle_t		models[2];
				entity_state_t	*state;

				ent = net_message.ReadShort();

				state = FindState(ent);

				if (state)
				{
					if(rand()&1)
					{
						S_StartSound(state->origin, TempSoundChannel(), 0, cl_sfx_lightning1, 1, 1);
					}
					else
					{
						S_StartSound(state->origin, TempSoundChannel(), 0, cl_sfx_lightning2, 1, 1);
					}

					for (i = 0; i < 5; i++)
					{	// make some lightning
						models[0] = R_RegisterModel("models/stlghtng.mdl");

						if((stream = NewStream(ent, i, NULL)) == NULL)
						{
							Con_Printf("stream list overflow\n");
							return;
						}
						//stream->type = TE_STREAM_ICECHUNKS;
						stream->type = TE_STREAM_LIGHTNING;
						stream->tag = i;
						stream->flags = i;
						stream->entity = ent;
						stream->skin = 0;
						stream->models[0] = models[0];
						stream->endTime = cl.serverTimeFloat+0.5;
						stream->lastTrailTime = 0;

						VectorCopy(state->origin, stream->source);
						stream->source[0] += rand()%30 - 15;
						stream->source[1] += rand()%30 - 15;
						VectorCopy(stream->source, stream->dest);
						stream->dest[0] += rand()%80 - 40;
						stream->dest[1] += rand()%80 - 40;
						stream->dest[2] += 64 + (rand()%48);
					}

				}
			}
			break;
		case TE_HWTELEPORT:
			pos[0] = net_message.ReadCoord ();
			pos[1] = net_message.ReadCoord ();
			pos[2] = net_message.ReadCoord ();
			cnt = net_message.ReadShort();  //skin#
			S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_teleport[rand() % 5], 1, 1);

			ex = CL_AllocExplosion ();
			VectorCopy(pos, ex->origin);
			ex->frameFunc = TeleportFlashThink;	
			ex->model = R_RegisterModel("models/teleport.mdl");
			ex->startTime = cl.serverTimeFloat;
			ex->endTime = ex->startTime + 2;
			ex->avel[2] = rand() * 360 + 360;
			ex->flags = SCALE_TYPE_XYONLY | DRF_TRANSLUCENT;
			ex->skin = cnt;
			ex->scale = 100;
			
			for(dir=0; dir<360; dir+=45)
			{
				cosval = 10 * cos(dir *M_PI*2 / 360);
				sinval = 10 * sin(dir *M_PI*2 / 360);
				ex = CL_AllocExplosion ();
				VectorCopy(pos, ex->origin);
				ex->model = R_RegisterModel("models/telesmk2.spr");
				ex->startTime = cl.serverTimeFloat;
				ex->endTime = ex->startTime + .5;
				ex->velocity[0] = cosval;
				ex->velocity[1] = sinval;
				ex->flags = DRF_TRANSLUCENT;

				ex = CL_AllocExplosion ();
				VectorCopy(pos, ex->origin);
				ex->origin[2] += 64;
				ex->model = R_RegisterModel("models/telesmk2.spr");
				ex->startTime = cl.serverTimeFloat;
				ex->endTime = ex->startTime + .5;
				ex->velocity[0] = cosval;
				ex->velocity[1] = sinval;
				ex->flags = DRF_TRANSLUCENT;
			}
			break;

		case TE_SWORD_EXPLOSION:
			{
				vec3_t			pos;
				int				ent;
				h2stream_t		*stream;
				qhandle_t		models[2];
				entity_state_t	*state;

				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();
				ent = net_message.ReadShort();

				state = FindState(ent);

				if (state)
				{
					if(rand()&1)
					{
						S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_lightning1, 1, 1);
					}
					else
					{
						S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_lightning2, 1, 1);
					}

					for (i = 0; i < 5; i++)
					{	// make some lightning
						models[0] = R_RegisterModel("models/stlghtng.mdl");

						if((stream = NewStream(ent, i, NULL)) == NULL)
						{
							Con_Printf("stream list overflow\n");
							return;
						}
						//stream->type = TE_STREAM_ICECHUNKS;
						stream->type = TE_STREAM_LIGHTNING;
						stream->tag = i;
						stream->flags = i;
						stream->entity = ent;
						stream->skin = 0;
						stream->models[0] = models[0];
						stream->endTime = cl.serverTimeFloat+0.5;
						stream->lastTrailTime = 0;

						VectorCopy(pos, stream->source);
						stream->source[0] += rand()%30 - 15;
						stream->source[1] += rand()%30 - 15;
						VectorCopy(stream->source, stream->dest);
						stream->dest[0] += rand()%80 - 40;
						stream->dest[1] += rand()%80 - 40;
						stream->dest[2] += 64 + (rand()%48);
					}
				}
				ex = CL_AllocExplosion();
				VectorCopy(pos, ex->origin);
				ex->model = R_RegisterModel("models/vorpshok.mdl");
				ex->startTime = cl.serverTimeFloat;
				ex->endTime = ex->startTime + 1.0;
				ex->flags |= MLS_ABSLIGHT;
				ex->abslight = 128;
				ex->skin = 0;
				ex->scale = 100;
				ex->frameFunc = SwordFrameFunc;

				S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_swordExplode, 1, 1);

			}
			break;

		case TE_AXE_BOUNCE:
			pos[0] = net_message.ReadCoord();
			pos[1] = net_message.ReadCoord();
			pos[2] = net_message.ReadCoord();

			ex = CL_AllocExplosion();
			VectorCopy(pos, ex->origin);
			ex->model = R_RegisterModel("models/spark.spr");
			ex->startTime = cl.serverTimeFloat;
			ex->flags |= MLS_ABSLIGHT;
			ex->abslight = 128;
			ex->skin = 0;
			ex->scale = 100;
			ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.05;

			S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_axeBounce, 1, 1);
			break;

		case TE_AXE_EXPLODE:

			pos[0] = net_message.ReadCoord();
			pos[1] = net_message.ReadCoord();
			pos[2] = net_message.ReadCoord();

			i = (host_frametime < .07) ? 0 : 3;	// based on framerate
			for( ; i < 5; i++)
			{
				ex = CL_AllocExplosion();
				VectorCopy(pos, ex->origin);
				ex->origin[0] += rand()%6 - 3;
				ex->origin[1] += rand()%6 - 3;
				ex->origin[2] += rand()%6 - 3;

				ex->velocity[0] = (ex->origin[0] - pos[0])*15;
				ex->velocity[1] = (ex->origin[1] - pos[1])*15;
				ex->velocity[2] = (ex->origin[2] - pos[2])*15;

				switch(rand()%6)
				{
				case 0:
				case 1:
					ex->model = R_RegisterModel("models/xpspblue.spr");
					break;
				case 2:
				case 3:
					ex->model = R_RegisterModel("models/xpspblue.spr");
					ex->flags |= MLS_ABSLIGHT|DRF_TRANSLUCENT;
					break;
				case 4:
				case 5:
					ex->model = R_RegisterModel("models/spark0.spr");
					ex->flags |= MLS_ABSLIGHT|DRF_TRANSLUCENT;
					break;
				}
				ex->flags |= MLS_ABSLIGHT;//|DRF_TRANSLUCENT;
				ex->abslight = 160+rand()%24;
				ex->skin = 0;
				ex->scale = 80 + rand()%40;
				ex->startTime = cl.serverTimeFloat + (rand()%50 / 200.0);
				ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.05;
			}


			S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_axeExplode, 1, 1);
			break;

		case TE_TIME_BOMB:
			pos[0] = net_message.ReadCoord();
			pos[1] = net_message.ReadCoord();
			pos[2] = net_message.ReadCoord();

			i = (host_frametime < .07) ? 0 : 14;	// based on framerate
			for(; i < 20; i++)
			{
				ex = CL_AllocExplosion();
				VectorCopy(pos, ex->origin);
				ex->origin[0] += rand()%6 - 3;
				ex->origin[1] += rand()%6 - 3;
				ex->origin[2] += rand()%6 - 3;

				ex->velocity[0] = (ex->origin[0] - pos[0])*40;
				ex->velocity[1] = (ex->origin[1] - pos[1])*40;
				ex->velocity[2] = (ex->origin[2] - pos[2])*40;

				switch(rand()%4)
				{
				case 0:
					ex->model = R_RegisterModel("models/sm_expld.spr");
					break;
				case 2:
					ex->model = R_RegisterModel("models/bg_expld.spr");
					break;
				case 1:
				case 3:
					ex->model = R_RegisterModel("models/gen_expl.spr");
					break;
				}
				ex->flags |= MLS_ABSLIGHT|DRF_TRANSLUCENT;
				ex->abslight = 160+rand()%24;
				ex->skin = 0;
				ex->scale = 80 + rand()%40;
				ex->startTime = cl.serverTimeFloat + (rand()%50 / 200.0);
				ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.05;
			}

			S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_axeExplode, 1, 1);
			break;

		case TE_FIREBALL:
			pos[0] = net_message.ReadCoord();
			pos[1] = net_message.ReadCoord();
			pos[2] = net_message.ReadCoord();

			ex = CL_AllocExplosion();
			VectorCopy(pos, ex->origin);
			ex->model = R_RegisterModel("models/blast.mdl");
			ex->flags |= MLS_ABSLIGHT|SCALE_TYPE_UNIFORM|SCALE_ORIGIN_CENTER;
			ex->abslight = 128;
			ex->skin = 0;
			ex->scale = 1;
			ex->startTime = cl.serverTimeFloat;
			ex->endTime = ex->startTime + 1.0;
			ex->frameFunc = fireBallUpdate;

			ex->exflags = EXFLAG_ROTATE;

			ex->avel[0] = 50;
			ex->avel[1] = 50;
			ex->avel[2] = 50;
		
			S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_fireBall, 1, 1);
			break;

		case TE_SUNSTAFF_POWER:
			{
				int				ent;
				h2stream_t		*stream;
				qhandle_t		models[4];
				entity_state_t	*state;

				ent = net_message.ReadShort();

				vel[0] = net_message.ReadCoord();
				vel[1] = net_message.ReadCoord();
				vel[2] = net_message.ReadCoord();
				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();

				for(i = 0; i < 2; i++)
				{
					ex = CL_AllocExplosion();
					VectorCopy(pos, ex->origin);
					if(i)
					{
						ex->model = R_RegisterModel("models/stsunsf3.mdl");
						ex->scale = 200;
						ex->frameFunc = sunPowerUpdate;
					}
					else
					{
						ex->model = R_RegisterModel("models/blast.mdl");
						ex->flags |= DRF_TRANSLUCENT;
						ex->frameFunc = sunBallUpdate;
						ex->scale = 120;
					}
					ex->flags |= MLS_ABSLIGHT|SCALE_TYPE_UNIFORM|SCALE_ORIGIN_CENTER;
					ex->abslight = 128;
					ex->skin = 0;
					ex->scale = 200;
					ex->startTime = cl.serverTimeFloat;
					ex->endTime = ex->startTime + .8;

					ex->exflags = EXFLAG_ROTATE;

					ex->avel[0] = 50;
					ex->avel[1] = 50;
					ex->avel[2] = 50;
				}



				S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_fireBall, 1, 1);

				state = FindState(ent);

				if (state)
				{
					S_StartSound(state->origin, TempSoundChannel(), 0, cl_sfx_sunstaff, 1, 1);

					models[0] = R_RegisterModel("models/stsunsf2.mdl");
					models[1] = R_RegisterModel("models/stsunsf1.mdl");
					models[2] = R_RegisterModel("models/stsunsf3.mdl");
					models[3] = R_RegisterModel("models/stsunsf4.mdl");

					if((stream = NewStream(ent, 0, NULL)) == NULL)
					{
						Con_Printf("stream list overflow\n");
						return;
					}
					stream->type = TE_STREAM_SUNSTAFF2;
					stream->tag = 0;
					//stream->flags = H2STREAM_ATTACHED;
					stream->flags = 0;
					stream->entity = ent;
					stream->skin = 0;
					stream->models[0] = models[0];
					stream->models[1] = models[1];
					stream->models[2] = models[2];
					stream->models[3] = models[3];
					stream->endTime = cl.serverTimeFloat+0.8;
					stream->lastTrailTime = 0;

					VectorCopy(vel, stream->source);
					VectorCopy(pos, stream->dest);

					//VectorCopy(vec3_origin, stream->offset);
					//VectorSubtract(stream->source, vel, stream->offset);

					// make some spiffy particles to glue it all together
				}
			}
			break;

		case TE_PURIFY2_EXPLODE:
			pos[0] = net_message.ReadCoord();
			pos[1] = net_message.ReadCoord();
			pos[2] = net_message.ReadCoord();

			ex = CL_AllocExplosion();
			VectorCopy(pos, ex->origin);
			ex->model = R_RegisterModel("models/xplod29.spr");
			ex->startTime = cl.serverTimeFloat;
			ex->flags |= MLS_ABSLIGHT;
			ex->abslight = 128;
			ex->skin = 0;
			ex->scale = 100;
			ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.05;

			i = (host_frametime < .07) ? 0 : 8;	// based on framerate
			for(i = 0; i < 12; i++)
			{
				ex = CL_AllocExplosion();
				VectorCopy(pos, ex->origin);
				ex->origin[0] += (rand()%20) - 10;
				ex->origin[1] += (rand()%20) - 10;
				ex->origin[2] += (rand()%20) - 10;

				ex->velocity[0] = (ex->origin[0] - pos[0])*12;
				ex->velocity[1] = (ex->origin[1] - pos[1])*12;
				ex->velocity[2] = (ex->origin[2] - pos[2])*12;

				switch(rand()%4)
				{
				case 0:
				case 1:
					ex->model = R_RegisterModel("models/sm_expld.spr");
					break;
				case 2:
					ex->model = R_RegisterModel("models/bg_expld.spr");
					break;
				case 3:
					ex->model = R_RegisterModel("models/gen_expl.spr");
					break;
				}
				if(host_frametime < 0.07)
				{
					ex->flags |= MLS_ABSLIGHT|DRF_TRANSLUCENT;
				}
				ex->abslight = 160 + rand()%64;
				ex->skin = 0;
				ex->scale = 80 + rand()%40;
				ex->startTime = cl.serverTimeFloat + (rand()%50 / 200.0);
				ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.04;
			}

			S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_purify2, 1, 1);
			 
			break;

		case TE_PLAYER_DEATH:
			{
				int				angle;	// from 0 to 256
				int				pitch;	// from 0 to 256
				int				force;
				int				style;
				float			throwPower, curAng, curPitch;

				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();

				angle = net_message.ReadByte();
				pitch = net_message.ReadByte();
				force = net_message.ReadByte();
				style = net_message.ReadByte();


				i = (host_frametime < 0.07) ? 0 : 8;
				for( ; i < 12; i++)
				{
					ex = CL_AllocExplosion();
					VectorCopy(pos, ex->origin);
					ex->origin[0] += (rand()%40)-20;
					ex->origin[1] += (rand()%40)-20;
					ex->origin[2] += rand()%40;
					ex->frameFunc = ChunkThink;

					throwPower = 3.5 + ((rand()%100)/100.0);
					curAng = angle*6.28/256.0 + ((rand()%100)/50.0) - 1.0;
					curPitch = pitch*6.28/256.0 + ((rand()%100)/100.0) - .5;

					ex->velocity[0] = force*throwPower * cos(curAng) * cos(curPitch);
					ex->velocity[1] = force*throwPower * sin(curAng) * cos(curPitch);
					ex->velocity[2] = force*throwPower * sin(curPitch);

					// are these in degrees or radians?
					ex->angles[0] = rand()%360;
					ex->angles[1] = rand()%360;
					ex->angles[2] = rand()%360;
					ex->exflags = EXFLAG_ROTATE;

					ex->avel[0] = rand()%850 - 425;
					ex->avel[1] = rand()%850 - 425;
					ex->avel[2] = rand()%850 - 425;

					ex->scale = 80 + rand()%40;
					ex->data = THINGTYPE_FLESH;

					switch(rand()%3)
					{
					case 0:
						ex->model = R_RegisterModel("models/flesh1.mdl");
						break;
					case 1:
						ex->model = R_RegisterModel("models/flesh2.mdl");
						break;
					case 2:
						ex->model = R_RegisterModel("models/flesh3.mdl");
						break;
					}
					
					ex->skin = 0;

					ex->startTime = cl.serverTimeFloat;
					ex->endTime = ex->startTime + 4.0;
				}

				switch(style)
				{
				case 0:
					S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_big_gib, 1, 1);
					break;
				case 1:
					if(rand()%2)
					{
						S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_gib1, 1, 1);
					}
					else
					{
						S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_gib2, 1, 1);
					}
					break;
				case 2:
					S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_telefrag, 1, 1);
					break;
				}
			}
			break;

		case TE_PURIFY1_EFFECT:
			{
				float	angle;
				float	pitch;
				float	dist;
				vec3_t	endPos;

				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();

				angle = net_message.ReadByte()*6.28/256.0;
				pitch = net_message.ReadByte()*6.28/256.0;
				dist = net_message.ReadShort();

				endPos[0] = pos[0] + dist * cos(angle) * cos(pitch);
				endPos[1] = pos[1] + dist * sin(angle) * cos(pitch);
				endPos[2] = pos[2] + dist * sin(pitch);

				//CLH2_TrailParticles (pos, endPos, rt_purify);
				CLH2_TrailParticles (pos, endPos, rt_purify);

				S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_purify1_fire, 1, 1);
				S_StartSound(endPos, TempSoundChannel(), 0, cl_sfx_purify1_hit, 1, 1);

				ex = CL_AllocExplosion();
				VectorCopy(endPos, ex->origin);
				ex->model = R_RegisterModel("models/fcircle.spr");
				ex->startTime = cl.serverTimeFloat;
				ex->flags |= MLS_ABSLIGHT;
				ex->abslight = 128;
				ex->skin = 0;
				ex->scale = 100;
				ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.05;
			}

			break;

		case TE_TELEPORT_LINGER:
			{
				float duration;
				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();

				duration = net_message.ReadCoord();

				ex = CL_AllocExplosion();
				VectorCopy(pos, ex->origin);
				ex->model = R_RegisterModel("models/bspark.spr");
				ex->startTime = cl.serverTimeFloat;
				ex->flags |= MLS_ABSLIGHT;
				ex->abslight = 128;
				ex->frameFunc = telEffectUpdate;
				ex->skin = 0;
				ex->scale = 0;
				ex->endTime = ex->startTime + duration;
			}

			break;

		case TE_LINE_EXPLOSION:
			{
				int distance;
				vec3_t	endPos,midPos,curPos,distVec;
				float ratio;

				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();
				endPos[0] = net_message.ReadCoord();
				endPos[1] = net_message.ReadCoord();
				endPos[2] = net_message.ReadCoord();

				VectorAdd(pos,endPos,midPos);
				VectorScale(midPos,0.5,midPos);

				VectorSubtract(midPos,pos,distVec);
				distance = (int)(VectorNormalize(distVec)*0.025);
				if (distance > 0)
				{
					VectorScale(distVec,40,distVec);

					VectorCopy(midPos,curPos);

					for (i=0;i<distance;i++)
					{
						ex = CL_AllocExplosion();
						VectorCopy(curPos, ex->origin);
						switch(rand()%3)
						{
						case 0:
							ex->model = R_RegisterModel("models/gen_expl.spr");
							break;
						case 1:
							ex->model = R_RegisterModel("models/bg_expld.spr");
							break;
						case 2:
							ex->model = R_RegisterModel("models/sm_expld.spr");
							break;
						}
						ex->flags |= MLS_ABSLIGHT;
						ex->abslight = 128;
						ex->skin = 0;
						ratio = (float)i/(float)distance;
						ex->scale = 200-(int)(150.0*ratio);
						ex->startTime = cl.serverTimeFloat+ratio*0.75;
						ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * (0.025+FRANDOM()*0.01);

						VectorAdd(curPos,distVec,curPos);
					}

					VectorScale(distVec,-1,distVec);
					VectorCopy(midPos,curPos);

					for (i=0;i<distance;i++)
					{
						ex = CL_AllocExplosion();
						VectorCopy(curPos, ex->origin);
						switch(rand()%3)
						{
						case 0:
							ex->model = R_RegisterModel("models/gen_expl.spr");
							break;
						case 1:
							ex->model = R_RegisterModel("models/bg_expld.spr");
							break;
						case 2:
							ex->model = R_RegisterModel("models/sm_expld.spr");
							break;
						}
						ex->flags |= MLS_ABSLIGHT;
						ex->abslight = 128;
						ex->skin = 0;
						ratio = (float)i/(float)distance;
						ex->scale = 200-(int)(150.0*ratio);
						ex->startTime = cl.serverTimeFloat+ratio*0.75;
						ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * (0.025+FRANDOM()*0.01);

						VectorAdd(curPos,distVec,curPos);
					}
				}
			}
			break;

		case TE_METEOR_CRUSH:
			{
				float	maxDist;

				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();
				maxDist = net_message.ReadLong();

				ex = CL_AllocExplosion();
				VectorCopy(pos, ex->origin);
				ex->model = R_RegisterModel("models/null.spr");
				ex->startTime = cl.serverTimeFloat;
				ex->endTime = ex->startTime + 0.4;
				ex->frameFunc = MeteorCrushSpawnThink;
				ex->data = maxDist;

				S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_axeExplode, 1, 1);
			}

			break;
			
		case TE_ACIDBALL:
			pos[0] = net_message.ReadCoord();
			pos[1] = net_message.ReadCoord();
			pos[2] = net_message.ReadCoord();

			i = (host_frametime < 0.07) ? 0 : 2;
			for( ; i < 5; i++)
			{
				ex = CL_AllocExplosion();
				VectorCopy(pos, ex->origin);
				ex->origin[0] += rand()%6 - 3;
				ex->origin[1] += rand()%6 - 3;
				ex->origin[2] += rand()%6 - 3;

				ex->velocity[0] = (ex->origin[0] - pos[0])*6;
				ex->velocity[1] = (ex->origin[1] - pos[1])*6;
				ex->velocity[2] = (ex->origin[2] - pos[2])*6;

				ex->model = R_RegisterModel("models/axplsn_2.spr");
				if(host_frametime < 0.07)
				{
					ex->flags |= MLS_ABSLIGHT|DRF_TRANSLUCENT;
				}
				ex->abslight = 160+rand()%24;
				ex->skin = 0;
				ex->scale = 80 + rand()%40;
				ex->startTime = cl.serverTimeFloat + (rand()%50 / 200.0);
				ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.05;
			}

			S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_acidhit, 1, 1);

			break;

		case TE_ACIDBLOB:
			pos[0] = net_message.ReadCoord();
			pos[1] = net_message.ReadCoord();
			pos[2] = net_message.ReadCoord();

			i = (host_frametime < 0.07) ? 0 : 7;
			for( ; i < 12; i++)
			{
				ex = CL_AllocExplosion();
				VectorCopy(pos, ex->origin);
				ex->origin[0] += rand()%6 - 3;
				ex->origin[1] += rand()%6 - 3;
				ex->origin[2] += rand()%6 - 3;

				ex->velocity[0] = (ex->origin[0] - pos[0])*25;
				ex->velocity[1] = (ex->origin[1] - pos[1])*25;
				ex->velocity[2] = (ex->origin[2] - pos[2])*25;

				switch(rand()%4)
				{
				case 0:
					ex->model = R_RegisterModel("models/axplsn_2.spr");
					break;
				case 1:
					ex->model = R_RegisterModel("models/axplsn_1.spr");
					break;
				case 2:
				case 3:
					ex->model = R_RegisterModel("models/axplsn_5.spr");
					break;
				}
				if(host_frametime < 0.07)
				{
					ex->flags |= MLS_ABSLIGHT|DRF_TRANSLUCENT;
				}
				ex->abslight = 1;
				ex->skin = 0;
				ex->scale = 80 + rand()%40;
				ex->startTime = cl.serverTimeFloat + (rand()%50 / 200.0);
				ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.05;
			}

			// always make 8 meteors
			i = (host_frametime < 0.07) ? 0 : 4;
			for( ; i < 8; i++)
			{
				ex = CL_AllocExplosion();
				VectorCopy(pos,ex->origin);
				ex->frameFunc = ChunkThink;

				// temp modify them...
				ex->velocity[0] = (rand()%500)-250;
				ex->velocity[1] = (rand()%500)-250;
				ex->velocity[2] = (rand()%200)+200;

				// are these in degrees or radians?
				ex->angles[0] = rand()%360;
				ex->angles[1] = rand()%360;
				ex->angles[2] = rand()%360;
				ex->exflags = EXFLAG_ROTATE;

				ex->avel[0] = rand()%850 - 425;
				ex->avel[1] = rand()%850 - 425;
				ex->avel[2] = rand()%850 - 425;

				ex->scale = 45 + rand()%10;
				ex->data = THINGTYPE_ACID;

				ex->model = R_RegisterModel("models/sucwp2p.mdl");
				ex->skin = 0;
				VectorScale(ex->avel, 4.0, ex->avel);

				ex->startTime = cl.serverTimeFloat;
				ex->endTime = ex->startTime + 4.0;
			}

			S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_acidhit, 1, 1);
			break;

		case TE_FIREWALL:
			{
				float travelAng, travelPitch;
				float fireCounts;
				vec3_t				endPos, curPos, posAdd;

				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();
				travelAng = net_message.ReadByte()*6.28/256.0;
				travelPitch = net_message.ReadByte()*6.28/256.0;
				fireCounts = net_message.ReadByte();

				CLH2_DimLight(0, pos);

				VectorCopy(pos, endPos);
				endPos[0] += cos(travelAng) * cos(travelPitch) * 450;
				endPos[1] += sin(travelAng) * cos(travelPitch) * 450;
				endPos[2] += sin(travelPitch) * 450;

				VectorCopy(pos, curPos);
				VectorSubtract(endPos, pos, posAdd);
				VectorScale(posAdd, .125, posAdd);

				for(i = 0; i < fireCounts; i++)
				{
					ex = CL_AllocExplosion();
					VectorCopy(curPos, ex->origin);
					switch(rand()%3)
					{
					case 0:
						ex->model = R_RegisterModel("models/firewal1.spr");
						break;
					case 1:
						ex->model = R_RegisterModel("models/firewal5.spr");
						break;
					case 2:
						ex->model = R_RegisterModel("models/firewal4.spr");
						break;
					}
					ex->startTime = cl.serverTimeFloat + .3/8.0 * i;
					ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.05;

					int PtContents;
					do
					{	// I dunno how expensive this is, but it kind of sucks anyway around it...
						PtContents = CM_PointContentsQ1(ex->origin, 0);

						if (PtContents == BSP29CONTENTS_EMPTY)
						{
							ex->origin[2] -= 16;
						}
						else
						{
							ex->origin[2] += 16;
						}
					}
					while (PtContents == BSP29CONTENTS_EMPTY);
					ex->origin[0] += (rand()%8)-4;
					ex->origin[1] += (rand()%8)-4;
					ex->origin[2] += (rand()%6)+21;

					
					ex = CL_AllocExplosion();
					VectorCopy(curPos, ex->origin);
					ex->origin[0] += (rand()%8)-4;
					ex->origin[1] += (rand()%8)-4;
					ex->origin[2] += (rand()%6)-3;
					ex->model = R_RegisterModel("models/flamestr.spr");
					ex->startTime = cl.serverTimeFloat + .3/8.0 * i;
					ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.05;
					ex->flags |= DRF_TRANSLUCENT;

					VectorAdd(curPos, posAdd, curPos);
				}
			}
			break;
			
		case TE_FIREWALL_IMPACT:
			// Add in the actual explosion
			pos[0] = net_message.ReadCoord();
			pos[1] = net_message.ReadCoord();
			pos[2] = net_message.ReadCoord();

			i = (host_frametime < 0.07) ? 0 : 8;
			for( ; i < 12; i++)
			{
				ex = CL_AllocExplosion();
				VectorCopy(pos, ex->origin);
				ex->origin[0] += (rand()%32)-16;
				ex->origin[1] += (rand()%32)-16;
				ex->origin[2] += (rand()%32)-16;
				ex->model = R_RegisterModel("models/fboom.spr");
				ex->startTime = cl.serverTimeFloat + ((rand()%150)/200);
				ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.05;
			}

			S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_flameend, 1, 1);

			break;
		case TE_HWBONERIC:
			pos[0] = net_message.ReadCoord();
			pos[1] = net_message.ReadCoord();
			pos[2] = net_message.ReadCoord();
			cnt = net_message.ReadByte ();
			CLH2_RunParticleEffect4 (pos, 3, 368 + rand() % 16, pt_h2grav, cnt);
			rnd = rand() % 100;
			if (rnd > 95)
				S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_ric1, 1, 1);
			else if (rnd > 91)
				S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_ric2, 1, 1);
			else if (rnd > 87)
				S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_ric3, 1, 1);

			break;

		case TE_POWERFLAME:
			{
				float travelAng, travelPitch;
				float fireCounts;
				vec3_t				endPos, curPos, posAdd;
				vec3_t				angles, forward, right, up;
				float				cVal, sVal;
				float				svTime;

				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();
				travelAng = net_message.ReadByte()*6.28/256.0;
				travelPitch = net_message.ReadByte()*6.28/256.0;
				fireCounts = net_message.ReadByte();
				svTime = net_message.ReadLong();

				angles[0] = travelPitch*360/(2*M_PI);
				angles[1] = travelAng*360/(2*M_PI);
				angles[2] = 0;

				AngleVectors(angles, forward, right, up);

				CLH2_DimLight(0, pos);

				VectorCopy(pos, endPos);
				endPos[0] += cos(travelAng) * cos(travelPitch) * 375;
				endPos[1] += sin(travelAng) * cos(travelPitch) * 375;
				endPos[2] += sin(travelPitch) * 375;

				VectorCopy(pos, curPos);
				VectorSubtract(endPos, pos, posAdd);
				VectorScale(posAdd, .125, posAdd);

				for(i = 0; i < fireCounts; i++)
				{
					cVal = cos((svTime + (i*.3/8.0))*8)*10;
					sVal = sin((svTime + (i*.3/8.0))*8)*10;
//					cVal = cos((svTime + (i*.3/8.0))*16)*35;
//					sVal = sin((svTime + (i*.3/8.0))*16)*35;



					ex = CL_AllocExplosion();
					VectorCopy(curPos, ex->origin);
					VectorMA(ex->origin, cVal, right, ex->origin);
					VectorMA(ex->origin, sVal, up, ex->origin);
					ex->origin[0] += (rand()%8)-4;
					ex->origin[1] += (rand()%8)-4;
					ex->origin[2] += (rand()%6)-3;
					ex->model = R_RegisterModel("models/flamestr.spr");
					ex->startTime = cl.serverTimeFloat + .3/8.0 * i;
					ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.05;
					ex->flags |= DRF_TRANSLUCENT;

					ex->velocity[0] = 0;
					ex->velocity[1] = 0;
					ex->velocity[2] = 0;
					VectorMA(ex->velocity, cVal * 4.0, right, ex->velocity);
					VectorMA(ex->velocity, sVal * 4.0, up, ex->velocity);

					ex = CL_AllocExplosion();
					VectorCopy(curPos, ex->origin);
					VectorMA(ex->origin, -cVal, right, ex->origin);
					VectorMA(ex->origin, -sVal, up, ex->origin);
					ex->origin[0] += (rand()%8)-4;
					ex->origin[1] += (rand()%8)-4;
					ex->origin[2] += (rand()%6)-3;
					ex->model = R_RegisterModel("models/flamestr.spr");
					ex->startTime = cl.serverTimeFloat + .3/8.0 * i;
					ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.05;
					ex->flags |= DRF_TRANSLUCENT;

					ex->velocity[0] = 0;
					ex->velocity[1] = 0;
					ex->velocity[2] = 0;
					VectorMA(ex->velocity, -cVal * 4.0, right, ex->velocity);
					VectorMA(ex->velocity, -sVal * 4.0, up, ex->velocity);

					VectorAdd(curPos, posAdd, curPos);
				}
			}

			break;

		case TE_BLOODRAIN:
			{
				float travelAng, travelPitch;
				float trailLen;
				vec3_t				vel;
				unsigned char		health;
				vec3_t				angles, forward, right, up;

				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();
				travelAng = net_message.ReadByte()*6.28/256.0;
				travelPitch = net_message.ReadByte()*6.28/256.0;
				trailLen = net_message.ReadByte();
				health = net_message.ReadByte();

				vel[0] = cos(travelAng) * cos(travelPitch) * 800;
				vel[1] = sin(travelAng) * cos(travelPitch) * 800;
				vel[2] = sin(travelPitch) * 800;
				
				ex = CL_AllocExplosion();
				VectorCopy(pos, ex->origin);
				ex->model = R_RegisterModel("models/sucwp1p.mdl");
				ex->startTime = cl.serverTimeFloat;
				ex->endTime = ex->startTime + trailLen*.3/240.0;
				ex->angles[0] = travelPitch*360/6.28;
				ex->angles[1] = travelAng*360/6.28;
				ex->scale = health;
				VectorCopy(vel, ex->velocity);
				ex->frameFunc = updateBloodRain;
				ex->exflags |= EXFLAG_COLLIDE;

				if(health > 90)
				{
					angles[0] = travelPitch*360/(2*M_PI);
					angles[1] = travelAng*360/(2*M_PI);
					angles[2] = 0;

					AngleVectors(angles, forward, right, up);

					ex = CL_AllocExplosion();
					VectorCopy(pos, ex->origin);
					VectorMA(ex->origin, 7, right, ex->origin);
					ex->model = R_RegisterModel("models/sucwp1p.mdl");
					ex->startTime = cl.serverTimeFloat;
					ex->endTime = ex->startTime + trailLen*.3/240.0;
					ex->angles[0] = travelPitch*360/6.28;
					ex->angles[1] = travelAng*360/6.28;
					ex->scale = health - 90;
					VectorCopy(vel, ex->velocity);
					ex->frameFunc = updateBloodRain;
					//ex->exflags & EXFLAG_COLLIDE;

					ex = CL_AllocExplosion();
					VectorCopy(pos, ex->origin);
					VectorMA(ex->origin, -7, right, ex->origin);
					ex->model = R_RegisterModel("models/sucwp1p.mdl");
					ex->startTime = cl.serverTimeFloat;
					ex->endTime = ex->startTime + trailLen*.3/240.0;
					ex->angles[0] = travelPitch*360/6.28;
					ex->angles[1] = travelAng*360/6.28;
					ex->scale = health - 90;
					VectorCopy(vel, ex->velocity);
					ex->frameFunc = updateBloodRain;
					ex->exflags |= EXFLAG_COLLIDE;

				}
			}

			break;

		case TE_AXE:
			{
				float travelAng, travelPitch;
				float trailLen;
				vec3_t				vel;

				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();
				travelAng = net_message.ReadByte()*6.28/256.0;
				travelPitch = net_message.ReadByte()*6.28/256.0;
				trailLen = net_message.ReadByte() * .01;

				vel[0] = cos(travelAng) * cos(travelPitch) * 1100;
				vel[1] = sin(travelAng) * cos(travelPitch) * 1100;
				vel[2] = sin(travelPitch) * 1100;
				
				ex = CL_AllocExplosion();
				VectorCopy(pos, ex->origin);
				ex->model = R_RegisterModel("models/axblade.mdl");
				ex->startTime = cl.serverTimeFloat;
				ex->endTime = ex->startTime + trailLen*.3;
				ex->angles[0] = travelPitch*360/6.28;
				ex->angles[1] = travelAng*360/6.28;
				VectorCopy(vel, ex->velocity);
				ex->exflags |= EXFLAG_COLLIDE;

				ex = CL_AllocExplosion();
				VectorCopy(pos, ex->origin);
				ex->model = R_RegisterModel("models/axtail.mdl");
				ex->startTime = cl.serverTimeFloat;
				ex->endTime = ex->startTime + trailLen*.3;
				ex->angles[0] = travelPitch*360/6.28;
				ex->angles[1] = travelAng*360/6.28;
				VectorCopy(vel, ex->velocity);
				ex->exflags |= EXFLAG_COLLIDE;
				ex->exflags |= EXFLAG_STILL_FRAME;
				ex->data = 0;
				ex->flags |= MLS_ABSLIGHT|DRF_TRANSLUCENT;
				ex->abslight = 128;
				ex->skin = 0;
			}
			break;

		case TE_PURIFY2_MISSILE:
			{
				float travelAng, travelPitch;
				float trailLen;
				vec3_t				vel;

				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();
				travelAng = net_message.ReadByte()*6.28/256.0;
				travelPitch = net_message.ReadByte()*6.28/256.0;
				trailLen = net_message.ReadByte() * .01;

				vel[0] = cos(travelAng) * cos(travelPitch) * 1000;
				vel[1] = sin(travelAng) * cos(travelPitch) * 1000;
				vel[2] = sin(travelPitch) * 1000;
				
				ex = CL_AllocExplosion();
				VectorCopy(pos, ex->origin);
				ex->model = R_RegisterModel("models/drgnball.mdl");
				ex->startTime = cl.serverTimeFloat;
				ex->endTime = ex->startTime + trailLen*.3;
				ex->angles[0] = travelPitch*360/6.28;
				ex->angles[1] = travelAng*360/6.28;
				VectorCopy(vel, ex->velocity);
				ex->exflags |= EXFLAG_COLLIDE;
				ex->frameFunc = updatePurify2;
				ex->scale = 150;
			}
			break;

		case TE_SWORD_SHOT:
			{
				float travelAng, travelPitch;
				float trailLen;
				vec3_t				vel;

				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();
				travelAng = net_message.ReadByte()*6.28/256.0;
				travelPitch = net_message.ReadByte()*6.28/256.0;
				trailLen = net_message.ReadByte() * .01;

				vel[0] = cos(travelAng) * cos(travelPitch) * 1200;
				vel[1] = sin(travelAng) * cos(travelPitch) * 1200;
				vel[2] = sin(travelPitch) * 1200;
				
				ex = CL_AllocExplosion();
				VectorCopy(pos, ex->origin);
				ex->model = R_RegisterModel("models/vorpshot.mdl");
				ex->startTime = cl.serverTimeFloat;
				ex->endTime = ex->startTime + trailLen*.3;
				ex->angles[0] = travelPitch*360/6.28;
				ex->angles[1] = travelAng*360/6.28;
				VectorCopy(vel, ex->velocity);
				ex->exflags |= EXFLAG_COLLIDE;
				ex->frameFunc = updateSwordShot;
				ex->scale = 100;
				ex->exflags |= EXFLAG_STILL_FRAME;
				ex->data = 16 + ((int)(cl.serverTimeFloat * 20.0)%13);
			}
			break;

		case TE_ICESHOT:
			{
				float travelAng, travelPitch;
				float trailLen;
				vec3_t				vel;
				explosion_t			*ex2;

				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();
				travelAng = net_message.ReadByte()*6.28/256.0;
				travelPitch = net_message.ReadByte()*6.28/256.0;
				trailLen = net_message.ReadByte() * .01;

				vel[0] = cos(travelAng) * cos(travelPitch) * 1200;
				vel[1] = sin(travelAng) * cos(travelPitch) * 1200;
				vel[2] = sin(travelPitch) * 1200;
				
				ex = CL_AllocExplosion();
				VectorCopy(pos, ex->origin);
				ex->model = R_RegisterModel("models/iceshot1.mdl");
				ex->startTime = cl.serverTimeFloat;
				ex->endTime = ex->startTime + trailLen*.3;
				ex->angles[0] = travelPitch*360/6.28;
				ex->angles[1] = travelAng*360/6.28;
				VectorCopy(vel, ex->velocity);
				ex->exflags |= EXFLAG_COLLIDE;
				ex->frameFunc = updateIceShot;
				ex->scale = 100;

				ex->exflags = EXFLAG_ROTATE;

				ex->avel[0] = 425;
				ex->avel[1] = 425;
				ex->avel[2] = 425;


				ex2 = ex;

				ex = CL_AllocExplosion();
				VectorCopy(pos, ex->origin);
				ex->model = R_RegisterModel("models/iceshot2.mdl");
				ex->startTime = cl.serverTimeFloat;
				ex->endTime = ex->startTime + trailLen*.3;
				ex->angles[0] = travelPitch*360/6.28;
				ex->angles[1] = travelAng*360/6.28;
				VectorCopy(vel, ex->velocity);
				ex->exflags |= EXFLAG_COLLIDE;
				//ex->frameFunc = updateSwordShot;
				ex->scale = 200;
				ex->flags |= MLS_ABSLIGHT|DRF_TRANSLUCENT;
				ex->abslight = 128;
				ex->exflags = EXFLAG_ROTATE;

				VectorCopy(ex2->avel, ex->avel);
			}

			break;

		case TE_METEOR:
			{
				float travelAng, travelPitch;
				float trailLen;
				vec3_t				vel;

				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();
				travelAng = net_message.ReadByte()*6.28/256.0;
				travelPitch = net_message.ReadByte()*6.28/256.0;
				trailLen = net_message.ReadByte() * .01;

				vel[0] = cos(travelAng) * cos(travelPitch) * 1000;
				vel[1] = sin(travelAng) * cos(travelPitch) * 1000;
				vel[2] = sin(travelPitch) * 1000;
				
				ex = CL_AllocExplosion();
				VectorCopy(pos, ex->origin);
				ex->model = R_RegisterModel("models/tempmetr.mdl");
				ex->startTime = cl.serverTimeFloat;
				ex->endTime = ex->startTime + trailLen*.3;
				ex->angles[0] = travelPitch*360/6.28;
				ex->angles[1] = travelAng*360/6.28;
				VectorCopy(vel, ex->velocity);
				ex->exflags |= EXFLAG_COLLIDE;
				ex->frameFunc = updateMeteor;
				ex->scale = 100;

				ex->exflags = EXFLAG_ROTATE;

				ex->avel[0] = 200;
				ex->avel[1] = 200;
				ex->avel[2] = 200;
			}
			break;

		case TE_LIGHTNINGBALL:
			{
				float travelAng, travelPitch;
				float trailLen;
				vec3_t				vel;
				float				speed;

				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();
				travelAng = net_message.ReadByte()*6.28/256.0;
				travelPitch = net_message.ReadByte()*6.28/256.0;
				speed = net_message.ReadShort();
				trailLen = net_message.ReadByte() * .01;

				// light
				CLH2_ExplosionLight(pos);

				vel[0] = cos(travelAng) * cos(travelPitch) * speed;
				vel[1] = sin(travelAng) * cos(travelPitch) * speed;
				vel[2] = sin(travelPitch) * speed;
				
				ex = CL_AllocExplosion();
				VectorCopy(pos, ex->origin);
				ex->model = R_RegisterModel("models/lball.mdl");
				ex->startTime = cl.serverTimeFloat;
				ex->endTime = ex->startTime + trailLen*.2;
				ex->angles[0] = travelPitch*360/6.28;
				ex->angles[1] = travelAng*360/6.28;
				VectorCopy(vel, ex->velocity);
				ex->exflags |= EXFLAG_COLLIDE;
				//ex->frameFunc = updateMeteor;
				//ex->scale = 230;

				//ex->exflags = EXFLAG_ROTATE;

				//ex->avel[0] = 200;
				//ex->avel[1] = 200;
				//ex->avel[2] = 200;
			}
			break;

		case TE_MEGAMETEOR:
			{
				float	travelAng, travelPitch;
				float	trailLen;
				vec3_t	vel;

				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();
				travelAng = net_message.ReadByte()*6.28/256.0;
				travelPitch = net_message.ReadByte()*6.28/256.0;
				trailLen = net_message.ReadByte() * .01;

				vel[0] = cos(travelAng) * cos(travelPitch) * 1600;
				vel[1] = sin(travelAng) * cos(travelPitch) * 1600;
				vel[2] = sin(travelPitch) * 1600;
				
				ex = CL_AllocExplosion();
				VectorCopy(pos, ex->origin);
				ex->model = R_RegisterModel("models/tempmetr.mdl");
				ex->startTime = cl.serverTimeFloat;
				ex->endTime = ex->startTime + trailLen*.3;
				ex->angles[0] = travelPitch*360/6.28;
				ex->angles[1] = travelAng*360/6.28;
				VectorCopy(vel, ex->velocity);
				ex->exflags |= EXFLAG_COLLIDE;
				ex->frameFunc = updateMeteor;
				ex->scale = 230;
				ex->flags |= DRF_TRANSLUCENT;

				ex->exflags = EXFLAG_ROTATE;

				ex->avel[0] = 200;
				ex->avel[1] = 200;
				ex->avel[2] = 200;
			}
			break;

		case TE_CUBEBEAM:
			{
				int				type;
				int				ent;
				int				ent2;
				int				tag;
				int				flags;
				int				skin;
				vec3_t			source;
				vec3_t			dest;
				vec3_t			smokeDir;
				float			duration;
				entity_state_t	*state;
				entity_state_t	*state2;

				type = TE_STREAM_COLORBEAM;

				ent = net_message.ReadShort();
				ent2 = net_message.ReadShort();
				flags = 0;
				tag = 0;
				duration = 0.1;
				skin = rand()%5;
//				source[0] = net_message.ReadCoord();
//				source[1] = net_message.ReadCoord();
//				source[2] = net_message.ReadCoord();
//				dest[0] = net_message.ReadCoord();
//				dest[1] = net_message.ReadCoord();
//				dest[2] = net_message.ReadCoord();

				state = FindState(ent);
				state2 = FindState(ent2);

				if (state || state2)
				{
					if (state)
					{
						VectorCopy(state->origin, source);
					}
					else//don't know where the damn cube is--prolly won't see beam anyway then, so put it all at the target
					{
						VectorCopy(state2->origin, source);
						source[2]+=10;
					}

					if (state2)
					{
						VectorCopy(state2->origin, dest);//in case they're both valid, copy me again
						dest[2]+=30;
					}
					else//don't know where the damn victim is--prolly won't see beam anyway then, so put it all at the cube
					{
						VectorCopy(source, dest);
						dest[2]+=10;
					}

					VectorSet(smokeDir,0,0,100);
					S_StartSound(source, TempSoundChannel(), 0, cl_sfx_sunstaff, 1, 1);
					S_StartSound(dest, TempSoundChannel(), 0, cl_sfx_sunhit, 1, 1);
					CLH2_SunStaffTrail(dest, source);
					CreateStream(TE_STREAM_COLORBEAM, ent, flags, tag, duration, skin, source, dest);
				}
			}
			break;

		case TE_LIGHTNINGEXPLODE:
			{
				int				ent;
				h2stream_t		*stream;
				qhandle_t		models[2];
				entity_state_t	*state;
				float			tempAng, tempPitch;

				ent = net_message.ReadShort();

				state = FindState(ent);
				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();

				if(rand()&1)
				{
					S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_lightning1, 1, 1);
				}
				else
				{
					S_StartSound(pos, TempSoundChannel(), 0, cl_sfx_lightning2, 1, 1);
				}

				for (i = 0; i < 10; i++)
				{	// make some lightning
					models[0] = R_RegisterModel("models/stlghtng.mdl");

					if((stream = NewStream(ent, i, NULL)) == NULL)
					{
						Con_Printf("stream list overflow\n");
						return;
					}
					stream->type = TE_STREAM_LIGHTNING;
					stream->tag = i;
					stream->flags = i;
					stream->entity = ent;
					stream->skin = 0;
					stream->models[0] = models[0];
					stream->endTime = cl.serverTimeFloat+0.5;
					stream->lastTrailTime = 0;

					tempAng = (rand()%628)/100.0;
					tempPitch = (rand()%628)/100.0;

					VectorCopy(pos, stream->source);
					VectorCopy(stream->source, stream->dest);
					stream->dest[0] += 75.0 * cos(tempAng) * cos(tempPitch);
					stream->dest[1] += 75.0 * sin(tempAng) * cos(tempPitch);
					stream->dest[2] += 75.0 * sin(tempPitch);
				}
			}
			break;

		case TE_ACID_BALL_FLY:
			{
				float travelAng, travelPitch;
				float trailLen;
				vec3_t				vel;

				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();
				travelAng = net_message.ReadByte()*6.28/256.0;
				travelPitch = net_message.ReadByte()*6.28/256.0;
				trailLen = net_message.ReadByte() * .01;

				vel[0] = cos(travelAng) * cos(travelPitch) * 850;
				vel[1] = sin(travelAng) * cos(travelPitch) * 850;
				vel[2] = sin(travelPitch) * 850;
				
				ex = CL_AllocExplosion();
				VectorCopy(pos, ex->origin);
				ex->model = R_RegisterModel("models/sucwp2p.mdl");
				ex->startTime = cl.serverTimeFloat;
				ex->endTime = ex->startTime + trailLen*.3;
				ex->angles[0] = travelPitch*360/6.28;
				ex->angles[1] = travelAng*360/6.28;
				VectorCopy(vel, ex->velocity);
				ex->exflags |= EXFLAG_COLLIDE;
				ex->frameFunc = updateAcidBall;
			}
			break;

		case TE_ACID_BLOB_FLY:
			{
				float travelAng, travelPitch;
				float trailLen;
				vec3_t				vel;

				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();
				travelAng = net_message.ReadByte()*6.28/256.0;
				travelPitch = net_message.ReadByte()*6.28/256.0;
				trailLen = net_message.ReadByte() * .01;

				vel[0] = cos(travelAng) * cos(travelPitch) * 1000;
				vel[1] = sin(travelAng) * cos(travelPitch) * 1000;
				vel[2] = sin(travelPitch) * 1000;
				
				ex = CL_AllocExplosion();
				VectorCopy(pos, ex->origin);
				ex->model = R_RegisterModel("models/sucwp2p.mdl");
				ex->startTime = cl.serverTimeFloat;
				ex->endTime = ex->startTime + trailLen*.3;
				ex->angles[0] = travelPitch*360/6.28;
				ex->angles[1] = travelAng*360/6.28;
				VectorCopy(vel, ex->velocity);
				ex->exflags |= EXFLAG_COLLIDE;
				ex->frameFunc = updateAcidBlob;
				ex->scale = 230;

				//ex->exflags = EXFLAG_ROTATE;

				//ex->avel[0] = 200;
				//ex->avel[1] = 200;
				//ex->avel[2] = 200;
			}
			break;

		case TE_CHAINLIGHTNING:
			{
				vec3_t			points[12];
				int				numTargs = 0;
				int				oldNum;
				int				temp;

				int				ent;
				h2stream_t		*stream;
				qhandle_t		models[2];

				ent = net_message.ReadShort();

				do
				{
					points[numTargs][0] = net_message.ReadCoord();
					points[numTargs][1] = net_message.ReadCoord();
					points[numTargs][2] = net_message.ReadCoord();

					oldNum = numTargs;

					if(points[numTargs][0]||points[numTargs][1]||points[numTargs][2])
					{
						if(numTargs < 9)
						{
							numTargs++;
						}
					}
				}while(points[oldNum][0]||points[oldNum][1]||points[oldNum][2]);

				if(numTargs == 0)
				{
					break;
				}

				for(temp = 0; temp < numTargs - 1; temp++)
				{
					// make the connecting lightning...
					models[0] = R_RegisterModel("models/stlghtng.mdl");

					if((stream = NewStream(ent, temp, NULL)) == NULL)
					{
						Con_Printf("stream list overflow\n");
						return;
					}
					stream->type = TE_STREAM_LIGHTNING;
					stream->tag = temp;
					stream->flags = temp;
					stream->entity = ent;
					stream->skin = 0;
					stream->models[0] = models[0];
					stream->endTime = cl.serverTimeFloat+0.3;
					stream->lastTrailTime = 0;

					VectorCopy(points[temp], stream->source);
					VectorCopy(points[temp + 1], stream->dest);

					ex = CL_AllocExplosion();
					VectorCopy(points[temp+1], ex->origin);
					ex->model = R_RegisterModel("models/vorpshok.mdl");
					ex->startTime = cl.serverTimeFloat;
					ex->endTime = ex->startTime + .3;
					ex->flags |= MLS_ABSLIGHT;//|DRF_TRANSLUCENT;
					//ex->abslight = 128;
					ex->abslight = 224;
					ex->skin = rand()&1;
					ex->scale = 150;
					ex->frameFunc = zapFrameFunc;
					//ex->frameFunc = SwordFrameFunc;
					//ex->flags = DRF_TRANSLUCENT;// | MLS_ABSLIGHT;
					//ex->angles[0] = travelPitch*360/6.28;
					//ex->angles[1] = travelAng*360/6.28;

				}

			}
			break;


		default:
			Sys_Error ("CL_ParseTEnt: bad type");
	}
}

/*
=================
CL_UpdateExplosions
=================
*/
void CL_UpdateExplosions (void)
{
	int			i;
	int			f;
	explosion_t	*ex;

	for (i=0, ex=cl_explosions ; i< MAX_EXPLOSIONS ; i++, ex++)
	{
		if (!ex->model)
			continue;

		if(ex->exflags & EXFLAG_COLLIDE)
		{
			if (CM_PointContentsQ1(ex->origin, 0) != BSP29CONTENTS_EMPTY)
			{
				if (ex->removeFunc)
				{
					ex->removeFunc(ex);
				}
				ex->model = 0;
				continue;
			}
		}


		// if we hit endTime, get rid of explosion (i assume endTime is greater than startTime, etc)
		if (ex->endTime <= cl.serverTimeFloat)
		{
			if (ex->removeFunc)
			{
				ex->removeFunc(ex);
			}
			ex->model = 0;
			continue;
		}

		VectorCopy(ex->origin, ex->oldorg);

		// set the current frame so i finish anim at endTime
		if(ex->exflags & EXFLAG_STILL_FRAME)
		{	// if it's a still frame, use the data field
			f = (int)ex->data;
		}
		else
		{
			f = (R_ModelNumFrames(ex->model) - 1) * (cl.serverTimeFloat - ex->startTime) / (ex->endTime - ex->startTime);
		}

		// apply velocity
		ex->origin[0] += host_frametime * ex->velocity[0];
		ex->origin[1] += host_frametime * ex->velocity[1];
		ex->origin[2] += host_frametime * ex->velocity[2];

		// apply acceleration
		ex->velocity[0] += host_frametime * ex->accel[0];
		ex->velocity[1] += host_frametime * ex->accel[1];
		ex->velocity[2] += host_frametime * ex->accel[2];

		// add in angular velocity
		if(ex->exflags & EXFLAG_ROTATE)
		{
			VectorMA(ex->angles, host_frametime, ex->avel, ex->angles);
		}
		// you can set startTime to some point in the future to delay the explosion showing up or thinking; it'll still move, though
		if (ex->startTime > cl.serverTimeFloat)
			continue;

		if (ex->frameFunc)
		{
			ex->frameFunc(ex);
		}

		// allow for the possibility for the frame func to reset startTime
		if (ex->startTime > cl.serverTimeFloat)
			continue;

		// just incase the frameFunc eliminates the thingy here.
		if (ex->model == 0)
			continue;

		refEntity_t	ent;
		Com_Memset(&ent, 0, sizeof(ent));
		ent.reType = RT_MODEL;
		VectorCopy(ex->origin, ent.origin);
		ent.hModel = ex->model;
		ent.frame = f;
		ent.skinNum = ex->skin;
		CL_SetRefEntAxis(&ent, ex->angles, vec3_origin, ex->scale, 0, ex->abslight, ex->flags);
		R_HandleCustomSkin(&ent, -1);
		R_AddRefEntityToScene(&ent);
	}
}

void CL_UpdateStreams(void)
{
	int				i;
	h2stream_t		*stream;
	vec3_t			dist;
	vec3_t			org;
	vec3_t			discard, right, up;
	float			cosTime, sinTime, lifeTime, cos2Time, sin2Time;
	float			d;
	float			yaw, pitch;
	int				segmentCount;
	int				offset;
	entity_state_t	*state;

	// Update streams
	for(i = 0, stream = clh2_Streams; i < MAX_STREAMS_H2; i++, stream++)
	{
		if(!stream->models[0])// || stream->endTime < cl.time)
		{ // Inactive
			continue;
		}

		if(stream->endTime < cl.serverTimeFloat)
		{ // Inactive
			if(stream->type!=TE_STREAM_LIGHTNING&&stream->type!=TE_STREAM_LIGHTNING_SMALL)
				continue;
			else if(stream->endTime + 0.25 < cl.serverTimeFloat)
				continue;
		}

		if(stream->flags&H2STREAM_ATTACHED&&stream->endTime >= cl.serverTimeFloat)
			{ // Attach the start position to owner
			state = FindState(stream->entity);
			if (state)
			{
				VectorAdd(state->origin, stream->offset, stream->source);
			}
		}

		VectorSubtract(stream->dest, stream->source, dist);
		vec3_t angles;
		if(dist[1] == 0 && dist[0] == 0)
		{
			yaw = 0;
			if(dist[2] > 0)
			{
				pitch = 90;
			}
			else
			{
				pitch = 270;
			}
		}
		else
		{
			VecToAnglesCommon(dist, angles, yaw, pitch);
			if(pitch < 0)
			{
				pitch += 360;
			}
		}
		angles[0] = pitch;
		angles[1] = yaw;

		VectorCopy(stream->source, org);
		d = VectorNormalize(dist);

		if(stream->type == TE_STREAM_SUNSTAFF2)
		{
			discard[YAW] = yaw;
			discard[PITCH] = pitch;
			discard[ROLL] = 0;
			AngleVectors(discard, discard, right, up);

			lifeTime = ((stream->endTime - cl.serverTimeFloat)/.8);
			cosTime = cos(cl.serverTimeFloat*5);
			sinTime = sin(cl.serverTimeFloat*5);
			cos2Time = cos(cl.serverTimeFloat*5 + 3.14);
			sin2Time = sin(cl.serverTimeFloat*5 + 3.14);
		}

		segmentCount = 0;
		if(stream->type == TE_STREAM_ICECHUNKS)
		{
			offset = (int)(cl.serverTimeFloat*40)%30;
			for(i = 0; i < 3; i++)
			{
				org[i] += dist[i]*offset;
			}
		}
		while(d > 0)
		{
			refEntity_t ent;
			Com_Memset(&ent, 0, sizeof(ent));
			ent.reType = RT_MODEL;
			VectorCopy(org, ent.origin);
			ent.hModel = stream->models[0];
			switch (stream->type)
			{
			case TE_STREAM_CHAIN:
				angles[2] = 0;
				CL_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_SUNSTAFF1:
				angles[2] = (int)(cl.serverTimeFloat*10)%360;
				//ent->frame = (int)(cl.time*20)%20;
				CL_SetRefEntAxis(&ent, angles, vec3_origin, 50 + 100 * ((stream->endTime - cl.serverTimeFloat)/.3), 0, 128, MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);

				Com_Memset(&ent, 0, sizeof(ent));
				ent.reType = RT_MODEL;
				VectorCopy(org, ent.origin);
				ent.hModel = stream->models[1];
				angles[2] = (int)(cl.serverTimeFloat*50)%360;
				//stream->endTime = cl.time+0.3;	// FIXME
				CL_SetRefEntAxis(&ent, angles, vec3_origin, 50 + 100 * ((stream->endTime - cl.serverTimeFloat)/.5), 0, 128, MLS_ABSLIGHT|DRF_TRANSLUCENT);
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_SUNSTAFF2:
				angles[2] = (int)(cl.serverTimeFloat*100)%360;
				VectorMA(ent.origin, cosTime * (40 * lifeTime), right,  ent.origin);
				VectorMA(ent.origin, sinTime * (40 * lifeTime), up,  ent.origin);
				CL_SetRefEntAxis(&ent, angles, vec3_origin, 100 + 150 * lifeTime, 0, 128, MLS_ABSLIGHT|DRF_TRANSLUCENT);
				R_AddRefEntityToScene(&ent);

				Com_Memset(&ent, 0, sizeof(ent));
				ent.reType = RT_MODEL;
				VectorCopy(org, ent.origin);
				ent.hModel = stream->models[0];
				angles[2] = (int)(cl.serverTimeFloat*100)%360;
				VectorMA(ent.origin, cos2Time * (40 * lifeTime), right,  ent.origin);
				VectorMA(ent.origin, sin2Time * (40 * lifeTime), up,  ent.origin);
				CL_SetRefEntAxis(&ent, angles, vec3_origin, 100 + 150 * lifeTime, 0, 128, MLS_ABSLIGHT|DRF_TRANSLUCENT);
				R_AddRefEntityToScene(&ent);

				for (int ix = 0; ix < 2; ix++)
				{
					Com_Memset(&ent, 0, sizeof(ent));
					ent.reType = RT_MODEL;
					VectorCopy(org, ent.origin);
					if (i)
					{
						VectorMA(ent.origin, cos2Time * (40 * lifeTime), right,  ent.origin);
						VectorMA(ent.origin, sin2Time * (40 * lifeTime), up,  ent.origin);
					}
					else
					{
						VectorMA(ent.origin, cosTime * (40 * lifeTime), right,  ent.origin);
						VectorMA(ent.origin, sinTime * (40 * lifeTime), up,  ent.origin);
					}
					ent.hModel = stream->models[1];
					angles[2] = (int)(cl.serverTimeFloat*20)%360;
					CL_SetRefEntAxis(&ent, angles, vec3_origin, 100 + 150 * lifeTime, 0, 128, MLS_ABSLIGHT);
					R_AddRefEntityToScene(&ent);
				}
				break;
			case TE_STREAM_LIGHTNING:
				if(stream->endTime < cl.serverTimeFloat)
				{//fixme: keep last non-translucent frame and angle
					CL_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128 + (stream->endTime - cl.serverTimeFloat)*192, MLS_ABSLIGHT|DRF_TRANSLUCENT);
				}
				else
				{
					angles[2] = rand()%360;
					ent.frame = rand()%6;
					CL_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, MLS_ABSLIGHT);
				}
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_LIGHTNING_SMALL:
				if(stream->endTime < cl.serverTimeFloat)
				{
					CL_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128 + (stream->endTime - cl.serverTimeFloat)*192, MLS_ABSLIGHT|DRF_TRANSLUCENT);
				}
				else
				{
					angles[2] = rand()%360;
					ent.frame = rand()%6;
					CL_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, MLS_ABSLIGHT);
				}
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_FAMINE:
				angles[2] = rand()%360;
				ent.frame = 0;
				CL_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_COLORBEAM:
				angles[2] = 0;
				ent.skinNum = stream->skin;
				CL_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, MLS_ABSLIGHT);
				R_HandleCustomSkin(&ent, -1);
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_GAZE:
				angles[2] = 0;
				ent.frame = (int)(cl.serverTimeFloat*40)%36;
				CL_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_ICECHUNKS:
				angles[2] = rand()%360;
				ent.frame = rand()%5;
				CL_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			default:
				angles[2] = 0;
				CL_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 0, 0);
				R_AddRefEntityToScene(&ent);
			}
			for(i = 0; i < 3; i++)
			{
				org[i] += dist[i]*30;
			}
			d -= 30;
			segmentCount++;
		}
		if(stream->type == TE_STREAM_SUNSTAFF1)
		{
			if(stream->lastTrailTime+0.2 < cl.serverTimeFloat)
			{
				stream->lastTrailTime = cl.serverTimeFloat;
				CLH2_SunStaffTrail(stream->source, stream->dest);
			}

			refEntity_t ent;
			Com_Memset(&ent, 0, sizeof(ent));
			ent.reType = RT_MODEL;
			VectorCopy(stream->dest, ent.origin);
			ent.hModel = stream->models[2];
			//ent->frame = (int)(cl.time*20)%20;
			CL_SetRefEntAxis(&ent, vec3_origin, vec3_origin, 80 + (rand() & 15), 0, 128, MLS_ABSLIGHT);
			R_AddRefEntityToScene(&ent);

			Com_Memset(&ent, 0, sizeof(ent));
			ent.reType = RT_MODEL;
			VectorCopy(stream->dest, ent.origin);
			ent.hModel = stream->models[3];
			CL_SetRefEntAxis(&ent, vec3_origin, vec3_origin, 150 + (rand() & 15), 0, 128, MLS_ABSLIGHT|DRF_TRANSLUCENT);
			R_AddRefEntityToScene(&ent);
		}
	}
}


/*
=================
CL_UpdateTEnts
=================
*/
void CL_UpdateTEnts (void)
{
	CL_UpdateExplosions ();
	CL_UpdateStreams();
	CL_UpdateTargetBall();
}

void MultiGrenadeExplodeSound (explosion_t *ex)//plug up all of -1's channels w/ grenade sounds
{
	if (!(rand()&7))
	{
		if (MultiGrenadeCurrentChannel>=8 || MultiGrenadeCurrentChannel<0)
		{
			MultiGrenadeCurrentChannel=0;
		}

		S_StartSound(ex->origin, TempSoundChannel(), MultiGrenadeCurrentChannel++, cl_sfx_explode, 1, 1);
	}


	ex->frameFunc=NULL;
}


void MultiGrenadeThink (explosion_t *ex)
{//FIXME: too messy
	explosion_t *missile;

	int attack_counter,number_explosions;

	float ftemp;

	VectorSet(ex->velocity,0,0,0);
	VectorSet(ex->accel,0,0,0);

	ex->frameFunc=MultiGrenadeExplodeSound;

    attack_counter=0;        
    number_explosions=(rand()&2)+6;

    while(attack_counter<number_explosions)
    {
		attack_counter+=1;
		missile=CL_AllocExplosion();

        missile->frameFunc = MultiGrenadePieceThink;

        VectorCopy(ex->origin,missile->origin);

		if (rand()&1)
			missile->model = R_RegisterModel("models/gen_expl.spr");
		else
			missile->model = R_RegisterModel("models/bg_expld.spr");

		switch (attack_counter % 3)
		{
		case 2:
		    missile->frameFunc = MultiGrenadePieceThink;
			missile->velocity[0]=(rand()%600)-300;
			missile->velocity[1]=(rand()%600)-300;
			missile->velocity[2]=0;//(rand()%100)+50;

			ftemp = ( rand() / RAND_MAX * (0.5) );
			missile->startTime = cl.serverTimeFloat;// + 0.1 + ftemp - (ex->startTime - cl.time);
			ftemp = ( rand() / RAND_MAX * (0.3) )-0.15;
			missile->endTime = missile->startTime + R_ModelNumFrames(missile->model) * 0.1 + ftemp;
			break;
		case 1:
		    missile->frameFunc = MultiGrenadePiece2Think;
			missile->velocity[0]=(rand()%80)-40;
			missile->velocity[1]=(rand()%80)-40;
			missile->velocity[2]=(rand()%150)+150;

			ftemp = ( rand() / RAND_MAX  * (0.5) );
			missile->startTime = cl.serverTimeFloat;// + 0.1 + ftemp - (ex->startTime - cl.time);
			ftemp = ( rand() / RAND_MAX  * (0.3) )-0.15;
			missile->endTime = missile->startTime + R_ModelNumFrames(missile->model) * 0.1 + ftemp;
			break;
		default://some extra explosions for at first
		    missile->frameFunc = NULL;
			missile->origin[0]+=(rand()%50)-25;
			missile->origin[1]+=(rand()%50)-25;
			missile->origin[2]+=(rand()%50)-25;

			ftemp = ( rand() / RAND_MAX * (0.2) );
			missile->startTime = cl.serverTimeFloat + ftemp;
			ftemp = ( rand() / RAND_MAX * (0.2) )-0.1;
			missile->endTime = missile->startTime + R_ModelNumFrames(missile->model) * 0.1 + ftemp;
			break;
		}

		ftemp = ( rand() / RAND_MAX * (0.2) );

        missile->data=ex->data*(0.7  +  ftemp);

    }
}

void MultiGrenadePieceThink (explosion_t *ex)
{//FIXME: too messy

	explosion_t *missile;

	int attack_counter,number_explosions;
	float ftemp;

	VectorSet(ex->velocity,0,0,0);
	VectorSet(ex->accel,0,0,0);

	ex->frameFunc=MultiGrenadeExplodeSound;

	if (ex->data < 70)
		ex->data = 70;

	ftemp = ( rand() / RAND_MAX * (0.2) )-0.1;
	ex->startTime = cl.serverTimeFloat + ((1-(ex->data-69)/180.0)+ftemp-0.3) * 1.25;
	if (ex->startTime < cl.serverTimeFloat)
	{
		ex->startTime = cl.serverTimeFloat;
	}
	ftemp = ( rand() / RAND_MAX * (0.4) )-0.4;
	ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.1 + ftemp;

	if (R_ModelNumFrames(ex->model) > 14)
	{
		ex->startTime -= 0.4;
		ex->endTime -= 0.4;
	}

	if (ex->data <= 71)
		return;

    attack_counter=0;        
    number_explosions=(rand()&3)+2;

    while(attack_counter<number_explosions)
    {
		attack_counter+=1;
		missile=CL_AllocExplosion();

        missile->frameFunc = MultiGrenadePieceThink;

        VectorCopy(ex->origin,missile->origin);

        missile->origin[0]+=(rand()%100)-50;
        missile->origin[1]+=(rand()%100)-50;
        missile->origin[2]+=(rand()%10)-4;

		ftemp = ( rand() / RAND_MAX * (0.2) );

        missile->data=ex->data*(0.7  +  ftemp);

		if (rand()&7)
			missile->model = R_RegisterModel("models/bg_expld.spr");
		else
			missile->model = R_RegisterModel("models/fl_expld.spr");

		ftemp = ( rand() / RAND_MAX * (0.5) );

		missile->startTime = cl.serverTimeFloat+0.01;
		missile->endTime = missile->startTime + R_ModelNumFrames(missile->model) * 0.1;
    }
}

void MultiGrenadePiece2Think (explosion_t *ex)
{//FIXME: too messy

	explosion_t *missile;

	int attack_counter,number_explosions;
	float ftemp;

	VectorSet(ex->velocity,0,0,0);
	VectorSet(ex->accel,0,0,0);

	ex->frameFunc=MultiGrenadeExplodeSound;

	if (ex->data < 70)
	{
		ex->data = 70;
	}

	ftemp = 0;//( rand() / RAND_MAX * (0.2) )-0.1;
	ex->startTime = cl.serverTimeFloat + (((1 - (ex->data-69)/200.0)+ftemp)*1.5) - 0.2;
	ftemp = ( rand() / RAND_MAX * (0.4) )-0.2;
	ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.1 + ftemp;

	if (ex->data <= 71)
		return;

    attack_counter=0;        
    number_explosions=(rand()%3)+2;

    while(attack_counter<number_explosions)
    {
		attack_counter+=1;
		missile=CL_AllocExplosion();

        missile->frameFunc = MultiGrenadePiece2Think;

        VectorCopy(ex->origin,missile->origin);

		missile->origin[0]+=(rand()%30)-15;
		missile->origin[1]+=(rand()%30)-15;

		ftemp = rand() / RAND_MAX;

		missile->origin[2]+=(ftemp*7)+30;
        missile->data=ex->data - ftemp * 10 - 20;

		if (rand()&1)
			missile->model = R_RegisterModel("models/gen_expl.spr");
		else
			missile->model = R_RegisterModel("models/bg_expld.spr");

		ftemp = ( rand() / RAND_MAX * (0.5) );

		missile->startTime = cl.serverTimeFloat+0.01;
		missile->endTime = missile->startTime + R_ModelNumFrames(missile->model) * 0.1;
    }
}

void ChunkThink(explosion_t *ex)
{
	vec3_t oldorg;
	int			moving = 1;

	if (CM_PointContentsQ1(ex->origin, 0) != BSP29CONTENTS_EMPTY) //||in_solid==true
	{
		//collided with world
		VectorCopy(ex->oldorg, ex->origin);

		if((int)ex->data == THINGTYPE_FLESH)
		{
			if(VectorNormalize(ex->velocity) > 100.0)
			{	// hit, now make a splash of blood
				vec3_t	dmin = {-40, -40, 10};
				vec3_t	dmax = {40, 40, 40};
				CLH2_RunParticleEffect2 (ex->origin, dmin, dmax, 136 + (rand()%5), pt_h2darken, 20);
			}
		}
		else if((int)ex->data == THINGTYPE_ACID)
		{
			if(VectorNormalize(ex->velocity) > 100.0)
			{	// hit, now make a splash of acid
				//vec3_t	dmin = {-40, -40, 10};
				//vec3_t	dmax = {40, 40, 40};

				if(!(rand()%3))
				{
					S_StartSound(ex->origin, TempSoundChannel(), 0, cl_sfx_dropfizzle, 1, 1);
				}
			}
		}

		ex->velocity[0] = 0;
		ex->velocity[1] = 0;
		ex->velocity[2] = 0;
		ex->avel[0] = 0;
		ex->avel[1] = 0;
		ex->avel[2] = 0;

		moving = 0;
	}

	if(cl.serverTimeFloat + host_frametime * 5 > ex->endTime)
	{	// chunk leaves in 5 frames about
		switch((int)ex->data)
		{
		case THINGTYPE_METEOR:
			if(cl.serverTimeFloat + host_frametime * 4 < ex->endTime)
			{	// just crossed the threshold
				ex->abslight = 200;
			}
			else
			{
				ex->abslight -= 35;
			}
			ex->flags |= DRF_TRANSLUCENT|MLS_ABSLIGHT;
			ex->scale *= 1.4;
			ex->angles[0] += 20;
			break;
		default:
			ex->scale *= .8;
			break;
		}
	}

	ex->velocity[2] -= host_frametime * movevars.gravity; // this is gravity

	switch((int)ex->data)
	{
	case THINGTYPE_GREYSTONE:
		break;
	case THINGTYPE_WOOD:
		break;
	case THINGTYPE_METAL:
		break;
	case THINGTYPE_FLESH:
		if(moving)CLH2_TrailParticles (ex->oldorg, ex->origin, rt_blood);
		break;
	case THINGTYPE_FIRE:
		break;
	case THINGTYPE_CLAY:
		break;
	case THINGTYPE_LEAVES:
		break;
	case THINGTYPE_HAY:
		break;
	case THINGTYPE_BROWNSTONE:
	case THINGTYPE_DIRT:
		break;
	case THINGTYPE_CLOTH:
		break;
	case THINGTYPE_WOOD_LEAF:
		break;
	case THINGTYPE_WOOD_METAL:
		break;
	case THINGTYPE_WOOD_STONE:
		break;
	case THINGTYPE_METAL_STONE:
		break;
	case THINGTYPE_METAL_CLOTH:
		break;
	case THINGTYPE_WEBS:
		break;
	case THINGTYPE_GLASS:
		break;
	case THINGTYPE_ICE:
		ex->velocity[2] += host_frametime * movevars.gravity * 0.5; // lower gravity for ice chunks
		if(moving)CLH2_TrailParticles (ex->oldorg, ex->origin, rt_ice);
		break;
	case THINGTYPE_CLEARGLASS:
		break;
	case THINGTYPE_REDGLASS:
		break;
	case THINGTYPE_ACID:
		if(moving)CLH2_TrailParticles (ex->oldorg, ex->origin, rt_grensmoke);
		break;
	case THINGTYPE_METEOR:
		VectorCopy(ex->oldorg, oldorg);
		if(!moving)
		{	// resting meteors still give off smoke
			oldorg[0] += rand()%7 - 3;
			oldorg[1] += rand()%7 - 3;
			oldorg[2] += 16;
		}
		CLH2_TrailParticles (oldorg, ex->origin, rt_smoke);
		break;
	case THINGTYPE_GREENFLESH:
		if(moving)CLH2_TrailParticles (ex->oldorg, ex->origin, rt_grensmoke);
		break;

	}


}

void BubbleThink(explosion_t *ex)
{
	if (CM_PointContentsQ1(ex->origin, 0) == BSP29CONTENTS_WATER) 
	{
		//still in water

		if (ex->data < cl.serverTimeFloat)//change course
		{
			ex->velocity[0] += rand()%20 - 10;
			ex->velocity[1] += rand()%20 - 10;
			ex->velocity[2] += rand()%10 + 10;

			if (ex->velocity[0] > 10)
				ex->velocity[0] = 5;
			if (ex->velocity[0] < -10)
				ex->velocity[0] = -5;
				
			if (ex->velocity[1] > 10)
				ex->velocity[1] = 5;
			if (ex->velocity[1] < -10)
				ex->velocity[1] = -5;
				
			if (ex->velocity[2] < 10)
				ex->velocity[2] = 15;
			if (ex->velocity[2] > 30)
				ex->velocity[2] = 25;

			ex->data += 0.5;
		}
	}
	else
	{
		ex->endTime= cl.serverTimeFloat;
	}

}

void MissileFlashThink(explosion_t *ex)
{
	ex->abslight-=(0.05*256);
	ex->scale+=5;
	if(ex->abslight < (0.05*256))
		ex->endTime = ex->startTime;	//remove
}


void TeleportFlashThink(explosion_t *ex)
{
	ex->scale -= 15;
	if(ex->scale < 10)
		ex->endTime = ex->startTime;
}

// remove tent if not in open air
void CheckSpaceThink(explosion_t *ex)
{
	if (CM_PointContentsQ1(ex->origin, 0) != BSP29CONTENTS_EMPTY) 
	{
		ex->endTime = ex->startTime;
	}
}

// functions to create tempents from client effect calls

void CreateRavenExplosions(vec3_t pos)
{
	explosion_t	*ex;

	ex = CL_AllocExplosion ();
	VectorCopy(pos, ex->origin);
	ex->velocity[2] = 8;
	ex->model = R_RegisterModel("models/whtsmk1.spr");
	ex->startTime = cl.serverTimeFloat;
	ex->endTime = ex->startTime + HX_FRAME_TIME * 10;

	ex = CL_AllocExplosion ();
	VectorCopy(pos, ex->origin);
	ex->origin[2] -= 5;
	ex->velocity[2] = 8;
	ex->model = R_RegisterModel("models/whtsmk1.spr");
	ex->startTime = cl.serverTimeFloat;
	ex->endTime = ex->startTime + HX_FRAME_TIME * 10;

	ex = CL_AllocExplosion ();
	VectorCopy(pos, ex->origin);
	ex->origin[2] -= 10;
	ex->velocity[2] = 8;
	ex->model = R_RegisterModel("models/whtsmk1.spr");
	ex->startTime = cl.serverTimeFloat;
	ex->endTime = ex->startTime + HX_FRAME_TIME * 10;
}

void CreateRavenDeath(vec3_t pos)
{
	explosion_t	*ex;

	ex = CL_AllocExplosion ();
	VectorCopy(pos, ex->origin);
	ex->velocity[1] = 8;
	ex->velocity[2] = -10;
	ex->model = R_RegisterModel("models/whtsmk1.spr");
	ex->startTime = cl.serverTimeFloat;
	ex->endTime = ex->startTime + HX_FRAME_TIME * 10;

	ex = CL_AllocExplosion ();
	VectorCopy(pos, ex->origin);
	ex->velocity[2] = -10;
	ex->model = R_RegisterModel("models/redsmk1.spr");
	ex->startTime = cl.serverTimeFloat;
	ex->endTime = ex->startTime + HX_FRAME_TIME * 10;

	ex = CL_AllocExplosion ();
	VectorCopy(pos, ex->origin);
	ex->velocity[1] = -8;
	ex->velocity[2] = -10;
	ex->model = R_RegisterModel("models/whtsmk1.spr");
	ex->startTime = cl.serverTimeFloat;
	ex->endTime = ex->startTime + HX_FRAME_TIME * 10;
	
	S_StartSound(pos, TempSoundChannel(), 1, cl_sfx_ravendie, 1, 1);
}

void CreateExplosionWithSound(vec3_t pos)
{
	explosion_t	*ex;

	ex = CL_AllocExplosion ();
	VectorCopy(pos, ex->origin);
	ex->startTime = cl.serverTimeFloat;
	ex->endTime = ex->startTime + HX_FRAME_TIME * 10;
	ex->model = R_RegisterModel("models/sm_expld.spr");
	
	S_StartSound(pos, TempSoundChannel(), 1, cl_sfx_explode, 1, 1);
}

void SwordFrameFunc(explosion_t *ex)
{
	ex->scale = (ex->endTime - cl.serverTimeFloat)*150 + 1;
	if(((int)(cl.serverTimeFloat * 20.0))%2)
	{
		ex->skin = 0;
	}
	else
	{
		ex->skin = 1;
	}

}

void zapFrameFunc(explosion_t *ex)
{
	ex->scale = (ex->endTime - cl.serverTimeFloat)*(150/.3) + 1;
	//ex->abslight = (224/.3) * (ex->endTime - cl.time) + 1;
	if(((int)(cl.serverTimeFloat * 20.0))%2)
	{
		ex->skin = 0;
	}
	else
	{
		ex->skin = 1;
	}

}

void fireBallUpdate(explosion_t *ex)
{
	ex->scale = (int)(((cl.serverTimeFloat - ex->startTime) / 1.0) * 250)+1;
}

void sunBallUpdate(explosion_t *ex)
{
	ex->scale = 121 - (int)(((cl.serverTimeFloat - ex->startTime) / .8) * 120);
}

void sunPowerUpdate(explosion_t *ex)
{

}

void purify1Update(explosion_t *ex)
{
	CLH2_TrailParticles (ex->oldorg, ex->origin, rt_purify);
	//CLH2_TrailParticles (ex->oldorg, ex->origin, rt_setstaff);
}

void MeteorBlastThink(explosion_t *ex)
{
	int i, maxI;
	explosion_t *ex2;
	vec3_t		tempVect, oldPos;
	int			hitWall = 0;

	CLH2_TrailParticles (ex->oldorg, ex->origin, rt_fireball);

	ex->data -= 1600 * host_frametime; // decrease distance, roughly...

	if(ex->data <= 0)
	{	// ran out of juice
		VectorCopy(ex->origin, oldPos);
		hitWall = 1;
	}
	else
	{
		VectorCopy(ex->oldorg, tempVect);

		for(i = 0; (i < 10)&&(!hitWall); i++)
		{
			VectorCopy(tempVect, oldPos);
			VectorScale(ex->origin, .1 * (i+1), tempVect);
			VectorMA(tempVect, 1.0 - (.1 * (i+1)), ex->oldorg, tempVect);
			if (CM_PointContentsQ1(tempVect, 0) != BSP29CONTENTS_EMPTY)
			{
				hitWall = 1;
			}
		}
	}

	if(hitWall)
	{	//collided with world
		VectorCopy(oldPos, ex->origin);

		maxI = (host_frametime <= 0.05) ? 12:5;
		for(i = 0; i < maxI; i++)
		{
			ex2 = CL_AllocExplosion();
			VectorCopy(ex->origin, ex2->origin);
			ex2->origin[0] += (rand()%10) - 5;
			ex2->origin[1] += (rand()%10) - 5;
			ex2->origin[2] += (rand()%10);

			ex2->velocity[0] = (ex2->origin[0] - ex->origin[0])*12;
			ex2->velocity[1] = (ex2->origin[1] - ex->origin[1])*12;
			ex2->velocity[2] = (ex2->origin[2] - ex->origin[2])*12;

			switch(rand()%4)
			{
			case 0:
			case 1:
				ex2->model = R_RegisterModel("models/sm_expld.spr");
				break;
			case 2:
				ex2->model = R_RegisterModel("models/bg_expld.spr");
				break;
			case 3:
				ex2->model = R_RegisterModel("models/gen_expl.spr");
				break;
			}
			ex2->flags |= MLS_ABSLIGHT|DRF_TRANSLUCENT;
			ex2->abslight = 160 + rand()%64;
			ex2->skin = 0;
			ex2->scale = 80 + rand()%40;
			ex2->startTime = cl.serverTimeFloat + (rand()%30 / 200.0);
			ex2->endTime = ex2->startTime + R_ModelNumFrames(ex2->model) * 0.03;
		}
		if(rand()&1)
		{
			S_StartSound(ex->origin, TempSoundChannel(), 0, cl_sfx_axeExplode, 1, 1);
		}

		ex->model = 0;
		ex->endTime = cl.serverTimeFloat;
	}
}

void MeteorCrushSpawnThink(explosion_t *ex)
{
	float chance;
	explosion_t *ex2;

	if(host_frametime <= 0.05)
	{
		chance = (host_frametime / .4 * 16);
	}
	else
	{
		chance = (host_frametime / .4 * 8);
	}

	while(chance > 0)
	{
		if(chance < 1.0)
		{	// take care of fractional numbers of meteors...
			if((rand()%100) > chance * 100)
			{
				return;
			}
		}

		ex2 = CL_AllocExplosion();
		VectorCopy(ex->origin, ex2->origin);
		ex2->origin[0] += (rand()%160) - 80;
		ex2->origin[1] += (rand()%160) - 80;
		ex2->model = R_RegisterModel("models/tempmetr.mdl");
		ex2->startTime = cl.serverTimeFloat;
		ex2->endTime = ex2->startTime + 2.0;
		ex2->frameFunc = MeteorBlastThink;

		ex2->exflags = EXFLAG_ROTATE;

		ex2->avel[0] = 800;
		ex2->avel[1] = 800;
		ex2->avel[2] = 800;

//		ex2->velocity[0] = rand()%1200 - 600;
//		ex2->velocity[1] = rand()%1200 - 600;
		ex2->velocity[0] = rand()%180 - 90;
		ex2->velocity[1] = rand()%180 - 90;
		ex2->velocity[2] = -1600 - (rand()%200);
		ex2->data = ex->data;	// stores how far the thingy can travel max

		ex2->scale = 200 + (rand()%120);


		chance -= 1.0;
	}
}

void updateBloodRain(explosion_t *ex)
{
	CLH2_TrailParticles (ex->oldorg, ex->origin, rt_blood);

	ex->scale -= host_frametime / .3 * 60;
	if(ex->scale <= 0)
	{
		ex->endTime = cl.serverTimeFloat - 1;
	}
}

void updatePurify2(explosion_t *ex)
{
	explosion_t	*ex2;
	int numSprites;

	CLH2_TrailParticles (ex->oldorg, ex->origin, rt_purify);

	if(host_frametime <= 0.05)
	{
		numSprites = 20;
	}
	else
	{
		numSprites = 8;
	}

	if((rand()%100)/100.0 < numSprites * host_frametime)
	{
		ex2 = CL_AllocExplosion();
		VectorCopy(ex->origin, ex2->origin);
		ex2->model = R_RegisterModel("models/ring.mdl");
		ex2->startTime = cl.serverTimeFloat;
		ex2->endTime = ex2->startTime + 1.2;

		ex2->scale = 150;

		VectorCopy(ex->angles, ex2->angles);
		ex2->angles[2] += 90;
		ex2->exflags |= EXFLAG_STILL_FRAME;
		ex2->data = 0;
		ex2->skin = 0;
		ex2->frameFunc = SmokeRingFrameFunc;

		ex2->velocity[2] = 15.0;

		ex2->flags |= DRF_TRANSLUCENT | SCALE_ORIGIN_CENTER;
	}
}

void updateSwordShot(explosion_t *ex)
{
	int testVal;

	CLH2_TrailParticles (ex->oldorg, ex->origin, rt_vorpal);

	ex->data = 16 + ((int)(cl.serverTimeFloat * 20.0)%13);

	ex->flags |= MLS_ABSLIGHT;
	ex->abslight = 128;

	testVal = (int)(cl.serverTimeFloat * 20.0);

	if(testVal % 2)
	{
		ex->skin = 0;
	}
	else
	{
		ex->skin = 1;
	}

}

void updateIceShot(explosion_t *ex)
{
	CLH2_TrailParticles (ex->oldorg, ex->origin, rt_ice);
}

void updateMeteor(explosion_t *ex)
{
	CLH2_TrailParticles (ex->oldorg, ex->origin, rt_smoke);
}

void updateAcidBlob(explosion_t *ex)
{
	explosion_t	*ex2;
	int testVal, testVal2;

	CLH2_TrailParticles (ex->oldorg, ex->origin, rt_acidball);

	testVal = (int)(cl.serverTimeFloat * 10.0);
	testVal2 = (int)((cl.serverTimeFloat - host_frametime)*10.0);

	if(testVal != testVal2)
	{
		if(!(testVal%2))
		{
			ex2 = CL_AllocExplosion();
			VectorCopy(ex->origin, ex2->origin);
			ex2->model = R_RegisterModel("models/muzzle1.spr");
			ex2->startTime = cl.serverTimeFloat;
			ex2->endTime = ex2->startTime + .4;

			VectorCopy(ex->angles, ex2->angles);
			ex2->angles[2] += 90;
			//ex->exflags |= EXFLAG_STILL_FRAME;
			//ex->data = 0;
			ex2->skin = 0;

			ex2->velocity[0] = 0;
			ex2->velocity[1] = 0;
			ex2->velocity[2] = 0;

			ex2->flags |= DRF_TRANSLUCENT | SCALE_ORIGIN_CENTER;
		}
	}
}

void updateAcidBall(explosion_t *ex)
{
	CLH2_TrailParticles (ex->oldorg, ex->origin, rt_acidball);
}

//*****************************************************************************
//
//						FLAG UPDATE FUNCTIONS
//
//*****************************************************************************

void CL_UpdatePoisonGas(refEntity_t *ent, vec3_t angles, int edict_num)
{
	explosion_t	*ex;
	float		smokeCount;

	if(host_frametime <= 0.05)
	{
		smokeCount = 32 * host_frametime;
	}
	else
	{
		smokeCount = 16 * host_frametime;
	}

	while(smokeCount > 0)
	{
		if(smokeCount < 1.0)
		{
			if((rand()%100)/100 > smokeCount)
			{	// account for fractional chance of more smoke...
				smokeCount = 0;
				continue;
			}
		}

		ex = CL_AllocExplosion();
		VectorCopy(ent->origin, ex->origin);
		ex->model = R_RegisterModel("models/grnsmk1.spr");
		ex->startTime = cl.serverTimeFloat;
		ex->endTime = ex->startTime + .7 + (rand()%200)*.001;

		ex->scale = 100;

		VectorCopy(angles, ex->angles);
		ex->angles[2] += 90;
		//ex->exflags |= EXFLAG_STILL_FRAME;
		//ex->data = 0;
		ex->skin = 0;

		ex->velocity[0] = (rand()%100) - 50;
		ex->velocity[1] = (rand()%100) - 50;
		ex->velocity[2] = 150.0;

		ex->flags |= DRF_TRANSLUCENT | SCALE_ORIGIN_CENTER;

		smokeCount -= 1.0;
	}
}

void CL_UpdateAcidBlob(refEntity_t *ent, vec3_t angles, int edict_num)
{
	explosion_t	*ex;
	int testVal, testVal2;

	testVal = (int)(cl.serverTimeFloat * 10.0);
	testVal2 = (int)((cl.serverTimeFloat - host_frametime)*10.0);

	if(testVal != testVal2)
	{
		if(!(testVal%2))
		{
			ex = CL_AllocExplosion();
			VectorCopy(ent->origin, ex->origin);
			ex->model = R_RegisterModel("models/muzzle1.spr");
			ex->startTime = cl.serverTimeFloat;
			ex->endTime = ex->startTime + .4;

			ex->scale = 100;

			VectorCopy(angles, ex->angles);
			ex->angles[2] += 90;
			//ex->exflags |= EXFLAG_STILL_FRAME;
			//ex->data = 0;
			ex->skin = 0;

			ex->velocity[0] = 0;
			ex->velocity[1] = 0;
			ex->velocity[2] = 0;

			ex->flags |= DRF_TRANSLUCENT | SCALE_ORIGIN_CENTER;
		}
	}
}

void CL_UpdateOnFire(refEntity_t *ent, vec3_t angles, int edict_num)
{
	explosion_t *ex;

	if((rand()%100)/100.0 < 5.0 * host_frametime)
	{
		ex = CL_AllocExplosion();
		VectorCopy(ent->origin, ex->origin);

		//raise and randomize origin some
		//ex->origin[0]+=(rand()%ent->maxs[0])-ent->maxs[0]/2;
		//ex->origin[1]+=(rand()%ent->maxs[1])-ent->maxs[1]/2;
		//ex->origin[2]+=ent->maxs[2]/2;
		ex->origin[0]+=(rand()%10) - 5;
		ex->origin[1]+=(rand()%10) - 5;
		ex->origin[2]+=rand()%20+10;//at least 10 up from origin, sprite's origin is in center!

		switch(rand()%3)
		{
		case 0:
			ex->model = R_RegisterModel("models/firewal1.spr");
			break;
		case 1:
			ex->model = R_RegisterModel("models/firewal2.spr");
			break;
		case 2:
			ex->model = R_RegisterModel("models/firewal3.spr");
			break;
		}
		
		ex->startTime = cl.serverTimeFloat;
		ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.05;

		ex->scale = 100;

		VectorCopy(angles, ex->angles);
		ex->angles[2] += 90;

		ex->velocity[0] = (rand()%40)-20;
		ex->velocity[1] = (rand()%40)-20;
		ex->velocity[2] = 50 + (rand()%100);

		if(rand()%5)//translucent 80% of the time
			ex->flags |= DRF_TRANSLUCENT;
	}
}

void PowerFlameBurnRemove(explosion_t *ex)
{
	explosion_t *ex2;

	ex2 = CL_AllocExplosion();
	VectorCopy(ex->origin, ex2->origin);
	switch(rand()%3)
	{
	case 0:
		ex2->model = R_RegisterModel("models/sm_expld.spr");
		break;
	case 1:
		ex2->model = R_RegisterModel("models/fboom.spr");
		break;
	case 2:
		ex2->model = R_RegisterModel("models/pow.spr");
		break;
	}
	ex2->startTime = cl.serverTimeFloat;
	ex2->endTime = ex2->startTime + R_ModelNumFrames(ex2->model) * 0.05;

	ex2->scale = 100;

	ex2->flags |= MLS_ABSLIGHT|DRF_TRANSLUCENT;
	ex2->abslight = 128;

	
	if(rand()&1)S_StartSound(ex2->origin, TempSoundChannel(), 1, cl_sfx_flameend, 1, 1);
}

void CL_UpdatePowerFlameBurn(refEntity_t *ent, int edict_num)
{
	explosion_t *ex, *ex2;
	vec3_t		srcVec;

	if((rand()%100)/100.0 < host_frametime * 10)
	{
		ex = CL_AllocExplosion();
		VectorCopy(ent->origin, ex->origin);
		ex->origin[0] += (rand()%120)-60;
		ex->origin[1] += (rand()%120)-60;
		ex->origin[2] += (rand()%120)-60+120;
		ex->model = R_RegisterModel("models/sucwp1p.mdl");
		ex->startTime = cl.serverTimeFloat;
		ex->endTime = ex->startTime + .25;
		ex->removeFunc = PowerFlameBurnRemove;

		ex->scale = 100;

		VectorSubtract(ent->origin, ex->origin, srcVec);
		VectorCopy(srcVec, ex->velocity);
		ex->velocity[2] += 24;
		VectorScale(ex->velocity, 1.0 / .25, ex->velocity);
		VectorNormalize(srcVec);
		vectoangles(srcVec, ex->angles);

		ex->flags |= MLS_ABSLIGHT;//|DRF_TRANSLUCENT;
		ex->abslight = 128;


		// I'm not seeing this right now... (?)
		ex2 = CL_AllocExplosion();
		VectorCopy(ex->origin, ex2->origin);
		ex2->model = R_RegisterModel("models/flamestr.spr");
		ex2->startTime = cl.serverTimeFloat;
		ex2->endTime = ex2->startTime + R_ModelNumFrames(ex2->model) * 0.05;
		ex2->flags |= DRF_TRANSLUCENT;
	}
}

void CL_UpdateHammer(refEntity_t *ent, int edict_num)
{
	int testVal, testVal2;
	// do this every .3 seconds
	testVal = (int)(cl.serverTimeFloat * 10.0);
	testVal2 = (int)((cl.serverTimeFloat - host_frametime)*10.0);

	if(testVal != testVal2)
	{
		if(!(testVal%3))
		{
			//S_StartSound(ent->origin, TempSoundChannel(), 1, cl_sfx_hammersound, 1, 1);
			S_StartSound(ent->origin, edict_num, 2, cl_sfx_hammersound, 1, 1);
		}
	}
}

void CL_UpdateBug(refEntity_t *ent)
{
	int testVal, testVal2;
	testVal = (int)(cl.serverTimeFloat * 10.0);
	testVal2 = (int)((cl.serverTimeFloat - host_frametime)*10.0);

	if(testVal != testVal2)
	{
	// do this every .1 seconds
//		if(!(testVal%3))
//		{
			S_StartSound(ent->origin, TempSoundChannel(), 1, cl_sfx_buzzbee, 1, 1);
//		}
	}
}

void CL_UpdateIceStorm(refEntity_t *ent, int edict_num)
{
	vec3_t			center, side1;
	vec3_t			side2 = {160, 160, 128};
	entity_state_t	*state;
	static float	playIceSound = .6;

	state = FindState(edict_num);
	if (state)
	{
		VectorCopy(state->origin, center);

		// handle the particles
		VectorCopy(center, side1);
		side1[0] -= 80;
		side1[1] -= 80;
		side1[2] += 104;
		CLH2_RainEffect2(side1, side2, rand()%400-200, rand()%400-200, rand()%15 + 9*16, (int)(30*20*host_frametime));

		playIceSound+=host_frametime;
		if(playIceSound >= .6)
		{
			S_StartSound(center, TempSoundChannel(), 0, cl_sfx_icestorm, 1, 1);
			playIceSound -= .6;
		}
	}

	// toss little ice chunks
	if(rand()%100 < host_frametime * 100.0 * 3)
	{
		explosion_t *ex;

		ex = CL_AllocExplosion();
		VectorCopy(ent->origin,ex->origin);
		ex->origin[0] += rand()%32 - 16;
		ex->origin[1] += rand()%32 - 16;
		ex->origin[2] += 48 + rand()%32;
		ex->frameFunc = ChunkThink;

		ex->velocity[0] += (rand()%300)-150;
		ex->velocity[1] += (rand()%300)-150;
		ex->velocity[2] += (rand()%200)-50;

		// are these in degrees or radians?
		ex->angles[0] = rand()%360;
		ex->angles[1] = rand()%360;
		ex->angles[2] = rand()%360;
		ex->exflags = EXFLAG_ROTATE;

		ex->avel[0] = rand()%850 - 425;
		ex->avel[1] = rand()%850 - 425;
		ex->avel[2] = rand()%850 - 425;

		ex->scale = 65 + rand()%10;

		ex->data = THINGTYPE_ICE;

		ex->model = R_RegisterModel("models/shard.mdl");
		ex->skin = 0;
		//ent->frame = rand()%2;
		ex->flags |= DRF_TRANSLUCENT|MLS_ABSLIGHT;
		ex->abslight = 128;

		ex->startTime = cl.serverTimeFloat;
		ex->endTime = ex->startTime + 2.0;
	}
}

void telPuffMove (explosion_t *ex)
{
	vec3_t tvec,tvec2;
	VectorSubtract(ex->angles, ex->origin, tvec);
	VectorCopy(tvec,tvec2);
	VectorNormalize(tvec);
	VectorScale(tvec,320,tvec);

	ex->velocity[0] = tvec[1];
	ex->velocity[1] = -tvec[0];
	ex->velocity[2] += FRANDOM();

	if (ex->velocity[2] > 40)
	{
		ex->velocity[2] = 30;
	}

	//keep it from getting too spread out:
	ex->velocity[0] += tvec2[0];
	ex->velocity[1] += tvec2[1];

	CLH2_TrailParticles (ex->oldorg, ex->origin, rt_magicmissile);

//	ex->accel[0] = ex->velocity[1]*5;
//	ex->accel[1] = -ex->velocity[0]*5;
//	ex->accel[2] += FRANDOM()*3.0;
}

void telEffectUpdate (explosion_t *ex)
{
	explosion_t *ex2;
	float angle;
	int testVal, testVal2;
	vec3_t tvec;

	if (ex->endTime - cl.serverTimeFloat <= 1.2)
		ex->frameFunc = NULL;

	testVal = (int)(cl.serverTimeFloat * 10.0);
	testVal2 = (int)((cl.serverTimeFloat - host_frametime)*10.0);

	if(testVal != testVal2)
	{
		if(!(testVal%3))
		{
			ex2 = CL_AllocExplosion();
			angle = FRANDOM() * 3.14159;

			VectorCopy(ex->origin,ex2->origin);
			VectorCopy(ex->origin,ex2->angles);
			ex2->origin[0]+=cos(angle)*10;
			ex2->origin[1]+=sin(angle)*10;

			VectorSubtract(ex->origin, ex2->origin, tvec);
			VectorScale(tvec,20,tvec);
			ex2->model = R_RegisterModel("models/sm_blue.spr");
			ex2->startTime = cl.serverTimeFloat;
			ex2->endTime = ex2->startTime + 3.2;

			ex2->scale = 100;

			ex2->data = 0;
			ex2->skin = 0;
			ex2->frameFunc = telPuffMove;

			ex2->velocity[0] = tvec[1];
			ex2->velocity[1] = -tvec[0];
			ex2->velocity[2] = 20.0;

			ex2->flags |= MLS_ABSLIGHT;
			ex2->abslight = 128;
		}
	}

	if (ex->endTime > cl.serverTimeFloat + 0.01)
	{
		ex->startTime = cl.serverTimeFloat+0.01;
	}


}

void CL_UpdateTargetBall(void)
{
	int i;
	explosion_t *ex1 = NULL;
	explosion_t *ex2 = NULL;
	qhandle_t	iceMod;
	vec3_t		newOrg;
	float		newScale;

	if (v_targDist < 24)
	{	// either there is no ball, or it's too close to be needed...
		return;
	}

	// search for the two thingies.  If they don't exist, make new ones and set v_oldTargOrg

	iceMod = R_RegisterModel("models/iceshot2.mdl");

	for(i = 0; i < MAX_EXPLOSIONS; i++)
	{
		if(cl_explosions[i].endTime > cl.serverTimeFloat)
		{	// make certain it's an active one
			if(cl_explosions[i].model == iceMod)
			{
				if(cl_explosions[i].flags & DRF_TRANSLUCENT)
				{
					ex1 = &cl_explosions[i];
				}
				else
				{
					ex2 = &cl_explosions[i];
				}
			}
		}
	}

	VectorCopy(cl.simorg, newOrg);
	newOrg[0] += cos(v_targAngle*M_PI*2/256.0) * 50 * cos(v_targPitch*M_PI*2/256.0);
	newOrg[1] += sin(v_targAngle*M_PI*2/256.0) * 50 * cos(v_targPitch*M_PI*2/256.0);
	newOrg[2] += 44 + sin(v_targPitch*M_PI*2/256.0) * 50 + cos(cl.serverTimeFloat*2)*5;
	if(v_targDist < 60)
	{	// make it scale back down up close...
		newScale = 172 - (172 * (1.0 - (v_targDist - 24.0)/36.0));
	}
	else
	{
		newScale = 80 + (120 * ((256.0 - v_targDist)/256.0));
	}
	if(ex1 == NULL)
	{
		ex1 = CL_AllocExplosion();
		ex1->model = iceMod;
		ex1->exflags |= EXFLAG_STILL_FRAME;
		ex1->data = 0;

		ex1->flags |= MLS_ABSLIGHT|DRF_TRANSLUCENT;
		ex1->skin = 0;
		VectorCopy(newOrg, ex1->origin);
		ex1->scale = newScale;
	}

	VectorScale(ex1->origin, (.75 - host_frametime * 1.5), ex1->origin);	// FIXME this should be affected by frametime...
	VectorMA(ex1->origin, (.25 + host_frametime * 1.5), newOrg, ex1->origin);
	ex1->startTime = cl.serverTimeFloat;
	ex1->endTime = ex1->startTime + host_frametime + 0.2;
	ex1->scale = (ex1->scale * (.75 - host_frametime * 1.5) + newScale * (.25 + host_frametime * 1.5));
	ex1->angles[0] = v_targPitch*360/256.0;
	ex1->angles[1] = v_targAngle*360/256.0;
	ex1->angles[2] = cl.serverTimeFloat * 240;
	ex1->abslight = 96 + (32 * cos(cl.serverTimeFloat*6.5)) + (64 * ((256.0 - v_targDist)/256.0));

	if(v_targDist < 60)
	{	// make it scale back down up close...
		newScale = 76 - (76 * (1.0 - (v_targDist - 24.0)/36.0));
	}
	else
	{
		newScale = 30 + (60 * ((256.0 - v_targDist)/256.0));
	}
	if(ex2 == NULL)
	{
		ex2 = CL_AllocExplosion();
		ex2->model = iceMod;
		ex2->exflags |= EXFLAG_STILL_FRAME;
		ex2->data = 0;

		ex2->flags |= MLS_ABSLIGHT;
		ex2->skin = 0;
		ex2->scale = newScale;
	}
	VectorCopy(ex1->origin, ex2->origin);
	ex2->startTime = cl.serverTimeFloat;
	ex2->endTime = ex2->startTime + host_frametime + 0.2;
	ex2->scale = (ex2->scale * (.75 - host_frametime * 1.5) + newScale * (.25 + host_frametime * 1.5));
	ex2->angles[0] = ex1->angles[0];
	ex2->angles[1] = ex1->angles[1];
	ex2->angles[2] = cl.serverTimeFloat * -360;
	ex2->abslight = 96 + (128 * cos(cl.serverTimeFloat*4.5));

	CLHW_TargetBallEffectParticles (ex1->origin, v_targDist);
}

void SmokeRingFrameFunc(explosion_t *ex)
{
	if(cl.serverTimeFloat - ex->startTime < .3)
	{
		ex->skin = 0;
	}
	else if(cl.serverTimeFloat - ex->startTime < .6)
	{
		ex->skin = 1;
	}
	else if(cl.serverTimeFloat - ex->startTime < .9)
	{
		ex->skin = 2;
	}
	else
	{
		ex->skin = 3;
	}
}
