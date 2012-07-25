//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../../client.h"
#include "local.h"

#define MAX_STREAMS_H2              32

struct h2stream_t
{
	int type;
	int entity;
	int tag;
	int flags;
	int skin;
	qhandle_t models[4];
	vec3_t source;
	vec3_t dest;
	vec3_t offset;
	float endTime;
	float lastTrailTime;
};

static h2stream_t clh2_Streams[MAX_STREAMS_H2];

void CLH2_ClearStreams()
{
	Com_Memset(clh2_Streams, 0, sizeof(clh2_Streams));
}

static h2stream_t* CLH2_NewStream(int ent, int tag)
{
	int i;
	h2stream_t* stream;

	// Search for a stream with matching entity and tag
	for (i = 0, stream = clh2_Streams; i < MAX_STREAMS_H2; i++, stream++)
	{
		if (stream->entity == ent && stream->tag == tag)
		{
			return stream;
		}
	}
	// Search for a free stream
	for (i = 0, stream = clh2_Streams; i < MAX_STREAMS_H2; i++, stream++)
	{
		if (!stream->models[0] || stream->endTime < cl.serverTime * 0.001)
		{
			return stream;
		}
	}
	return NULL;
}

static void CLH2_CreateStream(int type, int ent, int tag, int flags, int skin, int duration, const vec3_t source, const vec3_t dest, const qhandle_t* models)
{
	h2stream_t* stream = CLH2_NewStream(ent, tag);
	if (stream == NULL)
	{
		common->Printf("stream list overflow\n");
		return;
	}
	stream->type = type;
	stream->entity = ent;
	stream->tag = tag;
	stream->flags = flags;
	stream->skin = skin;
	stream->endTime = (cl.serverTime + duration) * 0.001;
	stream->lastTrailTime = 0;
	VectorCopy(source, stream->source);
	VectorCopy(dest, stream->dest);
	stream->models[0] = models[0];
	stream->models[1] = models[1];
	stream->models[2] = models[2];
	stream->models[3] = models[3];

	if (flags & H2STREAM_ATTACHED)
	{
		VectorCopy(vec3_origin, stream->offset);
		h2entity_state_t* state = CLH2_FindState(ent);
		if (state)
		{
			VectorSubtract(source, state->origin, stream->offset);
		}
	}
}

void CLH2_CreateStreamChain(int ent, int tag, int flags, int skin, int duration, const vec3_t source, const vec3_t dest)
{
	qhandle_t models[4];
	models[0] = R_RegisterModel("models/stchain.mdl");
	models[1] = models[2] = models[3] = 0;
	CLH2_CreateStream(H2TE_STREAM_CHAIN, ent, tag, flags, skin, duration, source, dest, models);
}

void CLH2_CreateStreamSunstaff1(int ent, int tag, int flags, int skin, int duration, const vec3_t source, const vec3_t dest)
{
	qhandle_t models[4];
	models[0] = R_RegisterModel("models/stsunsf1.mdl");
	models[1] = R_RegisterModel("models/stsunsf2.mdl");
	models[2] = R_RegisterModel("models/stsunsf3.mdl");
	models[3] = R_RegisterModel("models/stsunsf4.mdl");
	CLH2_CreateStream(H2TE_STREAM_SUNSTAFF1, ent, tag, flags, skin, duration, source, dest, models);
}

static void CLH2_CreateStreamSunstaff2(int ent, int tag, int flags, int skin, int duration, const vec3_t source, const vec3_t dest)
{
	qhandle_t models[4];
	models[0] = R_RegisterModel("models/stsunsf5.mdl");
	models[1] = 0;
	models[2] = R_RegisterModel("models/stsunsf3.mdl");
	models[3] = R_RegisterModel("models/stsunsf4.mdl");
	CLH2_CreateStream(H2TE_STREAM_SUNSTAFF2, ent, tag, flags, skin, duration, source, dest, models);
}

