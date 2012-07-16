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
//#include "../progsvm/progsvm.h"
#include "local.h"

unsigned clients_multicast;

Cvar* sv_phs;

//	Sends the contents of sv.multicast to a subset of the clients,
// then clears sv.multicast.
//
//	MULTICAST_ALL	same as broadcast
//	MULTICAST_PVS	send to clients potentially visible from org
//	MULTICAST_PHS	send to clients potentially hearable from org
void SVQH_Multicast(const vec3_t origin, int to)
{
	clients_multicast = 0;

	int leafnum = CM_PointLeafnum(origin);

	bool reliable = false;
	byte* mask;
	switch (to)
	{
	case MULTICAST_ALL_R:
		reliable = true;
		// intentional fallthrough
	case MULTICAST_ALL:
		mask = NULL;
		break;

	case MULTICAST_PHS_R:
		reliable = true;
		// intentional fallthrough
	case MULTICAST_PHS:
		mask = CM_ClusterPHS(CM_LeafCluster(leafnum));
		break;

	case MULTICAST_PVS_R:
		reliable = true;
		// intentional fallthrough
	case MULTICAST_PVS:
		mask = CM_ClusterPVS(CM_LeafCluster(leafnum));
		break;

	default:
		mask = NULL;
		common->Error("SVQH_Multicast: bad to:%i", to);
	}

	// send the data to all relevent clients
	client_t* client = svs.clients;
	for (int j = 0; j < MAX_CLIENTS_QHW; j++, client++)
	{
		if (client->state != CS_ACTIVE)
		{
			continue;
		}

		vec3_t adjust_origin;
		VectorCopy(client->qh_edict->GetOrigin(), adjust_origin);
		if (GGameType & GAME_HexenWorld)
		{
			adjust_origin[2] += 16;
		}
		else
		{
			if (to == MULTICAST_PHS_R || to == MULTICAST_PHS)
			{
				vec3_t delta;
				VectorSubtract(origin, adjust_origin, delta);
				if (VectorLength(delta) <= 1024)
				{
					goto inrange;
				}
			}
		}

		if (mask)
		{
			leafnum = CM_PointLeafnum(adjust_origin);
			leafnum = CM_LeafCluster(leafnum);
			if (leafnum < 0 || !(mask[leafnum >> 3] & (1 << (leafnum & 7))))
			{
				continue;
			}
		}

inrange:
		clients_multicast |= 1l << j;

		if (reliable)
		{
			SVQH_ClientReliableCheckBlock(client, sv.multicast.cursize);
			SVQH_ClientReliableWrite_SZ(client, sv.multicast._data, sv.multicast.cursize);
		}
		else
		{
			client->datagram.WriteData(sv.multicast._data, sv.multicast.cursize);
		}
	}

	sv.multicast.Clear();
}

void SVH2_StopSound(qhedict_t* entity, int channel)
{
	int ent = QH_NUM_FOR_EDICT(entity);
	channel = (ent << 3) | channel;

	if (GGameType & GAME_HexenWorld)
	{
		// use the entity origin unless it is a bmodel
		vec3_t origin;
		if (entity->GetSolid() == QHSOLID_BSP)
		{
			//FIXME: This may not work- should be using (absmin + absmax)*0.5?
			for (int i = 0; i < 3; i++)
			{
				origin[i] = entity->GetOrigin()[i] + 0.5 * (entity->GetMins()[i] + entity->GetMaxs()[i]);
			}
		}
		else
		{
			VectorCopy(entity->GetOrigin(), origin);
		}

		sv.multicast.WriteByte(h2svc_stopsound);
		sv.multicast.WriteShort(channel);
		SVQH_Multicast(origin, MULTICAST_ALL_R);
	}
	else
	{
		if (sv.qh_datagram.cursize > MAX_DATAGRAM_QH - 4)
		{
			return;
		}

		sv.qh_datagram.WriteByte(h2svc_stopsound);
		sv.qh_datagram.WriteShort(channel);
	}
}

