// cl_tent.c -- client side temporary entities

// HEADER FILES ------------------------------------------------------------

#include "quakedef.h"

// MACROS ------------------------------------------------------------------

#define FRANDOM() (rand()*(1.0/RAND_MAX))

// TYPES -------------------------------------------------------------------

// PUBLIC FUNCTION DEFINITIONS ---------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void ParseStream(int type);
static h2stream_t *NewStream(int ent, int tag, int *isNew);

void purify1Update(h2explosion_t *ex);
void CL_UpdateTargetBall(void);

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static sfxHandle_t		cl_sfx_tink1;
static sfxHandle_t		cl_sfx_ric1;
static sfxHandle_t		cl_sfx_ric2;
static sfxHandle_t		cl_sfx_ric3;
static sfxHandle_t		clh2_sfx_r_exp3;
static sfxHandle_t		cl_sfx_buzzbee;

static sfxHandle_t		cl_sfx_icestorm;
static sfxHandle_t		cl_sfx_sunstaff;
static sfxHandle_t		cl_sfx_sunhit;
static sfxHandle_t		cl_sfx_lightning1;
static sfxHandle_t		cl_sfx_lightning2;
static sfxHandle_t		cl_sfx_hammersound;
static sfxHandle_t		cl_sfx_tornado;



/*
=================
CL_ParseTEnts
=================
*/
void CL_InitTEnts (void)
{
	CLHW_InitExplosionSounds();
	cl_sfx_tink1 = S_RegisterSound("weapons/tink1.wav");
	cl_sfx_ric1 = S_RegisterSound("weapons/ric1.wav");
	cl_sfx_ric2 = S_RegisterSound("weapons/ric2.wav");
	cl_sfx_ric3 = S_RegisterSound("weapons/ric3.wav");
	clh2_sfx_r_exp3 = S_RegisterSound("weapons/r_exp3.wav");
	cl_sfx_buzzbee = S_RegisterSound("assassin/scrbfly.wav");

	cl_sfx_icestorm = S_RegisterSound("crusader/blizzard.wav");
	cl_sfx_sunstaff = S_RegisterSound("crusader/sunhum.wav");
	cl_sfx_sunhit = S_RegisterSound("crusader/sunhit.wav");
	cl_sfx_lightning1 = S_RegisterSound("crusader/lghtn1.wav");
	cl_sfx_lightning2 = S_RegisterSound("crusader/lghtn2.wav");
	cl_sfx_hammersound = S_RegisterSound("paladin/axblade.wav");
	cl_sfx_tornado = S_RegisterSound("crusader/tornado.wav");
}

