// cl_parse.c  -- parse a message received from the server

#include "quakedef.h"

const char* svc_strings[] =
{
	"h2svc_bad",
	"h2svc_nop",
	"h2svc_disconnect",
	"h2svc_updatestat",
	"h2svc_version",		// [long] server version
	"h2svc_setview",		// [short] entity number
	"h2svc_sound",			// <see code>
	"h2svc_time",			// [float] server time
	"h2svc_print",			// [string] null terminated string
	"h2svc_stufftext",		// [string] stuffed into client's console buffer
	// the string should be \n terminated
	"h2svc_setangle",		// [vec3] set the view angle to this absolute value

	"hwsvc_serverdata",		// [long] version ...
	"h2svc_lightstyle",		// [byte] [string]
	"h2svc_updatename",		// [byte] [string]
	"h2svc_updatefrags",	// [byte] [short]
	"h2svc_clientdata",		// <shortbits + data>
	"h2svc_stopsound",		// <see code>
	"h2svc_updatecolors",	// [byte] [byte]
	"h2svc_particle",		// [vec3] <variable>
	"h2svc_damage",			// [byte] impact [byte] blood [vec3] from

	"h2svc_spawnstatic",
	"OBSOLETE svc_spawnbinary",
	"h2svc_spawnbaseline",

	"h2svc_temp_entity",		// <variable>
	"h2svc_setpause",
	"h2svc_signonnum",
	"h2svc_centerprint",
	"h2svc_killedmonster",
	"h2svc_foundsecret",
	"h2svc_spawnstaticsound",
	"h2svc_intermission",
	"h2svc_finale",

	"h2svc_cdtrack",
	"h2svc_sellscreen",

	"hwsvc_smallkick",
	"hwsvc_bigkick",

	"hwsvc_updateping",
	"hwsvc_updateentertime",

	"hwsvc_updatestatlong",
	"hwsvc_muzzleflash",
	"hwsvc_updateuserinfo",
	"hwsvc_download",
	"hwsvc_playerinfo",
	"hwsvc_nails",
	"svc_choke",
	"hwsvc_modellist",
	"hwsvc_soundlist",
	"hwsvc_packetentities",
	"hwsvc_deltapacketentities",
	"hwsvc_maxspeed",
	"hwsvc_entgravity",

	"hwsvc_plaque",
	"hwsvc_particle_explosion",
	"hwsvc_set_view_tint",
	"hwsvc_start_effect",
	"hwsvc_end_effect",
	"hwsvc_set_view_flags",
	"hwsvc_clear_view_flags",
	"hwsvc_update_inv",
	"hwsvc_particle2",
	"hwsvc_particle3",
	"hwsvc_particle4",
	"hwsvc_turn_effect",
	"hwsvc_update_effect",
	"hwsvc_multieffect",
	"hwsvc_midi_name",
	"hwsvc_raineffect",
	"hwsvc_packmissile",

	"hwsvc_indexed_print",
	"hwsvc_targetupdate",
	"hwsvc_name_print",
	"hwsvc_sound_update_pos",
	"hwsvc_update_piv",
	"hwsvc_player_sound",
	"hwsvc_updatepclass",
	"hwsvc_updatedminfo",	// [byte] [short] [byte]
	"hwsvc_updatesiegeinfo",	// [byte] [byte]
	"hwsvc_updatesiegeteam",	// [byte] [byte]
	"hwsvc_updatesiegelosses",	// [byte] [byte]
	"hwsvc_haskey",		//[byte] [byte]
	"hwsvc_nonehaskey",		//[byte]
	"hwsvc_isdoc",		//[byte] [byte]
	"hwsvc_nodoc",		//[byte]
	"hwsvc_playerskipped"	//[byte]
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL"
};

//=============================================================================

/*
===============
CL_CheckOrDownloadFile

Returns true if the file exists, otherwise it attempts
to start a download from the server.
===============
*/
qboolean    CL_CheckOrDownloadFile(char* filename)
{
	fileHandle_t f;

	if (strstr(filename, ".."))
	{
		common->Printf("Refusing to download a path with ..\n");
		return true;
	}

	FS_FOpenFileRead(filename, &f, true);
	if (f)
	{	// it exists, no need to download
		FS_FCloseFile(f);
		return true;
	}

	//ZOID - can't download when recording
	if (clc.demorecording)
	{
		common->Printf("Unable to download %s in record mode.\n", clc.downloadName);
		return true;
	}
	//ZOID - can't download when playback
	if (clc.demoplaying)
	{
		return true;
	}

	String::Cpy(clc.downloadName, filename);
	common->Printf("Downloading %s...\n", clc.downloadName);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
	String::StripExtension(clc.downloadName, clc.downloadTempName);
	String::Cat(clc.downloadTempName, sizeof(clc.downloadTempName), ".tmp");

	clc.netchan.message.WriteByte(h2clc_stringcmd);
	clc.netchan.message.WriteString2(
		va("download %s", clc.downloadName));

	clc.downloadNumber++;

	return false;
}

/*
===============
R_NewMap
===============
*/
static void R_NewMap(void)
{
	CL_ClearParticles();

	R_EndRegistration();
}

