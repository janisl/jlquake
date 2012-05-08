/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cl_parse.c  -- parse a message received from the server

#include "quakedef.h"

const char* svc_strings[] =
{
	"q1svc_bad",
	"q1svc_nop",
	"q1svc_disconnect",
	"q1svc_updatestat",
	"q1svc_version",		// [long] server version
	"q1svc_setview",		// [short] entity number
	"q1svc_sound",			// <see code>
	"q1svc_time",			// [float] server time
	"q1svc_print",			// [string] null terminated string
	"q1svc_stufftext",		// [string] stuffed into client's console buffer
	// the string should be \n terminated
	"q1svc_setangle",		// [vec3] set the view angle to this absolute value

	"qwsvc_serverdata",		// [long] version ...
	"q1svc_lightstyle",		// [byte] [string]
	"q1svc_updatename",		// [byte] [string]
	"q1svc_updatefrags",	// [byte] [short]
	"q1svc_clientdata",		// <shortbits + data>
	"q1svc_stopsound",		// <see code>
	"q1svc_updatecolors",	// [byte] [byte]
	"q1svc_particle",		// [vec3] <variable>
	"q1svc_damage",			// [byte] impact [byte] blood [vec3] from

	"q1svc_spawnstatic",
	"OBSOLETE svc_spawnbinary",
	"q1svc_spawnbaseline",

	"q1svc_temp_entity",		// <variable>
	"q1svc_setpause",
	"q1svc_signonnum",
	"q1svc_centerprint",
	"q1svc_killedmonster",
	"q1svc_foundsecret",
	"q1svc_spawnstaticsound",
	"q1svc_intermission",
	"q1svc_finale",

	"q1svc_cdtrack",
	"q1svc_sellscreen",

	"qwsvc_smallkick",
	"qwsvc_bigkick",

	"qwsvc_updateping",
	"qwsvc_updateentertime",

	"qwsvc_updatestatlong",
	"qwsvc_muzzleflash",
	"qwsvc_updateuserinfo",
	"qwsvc_download",
	"qwsvc_playerinfo",
	"qwsvc_nails",
	"svc_choke",
	"qwsvc_modellist",
	"qwsvc_soundlist",
	"qwsvc_packetentities",
	"qwsvc_deltapacketentities",
	"qwsvc_maxspeed",
	"qwsvc_entgravity",

	"qwsvc_setinfo",
	"qwsvc_serverinfo",
	"qwsvc_updatepl",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL"
};

int cl_flagindex;

//=============================================================================

int packet_latency[NET_TIMINGS];

int CL_CalcNet(void)
{
	int a, i;
	qwframe_t* frame;
	int lost;

	for (i = clc.netchan.outgoingSequence - UPDATE_BACKUP_QW + 1
		 ; i <= clc.netchan.outgoingSequence
		 ; i++)
	{
		frame = &cl.qw_frames[i & UPDATE_MASK_QW];
		if (frame->receivedtime == -1)
		{
			packet_latency[i & NET_TIMINGSMASK] = 9999;		// dropped
		}
		else if (frame->receivedtime == -2)
		{
			packet_latency[i & NET_TIMINGSMASK] = 10000;	// choked
		}
		else if (frame->invalid)
		{
			packet_latency[i & NET_TIMINGSMASK] = 9998;		// invalid delta
		}
		else
		{
			packet_latency[i & NET_TIMINGSMASK] = (frame->receivedtime - frame->senttime) * 20;
		}
	}

	lost = 0;
	for (a = 0; a < NET_TIMINGS; a++)
	{
		i = (clc.netchan.outgoingSequence - a) & NET_TIMINGSMASK;
		if (packet_latency[i] == 9999)
		{
			lost++;
		}
	}
	return lost * 100 / NET_TIMINGS;
}

//=============================================================================

