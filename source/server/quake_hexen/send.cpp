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
#include "../progsvm/progsvm.h"
#include "local.h"
#include "../hexen2/local.h"

unsigned clients_multicast;

Cvar* svqhw_phs;

int svqw_nailmodel;
int svqw_supernailmodel;
int svqw_playermodel;
int svhw_magicmissmodel;
int svhw_playermodel[MAX_PLAYER_CLASS];
int svhw_ravenmodel;
int svhw_raven2model;

static char outputbuf[8000];

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

//	Sends the contents of sv.multicast to a subset of the clients,
// then clears sv.multicast.
void SVHW_MulticastSpecific(unsigned clients, bool reliable)
{
	clients_multicast = 0;

	// send the data to all relevent clients
	client_t* client = svs.clients;
	for (int j = 0; j < MAX_CLIENTS_QHW; j++, client++)
	{
		if (client->state != CS_ACTIVE)
		{
			continue;
		}

		if ((1l << j) & clients)
		{
			clients_multicast |= 1l << j;

			if (reliable)
			{
				client->netchan.message.WriteData(sv.multicast._data, sv.multicast.cursize);
			}
			else
			{
				client->datagram.WriteData(sv.multicast._data, sv.multicast.cursize);
			}
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
		if ((channel & QHPHS_OVERRIDE_R) || !svqhw_phs->value)	// no PHS flag
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

void SVQH_PrintToClient(client_t* cl, int level, char* string)
{
	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		SVQH_ClientReliableWrite_Begin(cl, GGameType & GAME_HexenWorld ? h2svc_print : q1svc_print, String::Length(string) + 3);
		SVQH_ClientReliableWrite_Byte(cl, level);
		SVQH_ClientReliableWrite_String(cl, string);
	}
	else
	{
		cl->qh_message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_print : q1svc_print);
		cl->qh_message.WriteString2(string);
	}
}

//	Sends text across to be displayed if the level passes
void SVQH_ClientPrintf(client_t* cl, int level, const char* fmt, ...)
{
	va_list argptr;
	char string[MAXPRINTMSG];

	if (level < cl->messagelevel)
	{
		return;
	}

	va_start(argptr,fmt);
	Q_vsnprintf(string, MAXPRINTMSG, fmt, argptr);
	va_end(argptr);

	SVQH_PrintToClient(cl, level, string);
}

//	Sends text to all active clients
void SVQH_BroadcastPrintf(int level, const char* fmt, ...)
{
	va_list argptr;
	char string[1024];
	int i;

	va_start(argptr,fmt);
	Q_vsnprintf(string, 1024, fmt, argptr);
	va_end(argptr);

	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		common->Printf("%s", string);	// print to the console
	}

	client_t* cl = svs.clients;
	for (i = 0; i < (GGameType & (GAME_QuakeWorld | GAME_HexenWorld) ? MAX_CLIENTS_QHW : svs.qh_maxclients); i++, cl++)
	{
		if (level < cl->messagelevel)
		{
			continue;
		}
		if (cl->state < CS_CONNECTED)
		{
			continue;
		}
		SVQH_PrintToClient(cl, level, string);
	}
}

//	Send text over to the client to be executed
void SVQH_SendClientCommand(client_t* cl, const char* fmt, ...)
{
	va_list argptr;
	char string[1024];
	va_start(argptr,fmt);
	Q_vsnprintf(string, 1024, fmt, argptr);
	va_end(argptr);

	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		SVQH_ClientReliableWrite_Begin(cl, GGameType & GAME_Hexen2 ? h2svc_stufftext : q1svc_stufftext, String::Length(string) + 2);
		SVQH_ClientReliableWrite_String(cl, string);
	}
	else
	{
		cl->qh_message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_stufftext : q1svc_stufftext);
		cl->qh_message.WriteString2(string);
	}
}

//	Sends text to all active clients
void SVQH_BroadcastCommand(const char* fmt, ...)
{
	if (!sv.state)
	{
		return;
	}

	va_list argptr;
	char string[1024];
	va_start(argptr,fmt);
	Q_vsnprintf(string, 1024, fmt, argptr);
	va_end(argptr);

	sv.qh_reliable_datagram.WriteByte(GGameType & GAME_HexenWorld ? h2svc_stufftext : q1svc_stufftext);
	sv.qh_reliable_datagram.WriteString2(string);
}

void SVQH_StartParticle(const vec3_t org, const vec3_t dir, int color, int count)
{
	int i, v;

	if (!(GGameType & GAME_HexenWorld) && sv.qh_datagram.cursize > MAX_DATAGRAM_QH - 16)
	{
		return;
	}
	QMsg& message = GGameType & GAME_HexenWorld ? sv.multicast : sv.qh_datagram;
	message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_particle : q1svc_particle);
	message.WriteCoord(org[0]);
	message.WriteCoord(org[1]);
	message.WriteCoord(org[2]);
	for (i = 0; i < 3; i++)
	{
		v = dir[i] * 16;
		if (v > 127)
		{
			v = 127;
		}
		else if (v < -128)
		{
			v = -128;
		}
		message.WriteChar(v);
	}
	message.WriteByte(count);
	message.WriteByte(color);

	if (GGameType & GAME_HexenWorld)
	{
		SVQH_Multicast(org, MULTICAST_PVS);
	}
}

void SVH2_StartParticle2(const vec3_t org, const vec3_t dmin, const vec3_t dmax, int color, int effect, int count)
{
	if (!(GGameType & GAME_HexenWorld) && sv.qh_datagram.cursize > MAX_DATAGRAM_QH - 36)
	{
		return;
	}
	QMsg& message = GGameType & GAME_HexenWorld ? sv.multicast : sv.qh_datagram;
	message.WriteByte(GGameType & GAME_HexenWorld ? hwsvc_particle2 : h2svc_particle2);
	message.WriteCoord(org[0]);
	message.WriteCoord(org[1]);
	message.WriteCoord(org[2]);
	message.WriteFloat(dmin[0]);
	message.WriteFloat(dmin[1]);
	message.WriteFloat(dmin[2]);
	message.WriteFloat(dmax[0]);
	message.WriteFloat(dmax[1]);
	message.WriteFloat(dmax[2]);

	message.WriteShort(color);
	message.WriteByte(count);
	message.WriteByte(effect);

	if (GGameType & GAME_HexenWorld)
	{
		SVQH_Multicast(org, MULTICAST_PVS);
	}
}

void SVH2_StartParticle3(const vec3_t org, const vec3_t box, int color, int effect, int count)
{
	if (!(GGameType & GAME_HexenWorld) && sv.qh_datagram.cursize > MAX_DATAGRAM_QH - 15)
	{
		return;
	}
	QMsg& message = GGameType & GAME_HexenWorld ? sv.multicast : sv.qh_datagram;
	message.WriteByte(GGameType & GAME_HexenWorld ? hwsvc_particle3 : h2svc_particle3);
	message.WriteCoord(org[0]);
	message.WriteCoord(org[1]);
	message.WriteCoord(org[2]);
	message.WriteByte(box[0]);
	message.WriteByte(box[1]);
	message.WriteByte(box[2]);

	message.WriteShort(color);
	message.WriteByte(count);
	message.WriteByte(effect);

	if (GGameType & GAME_HexenWorld)
	{
		SVQH_Multicast(org, MULTICAST_PVS);
	}
}

void SVH2_StartParticle4(const vec3_t org, float radius, int color, int effect, int count)
{
	if (!(GGameType & GAME_HexenWorld) && sv.qh_datagram.cursize > MAX_DATAGRAM_QH - 13)
	{
		return;
	}
	QMsg& message = GGameType & GAME_HexenWorld ? sv.multicast : sv.qh_datagram;
	message.WriteByte(GGameType & GAME_HexenWorld ? hwsvc_particle4 : h2svc_particle4);
	message.WriteCoord(org[0]);
	message.WriteCoord(org[1]);
	message.WriteCoord(org[2]);
	message.WriteByte(radius);

	message.WriteShort(color);
	message.WriteByte(count);
	message.WriteByte(effect);

	if (GGameType & GAME_HexenWorld)
	{
		SVQH_Multicast(org, MULTICAST_PVS);
	}
}

