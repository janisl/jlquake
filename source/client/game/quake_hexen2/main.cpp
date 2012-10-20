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
#include "main.h"
#include "demo.h"
#include "connection.h"
#include "network_channel.h"
#include "../quake/local.h"
#include "../hexen2/local.h"

Cvar* clqh_sbar;

Cvar* qhw_topcolor;
Cvar* qhw_bottomcolor;
Cvar* qhw_spectator;
Cvar* clqhw_rate;

int clqh_packet_latency[NET_TIMINGS_QH];

void CLQH_Init()
{
	if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)))
	{
		clqh_name = Cvar_Get("_cl_name", "player", CVAR_ARCHIVE);
		clqh_color = Cvar_Get("_cl_color", "0", CVAR_ARCHIVE);
		clqh_nolerp = Cvar_Get("cl_nolerp", "0", 0);

		Cmd_AddCommand("record", CLQH_Record_f);
		Cmd_AddCommand("stop", CLQH_Stop_f);
		Cmd_AddCommand("playdemo", CLQH_PlayDemo_f);
		Cmd_AddCommand("timedemo", CLQH_TimeDemo_f);
		Cmd_AddCommand("slist", NET_Slist_f);
		Cmd_AddCommand("connect", CLQH_Connect_f);
		Cmd_AddCommand("reconnect", CLQH_Reconnect_f);
		Cmd_AddCommand("startdemos", CLQH_Startdemos_f);
		Cmd_AddCommand("demos", CLQH_Demos_f);
		Cmd_AddCommand("stopdemo", CLQH_Stopdemo_f);

		Cmd_AddCommand("god", NULL);
		Cmd_AddCommand("notarget", NULL);
		Cmd_AddCommand("noclip", NULL);
		Cmd_AddCommand("say", NULL);
		Cmd_AddCommand("say_team", NULL);
		Cmd_AddCommand("tell", NULL);
		Cmd_AddCommand("kill", NULL);
		Cmd_AddCommand("pause", NULL);
		Cmd_AddCommand("ping", NULL);
		Cmd_AddCommand("give", NULL);

		if (GGameType & GAME_Quake)
		{
			cl_doubleeyes = Cvar_Get("cl_doubleeyes", "1", 0);

			Cmd_AddCommand("fly", NULL);
		}
		else
		{
			clh2_playerclass = Cvar_Get("_cl_playerclass", "5", CVAR_ARCHIVE);
		}
	}
	else
	{
		Info_SetValueForKey(cls.qh_userinfo, "name", "unnamed", MAX_INFO_STRING_QW, 64, 64, false, false);
		Info_SetValueForKey(cls.qh_userinfo, "topcolor", "0", MAX_INFO_STRING_QW, 64, 64, true, false);
		Info_SetValueForKey(cls.qh_userinfo, "bottomcolor", "0", MAX_INFO_STRING_QW, 64, 64, true, false);
		Info_SetValueForKey(cls.qh_userinfo, "rate", "2500", MAX_INFO_STRING_QW, 64, 64, true, false);
		Info_SetValueForKey(cls.qh_userinfo, "msg", "1", MAX_INFO_STRING_QW, 64, 64, true, false);
	
		clqh_name = Cvar_Get("name", "unnamed", CVAR_ARCHIVE | CVAR_USERINFO);
		qhw_topcolor = Cvar_Get("topcolor", "0", CVAR_ARCHIVE | CVAR_USERINFO);
		qhw_bottomcolor = Cvar_Get("bottomcolor", "0", CVAR_ARCHIVE | CVAR_USERINFO);
		qhw_spectator = Cvar_Get("spectator", "", CVAR_USERINFO);
		cl_timeout = Cvar_Get("cl_timeout", "60", 0);

		//
		// info mirrors
		//
		Cvar_Get("password", "", CVAR_USERINFO);
		Cvar_Get("skin", "", CVAR_ARCHIVE | CVAR_USERINFO);
		Cvar_Get("team", "", CVAR_ARCHIVE | CVAR_USERINFO);
		clqhw_rate = Cvar_Get("rate", "2500", CVAR_ARCHIVE | CVAR_USERINFO);
		Cvar_Get("msg", "1", CVAR_ARCHIVE | CVAR_USERINFO);
		Cvar_Get("noaim", "0", CVAR_ARCHIVE | CVAR_USERINFO);

		Cmd_AddCommand("record", CLQW_Record_f);
		Cmd_AddCommand("rerecord", CLQHW_ReRecord_f);
		Cmd_AddCommand("stop", CLQH_Stop_f);
		Cmd_AddCommand("playdemo", CLQHW_PlayDemo_f);
		Cmd_AddCommand("timedemo", CLQH_TimeDemo_f);

		Cmd_AddCommand("connect", CLQHW_Connect_f);
		Cmd_AddCommand("reconnect", CLQHW_Reconnect_f);

		Cmd_AddCommand("nextul", CLQW_NextUpload);
		Cmd_AddCommand("stopul", CLQW_StopUpload);

		//
		// forward to server commands
		//
		Cmd_AddCommand("kill", NULL);
		Cmd_AddCommand("say", NULL);
		Cmd_AddCommand("say_team", NULL);
		Cmd_AddCommand("serverinfo", NULL);

		CLQHW_InitPrediction();
		CL_InitCam();
		PMQH_Init();

		if (GGameType & GAME_Quake)
		{
			clqw_hudswap = Cvar_Get("cl_hudswap", "0", CVAR_ARCHIVE);
			clqw_localid = Cvar_Get("localid", "", 0);
			clqw_baseskin = Cvar_Get("baseskin", "base", 0);
			clqw_noskins = Cvar_Get("noskins", "0", 0);

			Cmd_AddCommand("skins", CLQW_SkinSkins_f);
			Cmd_AddCommand("allskins", CLQW_SkinAllSkins_f);

			Cmd_AddCommand("pause", NULL);
		}
		else
		{
			Info_SetValueForKey(cls.qh_userinfo, "playerclass", "0", MAX_INFO_STRING_QW, 64, 64, true, false);

			clh2_playerclass = Cvar_Get("playerclass", "0", CVAR_ARCHIVE | CVAR_USERINFO);
			clhw_talksounds = Cvar_Get("talksounds", "1", CVAR_ARCHIVE);
			clhw_teamcolor = Cvar_Get("clhw_teamcolor", "187", CVAR_ARCHIVE);

			CLHW_InitEffects();
		}
	}
}