/*
=================
Model_NextDownload
=================
*/
void Model_NextDownload(void)
{
	char* s;
	int i;
	extern char gamedirfile[];

	if (clc.downloadNumber == 0)
	{
		common->Printf("Checking models...\n");
		clc.downloadNumber = 1;
	}

	clc.downloadType = dl_model;
	for (
		; cl.qh_model_name[clc.downloadNumber][0]
		; clc.downloadNumber++)
	{
		s = cl.qh_model_name[clc.downloadNumber];
		if (s[0] == '*')
		{
			continue;	// inline brush model
		}
		if (!CL_CheckOrDownloadFile(s))
		{
			return;		// started a download
		}
	}

	CM_LoadMap(cl.qh_model_name[1], true, NULL);
	cl.model_clip[1] = 0;
	R_LoadWorld(cl.qh_model_name[1]);

	for (i = 2; i < MAX_MODELS_H2; i++)
	{
		if (!cl.qh_model_name[i][0])
		{
			break;
		}
		cl.model_draw[i] = R_RegisterModel(cl.qh_model_name[i]);
		if (cl.qh_model_name[i][0] == '*')
		{
			cl.model_clip[i] = CM_InlineModel(String::Atoi(cl.qh_model_name[i] + 1));
		}
		if (!cl.model_draw[i])
		{
			common->Printf("\nThe required model file '%s' could not be found or downloaded.\n\n",
				cl.qh_model_name[i]);
			common->Printf("You may need to download or purchase a %s client "
					   "pack in order to play on this server.\n\n", gamedirfile);
			CL_Disconnect();
			return;
		}
	}

	// all done
	R_NewMap();

	Hunk_Check();		// make sure nothing is hurt

	// done with modellist, request first of static signon messages
	clc.netchan.message.WriteByte(h2clc_stringcmd);
	clc.netchan.message.WriteString2(
		va("prespawn %i", cl.servercount));
}

/*
=================
Sound_NextDownload
=================
*/
void Sound_NextDownload(void)
{
	char* s;
	int i;

	if (clc.downloadNumber == 0)
	{
		common->Printf("Checking sounds...\n");
		clc.downloadNumber = 1;
	}

	clc.downloadType = dl_sound;
	for (
		; cl.qh_sound_name[clc.downloadNumber][0]
		; clc.downloadNumber++)
	{
		s = cl.qh_sound_name[clc.downloadNumber];
		if (!CL_CheckOrDownloadFile(va("sound/%s",s)))
		{
			return;		// started a download
		}
	}

	S_BeginRegistration();
	for (i = 1; i < MAX_SOUNDS_HW; i++)
	{
		if (!cl.qh_sound_name[i][0])
		{
			break;
		}
		cl.sound_precache[i] = S_RegisterSound(cl.qh_sound_name[i]);
	}
	S_EndRegistration();

	// done with sounds, request models now
	clc.netchan.message.WriteByte(h2clc_stringcmd);
	clc.netchan.message.WriteString2(
		va("modellist %i", cl.servercount));
}


/*
======================
CL_RequestNextDownload
======================
*/
void CL_RequestNextDownload(void)
{
	switch (clc.downloadType)
	{
	case dl_single:
		break;
	case dl_model:
		Model_NextDownload();
		break;
	case dl_sound:
		Sound_NextDownload();
		break;
	case dl_none:
		break;
	}
}

/*
=====================
CL_ParseDownload

A download message has been received from the server
=====================
*/
void CL_ParseDownload(void)
{
	int size, percent;

	// read the data
	size = net_message.ReadShort();
	percent = net_message.ReadByte();

	if (clc.demoplaying)
	{
		if (size > 0)
		{
			net_message.readcount += size;
		}
		return;	// not in demo playback
	}

	if (size == -1)
	{
		common->Printf("File not found.\n");
		if (clc.download)
		{
			common->Printf("cls.download shouldn't have been set\n");
			FS_FCloseFile(clc.download);
			clc.download = 0;
		}
		CL_RequestNextDownload();
		return;
	}

	// open the file if not opened yet
	if (!clc.download)
	{
		clc.download = FS_FOpenFileWrite(clc.downloadTempName);

		if (!clc.download)
		{
			net_message.readcount += size;
			common->Printf("Failed to open %s\n", clc.downloadTempName);
			CL_RequestNextDownload();
			return;
		}
	}

	FS_Write(net_message._data + net_message.readcount, size, clc.download);
	net_message.readcount += size;

	if (percent != 100)
	{
// change display routines by zoid
		// request next block
#if 0
		common->Printf(".");
		if (10 * (percent / 10) != cls.downloadpercent)
		{
			cls.downloadpercent = 10 * (percent / 10);
			common->Printf("%i%%", cls.downloadpercent);
		}
#endif
		clc.downloadPercent = percent;

		clc.netchan.message.WriteByte(h2clc_stringcmd);
		clc.netchan.message.WriteString2("nextdl");
	}
	else
	{
#if 0
		common->Printf("100%%\n");
#endif

		FS_FCloseFile(clc.download);

		// rename the temp file to it's final name
		FS_Rename(clc.downloadTempName, clc.downloadName);

		clc.download = 0;
		clc.downloadPercent = 0;

		// get another file if needed

		CL_RequestNextDownload();
	}
}


/*
=====================================================================

  SERVER CONNECTING MESSAGES

=====================================================================
*/

