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
#include "../quake_hexen2/parse.h"

//	Server information pertaining to this client only
void CLQ1_ParseClientdata(QMsg& message)
{
	int bits = message.ReadShort();

	if (bits & Q1SU_VIEWHEIGHT)
	{
		cl.qh_viewheight = message.ReadChar();
	}
	else
	{
		cl.qh_viewheight = Q1DEFAULT_VIEWHEIGHT;
	}

	if (bits & Q1SU_IDEALPITCH)
	{
		cl.qh_idealpitch = message.ReadChar();
	}
	else
	{
		cl.qh_idealpitch = 0;
	}

	VectorCopy(cl.qh_mvelocity[0], cl.qh_mvelocity[1]);
	for (int i = 0; i < 3; i++)
	{
		if (bits & (Q1SU_PUNCH1 << i))
		{
			cl.qh_punchangles[i] = message.ReadChar();
		}
		else
		{
			cl.qh_punchangles[i] = 0;
		}
		if (bits & (Q1SU_VELOCITY1 << i))
		{
			cl.qh_mvelocity[0][i] = message.ReadChar() * 16;
		}
		else
		{
			cl.qh_mvelocity[0][i] = 0;
		}
	}

	// [always sent]	if (bits & Q1SU_ITEMS)
	int i = message.ReadLong();

	if (cl.q1_items != i)
	{
		// set flash times
		for (int j = 0; j < 32; j++)
		{
			if ((i & (1 << j)) && !(cl.q1_items & (1 << j)))
			{
				cl.q1_item_gettime[j] = cl.qh_serverTimeFloat;
			}
		}
		cl.q1_items = i;
	}

	cl.qh_onground = (bits & Q1SU_ONGROUND) != 0;

	if (bits & Q1SU_WEAPONFRAME)
	{
		cl.qh_stats[Q1STAT_WEAPONFRAME] = message.ReadByte();
	}
	else
	{
		cl.qh_stats[Q1STAT_WEAPONFRAME] = 0;
	}

	if (bits & Q1SU_ARMOR)
	{
		i = message.ReadByte();
	}
	else
	{
		i = 0;
	}
	if (cl.qh_stats[Q1STAT_ARMOR] != i)
	{
		cl.qh_stats[Q1STAT_ARMOR] = i;
	}

	if (bits & Q1SU_WEAPON)
	{
		i = message.ReadByte();
	}
	else
	{
		i = 0;
	}
	if (cl.qh_stats[Q1STAT_WEAPON] != i)
	{
		cl.qh_stats[Q1STAT_WEAPON] = i;
	}

	i = message.ReadShort();
	if (cl.qh_stats[Q1STAT_HEALTH] != i)
	{
		cl.qh_stats[Q1STAT_HEALTH] = i;
	}

	i = message.ReadByte();
	if (cl.qh_stats[Q1STAT_AMMO] != i)
	{
		cl.qh_stats[Q1STAT_AMMO] = i;
	}

	for (i = 0; i < 4; i++)
	{
		int j = message.ReadByte();
		if (cl.qh_stats[Q1STAT_SHELLS + i] != j)
		{
			cl.qh_stats[Q1STAT_SHELLS + i] = j;
		}
	}

	i = message.ReadByte();

	if (q1_standard_quake)
	{
		if (cl.qh_stats[Q1STAT_ACTIVEWEAPON] != i)
		{
			cl.qh_stats[Q1STAT_ACTIVEWEAPON] = i;
		}
	}
	else
	{
		if (cl.qh_stats[Q1STAT_ACTIVEWEAPON] != (1 << i))
		{
			cl.qh_stats[Q1STAT_ACTIVEWEAPON] = (1 << i);
		}
	}
}

void CLQ1_ParseVersion(QMsg& message)
{
	int i = message.ReadLong();
	if (i != Q1PROTOCOL_VERSION)
	{
		common->Error("CL_ParseServerMessage: Server is protocol %i instead of %i\n", i, Q1PROTOCOL_VERSION);
	}
}

