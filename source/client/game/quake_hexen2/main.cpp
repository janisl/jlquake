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
#include "../../../server/public.h"

Cvar* clqh_sbar;

Cvar* qhw_topcolor;
Cvar* qhw_bottomcolor;
Cvar* qhw_spectator;

int clqh_packet_latency[NET_TIMINGS_QH];

static float save_sensitivity;

static void CLQH_PrintEntities_f()
{
	if (GGameType & GAME_Hexen2)
	{
		h2entity_t* ent;
		int i;

		for (i = 0,ent = h2cl_entities; i < cl.qh_num_entities; i++,ent++)
		{
			common->Printf("%3i:",i);
			if (!ent->state.modelindex)
			{
				common->Printf("EMPTY\n");
				continue;
			}
			common->Printf("%s:%2i  (%5.1f,%5.1f,%5.1f) [%5.1f %5.1f %5.1f]\n",
				R_ModelName(cl.model_draw[ent->state.modelindex]),ent->state.frame, ent->state.origin[0], ent->state.origin[1], ent->state.origin[2], ent->state.angles[0], ent->state.angles[1], ent->state.angles[2]);
		}
	}
	else
	{
		q1entity_t* ent;
		int i;

		for (i = 0,ent = clq1_entities; i < cl.qh_num_entities; i++,ent++)
		{
			common->Printf("%3i:",i);
			if (!ent->state.modelindex)
			{
				common->Printf("EMPTY\n");
				continue;
			}
			common->Printf("%s:%2i  (%5.1f,%5.1f,%5.1f) [%5.1f %5.1f %5.1f]\n",
				R_ModelName(cl.model_draw[ent->state.modelindex]),ent->state.frame, ent->state.origin[0], ent->state.origin[1], ent->state.origin[2], ent->state.angles[0], ent->state.angles[1], ent->state.angles[2]);
		}
	}
}

static void CLH2_Sensitivity_save_f()
{
	if (Cmd_Argc() != 2)
	{
		common->Printf("sensitivity_save <save/restore>\n");
		return;
	}

	if (String::ICmp(Cmd_Argv(1),"save") == 0)
	{
		save_sensitivity = cl_sensitivity->value;
	}
	else if (String::ICmp(Cmd_Argv(1),"restore") == 0)
	{
		Cvar_SetValue("sensitivity", save_sensitivity);
	}
}

static void CLQH_Name_f()
{
	if (Cmd_Argc() == 1)
	{
		common->Printf("\"name\" is \"%s\"\n", clqh_name->string);
		return;
	}
	char* newName;
	if (Cmd_Argc() == 2)
	{
		newName = Cmd_Argv(1);
	}
	else
	{
		newName = Cmd_ArgsUnmodified();
	}
	newName[15] = 0;

	if (GGameType & GAME_Hexen2)
	{
		//this is for the fuckers who put braces in the name causing loadgame to crash.
		char* pdest = strchr(newName,'{');
		if (pdest)
		{
			*pdest = 0;	//zap the brace
			common->Printf("Illegal char in name removed!\n");
		}
	}

	if (String::Cmp(clqh_name->string, newName) == 0)
	{
		return;
	}
	Cvar_Set("_cl_name", newName);
	if (cls.state == CA_ACTIVE)
	{
		CL_ForwardKnownCommandToServer();
	}
}

static void CLQH_Color_f()
{
	int top, bottom;
	int playercolor;

	if (Cmd_Argc() == 1)
	{
		common->Printf("\"color\" is \"%i %i\"\n", ((int)clqh_color->value) >> 4, ((int)clqh_color->value) & 0x0f);
		common->Printf("color <0-13> [0-13]\n");
		return;
	}

	if (Cmd_Argc() == 2)
	{
		top = bottom = String::Atoi(Cmd_Argv(1));
	}
	else
	{
		top = String::Atoi(Cmd_Argv(1));
		bottom = String::Atoi(Cmd_Argv(2));
	}

	top &= 15;
	if (top > 13)
	{
		top = 13;
	}
	bottom &= 15;
	if (bottom > 13)
	{
		bottom = 13;
	}

	playercolor = top * 16 + bottom;

	Cvar_SetValue("_cl_color", playercolor);
	if (cls.state == CA_ACTIVE)
	{
		CL_ForwardKnownCommandToServer();
	}
}

