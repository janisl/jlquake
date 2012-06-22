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

#include "../server.h"
#include "local.h"

//	Sends text across to be displayed if the level passes
void SVQ2_ClientPrintf(client_t* cl, int level, const char* fmt, ...)
{
	if (level < cl->messagelevel)
	{
		return;
	}

	va_list argptr;
	char string[1024];
	va_start(argptr,fmt);
	Q_vsnprintf(string, 1024, fmt, argptr);
	va_end(argptr);

	cl->netchan.message.WriteByte(q2svc_print);
	cl->netchan.message.WriteByte(level);
	cl->netchan.message.WriteString2(string);
}

//	Sends text to all active clients
void SVQ2_BroadcastPrintf(int level, const char* fmt, ...)
{
	va_list argptr;
	char string[2048];
	va_start(argptr,fmt);
	Q_vsnprintf(string, 2048, fmt, argptr);
	va_end(argptr);

	// echo to console
	if (com_dedicated->value)
	{
		char copy[1024];
		int i;

		// mask off high bits
		for (i = 0; i < 1023 && string[i]; i++)
			copy[i] = string[i] & 127;
		copy[i] = 0;
		common->Printf("%s", copy);
	}

	client_t* cl = svs.clients;
	for (int i = 0; i < sv_maxclients->value; i++, cl++)
	{
		if (level < cl->messagelevel)
		{
			continue;
		}
		if (cl->state != CS_ACTIVE)
		{
			continue;
		}
		cl->netchan.message.WriteByte(q2svc_print);
		cl->netchan.message.WriteByte(level);
		cl->netchan.message.WriteString2(string);
	}
}

//	Sends the contents of sv.multicast to a subset of the clients,
// then clears sv.multicast.
//	Q2MULTICAST_ALL	same as broadcast (origin can be NULL)
//	Q2MULTICAST_PVS	send to clients potentially visible from org
//	Q2MULTICAST_PHS	send to clients potentially hearable from org
void SVQ2_Multicast(const vec3_t origin, q2multicast_t to)
{
	int area1;
	if (to != Q2MULTICAST_ALL_R && to != Q2MULTICAST_ALL)
	{
		int leafnum = CM_PointLeafnum(origin);
		area1 = CM_LeafArea(leafnum);
	}
	else
	{
		area1 = 0;
	}

	// if doing a serverrecord, store everything
	if (svs.q2_demofile)
	{
		svs.q2_demo_multicast.WriteData(sv.multicast._data, sv.multicast.cursize);
	}

	bool reliable = false;
	byte* mask;
	switch (to)
	{
	case Q2MULTICAST_ALL_R:
		reliable = true;	// intentional fallthrough
	case Q2MULTICAST_ALL:
		mask = NULL;
		break;

	case Q2MULTICAST_PHS_R:
		reliable = true;	// intentional fallthrough
	case Q2MULTICAST_PHS:
	{
		int leafnum = CM_PointLeafnum(origin);
		int cluster = CM_LeafCluster(leafnum);
		mask = CM_ClusterPHS(cluster);
		break;
	}

	case Q2MULTICAST_PVS_R:
		reliable = true;	// intentional fallthrough
	case Q2MULTICAST_PVS:
	{
		int leafnum = CM_PointLeafnum(origin);
		int cluster = CM_LeafCluster(leafnum);
		mask = CM_ClusterPVS(cluster);
	}
		break;

	default:
		mask = NULL;
		common->FatalError("SV_Multicast: bad to:%i", to);
	}

	// send the data to all relevent clients
	client_t* client = svs.clients;
	for (int j = 0; j < sv_maxclients->value; j++, client++)
	{
		if (client->state == CS_FREE || client->state == CS_ZOMBIE)
		{
			continue;
		}
		if (client->state != CS_ACTIVE && !reliable)
		{
			continue;
		}

		if (mask)
		{
			int leafnum = CM_PointLeafnum(client->q2_edict->s.origin);
			int cluster = CM_LeafCluster(leafnum);
			int area2 = CM_LeafArea(leafnum);
			if (!CM_AreasConnected(area1, area2))
			{
				continue;
			}
			if (mask && (!(mask[cluster >> 3] & (1 << (cluster & 7)))))
			{
				continue;
			}
		}

		if (reliable)
		{
			client->netchan.message.WriteData(sv.multicast._data, sv.multicast.cursize);
		}
		else
		{
			client->datagram.WriteData(sv.multicast._data, sv.multicast.cursize);
		}
	}

	sv.multicast.Clear();
}