/*
==================
CL_ParseServerData
==================
*/
int cl_keyholder = -1;
int cl_doc = -1;
void CL_ParseServerData(void)
{
	char* str;
	qboolean cflag = false;
	extern char gamedirfile[MAX_OSPATH];
	int protover;

	common->DPrintf("Serverdata packet received.\n");
//
// wipe the clientActive_t struct
//
	CL_ClearState();

// parse protocol version number
	protover = net_message.ReadLong();
	if (protover != PROTOCOL_VERSION && protover != OLD_PROTOCOL_VERSION)
	{
		common->Error("Server returned version %i, not %i", protover, PROTOCOL_VERSION);
	}

	cl.servercount = net_message.ReadLong();

	// game directory
	str = const_cast<char*>(net_message.ReadString2());

	if (String::ICmp(gamedirfile, str))
	{
		// save current config
		Host_WriteConfiguration("config.cfg");
		cflag = true;
	}

	COM_Gamedir(str);

	//ZOID--run the autoexec.cfg in the gamedir
	//if it exists
	if (cflag)
	{
		if (FS_FileExists("config.cfg"))
		{
			Cbuf_AddText("cl_warncmd 0\n");
			Cbuf_AddText("exec config.cfg\n");
			Cbuf_AddText("exec frontend.cfg\n");
			Cbuf_AddText("cl_warncmd 1\n");
		}
	}

	// parse player slot, high bit means spectator
	cl.playernum = net_message.ReadByte();
	if (cl.playernum & 128)
	{
		cl.qh_spectator = true;
		cl.playernum &= ~128;
	}
	cl.viewentity = cl.playernum + 1;

	// get the full level name
	str = const_cast<char*>(net_message.ReadString2());
	String::NCpy(cl.qh_levelname, str, sizeof(cl.qh_levelname) - 1);

	// get the movevars
	if (protover == PROTOCOL_VERSION)
	{
		movevars.gravity            = net_message.ReadFloat();
		movevars.stopspeed          = net_message.ReadFloat();
		movevars.maxspeed           = net_message.ReadFloat();
		movevars.spectatormaxspeed  = net_message.ReadFloat();
		movevars.accelerate         = net_message.ReadFloat();
		movevars.airaccelerate      = net_message.ReadFloat();
		movevars.wateraccelerate    = net_message.ReadFloat();
		movevars.friction           = net_message.ReadFloat();
		movevars.waterfriction      = net_message.ReadFloat();
		movevars.entgravity         = net_message.ReadFloat();
	}
	else
	{
		movevars.gravity            = 800;
		movevars.stopspeed          = 100;
		movevars.maxspeed           = 320;
		movevars.spectatormaxspeed  = 500;
		movevars.accelerate         = 10;
		movevars.airaccelerate      = 0.7;
		movevars.wateraccelerate    = 10;
		movevars.friction           = 6;
		movevars.waterfriction      = 1;
		movevars.entgravity         = 1.0;
	}

	// seperate the printfs so the server message can have a color
	common->Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	common->Printf(S_COLOR_RED "%s" S_COLOR_WHITE "\n", str);

	// ask for the sound list next
	clc.netchan.message.WriteByte(h2clc_stringcmd);
	clc.netchan.message.WriteString2(
		va("soundlist %i", cl.servercount));

	// now waiting for downloads, etc
	cls.state = CA_LOADING;
	cl_keyholder = -1;
	cl_doc = -1;
}

/*
==================
CL_ParseSoundlist
==================
*/
void CL_ParseSoundlist(void)
{
	int numsounds;
	char* str;

// precache sounds
	Com_Memset(cl.sound_precache, 0, sizeof(cl.sound_precache));
	for (numsounds = 1;; numsounds++)
	{
		str = const_cast<char*>(net_message.ReadString2());
		if (!str[0])
		{
			break;
		}
		if (numsounds == MAX_SOUNDS_HW)
		{
			common->Error("Server sent too many sound_precache");
		}
		String::Cpy(cl.qh_sound_name[numsounds], str);
	}

	clc.downloadNumber = 0;
	clc.downloadType = dl_sound;
	Sound_NextDownload();
}

/*
==================
CL_ParseModellist
==================
*/
void CL_ParseModellist(void)
{
	int nummodels;
	char* str;

// precache models and note certain default indexes
	Com_Memset(cl.model_draw, 0, sizeof(cl.model_draw));
	clhw_playerindex[0] = -1;
	clhw_playerindex[1] = -1;
	clhw_playerindex[2] = -1;
	clhw_playerindex[3] = -1;
	clhw_playerindex[4] = -1;
	clhw_playerindex[5] = -1;	//mg-siege
	clh2_ballindex = -1;
	clh2_missilestarindex = -1;
	clh2_ravenindex = -1;
	clh2_raven2index = -1;

	for (nummodels = 1;; nummodels++)
	{
		str = const_cast<char*>(net_message.ReadString2());
		if (!str[0])
		{
			break;
		}
		if (nummodels == MAX_MODELS_H2)
		{
			common->Error("Server sent too many model_precache");
		}
		String::Cpy(cl.qh_model_name[nummodels], str);

		if (!String::Cmp(cl.qh_model_name[nummodels],"models/paladin.mdl"))
		{
			clhw_playerindex[0] = nummodels;
		}
		if (!String::Cmp(cl.qh_model_name[nummodels],"models/crusader.mdl"))
		{
			clhw_playerindex[1] = nummodels;
		}
		if (!String::Cmp(cl.qh_model_name[nummodels],"models/necro.mdl"))
		{
			clhw_playerindex[2] = nummodels;
		}
		if (!String::Cmp(cl.qh_model_name[nummodels],"models/assassin.mdl"))
		{
			clhw_playerindex[3] = nummodels;
		}
		if (!String::Cmp(cl.qh_model_name[nummodels],"models/succubus.mdl"))
		{
			clhw_playerindex[4] = nummodels;
		}
		if (!String::Cmp(cl.qh_model_name[nummodels],"models/hank.mdl"))
		{
			clhw_playerindex[5] = nummodels;	//mg-siege
		}
		if (!String::Cmp(cl.qh_model_name[nummodels],"models/ball.mdl"))
		{
			clh2_ballindex = nummodels;
		}
		if (!String::Cmp(cl.qh_model_name[nummodels],"models/newmmis.mdl"))
		{
			clh2_missilestarindex = nummodels;
		}
		if (!String::Cmp(cl.qh_model_name[nummodels],"models/ravproj.mdl"))
		{
			clh2_ravenindex = nummodels;
		}
		if (!String::Cmp(cl.qh_model_name[nummodels],"models/vindsht1.mdl"))
		{
			clh2_raven2index = nummodels;
		}
	}

	clh2_player_models[0] = R_RegisterModel("models/paladin.mdl");
	clh2_player_models[1] = R_RegisterModel("models/crusader.mdl");
	clh2_player_models[2] = R_RegisterModel("models/necro.mdl");
	clh2_player_models[3] = R_RegisterModel("models/assassin.mdl");
	clh2_player_models[4] = R_RegisterModel("models/succubus.mdl");
//siege
	clh2_player_models[5] = R_RegisterModel("models/hank.mdl");

	clc.downloadNumber = 0;
	clc.downloadType = dl_model;
	Model_NextDownload();
}