void CLQW_ParseDisconnect()
{
	if (cls.state == CA_CONNECTED)
	{
		common->Error("Server disconnected\n"
						"Server version may not be compatible");
	}
	else
	{
		common->Error("Server disconnected");
	}
}

void CLQ1_ParsePrint(QMsg& message)
{
	const char* txt = message.ReadString2();

	if (txt[0] == 1)
	{
		S_StartLocalSound("misc/talk.wav");
	}
	if (txt[0] == 1 || txt[0] == 2)
	{
		common->Printf(S_COLOR_ORANGE "%s" S_COLOR_WHITE, txt + 1);
	}
	else
	{
		common->Printf("%s", txt);
	}
}

void CLQW_ParsePrint(QMsg& message)
{
	int i = message.ReadByte();
	const char* txt = message.ReadString2();

	if (i == PRINT_CHAT)
	{
		S_StartLocalSound("misc/talk.wav");
		common->Printf(S_COLOR_ORANGE "%s" S_COLOR_WHITE, txt);
	}
	else
	{
		common->Printf("%s", txt);
	}
}

void CLQ1_ParseStartSoundPacket(QMsg& message)
{
	CLQH_ParseStartSoundPacket(message, 0);
}

void CLQW_ParseStartSoundPacket(QMsg& message)
{
	CLQHW_ParseStartSoundPacket(message, 64.0);
}

void CLQ1_UpdateName(QMsg& message)
{
	int i = message.ReadByte();
	if (i >= cl.qh_maxclients)
	{
		common->Error("CL_ParseServerMessage: svc_updatename > MAX_CLIENTS_QH");
	}
	String::Cpy(cl.q1_players[i].name, message.ReadString2());
}

void CLQ1_ParseUpdateFrags(QMsg& message)
{
	int i = message.ReadByte();
	if (i >= cl.qh_maxclients)
	{
		common->Error("CL_ParseServerMessage: q1svc_updatefrags > MAX_CLIENTS_QH");
	}
	cl.q1_players[i].frags = message.ReadShort();
}

void CLQW_ParseUpdateFrags(QMsg& message)
{
	int i = message.ReadByte();
	if (i >= MAX_CLIENTS_QHW)
	{
		common->Error("CL_ParseServerMessage: q1svc_updatefrags > MAX_CLIENTS_QHW");
	}
	cl.q1_players[i].frags = message.ReadShort();
}

void CLQ1_ParseUpdateColours(QMsg& message)
{
	int i = message.ReadByte();
	if (i >= cl.qh_maxclients)
	{
		common->Error("CL_ParseServerMessage: q1svc_updatecolors > MAX_CLIENTS_QH");
	}
	int j = message.ReadByte();

	cl.q1_players[i].topcolor = (j & 0xf0) >> 4;
	cl.q1_players[i].bottomcolor = (j & 15);
	CLQ1_TranslatePlayerSkin(i);
}

void CLQW_ParseUpdatePing(QMsg& message)
{
	int i = message.ReadByte();
	if (i >= MAX_CLIENTS_QHW)
	{
		common->Error("CL_ParseServerMessage: qwsvc_updateping > MAX_CLIENTS_QHW");
	}
	cl.q1_players[i].ping = message.ReadShort();
}

void CLQW_ParseUpdatePacketLossage(QMsg& message)
{
	int i = message.ReadByte();
	if (i >= MAX_CLIENTS_QHW)
	{
		common->Error("CL_ParseServerMessage: qwsvc_updatepl > MAX_CLIENTS_QHW");
	}
	cl.q1_players[i].pl = message.ReadByte();
}