void CLH2_CreateStreamSunstaffPower(int ent, const vec3_t source, const vec3_t dest)
{
	qhandle_t models[4];
	models[0] = R_RegisterModel("models/stsunsf2.mdl");
	models[1] = R_RegisterModel("models/stsunsf1.mdl");
	models[2] = R_RegisterModel("models/stsunsf3.mdl");
	models[3] = R_RegisterModel("models/stsunsf4.mdl");
	CLH2_CreateStream(H2TE_STREAM_SUNSTAFF2, ent, 0, 0, 0, 800, source, dest, models);
}

void CLH2_CreateStreamLightning(int ent, int tag, int flags, int skin, int duration, const vec3_t source, const vec3_t dest)
{
	qhandle_t models[4];
	models[0] = R_RegisterModel("models/stlghtng.mdl");
	models[1] = models[2] = models[3] = 0;
	CLH2_CreateStream(H2TE_STREAM_LIGHTNING, ent, tag, flags, skin, duration, source, dest, models);
}

static void CLH2_CreateStreamLightningSmall(int ent, int tag, int flags, int skin, int duration, const vec3_t source, const vec3_t dest)
{
	qhandle_t models[4];
	models[0] = R_RegisterModel("models/stltng2.mdl");
	models[1] = models[2] = models[3] = 0;
	CLH2_CreateStream(H2TE_STREAM_LIGHTNING_SMALL, ent, tag, flags, skin, duration, source, dest, models);
}

static void CLHW_CreateStreamLightningSmall(int ent, int tag, int flags, int skin, int duration, const vec3_t source, const vec3_t dest)
{
	qhandle_t models[4];
	models[0] = R_RegisterModel("models/stltng2.mdl");
	models[1] = models[2] = models[3] = 0;
	CLH2_CreateStream(HWTE_STREAM_LIGHTNING_SMALL, ent, tag, flags, skin, duration, source, dest, models);
}

static void CLH2_CreateStreamFaMine(int ent, int tag, int flags, int skin, int duration, const vec3_t source, const vec3_t dest)
{
	qhandle_t models[4];
	models[0] = R_RegisterModel("models/fambeam.mdl");
	models[1] = models[2] = models[3] = 0;
	CLH2_CreateStream(H2TE_STREAM_FAMINE, ent, tag, flags, skin, duration, source, dest, models);
}

void CLH2_CreateStreamColourBeam(int ent, int tag, int flags, int skin, int duration, const vec3_t source, const vec3_t dest)
{
	qhandle_t models[4];
	models[0] = R_RegisterModel("models/stclrbm.mdl");
	models[1] = models[2] = models[3] = 0;
	CLH2_CreateStream(H2TE_STREAM_COLORBEAM, ent, tag, flags, skin, duration, source, dest, models);
}

void CLH2_CreateStreamIceChunks(int ent, int tag, int flags, int skin, int duration, const vec3_t source, const vec3_t dest)
{
	qhandle_t models[4];
	models[0] = R_RegisterModel("models/stice.mdl");
	models[1] = models[2] = models[3] = 0;
	CLH2_CreateStream(H2TE_STREAM_ICECHUNKS, ent, tag, flags, skin, duration, source, dest, models);
}

static void CLH2_CreateStreamGaze(int ent, int tag, int flags, int skin, int duration, const vec3_t source, const vec3_t dest)
{
	qhandle_t models[4];
	models[0] = R_RegisterModel("models/stmedgaz.mdl");
	models[1] = models[2] = models[3] = 0;
	CLH2_CreateStream(H2TE_STREAM_GAZE, ent, tag, flags, skin, duration, source, dest, models);
}

void CLH2_ParseStream(QMsg& message, int type)
{
	int ent = message.ReadShort();
	int flags = message.ReadByte();
	int tag = flags & 15;
	int duration = message.ReadByte() * 50;
	int skin = 0;
	if (type == H2TE_STREAM_COLORBEAM)
	{
		skin = message.ReadByte();
	}
	vec3_t source;
	message.ReadPos(source);
	vec3_t dest;
	message.ReadPos(dest);

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
	case HWTE_STREAM_LIGHTNING_SMALL:
		CLHW_CreateStreamLightningSmall(ent, tag, flags, skin, duration, source, dest);
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
		throw Exception("CLH2_ParseStream: bad type");
	}
}