static void CL_CalcModelChecksum(const char* ModelName, const char* CVarName)
{
	Array<byte> Buffer;
	if (!FS_ReadFile(ModelName, Buffer))
	{
		throw DropException(va("Couldn't load %s", ModelName));
	}

	unsigned short crc;
	CRC_Init(&crc);
	for (int i = 0; i < Buffer.Num(); i++)
	{
		CRC_ProcessByte(&crc, Buffer[i]);
	}

	char st[40];
	sprintf(st, "%d", (int)crc);
	Info_SetValueForKey(cls.qh_userinfo, CVarName, st, MAX_INFO_STRING_QW, 64, 64, true, false);

	clc.netchan.message.WriteByte(q1clc_stringcmd);
	sprintf(st, "setinfo %s %d", CVarName, (int)crc);
	clc.netchan.message.WriteString2(st);
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
		Con_Printf("Checking models...\n");
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
		if (!CLQW_CheckOrDownloadFile(s))
		{
			return;		// started a download
		}
	}

	CM_LoadMap(cl.qh_model_name[1], true, NULL);
	cl.model_clip[1] = 0;
	R_LoadWorld(cl.qh_model_name[1]);

	for (i = 2; i < MAX_MODELS_Q1; i++)
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
			Con_Printf("\nThe required model file '%s' could not be found or downloaded.\n\n",
				cl.qh_model_name[i]);
			Con_Printf("You may need to download or purchase a %s client "
					   "pack in order to play on this server.\n\n", gamedirfile);
			CL_Disconnect();
			return;
		}
	}

	CL_CalcModelChecksum("progs/player.mdl", pmodel_name);
	CL_CalcModelChecksum("progs/eyes.mdl", emodel_name);

	// all done
	R_NewMap();
	Hunk_Check();		// make sure nothing is hurt

	int CheckSum1;
	int CheckSum2;
	CM_MapChecksums(CheckSum1, CheckSum2);

	// done with modellist, request first of static signon messages
	clc.netchan.message.WriteByte(q1clc_stringcmd);
//	clc.netchan.message.WriteString2(va("prespawn %i 0 %i", cl.servercount, cl.worldmodel->checksum2));
	clc.netchan.message.WriteString2(va(prespawn_name, cl.servercount, CheckSum2));
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
		Con_Printf("Checking sounds...\n");
		clc.downloadNumber = 1;
	}

	clc.downloadType = dl_sound;
	for (
		; cl.qh_sound_name[clc.downloadNumber][0]
		; clc.downloadNumber++)
	{
		s = cl.qh_sound_name[clc.downloadNumber];
		if (!CLQW_CheckOrDownloadFile(va("sound/%s",s)))
		{
			return;		// started a download
		}
	}

	S_BeginRegistration();
	for (i = 1; i < MAX_SOUNDS_Q1; i++)
	{
		if (!cl.qh_sound_name[i][0])
		{
			break;
		}
		cl.sound_precache[i] = S_RegisterSound(cl.qh_sound_name[i]);
	}
	S_EndRegistration();

	// done with sounds, request models now
	Com_Memset(cl.model_draw, 0, sizeof(cl.model_draw));
	clq1_playerindex = -1;
	clq1_spikeindex = -1;
	cl_flagindex = -1;
	clc.netchan.message.WriteByte(q1clc_stringcmd);
