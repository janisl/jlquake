
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
	case H2TE_WIZSPIKE:	// spike hitting wall
		CLH2_ParseWizSpike(net_message);
		break;
	case H2TE_KNIGHTSPIKE:	// spike hitting wall
	case H2TE_GUNSHOT:		// bullet hitting wall
		CLH2_ParseKnightSpike(net_message);
		break;
	case H2TE_SPIKE:			// spike hitting wall
		CLH2_ParseSpike(net_message);
		break;
	case H2TE_SUPERSPIKE:			// super spike hitting wall
		CLH2_ParseSuperSpike(net_message);
		break;
	case H2TE_EXPLOSION:			// rocket explosion
		CLH2_ParseExplosion(net_message);
		break;
	case H2TE_LIGHTNING1:
	case H2TE_LIGHTNING2:
	case H2TE_LIGHTNING3:
		CLH2_ParseBeam(net_message);
		break;

	case H2TE_STREAM_CHAIN:
	case H2TE_STREAM_SUNSTAFF1:
	case H2TE_STREAM_SUNSTAFF2:
	case H2TE_STREAM_LIGHTNING:
	case H2TE_STREAM_LIGHTNING_SMALL:
	case H2TE_STREAM_COLORBEAM:
	case H2TE_STREAM_ICECHUNKS:
	case H2TE_STREAM_GAZE:
	case H2TE_STREAM_FAMINE:
		ParseStream(type);
		break;

	case H2TE_LAVASPLASH:
		CLH2_ParseLavaSplash(net_message);
		break;
	case H2TE_TELEPORT:
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

	ent = net_message.ReadShort();
	flags = net_message.ReadByte();
	tag = flags&15;
	int duration = net_message.ReadByte() * 50;
	skin = 0;
	if(type == H2TE_STREAM_COLORBEAM)
	{
		skin = net_message.ReadByte();
	}
	source[0] = net_message.ReadCoord();
	source[1] = net_message.ReadCoord();
	source[2] = net_message.ReadCoord();
	dest[0] = net_message.ReadCoord();
	dest[1] = net_message.ReadCoord();
	dest[2] = net_message.ReadCoord();

	switch (type)
	{
	case H2TE_STREAM_CHAIN:
		CLH2_CreateStreamChain(ent, tag, flags, skin, duration, source, dest);
		break;
	case H2TE_STREAM_SUNSTAFF1:
		CLH2_CreateStreamSunstaff1(ent, tag, flags, skin, duration, source, dest);
		break;
	case H2TE_STREAM_SUNSTAFF2:
		CLH2_CreateStreamSunstaff2(ent, tag, flags, skin, duration, source, dest);
		break;
	case H2TE_STREAM_LIGHTNING:
		CLH2_CreateStreamLightning(ent, tag, flags, skin, duration, source, dest);
		break;
	case H2TE_STREAM_LIGHTNING_SMALL:
		CLH2_CreateStreamLightningSmall(ent, tag, flags, skin, duration, source, dest);
		break;
	case H2TE_STREAM_FAMINE:
		CLH2_CreateStreamFaMine(ent, tag, flags, skin, duration, source, dest);
		break;
	case H2TE_STREAM_COLORBEAM:
		CLH2_CreateStreamColourBeam(ent, tag, flags, skin, duration, source, dest);
		break;
	case H2TE_STREAM_ICECHUNKS:
		CLH2_CreateStreamIceChunks(ent, tag, flags, skin, duration, source, dest);
		break;
	case H2TE_STREAM_GAZE:
		CLH2_CreateStreamGaze(ent, tag, flags, skin, duration, source, dest);
		break;
	default:
		Sys_Error("ParseStream: bad type");
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
			if(stream->type!=H2TE_STREAM_LIGHTNING&&stream->type!=H2TE_STREAM_LIGHTNING_SMALL)
				continue;
			else if(stream->endTime + 0.25 < cl.serverTimeFloat)
				continue;
		}

		if(stream->flags&H2STREAM_ATTACHED&&stream->endTime >= cl.serverTimeFloat)
		{ // Attach the start position to owner
			VectorAdd(h2cl_entities[stream->entity].state.origin, stream->offset,
				stream->source);
		}

		VectorSubtract(stream->dest, stream->source, dist);
		vec3_t angles;
		VecToAnglesBuggy(dist, angles);

		VectorCopy(stream->source, org);
		d = VectorNormalize(dist);
		segmentCount = 0;
		if(stream->type == H2TE_STREAM_ICECHUNKS)
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
			case H2TE_STREAM_CHAIN:
				angles[2] = 0;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			case H2TE_STREAM_SUNSTAFF1:
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
			case H2TE_STREAM_SUNSTAFF2:
				angles[2] = (int)(cl.serverTimeFloat*10)%360;
				ent.frame = (int)(cl.serverTimeFloat*10)%8;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			case H2TE_STREAM_LIGHTNING:
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
			case H2TE_STREAM_LIGHTNING_SMALL:
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
			case H2TE_STREAM_FAMINE:
				angles[2] = rand()%360;
				ent.frame = 0;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			case H2TE_STREAM_COLORBEAM:
				angles[2] = 0;
				ent.skinNum = stream->skin;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			case H2TE_STREAM_GAZE:
				angles[2] = 0;
				ent.frame = (int)(cl.serverTimeFloat*40)%36;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			case H2TE_STREAM_ICECHUNKS:
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
		if (stream->type == H2TE_STREAM_SUNSTAFF1 ||
			stream->type == H2TE_STREAM_SUNSTAFF2)
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
