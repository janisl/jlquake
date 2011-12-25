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

#include "../../client.h"
#include "local.h"

h2stream_t clh2_Streams[MAX_STREAMS_H2];

void CLH2_ClearStreams()
{
	Com_Memset(clh2_Streams, 0, sizeof(clh2_Streams));
}

static h2stream_t* CLH2_NewStream(int ent, int tag)
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
		if(!stream->models[0] || stream->endTime < cl_common->serverTime * 0.001)
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
		Log::write("stream list overflow\n");
		return;
	}
	stream->type = type;
	stream->entity = ent;
	stream->tag = tag;
	stream->flags = flags;
	stream->skin = skin;
	stream->endTime = (cl_common->serverTime + duration) * 0.001;
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
