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
#include "../../common/hexen2strings.h"

Cvar* svqh_deathmatch;			// 0, 1, or 2
Cvar* svqh_coop;				// 0 or 1
Cvar* svqh_teamplay;
Cvar* qh_timelimit;
Cvar* qh_fraglimit;
Cvar* qh_skill;					// 0 - 3
Cvar* h2_randomclass;				// 0, 1, or 2

Cvar* svqh_highchars;
Cvar* hw_spartanPrint;
Cvar* hw_dmMode;
Cvar* svqh_idealpitchscale;
Cvar* svqw_mapcheck;
Cvar* svhw_allowtaunts;
Cvar* svqhw_spectalk;
Cvar* qh_pausable;
Cvar* svhw_namedistance;

Cvar* svh2_update_player;
Cvar* svh2_update_monsters;
Cvar* svh2_update_missiles;
Cvar* svh2_update_misc;

Cvar* qhw_allow_download;
Cvar* qhw_allow_download_skins;
Cvar* qhw_allow_download_models;
Cvar* qhw_allow_download_sounds;
Cvar* qhw_allow_download_maps;

Cvar* hw_damageScale;
Cvar* hw_shyRespawn;
Cvar* hw_meleeDamScale;
Cvar* hw_manaScale;
Cvar* hw_tomeMode;
Cvar* hw_tomeRespawn;
Cvar* hw_w2Respawn;
Cvar* hw_altRespawn;
Cvar* hw_fixedLevel;
Cvar* hw_autoItems;
Cvar* hw_easyFourth;
Cvar* hw_patternRunner;

int svqh_current_skill;
int svh2_kingofhill;

fileHandle_t svqhw_fraglogfile;

void SVQH_FreeMemory()
{
	if (sv.h2_states)
	{
		Mem_Free(sv.h2_states);
		sv.h2_states = NULL;
	}
	if (sv.qh_edicts)
	{
		Mem_Free(sv.qh_edicts);
		sv.qh_edicts = NULL;
	}
	PR_UnloadProgs();
}

int SVQH_CalcPing(client_t* cl)
{
	float ping = 0;
	int count = 0;
	if (GGameType & GAME_HexenWorld)
	{
		hwclient_frame_t* frame = cl->hw_frames;
		for (int i = 0; i < UPDATE_BACKUP_HW; i++, frame++)
		{
			if (frame->ping_time > 0)
			{
				ping += frame->ping_time;
				count++;
			}
		}
	}
	else
	{
		qwclient_frame_t* frame = cl->qw_frames;
		for (int i = 0; i < UPDATE_BACKUP_QW; i++, frame++)
		{
			if (frame->ping_time > 0)
			{
				ping += frame->ping_time;
				count++;
			}
		}
	}
	if (!count)
	{
		return 9999;
	}
	ping /= count;

	return ping * 1000;
}

//	Writes all update values to a sizebuf
void SVQHW_FullClientUpdate(client_t* client, QMsg* buf)
{
	char info[MAX_INFO_STRING_QW];

	int i = client - svs.clients;

	if (GGameType & GAME_HexenWorld)
	{
		buf->WriteByte(hwsvc_updatedminfo);
		buf->WriteByte(i);
		buf->WriteShort(client->qh_old_frags);
		buf->WriteByte((client->h2_playerclass << 5) | ((int)client->qh_edict->GetLevel() & 31));

		if (hw_dmMode->value == HWDM_SIEGE)
		{
			buf->WriteByte(hwsvc_updatesiegeinfo);
			buf->WriteByte((int)ceil(qh_timelimit->value));
			buf->WriteByte((int)ceil(qh_fraglimit->value));

			buf->WriteByte(hwsvc_updatesiegeteam);
			buf->WriteByte(i);
			buf->WriteByte(client->hw_siege_team);

			buf->WriteByte(hwsvc_updatesiegelosses);
			buf->WriteByte(*pr_globalVars.defLosses);
			buf->WriteByte(*pr_globalVars.attLosses);

			buf->WriteByte(h2svc_time);	//send server time upon connection
			buf->WriteFloat(sv.qh_time);
		}
	}
	else
	{
		buf->WriteByte(q1svc_updatefrags);
		buf->WriteByte(i);
		buf->WriteShort(client->qh_old_frags);
	}

	buf->WriteByte(GGameType & GAME_HexenWorld ? hwsvc_updateping : qwsvc_updateping);
	buf->WriteByte(i);
	buf->WriteShort(SVQH_CalcPing(client));

	if (GGameType & GAME_QuakeWorld)
	{
		buf->WriteByte(qwsvc_updatepl);
		buf->WriteByte(i);
		buf->WriteByte(client->qw_lossage);
	}

	buf->WriteByte(GGameType & GAME_HexenWorld ? hwsvc_updateentertime : qwsvc_updateentertime);
	buf->WriteByte(i);
	buf->WriteFloat(svs.realtime * 0.001 - client->qh_connection_started);

	String::Cpy(info, client->userinfo);
	Info_RemovePrefixedKeys(info, '_', MAX_INFO_STRING_QW);	// server passwords, etc

	buf->WriteByte(GGameType & GAME_HexenWorld ? hwsvc_updateuserinfo : qwsvc_updateuserinfo);
	buf->WriteByte(i);
	buf->WriteLong(client->qh_userid);
	buf->WriteString2(info);
}