/*
=================
CL_ClearTEnts
=================
*/
void CL_ClearTEnts (void)
{
	CLH2_ClearExplosions();
	CLH2_ClearStreams();
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

h2entity_state_t *FindState(int EntNum)
{
	packet_entities_t		*pack;
	static h2entity_state_t	pretend_player;
	int						pnum;

	if (EntNum >= 1 && EntNum <= HWMAX_CLIENTS)
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
	h2entity_state_t	*state;

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
	stream->endTime = cl_common->serverTime * 0.001+duration;
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
	h2entity_state_t	*state;

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
	stream->endTime = cl_common->serverTime * 0.001+duration;
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
				if(stream->endTime > cl_common->serverTime * 0.001)
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
		if(!stream->models[0] || stream->endTime < cl_common->serverTime * 0.001)
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
	int type;
	vec3_t pos, vel;
	int rnd;
	int cnt, i;

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
				S_StartSound(pos, CLH2_TempSoundChannel(), 0, cl_sfx_tink1, 1, 1);
			else
			{
				rnd = rand() & 3;
				if (rnd == 1)
					S_StartSound(pos, CLH2_TempSoundChannel(), 0, cl_sfx_ric1, 1, 1);
				else if (rnd == 2)
					S_StartSound(pos, CLH2_TempSoundChannel(), 0, cl_sfx_ric2, 1, 1);
				else
					S_StartSound(pos, CLH2_TempSoundChannel(), 0, cl_sfx_ric3, 1, 1);
			}
			break;
		case TE_SUPERSPIKE:			// super spike hitting wall
			pos[0] = net_message.ReadCoord ();
			pos[1] = net_message.ReadCoord ();
			pos[2] = net_message.ReadCoord ();
			CLH2_RunParticleEffect (pos, vec3_origin, 20);

			if ( rand() % 5 )
				S_StartSound(pos, CLH2_TempSoundChannel(), 0, cl_sfx_tink1, 1, 1);
			else
			{
				rnd = rand() & 3;
				if (rnd == 1)
					S_StartSound(pos, CLH2_TempSoundChannel(), 0, cl_sfx_ric1, 1, 1);
				else if (rnd == 2)
					S_StartSound(pos, CLH2_TempSoundChannel(), 0, cl_sfx_ric2, 1, 1);
				else
					S_StartSound(pos, CLH2_TempSoundChannel(), 0, cl_sfx_ric3, 1, 1);
			}
			break;

		case TE_DRILLA_EXPLODE:
			CLHW_ParseDrillaExplode(net_message);
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
			S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_r_exp3, 1, 1);
			break;
			
		case TE_TAREXPLOSION:			// tarbaby explosion
			pos[0] = net_message.ReadCoord ();
			pos[1] = net_message.ReadCoord ();
			pos[2] = net_message.ReadCoord ();
			CLH2_BlobExplosion (pos);

			S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_sfx_r_exp3, 1, 1);
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

		case TE_BIGGRENADE:
			CLHW_ParseBigGrenade(net_message);
			break;
		case TE_CHUNK:
			CLHW_ParseChunk(net_message);
			break;
		case TE_CHUNK2:
			CLHW_ParseChunk2(net_message);
			break;
		case TE_XBOWHIT:
			CLHW_ParseXBowHit(net_message);
			break;
		case TE_METEORHIT:
			CLHW_ParseMeteorHit(net_message);
			break;
		case TE_HWBONEPOWER:
			CLHW_ParseBonePower(net_message);
			break;
		case TE_HWBONEPOWER2:
			CLHW_ParseBonePower2(net_message);
			break;
		case TE_HWRAVENDIE:
			CLHW_ParseRavenDie(net_message);
			break;
		case TE_HWRAVENEXPLODE:
			CLHW_ParseRavenExplode(net_message);
			break;
		case TE_ICEHIT:
			CLHW_ParseIceHit(net_message);
			break;

		case TE_ICESTORM:
			{
				int				ent;
				vec3_t			center;
				h2stream_t		*stream;
				qhandle_t		models[2];
				h2entity_state_t	*state;
				static float	playIceSound = .6;

				ent = net_message.ReadShort();

				state = FindState(ent);
				if (state)
				{
					VectorCopy(state->origin, center);

					playIceSound+=host_frametime;
					if(playIceSound >= .6)
					{
						S_StartSound(center, CLH2_TempSoundChannel(), 0, cl_sfx_icestorm, 1, 1);
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
						stream->endTime = cl_common->serverTime * 0.001+0.3;
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
			CLHW_ParseMissileFlash(net_message);
			break;

		case TE_SUNSTAFF_CHEAP:
			{
				int				ent;
				vec3_t			points[4];
				int				reflect_count;
				short int		tempVal;
				h2entity_state_t	*state;
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
						stream->endTime = cl_common->serverTime * 0.001+0.5;	// FIXME
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
				h2entity_state_t	*state;

				ent = net_message.ReadShort();

				state = FindState(ent);

				if (state)
				{
					if(rand()&1)
					{
						S_StartSound(state->origin, CLH2_TempSoundChannel(), 0, cl_sfx_lightning1, 1, 1);
					}
					else
					{
						S_StartSound(state->origin, CLH2_TempSoundChannel(), 0, cl_sfx_lightning2, 1, 1);
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
						stream->endTime = cl_common->serverTime * 0.001+0.5;
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
			CLHW_ParseTeleport(net_message);
			break;
		case TE_SWORD_EXPLOSION:
			{
				vec3_t			pos;
				int				ent;
				h2stream_t		*stream;
				qhandle_t		models[2];
				h2entity_state_t	*state;

				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();
				ent = net_message.ReadShort();

				state = FindState(ent);

				if (state)
				{
					if(rand()&1)
					{
						S_StartSound(pos, CLH2_TempSoundChannel(), 0, cl_sfx_lightning1, 1, 1);
					}
					else
					{
						S_StartSound(pos, CLH2_TempSoundChannel(), 0, cl_sfx_lightning2, 1, 1);
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
						stream->endTime = cl_common->serverTime * 0.001+0.5;
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
				CLHW_SwordExplosion(pos);
			}
			break;

		case TE_AXE_BOUNCE:
			CLHW_ParseAxeBounce(net_message);
			break;
		case TE_AXE_EXPLODE:
			CLHW_ParseAxeExplode(net_message);
			break;
		case TE_TIME_BOMB:
			CLHW_ParseTimeBomb(net_message);
			break;
		case TE_FIREBALL:
			CLHW_ParseFireBall(net_message);
			break;

		case TE_SUNSTAFF_POWER:
			{
				int				ent;
				h2stream_t		*stream;
				qhandle_t		models[4];
				h2entity_state_t	*state;

				ent = net_message.ReadShort();

				vel[0] = net_message.ReadCoord();
				vel[1] = net_message.ReadCoord();
				vel[2] = net_message.ReadCoord();
				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();

				CLHW_SunStaffExplosions(pos);

				state = FindState(ent);

				if (state)
				{
					S_StartSound(state->origin, CLH2_TempSoundChannel(), 0, cl_sfx_sunstaff, 1, 1);

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
					stream->endTime = cl_common->serverTime * 0.001+0.8;
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
			CLHW_ParsePurify2Explode(net_message);
			break;
		case TE_PLAYER_DEATH:
			CLHW_ParsePlayerDeath(net_message);
			break;
		case TE_PURIFY1_EFFECT:
			CLHW_ParsePurify1Effect(net_message);
			break;
		case TE_TELEPORT_LINGER:
			CLHW_ParseTeleportLinger(net_message);
			break;
		case TE_LINE_EXPLOSION:
			CLHW_ParseLineExplosion(net_message);
			break;
		case TE_METEOR_CRUSH:
			CLHW_ParseMeteorCrush(net_message);
			break;
		case TE_ACIDBALL:
			CLHW_ParseAcidBall(net_message);
			break;
		case TE_ACIDBLOB:
			CLHW_ParseAcidBlob(net_message);
			break;
		case TE_FIREWALL:
			CLHW_ParseFireWall(net_message);
			break;
		case TE_FIREWALL_IMPACT:
			CLHW_ParseFireWallImpact(net_message);
			break;
		case TE_HWBONERIC:
			pos[0] = net_message.ReadCoord();
			pos[1] = net_message.ReadCoord();
			pos[2] = net_message.ReadCoord();
			cnt = net_message.ReadByte ();
			CLH2_RunParticleEffect4 (pos, 3, 368 + rand() % 16, pt_h2grav, cnt);
			rnd = rand() % 100;
			if (rnd > 95)
				S_StartSound(pos, CLH2_TempSoundChannel(), 0, cl_sfx_ric1, 1, 1);
			else if (rnd > 91)
				S_StartSound(pos, CLH2_TempSoundChannel(), 0, cl_sfx_ric2, 1, 1);
			else if (rnd > 87)
				S_StartSound(pos, CLH2_TempSoundChannel(), 0, cl_sfx_ric3, 1, 1);

			break;

		case TE_POWERFLAME:
			CLHW_ParsePowerFlame(net_message);
			break;
		case TE_BLOODRAIN:
			CLHW_ParseBloodRain(net_message);
			break;
		case TE_AXE:
			CLHW_ParseAxe(net_message);
			break;
		case TE_PURIFY2_MISSILE:
			CLHW_ParsePurify2Missile(net_message);
			break;
		case TE_SWORD_SHOT:
			CLHW_ParseSwordShot(net_message);
			break;
		case TE_ICESHOT:
			CLHW_ParseIceShot(net_message);
			break;
		case TE_METEOR:
			CLHW_ParseMeteor(net_message);
			break;
		case TE_LIGHTNINGBALL:
			CLHW_ParseLightningBall(net_message);
			break;
		case TE_MEGAMETEOR:
			CLHW_ParseMegaMeteor(net_message);
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
				h2entity_state_t	*state;
				h2entity_state_t	*state2;

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
					S_StartSound(source, CLH2_TempSoundChannel(), 0, cl_sfx_sunstaff, 1, 1);
					S_StartSound(dest, CLH2_TempSoundChannel(), 0, cl_sfx_sunhit, 1, 1);
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
				h2entity_state_t	*state;
				float			tempAng, tempPitch;

				ent = net_message.ReadShort();

				state = FindState(ent);
				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();

				if(rand()&1)
				{
					S_StartSound(pos, CLH2_TempSoundChannel(), 0, cl_sfx_lightning1, 1, 1);
				}
				else
				{
					S_StartSound(pos, CLH2_TempSoundChannel(), 0, cl_sfx_lightning2, 1, 1);
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
					stream->endTime = cl_common->serverTime * 0.001+0.5;
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
			CLHW_ParseAcidBallFly(net_message);
			break;
		case TE_ACID_BLOB_FLY:
			CLHW_ParseAcidBlobFly(net_message);
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
					stream->endTime = cl_common->serverTime * 0.001+0.3;
					stream->lastTrailTime = 0;

					VectorCopy(points[temp], stream->source);
					VectorCopy(points[temp + 1], stream->dest);

					CLHW_ChainLightningExplosion(points[temp + 1]);
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
	h2explosion_t	*ex;

	for (i=0, ex=clh2_explosions ; i< H2MAX_EXPLOSIONS ; i++, ex++)
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
		if (ex->endTime <= cl_common->serverTime * 0.001)
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
			f = (R_ModelNumFrames(ex->model) - 1) * (cl_common->serverTime * 0.001 - ex->startTime) / (ex->endTime - ex->startTime);
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
		if (ex->startTime > cl_common->serverTime * 0.001)
			continue;

		if (ex->frameFunc)
		{
			ex->frameFunc(ex);
		}

		// allow for the possibility for the frame func to reset startTime
		if (ex->startTime > cl_common->serverTime * 0.001)
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
		CLH2_SetRefEntAxis(&ent, ex->angles, vec3_origin, ex->scale, 0, ex->abslight, ex->flags);
		CLH2_HandleCustomSkin(&ent, -1);
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
	int				segmentCount;
	int				offset;
	h2entity_state_t	*state;

	// Update streams
	for(i = 0, stream = clh2_Streams; i < MAX_STREAMS_H2; i++, stream++)
	{
		if(!stream->models[0])// || stream->endTime < cl.time)
		{ // Inactive
			continue;
		}

		if(stream->endTime < cl_common->serverTime * 0.001)
		{ // Inactive
			if(stream->type!=TE_STREAM_LIGHTNING&&stream->type!=TE_STREAM_LIGHTNING_SMALL)
				continue;
			else if(stream->endTime + 0.25 < cl_common->serverTime * 0.001)
				continue;
		}

		if(stream->flags&H2STREAM_ATTACHED&&stream->endTime >= cl_common->serverTime * 0.001)
			{ // Attach the start position to owner
			state = FindState(stream->entity);
			if (state)
			{
				VectorAdd(state->origin, stream->offset, stream->source);
			}
		}

		VectorSubtract(stream->dest, stream->source, dist);
		vec3_t angles;
		VecToAnglesBuggy(dist, angles);

		VectorCopy(stream->source, org);
		d = VectorNormalize(dist);

		if(stream->type == TE_STREAM_SUNSTAFF2)
		{
			AngleVectors(angles, discard, right, up);

			lifeTime = ((stream->endTime - cl_common->serverTime * 0.001)/.8);
			cosTime = cos(cl_common->serverTime * 0.001*5);
			sinTime = sin(cl_common->serverTime * 0.001*5);
			cos2Time = cos(cl_common->serverTime * 0.001*5 + 3.14);
			sin2Time = sin(cl_common->serverTime * 0.001*5 + 3.14);
		}

		segmentCount = 0;
		if(stream->type == TE_STREAM_ICECHUNKS)
		{
			offset = (int)(cl_common->serverTime * 0.001*40)%30;
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
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_SUNSTAFF1:
				angles[2] = (int)(cl_common->serverTime * 0.001*10)%360;
				//ent->frame = (int)(cl.time*20)%20;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 50 + 100 * ((stream->endTime - cl_common->serverTime * 0.001)/.3), 0, 128, H2MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);

				Com_Memset(&ent, 0, sizeof(ent));
				ent.reType = RT_MODEL;
				VectorCopy(org, ent.origin);
				ent.hModel = stream->models[1];
				angles[2] = (int)(cl_common->serverTime * 0.001*50)%360;
				//stream->endTime = cl.time+0.3;	// FIXME
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 50 + 100 * ((stream->endTime - cl_common->serverTime * 0.001)/.5), 0, 128, H2MLS_ABSLIGHT|H2DRF_TRANSLUCENT);
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_SUNSTAFF2:
				angles[2] = (int)(cl_common->serverTime * 0.001*100)%360;
				VectorMA(ent.origin, cosTime * (40 * lifeTime), right,  ent.origin);
				VectorMA(ent.origin, sinTime * (40 * lifeTime), up,  ent.origin);
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 100 + 150 * lifeTime, 0, 128, H2MLS_ABSLIGHT|H2DRF_TRANSLUCENT);
				R_AddRefEntityToScene(&ent);

				Com_Memset(&ent, 0, sizeof(ent));
				ent.reType = RT_MODEL;
				VectorCopy(org, ent.origin);
				ent.hModel = stream->models[0];
				angles[2] = (int)(cl_common->serverTime * 0.001*100)%360;
				VectorMA(ent.origin, cos2Time * (40 * lifeTime), right,  ent.origin);
				VectorMA(ent.origin, sin2Time * (40 * lifeTime), up,  ent.origin);
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 100 + 150 * lifeTime, 0, 128, H2MLS_ABSLIGHT|H2DRF_TRANSLUCENT);
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
					angles[2] = (int)(cl_common->serverTime * 0.001*20)%360;
					CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 100 + 150 * lifeTime, 0, 128, H2MLS_ABSLIGHT);
					R_AddRefEntityToScene(&ent);
				}
				break;
			case TE_STREAM_LIGHTNING:
				if(stream->endTime < cl_common->serverTime * 0.001)
				{//fixme: keep last non-translucent frame and angle
					CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128 + (stream->endTime - cl_common->serverTime * 0.001)*192, H2MLS_ABSLIGHT|H2DRF_TRANSLUCENT);
				}
				else
				{
					angles[2] = rand()%360;
					ent.frame = rand()%6;
					CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				}
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_LIGHTNING_SMALL:
				if(stream->endTime < cl_common->serverTime * 0.001)
				{
					CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128 + (stream->endTime - cl_common->serverTime * 0.001)*192, H2MLS_ABSLIGHT|H2DRF_TRANSLUCENT);
				}
				else
				{
					angles[2] = rand()%360;
					ent.frame = rand()%6;
					CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				}
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_FAMINE:
				angles[2] = rand()%360;
				ent.frame = 0;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_COLORBEAM:
				angles[2] = 0;
				ent.skinNum = stream->skin;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				CLH2_HandleCustomSkin(&ent, -1);
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_GAZE:
				angles[2] = 0;
				ent.frame = (int)(cl_common->serverTime * 0.001*40)%36;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_ICECHUNKS:
				angles[2] = rand()%360;
				ent.frame = rand()%5;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			default:
				angles[2] = 0;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 0, 0);
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
			if(stream->lastTrailTime+0.2 < cl_common->serverTime * 0.001)
			{
				stream->lastTrailTime = cl_common->serverTime * 0.001;
				CLH2_SunStaffTrail(stream->source, stream->dest);
			}

			refEntity_t ent;
			Com_Memset(&ent, 0, sizeof(ent));
			ent.reType = RT_MODEL;
			VectorCopy(stream->dest, ent.origin);
			ent.hModel = stream->models[2];
			//ent->frame = (int)(cl.time*20)%20;
			CLH2_SetRefEntAxis(&ent, vec3_origin, vec3_origin, 80 + (rand() & 15), 0, 128, H2MLS_ABSLIGHT);
			R_AddRefEntityToScene(&ent);

			Com_Memset(&ent, 0, sizeof(ent));
			ent.reType = RT_MODEL;
			VectorCopy(stream->dest, ent.origin);
			ent.hModel = stream->models[3];
			CLH2_SetRefEntAxis(&ent, vec3_origin, vec3_origin, 150 + (rand() & 15), 0, 128, H2MLS_ABSLIGHT|H2DRF_TRANSLUCENT);
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

void CreateExplosionWithSound(vec3_t pos)
{
	h2explosion_t	*ex;

	ex = CLH2_AllocExplosion ();
	VectorCopy(pos, ex->origin);
	ex->startTime = cl_common->serverTime * 0.001;
	ex->endTime = ex->startTime + HX_FRAME_TIME * 10;
	ex->model = R_RegisterModel("models/sm_expld.spr");
	
	S_StartSound(pos, CLH2_TempSoundChannel(), 1, clh2_sfx_explode, 1, 1);
}

void purify1Update(h2explosion_t *ex)
{
	CLH2_TrailParticles (ex->oldorg, ex->origin, rt_purify);
	//CLH2_TrailParticles (ex->oldorg, ex->origin, rt_setstaff);
}

//*****************************************************************************
//
//						FLAG UPDATE FUNCTIONS
//
//*****************************************************************************

void CL_UpdatePoisonGas(refEntity_t *ent, vec3_t angles, int edict_num)
{
	h2explosion_t	*ex;
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

		ex = CLH2_AllocExplosion();
		VectorCopy(ent->origin, ex->origin);
		ex->model = R_RegisterModel("models/grnsmk1.spr");
		ex->startTime = cl_common->serverTime * 0.001;
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

		ex->flags |= H2DRF_TRANSLUCENT | H2SCALE_ORIGIN_CENTER;

		smokeCount -= 1.0;
	}
}

void CL_UpdateAcidBlob(refEntity_t *ent, vec3_t angles, int edict_num)
{
	h2explosion_t	*ex;
	int testVal, testVal2;

	testVal = (int)(cl_common->serverTime * 0.001 * 10.0);
	testVal2 = (int)((cl_common->serverTime * 0.001 - host_frametime)*10.0);

	if(testVal != testVal2)
	{
		if(!(testVal%2))
		{
			ex = CLH2_AllocExplosion();
			VectorCopy(ent->origin, ex->origin);
			ex->model = R_RegisterModel("models/muzzle1.spr");
			ex->startTime = cl_common->serverTime * 0.001;
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

			ex->flags |= H2DRF_TRANSLUCENT | H2SCALE_ORIGIN_CENTER;
		}
	}
}

void CL_UpdateOnFire(refEntity_t *ent, vec3_t angles, int edict_num)
{
	h2explosion_t *ex;

	if((rand()%100)/100.0 < 5.0 * host_frametime)
	{
		ex = CLH2_AllocExplosion();
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
		
		ex->startTime = cl_common->serverTime * 0.001;
		ex->endTime = ex->startTime + R_ModelNumFrames(ex->model) * 0.05;

		ex->scale = 100;

		VectorCopy(angles, ex->angles);
		ex->angles[2] += 90;

		ex->velocity[0] = (rand()%40)-20;
		ex->velocity[1] = (rand()%40)-20;
		ex->velocity[2] = 50 + (rand()%100);

		if(rand()%5)//translucent 80% of the time
			ex->flags |= H2DRF_TRANSLUCENT;
	}
}

void PowerFlameBurnRemove(h2explosion_t *ex)
{
	h2explosion_t *ex2;

	ex2 = CLH2_AllocExplosion();
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
	ex2->startTime = cl_common->serverTime * 0.001;
	ex2->endTime = ex2->startTime + R_ModelNumFrames(ex2->model) * 0.05;

	ex2->scale = 100;

	ex2->flags |= H2MLS_ABSLIGHT|H2DRF_TRANSLUCENT;
	ex2->abslight = 128;

	
	if(rand()&1)S_StartSound(ex2->origin, CLH2_TempSoundChannel(), 1, clh2_sfx_flameend, 1, 1);
}

void CL_UpdatePowerFlameBurn(refEntity_t *ent, int edict_num)
{
	h2explosion_t *ex, *ex2;
	vec3_t		srcVec;

	if((rand()%100)/100.0 < host_frametime * 10)
	{
		ex = CLH2_AllocExplosion();
		VectorCopy(ent->origin, ex->origin);
		ex->origin[0] += (rand()%120)-60;
		ex->origin[1] += (rand()%120)-60;
		ex->origin[2] += (rand()%120)-60+120;
		ex->model = R_RegisterModel("models/sucwp1p.mdl");
		ex->startTime = cl_common->serverTime * 0.001;
		ex->endTime = ex->startTime + .25;
		ex->removeFunc = PowerFlameBurnRemove;

		ex->scale = 100;

		VectorSubtract(ent->origin, ex->origin, srcVec);
		VectorCopy(srcVec, ex->velocity);
		ex->velocity[2] += 24;
		VectorScale(ex->velocity, 1.0 / .25, ex->velocity);
		VectorNormalize(srcVec);
		VecToAnglesBuggy(srcVec, ex->angles);

		ex->flags |= H2MLS_ABSLIGHT;//|H2DRF_TRANSLUCENT;
		ex->abslight = 128;


		// I'm not seeing this right now... (?)
		ex2 = CLH2_AllocExplosion();
		VectorCopy(ex->origin, ex2->origin);
		ex2->model = R_RegisterModel("models/flamestr.spr");
		ex2->startTime = cl_common->serverTime * 0.001;
		ex2->endTime = ex2->startTime + R_ModelNumFrames(ex2->model) * 0.05;
		ex2->flags |= H2DRF_TRANSLUCENT;
	}
}

void CL_UpdateHammer(refEntity_t *ent, int edict_num)
{
	int testVal, testVal2;
	// do this every .3 seconds
	testVal = (int)(cl_common->serverTime * 0.001 * 10.0);
	testVal2 = (int)((cl_common->serverTime * 0.001 - host_frametime)*10.0);

	if(testVal != testVal2)
	{
		if(!(testVal%3))
		{
			//S_StartSound(ent->origin, CLH2_TempSoundChannel(), 1, cl_sfx_hammersound, 1, 1);
			S_StartSound(ent->origin, edict_num, 2, cl_sfx_hammersound, 1, 1);
		}
	}
}

void CL_UpdateBug(refEntity_t *ent)
{
	int testVal, testVal2;
	testVal = (int)(cl_common->serverTime * 0.001 * 10.0);
	testVal2 = (int)((cl_common->serverTime * 0.001 - host_frametime)*10.0);

	if(testVal != testVal2)
	{
	// do this every .1 seconds
//		if(!(testVal%3))
//		{
			S_StartSound(ent->origin, CLH2_TempSoundChannel(), 1, cl_sfx_buzzbee, 1, 1);
//		}
	}
}

void CL_UpdateIceStorm(refEntity_t *ent, int edict_num)
{
	vec3_t			center, side1;
	vec3_t			side2 = {160, 160, 128};
	h2entity_state_t	*state;
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
			S_StartSound(center, CLH2_TempSoundChannel(), 0, cl_sfx_icestorm, 1, 1);
			playIceSound -= .6;
		}
	}

	// toss little ice chunks
	if(rand()%100 < host_frametime * 100.0 * 3)
	{
		CLHW_CreateIceChunk(ent->origin);
	}
}

void CL_UpdateTargetBall(void)
{
	int i;
	h2explosion_t *ex1 = NULL;
	h2explosion_t *ex2 = NULL;
	qhandle_t	iceMod;
	vec3_t		newOrg;
	float		newScale;

	if (v_targDist < 24)
	{	// either there is no ball, or it's too close to be needed...
		return;
	}

	// search for the two thingies.  If they don't exist, make new ones and set v_oldTargOrg

	iceMod = R_RegisterModel("models/iceshot2.mdl");

	for(i = 0; i < H2MAX_EXPLOSIONS; i++)
	{
		if(clh2_explosions[i].endTime > cl_common->serverTime * 0.001)
		{	// make certain it's an active one
			if(clh2_explosions[i].model == iceMod)
			{
				if(clh2_explosions[i].flags & H2DRF_TRANSLUCENT)
				{
					ex1 = &clh2_explosions[i];
				}
				else
				{
					ex2 = &clh2_explosions[i];
				}
			}
		}
	}

	VectorCopy(cl.simorg, newOrg);
	newOrg[0] += cos(v_targAngle*M_PI*2/256.0) * 50 * cos(v_targPitch*M_PI*2/256.0);
	newOrg[1] += sin(v_targAngle*M_PI*2/256.0) * 50 * cos(v_targPitch*M_PI*2/256.0);
	newOrg[2] += 44 + sin(v_targPitch*M_PI*2/256.0) * 50 + cos(cl_common->serverTime * 0.001*2)*5;
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
		ex1 = CLH2_AllocExplosion();
		ex1->model = iceMod;
		ex1->exflags |= EXFLAG_STILL_FRAME;
		ex1->data = 0;

		ex1->flags |= H2MLS_ABSLIGHT|H2DRF_TRANSLUCENT;
		ex1->skin = 0;
		VectorCopy(newOrg, ex1->origin);
		ex1->scale = newScale;
	}

	VectorScale(ex1->origin, (.75 - host_frametime * 1.5), ex1->origin);	// FIXME this should be affected by frametime...
	VectorMA(ex1->origin, (.25 + host_frametime * 1.5), newOrg, ex1->origin);
	ex1->startTime = cl_common->serverTime * 0.001;
	ex1->endTime = ex1->startTime + host_frametime + 0.2;
	ex1->scale = (ex1->scale * (.75 - host_frametime * 1.5) + newScale * (.25 + host_frametime * 1.5));
	ex1->angles[0] = v_targPitch*360/256.0;
	ex1->angles[1] = v_targAngle*360/256.0;
	ex1->angles[2] = cl_common->serverTime * 0.001 * 240;
	ex1->abslight = 96 + (32 * cos(cl_common->serverTime * 0.001*6.5)) + (64 * ((256.0 - v_targDist)/256.0));

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
		ex2 = CLH2_AllocExplosion();
		ex2->model = iceMod;
		ex2->exflags |= EXFLAG_STILL_FRAME;
		ex2->data = 0;

		ex2->flags |= H2MLS_ABSLIGHT;
		ex2->skin = 0;
		ex2->scale = newScale;
	}
	VectorCopy(ex1->origin, ex2->origin);
	ex2->startTime = cl_common->serverTime * 0.001;
	ex2->endTime = ex2->startTime + host_frametime + 0.2;
	ex2->scale = (ex2->scale * (.75 - host_frametime * 1.5) + newScale * (.25 + host_frametime * 1.5));
	ex2->angles[0] = ex1->angles[0];
	ex2->angles[1] = ex1->angles[1];
	ex2->angles[2] = cl_common->serverTime * 0.001 * -360;
	ex2->abslight = 96 + (128 * cos(cl_common->serverTime * 0.001*4.5));

	CLHW_TargetBallEffectParticles (ex1->origin, v_targDist);
}