//	Sends text to all active clients
void SVQ2_BroadcastCommand(const char* fmt, ...)
{
	va_list argptr;
	char string[1024];

	if (!sv.state)
	{
		return;
	}
	va_start(argptr,fmt);
	Q_vsnprintf(string, 1024, fmt, argptr);
	va_end(argptr);

	sv.multicast.WriteByte(q2svc_stufftext);
	sv.multicast.WriteString2(string);
	SVQ2_Multicast(NULL, Q2MULTICAST_ALL_R);
}

//	Each entity can have eight independant sound sources, like voice,
// weapon, feet, etc.
//	If cahnnel & 8, the sound will be sent to everyone, not just
// things in the PHS.
//	FIXME: if entity isn't in PHS, they must be forced to be sent or
// have the origin explicitly sent.
//	Channel 0 is an auto-allocate channel, the others override anything
// already running on that entity/channel pair.
//	An attenuation of 0 will play full volume everywhere in the level.
// Larger attenuations will drop off.  (max 4 attenuation)
//	Timeofs can range from 0.0 to 0.1 to cause sounds to be started
// later in the frame than they normally would.
//	If origin is NULL, the origin is determined from the entity origin
// or the midpoint of the entity box for bmodels.
void SVQ2_StartSound(const vec3_t origin, q2edict_t* entity, int channel,
	int soundindex, float volume,
	float attenuation, float timeofs)
{
	int sendchan;
	int flags;
	int i;
	int ent;
	vec3_t origin_v;
	qboolean use_phs;

	if (volume < 0 || volume > 1.0)
	{
		common->FatalError("SV_StartSound: volume = %f", volume);
	}

	if (attenuation < 0 || attenuation > 4)
	{
		common->FatalError("SV_StartSound: attenuation = %f", attenuation);
	}

//	if (channel < 0 || channel > 15)
//		Com_Error (ERR_FATAL, "SV_StartSound: channel = %i", channel);

	if (timeofs < 0 || timeofs > 0.255)
	{
		common->FatalError("SV_StartSound: timeofs = %f", timeofs);
	}

	ent = Q2_NUM_FOR_EDICT(entity);

	if (channel & 8)	// no PHS flag
	{
		use_phs = false;
		channel &= 7;
	}
	else
	{
		use_phs = true;
	}

	sendchan = (ent << 3) | (channel & 7);

	flags = 0;
	if (volume != Q2DEFAULT_SOUND_PACKET_VOLUME)
	{
		flags |= Q2SND_VOLUME;
	}
	if (attenuation != Q2DEFAULT_SOUND_PACKET_ATTENUATION)
	{
		flags |= Q2SND_ATTENUATION;
	}

	// the client doesn't know that bmodels have weird origins
	// the origin can also be explicitly set
	if ((entity->svflags & Q2SVF_NOCLIENT) ||
		(entity->solid == Q2SOLID_BSP) ||
		origin)
	{
		flags |= Q2SND_POS;
	}

	// always send the entity number for channel overrides
	flags |= Q2SND_ENT;

	if (timeofs)
	{
		flags |= Q2SND_OFFSET;
	}

	// use the entity origin unless it is a bmodel or explicitly specified
	if (!origin)
	{
		origin = origin_v;
		if (entity->solid == Q2SOLID_BSP)
		{
			for (i = 0; i < 3; i++)
				origin_v[i] = entity->s.origin[i] + 0.5 * (entity->mins[i] + entity->maxs[i]);
		}
		else
		{
			VectorCopy(entity->s.origin, origin_v);
		}
	}

	sv.multicast.WriteByte(q2svc_sound);
	sv.multicast.WriteByte(flags);
	sv.multicast.WriteByte(soundindex);

	if (flags & Q2SND_VOLUME)
	{
		sv.multicast.WriteByte(volume * 255);
	}
	if (flags & Q2SND_ATTENUATION)
	{
		sv.multicast.WriteByte(attenuation * 64);
	}
	if (flags & Q2SND_OFFSET)
	{
		sv.multicast.WriteByte(timeofs * 1000);
	}

	if (flags & Q2SND_ENT)
	{
		sv.multicast.WriteShort(sendchan);
	}

	if (flags & Q2SND_POS)
	{
		sv.multicast.WritePos(origin);
	}

	// if the sound doesn't attenuate,send it to everyone
	// (global radio chatter, voiceovers, etc)
	if (attenuation == ATTN_NONE)
	{
		use_phs = false;
	}

	if (channel & Q2CHAN_RELIABLE)
	{
		if (use_phs)
		{
			SVQ2_Multicast(origin, Q2MULTICAST_PHS_R);
		}
		else
		{
			SVQ2_Multicast(origin, Q2MULTICAST_ALL_R);
		}
	}
	else
	{
		if (use_phs)
		{
			SVQ2_Multicast(origin, Q2MULTICAST_PHS);
		}
		else
		{
			SVQ2_Multicast(origin, Q2MULTICAST_ALL);
		}
	}
}