void SVH2_StartRainEffect(const vec3_t org, const vec3_t e_size, int x_dir, int y_dir, int color, int count)
{
	QMsg& message = GGameType & GAME_HexenWorld ? sv.multicast : sv.qh_datagram;
	message.WriteByte(GGameType & GAME_HexenWorld ? hwsvc_raineffect : h2svc_raineffect);
	message.WriteCoord(org[0]);
	message.WriteCoord(org[1]);
	message.WriteCoord(org[2]);
	message.WriteCoord(e_size[0]);
	message.WriteCoord(e_size[1]);
	message.WriteCoord(e_size[2]);
	message.WriteAngle(x_dir);
	message.WriteAngle(y_dir);
	message.WriteShort(color);
	message.WriteShort(count);

	if (GGameType & GAME_HexenWorld)
	{
		SVQH_Multicast(org, MULTICAST_PVS);
	}
}

void SVQH_ClearDatagram()
{
	sv.qh_datagram.Clear();
}

static void SVQ1_WriteClientdataToMessage(qhedict_t* ent, QMsg* msg)
{
	int bits = 0;

	if (ent->GetViewOfs()[2] != Q1DEFAULT_VIEWHEIGHT)
	{
		bits |= Q1SU_VIEWHEIGHT;
	}

	if (ent->GetIdealPitch())
	{
		bits |= Q1SU_IDEALPITCH;
	}

	// stuff the sigil bits into the high bits of items for sbar, or else
	// mix in items2
	eval_t* val = GetEdictFieldValue(ent, "items2");

	int items;
	if (val)
	{
		items = (int)ent->GetItems() | ((int)val->_float << 23);
	}
	else
	{
		items = (int)ent->GetItems() | ((int)*pr_globalVars.serverflags << 28);
	}

	bits |= Q1SU_ITEMS;

	if ((int)ent->GetFlags() & QHFL_ONGROUND)
	{
		bits |= Q1SU_ONGROUND;
	}

	if (ent->GetWaterLevel() >= 2)
	{
		bits |= Q1SU_INWATER;
	}

	for (int i = 0; i < 3; i++)
	{
		if (ent->GetPunchAngle()[i])
		{
			bits |= (Q1SU_PUNCH1 << i);
		}
		if (ent->GetVelocity()[i])
		{
			bits |= (Q1SU_VELOCITY1 << i);
		}
	}

	if (ent->GetWeaponFrame())
	{
		bits |= Q1SU_WEAPONFRAME;
	}

	if (ent->GetArmorValue())
	{
		bits |= Q1SU_ARMOR;
	}

	bits |= Q1SU_WEAPON;

	// send the data

	msg->WriteByte(q1svc_clientdata);
	msg->WriteShort(bits);

	if (bits & Q1SU_VIEWHEIGHT)
	{
		msg->WriteChar(ent->GetViewOfs()[2]);
	}

	if (bits & Q1SU_IDEALPITCH)
	{
		msg->WriteChar(ent->GetIdealPitch());
	}

	for (int i = 0; i < 3; i++)
	{
		if (bits & (Q1SU_PUNCH1 << i))
		{
			msg->WriteChar(ent->GetPunchAngle()[i]);
		}
		if (bits & (Q1SU_VELOCITY1 << i))
		{
			msg->WriteChar(ent->GetVelocity()[i] / 16);
		}
	}

	// [always sent]	if (bits & Q1SU_ITEMS)
	msg->WriteLong(items);

	if (bits & Q1SU_WEAPONFRAME)
	{
		msg->WriteByte(ent->GetWeaponFrame());
	}
	if (bits & Q1SU_ARMOR)
	{
		msg->WriteByte(ent->GetArmorValue());
	}
	if (bits & Q1SU_WEAPON)
	{
		msg->WriteByte(SVQH_ModelIndex(PR_GetString(ent->GetWeaponModel())));
	}

	msg->WriteShort(ent->GetHealth());
	msg->WriteByte(ent->GetCurrentAmmo());
	msg->WriteByte(ent->GetAmmoShells());
	msg->WriteByte(ent->GetAmmoNails());
	msg->WriteByte(ent->GetAmmoRockets());
	msg->WriteByte(ent->GetAmmoCells());

	if (q1_standard_quake)
	{
		msg->WriteByte(ent->GetWeapon());
	}
	else
	{
		for (int i = 0; i < 32; i++)
		{
			if (((int)ent->GetWeapon()) & (1 << i))
			{
				msg->WriteByte(i);
				break;
			}
		}
	}
}