//	Writes all update values to a client's reliable stream
void SVQHW_FullClientUpdateToClient(client_t* client, client_t* cl)
{
	SVQH_ClientReliableCheckBlock(cl, 24 + String::Length(client->userinfo));
	if (cl->qw_num_backbuf)
	{
		SVQHW_FullClientUpdate(client, &cl->qw_backbuf);
		SVQH_ClientReliable_FinishWrite(cl);
	}
	else
	{
		SVQHW_FullClientUpdate(client, &cl->netchan.message);
	}
}

//	Pull specific info from a newly changed userinfo string
// into a more C freindly form.
void SVQHW_ExtractFromUserinfo(client_t* cl)
{
	const char* val;
	char* p, * q;
	int i;
	client_t* client;
	int dupc = 1;
	char newname[80];


	// name for C code
	val = Info_ValueForKey(cl->userinfo, "name");

	// trim user name
	String::NCpy(newname, val, sizeof(newname) - 1);
	newname[sizeof(newname) - 1] = 0;

	for (p = newname; (*p == ' ' || *p == '\r' || *p == '\n') && *p; p++)
		;

	if (p != newname && !*p)
	{
		//white space only
		String::Cpy(newname, "unnamed");
		p = newname;
	}

	if (p != newname && *p)
	{
		for (q = newname; *p; *q++ = *p++)
			;
		*q = 0;
	}
	for (p = newname + String::Length(newname) - 1; p != newname && (*p == ' ' || *p == '\r' || *p == '\n'); p--)
		;
	p[1] = 0;

	if (String::Cmp(val, newname))
	{
		Info_SetValueForKey(cl->userinfo, "name", newname, MAX_INFO_STRING_QW, 64, 64, !svqh_highchars->value);
		val = Info_ValueForKey(cl->userinfo, "name");
	}

	if (!val[0] || !String::ICmp(val, "console"))
	{
		Info_SetValueForKey(cl->userinfo, "name", "unnamed", MAX_INFO_STRING_QW, 64, 64, !svqh_highchars->value);
		val = Info_ValueForKey(cl->userinfo, "name");
	}

	// check to see if another user by the same name exists
	while (1)
	{
		for (i = 0, client = svs.clients; i < MAX_CLIENTS_QHW; i++, client++)
		{
			if (client->state != CS_ACTIVE || client == cl)
			{
				continue;
			}
			if (!String::ICmp(client->name, val))
			{
				break;
			}
		}
		if (i != MAX_CLIENTS_QHW)
		{
			// dup name
			char tmp[80];
			String::NCpyZ(tmp, val, sizeof(tmp));
			if (String::Length(tmp) > (int)sizeof(cl->name) - 1)
			{
				tmp[sizeof(cl->name) - 4] = 0;
			}
			p = tmp;

			if (tmp[0] == '(')
			{
				if (tmp[2] == ')')
				{
					p = tmp + 3;
				}
				else if (tmp[3] == ')')
				{
					p = tmp + 4;
				}
			}

			sprintf(newname, "(%d)%-.40s", dupc++, p);
			Info_SetValueForKey(cl->userinfo, "name", newname, MAX_INFO_STRING_QW, 64, 64, !svqh_highchars->value);
			val = Info_ValueForKey(cl->userinfo, "name");
		}
		else
		{
			break;
		}
	}

	if (String::NCmp(val, cl->name, String::Length(cl->name)))
	{
		if (!sv.qh_paused)
		{
			if (!cl->qw_lastnametime || svs.realtime * 0.001 - cl->qw_lastnametime > 5)
			{
				cl->qw_lastnamecount = 0;
				cl->qw_lastnametime = svs.realtime * 0.001;
			}
			else if (cl->qw_lastnamecount++ > 4)
			{
				SVQH_BroadcastPrintf(PRINT_HIGH, "%s was kicked for name spam\n", cl->name);
				SVQH_ClientPrintf(cl, PRINT_HIGH, "You were kicked from the game for name spamming\n");
				SVQHW_DropClient(cl);
				return;
			}
		}

		if (cl->state >= CS_ACTIVE && !cl->qh_spectator)
		{
			SVQH_BroadcastPrintf(PRINT_HIGH, "%s changed name to %s\n", cl->name, val);
		}
	}

	String::NCpy(cl->name, val, sizeof(cl->name) - 1);

	if (GGameType & GAME_Hexen2)
	{
		// playerclass command
		val = Info_ValueForKey(cl->userinfo, "playerclass");
		if (String::Length(val))
		{
			i = String::Atoi(val);
			if (i > CLASS_DEMON && hw_dmMode->value != HWDM_SIEGE)
			{
				i = CLASS_PALADIN;
			}
			if (i < 0 || i > MAX_PLAYER_CLASS || (!cl->hw_portals && i == CLASS_DEMON))
			{
				i = 0;
			}
			cl->hw_next_playerclass =  i;
			cl->qh_edict->SetNextPlayerClass(i);

			if (cl->qh_edict->GetHealth() > 0)
			{
				sprintf(newname,"%d",cl->h2_playerclass);
				Info_SetValueForKey(cl->userinfo, "playerclass", newname, MAX_INFO_STRING_QW, 64, 64, !svqh_highchars->value);
			}
		}
	}

	// msg command
	val = Info_ValueForKey(cl->userinfo, "msg");
	if (String::Length(val))
	{
		cl->messagelevel = String::Atoi(val);
	}
}

