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

#include "local.h"
#include "../../client_main.h"
#include "../../public.h"
#include "../../ui/ui.h"
#include "../../ui/console.h"
#include "../../../common/common_defs.h"

Cvar* q2_hand;
Cvar* clq2_footsteps;
Cvar* clq2_name;
Cvar* clq2_skin;
Cvar* clq2_vwep;
Cvar* clq2_predict;
Cvar* clq2_noskins;
Cvar* clq2_showmiss;
Cvar* clq2_gender;
Cvar* clq2_gender_auto;
Cvar* clq2_maxfps;

q2centity_t clq2_entities[MAX_EDICTS_Q2];

char clq2_weaponmodels[MAX_CLIENTWEAPONMODELS_Q2][MAX_QPATH];
int clq2_num_weaponmodels;

static bool vid_restart_requested;

void CLQ2_PingServers_f()
{
	NET_Config(true);		// allow remote

	// send a broadcast packet
	common->Printf("pinging broadcast...\n");

	Cvar* noudp = Cvar_Get("noudp", "0", CVAR_INIT);
	if (!noudp->value)
	{
		netadr_t adr;
		adr.type = NA_BROADCAST;
		adr.port = BigShort(Q2PORT_SERVER);
		NET_OutOfBandPrint(NS_CLIENT, adr, "info %i", Q2PROTOCOL_VERSION);
	}

	// send a packet to each address book entry
	for (int i = 0; i < 16; i++)
	{
		char name[32];
		String::Sprintf(name, sizeof(name), "adr%i", i);
		const char* adrstring = Cvar_VariableString(name);
		if (!adrstring || !adrstring[0])
		{
			continue;
		}

		common->Printf("pinging %s...\n", adrstring);
		netadr_t adr;
		if (!SOCK_StringToAdr(adrstring, &adr, Q2PORT_SERVER))
		{
			common->Printf("Bad address: %s\n", adrstring);
			continue;
		}
		NET_OutOfBandPrint(NS_CLIENT, adr, "info %i", Q2PROTOCOL_VERSION);
	}
}

void CLQ2_ClearState()
{
	Com_Memset(&clq2_entities, 0, sizeof(clq2_entities));
	CLQ2_ClearTEnts();
}

//	Specifies the model that will be used as the world
static void R_BeginRegistrationAndLoadWorld(const char* model)
{
	char fullname[MAX_QPATH];
	String::Sprintf(fullname, sizeof(fullname), "maps/%s.bsp", model);

	R_Shutdown(false);
	CL_InitRenderer();

	R_LoadWorld(fullname);
}

