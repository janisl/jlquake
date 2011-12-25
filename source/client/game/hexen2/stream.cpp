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

h2stream_t* CLH2_NewStream(int ent, int tag)
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

void CLH2_InitStream(h2stream_t* stream, int type, int ent, int tag, int flags, int skin, int duration, const vec3_t source, const vec3_t dest, const qhandle_t* models)
{
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