static void SVH2_WriteClientdataToMessage(client_t* client, qhedict_t* ent, QMsg* msg)
{
	int bits = 0;

	if (client->h2_send_all_v)
	{
		bits = Q1SU_VIEWHEIGHT | Q1SU_IDEALPITCH | H2SU_IDEALROLL |
			   Q1SU_VELOCITY1 | (Q1SU_VELOCITY1 << 1) | (Q1SU_VELOCITY1 << 2) |
			   Q1SU_PUNCH1 | (Q1SU_PUNCH1 << 1) | (Q1SU_PUNCH1 << 2) | Q1SU_WEAPONFRAME |
			   Q1SU_ARMOR | Q1SU_WEAPON;
	}
	else
	{
		if (ent->GetViewOfs()[2] != client->h2_old_v.view_ofs[2])
		{
			bits |= Q1SU_VIEWHEIGHT;
		}

		if (ent->GetIdealPitch() != client->h2_old_v.idealpitch)
		{
			bits |= Q1SU_IDEALPITCH;
		}

		if (ent->GetIdealRoll() != client->h2_old_v.idealroll)
		{
			bits |= H2SU_IDEALROLL;
		}

		for (int i = 0; i < 3; i++)
		{
			if (ent->GetPunchAngle()[i] != client->h2_old_v.punchangle[i])
			{
				bits |= (Q1SU_PUNCH1 << i);
			}
			if (ent->GetVelocity()[i] != client->h2_old_v.velocity[i])
			{
				bits |= (Q1SU_VELOCITY1 << i);
			}
		}

		if (ent->GetWeaponFrame() != client->h2_old_v.weaponframe)
		{
			bits |= Q1SU_WEAPONFRAME;
		}

		if (ent->GetArmorValue() != client->h2_old_v.armorvalue)
		{
			bits |= Q1SU_ARMOR;
		}

		if (ent->GetWeaponModel() != client->h2_old_v.weaponmodel)
		{
			bits |= Q1SU_WEAPON;
		}
	}

	// send the data

	//fjm: this wasn't in here b4, and the centerview command requires it.
	if ((int)ent->GetFlags() & QHFL_ONGROUND)
	{
		bits |= Q1SU_ONGROUND;
	}

	static int next_update = 0;
	static int next_count = 0;

	next_count++;
	if (next_count >= 3)
	{
		next_count = 0;
		next_update++;
		if (next_update > 11)
		{
			next_update = 0;
		}

		switch (next_update)
		{
		case 0: bits |= Q1SU_VIEWHEIGHT;
			break;
		case 1: bits |= Q1SU_IDEALPITCH;
			break;
		case 2: bits |= H2SU_IDEALROLL;
			break;
		case 3: bits |= Q1SU_VELOCITY1;
			break;
		case 4: bits |= (Q1SU_VELOCITY1 << 1);
			break;
		case 5: bits |= (Q1SU_VELOCITY1 << 2);
			break;
		case 6: bits |= Q1SU_PUNCH1;
			break;
		case 7: bits |= (Q1SU_PUNCH1 << 1);
			break;
		case 8: bits |= (Q1SU_PUNCH1 << 2);
			break;
		case 9: bits |= Q1SU_WEAPONFRAME;
			break;
		case 10: bits |= Q1SU_ARMOR;
			break;
		case 11: bits |= Q1SU_WEAPON;
			break;
		}
	}

	msg->WriteByte(h2svc_clientdata);
	msg->WriteShort(bits);

	if (bits & Q1SU_VIEWHEIGHT)
	{
		msg->WriteChar(ent->GetViewOfs()[2]);
	}

	if (bits & Q1SU_IDEALPITCH)
	{
		msg->WriteChar(ent->GetIdealPitch());
	}

	if (bits & H2SU_IDEALROLL)
	{
		msg->WriteChar(ent->GetIdealRoll());
	}

	for (int i = 0; i < 3; i++)
	{
		if (bits & (Q1SU_PUNCH1 << i))
		{
			msg->WriteChar(ent->GetPunchAngle()[i]);
		}
		if (bits & (Q1SU_VELOCITY1 << i))
		{
			msg->WriteChar(ent->GetVelocity()[i] / 16);
		}
	}

	if (bits & Q1SU_WEAPONFRAME)
	{
		msg->WriteByte(ent->GetWeaponFrame());
	}
	if (bits & Q1SU_ARMOR)
	{
		msg->WriteByte(ent->GetArmorValue());
	}
	if (bits & Q1SU_WEAPON)
	{
		msg->WriteShort(SVQH_ModelIndex(PR_GetString(ent->GetWeaponModel())));
	}

	int sc1, sc2;
	if (client->h2_send_all_v)
	{
		sc1 = sc2 = 0xffffffff;
		client->h2_send_all_v = false;
	}
	else
	{
		sc1 = sc2 = 0;

		if (ent->GetHealth() != client->h2_old_v.health)
		{
			sc1 |= SC1_HEALTH;
		}
		if (ent->GetLevel() != client->h2_old_v.level)
		{
			sc1 |= SC1_LEVEL;
		}
		if (ent->GetIntelligence() != client->h2_old_v.intelligence)
		{
			sc1 |= SC1_INTELLIGENCE;
		}
		if (ent->GetWisdom() != client->h2_old_v.wisdom)
		{
			sc1 |= SC1_WISDOM;
		}
		if (ent->GetStrength() != client->h2_old_v.strength)
		{
			sc1 |= SC1_STRENGTH;
		}
		if (ent->GetDexterity() != client->h2_old_v.dexterity)
		{
			sc1 |= SC1_DEXTERITY;
		}
		if (ent->GetWeapon() != client->h2_old_v.weapon)
		{
			sc1 |= SC1_WEAPON;
		}
		if (ent->GetBlueMana() != client->h2_old_v.bluemana)
		{
			sc1 |= SC1_BLUEMANA;
		}
		if (ent->GetGreenMana() != client->h2_old_v.greenmana)
		{
			sc1 |= SC1_GREENMANA;
		}
		if (ent->GetExperience() != client->h2_old_v.experience)
		{
			sc1 |= SC1_EXPERIENCE;
		}
		if (ent->GetCntTorch() != client->h2_old_v.cnt_torch)
		{
			sc1 |= SC1_CNT_TORCH;
		}
		if (ent->GetCntHBoost() != client->h2_old_v.cnt_h_boost)
		{
			sc1 |= SC1_CNT_H_BOOST;
		}
		if (ent->GetCntSHBoost() != client->h2_old_v.cnt_sh_boost)
		{
			sc1 |= SC1_CNT_SH_BOOST;
		}
		if (ent->GetCntManaBoost() != client->h2_old_v.cnt_mana_boost)
		{
			sc1 |= SC1_CNT_MANA_BOOST;
		}
		if (ent->GetCntTeleport() != client->h2_old_v.cnt_teleport)
		{
			sc1 |= SC1_CNT_TELEPORT;
		}
		if (ent->GetCntTome() != client->h2_old_v.cnt_tome)
		{
			sc1 |= SC1_CNT_TOME;
		}
		if (ent->GetCntSummon() != client->h2_old_v.cnt_summon)
		{
			sc1 |= SC1_CNT_SUMMON;
		}
		if (ent->GetCntInvisibility() != client->h2_old_v.cnt_invisibility)
		{
			sc1 |= SC1_CNT_INVISIBILITY;
		}
		if (ent->GetCntGlyph() != client->h2_old_v.cnt_glyph)
		{
			sc1 |= SC1_CNT_GLYPH;
		}
		if (ent->GetCntHaste() != client->h2_old_v.cnt_haste)
		{
			sc1 |= SC1_CNT_HASTE;
		}
		if (ent->GetCntBlast() != client->h2_old_v.cnt_blast)
		{
			sc1 |= SC1_CNT_BLAST;
		}
		if (ent->GetCntPolyMorph() != client->h2_old_v.cnt_polymorph)
		{
			sc1 |= SC1_CNT_POLYMORPH;
		}
		if (ent->GetCntFlight() != client->h2_old_v.cnt_flight)
		{
			sc1 |= SC1_CNT_FLIGHT;
		}
		if (ent->GetCntCubeOfForce() != client->h2_old_v.cnt_cubeofforce)
		{
			sc1 |= SC1_CNT_CUBEOFFORCE;
		}
		if (ent->GetCntInvincibility() != client->h2_old_v.cnt_invincibility)
		{
			sc1 |= SC1_CNT_INVINCIBILITY;
		}
		if (ent->GetArtifactActive() != client->h2_old_v.artifact_active)
		{
			sc1 |= SC1_ARTIFACT_ACTIVE;
		}
		if (ent->GetArtifactLow() != client->h2_old_v.artifact_low)
		{
			sc1 |= SC1_ARTIFACT_LOW;
		}
		if (ent->GetMoveType() != client->h2_old_v.movetype)
		{
			sc1 |= SC1_MOVETYPE;
		}
		if (ent->GetCameraMode() != client->h2_old_v.cameramode)
		{
			sc1 |= SC1_CAMERAMODE;
		}
		if (ent->GetHasted() != client->h2_old_v.hasted)
		{
			sc1 |= SC1_HASTED;
		}
		if (ent->GetInventory() != client->h2_old_v.inventory)
		{
			sc1 |= SC1_INVENTORY;
		}
		if (ent->GetRingsActive() != client->h2_old_v.rings_active)
		{
			sc1 |= SC1_RINGS_ACTIVE;
		}

		if (ent->GetRingsLow() != client->h2_old_v.rings_low)
		{
			sc2 |= SC2_RINGS_LOW;
		}
		if (ent->GetArmorAmulet() != client->h2_old_v.armor_amulet)
		{
			sc2 |= SC2_AMULET;
		}
		if (ent->GetArmorBracer() != client->h2_old_v.armor_bracer)
		{
			sc2 |= SC2_BRACER;
		}
		if (ent->GetArmorBreastPlate() != client->h2_old_v.armor_breastplate)
		{
			sc2 |= SC2_BREASTPLATE;
		}
		if (ent->GetArmorHelmet() != client->h2_old_v.armor_helmet)
		{
			sc2 |= SC2_HELMET;
		}
		if (ent->GetRingFlight() != client->h2_old_v.ring_flight)
		{
			sc2 |= SC2_FLIGHT_T;
		}
		if (ent->GetRingWater() != client->h2_old_v.ring_water)
		{
			sc2 |= SC2_WATER_T;
		}
		if (ent->GetRingTurning() != client->h2_old_v.ring_turning)
		{
			sc2 |= SC2_TURNING_T;
		}
		if (ent->GetRingRegeneration() != client->h2_old_v.ring_regeneration)
		{
			sc2 |= SC2_REGEN_T;
		}
		if (ent->GetHasteTime() != client->h2_old_v.haste_time)
		{
			sc2 |= SC2_HASTE_T;
		}
		if (ent->GetTomeTime() != client->h2_old_v.tome_time)
		{
			sc2 |= SC2_TOME_T;
		}
		if (ent->GetPuzzleInv1() != client->h2_old_v.puzzle_inv1)
		{
			sc2 |= SC2_PUZZLE1;
		}
		if (ent->GetPuzzleInv2() != client->h2_old_v.puzzle_inv2)
		{
			sc2 |= SC2_PUZZLE2;
		}
		if (ent->GetPuzzleInv3() != client->h2_old_v.puzzle_inv3)
		{
			sc2 |= SC2_PUZZLE3;
		}
		if (ent->GetPuzzleInv4() != client->h2_old_v.puzzle_inv4)
		{
			sc2 |= SC2_PUZZLE4;
		}
		if (ent->GetPuzzleInv5() != client->h2_old_v.puzzle_inv5)
		{
			sc2 |= SC2_PUZZLE5;
		}
		if (ent->GetPuzzleInv6() != client->h2_old_v.puzzle_inv6)
		{
			sc2 |= SC2_PUZZLE6;
		}
		if (ent->GetPuzzleInv7() != client->h2_old_v.puzzle_inv7)
		{
			sc2 |= SC2_PUZZLE7;
		}
		if (ent->GetPuzzleInv8() != client->h2_old_v.puzzle_inv8)
		{
			sc2 |= SC2_PUZZLE8;
		}
		if (ent->GetMaxHealth() != client->h2_old_v.max_health)
		{
			sc2 |= SC2_MAXHEALTH;
		}
		if (ent->GetMaxMana() != client->h2_old_v.max_mana)
		{
			sc2 |= SC2_MAXMANA;
		}
		if (ent->GetFlags() != client->h2_old_v.flags)
		{
			sc2 |= SC2_FLAGS;
		}
		if (static_cast<int>(info_mask) != client->h2_info_mask)
		{
			sc2 |= SC2_OBJ;
		}
		if (static_cast<int>(info_mask2) != client->h2_info_mask2)
		{
			sc2 |= SC2_OBJ2;
		}
	}

	byte test;

	if (!sc1 && !sc2)
	{
		goto end;
	}

	client->qh_message.WriteByte(h2svc_update_inv);
	test = 0;
	if (sc1 & 0x000000ff)
	{
		test |= 1;
	}
	if (sc1 & 0x0000ff00)
	{
		test |= 2;
	}
	if (sc1 & 0x00ff0000)
	{
		test |= 4;
	}
	if (sc1 & 0xff000000)
	{
		test |= 8;
	}
	if (sc2 & 0x000000ff)
	{
		test |= 16;
	}
	if (sc2 & 0x0000ff00)
	{
		test |= 32;
	}
	if (sc2 & 0x00ff0000)
	{
		test |= 64;
	}
	if (sc2 & 0xff000000)
	{
		test |= 128;
	}

	client->qh_message.WriteByte(test);

	if (test & 1)
	{
		client->qh_message.WriteByte(sc1 & 0xff);
	}
	if (test & 2)
	{
		client->qh_message.WriteByte((sc1 >> 8) & 0xff);
	}
	if (test & 4)
	{
		client->qh_message.WriteByte((sc1 >> 16) & 0xff);
	}
	if (test & 8)
	{
		client->qh_message.WriteByte((sc1 >> 24) & 0xff);
	}
	if (test & 16)
	{
		client->qh_message.WriteByte(sc2 & 0xff);
	}
	if (test & 32)
	{
		client->qh_message.WriteByte((sc2 >> 8) & 0xff);
	}
	if (test & 64)
	{
		client->qh_message.WriteByte((sc2 >> 16) & 0xff);
	}
	if (test & 128)
	{
		client->qh_message.WriteByte((sc2 >> 24) & 0xff);
	}

	if (sc1 & SC1_HEALTH)
	{
		client->qh_message.WriteShort(ent->GetHealth());
	}
	if (sc1 & SC1_LEVEL)
	{
		client->qh_message.WriteByte(ent->GetLevel());
	}
	if (sc1 & SC1_INTELLIGENCE)
	{
		client->qh_message.WriteByte(ent->GetIntelligence());
	}
	if (sc1 & SC1_WISDOM)
	{
		client->qh_message.WriteByte(ent->GetWisdom());
	}
	if (sc1 & SC1_STRENGTH)
	{
		client->qh_message.WriteByte(ent->GetStrength());
	}
	if (sc1 & SC1_DEXTERITY)
	{
		client->qh_message.WriteByte(ent->GetDexterity());
	}
	if (sc1 & SC1_WEAPON)
	{
		client->qh_message.WriteByte(ent->GetWeapon());
	}
	if (sc1 & SC1_BLUEMANA)
	{
		client->qh_message.WriteByte(ent->GetBlueMana());
	}
	if (sc1 & SC1_GREENMANA)
	{
		client->qh_message.WriteByte(ent->GetGreenMana());
	}
	if (sc1 & SC1_EXPERIENCE)
	{
		client->qh_message.WriteLong(ent->GetExperience());
	}
	if (sc1 & SC1_CNT_TORCH)
	{
		client->qh_message.WriteByte(ent->GetCntTorch());
	}
	if (sc1 & SC1_CNT_H_BOOST)
	{
		client->qh_message.WriteByte(ent->GetCntHBoost());
	}
	if (sc1 & SC1_CNT_SH_BOOST)
	{
		client->qh_message.WriteByte(ent->GetCntSHBoost());
	}
	if (sc1 & SC1_CNT_MANA_BOOST)
	{
		client->qh_message.WriteByte(ent->GetCntManaBoost());
	}
	if (sc1 & SC1_CNT_TELEPORT)
	{
		client->qh_message.WriteByte(ent->GetCntTeleport());
	}
	if (sc1 & SC1_CNT_TOME)
	{
		client->qh_message.WriteByte(ent->GetCntTome());
	}
	if (sc1 & SC1_CNT_SUMMON)
	{
		client->qh_message.WriteByte(ent->GetCntSummon());
	}
	if (sc1 & SC1_CNT_INVISIBILITY)
	{
		client->qh_message.WriteByte(ent->GetCntInvisibility());
	}
	if (sc1 & SC1_CNT_GLYPH)
	{
		client->qh_message.WriteByte(ent->GetCntGlyph());
	}
	if (sc1 & SC1_CNT_HASTE)
	{
		client->qh_message.WriteByte(ent->GetCntHaste());
	}
	if (sc1 & SC1_CNT_BLAST)
	{
		client->qh_message.WriteByte(ent->GetCntBlast());
	}
	if (sc1 & SC1_CNT_POLYMORPH)
	{
		client->qh_message.WriteByte(ent->GetCntPolyMorph());
	}
	if (sc1 & SC1_CNT_FLIGHT)
	{
		client->qh_message.WriteByte(ent->GetCntFlight());
	}
	if (sc1 & SC1_CNT_CUBEOFFORCE)
	{
		client->qh_message.WriteByte(ent->GetCntCubeOfForce());
	}
	if (sc1 & SC1_CNT_INVINCIBILITY)
	{
		client->qh_message.WriteByte(ent->GetCntInvincibility());
	}
	if (sc1 & SC1_ARTIFACT_ACTIVE)
	{
		client->qh_message.WriteFloat(ent->GetArtifactActive());
	}
	if (sc1 & SC1_ARTIFACT_LOW)
	{
		client->qh_message.WriteFloat(ent->GetArtifactLow());
	}
	if (sc1 & SC1_MOVETYPE)
	{
		client->qh_message.WriteByte(ent->GetMoveType());
	}
	if (sc1 & SC1_CAMERAMODE)
	{
		client->qh_message.WriteByte(ent->GetCameraMode());
	}
	if (sc1 & SC1_HASTED)
	{
		client->qh_message.WriteFloat(ent->GetHasted());
	}
	if (sc1 & SC1_INVENTORY)
	{
		client->qh_message.WriteByte(ent->GetInventory());
	}
	if (sc1 & SC1_RINGS_ACTIVE)
	{
		client->qh_message.WriteFloat(ent->GetRingsActive());
	}

	if (sc2 & SC2_RINGS_LOW)
	{
		client->qh_message.WriteFloat(ent->GetRingsLow());
	}
	if (sc2 & SC2_AMULET)
	{
		client->qh_message.WriteByte(ent->GetArmorAmulet());
	}
	if (sc2 & SC2_BRACER)
	{
		client->qh_message.WriteByte(ent->GetArmorBracer());
	}
	if (sc2 & SC2_BREASTPLATE)
	{
		client->qh_message.WriteByte(ent->GetArmorBreastPlate());
	}
	if (sc2 & SC2_HELMET)
	{
		client->qh_message.WriteByte(ent->GetArmorHelmet());
	}
	if (sc2 & SC2_FLIGHT_T)
	{
		client->qh_message.WriteByte(ent->GetRingFlight());
	}
	if (sc2 & SC2_WATER_T)
	{
		client->qh_message.WriteByte(ent->GetRingWater());
	}
	if (sc2 & SC2_TURNING_T)
	{
		client->qh_message.WriteByte(ent->GetRingTurning());
	}
	if (sc2 & SC2_REGEN_T)
	{
		client->qh_message.WriteByte(ent->GetRingRegeneration());
	}
	if (sc2 & SC2_HASTE_T)
	{
		client->qh_message.WriteFloat(ent->GetHasteTime());
	}
	if (sc2 & SC2_TOME_T)
	{
		client->qh_message.WriteFloat(ent->GetTomeTime());
	}
	if (sc2 & SC2_PUZZLE1)
	{
		client->qh_message.WriteString2(PR_GetString(ent->GetPuzzleInv1()));
	}
	if (sc2 & SC2_PUZZLE2)
	{
		client->qh_message.WriteString2(PR_GetString(ent->GetPuzzleInv2()));
	}
	if (sc2 & SC2_PUZZLE3)
	{
		client->qh_message.WriteString2(PR_GetString(ent->GetPuzzleInv3()));
	}
	if (sc2 & SC2_PUZZLE4)
	{
		client->qh_message.WriteString2(PR_GetString(ent->GetPuzzleInv4()));
	}
	if (sc2 & SC2_PUZZLE5)
	{
		client->qh_message.WriteString2(PR_GetString(ent->GetPuzzleInv5()));
	}
	if (sc2 & SC2_PUZZLE6)
	{
		client->qh_message.WriteString2(PR_GetString(ent->GetPuzzleInv6()));
	}
	if (sc2 & SC2_PUZZLE7)
	{
		client->qh_message.WriteString2(PR_GetString(ent->GetPuzzleInv7()));
	}
	if (sc2 & SC2_PUZZLE8)
	{
		client->qh_message.WriteString2(PR_GetString(ent->GetPuzzleInv8()));
	}
	if (sc2 & SC2_MAXHEALTH)
	{
		client->qh_message.WriteShort(ent->GetMaxHealth());
	}
	if (sc2 & SC2_MAXMANA)
	{
		client->qh_message.WriteByte(ent->GetMaxMana());
	}
	if (sc2 & SC2_FLAGS)
	{
		client->qh_message.WriteFloat(ent->GetFlags());
	}
	if (sc2 & SC2_OBJ)
	{
		client->qh_message.WriteLong(info_mask);
		client->h2_info_mask = info_mask;
	}
	if (sc2 & SC2_OBJ2)
	{
		client->qh_message.WriteLong(info_mask2);
		client->h2_info_mask2 = info_mask2;
	}

end:
	client->h2_old_v.movetype = ent->GetMoveType();
	VectorCopy(ent->GetVelocity(), client->h2_old_v.velocity);
	VectorCopy(ent->GetPunchAngle(), client->h2_old_v.punchangle);
	client->h2_old_v.weapon = ent->GetWeapon();
	client->h2_old_v.weaponmodel = ent->GetWeaponModel();
	client->h2_old_v.weaponframe = ent->GetWeaponFrame();
	client->h2_old_v.health = ent->GetHealth();
	client->h2_old_v.max_health = ent->GetMaxHealth();
	client->h2_old_v.bluemana = ent->GetBlueMana();
	client->h2_old_v.greenmana = ent->GetGreenMana();
	client->h2_old_v.max_mana = ent->GetMaxMana();
	client->h2_old_v.armor_amulet = ent->GetArmorAmulet();
	client->h2_old_v.armor_bracer = ent->GetArmorBracer();
	client->h2_old_v.armor_breastplate = ent->GetArmorBreastPlate();
	client->h2_old_v.armor_helmet = ent->GetArmorHelmet();
	client->h2_old_v.level = ent->GetLevel();
	client->h2_old_v.intelligence = ent->GetIntelligence();
	client->h2_old_v.wisdom = ent->GetWisdom();
	client->h2_old_v.dexterity = ent->GetDexterity();
	client->h2_old_v.strength = ent->GetStrength();
	client->h2_old_v.experience = ent->GetExperience();
	client->h2_old_v.ring_flight = ent->GetRingFlight();
	client->h2_old_v.ring_water = ent->GetRingWater();
	client->h2_old_v.ring_turning = ent->GetRingTurning();
	client->h2_old_v.ring_regeneration = ent->GetRingRegeneration();
	client->h2_old_v.haste_time = ent->GetHasteTime();
	client->h2_old_v.tome_time = ent->GetTomeTime();
	client->h2_old_v.puzzle_inv1 = ent->GetPuzzleInv1();
	client->h2_old_v.puzzle_inv2 = ent->GetPuzzleInv2();
	client->h2_old_v.puzzle_inv3 = ent->GetPuzzleInv3();
	client->h2_old_v.puzzle_inv4 = ent->GetPuzzleInv4();
	client->h2_old_v.puzzle_inv5 = ent->GetPuzzleInv5();
	client->h2_old_v.puzzle_inv6 = ent->GetPuzzleInv6();
	client->h2_old_v.puzzle_inv7 = ent->GetPuzzleInv7();
	client->h2_old_v.puzzle_inv8 = ent->GetPuzzleInv8();
	VectorCopy(ent->GetViewOfs(), client->h2_old_v.view_ofs);
	client->h2_old_v.idealpitch = ent->GetIdealPitch();
	client->h2_old_v.idealroll = ent->GetIdealRoll();
	client->h2_old_v.flags = ent->GetFlags();
	client->h2_old_v.armorvalue = ent->GetArmorValue();
	client->h2_old_v.rings_active = ent->GetRingsActive();
	client->h2_old_v.rings_low = ent->GetRingsLow();
	client->h2_old_v.artifact_active = ent->GetArtifactActive();
	client->h2_old_v.artifact_low = ent->GetArtifactLow();
	client->h2_old_v.hasted = ent->GetHasted();
	client->h2_old_v.inventory = ent->GetInventory();
	client->h2_old_v.cnt_torch = ent->GetCntTorch();
	client->h2_old_v.cnt_h_boost = ent->GetCntHBoost();
	client->h2_old_v.cnt_sh_boost = ent->GetCntSHBoost();
	client->h2_old_v.cnt_mana_boost = ent->GetCntManaBoost();
	client->h2_old_v.cnt_teleport = ent->GetCntTeleport();
	client->h2_old_v.cnt_tome = ent->GetCntTome();
	client->h2_old_v.cnt_summon = ent->GetCntSummon();
	client->h2_old_v.cnt_invisibility = ent->GetCntInvisibility();
	client->h2_old_v.cnt_glyph = ent->GetCntGlyph();
	client->h2_old_v.cnt_haste = ent->GetCntHaste();
	client->h2_old_v.cnt_blast = ent->GetCntBlast();
	client->h2_old_v.cnt_polymorph = ent->GetCntPolyMorph();
	client->h2_old_v.cnt_flight = ent->GetCntFlight();
	client->h2_old_v.cnt_cubeofforce = ent->GetCntCubeOfForce();
	client->h2_old_v.cnt_invincibility = ent->GetCntInvincibility();
	client->h2_old_v.cameramode = ent->GetCameraMode();
}