//	Call before entering a new level, or after changing dlls
void CLQ2_PrepRefresh()
{
	if (!cl.q2_configstrings[Q2CS_MODELS + 1][0])
	{
		return;		// no map loaded

	}
	// let the render dll load the map
	char mapname[32];
	String::Cpy(mapname, cl.q2_configstrings[Q2CS_MODELS + 1] + 5);		// skip "maps/"
	mapname[String::Length(mapname) - 4] = 0;		// cut off ".bsp"

	// register models, pics, and skins
	common->Printf("Map: %s\r", mapname);
	SCR_UpdateScreen();
	R_BeginRegistrationAndLoadWorld(mapname);
	common->Printf("                                     \r");

	// precache status bar pics
	common->Printf("pics\r");
	SCR_UpdateScreen();
	SCR_TouchPics();
	common->Printf("                                     \r");

	CLQ2_RegisterTEntModels();

	clq2_num_weaponmodels = 1;
	String::Cpy(clq2_weaponmodels[0], "weapon.md2");

	for (int i = 1; i < MAX_MODELS_Q2 && cl.q2_configstrings[Q2CS_MODELS + i][0]; i++)
	{
		char name[MAX_QPATH];
		String::Cpy(name, cl.q2_configstrings[Q2CS_MODELS + i]);
		name[37] = 0;	// never go beyond one line
		if (name[0] != '*')
		{
			common->Printf("%s\r", name);
		}
		SCR_UpdateScreen();
		if (name[0] == '#')
		{
			// special player weapon model
			if (clq2_num_weaponmodels < MAX_CLIENTWEAPONMODELS_Q2)
			{
				String::NCpy(clq2_weaponmodels[clq2_num_weaponmodels], cl.q2_configstrings[Q2CS_MODELS + i] + 1,
					sizeof(clq2_weaponmodels[clq2_num_weaponmodels]) - 1);
				clq2_num_weaponmodels++;
			}
		}
		else
		{
			cl.model_draw[i] = R_RegisterModel(cl.q2_configstrings[Q2CS_MODELS + i]);
			if (name[0] == '*')
			{
				cl.model_clip[i] = CM_InlineModel(String::Atoi(cl.q2_configstrings[Q2CS_MODELS + i] + 1));
			}
			else
			{
				cl.model_clip[i] = 0;
			}
		}
		if (name[0] != '*')
		{
			common->Printf("                                     \r");
		}
	}

	common->Printf("images\r");
	SCR_UpdateScreen();
	for (int i = 1; i < MAX_IMAGES_Q2 && cl.q2_configstrings[Q2CS_IMAGES + i][0]; i++)
	{
		cl.q2_image_precache[i] = R_RegisterPic(cl.q2_configstrings[Q2CS_IMAGES + i]);
	}

	common->Printf("                                     \r");
	for (int i = 0; i < MAX_CLIENTS_Q2; i++)
	{
		if (!cl.q2_configstrings[Q2CS_PLAYERSKINS + i][0])
		{
			continue;
		}
		common->Printf("client %i\r", i);
		SCR_UpdateScreen();
		CLQ2_ParseClientinfo(i);
		common->Printf("                                     \r");
	}

	CLQ2_LoadClientinfo(&cl.q2_baseclientinfo, "unnamed\\male/grunt");

	// set sky textures and speed
	common->Printf("sky\r");
	SCR_UpdateScreen();
	float rotate = String::Atof(cl.q2_configstrings[Q2CS_SKYROTATE]);
	vec3_t axis;
	sscanf(cl.q2_configstrings[Q2CS_SKYAXIS], "%f %f %f",
		&axis[0], &axis[1], &axis[2]);
	R_SetSky(cl.q2_configstrings[Q2CS_SKY], rotate, axis);
	common->Printf("                                     \r");

	R_EndRegistration();

	// clear any lines of console text
	Con_ClearNotify();

	SCR_UpdateScreen();
	cl.q2_refresh_prepped = true;

	// start the cd track
	CDAudio_Play(String::Atoi(cl.q2_configstrings[Q2CS_CDTRACK]), true);
}

void CLQ2_FixUpGender()
{
	if (clq2_gender_auto->value)
	{

		if (clq2_gender->modified)
		{
			// was set directly, don't override the user
			clq2_gender->modified = false;
			return;
		}

		char sk[80];
		String::NCpy(sk, clq2_skin->string, sizeof(sk) - 1);
		char* p;
		if ((p = strchr(sk, '/')) != NULL)
		{
			*p = 0;
		}
		if (String::ICmp(sk, "male") == 0 || String::ICmp(sk, "cyborg") == 0)
		{
			Cvar_SetLatched("gender", "male");
		}
		else if (String::ICmp(sk, "female") == 0 || String::ICmp(sk, "crackhor") == 0)
		{
			Cvar_SetLatched("gender", "female");
		}
		else
		{
			Cvar_SetLatched("gender", "none");
		}
		clq2_gender->modified = false;
	}
}

static void CLQ2_Pause_f()
{
	// never pause in multiplayer
	if (Cvar_VariableValue("maxclients") > 1 || !ComQ2_ServerState())
	{
		Cvar_SetValueLatched("paused", 0);
		return;
	}

	Cvar_SetValueLatched("paused", !cl_paused->value);
}

//	Just sent as a hint to the client that they should drop to full console
static void CLQ2_Changing_f()
{
	//ZOID
	//if we are downloading, we don't change!  This so we don't suddenly stop downloading a map
	if (clc.download)
	{
		return;
	}

	SCRQ2_BeginLoadingPlaque(false);
	cls.state = CA_CONNECTED;	// not active anymore, but not disconnected
	common->Printf("\nChanging map...\n");
}

//	Load or download any custom player skins and models
static void CLQ2_Skins_f()
{
	for (int i = 0; i < MAX_CLIENTS_Q2; i++)
	{
		if (!cl.q2_configstrings[Q2CS_PLAYERSKINS + i][0])
		{
			continue;
		}
		common->Printf("client %i: %s\n", i, cl.q2_configstrings[Q2CS_PLAYERSKINS + i]);
		SCR_UpdateScreen();
		Com_EventLoop();	// pump message loop
		CLQ2_ParseClientinfo(i);
	}
}