/*
===================
CL_ParseStaticSound
===================
*/
void CL_ParseStaticSound(void)
{
	vec3_t org;
	int sound_num, vol, atten;
	int i;

	for (i = 0; i < 3; i++)
		org[i] = net_message.ReadCoord();
	sound_num = net_message.ReadByte();
	vol = net_message.ReadByte();
	atten = net_message.ReadByte();

	S_StaticSound(cl.sound_precache[sound_num], org, vol, atten);
}



/*
=====================================================================

ACTION MESSAGES

=====================================================================
*/

/*
==================
CL_ParseStartSoundPacket
==================
*/
void CL_ParseStartSoundPacket(void)
{
	vec3_t pos;
	int channel, ent;
	int sound_num;
	int volume;
	float attenuation;
	int i;

	channel = net_message.ReadShort();

	if (channel & QHWSND_VOLUME)
	{
		volume = net_message.ReadByte();
	}
	else
	{
		volume = QHDEFAULT_SOUND_PACKET_VOLUME;
	}

	if (channel & QHWSND_ATTENUATION)
	{
		attenuation = net_message.ReadByte() / 32.0;
	}
	else
	{
		attenuation = QHDEFAULT_SOUND_PACKET_ATTENUATION;
	}

	sound_num = net_message.ReadByte();

	for (i = 0; i < 3; i++)
		pos[i] = net_message.ReadCoord();

	ent = (channel >> 3) & 1023;
	channel &= 7;

	if (ent > MAX_EDICTS_QH)
	{
		common->Error("CL_ParseStartSoundPacket: ent = %i", ent);
	}

	S_StartSound(pos, ent, channel, cl.sound_precache[sound_num], volume / 255.0, attenuation);
}


/*
==================
CL_ParseClientdata

Server information pertaining to this client only, sent every frame
==================
*/
void CL_ParseClientdata(void)
{
	float latency;
	hwframe_t* frame;

// calculate simulated time of message
	cl.qh_parsecount = clc.netchan.incomingAcknowledged;
	frame = &cl.hw_frames[cl.qh_parsecount & UPDATE_MASK_HW];

	frame->receivedtime = realtime;

// calculate latency
	latency = frame->receivedtime - frame->senttime;

	if (latency < 0 || latency > 1.0)
	{
//		common->Printf ("Odd latency: %5.2f\n", latency);
	}
	else
	{
		// drift the average latency towards the observed latency
		if (latency < cls.qh_latency)
		{
			cls.qh_latency = latency;
		}
		else
		{
			cls.qh_latency += 0.001;	// drift up, so correction are needed
		}
	}
}

/*
=====================
CL_NewTranslation
=====================
*/
void CL_NewTranslation(int slot)
{
	if (slot > MAX_CLIENTS_QHW)
	{
		common->FatalError("CL_NewTranslation: slot > MAX_CLIENTS_QHW");
	}

	CLH2_TranslatePlayerSkin(slot);
}

/*
==============
CL_UpdateUserinfo
==============
*/
void CL_UpdateUserinfo(void)
{
	int slot;
	h2player_info_t* player;

	slot = net_message.ReadByte();
	if (slot >= MAX_CLIENTS_QHW)
	{
		common->Error("CL_ParseServerMessage: hwsvc_updateuserinfo > MAX_CLIENTS_QHW");
	}

	player = &cl.h2_players[slot];
	player->userid = net_message.ReadLong();
	String::NCpy(player->userinfo, net_message.ReadString2(), sizeof(player->userinfo) - 1);

	String::NCpy(player->name, Info_ValueForKey(player->userinfo, "name"), sizeof(player->name) - 1);
	player->topColour = String::Atoi(Info_ValueForKey(player->userinfo, "topcolor"));
	player->bottomColour = String::Atoi(Info_ValueForKey(player->userinfo, "bottomcolor"));
	if (Info_ValueForKey(player->userinfo, "*spectator")[0])
	{
		player->spectator = true;
	}
	else
	{
		player->spectator = false;
	}

	player->playerclass = String::Atoi(Info_ValueForKey(player->userinfo, "playerclass"));
/*	if (cl.playernum == slot && player->playerclass != playerclass.value)
    {
        Cvar_SetValue ("playerclass",player->playerclass);
    }
*/
	Sbar_Changed();
	player->Translated = false;
	CL_NewTranslation(slot);
}


/*
=====================
CL_SetStat
=====================
*/
void CL_SetStat(int stat, int value)
{
	if (stat < 0 || stat >= MAX_CL_STATS)
	{
		common->FatalError("CL_SetStat: %i is invalid", stat);
	}

	Sbar_Changed();

	cl.qh_stats[stat] = value;
}

/*
==============
CL_MuzzleFlash
==============
*/
void CL_MuzzleFlash()
{
	int i = net_message.ReadShort();
	if ((unsigned)(i - 1) >= MAX_CLIENTS_QHW)
	{
		return;
	}
	hwplayer_state_t* pl = &cl.hw_frames[cl.qh_parsecount & UPDATE_MASK_HW].playerstate[i - 1];
	CLH2_MuzzleFlashLight(i, pl->origin, pl->viewangles, false);
}

void CL_Plaque(void)
{
	int index;

	index = net_message.ReadShort();

	if (index > 0 && index <= prh2_string_count)
	{
		plaquemessage = &prh2_global_strings[prh2_string_index[index - 1]];
	}
	else
	{
		plaquemessage = "";
	}
}