void SVQH_WriteClientdataToMessage(client_t* client, QMsg* msg)
{
	qhedict_t* ent = client->qh_edict;

	// send a damage message if the player got hit this frame
	if (ent->GetDmgTake() || ent->GetDmgSave())
	{
		qhedict_t* other = PROG_TO_EDICT(ent->GetDmgInflictor());
		msg->WriteByte(GGameType & GAME_Hexen2 ? h2svc_damage : q1svc_damage);
		msg->WriteByte(ent->GetDmgSave());
		msg->WriteByte(ent->GetDmgTake());
		for (int i = 0; i < 3; i++)
			msg->WriteCoord(other->GetOrigin()[i] + 0.5 * (other->GetMins()[i] + other->GetMaxs()[i]));

		ent->SetDmgTake(0);
		ent->SetDmgSave(0);
	}

	if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)))
	{
		// send the current viewpos offset from the view entity
		SVQH_SetIdealPitch(ent);			// how much to look up / down ideally
	}

	// a fixangle might get lost in a dropped packet.  Oh well.
	if (ent->GetFixAngle())
	{
		msg->WriteByte(GGameType & GAME_Hexen2 ? h2svc_setangle : q1svc_setangle);
		for (int i = 0; i < 3; i++)
			msg->WriteAngle(ent->GetAngles()[i]);
		ent->SetFixAngle(0);
	}

	if (GGameType & GAME_Quake && !(GGameType & GAME_QuakeWorld))
	{
		SVQ1_WriteClientdataToMessage(ent, msg);
	}

	if (GGameType & GAME_Hexen2 && !(GGameType & GAME_HexenWorld))
	{
		SVH2_WriteClientdataToMessage(client, ent, msg);
	}

	// if the player has a target, send its info...
	if (GGameType & GAME_HexenWorld && ent->GetTargDist())
	{
		msg->WriteByte(hwsvc_targetupdate);
		msg->WriteByte(ent->GetTargAng());
		msg->WriteByte(ent->GetTargPitch());
		msg->WriteByte((ent->GetTargDist() < 255.0) ? (int)(ent->GetTargDist()) : 255);
	}
}