//	clc.netchan.message.WriteString2(va("modellist %i 0", cl.servercount));
	clc.netchan.message.WriteString2(va(modellist_name, cl.servercount, 0));
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
	case dl_skin:
		CLQW_SkinNextDownload();
		break;
	case dl_model:
		Model_NextDownload();
		break;
	case dl_sound:
		Sound_NextDownload();
		break;
	case dl_none:
	default:
		Con_DPrintf("Unknown download type.\n");
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
	char name[1024];

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
		Con_Printf("File not found.\n");
		if (clc.download)
		{
			Con_Printf("cls.download shouldn't have been set\n");
			FS_FCloseFile(clc.download);
			clc.download = 0;
		}
		CL_RequestNextDownload();
		return;
	}

	// open the file if not opened yet
	if (!clc.download)
	{
		if (String::NCmp(clc.downloadTempName, "skins/", 6))
		{
			clc.download = FS_FOpenFileWrite(clc.downloadTempName);
		}
		else
		{
			sprintf(name, "qw/%s", clc.downloadTempName);
			clc.download = FS_SV_FOpenFileWrite(name);
		}

		if (!clc.download)
		{
			net_message.readcount += size;
			Con_Printf("Failed to open %s\n", clc.downloadTempName);
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
		Con_Printf(".");
		if (10 * (percent / 10) != cls.downloadpercent)
		{
			cls.downloadpercent = 10 * (percent / 10);
			Con_Printf("%i%%", cls.downloadpercent);
		}
#endif
		clc.downloadPercent = percent;

		clc.netchan.message.WriteByte(q1clc_stringcmd);
		clc.netchan.message.WriteString2("nextdl");
	}
	else
	{
		char oldn[MAX_OSPATH];
		char newn[MAX_OSPATH];

#if 0
		Con_Printf("100%%\n");
#endif

		FS_FCloseFile(clc.download);

		// rename the temp file to it's final name
		if (String::Cmp(clc.downloadTempName, clc.downloadName))
		{
			if (String::NCmp(clc.downloadTempName,"skins/",6))
			{
				FS_Rename(clc.downloadTempName, clc.downloadName);
			}
			else
			{
				sprintf(oldn, "qw/%s", clc.downloadTempName);
				sprintf(newn, "qw/%s", clc.downloadName);
				FS_SV_Rename(oldn, newn);
			}
		}

		clc.download = 0;
		clc.downloadPercent = 0;

		// get another file if needed

		CL_RequestNextDownload();
	}
}

static byte* upload_data;
static int upload_pos;
static int upload_size;

void CL_NextUpload(void)
{
	byte buffer[1024];
	int r;
	int percent;
	int size;

	if (!upload_data)
	{
		return;
	}

	r = upload_size - upload_pos;
	if (r > 768)
	{
		r = 768;
	}
	Com_Memcpy(buffer, upload_data + upload_pos, r);
	clc.netchan.message.WriteByte(qwclc_upload);
	clc.netchan.message.WriteShort(r);

	upload_pos += r;
	size = upload_size;
	if (!size)
	{
		size = 1;
	}
	percent = upload_pos * 100 / size;
	clc.netchan.message.WriteByte(percent);
	clc.netchan.message.WriteData(buffer, r);

	Con_DPrintf("UPLOAD: %6d: %d written\n", upload_pos - r, r);

	if (upload_pos != upload_size)
	{
		return;
	}

	Con_Printf("Upload completed\n");

	free(upload_data);
	upload_data = 0;
	upload_pos = upload_size = 0;
}

void CL_StartUpload(byte* data, int size)
{
	if (cls.state < CA_LOADING)
	{
		return;	// gotta be connected

	}
	// override
	if (upload_data)
	{
		free(upload_data);
	}

	Con_DPrintf("Upload starting of %d...\n", size);

	upload_data = (byte*)malloc(size);
	Com_Memcpy(upload_data, data, size);
	upload_size = size;
	upload_pos = 0;

	CL_NextUpload();
}

qboolean CL_IsUploading(void)
{
	if (upload_data)
	{
		return true;
	}
	return false;
}

