
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

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
// CL_InitTEnts
//
//==========================================================================

void CL_InitTEnts(void)
{
	CLH2_InitTEntsCommon();
}

//==========================================================================
//
// CL_ParseTEnt
//
//==========================================================================

void CL_ParseTEnt()
{
	int type = net_message.ReadByte();
	switch (type)
	{
	case TE_WIZSPIKE:	// spike hitting wall
		CLH2_ParseWizSpike(net_message);
		break;
	case TE_KNIGHTSPIKE:	// spike hitting wall
	case TE_GUNSHOT:		// bullet hitting wall
		CLH2_ParseKnightSpike(net_message);
		break;
	case TE_SPIKE:			// spike hitting wall
		CLH2_ParseSpike(net_message);
		break;
	case TE_SUPERSPIKE:			// super spike hitting wall
		CLH2_ParseSuperSpike(net_message);
		break;
	case TE_EXPLOSION:			// rocket explosion
		CLH2_ParseExplosion(net_message);
		break;
	case TE_LIGHTNING1:
	case TE_LIGHTNING2:
	case TE_LIGHTNING3:
		CLH2_ParseBeam(net_message);
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
		CLH2_ParseLavaSplash(net_message);
		break;
	case TE_TELEPORT:
		CLH2_ParseTeleport(net_message);
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
	qhandle_t models[4];

	ent = net_message.ReadShort();
	flags = net_message.ReadByte();
	tag = flags&15;
	int duration = net_message.ReadByte() * 50;
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

	if((stream = CLH2_NewStream(ent, tag)) == NULL)
	{
		Con_Printf("stream list overflow\n");
		return;
	}
	CLH2_InitStream(stream, type, ent, tag, flags, skin, duration, source, dest, models);
	if (flags & H2STREAM_ATTACHED)
	{
		VectorSubtract(source, cl_entities[ent].state.origin, stream->offset);
	}
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
			VectorAdd(cl_entities[stream->entity].state.origin, stream->offset,
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
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_SUNSTAFF1:
				angles[2] = (int)(cl.serverTimeFloat*10)%360;
				//ent->frame = (int)(cl.time*20)%20;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);

				Com_Memset(&ent, 0, sizeof(ent));
				ent.reType = RT_MODEL;
				VectorCopy(org, ent.origin);
				ent.hModel = stream->models[1];
				angles[2] = (int)(cl.serverTimeFloat*50)%360;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT|H2DRF_TRANSLUCENT);
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_SUNSTAFF2:
				angles[2] = (int)(cl.serverTimeFloat*10)%360;
				ent.frame = (int)(cl.serverTimeFloat*10)%8;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_LIGHTNING:
				if (stream->endTime < cl.serverTimeFloat)
				{//fixme: keep last non-translucent frame and angle
					angles[2] = 0;
					CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128 + (stream->endTime - cl.serverTimeFloat) * 192, H2MLS_ABSLIGHT|H2DRF_TRANSLUCENT);
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
				if (stream->endTime < cl.serverTimeFloat)
				{
					angles[2] = 0;
					CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128 + (stream->endTime - cl.serverTimeFloat)*192, H2MLS_ABSLIGHT|H2DRF_TRANSLUCENT);
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
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_GAZE:
				angles[2] = 0;
				ent.frame = (int)(cl.serverTimeFloat*40)%36;
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
			CLH2_SetRefEntAxis(&ent, vec3_origin, vec3_origin, 80 + (rand() & 15), 0, 128, H2MLS_ABSLIGHT);
			//ent->frame = (int)(cl.time*20)%20;
			R_AddRefEntityToScene(&ent);

			Com_Memset(&ent, 0, sizeof(ent));
			ent.reType = RT_MODEL;
			VectorCopy(stream->dest, ent.origin);
			ent.hModel = stream->models[3];
			CLH2_SetRefEntAxis(&ent, vec3_origin, vec3_origin, 150 + (rand() & 15), 0, 128, H2MLS_ABSLIGHT | H2DRF_TRANSLUCENT);
			R_AddRefEntityToScene(&ent);
		}
	}
}