static void CLH2_Class_f()
{
	if (Cmd_Argc() == 1)
	{
		if (!(int)clh2_playerclass->value)
		{
			common->Printf("\"playerclass\" is %d (\"unknown\")\n", (int)clh2_playerclass->value);
		}
		else
		{
			common->Printf("\"playerclass\" is %d (\"%s\")\n", (int)clh2_playerclass->value,h2_ClassNames[(int)clh2_playerclass->value - 1]);
		}
		return;
	}

	float newClass;
	if (Cmd_Argc() == 2)
	{
		newClass = String::Atof(Cmd_Argv(1));
	}
	else
	{
		newClass = String::Atof(Cmd_ArgsUnmodified());
	}

	Cvar_SetValue("_cl_playerclass", newClass);

	if (GGameType & GAME_H2Portals)
	{
		PR_SetPlayerClassGlobal(newClass);
	}

	if (cls.state == CA_ACTIVE)
	{
		CL_ForwardKnownCommandToServer();
	}
}

//	Dump userdata / masterdata for a user
void CLQHW_User_f()
{
	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: user <username / userid>\n");
		return;
	}

	int uid = String::Atoi(Cmd_Argv(1));

	for (int i = 0; i < MAX_CLIENTS_QHW; i++)
	{
		if (GGameType & GAME_HexenWorld)
		{
			if (!cl.h2_players[i].name[0])
			{
				continue;
			}
			if (cl.h2_players[i].userid == uid ||
				!String::Cmp(cl.h2_players[i].name, Cmd_Argv(1)))
			{
				Info_Print(cl.h2_players[i].userinfo);
				return;
			}
		}
		else
		{
			if (!cl.q1_players[i].name[0])
			{
				continue;
			}
			if (cl.q1_players[i].userid == uid ||
				!String::Cmp(cl.q1_players[i].name, Cmd_Argv(1)))
			{
				Info_Print(cl.q1_players[i].userinfo);
				return;
			}
		}
	}
	common->Printf("User not in server.\n");
}

//	Dump userids for all current players
static void CLQHW_Users_f()
{
	int c = 0;
	common->Printf("userid frags name\n");
	common->Printf("------ ----- ----\n");
	for (int i = 0; i < MAX_CLIENTS_QHW; i++)
	{
		if (GGameType & GAME_HexenWorld)
		{
			if (cl.h2_players[i].name[0])
			{
				common->Printf("%6i %4i %s\n", cl.h2_players[i].userid, cl.h2_players[i].frags, cl.h2_players[i].name);
				c++;
			}
		}
		else
		{
			if (cl.q1_players[i].name[0])
			{
				common->Printf("%6i %4i %s\n", cl.q1_players[i].userid, cl.q1_players[i].frags, cl.q1_players[i].name);
				c++;
			}
		}
	}

	common->Printf("%i total users\n", c);
}

static void CLQHW_Color_f()
{
	// just for quake compatability...
	if (Cmd_Argc() == 1)
	{
		common->Printf("\"color\" is \"%s %s\"\n",
			Info_ValueForKey(cls.qh_userinfo, "topcolor"),
			Info_ValueForKey(cls.qh_userinfo, "bottomcolor"));
		common->Printf("color <0-13> [0-13]\n");
		return;
	}

	int top, bottom;
	if (Cmd_Argc() == 2)
	{
		top = bottom = String::Atoi(Cmd_Argv(1));
	}
	else
	{
		top = String::Atoi(Cmd_Argv(1));
		bottom = String::Atoi(Cmd_Argv(2));
	}

	top &= 15;
	if (top > 13)
	{
		top = 13;
	}
	bottom &= 15;
	if (bottom > 13)
	{
		bottom = 13;
	}

	char num[16];
	sprintf(num, "%i", top);
	Cvar_Set("topcolor", num);
	sprintf(num, "%i", bottom);
	Cvar_Set("bottomcolor", num);
}

//	Sent by server when serverinfo changes
static void CLQHW_FullServerinfo_f()
{
	if (Cmd_Argc() != 2)
	{
		common->Printf("usage: fullserverinfo <complete info string>\n");
		return;
	}

	String::Cpy(cl.qh_serverinfo, Cmd_Argv(1));

	if (GGameType & GAME_HexenWorld)
	{
		const char* p = Info_ValueForKey(cl.qh_serverinfo, "*version");
		if (p && *p)
		{
			float v = String::Atof(p);
			if (v)
			{
				if (!clqh_server_version)
				{
					common->Printf("Version %1.2f Server\n", v);
				}
				clqh_server_version = v;
			}
		}
	}
}