void CL_StopUpload(void)
{
	if (upload_data)
	{
		free(upload_data);
	}
	upload_data = NULL;
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
void CL_ParseServerData(void)
{
	char* str;
	qboolean cflag = false;
	extern char gamedirfile[MAX_OSPATH];
	int protover;

	Con_DPrintf("Serverdata packet received.\n");
//
// wipe the clientActive_t struct
//
	CL_ClearState();

// parse protocol version number
// allow 2.2 and 2.29 demos to play
	protover = net_message.ReadLong();
	if (protover != PROTOCOL_VERSION &&
		!(clc.demoplaying && (protover == 26 || protover == 27 || protover == 28)))
	{
		Host_EndGame("Server returned version %i, not %i\nYou probably need to upgrade.\nCheck http://www.quakeworld.net/", protover, PROTOCOL_VERSION);
	}

	cl.servercount = net_message.ReadLong();

	// game directory
	str = const_cast<char*>(net_message.ReadString2());

	if (String::ICmp(gamedirfile, str))
	{
		// save current config
		Host_WriteConfiguration();
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

	// seperate the printfs so the server message can have a color
	Con_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	Con_Printf(S_COLOR_ORANGE "%s" S_COLOR_WHITE "\n", str);

	// ask for the sound list next
	Com_Memset(cl.qh_sound_name, 0, sizeof(cl.qh_sound_name));
	clc.netchan.message.WriteByte(q1clc_stringcmd);
//	clc.netchan.message.WriteString2(va("soundlist %i 0", cl.servercount));
	clc.netchan.message.WriteString2(va(soundlist_name, cl.servercount, 0));

	// now waiting for downloads, etc
	cls.state = CA_LOADING;
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
	int n;

// precache sounds
//	Com_Memset(cl.sound_precache, 0, sizeof(cl.sound_precache));

	numsounds = net_message.ReadByte();

	for (;; )
	{
		str = const_cast<char*>(net_message.ReadString2());
		if (!str[0])
		{
			break;
		}
		numsounds++;
		if (numsounds == MAX_SOUNDS_Q1)
		{
			Host_EndGame("Server sent too many sound_precache");
		}
		String::Cpy(cl.qh_sound_name[numsounds], str);
	}

	n = net_message.ReadByte();

	if (n)
	{
		clc.netchan.message.WriteByte(q1clc_stringcmd);
//		clc.netchan.message.WriteString2(va("soundlist %i %i", cl.servercount, n));
		clc.netchan.message.WriteString2(va(soundlist_name, cl.servercount, n));
		return;
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
	int n;

// precache models and note certain default indexes
	nummodels = net_message.ReadByte();

	for (;; )
	{
		str = const_cast<char*>(net_message.ReadString2());
		if (!str[0])
		{
			break;
		}
		nummodels++;
		if (nummodels == MAX_MODELS_Q1)
		{
			Host_EndGame("Server sent too many model_precache");
		}
		String::Cpy(cl.qh_model_name[nummodels], str);

		if (!String::Cmp(cl.qh_model_name[nummodels],"progs/spike.mdl"))
		{
			clq1_spikeindex = nummodels;
		}
		if (!String::Cmp(cl.qh_model_name[nummodels],"progs/player.mdl"))
		{
			clq1_playerindex = nummodels;
		}
		if (!String::Cmp(cl.qh_model_name[nummodels],"progs/flag.mdl"))
		{
			cl_flagindex = nummodels;
		}
	}

	n = net_message.ReadByte();

	if (n)
	{
		clc.netchan.message.WriteByte(q1clc_stringcmd);
//		clc.netchan.message.WriteString2(va("modellist %i %i", cl.servercount, n));
		clc.netchan.message.WriteString2(va(modellist_name, cl.servercount, n));
		return;
	}

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

	if (channel & SND_VOLUME)
	{
		volume = net_message.ReadByte();
	}
	else
	{
		volume = DEFAULT_SOUND_PACKET_VOLUME;
	}

	if (channel & SND_ATTENUATION)
	{
		attenuation = net_message.ReadByte() / 64.0;
	}
	else
	{
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;
	}

	sound_num = net_message.ReadByte();

	for (i = 0; i < 3; i++)
		pos[i] = net_message.ReadCoord();

	ent = (channel >> 3) & 1023;
	channel &= 7;

	if (ent > MAX_EDICTS_Q1)
	{
		Host_EndGame("CL_ParseStartSoundPacket: ent = %i", ent);
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
	qwframe_t* frame;

// calculate simulated time of message
	cl.qh_parsecount = clc.netchan.incomingAcknowledged;
	frame = &cl.qw_frames[cl.qh_parsecount &  UPDATE_MASK_QW];

	frame->receivedtime = realtime;

// calculate latency
	latency = frame->receivedtime - frame->senttime;

	if (latency < 0 || latency > 1.0)
	{
//		Con_Printf ("Odd latency: %5.2f\n", latency);
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
	if (slot > MAX_CLIENTS_QW)
	{
		Sys_Error("CL_NewTranslation: slot > MAX_CLIENTS_QW");
	}

	CLQ1_TranslatePlayerSkin(slot);
}

/*
==============
CL_UpdateUserinfo
==============
*/
void CL_ProcessUserInfo(int slot, q1player_info_t* player)
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

	CL_NewTranslation(slot);
}

/*
==============
CL_UpdateUserinfo
==============
*/
void CL_UpdateUserinfo(void)
{
	int slot;
	q1player_info_t* player;

	slot = net_message.ReadByte();
	if (slot >= MAX_CLIENTS_QW)
	{
		Host_EndGame("CL_ParseServerMessage: qwsvc_updateuserinfo > MAX_CLIENTS_QW");
	}

	player = &cl.q1_players[slot];
	player->userid = net_message.ReadLong();
	String::NCpy(player->userinfo, net_message.ReadString2(), sizeof(player->userinfo) - 1);

	CL_ProcessUserInfo(slot, player);
}

/*
==============
CL_SetInfo
==============
*/
void CL_SetInfo(void)
{
	int slot;
	q1player_info_t* player;
	char key[MAX_MSGLEN_QW];
	char value[MAX_MSGLEN_QW];

	slot = net_message.ReadByte();
	if (slot >= MAX_CLIENTS_QW)
	{
		Host_EndGame("CL_ParseServerMessage: qwsvc_setinfo > MAX_CLIENTS_QW");
	}

	player = &cl.q1_players[slot];

	String::NCpy(key, net_message.ReadString2(), sizeof(key) - 1);
	key[sizeof(key) - 1] = 0;
	String::NCpy(value, net_message.ReadString2(), sizeof(value) - 1);
	key[sizeof(value) - 1] = 0;

	Con_DPrintf("SETINFO %s: %s=%s\n", player->name, key, value);

	if (key[0] != '*')
	{
		Info_SetValueForKey(player->userinfo, key, value, MAX_INFO_STRING_QW, 64, 64,
			String::ICmp(key, "name") != 0, String::ICmp(key, "team") == 0);
	}

	CL_ProcessUserInfo(slot, player);
}

/*
==============
CL_ServerInfo
==============
*/
void CL_ServerInfo(void)
{
	char key[MAX_MSGLEN_QW];
	char value[MAX_MSGLEN_QW];

	String::NCpy(key, net_message.ReadString2(), sizeof(key) - 1);
	key[sizeof(key) - 1] = 0;
	String::NCpy(value, net_message.ReadString2(), sizeof(value) - 1);
	key[sizeof(value) - 1] = 0;

	Con_DPrintf("SERVERINFO: %s=%s\n", key, value);

	if (key[0] != '*')
	{
		Info_SetValueForKey(cl.qh_serverinfo, key, value, MAX_SERVERINFO_STRING, 64, 64,
			String::ICmp(key, "name") != 0, String::ICmp(key, "team") == 0);
	}
}

/*
=====================
CL_SetStat
=====================
*/
void CL_SetStat(int stat, int value)
{
	int j;
	if (stat < 0 || stat >= MAX_CL_STATS)
	{
		Sys_Error("CL_SetStat: %i is invalid", stat);
	}

	if (stat == STAT_ITEMS)
	{	// set flash times
		for (j = 0; j < 32; j++)
			if ((value & (1 << j)) && !(cl.qh_stats[stat] & (1 << j)))
			{
				cl.q1_item_gettime[j] = cl.qh_serverTimeFloat;
			}
	}

	cl.qh_stats[stat] = value;
}

/*
==============
CL_MuzzleFlash
==============
*/
void CL_MuzzleFlash(void)
{
	int i = net_message.ReadShort();

	if ((unsigned)(i - 1) >= MAX_CLIENTS_QW)
	{
		return;
	}
	qwplayer_state_t* pl = &cl.qw_frames[cl.qh_parsecount &  UPDATE_MASK_QW].playerstate[i - 1];
	CLQ1_MuzzleFlashLight(i, pl->origin, pl->viewangles);
}

static void CL_ParsePrint()
{
	int i = net_message.ReadByte();
	const char* txt = net_message.ReadString2();

	if (i == PRINT_CHAT)
	{
		S_StartLocalSound("misc/talk.wav");
		Con_Printf(S_COLOR_ORANGE "%s" S_COLOR_WHITE, txt);
	}
	else
	{
		Con_Printf("%s", txt);
	}
}

#define SHOWNET(x) if (cl_shownet->value == 2) {Con_Printf("%3i:%s\n", net_message.readcount - 1, x); }
/*
=====================
CL_ParseServerMessage
=====================
*/
int received_framecount;
void CL_ParseServerMessage(void)
{
	int cmd;
	char* s;
	int i, j;

	received_framecount = host_framecount;
	CLQ1_ClearProjectiles();

//
// if recording demos, copy the message out
//
	if (cl_shownet->value == 1)
	{
		Con_Printf("%i ",net_message.cursize);
	}
	else if (cl_shownet->value == 2)
	{
		Con_Printf("------------------\n");
	}


	CL_ParseClientdata();

//
// parse the message
//
	while (1)
	{
		if (net_message.badread)
		{
			Host_EndGame("CL_ParseServerMessage: Bad server message");
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
			Host_EndGame("CL_ParseServerMessage: Illegible server message");
			break;

		case q1svc_nop:
//			Con_Printf ("q1svc_nop\n");
			break;

		case q1svc_disconnect:
			if (cls.state == CA_CONNECTED)
			{
				Host_EndGame("Server disconnected\n"
							 "Server version may not be compatible");
			}
			else
			{
				Host_EndGame("Server disconnected");
			}
			break;

		case q1svc_print:
			CL_ParsePrint();
			break;

		case q1svc_centerprint:
			SCR_CenterPrint(const_cast<char*>(net_message.ReadString2()));
			break;

		case q1svc_stufftext:
			s = const_cast<char*>(net_message.ReadString2());
			Con_DPrintf("stufftext: %s\n", s);
			Cbuf_AddText(s);
			break;

		case q1svc_damage:
			V_ParseDamage();
			break;

		case qwsvc_serverdata:
			Cbuf_Execute();			// make sure any stuffed commands are done
			CL_ParseServerData();
			break;

		case q1svc_setangle:
			for (i = 0; i < 3; i++)
				cl.viewangles[i] = net_message.ReadAngle();
//			cl.viewangles[PITCH] = cl.viewangles[ROLL] = 0;
			break;

		case q1svc_lightstyle:
			i = net_message.ReadByte();
			CL_SetLightStyle(i, net_message.ReadString2());
			break;

		case q1svc_sound:
			CL_ParseStartSoundPacket();
			break;

		case q1svc_stopsound:
			i = net_message.ReadShort();
			S_StopSound(i >> 3, i & 7);
			break;

		case q1svc_updatefrags:
			i = net_message.ReadByte();
			if (i >= MAX_CLIENTS_QW)
			{
				Host_EndGame("CL_ParseServerMessage: q1svc_updatefrags > MAX_CLIENTS_QW");
			}
			cl.q1_players[i].frags = net_message.ReadShort();
			break;

		case qwsvc_updateping:
			i = net_message.ReadByte();
			if (i >= MAX_CLIENTS_QW)
			{
				Host_EndGame("CL_ParseServerMessage: qwsvc_updateping > MAX_CLIENTS_QW");
			}
			cl.q1_players[i].ping = net_message.ReadShort();
			break;

		case qwsvc_updatepl:
			i = net_message.ReadByte();
			if (i >= MAX_CLIENTS_QW)
			{
				Host_EndGame("CL_ParseServerMessage: qwsvc_updatepl > MAX_CLIENTS_QW");
			}
			cl.q1_players[i].pl = net_message.ReadByte();
			break;

		case qwsvc_updateentertime:
			// time is sent over as seconds ago
			i = net_message.ReadByte();
			if (i >= MAX_CLIENTS_QW)
			{
				Host_EndGame("CL_ParseServerMessage: qwsvc_updateentertime > MAX_CLIENTS_QW");
			}
			cl.q1_players[i].entertime = realtime - net_message.ReadFloat();
			break;

		case q1svc_spawnbaseline:
			CLQ1_ParseSpawnBaseline(net_message);
			break;
		case q1svc_spawnstatic:
			CLQ1_ParseSpawnStatic(net_message);
			break;
		case q1svc_temp_entity:
			CLQW_ParseTEnt(net_message);
			break;

		case q1svc_killedmonster:
			cl.qh_stats[STAT_MONSTERS]++;
			break;

		case q1svc_foundsecret:
			cl.qh_stats[STAT_SECRETS]++;
			break;

		case q1svc_updatestat:
			i = net_message.ReadByte();
			j = net_message.ReadByte();
			CL_SetStat(i, j);
			break;
		case qwsvc_updatestatlong:
			i = net_message.ReadByte();
			j = net_message.ReadLong();
			CL_SetStat(i, j);
			break;

		case q1svc_spawnstaticsound:
			CL_ParseStaticSound();
			break;

		case q1svc_cdtrack:
		{
			byte cdtrack = net_message.ReadByte();
			CDAudio_Play(cdtrack, true);
		}
		break;

		case q1svc_intermission:
			cl.qh_intermission = 1;
			cl.qh_completed_time = realtime;
			for (i = 0; i < 3; i++)
				cl.qh_simorg[i] = net_message.ReadCoord();
			for (i = 0; i < 3; i++)
				cl.qh_simangles[i] = net_message.ReadAngle();
			VectorCopy(vec3_origin, cl.qh_simvel);
			break;

		case q1svc_finale:
			cl.qh_intermission = 2;
			cl.qh_completed_time = realtime;
			SCR_CenterPrint(const_cast<char*>(net_message.ReadString2()));
			break;

		case q1svc_sellscreen:
			Cmd_ExecuteString("help");
			break;

		case qwsvc_smallkick:
			cl.qh_punchangle = -2;
			break;
		case qwsvc_bigkick:
			cl.qh_punchangle = -4;
			break;

		case qwsvc_muzzleflash:
			CL_MuzzleFlash();
			break;

		case qwsvc_updateuserinfo:
			CL_UpdateUserinfo();
			break;

		case qwsvc_setinfo:
			CL_SetInfo();
			break;

		case qwsvc_serverinfo:
			CL_ServerInfo();
			break;

		case qwsvc_download:
			CL_ParseDownload();
			break;

		case qwsvc_playerinfo:
			CLQW_ParsePlayerinfo(net_message);
			break;

		case qwsvc_nails:
			CLQW_ParseNails(net_message);
			break;

		case qwsvc_chokecount:		// some preceding packets were choked
			i = net_message.ReadByte();
			for (j = 0; j < i; j++)
				cl.qw_frames[(clc.netchan.incomingAcknowledged - 1 - j) & UPDATE_MASK_QW].receivedtime = -2;
			break;

		case qwsvc_modellist:
			CL_ParseModellist();
			break;

		case qwsvc_soundlist:
			CL_ParseSoundlist();
			break;

		case qwsvc_packetentities:
			CLQW_ParsePacketEntities(net_message);
			break;

		case qwsvc_deltapacketentities:
			CLQW_ParseDeltaPacketEntities(net_message);
			break;

		case qwsvc_maxspeed:
			movevars.maxspeed = net_message.ReadFloat();
			break;

		case qwsvc_entgravity:
			movevars.entgravity = net_message.ReadFloat();
			break;

		case q1svc_setpause:
			cl.qh_paused = net_message.ReadByte();
			if (cl.qh_paused)
			{
				CDAudio_Pause();
			}
			else
			{
				CDAudio_Resume();
			}
			break;

		}
	}

	CL_SetSolidEntities();
}
