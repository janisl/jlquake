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

Cvar* svqh_deathmatch;			// 0, 1, or 2
Cvar* svqh_coop;				// 0 or 1
Cvar* svqh_teamplay;
Cvar* qh_timelimit;
Cvar* qh_fraglimit;
Cvar* qh_skill;					// 0 - 3

Cvar* svqh_highchars;
Cvar* hw_spartanPrint;
Cvar* hw_dmMode;
Cvar* svqh_idealpitchscale;
Cvar* svqw_mapcheck;
Cvar* svhw_allowtaunts;
Cvar* svqhw_spectalk;

Cvar* qhw_allow_download;
Cvar* qhw_allow_download_skins;
Cvar* qhw_allow_download_models;
Cvar* qhw_allow_download_sounds;
Cvar* qhw_allow_download_maps;

int svqh_current_skill;

fileHandle_t svqhw_fraglogfile;

int SVQH_CalcPing(client_t* cl)
{
	float ping;
	int count;

	ping = 0;
	count = 0;
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
	int i;
	char info[MAX_INFO_STRING_QW];

	i = client - svs.clients;

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