static void SVQHW_FlushRedirect(char* buffer)
{
	char send[8000 + 6];

	send[0] = 0xff;
	send[1] = 0xff;
	send[2] = 0xff;
	send[3] = 0xff;
	send[4] = A2C_PRINT;
	Com_Memcpy(send + 5, buffer, String::Length(buffer) + 1);

	NET_SendPacket(NS_SERVER, String::Length(send) + 1, send, svs.redirectAddress);

	// clear it
	buffer[0] = 0;
}

//  Send common->Printf data to the remote client instead of the console
void SVQHW_BeginRedirect(const netadr_t& addr)
{
	svs.redirectAddress = addr;
	Com_BeginRedirect(outputbuf, sizeof(outputbuf), SVQHW_FlushRedirect);
}

void SVQW_FindModelNumbers()
{
	svqw_nailmodel = -1;
	svqw_supernailmodel = -1;
	svqw_playermodel = -1;

	for (int i = 0; i < MAX_MODELS_Q1; i++)
	{
		if (!sv.qh_model_precache[i])
		{
			break;
		}
		if (!String::Cmp(sv.qh_model_precache[i],"progs/spike.mdl"))
		{
			svqw_nailmodel = i;
		}
		if (!String::Cmp(sv.qh_model_precache[i],"progs/s_spike.mdl"))
		{
			svqw_supernailmodel = i;
		}
		if (!String::Cmp(sv.qh_model_precache[i],"progs/player.mdl"))
		{
			svqw_playermodel = i;
		}
	}
}