//	Each entity can have eight independant sound sources, like voice,
// weapon, feet, etc.
//	Channel 0 is an auto-allocate channel, the others override anything
// allready running on that entity/channel pair.
//	An attenuation of 0 will play full volume everywhere in the level.
// Larger attenuations will drop off.  (max 4 attenuation)
void SVQH_StartSound(qhedict_t* entity, int channel, const char* sample, int volume,
	float attenuation)
{
	if (GGameType & GAME_Hexen2 && !(GGameType & GAME_HexenWorld) &&
		String::ICmp(sample, "misc/null.wav") == 0)
	{
		SVH2_StopSound(entity,channel);
		return;
	}

	if (volume < 0 || volume > 255)
	{
		common->Error("SVQH_StartSound: volume = %i", volume);
	}

	if (attenuation < 0 || attenuation > 4)
	{
		common->Error("SVQH_StartSound: attenuation = %f", attenuation);
	}

	if (channel < 0 || channel > (GGameType & (GAME_QuakeWorld | GAME_HexenWorld) ? 15 : 7))
	{
		common->Error("SVQH_StartSound: channel = %i", channel);
	}

	// find precache number for sound
	int maxSounds = GGameType & GAME_Hexen2 ? GGameType & GAME_HexenWorld ? MAX_SOUNDS_HW : MAX_SOUNDS_H2 : MAX_SOUNDS_Q1;
	int sound_num;
	for (sound_num = 1; sound_num < maxSounds &&
		 sv.qh_sound_precache[sound_num]; sound_num++)
	{
		if (!String::Cmp(sample, sv.qh_sound_precache[sound_num]))
		{
			break;
		}
	}

	if (sound_num == maxSounds || !sv.qh_sound_precache[sound_num])
	{
		common->Printf("SVQH_StartSound: %s not precached\n", sample);
		return;
	}

	int ent = QH_NUM_FOR_EDICT(entity);

	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		bool use_phs;
		bool reliable = false;
		if ((channel & QHPHS_OVERRIDE_R) || !sv_phs->value)	// no PHS flag
		{
			if (channel & QHPHS_OVERRIDE_R)
			{
				reliable = true;// sounds that break the phs are reliable
			}
			use_phs = false;
			channel &= 7;
		}
		else
		{
			use_phs = true;
		}

		channel = (ent << 3) | channel;
		if (volume != QHDEFAULT_SOUND_PACKET_VOLUME)
		{
			channel |= QHWSND_VOLUME;
		}
		if (attenuation != QHDEFAULT_SOUND_PACKET_ATTENUATION)
		{
			channel |= QHWSND_ATTENUATION;
		}

		// use the entity origin unless it is a bmodel
		vec3_t origin;
		if (entity->GetSolid() == QHSOLID_BSP)
		{
			for (int i = 0; i < 3; i++)
			{
				origin[i] = entity->GetOrigin()[i] + 0.5 * (entity->GetMins()[i] + entity->GetMaxs()[i]);
			}
		}
		else
		{
			VectorCopy(entity->GetOrigin(), origin);
		}

		sv.multicast.WriteByte(GGameType & GAME_Hexen2 ? h2svc_sound : q1svc_sound);
		sv.multicast.WriteShort(channel);
		if (channel & QHWSND_VOLUME)
		{
			sv.multicast.WriteByte(volume);
		}
		if (channel & QHWSND_ATTENUATION)
		{
			sv.multicast.WriteByte(attenuation * (GGameType & GAME_Hexen2 ? 32 : 64));
		}
		sv.multicast.WriteByte(sound_num);
		for (int i = 0; i < 3; i++)
		{
			sv.multicast.WriteCoord(origin[i]);
		}

		if (use_phs)
		{
			SVQH_Multicast(origin, reliable ? MULTICAST_PHS_R : MULTICAST_PHS);
		}
		else
		{
			SVQH_Multicast(origin, reliable ? MULTICAST_ALL_R : MULTICAST_ALL);
		}
	}
	else
	{
		if (sv.qh_datagram.cursize > MAX_DATAGRAM_QH - 16)
		{
			return;
		}

		channel = (ent << 3) | channel;

		int field_mask = 0;
		if (volume != QHDEFAULT_SOUND_PACKET_VOLUME)
		{
			field_mask |= QHSND_VOLUME;
		}
		if (attenuation != QHDEFAULT_SOUND_PACKET_ATTENUATION)
		{
			field_mask |= QHSND_ATTENUATION;
		}
		if (GGameType & GAME_Hexen2 && sound_num > 255)
		{
			field_mask |= QHSND_ATTENUATION;
			sound_num -= 255;
		}

		// directed messages go only to the entity the are targeted on
		sv.qh_datagram.WriteByte(GGameType & GAME_Hexen2 ? h2svc_sound : q1svc_sound);
		sv.qh_datagram.WriteByte(field_mask);
		if (field_mask & QHSND_VOLUME)
		{
			sv.qh_datagram.WriteByte(volume);
		}
		if (field_mask & QHSND_ATTENUATION)
		{
			sv.qh_datagram.WriteByte(attenuation * 64);
		}
		sv.qh_datagram.WriteShort(channel);
		sv.qh_datagram.WriteByte(sound_num);
		for (int i = 0; i < 3; i++)
		{
			sv.qh_datagram.WriteCoord(entity->GetOrigin()[i] + 0.5 * (entity->GetMins()[i] + entity->GetMaxs()[i]));
		}
	}
}

void SVH2_UpdateSoundPos(qhedict_t* entity, int channel)
{
	int ent = QH_NUM_FOR_EDICT(entity);
	channel = (ent << 3) | channel;

	if (GGameType & GAME_HexenWorld)
	{
		// use the entity origin unless it is a bmodel
		vec3_t origin;
		if (entity->GetSolid() == QHSOLID_BSP)
		{
			for (int i = 0; i < 3; i++)	//FIXME: This may not work- should be using (absmin + absmax)*0.5?
			{
				origin[i] = entity->GetOrigin()[i] + 0.5 * (entity->GetMins()[i] + entity->GetMaxs()[i]);
			}
		}
		else
		{
			VectorCopy(entity->GetOrigin(), origin);
		}

		sv.multicast.WriteByte(hwsvc_sound_update_pos);
		sv.multicast.WriteShort(channel);
		for (int i = 0; i < 3; i++)
		{
			sv.multicast.WriteCoord(entity->GetOrigin()[i] + 0.5 * (entity->GetMins()[i] + entity->GetMaxs()[i]));
		}
		SVQH_Multicast(origin, MULTICAST_PHS);
	}
	else
	{
		if (sv.qh_datagram.cursize > MAX_DATAGRAM_QH - 4)
		{
			return;
		}

		sv.qh_datagram.WriteByte(h2svc_sound_update_pos);
		sv.qh_datagram.WriteShort(channel);
		for (int i = 0; i < 3; i++)
		{
			sv.qh_datagram.WriteCoord(entity->GetOrigin()[i] + 0.5 * (entity->GetMins()[i] + entity->GetMaxs()[i]));
		}
	}
}
