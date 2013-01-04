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
#include "../../cinematic/public.h"
#include "../parse.h"
#include "../light_styles.h"

const char* svcq2_strings[256] =
{
	"q2svc_bad",

	"q2svc_muzzleflash",
	"svc_muzzlflash2",
	"q2svc_temp_entity",
	"q2svc_layout",
	"q2svc_inventory",

	"q2svc_nop",
	"q2svc_disconnect",
	"q2svc_reconnect",
	"q2svc_sound",
	"q2svc_print",
	"q2svc_stufftext",
	"q2svc_serverdata",
	"q2svc_configstring",
	"q2svc_spawnbaseline",
	"q2svc_centerprint",
	"q2svc_download",
	"q2svc_playerinfo",
	"q2svc_packetentities",
	"q2svc_deltapacketentities",
	"q2svc_frame"
};

static void CLQ2_ParseDisconnect()
{
	common->ServerDisconnected("Server disconnected\n");
}

static void CLQ2_ParseReconnect()
{
	common->Printf("Server disconnected, reconnecting\n");
	if (clc.download)
	{
		//ZOID, close download
		FS_FCloseFile(clc.download);
		clc.download = 0;
	}
	cls.state = CA_CONNECTING;
	cls.q2_connect_time = -99999;	// CLQ2_CheckForResend() will fire immediately
}

static void CLQ2_ParsePrint(QMsg& message)
{
	int i = message.ReadByte();
	const char* txt = message.ReadString2();

	if (i == PRINT_CHAT)
	{
		S_StartLocalSound("misc/talk.wav");
		common->Printf(S_COLOR_GREEN "%s" S_COLOR_WHITE, txt);
	}
	else
	{
		common->Printf("%s", txt);
	}
}

static void CLQ2_ParseServerData(QMsg& message)
{
	common->DPrintf("Serverdata packet received.\n");

	Cbuf_Execute();			// make sure any stuffed commands are done

	//
	// wipe the clientActive_t struct
	//
	CL_ClearState();
	cls.state = CA_CONNECTED;

	// parse protocol version number
	int i = message.ReadLong();
	cls.q2_serverProtocol = i;

	// BIG HACK to let demos from release work with the 3.0x patch!!!
	if (ComQ2_ServerState() && Q2PROTOCOL_VERSION == 34)
	{
	}
	else if (i != Q2PROTOCOL_VERSION)
	{
		common->Error("Server returned version %i, not %i", i, Q2PROTOCOL_VERSION);
	}

	cl.servercount = message.ReadLong();
	cl.q2_attractloop = message.ReadByte();

	// game directory
	const char* str = message.ReadString2();
	String::NCpy(cl.q2_gamedir, str, sizeof(cl.q2_gamedir) - 1);

	// set gamedir
	if ((*str && (!fs_gamedirvar->string || !*fs_gamedirvar->string || String::Cmp(fs_gamedirvar->string, str))) || (!*str && (fs_gamedirvar->string || *fs_gamedirvar->string)))
	{
		Cvar_SetLatched("game", str);
	}

	// parse player entity number
	cl.playernum = message.ReadShort();
	cl.viewentity = cl.playernum + 1;

	// get the full level name
	str = const_cast<char*>(message.ReadString2());

	if (cl.playernum == -1)
	{	// playing a cinematic or showing a pic, not a level
		SCR_PlayCinematic(str);
	}
	else
	{
		// seperate the printfs so the server message can have a color
		common->Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
		common->Printf(S_COLOR_GREEN "%s" S_COLOR_WHITE "\n", str);

		// need to prep refresh at next oportunity
		cl.q2_refresh_prepped = false;
	}
}