void SVHW_FindModelNumbers()
{
	svhw_playermodel[0] = -1;
	svhw_playermodel[1] = -1;
	svhw_playermodel[2] = -1;
	svhw_playermodel[3] = -1;
	svhw_playermodel[4] = -1;
	svhw_magicmissmodel = -1;
	svhw_ravenmodel = -1;
	svhw_raven2model = -1;

	for (int i = 0; i < MAX_MODELS_H2; i++)
	{
		if (!sv.qh_model_precache[i])
		{
			break;
		}
		if (!String::Cmp(sv.qh_model_precache[i],"models/paladin.mdl"))
		{
			svhw_playermodel[0] = i;
		}
		if (!String::Cmp(sv.qh_model_precache[i],"models/crusader.mdl"))
		{
			svhw_playermodel[1] = i;
		}
		if (!String::Cmp(sv.qh_model_precache[i],"models/necro.mdl"))
		{
			svhw_playermodel[2] = i;
		}
		if (!String::Cmp(sv.qh_model_precache[i],"models/assassin.mdl"))
		{
			svhw_playermodel[3] = i;
		}
		if (!String::Cmp(sv.qh_model_precache[i],"models/succubus.mdl"))
		{
			svhw_playermodel[4] = i;
		}
		if (!String::Cmp(sv.qh_model_precache[i],"models/ball.mdl"))
		{
			svhw_magicmissmodel = i;
		}
		if (!String::Cmp(sv.qh_model_precache[i],"models/ravproj.mdl"))
		{
			svhw_ravenmodel = i;
		}
		if (!String::Cmp(sv.qh_model_precache[i],"models/vindsht1.mdl"))
		{
			svhw_raven2model = i;
		}
	}
}

//	Performs a delta update of the stats array.  This should only be performed
// when a reliable message can be delivered this frame.
static void SVQHW_UpdateClientStats(client_t* client)
{
	qhedict_t* ent = client->qh_edict;
	// if we are a spectator and we are tracking a player, we get his stats
	// so our status bar reflects his
	if (client->qh_spectator && client->qh_spec_track > 0)
	{
		ent = svs.clients[client->qh_spec_track - 1].qh_edict;
	}

	int stats[MAX_CL_STATS];
	Com_Memset(stats, 0, sizeof(stats));
	stats[Q1STAT_WEAPON] = SVQH_ModelIndex(PR_GetString(ent->GetWeaponModel()));
	if (GGameType & GAME_QuakeWorld)
	{
		stats[Q1STAT_HEALTH] = ent->GetHealth();
		stats[Q1STAT_AMMO] = ent->GetCurrentAmmo();
		stats[Q1STAT_ARMOR] = ent->GetArmorValue();
		stats[Q1STAT_SHELLS] = ent->GetAmmoShells();
		stats[Q1STAT_NAILS] = ent->GetAmmoNails();
		stats[Q1STAT_ROCKETS] = ent->GetAmmoRockets();
		stats[Q1STAT_CELLS] = ent->GetAmmoCells();
		if (!client->qh_spectator)
		{
			stats[Q1STAT_ACTIVEWEAPON] = ent->GetWeapon();
		}
		// stuff the sigil bits into the high bits of items for sbar
		stats[QWSTAT_ITEMS] = (int)ent->GetItems() | ((int)*pr_globalVars.serverflags << 28);
	}

	for (int i = 0; i < MAX_CL_STATS; i++)
	{
		if (stats[i] != client->qh_stats[i])
		{
			client->qh_stats[i] = stats[i];
			if (stats[i] >= 0 && stats[i] <= 255)
			{
				SVQH_ClientReliableWrite_Begin(client, GGameType & GAME_HexenWorld ? h2svc_updatestat : q1svc_updatestat, 3);
				SVQH_ClientReliableWrite_Byte(client, i);
				SVQH_ClientReliableWrite_Byte(client, stats[i]);
			}
			else
			{
				SVQH_ClientReliableWrite_Begin(client, GGameType & GAME_HexenWorld ? hwsvc_updatestatlong : qwsvc_updatestatlong, 6);
				SVQH_ClientReliableWrite_Byte(client, i);
				SVQH_ClientReliableWrite_Long(client, stats[i]);
			}
		}
	}
}

