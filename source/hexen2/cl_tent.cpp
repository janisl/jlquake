
//**************************************************************************
//**
//** cl_tent.c
//**
//** Client side temporary entity effects.
//**
//** $Header: /H2 Mission Pack/CL_TENT.C 6     3/02/98 2:51p Jweier $
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "quakedef.h"

// MACROS ------------------------------------------------------------------

#define	TE_SPIKE				0
#define	TE_SUPERSPIKE			1
#define	TE_GUNSHOT				2
#define	TE_EXPLOSION			3
//#define	TE_TAREXPLOSION			4
#define	TE_LIGHTNING1			5
#define	TE_LIGHTNING2			6
#define	TE_WIZSPIKE				7
#define	TE_KNIGHTSPIKE			8
#define	TE_LIGHTNING3			9
#define	TE_LAVASPLASH			10
#define	TE_TELEPORT				11
//#define TE_EXPLOSION2			12
#define TE_STREAM_CHAIN			25
#define TE_STREAM_SUNSTAFF1		26
#define TE_STREAM_SUNSTAFF2		27
#define TE_STREAM_LIGHTNING		28
#define TE_STREAM_COLORBEAM		29
#define TE_STREAM_ICECHUNKS		30
#define TE_STREAM_GAZE			31
#define TE_STREAM_FAMINE		32
#define TE_STREAM_LIGHTNING_SMALL		24

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void ParseStream(int type);
static h2stream_t *NewStream(int ent, int tag);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static sfxHandle_t cl_sfx_tink1;
static sfxHandle_t cl_sfx_ric1;
static sfxHandle_t cl_sfx_ric2;
static sfxHandle_t cl_sfx_ric3;
static sfxHandle_t cl_sfx_r_exp3;

// CODE --------------------------------------------------------------------

//==========================================================================
//
// CL_InitTEnts
//
//==========================================================================

void CL_InitTEnts(void)
{
	cl_sfx_tink1 = S_RegisterSound("weapons/tink1.wav");
	cl_sfx_ric1 = S_RegisterSound("weapons/ric1.wav");
	cl_sfx_ric2 = S_RegisterSound("weapons/ric2.wav");
	cl_sfx_ric3 = S_RegisterSound("weapons/ric3.wav");
	cl_sfx_r_exp3 = S_RegisterSound("weapons/r_exp3.wav");
}

//==========================================================================
//
// CL_ClearTEnts
//
//==========================================================================

void CL_ClearTEnts(void)
{
	CLH2_ClearStreams();
}

//==========================================================================
//
// CL_ParseTEnt
//
//==========================================================================

void CL_ParseTEnt(void)
{
	int type;
	vec3_t pos;
	int rnd;

	type = net_message.ReadByte();
	switch(type)
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
			S_StartSound(pos, -1, 0, cl_sfx_tink1, 1, 1);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				S_StartSound(pos, -1, 0, cl_sfx_ric1, 1, 1);
			else if (rnd == 2)
				S_StartSound(pos, -1, 0, cl_sfx_ric2, 1, 1);
			else
				S_StartSound(pos, -1, 0, cl_sfx_ric3, 1, 1);
		}
		break;
	case TE_SUPERSPIKE:			// super spike hitting wall
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		CLH2_RunParticleEffect (pos, vec3_origin, 20);

		if ( rand() % 5 )
			S_StartSound(pos, -1, 0, cl_sfx_tink1, 1, 1);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				S_StartSound(pos, -1, 0, cl_sfx_ric1, 1, 1);
			else if (rnd == 2)
				S_StartSound(pos, -1, 0, cl_sfx_ric2, 1, 1);
			else
				S_StartSound(pos, -1, 0, cl_sfx_ric3, 1, 1);
		}
		break;
		
	case TE_GUNSHOT:			// bullet hitting wall
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		CLH2_RunParticleEffect (pos, vec3_origin, 20);
		break;
		
	case TE_EXPLOSION:			// rocket explosion
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		CLH2_ParticleExplosion (pos);
		break;
	case TE_LIGHTNING1:
	case TE_LIGHTNING2:
	case TE_LIGHTNING3:
		net_message.ReadShort();
		net_message.ReadCoord();
		net_message.ReadCoord();
		net_message.ReadCoord();
		net_message.ReadCoord();
		net_message.ReadCoord();
		net_message.ReadCoord();
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
	default:
		Sys_Error ("CL_ParseTEnt: bad type");
	}
}

//==========================================================================
//
// ParseStream
//
//==========================================================================

static void ParseStream(int type)
{
	int ent;
	int tag;
	int flags;
	int skin;
	vec3_t source;
	vec3_t dest;
	h2stream_t *stream;
	float duration;
	qhandle_t models[4];

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
//		duration*=2;
		break;
	case TE_STREAM_LIGHTNING_SMALL:
		models[0] = R_RegisterModel("models/stltng2.mdl");
//		duration*=2;
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

	if((stream = NewStream(ent, tag)) == NULL)
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
		VectorSubtract(source, cl_entities[ent].origin, stream->offset);
	}
}

//==========================================================================
//
// NewStream
//
//==========================================================================