void CLQ2_LoadClientinfo(q2clientinfo_t* ci, const char* s)
{
	String::NCpy(ci->cinfo, s, sizeof(ci->cinfo));
	ci->cinfo[sizeof(ci->cinfo) - 1] = 0;

	// isolate the player's name
	String::NCpy(ci->name, s, sizeof(ci->name));
	ci->name[sizeof(ci->name) - 1] = 0;
	const char* t1 = strstr(s, "\\");
	if (t1)
	{
		ci->name[t1 - s] = 0;
		s = t1 + 1;
	}

	if (clq2_noskins->value || *s == 0)
	{
		char model_filename[MAX_QPATH];
		String::Sprintf(model_filename, sizeof(model_filename), "players/male/tris.md2");
		char weapon_filename[MAX_QPATH];
		String::Sprintf(weapon_filename, sizeof(weapon_filename), "players/male/weapon.md2");
		char skin_filename[MAX_QPATH];
		String::Sprintf(skin_filename, sizeof(skin_filename), "players/male/grunt.pcx");
		String::Sprintf(ci->iconname, sizeof(ci->iconname), "/players/male/grunt_i.pcx");
		ci->model = R_RegisterModel(model_filename);
		Com_Memset(ci->weaponmodel, 0, sizeof(ci->weaponmodel));
		ci->weaponmodel[0] = R_RegisterModel(weapon_filename);
		ci->skin = R_RegisterSkinQ2(skin_filename);
		ci->icon = R_RegisterPic(ci->iconname);
	}
	else
	{
		// isolate the model name
		char model_name[MAX_QPATH];
		String::Cpy(model_name, s);
		char* t = strstr(model_name, "/");
		if (!t)
		{
			t = strstr(model_name, "\\");
		}
		if (!t)
		{
			t = model_name;
		}
		*t = 0;

		// isolate the skin name
		char skin_name[MAX_QPATH];
		String::Cpy(skin_name, s + String::Length(model_name) + 1);

		// model file
		char model_filename[MAX_QPATH];
		String::Sprintf(model_filename, sizeof(model_filename), "players/%s/tris.md2", model_name);
		ci->model = R_RegisterModel(model_filename);
		if (!ci->model)
		{
			String::Cpy(model_name, "male");
			String::Sprintf(model_filename, sizeof(model_filename), "players/male/tris.md2");
			ci->model = R_RegisterModel(model_filename);
		}

		// skin file
		char skin_filename[MAX_QPATH];
		String::Sprintf(skin_filename, sizeof(skin_filename), "players/%s/%s.pcx", model_name, skin_name);
		ci->skin = R_RegisterSkinQ2(skin_filename);

		// if we don't have the skin and the model wasn't male,
		// see if the male has it (this is for CTF's skins)
		if (!ci->skin && String::ICmp(model_name, "male"))
		{
			// change model to male
			String::Cpy(model_name, "male");
			String::Sprintf(model_filename, sizeof(model_filename), "players/male/tris.md2");
			ci->model = R_RegisterModel(model_filename);

			// see if the skin exists for the male model
			String::Sprintf(skin_filename, sizeof(skin_filename), "players/%s/%s.pcx", model_name, skin_name);
			ci->skin = R_RegisterSkinQ2(skin_filename);
		}

		// if we still don't have a skin, it means that the male model didn't have
		// it, so default to grunt
		if (!ci->skin)
		{
			// see if the skin exists for the male model
			String::Sprintf(skin_filename, sizeof(skin_filename), "players/%s/grunt.pcx", model_name, skin_name);
			ci->skin = R_RegisterSkinQ2(skin_filename);
		}

		// weapon file
		for (int i = 0; i < clq2_num_weaponmodels; i++)
		{
			char weapon_filename[MAX_QPATH];
			String::Sprintf(weapon_filename, sizeof(weapon_filename), "players/%s/%s", model_name, clq2_weaponmodels[i]);
			ci->weaponmodel[i] = R_RegisterModel(weapon_filename);
			if (!ci->weaponmodel[i] && String::Cmp(model_name, "cyborg") == 0)
			{
				// try male
				String::Sprintf(weapon_filename, sizeof(weapon_filename), "players/male/%s", clq2_weaponmodels[i]);
				ci->weaponmodel[i] = R_RegisterModel(weapon_filename);
			}
			if (!clq2_vwep->value)
			{
				break;	// only one when vwep is off
			}
		}

		// icon file
		String::Sprintf(ci->iconname, sizeof(ci->iconname), "/players/%s/%s_i.pcx", model_name, skin_name);
		ci->icon = R_RegisterPic(ci->iconname);
	}

	// must have loaded all data types to be valud
	if (!ci->skin || !ci->icon || !ci->model || !ci->weaponmodel[0])
	{
		ci->skin = NULL;
		ci->icon = NULL;
		ci->model = 0;
		ci->weaponmodel[0] = 0;
		return;
	}
}

//	Load the skin, icon, and model for a client
void CLQ2_ParseClientinfo(int player)
{
	const char* s = cl.q2_configstrings[player + Q2CS_PLAYERSKINS];

	q2clientinfo_t* ci = &cl.q2_clientinfo[player];

	CLQ2_LoadClientinfo(ci, s);
}