void CL_IndexedPrint(void)
{
	int i = net_message.ReadByte();
	if (i == PRINT_CHAT)
	{
		S_StartLocalSound("misc/talk.wav");
	}

	int index = net_message.ReadShort();

	if (index > 0 && index <= prh2_string_count)
	{
		if (i == PRINT_CHAT)
		{
			common->Printf(S_COLOR_RED "%s" S_COLOR_WHITE, &prh2_global_strings[prh2_string_index[index - 1]]);
		}
		else
		{
			common->Printf("%s",&prh2_global_strings[prh2_string_index[index - 1]]);
		}
	}
}

void CL_NamePrint(void)
{
	int i = net_message.ReadByte();
	int index = net_message.ReadByte();

	if (i == PRINT_CHAT)
	{
		S_StartLocalSound("misc/talk.wav");
	}

	if (index >= 0 && index < MAX_CLIENTS_QHW)
	{
		if (i == PRINT_CHAT)
		{
			common->Printf(S_COLOR_RED "%s" S_COLOR_WHITE, &cl.h2_players[index].name);
		}
		else
		{
			common->Printf("%s",&cl.h2_players[index].name);
		}
	}
}

void CL_ParticleExplosion(void)
{
	vec3_t org;
	short color, radius, counter;

	org[0] = net_message.ReadCoord();
	org[1] = net_message.ReadCoord();
	org[2] = net_message.ReadCoord();
	color = net_message.ReadShort();
	radius = net_message.ReadShort();
	counter = net_message.ReadShort();

	CLH2_ColouredParticleExplosion(org,color,radius,counter);
}

static void CL_ParsePrint()
{
	int i = net_message.ReadByte();
	const char* txt = net_message.ReadString2();

	if (i == PRINT_CHAT)
	{
		S_StartLocalSound("misc/talk.wav");
		common->Printf(S_COLOR_RED "%s" S_COLOR_WHITE, txt);
	}
	else if (i >= PRINT_SOUND)
	{
		if (!talksounds->value)
		{
			return;
		}
		char temp[100];
		sprintf(temp, "taunt/taunt%.3d.wav", i - PRINT_SOUND + 1);
		S_StartLocalSound(temp);
		common->Printf(S_COLOR_RED "%s" S_COLOR_WHITE, txt);
	}
	else
	{
		common->Printf("%s", txt);
	}
}

#define SHOWNET(x) if (cl_shownet->value == 2) {common->Printf("%3i:%s\n", net_message.readcount - 1, x); }
/*
=====================
CL_ParseServerMessage
=====================
*/
int received_framecount;
int LastServerMessageSize = 0;