static bool SVQH_SendClientDatagram(client_t* client)
{
	byte buf[MAX_MSGLEN];
	QMsg msg;
	msg.InitOOB(buf, GGameType & GAME_Hexen2 ? MAX_MSGLEN_H2 : MAX_DATAGRAM_QH);

	msg.WriteByte(GGameType & GAME_Hexen2 ? h2svc_time : q1svc_time);
	msg.WriteFloat(sv.qh_time * 0.001f);

	// add the client specific data to the datagram
	SVQH_WriteClientdataToMessage(client, &msg);

	if (GGameType & GAME_Hexen2)
	{
		SVH2_PrepareClientEntities(client, client->qh_edict, &msg);
	}
	else
	{
		SVQ1_WriteEntitiesToClient(client->qh_edict, &msg);
	}

	// copy the server datagram if there is space
	if (msg.cursize + sv.qh_datagram.cursize < msg.maxsize)
	{
		msg.WriteData(sv.qh_datagram._data, sv.qh_datagram.cursize);
	}

	if (GGameType & GAME_Hexen2)
	{
		if (msg.cursize + client->datagram.cursize < msg.maxsize)
		{
			msg.WriteData(client->datagram._data, client->datagram.cursize);
		}

		client->datagram.Clear();
	}

	// send the datagram
	if (NET_SendUnreliableMessage(&client->netchan, &msg) == -1)
	{
		SVQH_DropClient(client, true);// if the message couldn't send, kick off
		return false;
	}

	return true;
}

static bool SVQHW_SendClientDatagram(client_t* client)
{
	byte buf[MAX_DATAGRAM];
	QMsg msg;
	msg.InitOOB(buf, GGameType & GAME_HexenWorld ? MAX_DATAGRAM_HW : MAX_DATAGRAM_QW);
	msg.allowoverflow = true;

	// add the client specific data to the datagram
	SVQH_WriteClientdataToMessage(client, &msg);

	// send over all the objects that are in the PVS
	// this will include clients, a packetentities, and
	// possibly a nails update
	if (GGameType & GAME_HexenWorld)
	{
		SVHW_WriteEntitiesToClient(client, &msg);
	}
	else
	{
		SVQW_WriteEntitiesToClient(client, &msg);
	}

	// copy the accumulated multicast datagram
	// for this client out to the message
	if (client->datagram.overflowed)
	{
		common->Printf("WARNING: datagram overflowed for %s\n", client->name);
	}
	else
	{
		msg.WriteData(client->datagram._data, client->datagram.cursize);
	}
	client->datagram.Clear();

	// send deltas over reliable stream
	if (Netchan_CanReliable(&client->netchan))
	{
		SVQHW_UpdateClientStats(client);
	}

	if (msg.overflowed)
	{
		common->Printf("WARNING: msg overflowed for %s\n", client->name);
		msg.Clear();
	}

	// send the datagram
	Netchan_Transmit(&client->netchan, msg.cursize, buf);

	return true;
}

static void SVQH_UpdateToReliableMessages()
{
	// check for changes to be sent over the reliable streams
	client_t* host_client = svs.clients;
	for (int i = 0; i < svs.qh_maxclients; i++, host_client++)
	{
		if (host_client->qh_old_frags != host_client->qh_edict->GetFrags())
		{
			client_t* client = svs.clients;
			for (int j = 0; j < svs.qh_maxclients; j++, client++)
			{
				if (client->state < CS_CONNECTED)
				{
					continue;
				}
				client->qh_message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_updatefrags : q1svc_updatefrags);
				client->qh_message.WriteByte(i);
				client->qh_message.WriteShort(host_client->qh_edict->GetFrags());
			}

			host_client->qh_old_frags = host_client->qh_edict->GetFrags();
		}
	}

	client_t* client = svs.clients;
	for (int j = 0; j < svs.qh_maxclients; j++, client++)
	{
		if (client->state < CS_CONNECTED)
		{
			continue;
		}
		client->qh_message.WriteData(sv.qh_reliable_datagram._data, sv.qh_reliable_datagram.cursize);
	}

	sv.qh_reliable_datagram.Clear();
}

static bool ValidToShowName(qhedict_t* edict)
{
	if (edict->GetDeadFlag())
	{
		return false;
	}
	if ((int)edict->GetEffects() & H2EF_NODRAW)
	{
		return false;
	}
	return true;
}

static void UpdatePIV()
{
	client_t* host_client = svs.clients;
	for (int i = 0; i < MAX_CLIENTS_QHW; i++, host_client++)
	{
		host_client->hw_PIV = 0;
	}

	host_client = svs.clients;
	for (int i = 0; i < MAX_CLIENTS_QHW; i++, host_client++)
	{
		if (host_client->state != CS_ACTIVE || host_client->qh_spectator)
		{
			continue;
		}

		vec3_t adjust_org1;
		VectorCopy(host_client->qh_edict->GetOrigin(), adjust_org1);
		adjust_org1[2] += 24;

		client_t* client = host_client + 1;
		for (int j = i + 1; j < MAX_CLIENTS_QHW; j++, client++)
		{
			if (client->state != CS_ACTIVE || client->qh_spectator)
			{
				continue;
			}

			vec3_t distvec;
			VectorSubtract(client->qh_edict->GetOrigin(), host_client->qh_edict->GetOrigin(), distvec);
			float dist = VectorNormalize(distvec);
			if (dist > svhw_namedistance->value)
			{
				continue;
			}

			vec3_t adjust_org2;
			VectorCopy(client->qh_edict->GetOrigin(), adjust_org2);
			adjust_org2[2] += 24;

			q1trace_t trace = SVQH_MoveHull0(adjust_org1, vec3_origin, vec3_origin, adjust_org2, false, host_client->qh_edict);
			if (QH_EDICT_NUM(trace.entityNum) == client->qh_edict)
			{
				//can see each other, check for invisible, dead
				if (ValidToShowName(client->qh_edict))
				{
					host_client->hw_PIV |= 1 << j;
				}
				if (ValidToShowName(host_client->qh_edict))
				{
					client->hw_PIV |= 1 << i;
				}
			}
		}
	}
}

static void SVQHW_UpdateToReliableMessages()
{
	bool CheckPIV = false;
	if (GGameType & GAME_HexenWorld && sv.qh_time - sv.hw_next_PIV_time * 1000 >= 1000)
	{
		sv.hw_next_PIV_time = sv.qh_time * 0.001f + 1;
		CheckPIV = true;
		UpdatePIV();
	}

	// check for changes to be sent over the reliable streams to all clients
	client_t* host_client = svs.clients;
	for (int i = 0; i < MAX_CLIENTS_QHW; i++, host_client++)
	{
		if (host_client->state != CS_ACTIVE)
		{
			continue;
		}
		if (host_client->qh_sendinfo)
		{
			host_client->qh_sendinfo = false;
			SVQHW_FullClientUpdate(host_client, &sv.qh_reliable_datagram);
		}
		if (host_client->qh_old_frags != host_client->qh_edict->GetFrags())
		{
			client_t* client = svs.clients;
			for (int j = 0; j < MAX_CLIENTS_QHW; j++, client++)
			{
				if (client->state < CS_CONNECTED)
				{
					continue;
				}
				if (GGameType & GAME_HexenWorld)
				{
					client->netchan.message.WriteByte(hwsvc_updatedminfo);
					client->netchan.message.WriteByte(i);
					client->netchan.message.WriteShort(host_client->qh_edict->GetFrags());
					client->netchan.message.WriteByte((host_client->h2_playerclass << 5) | ((int)host_client->qh_edict->GetLevel() & 31));

					if (hw_dmMode->value == HWDM_SIEGE)
					{
						client->netchan.message.WriteByte(hwsvc_updatesiegelosses);
						client->netchan.message.WriteByte(*pr_globalVars.defLosses);
						client->netchan.message.WriteByte(*pr_globalVars.attLosses);
					}
				}
				else
				{
					SVQH_ClientReliableWrite_Begin(client, q1svc_updatefrags, 4);
					SVQH_ClientReliableWrite_Byte(client, i);
					SVQH_ClientReliableWrite_Short(client, host_client->qh_edict->GetFrags());
				}
			}

			host_client->qh_old_frags = host_client->qh_edict->GetFrags();
		}

		if (GGameType & GAME_HexenWorld)
		{
			SVHW_WriteInventory(host_client, host_client->qh_edict, &host_client->netchan.message);

			if (CheckPIV && host_client->hw_PIV != host_client->hw_LastPIV)
			{
				host_client->netchan.message.WriteByte(hwsvc_update_piv);
				host_client->netchan.message.WriteLong(host_client->hw_PIV);
				host_client->hw_LastPIV = host_client->hw_PIV;
			}
		}

		// maxspeed/entgravity changes
		qhedict_t* ent = host_client->qh_edict;

		eval_t* val = GetEdictFieldValue(ent, "gravity");
		if (val && host_client->qh_entgravity != val->_float)
		{
			host_client->qh_entgravity = val->_float;
			SVQH_ClientReliableWrite_Begin(host_client, GGameType & GAME_HexenWorld ? hwsvc_entgravity : qwsvc_entgravity, 5);
			SVQH_ClientReliableWrite_Float(host_client, host_client->qh_entgravity);
		}
		val = GetEdictFieldValue(ent, "maxspeed");
		if (val && host_client->qh_maxspeed != val->_float)
		{
			host_client->qh_maxspeed = val->_float;
			SVQH_ClientReliableWrite_Begin(host_client, GGameType & GAME_HexenWorld ? hwsvc_maxspeed : qwsvc_maxspeed, 5);
			SVQH_ClientReliableWrite_Float(host_client, host_client->qh_maxspeed);
		}

	}

	if (sv.qh_datagram.overflowed)
	{
		sv.qh_datagram.Clear();
	}

	// append the broadcast messages to each client messages
	client_t* client = svs.clients;
	for (int j = 0; j < MAX_CLIENTS_QHW; j++, client++)
	{
		if (client->state < CS_CONNECTED)
		{
			continue;	// reliables go to all connected or spawned
		}
		SVQH_ClientReliableCheckBlock(client, sv.qh_reliable_datagram.cursize);
		SVQH_ClientReliableWrite_SZ(client, sv.qh_reliable_datagram._data, sv.qh_reliable_datagram.cursize);

		if (client->state != CS_ACTIVE)
		{
			continue;	// datagrams only go to spawned
		}
		client->datagram.WriteData(sv.qh_datagram._data, sv.qh_datagram.cursize);
	}

	sv.qh_reliable_datagram.Clear();
	sv.qh_datagram.Clear();
}