static void CLQ2_ParseConfigString(QMsg& message)
{
	int i = message.ReadShort();
	if (i < 0 || i >= MAX_CONFIGSTRINGS_Q2)
	{
		common->Error("configstring > MAX_CONFIGSTRINGS_Q2");
	}
	const char* s = message.ReadString2();
	String::Cpy(cl.q2_configstrings[i], s);

	// do something apropriate

	if (i >= Q2CS_LIGHTS && i < Q2CS_LIGHTS + MAX_LIGHTSTYLES_Q2)
	{
		CL_SetLightStyle(i - Q2CS_LIGHTS, cl.q2_configstrings[i]);
	}
	else if (i == Q2CS_CDTRACK)
	{
		if (cl.q2_refresh_prepped)
		{
			CDAudio_Play(String::Atoi(cl.q2_configstrings[Q2CS_CDTRACK]), true);
		}
	}
	else if (i >= Q2CS_MODELS && i < Q2CS_MODELS + MAX_MODELS_Q2)
	{
		if (cl.q2_refresh_prepped)
		{
			cl.model_draw[i - Q2CS_MODELS] = R_RegisterModel(cl.q2_configstrings[i]);
			if (cl.q2_configstrings[i][0] == '*')
			{
				cl.model_clip[i - Q2CS_MODELS] = CM_InlineModel(String::Atoi(cl.q2_configstrings[i] + 1));
			}
			else
			{
				cl.model_clip[i - Q2CS_MODELS] = 0;
			}
		}
	}
	else if (i >= Q2CS_SOUNDS && i < Q2CS_SOUNDS + MAX_MODELS_Q2)
	{
		if (cl.q2_refresh_prepped)
		{
			cl.sound_precache[i - Q2CS_SOUNDS] = S_RegisterSound(cl.q2_configstrings[i]);
		}
	}
	else if (i >= Q2CS_IMAGES && i < Q2CS_IMAGES + MAX_MODELS_Q2)
	{
		if (cl.q2_refresh_prepped)
		{
			cl.q2_image_precache[i - Q2CS_IMAGES] = R_RegisterPic(cl.q2_configstrings[i]);
		}
	}
	else if (i >= Q2CS_PLAYERSKINS && i < Q2CS_PLAYERSKINS + MAX_CLIENTS_Q2)
	{
		if (cl.q2_refresh_prepped)
		{
			CLQ2_ParseClientinfo(i - Q2CS_PLAYERSKINS);
		}
	}
}

static void CLQ2_ParseStartSoundPacket(QMsg& message)
{
	int flags = message.ReadByte();
	int sound_num = message.ReadByte();

	float volume;
	if (flags & Q2SND_VOLUME)
	{
		volume = message.ReadByte() / 255.0;
	}
	else
	{
		volume = Q2DEFAULT_SOUND_PACKET_VOLUME;
	}

	float attenuation;
	if (flags & Q2SND_ATTENUATION)
	{
		attenuation = message.ReadByte() / 64.0;
	}
	else
	{
		attenuation = QHDEFAULT_SOUND_PACKET_ATTENUATION;
	}

	float ofs;
	if (flags & Q2SND_OFFSET)
	{
		ofs = message.ReadByte() / 1000.0;
	}
	else
	{
		ofs = 0;
	}

	int channel, ent;
	if (flags & Q2SND_ENT)
	{
		// entity reletive
		channel = message.ReadShort();
		ent = channel >> 3;
		if (ent > MAX_EDICTS_Q2)
		{
			common->Error("CLQ2_ParseStartSoundPacket: ent = %i", ent);
		}

		channel &= 7;
	}
	else
	{
		ent = 0;
		channel = 0;
	}

	vec3_t pos_v;
	float* pos;
	if (flags & Q2SND_POS)
	{
		// positioned in space
		message.ReadPos(pos_v);

		pos = pos_v;
	}
	else	// use entity number
	{
		pos = NULL;
	}

	if (!cl.sound_precache[sound_num])
	{
		return;
	}

	S_StartSound(pos, ent, channel, cl.sound_precache[sound_num], volume, attenuation, ofs);
}

static void CLQ2_DownloadFileName(char* dest, int destlen, char* fn)
{
	if (String::NCmp(fn, "players", 7) == 0)
	{
		String::Sprintf(dest, destlen, "%s/%s", fs_PrimaryBaseGame, fn);
	}
	else
	{
		String::Sprintf(dest, destlen, "%s/%s", FS_Gamedir(), fn);
	}
}