const char* SVQ1_GetMapName()
{
	return PR_GetString(sv.qh_edicts->GetMessage());
}

const char* SVH2_GetMapName()
{
	if (sv.qh_edicts->GetMessage() > 0 && sv.qh_edicts->GetMessage() <= prh2_string_count)
	{
		return &prh2_global_strings[prh2_string_index[(int)sv.qh_edicts->GetMessage() - 1]];
	}
	else
	{
		return PR_GetString(sv.qh_edicts->GetNetName());
	}
}

//	Sends the first message from the server to a connected client.
// This will be sent on the initial connection and upon each server load.
void SVQH_SendServerinfo(client_t* client)
{
	SVQH_ClientPrintf(client, 0, "%c\nJLQuake VERSION " JLQUAKE_VERSION_STRING " SERVER (%i CRC)\n", 2, pr_crc);

	client->qh_message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_serverinfo : q1svc_serverinfo);
	client->qh_message.WriteLong(GGameType & GAME_Hexen2 ? H2PROTOCOL_VERSION : Q1PROTOCOL_VERSION);
	client->qh_message.WriteByte(svs.qh_maxclients);

	if (!svqh_coop->value && svqh_deathmatch->value)
	{
		client->qh_message.WriteByte(QHGAME_DEATHMATCH);
		if (GGameType & GAME_Hexen2)
		{
			client->qh_message.WriteShort(svh2_kingofhill);
		}
	}
	else
	{
		client->qh_message.WriteByte(QHGAME_COOP);
	}

	client->qh_message.WriteString2(GGameType & GAME_Hexen2 ? SVH2_GetMapName() : SVQ1_GetMapName());

	for (const char** s = sv.qh_model_precache + 1; *s; s++)
	{
		client->qh_message.WriteString2(*s);
	}
	client->qh_message.WriteByte(0);

	for (const char** s = sv.qh_sound_precache + 1; *s; s++)
	{
		client->qh_message.WriteString2(*s);
	}
	client->qh_message.WriteByte(0);

	// send music
	if (GGameType & GAME_Hexen2)
	{
		client->qh_message.WriteByte(h2svc_cdtrack);
		client->qh_message.WriteByte(sv.h2_cd_track);
		client->qh_message.WriteByte(sv.h2_cd_track);

		client->qh_message.WriteByte(h2svc_midi_name);
		client->qh_message.WriteString2(sv.h2_midi_name);
	}
	else
	{
		client->qh_message.WriteByte(q1svc_cdtrack);
		client->qh_message.WriteByte(sv.qh_edicts->GetSounds());
		client->qh_message.WriteByte(sv.qh_edicts->GetSounds());
	}

	// set view
	client->qh_message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_setview : q1svc_setview);
	client->qh_message.WriteShort(QH_NUM_FOR_EDICT(client->qh_edict));

	client->qh_message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_signonnum : q1svc_signonnum);
	client->qh_message.WriteByte(1);

	client->qh_sendsignon = true;
	client->state = CS_CONNECTED;		// need prespawn, spawn, etc
}
