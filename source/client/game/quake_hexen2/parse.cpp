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

#include "parse.h"
#include "../../client_main.h"
#include "../../ui/ui.h"
#include "../light_styles.h"
#include "../../../common/player_move.h"

void CLQH_ParseTime(QMsg& message)
{
	cl.qh_mtime[1] = cl.qh_mtime[0];
	cl.qh_mtime[0] = message.ReadFloat();
}

void CLQH_ParseDisconnect()
{
	common->EndGame("Server disconnected\n");
}

void CLQH_ParseSetAngle(QMsg& message)
{
	for (int i = 0; i < 3; i++)
	{
		cl.viewangles[i] = message.ReadAngle();
	}
}

void CLQH_ParseSetView(QMsg& message)
{
	cl.viewentity = message.ReadShort();
}

void CLQH_ParseLightStyle(QMsg& message)
{
	int i = message.ReadByte();
	CL_SetLightStyle(i, message.ReadString2());
}

void CLQH_ParseStartSoundPacket(QMsg& message, int overflowMask)
{
	int field_mask = message.ReadByte();

	int volume;
	if (field_mask & QHSND_VOLUME)
	{
		volume = message.ReadByte();
	}
	else
	{
		volume = QHDEFAULT_SOUND_PACKET_VOLUME;
	}

	float attenuation;
	if (field_mask & QHSND_ATTENUATION)
	{
		attenuation = message.ReadByte() / 64.0;
	}
	else
	{
		attenuation = QHDEFAULT_SOUND_PACKET_ATTENUATION;
	}

	int channel = message.ReadShort();
	int sound_num = message.ReadByte();
	if (overflowMask && field_mask & overflowMask)
	{
		sound_num += 255;
	}

	int ent = channel >> 3;
	channel &= 7;

	if (ent > MAX_EDICTS_QH)
	{
		common->Error("CL_ParseStartSoundPacket: ent = %i", ent);
	}

	vec3_t pos;
	message.ReadPos(pos);

	S_StartSound(pos, ent, channel, cl.sound_precache[sound_num], volume / 255.0, attenuation);
}

void CLQHW_ParseStartSoundPacket(QMsg& message, float attenuationScale)
{
	int channel = message.ReadShort();

	int volume;
	if (channel & QHWSND_VOLUME)
	{
		volume = message.ReadByte();
	}
	else
	{
		volume = QHDEFAULT_SOUND_PACKET_VOLUME;
	}

	float attenuation;
	if (channel & QHWSND_ATTENUATION)
	{
		attenuation = message.ReadByte() / attenuationScale;
	}
	else
	{
		attenuation = QHDEFAULT_SOUND_PACKET_ATTENUATION;
	}

	int sound_num = message.ReadByte();

	vec3_t pos;
	message.ReadPos(pos);

	int ent = (channel >> 3) & 1023;
	channel &= 7;

	if (ent > MAX_EDICTS_QH)
	{
		common->Error("CLQHW_ParseStartSoundPacket: ent = %i", ent);
	}

	S_StartSound(pos, ent, channel, cl.sound_precache[sound_num], volume / 255.0, attenuation);
}

void CLQH_ParseStopSound(QMsg& message)
{
	int i = message.ReadShort();
	S_StopSound(i >> 3, i & 7);
}

void CLQH_ParseSetPause(QMsg& message)
{
	cl.qh_paused = message.ReadByte();

	if (cl.qh_paused)
	{
		CDAudio_Pause();
	}
	else
	{
		CDAudio_Resume();
	}
}

void CLQH_ParseKilledMonster()
{
	cl.qh_stats[Q1STAT_MONSTERS]++;
}

void CLQH_ParseFoundSecret()
{
	cl.qh_stats[Q1STAT_SECRETS]++;
}

void CLQH_ParseUpdateStat(QMsg& message)
{
	int i = message.ReadByte();
	if (i < 0 || i >= MAX_CL_STATS)
	{
		common->Error("svc_updatestat: %i is invalid", i);
	}
	cl.qh_stats[i] = message.ReadLong();
}

//	Except Hexen 2
void CLQH_ParseStaticSound(QMsg& message)
{
	vec3_t org;
	message.ReadPos(org);
	int sound_num = message.ReadByte();
	int vol = message.ReadByte();
	int atten = message.ReadByte();

	S_StaticSound(cl.sound_precache[sound_num], org, vol, atten);
}

void CLQHW_ParseCDTrack(QMsg& message)
{
	byte cdtrack = message.ReadByte();
	CDAudio_Play(cdtrack, true);
}

void CLQH_ParseSellScreen()
{
	Cmd_ExecuteString("help");
}

void CLQHW_ParseFinale(QMsg& message)
{
	cl.qh_intermission = 2;
	cl.qh_completed_time = cls.realtime * 0.001;
	SCR_CenterPrint(message.ReadString2());
}

void CLQHW_ParseSmallKick()
{
	cl.qh_punchangle = -2;
}

void CLQHW_ParseBigKick()
{
	cl.qh_punchangle = -4;
}

void CLQHW_ParseMaxSpeed(QMsg& message)
{
	movevars.maxspeed = message.ReadFloat();
}

void CLQHW_ParseEntGravity(QMsg& message)
{
	movevars.entgravity = message.ReadFloat();
}