//	Allow clients to change userinfo
static void CLQHW_FullInfo_f()
{
	if (Cmd_Argc() != 2)
	{
		common->Printf("fullinfo <complete info string>\n");
		return;
	}

	const char* s = Cmd_Argv(1);
	if (*s == '\\')
	{
		s++;
	}
	while (*s)
	{
		char key[512];
		char* o = key;
		while (*s && *s != '\\')
		{
			*o++ = *s++;
		}
		*o = 0;

		if (!*s)
		{
			common->Printf("MISSING VALUE\n");
			return;
		}

		char value[512];
		o = value;
		s++;
		while (*s && *s != '\\')
		{
			*o++ = *s++;
		}
		*o = 0;

		if (*s)
		{
			s++;
		}

		if (GGameType & GAME_QuakeWorld && (!String::ICmp(key, "pmodel") || !String::ICmp(key, "emodel")))
		{
			continue;
		}

		if (key[0] == '*')
		{
			common->Printf("Can't set * keys\n");
			continue;
		}

		Info_SetValueForKey(cls.qh_userinfo, key, value, MAX_INFO_STRING_QW, 64, 64,
			String::ICmp(key, "name") != 0, String::ICmp(key, "team") == 0);
	}
}

//	Allow clients to change userinfo
static void CLQHW_SetInfo_f()
{
	if (Cmd_Argc() == 1)
	{
		Info_Print(cls.qh_userinfo);
		return;
	}
	if (Cmd_Argc() != 3)
	{
		common->Printf("usage: setinfo [ <key> <value> ]\n");
		return;
	}
	if (!String::ICmp(Cmd_Argv(1), "pmodel") || !String::Cmp(Cmd_Argv(1), "emodel"))
	{
		return;
	}

	if (Cmd_Argv(1)[0] == '*')
	{
		common->Printf("Can't set * keys\n");
		return;
	}

	Info_SetValueForKey(cls.qh_userinfo, Cmd_Argv(1), Cmd_Argv(2), MAX_INFO_STRING_QW, 64, 64,
		String::ICmp(Cmd_Argv(1), "name") != 0, String::ICmp(Cmd_Argv(1), "team") == 0);
	if (cls.state == CA_CONNECTED || cls.state == CA_LOADING || cls.state == CA_ACTIVE)
	{
		CL_ForwardKnownCommandToServer();
	}
}

//	Just sent as a hint to the client that they should drop to full console
static void CLQHW_Changing_f()
{
	if (clc.download)	// don't change when downloading
	{
		return;
	}

	S_StopAllSounds();
	cl.qh_intermission = 0;
	cls.state = CA_CONNECTED;	// not active anymore, but not disconnected
	common->Printf("\nChanging map...\n");
}

static void CLQHW_Download_f()
{
	if (cls.state == CA_DISCONNECTED)
	{
		common->Printf("Must be connected.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: download <datafile>\n");
		return;
	}

	String::NCpyZ(clc.downloadName, Cmd_Argv(1), sizeof(clc.downloadName));
	String::Cpy(clc.downloadTempName, clc.downloadName);

	clc.download = FS_FOpenFileWrite(clc.downloadName);
	clc.downloadType = dl_single;

	CL_AddReliableCommand(va("download %s\n", Cmd_Argv(1)));
}

//	HexenWorld doesn't use custom skins but we need this because it's
// used during connection to the server.
static void CLHW_Skins_f()
{
	if (cls.state == CA_DISCONNECTED)
	{
		common->Printf("WARNING: cannot complete command because there is no connection to a server\n");
		return;
	}

	clc.downloadNumber = 0;
	clc.downloadType = dl_none;

	if (cls.state != CA_ACTIVE)
	{
		// get next signon phase
		CL_AddReliableCommand(va("begin %i", cl.servercount));
	}
}