void CLQW_ParseUpdateEnterTime(QMsg& message)
{
	// time is sent over as seconds ago
	int i = message.ReadByte();
	if (i >= MAX_CLIENTS_QHW)
	{
		common->Error("CL_ParseServerMessage: qwsvc_updateentertime > MAX_CLIENTS_QHW");
	}
	cl.q1_players[i].entertime = cls.realtime * 0.001 - message.ReadFloat();
}

void CLQ1_ParseParticleEffect(QMsg& message)
{
	vec3_t org;
	message.ReadPos(org);
	vec3_t dir;
	for (int i = 0; i < 3; i++)
	{
		dir[i] = message.ReadChar() * (1.0 / 16);
	}
	int count = message.ReadByte();
	int colour = message.ReadByte();

	if (count == 255)
	{
		// rocket explosion
		CLQ1_ParticleExplosion(org);
	}
	else
	{
		CLQ1_RunParticleEffect(org, dir, colour, count);
	}
}

void CLQ1_ParseSignonNum(QMsg& message)
{
	int i = message.ReadByte();
	if (i <= clc.qh_signon)
	{
		common->Error("Received signon %i when at %i", i, clc.qh_signon);
	}
	clc.qh_signon = i;
	CLQ1_SignonReply();
}

static void CLQW_SetStat(int stat, int value)
{
	if (stat < 0 || stat >= MAX_CL_STATS)
	{
		common->FatalError("CLQW_SetStat: %i is invalid", stat);
	}

	if (stat == QWSTAT_ITEMS)
	{
		// set flash times
		for (int j = 0; j < 32; j++)
		{
			if ((value & (1 << j)) && !(cl.qh_stats[stat] & (1 << j)))
			{
				cl.q1_item_gettime[j] = cl.qh_serverTimeFloat;
			}
		}
	}

	cl.qh_stats[stat] = value;
}

void CLQW_ParseUpdateStat(QMsg& message)
{
	int i = message.ReadByte();
	int j = message.ReadByte();
	CLQW_SetStat(i, j);
}

void CLQW_ParseUpdateStatLong(QMsg& message)
{
	int i = message.ReadByte();
	int j = message.ReadLong();
	CLQW_SetStat(i, j);
}

void CLQ1_ParseCDTrack(QMsg& message)
{
	byte cdtrack = message.ReadByte();
	message.ReadByte();	//	looptrack

	if ((clc.demoplaying || clc.demorecording) && (cls.qh_forcetrack != -1))
	{
		CDAudio_Play((byte)cls.qh_forcetrack, true);
	}
	else
	{
		CDAudio_Play(cdtrack, true);
	}
}

void CLQ1_ParseIntermission()
{
	cl.qh_intermission = 1;
	cl.qh_completed_time = cl.qh_serverTimeFloat;
}

void CLQW_ParseIntermission(QMsg& message)
{
	cl.qh_intermission = 1;
	cl.qh_completed_time = cls.realtime * 0.001;
	message.ReadPos(cl.qh_simorg);
	for (int i = 0; i < 3; i++)
	{
		cl.qh_simangles[i] = message.ReadAngle();
	}
	VectorCopy(vec3_origin, cl.qh_simvel);
}

void CLQ1_ParseFinale(QMsg& message)
{
	cl.qh_intermission = 2;
	cl.qh_completed_time = cl.qh_serverTimeFloat;
	SCR_CenterPrint(message.ReadString2());
}

void CLQ1_ParseCutscene(QMsg& message)
{
	cl.qh_intermission = 3;
	cl.qh_completed_time = cl.qh_serverTimeFloat;
	SCR_CenterPrint(message.ReadString2());
}

void CLQW_MuzzleFlash(QMsg& message)
{
	int i = message.ReadShort();

	if ((unsigned)(i - 1) >= MAX_CLIENTS_QHW)
	{
		return;
	}
	qwplayer_state_t* pl = &cl.qw_frames[cl.qh_parsecount &  UPDATE_MASK_QW].playerstate[i - 1];
	CLQ1_MuzzleFlashLight(i, pl->origin, pl->viewangles);
}