void CLH2_UpdateStreams()
{
	h2stream_t* stream = clh2_Streams;
	for (int streamIndex = 0; streamIndex < MAX_STREAMS_H2; streamIndex++, stream++)
	{
		if (!stream->models[0])
		{
			// Inactive
			continue;
		}

		if (stream->endTime * 1000 < cl.serverTime)
		{
			// Inactive
			if (stream->type != H2TE_STREAM_LIGHTNING && stream->type != H2TE_STREAM_LIGHTNING_SMALL)
			{
				continue;
			}
			else if (stream->endTime * 1000 + 250 < cl.serverTime)
			{
				continue;
			}
		}

		if (stream->flags & H2STREAM_ATTACHED && stream->endTime * 1000 >= cl.serverTime)
		{
			// Attach the start position to owner
			h2entity_state_t* state = CLH2_FindState(stream->entity);
			if (state)
			{
				VectorAdd(state->origin, stream->offset, stream->source);
			}
		}

		vec3_t dist;
		VectorSubtract(stream->dest, stream->source, dist);
		vec3_t angles;
		VecToAnglesBuggy(dist, angles);

		vec3_t org;
		VectorCopy(stream->source, org);
		float d = VectorNormalize(dist);

		vec3_t right, up;
		float cosTime, sinTime, lifeTime, cos2Time, sin2Time;
		if ((GGameType & GAME_HexenWorld) && stream->type == H2TE_STREAM_SUNSTAFF2)
		{
			vec3_t discard;
			AngleVectors(angles, discard, right, up);

			lifeTime = ((stream->endTime - cl.serverTime * 0.001) / .8);
			cosTime = cos(cl.serverTime * 0.001 * 5);
			sinTime = sin(cl.serverTime * 0.001 * 5);
			cos2Time = cos(cl.serverTime * 0.001 * 5 + 3.14);
			sin2Time = sin(cl.serverTime * 0.001 * 5 + 3.14);
		}

		int segmentCount = 0;
		if (stream->type == H2TE_STREAM_ICECHUNKS)
		{
			int offset = (cl.serverTime / 25) % 30;
			for (int i = 0; i < 3; i++)
			{
				org[i] += dist[i] * offset;
			}
		}
		while (d > 0)
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
				angles[2] = (cl.serverTime / 100) % 360;
				if (GGameType & GAME_HexenWorld)
				{
					CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 50 + 100 * ((stream->endTime - cl.serverTime * 0.001) / .3), 0, 128, H2MLS_ABSLIGHT);
				}
				else
				{
					CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				}
				R_AddRefEntityToScene(&ent);

				Com_Memset(&ent, 0, sizeof(ent));
				ent.reType = RT_MODEL;
				VectorCopy(org, ent.origin);
				ent.hModel = stream->models[1];
				angles[2] = (cl.serverTime / 20) % 360;
				if (GGameType & GAME_HexenWorld)
				{
					CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 50 + 100 * ((stream->endTime - cl.serverTime * 0.001) / .5), 0, 128, H2MLS_ABSLIGHT | H2DRF_TRANSLUCENT);
				}
				else
				{
					CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT | H2DRF_TRANSLUCENT);
				}
				R_AddRefEntityToScene(&ent);
				break;
			case H2TE_STREAM_SUNSTAFF2:
				if (!(GGameType & GAME_HexenWorld))
				{
					angles[2] = (cl.serverTime / 100) % 360;
					ent.frame = (cl.serverTime / 100) % 8;
					CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
					R_AddRefEntityToScene(&ent);
				}
				else
				{
					angles[2] = (int)(cl.serverTime * 0.001 * 100) % 360;
					VectorMA(ent.origin, cosTime * (40 * lifeTime), right,  ent.origin);
					VectorMA(ent.origin, sinTime * (40 * lifeTime), up,  ent.origin);
					CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 100 + 150 * lifeTime, 0, 128, H2MLS_ABSLIGHT | H2DRF_TRANSLUCENT);
					R_AddRefEntityToScene(&ent);

					Com_Memset(&ent, 0, sizeof(ent));
					ent.reType = RT_MODEL;
					VectorCopy(org, ent.origin);
					ent.hModel = stream->models[0];
					angles[2] = (int)(cl.serverTime * 0.001 * 100) % 360;
					VectorMA(ent.origin, cos2Time * (40 * lifeTime), right,  ent.origin);
					VectorMA(ent.origin, sin2Time * (40 * lifeTime), up,  ent.origin);
					CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 100 + 150 * lifeTime, 0, 128, H2MLS_ABSLIGHT | H2DRF_TRANSLUCENT);
					R_AddRefEntityToScene(&ent);

					for (int ix = 0; ix < 2; ix++)
					{
						Com_Memset(&ent, 0, sizeof(ent));
						ent.reType = RT_MODEL;
						VectorCopy(org, ent.origin);
						if (ix)
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
						angles[2] = (int)(cl.serverTime * 0.001 * 20) % 360;
						CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 100 + 150 * lifeTime, 0, 128, H2MLS_ABSLIGHT);
						R_AddRefEntityToScene(&ent);
					}
				}
				break;
			case H2TE_STREAM_LIGHTNING:
				if (stream->endTime * 1000 < cl.serverTime)
				{	//fixme: keep last non-translucent frame and angle
					angles[2] = 0;
					CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128 + (stream->endTime - cl.serverTime / 1000.0) * 192, H2MLS_ABSLIGHT | H2DRF_TRANSLUCENT);
				}
				else
				{
					angles[2] = rand() % 360;
					ent.frame = rand() % 6;
					CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				}
				R_AddRefEntityToScene(&ent);
				break;
			case H2TE_STREAM_LIGHTNING_SMALL:
			case HWTE_STREAM_LIGHTNING_SMALL:
				if (stream->endTime * 1000 < cl.serverTime)
				{
					angles[2] = 0;
					CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128 + (stream->endTime - cl.serverTime / 1000.0) * 192, H2MLS_ABSLIGHT | H2DRF_TRANSLUCENT);
				}
				else
				{
					angles[2] = rand() % 360;
					ent.frame = rand() % 6;
					CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				}
				R_AddRefEntityToScene(&ent);
				break;
			case H2TE_STREAM_FAMINE:
				angles[2] = rand() % 360;
				ent.frame = 0;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			case H2TE_STREAM_COLORBEAM:
				angles[2] = 0;
				ent.skinNum = stream->skin;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				CLH2_HandleCustomSkin(&ent, -1);
				R_AddRefEntityToScene(&ent);
				break;
			case H2TE_STREAM_GAZE:
				angles[2] = 0;
				ent.frame = (cl.serverTime /  25) % 36;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			case H2TE_STREAM_ICECHUNKS:
				angles[2] = rand() % 360;
				ent.frame = rand() % 5;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			default:
				angles[2] = 0;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 0, 0);
				R_AddRefEntityToScene(&ent);
			}
			for (int i = 0; i < 3; i++)
			{
				org[i] += dist[i] * 30;
			}
			d -= 30;
			segmentCount++;
		}
		if (stream->type == H2TE_STREAM_SUNSTAFF1 ||
			(!(GGameType & GAME_HexenWorld) && stream->type == H2TE_STREAM_SUNSTAFF2))
		{
			if (stream->lastTrailTime * 1000 + 200 < cl.serverTime)
			{
				stream->lastTrailTime = cl.serverTime / 1000.0;
				CLH2_SunStaffTrail(stream->source, stream->dest);
			}

			refEntity_t ent;
			Com_Memset(&ent, 0, sizeof(ent));
			ent.reType = RT_MODEL;
			VectorCopy(stream->dest, ent.origin);
			ent.hModel = stream->models[2];
			CLH2_SetRefEntAxis(&ent, vec3_origin, vec3_origin, 80 + (rand() & 15), 0, 128, H2MLS_ABSLIGHT);
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