void CLQH_Init()
{
	Cmd_AddCommand("stop", CLQH_Stop_f);
	Cmd_AddCommand("timedemo", CLQH_TimeDemo_f);
	Cmd_AddCommand("say_team", NULL);

	//
	// forward to server commands
	//
	Cmd_AddCommand("kill", NULL);
	Cmd_AddCommand("say", NULL);

	if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)))
	{
		clqh_name = Cvar_Get("_cl_name", "player", CVAR_ARCHIVE);
		clqh_color = Cvar_Get("_cl_color", "0", CVAR_ARCHIVE);
		clqh_nolerp = Cvar_Get("cl_nolerp", "0", 0);

		Cmd_AddCommand("record", CLQH_Record_f);
		Cmd_AddCommand("playdemo", CLQH_PlayDemo_f);
		Cmd_AddCommand("slist", NET_Slist_f);
		Cmd_AddCommand("connect", CLQH_Connect_f);
		Cmd_AddCommand("reconnect", CLQH_Reconnect_f);
		Cmd_AddCommand("startdemos", CLQH_Startdemos_f);
		Cmd_AddCommand("demos", CLQH_Demos_f);
		Cmd_AddCommand("stopdemo", CLQH_Stopdemo_f);
		Cmd_AddCommand("entities", CLQH_PrintEntities_f);
		Cmd_AddCommand("name", CLQH_Name_f);
		Cmd_AddCommand("color", CLQH_Color_f);

		Cmd_AddCommand("god", NULL);
		Cmd_AddCommand("notarget", NULL);
		Cmd_AddCommand("noclip", NULL);
		Cmd_AddCommand("tell", NULL);
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

			Cmd_AddCommand("playerclass", CLH2_Class_f);
		}

		Chase_Init();
	}
	else
	{
		Info_SetValueForKey(cls.qh_userinfo, "name", "unnamed", MAX_INFO_STRING_QW, 64, 64, false, false);
		Info_SetValueForKey(cls.qh_userinfo, "topcolor", "0", MAX_INFO_STRING_QW, 64, 64, true, false);
		Info_SetValueForKey(cls.qh_userinfo, "bottomcolor", "0", MAX_INFO_STRING_QW, 64, 64, true, false);
		Info_SetValueForKey(cls.qh_userinfo, "rate", "2500", MAX_INFO_STRING_QW, 64, 64, true, false);
		Info_SetValueForKey(cls.qh_userinfo, "msg", "1", MAX_INFO_STRING_QW, 64, 64, true, false);
		if (GGameType & GAME_QuakeWorld)
		{
			Info_SetValueForKey(cls.qh_userinfo, "*ver", JLQUAKE_VERSION_STRING, MAX_INFO_STRING_QW, 64, 64, true, false);
		}
	
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
		Cvar_Get("rate", "1000000", CVAR_ARCHIVE | CVAR_USERINFO);
		Cvar_Get("msg", "1", CVAR_ARCHIVE | CVAR_USERINFO);
		Cvar_Get("noaim", "0", CVAR_ARCHIVE | CVAR_USERINFO);

		Cmd_AddCommand("record", CLQW_Record_f);
		Cmd_AddCommand("rerecord", CLQHW_ReRecord_f);
		Cmd_AddCommand("playdemo", CLQHW_PlayDemo_f);

		Cmd_AddCommand("connect", CLQHW_Connect_f);
		Cmd_AddCommand("reconnect", CLQHW_Reconnect_f);

		Cmd_AddCommand("nextul", CLQW_NextUpload);
		Cmd_AddCommand("stopul", CLQW_StopUpload);

		Cmd_AddCommand("user", CLQHW_User_f);
		Cmd_AddCommand("users", CLQHW_Users_f);
		Cmd_AddCommand("color", CLQHW_Color_f);
		Cmd_AddCommand("fullserverinfo", CLQHW_FullServerinfo_f);
		Cmd_AddCommand("fullinfo", CLQHW_FullInfo_f);
		Cmd_AddCommand("setinfo", CLQHW_SetInfo_f);
		Cmd_AddCommand("changing", CLQHW_Changing_f);
		Cmd_AddCommand("download", CLQHW_Download_f);

		//
		// forward to server commands
		//
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

			Cmd_AddCommand("skins", CLHW_Skins_f);

			CLHW_InitEffects();
		}
	}

	if (GGameType & GAME_Quake)
	{
		CLQ1_InitTEnts();
		SbarQ1_Init();
	}
	else
	{
		Cmd_AddCommand("sensitivity_save", CLH2_Sensitivity_save_f);

		CLH2_InitTEnts();
		MIDI_Init();
		SbarH2_Init();
	}
}

void CLQH_OnEndGame()
{
	if (cls.qh_demonum != -1)
	{
		CL_NextDemo();
	}
	else
	{
		CL_Disconnect(true);
	}
}