static h2stream_t *NewStream(int ent, int tag)
{
	int i;
	h2stream_t *stream;

	// Search for a stream with matching entity and tag
	for(i = 0, stream = clh2_Streams; i < MAX_STREAMS_H2; i++, stream++)
	{
		if(stream->entity == ent && stream->tag == tag)
		{
			return stream;
		}
	}
	// Search for a free stream
	for(i = 0, stream = clh2_Streams; i < MAX_STREAMS_H2; i++, stream++)
	{
		if(!stream->models[0] || stream->endTime < cl.serverTimeFloat)
		{
			return stream;
		}
	}
	return NULL;
}

//==========================================================================
//
// CL_UpdateTEnts
//
//==========================================================================

void CL_UpdateTEnts(void)
{
	int i;
	h2stream_t *stream;
	vec3_t dist;
	vec3_t org;
	float d;
	int segmentCount;
	int offset;

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
			VectorAdd(cl_entities[stream->entity].origin, stream->offset,
				stream->source);
		}

		VectorSubtract(stream->dest, stream->source, dist);
		vec3_t angles;
		VecToAnglesBuggy(dist, angles);

		VectorCopy(stream->source, org);
		d = VectorNormalize(dist);
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
				CL_SetRefEntAxis(&ent, angles, 0, 0, 128, MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_SUNSTAFF1:
				angles[2] = (int)(cl.serverTimeFloat*10)%360;
				//ent->frame = (int)(cl.time*20)%20;
				CL_SetRefEntAxis(&ent, angles, 0, 0, 128, MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);

				Com_Memset(&ent, 0, sizeof(ent));
				ent.reType = RT_MODEL;
				VectorCopy(org, ent.origin);
				ent.hModel = stream->models[1];
				angles[2] = (int)(cl.serverTimeFloat*50)%360;
				CL_SetRefEntAxis(&ent, angles, 0, 0, 128, MLS_ABSLIGHT|DRF_TRANSLUCENT);
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_SUNSTAFF2:
				angles[2] = (int)(cl.serverTimeFloat*10)%360;
				ent.frame = (int)(cl.serverTimeFloat*10)%8;
				CL_SetRefEntAxis(&ent, angles, 0, 0, 128, MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_LIGHTNING:
				if (stream->endTime < cl.serverTimeFloat)
				{//fixme: keep last non-translucent frame and angle
					angles[2] = 0;
					CL_SetRefEntAxis(&ent, angles, 0, 0, 128 + (stream->endTime - cl.serverTimeFloat) * 192, MLS_ABSLIGHT|DRF_TRANSLUCENT);
				}
				else
				{
					angles[2] = rand()%360;
					ent.frame = rand()%6;
					CL_SetRefEntAxis(&ent, angles, 0, 0, 128, MLS_ABSLIGHT);
				}
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_LIGHTNING_SMALL:
				if (stream->endTime < cl.serverTimeFloat)
				{
					angles[2] = 0;
					CL_SetRefEntAxis(&ent, angles, 0, 0, 128 + (stream->endTime - cl.serverTimeFloat)*192, MLS_ABSLIGHT|DRF_TRANSLUCENT);
				}
				else
				{
					angles[2] = rand()%360;
					ent.frame = rand()%6;
					CL_SetRefEntAxis(&ent, angles, 0, 0, 128, MLS_ABSLIGHT);
				}
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_FAMINE:
				angles[2] = rand()%360;
				ent.frame = 0;
				CL_SetRefEntAxis(&ent, angles, 0, 0, 128, MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_COLORBEAM:
				angles[2] = 0;
				ent.skinNum = stream->skin;
				CL_SetRefEntAxis(&ent, angles, 0, 0, 128, MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_GAZE:
				angles[2] = 0;
				ent.frame = (int)(cl.serverTimeFloat*40)%36;
				CL_SetRefEntAxis(&ent, angles, 0, 0, 128, MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_ICECHUNKS:
				angles[2] = rand()%360;
				ent.frame = rand()%5;
				CL_SetRefEntAxis(&ent, angles, 0, 0, 128, MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			default:
				angles[2] = 0;
				CL_SetRefEntAxis(&ent, angles, 0, 0, 0, 0);
				R_AddRefEntityToScene(&ent);
			}
			for(i = 0; i < 3; i++)
			{
				org[i] += dist[i]*30;
			}
			d -= 30;
			segmentCount++;
		}
		if (stream->type == TE_STREAM_SUNSTAFF1 ||
			stream->type == TE_STREAM_SUNSTAFF2)
		{
			if (stream->lastTrailTime + 0.2 < cl.serverTimeFloat)
			{
				stream->lastTrailTime = cl.serverTimeFloat;
				CLH2_SunStaffTrail(stream->source, stream->dest);
			}

			refEntity_t ent;
			Com_Memset(&ent, 0, sizeof(ent));
			ent.reType = RT_MODEL;
			VectorCopy(stream->dest, ent.origin);
			ent.hModel = stream->models[2];
			CL_SetRefEntAxis(&ent, vec3_origin, 80 + (rand() & 15), 0, 128, MLS_ABSLIGHT);
			//ent->frame = (int)(cl.time*20)%20;
			R_AddRefEntityToScene(&ent);

			Com_Memset(&ent, 0, sizeof(ent));
			ent.reType = RT_MODEL;
			VectorCopy(stream->dest, ent.origin);
			ent.hModel = stream->models[3];
			CL_SetRefEntAxis(&ent, vec3_origin, 150 + (rand() & 15), 0, 128, MLS_ABSLIGHT | DRF_TRANSLUCENT);
			R_AddRefEntityToScene(&ent);
		}
	}
}