static void CLQ2_Userinfo_f()
{
	common->Printf("User info settings:\n");
	Info_Print(Cvar_InfoString(CVAR_USERINFO, MAX_INFO_STRING_Q2, MAX_INFO_KEY_Q2,
			MAX_INFO_VALUE_Q2, true, false));
}

//	The server will send this command right
// before allowing the client into the server
static void CLQ2_Precache_f()
{
	//Yet another hack to let old demos work
	//the old precache sequence
	if (Cmd_Argc() < 2)
	{
		int map_checksum;		// for detecting cheater maps
		CM_LoadMap(cl.q2_configstrings[Q2CS_MODELS + 1], true, &map_checksum);
		CLQ2_RegisterSounds();
		CLQ2_PrepRefresh();
		return;
	}

	clq2_precache_check = Q2CS_MODELS;
	clq2_precache_spawncount = String::Atoi(Cmd_Argv(1));
	clq2_precache_model = 0;
	clq2_precache_model_skin = 0;

	CLQ2_RequestNextDownload();
}

//	Request a download from the server
static void CLQ2_Download_f()
{
	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: download <filename>\n");
		return;
	}

	char filename[MAX_OSPATH];
	String::Sprintf(filename, sizeof(filename), "%s", Cmd_Argv(1));

	if (strstr(filename, ".."))
	{
		common->Printf("Refusing to download a path with ..\n");
		return;
	}

	if (FS_ReadFile(filename, NULL) != -1)
	{	// it exists, no need to download
		common->Printf("File already exists.\n");
		return;
	}

	String::Cpy(clc.downloadName, filename);
	common->Printf("Downloading %s\n", clc.downloadName);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
	String::StripExtension(clc.downloadName, clc.downloadTempName);
	String::Cat(clc.downloadTempName, sizeof(clc.downloadTempName), ".tmp");

	CL_AddReliableCommand(va("download %s", clc.downloadName));

	clc.downloadNumber++;
}

//	Restart the sound subsystem so it can pick up
// new parameters and flush all sounds
static void CLQ2_Snd_Restart_f()
{
	S_Shutdown();
	S_Init();
	CLQ2_RegisterSounds();
}

//	Console command to re-start the video mode and refresh.
static void VID_Restart_f()
{
	vid_restart_requested = true;
}