static void CLQW_ProcessUserInfo(int slot, q1player_info_t* player)
{
	String::NCpy(player->name, Info_ValueForKey(player->userinfo, "name"), sizeof(player->name) - 1);
	player->topcolor = String::Atoi(Info_ValueForKey(player->userinfo, "topcolor"));
	player->bottomcolor = String::Atoi(Info_ValueForKey(player->userinfo, "bottomcolor"));
	if (Info_ValueForKey(player->userinfo, "*spectator")[0])
	{
		player->spectator = true;
	}
	else
	{
		player->spectator = false;
	}

	if (cls.state == CA_ACTIVE)
	{
		CLQW_SkinFind(player);
	}

	CLQ1_TranslatePlayerSkin(slot);
}

void CLQW_ParseUpdateUserinfo(QMsg& message)
{
	int slot = message.ReadByte();
	if (slot >= MAX_CLIENTS_QHW)
	{
		common->Error("CL_ParseServerMessage: qwsvc_updateuserinfo > MAX_CLIENTS_QHW");
	}

	q1player_info_t* player = &cl.q1_players[slot];
	player->userid = message.ReadLong();
	String::NCpy(player->userinfo, message.ReadString2(), sizeof(player->userinfo) - 1);

	CLQW_ProcessUserInfo(slot, player);
}

void CLQW_ParseSetInfo(QMsg& message)
{
	int slot = message.ReadByte();
	if (slot >= MAX_CLIENTS_QHW)
	{
		common->Error("CL_ParseServerMessage: qwsvc_setinfo > MAX_CLIENTS_QHW");
	}

	q1player_info_t* player = &cl.q1_players[slot];

	char key[MAX_MSGLEN_QW];
	String::NCpy(key, message.ReadString2(), sizeof(key) - 1);
	key[sizeof(key) - 1] = 0;
	char value[MAX_MSGLEN_QW];
	String::NCpy(value, message.ReadString2(), sizeof(value) - 1);
	key[sizeof(value) - 1] = 0;

	common->DPrintf("SETINFO %s: %s=%s\n", player->name, key, value);

	if (key[0] != '*')
	{
		Info_SetValueForKey(player->userinfo, key, value, MAX_INFO_STRING_QW, 64, 64,
			String::ICmp(key, "name") != 0, String::ICmp(key, "team") == 0);
	}

	CLQW_ProcessUserInfo(slot, player);
}

void CLQW_ParseServerInfo(QMsg& message)
{
	char key[MAX_MSGLEN_QW];
	String::NCpy(key, message.ReadString2(), sizeof(key) - 1);
	key[sizeof(key) - 1] = 0;
	char value[MAX_MSGLEN_QW];
	String::NCpy(value, message.ReadString2(), sizeof(value) - 1);
	key[sizeof(value) - 1] = 0;

	common->DPrintf("SERVERINFO: %s=%s\n", key, value);

	if (key[0] != '*')
	{
		Info_SetValueForKey(cl.qh_serverinfo, key, value, MAX_SERVERINFO_STRING, 64, 64,
			String::ICmp(key, "name") != 0, String::ICmp(key, "team") == 0);
	}
}

void CLQW_CalcModelChecksum(const char* modelName, const char* cvarName)
{
	Array<byte> buffer;
	if (!FS_ReadFile(modelName, buffer))
	{
		common->Error("Couldn't load %s", modelName);
	}

	unsigned short crc;
	CRC_Init(&crc);
	for (int i = 0; i < buffer.Num(); i++)
	{
		CRC_ProcessByte(&crc, buffer[i]);
	}

	char st[40];
	sprintf(st, "%d", (int)crc);
	Info_SetValueForKey(cls.qh_userinfo, cvarName, st, MAX_INFO_STRING_QW, 64, 64, true, false);

	sprintf(st, "setinfo %s %d", cvarName, (int)crc);
	CL_AddReliableCommand(st);
}