static void CLQ2_ParseDownload(QMsg& message)
{
	// read the data
	int size = message.ReadShort();
	int percent = message.ReadByte();
	if (size == -1)
	{
		common->Printf("Server does not have this file.\n");
		if (clc.download)
		{
			// if here, we tried to resume a file but the server said no
			FS_FCloseFile(clc.download);
			clc.download = 0;
		}
		CLQ2_RequestNextDownload();
		return;
	}

	// open the file if not opened yet
	if (!clc.download)
	{
		char name[MAX_OSPATH];
		CLQ2_DownloadFileName(name, sizeof(name), clc.downloadTempName);

		clc.download = FS_SV_FOpenFileWrite(name);
		if (!clc.download)
		{
			message.readcount += size;
			common->Printf("Failed to open %s\n", clc.downloadTempName);
			CLQ2_RequestNextDownload();
			return;
		}
	}

	FS_Write(message._data + message.readcount, size, clc.download);
	message.readcount += size;

	if (percent != 100)
	{
		// request next block
		clc.downloadPercent = percent;

		CL_AddReliableCommand("nextdl");
	}
	else
	{
		FS_FCloseFile(clc.download);

		// rename the temp file to it's final name
		char oldn[MAX_OSPATH];
		CLQ2_DownloadFileName(oldn, sizeof(oldn), clc.downloadTempName);
		char newn[MAX_OSPATH];
		CLQ2_DownloadFileName(newn, sizeof(newn), clc.downloadName);
		int r = rename(oldn, newn);
		if (r)
		{
			common->Printf("failed to rename.\n");
		}

		clc.download = 0;
		clc.downloadPercent = 0;

		// get another file if needed

		CLQ2_RequestNextDownload();
	}
}

static void CLQ2_ParseLayout(QMsg& message)
{
	const char* s = message.ReadString2();
	String::NCpy(cl.q2_layout, s, sizeof(cl.q2_layout) - 1);
}

void CLQ2_ParseServerMessage(QMsg& message)
{
	if (cl_shownet->value == 1)
	{
		common->Printf("%i ", message.cursize);
	}
	else if (cl_shownet->value >= 2)
	{
		common->Printf("------------------\n");
	}

	//
	// parse the message
	//
	while (1)
	{
		if (message.readcount > message.cursize)
		{
			common->Error("CLQ2_ParseServerMessage: Bad server message");
			break;
		}

		int cmd = message.ReadByte();

		if (cmd == -1)
		{
			SHOWNET(message, "END OF MESSAGE");
			break;
		}

		if (cl_shownet->value >= 2)
		{
			if (!svcq2_strings[cmd])
			{
				common->Printf("%3i:BAD CMD %i\n", message.readcount - 1,cmd);
			}
			else
			{
				SHOWNET(message, svcq2_strings[cmd]);
			}
		}

		// other commands
		switch (cmd)
		{
		default:
			common->Error("CLQ2_ParseServerMessage: Illegible server message\n");
			break;
		case q2svc_nop:
			break;
		case q2svc_disconnect:
			CLQ2_ParseDisconnect();
			break;
		case q2svc_reconnect:
			CLQ2_ParseReconnect();
			break;
		case q2svc_print:
			CLQ2_ParsePrint(message);
			break;
		case q2svc_centerprint:
			CL_ParseCenterPrint(message);
			break;
		case q2svc_stufftext:
			CL_ParseStuffText(message);
			break;
		case q2svc_serverdata:
			CLQ2_ParseServerData(message);
			break;
		case q2svc_configstring:
			CLQ2_ParseConfigString(message);
			break;
		case q2svc_sound:
			CLQ2_ParseStartSoundPacket(message);
			break;
		case q2svc_spawnbaseline:
			CLQ2_ParseBaseline(message);
			break;
		case q2svc_temp_entity:
			CLQ2_ParseTEnt(message);
			break;
		case q2svc_muzzleflash:
			CLQ2_ParseMuzzleFlash(message);
			break;
		case q2svc_muzzleflash2:
			CLQ2_ParseMuzzleFlash2(message);
			break;
		case q2svc_download:
			CLQ2_ParseDownload(message);
			break;
		case q2svc_frame:
			CLQ2_ParseFrame(message);
			break;
		case q2svc_inventory:
			CLQ2_ParseInventory(message);
			break;
		case q2svc_layout:
			CLQ2_ParseLayout(message);
			break;
		case q2svc_playerinfo:
		case q2svc_packetentities:
		case q2svc_deltapacketentities:
			common->Error("Out of place frame data");
			break;
		}
	}
}