byte cl_fraglimit;
float cl_timelimit;
float cl_server_time_offset;
unsigned int defLosses;		// Defenders losses in Siege
unsigned int attLosses;		// Attackers Losses in Siege
void CL_ParseServerMessage(void)
{
	int cmd;
	char* s;
	int i, j;
	unsigned sc1, sc2;
	byte test;
	vec3_t pos;

	LastServerMessageSize += net_message.cursize;

	received_framecount = host_framecount;
	CLH2_ClearProjectiles();
	CLH2_ClearMissiles();
	// This clears out the target field on each netupdate; it won't be drawn unless another update comes...
	CLHW_ClearTarget();

//
// if recording demos, copy the message out
//
	if (cl_shownet->value == 1)
	{
		common->Printf("%i ",net_message.cursize);
	}
	else if (cl_shownet->value == 2)
	{
		common->Printf("------------------\n");
	}


	CL_ParseClientdata();

//
// parse the message
//
	while (1)
	{
		if (net_message.badread)
		{
			common->Error("CL_ParseServerMessage: Bad server message");
			break;
		}

		cmd = net_message.ReadByte();

		if (cmd == -1)
		{
			net_message.readcount++;	// so the EOM showner has the right value
			SHOWNET("END OF MESSAGE");
			break;
		}

		SHOWNET(svc_strings[cmd]);

		// other commands
		switch (cmd)
		{
		default:
			common->Error("CL_ParseServerMessage: Illegible server message\n");
			break;

		case h2svc_nop:
//			common->Printf ("h2svc_nop\n");
			break;

		case h2svc_disconnect:
			common->Error("Server disconnected\n");
			break;

		case h2svc_print:
			CL_ParsePrint();
			break;

		case h2svc_centerprint:
			SCR_CenterPrint(const_cast<char*>(net_message.ReadString2()));
			break;

		case h2svc_stufftext:
			s = const_cast<char*>(net_message.ReadString2());
			common->DPrintf("stufftext: %s\n", s);
			Cbuf_AddText(s);
			break;

		case h2svc_damage:
			V_ParseDamage();
			break;

		case hwsvc_serverdata:
			Cbuf_Execute();			// make sure any stuffed commands are done
			CL_ParseServerData();
			break;

		case h2svc_setangle:
			for (i = 0; i < 3; i++)
				cl.viewangles[i] = net_message.ReadAngle();
//			cl.viewangles[PITCH] = cl.viewangles[ROLL] = 0;
			break;

		case h2svc_lightstyle:
			i = net_message.ReadByte();
			CL_SetLightStyle(i, net_message.ReadString2());
			break;

		case h2svc_sound:
			CL_ParseStartSoundPacket();
			break;

		case hwsvc_sound_update_pos:
		{	//FIXME: put a field on the entity that lists the channels
		//it should update when it moves- if a certain flag
		//is on the ent, this update_channels field could
		//be set automatically by each sound and stopSound
		//called for this ent?
			vec3_t pos;
			int channel, ent;

			channel = net_message.ReadShort();

			ent = channel >> 3;
			channel &= 7;

			if (ent > MAX_EDICTS_QH)
			{
				Host_FatalError("hwsvc_sound_update_pos: ent = %i", ent);
			}

			for (i = 0; i < 3; i++)
				pos[i] = net_message.ReadCoord();

			S_UpdateSoundPos(ent, channel, pos);
		}
		break;

		case h2svc_stopsound:
			i = net_message.ReadShort();
			S_StopSound(i >> 3, i & 7);
			break;

		case h2svc_updatefrags:
			Sbar_Changed();
			i = net_message.ReadByte();
			if (i >= MAX_CLIENTS_QHW)
			{
				common->Error("CL_ParseServerMessage: h2svc_updatefrags > MAX_CLIENTS_QHW");
			}
			cl.h2_players[i].frags = net_message.ReadShort();
			break;

		case hwsvc_updateping:
			i = net_message.ReadByte();
			if (i >= MAX_CLIENTS_QHW)
			{
				common->Error("CL_ParseServerMessage: hwsvc_updateping > MAX_CLIENTS_QHW");
			}
			cl.h2_players[i].ping = net_message.ReadShort();
			break;

		case hwsvc_updateentertime:
			// time is sent over as seconds ago
			i = net_message.ReadByte();
			if (i >= MAX_CLIENTS_QHW)
			{
				common->Error("CL_ParseServerMessage: hwsvc_updateentertime > MAX_CLIENTS_QHW");
			}
			cl.h2_players[i].entertime = realtime - net_message.ReadFloat();
			break;

		case hwsvc_updatepclass:
			// playerclass has changed for this dude
			i = net_message.ReadByte();
			if (i >= MAX_CLIENTS_QHW)
			{
				common->Error("CL_ParseServerMessage: hwsvc_updatepclass > MAX_CLIENTS_QHW");
			}
			cl.h2_players[i].playerclass = net_message.ReadByte();
			cl.h2_players[i].level = cl.h2_players[i].playerclass & 31;
			cl.h2_players[i].playerclass = cl.h2_players[i].playerclass >> 5;
			break;

		case hwsvc_updatedminfo:
			// This dude killed someone, update his frags and level
			i = net_message.ReadByte();
			if (i >= MAX_CLIENTS_QHW)
			{
				common->Error("CL_ParseServerMessage: hwsvc_updatedminfo > MAX_CLIENTS_QHW");
			}
			cl.h2_players[i].frags = net_message.ReadShort();
			cl.h2_players[i].playerclass = net_message.ReadByte();
			cl.h2_players[i].level = cl.h2_players[i].playerclass & 31;
			cl.h2_players[i].playerclass = cl.h2_players[i].playerclass >> 5;
			break;

		case hwsvc_updatesiegelosses:
			// This dude killed someone, update his frags and level
			defLosses = net_message.ReadByte();
			attLosses = net_message.ReadByte();
			break;

		case hwsvc_updatesiegeteam:
			// This dude killed someone, update his frags and level
			i = net_message.ReadByte();
			if (i >= MAX_CLIENTS_QHW)
			{
				common->Error("CL_ParseServerMessage: hwsvc_updatesiegeteam > MAX_CLIENTS_QHW");
			}
			cl.h2_players[i].siege_team = net_message.ReadByte();
			break;

		case hwsvc_updatesiegeinfo:
			// This dude killed someone, update his frags and level
			clhw_siege = true;
			cl_timelimit = net_message.ReadByte() * 60;
			cl_fraglimit = net_message.ReadByte();
			break;

		case hwsvc_haskey:
			cl_keyholder = net_message.ReadShort() - 1;
			break;

		case hwsvc_isdoc:
			cl_doc = net_message.ReadShort() - 1;
			break;

		case hwsvc_nonehaskey:
			cl_keyholder = -1;
			break;

		case hwsvc_nodoc:
			cl_doc = -1;
			break;

		case h2svc_time:
			cl_server_time_offset = ((int)net_message.ReadFloat()) - cl.qh_serverTimeFloat;
			break;

		case h2svc_spawnbaseline:
			CLH2_ParseSpawnBaseline(net_message);
			break;
		case h2svc_spawnstatic:
			CLH2_ParseSpawnStatic(net_message);
			break;
		case h2svc_temp_entity:
			CLHW_ParseTEnt(net_message);
			break;

		case h2svc_killedmonster:
			cl.qh_stats[STAT_MONSTERS]++;
			break;

		case h2svc_foundsecret:
			cl.qh_stats[STAT_SECRETS]++;
			break;

		case h2svc_updatestat:
			i = net_message.ReadByte();
			j = net_message.ReadByte();
			CL_SetStat(i, j);
			break;

		case hwsvc_updatestatlong:
			i = net_message.ReadByte();
			j = net_message.ReadLong();
			CL_SetStat(i, j);
			break;

		case h2svc_spawnstaticsound:
			CL_ParseStaticSound();
			break;

		case h2svc_cdtrack:
		{
			byte cdtrack = net_message.ReadByte();
			CDAudio_Play(cdtrack, true);
		}
		break;

		case h2svc_intermission:
//			if(clhw_siege)
//			{//MG
			cl.qh_intermission = net_message.ReadByte();
			cl.qh_completed_time = realtime;
			break;
/*			}
            else
            {//Old Quake way- won't work
                cl.intermission = 1;
                cl.completed_time = realtime;
                vid.recalc_refdef = true;	// go to full screen
                for (i=0 ; i<3 ; i++)
                    cl.simorg[i] = net_message.ReadCoord ();
                for (i=0 ; i<3 ; i++)
                    cl.simangles[i] = net_message.ReadAngle ();
                VectorCopy (vec3_origin, cl.simvel);
                break;
            }
*/
		case h2svc_finale:
			cl.qh_intermission = 2;
			cl.qh_completed_time = realtime;
			SCR_CenterPrint(const_cast<char*>(net_message.ReadString2()));
			break;

		case h2svc_sellscreen:
			Cmd_ExecuteString("help");
			break;

		case hwsvc_smallkick:
			cl.qh_punchangle = -2;
			break;

		case hwsvc_bigkick:
			cl.qh_punchangle = -4;
			break;

		case hwsvc_muzzleflash:
			CL_MuzzleFlash();
			break;

		case hwsvc_updateuserinfo:
			CL_UpdateUserinfo();
			break;

		case hwsvc_download:
			CL_ParseDownload();
			break;

		case hwsvc_playerinfo:
			CLHW_ParsePlayerinfo(net_message);
			break;
		case hwsvc_playerskipped:
			CLHW_SavePlayer(net_message);
			break;
		case hwsvc_nails:
			CLHW_ParseNails(net_message);
			break;
		case hwsvc_packmissile:
			CLHW_ParsePackMissiles(net_message);
			break;

		case hwsvc_chokecount:		// some preceding packets were choked
			i = net_message.ReadByte();
			for (j = 0; j < i; j++)
				cl.hw_frames[(clc.netchan.incomingAcknowledged - 1 - j) & UPDATE_MASK_HW].receivedtime = -2;
			break;

		case hwsvc_modellist:
			CL_ParseModellist();
			break;

		case hwsvc_soundlist:
			CL_ParseSoundlist();
			break;

		case hwsvc_packetentities:
			CLHW_ParsePacketEntities(net_message);
			break;
		case hwsvc_deltapacketentities:
			CLHW_ParseDeltaPacketEntities(net_message);
			break;

		case hwsvc_maxspeed:
			movevars.maxspeed = net_message.ReadFloat();
			break;

		case hwsvc_entgravity:
			movevars.entgravity = net_message.ReadFloat();
			break;

		case hwsvc_plaque:
			CL_Plaque();
			break;

		case hwsvc_indexed_print:
			CL_IndexedPrint();
			break;

		case hwsvc_name_print:
			CL_NamePrint();
			break;

		case hwsvc_particle_explosion:
			CL_ParticleExplosion();
			break;

		case hwsvc_set_view_tint:
			i = net_message.ReadByte();
//				cl.h2_viewent.colorshade = i;
			break;

		case hwsvc_start_effect:
			CLH2_ParseEffect(net_message);
			break;
		case hwsvc_end_effect:
			CLH2_ParseEndEffect(net_message);
			break;

		case hwsvc_set_view_flags:
			cl.h2_viewent.state.drawflags |= net_message.ReadByte();
			break;

		case hwsvc_clear_view_flags:
			cl.h2_viewent.state.drawflags &= ~net_message.ReadByte();
			break;

		case hwsvc_update_inv:
			sc1 = sc2 = 0;

			test = net_message.ReadByte();
			if (test & 1)
			{
				sc1 |= ((int)net_message.ReadByte());
			}
			if (test & 2)
			{
				sc1 |= ((int)net_message.ReadByte()) << 8;
			}
			if (test & 4)
			{
				sc1 |= ((int)net_message.ReadByte()) << 16;
			}
			if (test & 8)
			{
				sc1 |= ((int)net_message.ReadByte()) << 24;
			}
			if (test & 16)
			{
				sc2 |= ((int)net_message.ReadByte());
			}
			if (test & 32)
			{
				sc2 |= ((int)net_message.ReadByte()) << 8;
			}
			if (test & 64)
			{
				sc2 |= ((int)net_message.ReadByte()) << 16;
			}
			if (test & 128)
			{
				sc2 |= ((int)net_message.ReadByte()) << 24;
			}

			if (sc1 & SC1_HEALTH)
			{
				cl.h2_v.health = net_message.ReadShort();
			}
			if (sc1 & SC1_LEVEL)
			{
				cl.h2_v.level = net_message.ReadByte();
			}
			if (sc1 & SC1_INTELLIGENCE)
			{
				cl.h2_v.intelligence = net_message.ReadByte();
			}
			if (sc1 & SC1_WISDOM)
			{
				cl.h2_v.wisdom = net_message.ReadByte();
			}
			if (sc1 & SC1_STRENGTH)
			{
				cl.h2_v.strength = net_message.ReadByte();
			}
			if (sc1 & SC1_DEXTERITY)
			{
				cl.h2_v.dexterity = net_message.ReadByte();
			}
//				if (sc1 & SC1_WEAPON)
//					cl.h2_v.weapon = net_message.ReadByte();
			if (sc1 & SC1_TELEPORT_TIME)
			{
//				common->Printf("Teleport_time>time, got bit\n");
				cl.h2_v.teleport_time = realtime + 2;	//can't airmove for 2 seconds
			}

			if (sc1 & SC1_BLUEMANA)
			{
				cl.h2_v.bluemana = net_message.ReadByte();
			}
			if (sc1 & SC1_GREENMANA)
			{
				cl.h2_v.greenmana = net_message.ReadByte();
			}
			if (sc1 & SC1_EXPERIENCE)
			{
				cl.h2_v.experience = net_message.ReadLong();
			}
			if (sc1 & SC1_CNT_TORCH)
			{
				cl.h2_v.cnt_torch = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_H_BOOST)
			{
				cl.h2_v.cnt_h_boost = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_SH_BOOST)
			{
				cl.h2_v.cnt_sh_boost = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_MANA_BOOST)
			{
				cl.h2_v.cnt_mana_boost = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_TELEPORT)
			{
				cl.h2_v.cnt_teleport = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_TOME)
			{
				cl.h2_v.cnt_tome = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_SUMMON)
			{
				cl.h2_v.cnt_summon = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_INVISIBILITY)
			{
				cl.h2_v.cnt_invisibility = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_GLYPH)
			{
				cl.h2_v.cnt_glyph = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_HASTE)
			{
				cl.h2_v.cnt_haste = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_BLAST)
			{
				cl.h2_v.cnt_blast = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_POLYMORPH)
			{
				cl.h2_v.cnt_polymorph = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_FLIGHT)
			{
				cl.h2_v.cnt_flight = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_CUBEOFFORCE)
			{
				cl.h2_v.cnt_cubeofforce = net_message.ReadByte();
			}
			if (sc1 & SC1_CNT_INVINCIBILITY)
			{
				cl.h2_v.cnt_invincibility = net_message.ReadByte();
			}
			if (sc1 & SC1_ARTIFACT_ACTIVE)
			{
				cl.h2_v.artifact_active = net_message.ReadByte();
			}
			if (sc1 & SC1_ARTIFACT_LOW)
			{
				cl.h2_v.artifact_low = net_message.ReadByte();
			}
			if (sc1 & SC1_MOVETYPE)
			{
				cl.h2_v.movetype = net_message.ReadByte();
			}
			if (sc1 & SC1_CAMERAMODE)
			{
				cl.h2_v.cameramode = net_message.ReadByte();
			}
			if (sc1 & SC1_HASTED)
			{
				cl.h2_v.hasted = net_message.ReadFloat();
			}
			if (sc1 & SC1_INVENTORY)
			{
				cl.h2_v.inventory = net_message.ReadByte();
			}
			if (sc1 & SC1_RINGS_ACTIVE)
			{
				cl.h2_v.rings_active = net_message.ReadByte();
			}

			if (sc2 & SC2_RINGS_LOW)
			{
				cl.h2_v.rings_low = net_message.ReadByte();
			}
			if (sc2 & SC2_AMULET)
			{
				cl.h2_v.armor_amulet = net_message.ReadByte();
			}
			if (sc2 & SC2_BRACER)
			{
				cl.h2_v.armor_bracer = net_message.ReadByte();
			}
			if (sc2 & SC2_BREASTPLATE)
			{
				cl.h2_v.armor_breastplate = net_message.ReadByte();
			}
			if (sc2 & SC2_HELMET)
			{
				cl.h2_v.armor_helmet = net_message.ReadByte();
			}
			if (sc2 & SC2_FLIGHT_T)
			{
				cl.h2_v.ring_flight = net_message.ReadByte();
			}
			if (sc2 & SC2_WATER_T)
			{
				cl.h2_v.ring_water = net_message.ReadByte();
			}
			if (sc2 & SC2_TURNING_T)
			{
				cl.h2_v.ring_turning = net_message.ReadByte();
			}
			if (sc2 & SC2_REGEN_T)
			{
				cl.h2_v.ring_regeneration = net_message.ReadByte();
			}
//				if (sc2 & SC2_HASTE_T)
//					cl.h2_v.haste_time = net_message.ReadFloat();
//				if (sc2 & SC2_TOME_T)
//					cl.h2_v.tome_time = net_message.ReadFloat();
			if (sc2 & SC2_PUZZLE1)
			{
				sprintf(cl.h2_puzzle_pieces[0], "%.9s", net_message.ReadString2());
			}
			if (sc2 & SC2_PUZZLE2)
			{
				sprintf(cl.h2_puzzle_pieces[1], "%.9s", net_message.ReadString2());
			}
			if (sc2 & SC2_PUZZLE3)
			{
				sprintf(cl.h2_puzzle_pieces[2], "%.9s", net_message.ReadString2());
			}
			if (sc2 & SC2_PUZZLE4)
			{
				sprintf(cl.h2_puzzle_pieces[3], "%.9s", net_message.ReadString2());
			}
			if (sc2 & SC2_PUZZLE5)
			{
				sprintf(cl.h2_puzzle_pieces[4], "%.9s", net_message.ReadString2());
			}
			if (sc2 & SC2_PUZZLE6)
			{
				sprintf(cl.h2_puzzle_pieces[5], "%.9s", net_message.ReadString2());
			}
			if (sc2 & SC2_PUZZLE7)
			{
				sprintf(cl.h2_puzzle_pieces[6], "%.9s", net_message.ReadString2());
			}
			if (sc2 & SC2_PUZZLE8)
			{
				sprintf(cl.h2_puzzle_pieces[7], "%.9s", net_message.ReadString2());
			}
			if (sc2 & SC2_MAXHEALTH)
			{
				cl.h2_v.max_health = net_message.ReadShort();
			}
			if (sc2 & SC2_MAXMANA)
			{
				cl.h2_v.max_mana = net_message.ReadByte();
			}
			if (sc2 & SC2_FLAGS)
			{
				cl.h2_v.flags = net_message.ReadFloat();
			}

			if ((sc1 & SC1_INV) || (sc2 & SC2_INV))
			{
				SB_InvChanged();
			}
			break;

		case h2svc_particle:
			R_ParseParticleEffect();
			break;

		case hwsvc_particle2:
			R_ParseParticleEffect2();
			break;

		case hwsvc_particle3:
			R_ParseParticleEffect3();
			break;

		case hwsvc_particle4:
			R_ParseParticleEffect4();
			break;

		case hwsvc_turn_effect:
			CLHW_ParseTurnEffect(net_message);
			break;
		case hwsvc_update_effect:
			CLHW_ParseReviseEffect(net_message);
			break;
		case hwsvc_multieffect:
			CLHW_ParseMultiEffect(net_message);
			break;

		case hwsvc_midi_name:
		{
			char midi_name[MAX_QPATH];		// midi file name
			String::Cpy(midi_name,net_message.ReadString2());
			if (String::ICmp(bgmtype->string,"midi") == 0)
			{
				MIDI_Play(midi_name);
			}
			else
			{
				MIDI_Stop();
			}
		}
		break;

		case hwsvc_raineffect:
			R_ParseRainEffect();
			break;

		case hwsvc_targetupdate:
			CLHW_ParseTarget(net_message);
			break;

		case hwsvc_update_piv:
			cl.hw_PIV = net_message.ReadLong();
			break;

		case hwsvc_player_sound:
			test = net_message.ReadByte();
			pos[0] = net_message.ReadCoord();
			pos[1] = net_message.ReadCoord();
			pos[2] = net_message.ReadCoord();
			i = net_message.ReadShort();
			S_StartSound(pos, test, 1, cl.sound_precache[i], 1.0, 1.0);
			break;
		}
	}

	CLHW_SetSolidEntities();
}