static void SVQH_CleanupEnts()
{
	qhedict_t* ent = NEXT_EDICT(sv.qh_edicts);
	for (int e = 1; e < sv.qh_num_edicts; e++, ent = NEXT_EDICT(ent))
	{
		ent->SetEffects((int)ent->GetEffects() & (GGameType & GAME_Hexen2 ? ~H2EF_MUZZLEFLASH : ~Q1EF_MUZZLEFLASH));
		if (GGameType& GAME_HexenWorld)
		{
			ent->SetWpnSound(0);
		}
	}
}

//	Send a nop message without trashing or sending the accumulated client message buffer
static void SVQH_SendNop(client_t* client)
{
	QMsg msg;
	byte buf[4];
	msg.InitOOB(buf, sizeof(buf));

	msg.WriteChar(GGameType & GAME_Hexen2 ? h2svc_nop : q1svc_nop);

	if (NET_SendUnreliableMessage(&client->netchan, &msg) == -1)
	{
		SVQH_DropClient(client, true);	// if the message couldn't send, kick off
	}
	client->qh_last_message = svs.realtime * 0.001;
}

void SVQH_SendClientMessages()
{
	// update frags, names, etc
	SVQH_UpdateToReliableMessages();

	// build individual updates
	client_t* host_client = svs.clients;
	for (int i = 0; i < svs.qh_maxclients; i++, host_client++)
	{
		if (host_client->state < CS_CONNECTED)
		{
			continue;
		}

		if (host_client->state == CS_ACTIVE)
		{
			if (!SVQH_SendClientDatagram(host_client))
			{
				continue;
			}
		}
		else
		{
			// the player isn't totally in the game yet
			// send small keepalive messages if too much time has passed
			// send a full message when the next signon stage has been requested
			// some other message data (name changes, etc) may accumulate
			// between signon stages
			if (!host_client->qh_sendsignon)
			{
				if (svs.realtime * 0.001 - host_client->qh_last_message > 5)
				{
					SVQH_SendNop(host_client);
				}
				continue;	// don't send out non-signon messages
			}
		}

		// check for an overflowed message.  Should only happen
		// on a very fucked up connection that backs up a lot, then
		// changes level
		if (host_client->qh_message.overflowed)
		{
			SVQH_DropClient(host_client, true);
			host_client->qh_message.overflowed = false;
			continue;
		}

		if (host_client->qh_message.cursize || host_client->qh_dropasap)
		{
			if (!NET_CanSendMessage(&host_client->netchan))
			{
				continue;
			}

			if (host_client->qh_dropasap)
			{
				SVQH_DropClient(host_client, false);	// went to another level
			}
			else
			{
				if (NET_SendMessage(&host_client->netchan, &host_client->qh_message) == -1)
				{
					SVQH_DropClient(host_client, true);	// if the message couldn't send, kick off
				}
				host_client->qh_message.Clear();
				host_client->qh_last_message = svs.realtime * 0.001;
				host_client->qh_sendsignon = false;
			}
		}
	}

	// clear muzzle flashes
	SVQH_CleanupEnts();
}

void SVQHW_SendClientMessages()
{
	// update frags, names, etc
	SVQHW_UpdateToReliableMessages();

	// build individual updates
	client_t* c = svs.clients;
	for (int i = 0; i < MAX_CLIENTS_QHW; i++, c++)
	{
		if (!c->state)
		{
			continue;
		}

		if (GGameType & GAME_QuakeWorld && c->qw_drop)
		{
			SVQHW_DropClient(c);
			c->qw_drop = false;
			continue;
		}

		// check to see if we have a backbuf to stick in the reliable
		if (c->qw_num_backbuf)
		{
			// will it fit?
			if (c->netchan.message.cursize + c->qw_backbuf_size[0] <
				c->netchan.message.maxsize)
			{

				common->DPrintf("%s: backbuf %d bytes\n",
					c->name, c->qw_backbuf_size[0]);

				// it'll fit
				c->netchan.message.WriteData(c->qw_backbuf_data[0],
					c->qw_backbuf_size[0]);

				//move along, move along
				for (int j = 1; j < c->qw_num_backbuf; j++)
				{
					Com_Memcpy(c->qw_backbuf_data[j - 1], c->qw_backbuf_data[j],
						c->qw_backbuf_size[j]);
					c->qw_backbuf_size[j - 1] = c->qw_backbuf_size[j];
				}

				c->qw_num_backbuf--;
				if (c->qw_num_backbuf)
				{
					c->qw_backbuf.InitOOB(c->qw_backbuf_data[c->qw_num_backbuf - 1], sizeof(c->qw_backbuf_data[c->qw_num_backbuf - 1]));
					c->qw_backbuf.cursize = c->qw_backbuf_size[c->qw_num_backbuf - 1];
				}
			}
		}

		// if the reliable message overflowed,
		// drop the client
		if (c->netchan.message.overflowed)
		{
			c->netchan.message.Clear();
			c->datagram.Clear();
			SVQH_BroadcastPrintf(PRINT_HIGH, "%s overflowed\n", c->name);
			common->Printf("WARNING: reliable overflow for %s\n",c->name);
			SVQHW_DropClient(c);
			c->qh_send_message = true;
		}

		// only send messages if the client has sent one
		// and the bandwidth is not choked
		if (!c->qh_send_message)
		{
			continue;
		}
		c->qh_send_message = false;	// try putting this after choke?

		if (c->state == CS_ACTIVE)
		{
			SVQHW_SendClientDatagram(c);
		}
		else
		{
			Netchan_Transmit(&c->netchan, 0, NULL);		// just update reliable

		}
	}

	if (GGameType & GAME_HexenWorld)
	{
		// clear muzzle flashes & wpn_sound
		SVQH_CleanupEnts();
	}
}

//	FIXME: does this sequence right?
void SVQHW_SendMessagesToAll()
{
	int i;
	client_t* c;

	for (i = 0, c = svs.clients; i < MAX_CLIENTS_QHW; i++, c++)
		if (c->state)		// FIXME: should this only send to active?
		{
			c->qh_send_message = true;
		}

	SVQHW_SendClientMessages();
}