void CLQ2_Init()
{
	cls.realtime = Sys_Milliseconds();

	//
	// register our variables
	//
	clq2_footsteps = Cvar_Get("cl_footsteps", "1", 0);
	clq2_noskins = Cvar_Get("cl_noskins", "0", 0);
	clq2_predict = Cvar_Get("cl_predict", "1", 0);
	clq2_showmiss = Cvar_Get("clq2_showmiss", "0", 0);
	clq2_vwep = Cvar_Get("cl_vwep", "1", CVAR_ARCHIVE);
	cl_paused = Cvar_Get("paused", "0", CVAR_CHEAT);
	cl_timeout = Cvar_Get("cl_timeout", "120", 0);
	clq2_maxfps = Cvar_Get("cl_maxfps", "90", 0);

	Cvar_Get("adr0", "", CVAR_ARCHIVE);
	Cvar_Get("adr1", "", CVAR_ARCHIVE);
	Cvar_Get("adr2", "", CVAR_ARCHIVE);
	Cvar_Get("adr3", "", CVAR_ARCHIVE);
	Cvar_Get("adr4", "", CVAR_ARCHIVE);
	Cvar_Get("adr5", "", CVAR_ARCHIVE);
	Cvar_Get("adr6", "", CVAR_ARCHIVE);
	Cvar_Get("adr7", "", CVAR_ARCHIVE);
	Cvar_Get("adr8", "", CVAR_ARCHIVE);

	//
	// userinfo
	//
	Cvar_Get("password", "", CVAR_USERINFO);
	Cvar_Get("spectator", "0", CVAR_USERINFO);
	clq2_name = Cvar_Get("name", "unnamed", CVAR_USERINFO | CVAR_ARCHIVE);
	clq2_skin = Cvar_Get("skin", "male/grunt", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("rate", "25000", CVAR_USERINFO | CVAR_ARCHIVE);		// FIXME
	Cvar_Get("msg", "1", CVAR_USERINFO | CVAR_ARCHIVE);
	q2_hand = Cvar_Get("hand", "0", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("fov", "90", CVAR_USERINFO | CVAR_ARCHIVE);
	clq2_gender = Cvar_Get("gender", "male", CVAR_USERINFO | CVAR_ARCHIVE);
	clq2_gender_auto = Cvar_Get("gender_auto", "1", CVAR_ARCHIVE);
	clq2_gender->modified = false;	// clear this so we know when user sets it manually

	//
	// register our commands
	//
	Cmd_AddCommand("pingservers", CLQ2_PingServers_f);
	Cmd_AddCommand("record", CLQ2_Record_f);
	Cmd_AddCommand("stop", CLQ2_Stop_f);
	Cmd_AddCommand("connect", CLQ2_Connect_f);
	Cmd_AddCommand("reconnect", CLQ2_Reconnect_f);
	Cmd_AddCommand("pause", CLQ2_Pause_f);
	Cmd_AddCommand("changing", CLQ2_Changing_f);
	Cmd_AddCommand("skins", CLQ2_Skins_f);
	Cmd_AddCommand("userinfo", CLQ2_Userinfo_f);
	Cmd_AddCommand("precache", CLQ2_Precache_f);
	Cmd_AddCommand("download", CLQ2_Download_f);
	Cmd_AddCommand("snd_restart", CLQ2_Snd_Restart_f);
	Cmd_AddCommand("vid_restart", VID_Restart_f);

	//
	// forward to server commands
	//
	// the only thing this does is allow command completion
	// to work -- all unknown commands are automatically
	// forwarded to the server
	Cmd_AddCommand("wave", NULL);
	Cmd_AddCommand("inven", NULL);
	Cmd_AddCommand("kill", NULL);
	Cmd_AddCommand("use", NULL);
	Cmd_AddCommand("drop", NULL);
	Cmd_AddCommand("say", NULL);
	Cmd_AddCommand("say_team", NULL);
	Cmd_AddCommand("info", NULL);
	Cmd_AddCommand("prog", NULL);
	Cmd_AddCommand("give", NULL);
	Cmd_AddCommand("god", NULL);
	Cmd_AddCommand("notarget", NULL);
	Cmd_AddCommand("noclip", NULL);
	Cmd_AddCommand("invuse", NULL);
	Cmd_AddCommand("invprev", NULL);
	Cmd_AddCommand("invnext", NULL);
	Cmd_AddCommand("invdrop", NULL);
	Cmd_AddCommand("weapnext", NULL);
	Cmd_AddCommand("weapprev", NULL);

	cls.disable_screen = true;	// don't draw yet

	FS_ExecAutoexec();
	Cbuf_Execute();
}

void CLQ2_UpdateSounds()
{
	if (cl_paused->value)
	{
		return;
	}

	if (!cl.q2_sound_prepped)
	{
		return;
	}

	for (int i = 0; i < MAX_EDICTS_Q2; i++)
	{
		vec3_t origin;
		CLQ2_GetEntitySoundOrigin(i, origin);
		S_UpdateEntityPosition(i, origin);
	}

	S_ClearLoopingSounds(false);
	for (int i = 0; i < cl.q2_frame.num_entities; i++)
	{
		int num = (cl.q2_frame.parse_entities + i) & (MAX_PARSE_ENTITIES_Q2 - 1);
		q2entity_state_t* ent = &clq2_parse_entities[num];
		if (!ent->sound)
		{
			continue;
		}
		S_AddLoopingSound(num, ent->origin, vec3_origin, 0, cl.sound_precache[ent->sound], 0, 0);
	}
}

//	This function gets called once just before drawing each frame, and it's sole purpose in life
// is to check to see if any of the video mode parameters have changed, and if they have to
// update the rendering DLL and/or video mode to match.
void CLQ2_CheckVidChanges()
{
	if (vid_restart_requested)
	{
		S_StopAllSounds();
		/*
		** refresh has changed
		*/
		vid_restart_requested = false;
		cl.q2_refresh_prepped = false;
		cls.disable_screen = true;

		R_Shutdown(true);
		CL_InitRenderer();
		cls.disable_screen = false;
	}
}

void CLQ2_FixCvarCheats()
{
	if (!String::Cmp(cl.q2_configstrings[Q2CS_MAXCLIENTS], "1") ||
		!cl.q2_configstrings[Q2CS_MAXCLIENTS][0])
	{
		return;		// single player can cheat
	}
	Cvar_SetCheatState();
}
