/*
Copyright (C) 1997-2001 Id Software, Inc.

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

#include "client.h"

const char* svc_strings[256] =
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

//=============================================================================

void CL_DownloadFileName(char* dest, int destlen, char* fn)
{
	if (String::NCmp(fn, "players", 7) == 0)
	{
		String::Sprintf(dest, destlen, "%s/%s", BASEDIRNAME, fn);
	}
	else
	{
		String::Sprintf(dest, destlen, "%s/%s", FS_Gamedir(), fn);
	}
}

/*
===============
CL_CheckOrDownloadFile

Returns true if the file exists, otherwise it attempts
to start a download from the server.
===============
*/
qboolean    CL_CheckOrDownloadFile(char* filename)
{
	if (strstr(filename, ".."))
	{
		Com_Printf("Refusing to download a path with ..\n");
		return true;
	}

	if (FS_ReadFile(filename, NULL) != -1)
	{	// it exists, no need to download
		return true;
	}

	String::Cpy(clc.downloadName, filename);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
	String::StripExtension(clc.downloadName, clc.downloadTempName);
	String::Cat(clc.downloadTempName, sizeof(clc.downloadTempName), ".tmp");

	Com_Printf("Downloading %s\n", clc.downloadName);
	clc.netchan.message.WriteByte(q2clc_stringcmd);
	clc.netchan.message.WriteString2(
		va("download %s", clc.downloadName));

	clc.downloadNumber++;

	return false;
}

/*
===============
CL_Download_f

Request a download from the server
===============
*/
void    CL_Download_f(void)
{
	char filename[MAX_OSPATH];

	if (Cmd_Argc() != 2)
	{
		Com_Printf("Usage: download <filename>\n");
		return;
	}

	String::Sprintf(filename, sizeof(filename), "%s", Cmd_Argv(1));

	if (strstr(filename, ".."))
	{
		Com_Printf("Refusing to download a path with ..\n");
		return;
	}

	if (FS_ReadFile(filename, NULL) != -1)
	{	// it exists, no need to download
		Com_Printf("File already exists.\n");
		return;
	}

	String::Cpy(clc.downloadName, filename);
	Com_Printf("Downloading %s\n", clc.downloadName);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
	String::StripExtension(clc.downloadName, clc.downloadTempName);
	String::Cat(clc.downloadTempName, sizeof(clc.downloadTempName), ".tmp");

	clc.netchan.message.WriteByte(q2clc_stringcmd);
	clc.netchan.message.WriteString2(
		va("download %s", clc.downloadName));

	clc.downloadNumber++;
}

/*
======================
CL_RegisterSounds
======================
*/
void CL_RegisterSounds(void)
{
	int i;

	S_BeginRegistration();
	CLQ2_RegisterTEntSounds();
	for (i = 1; i < MAX_SOUNDS_Q2; i++)
	{
		if (!cl.q2_configstrings[Q2CS_SOUNDS + i][0])
		{
			break;
		}
		cl.sound_precache[i] = S_RegisterSound(cl.q2_configstrings[Q2CS_SOUNDS + i]);
		Sys_SendKeyEvents();	// pump message loop
		IN_ProcessEvents();
	}
	S_EndRegistration();
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
	char name[MAX_OSPATH];
	int r;

	// read the data
	size = net_message.ReadShort();
	percent = net_message.ReadByte();
	if (size == -1)
	{
		Com_Printf("Server does not have this file.\n");
		if (clc.download)
		{
			// if here, we tried to resume a file but the server said no
			FS_FCloseFile(clc.download);
			clc.download = 0;
		}
		CL_RequestNextDownload();
		return;
	}

	// open the file if not opened yet
	if (!clc.download)
	{
		CL_DownloadFileName(name, sizeof(name), clc.downloadTempName);

		clc.download = FS_SV_FOpenFileWrite(name);
		if (!clc.download)
		{
			net_message.readcount += size;
			Com_Printf("Failed to open %s\n", clc.downloadTempName);
			CL_RequestNextDownload();
			return;
		}
	}

	FS_Write(net_message._data + net_message.readcount, size, clc.download);
	net_message.readcount += size;

	if (percent != 100)
	{
		// request next block
// change display routines by zoid
#if 0
		Com_Printf(".");
		if (10 * (percent / 10) != cls.downloadpercent)
		{
			cls.downloadpercent = 10 * (percent / 10);
			Com_Printf("%i%%", cls.downloadpercent);
		}
#endif
		clc.downloadPercent = percent;

		clc.netchan.message.WriteByte(q2clc_stringcmd);
		clc.netchan.message.WriteString2("nextdl");
	}
	else
	{
		char oldn[MAX_OSPATH];
		char newn[MAX_OSPATH];

//		Com_Printf ("100%%\n");

		FS_FCloseFile(clc.download);

		// rename the temp file to it's final name
		CL_DownloadFileName(oldn, sizeof(oldn), clc.downloadTempName);
		CL_DownloadFileName(newn, sizeof(newn), clc.downloadName);
		r = rename(oldn, newn);
		if (r)
		{
			Com_Printf("failed to rename.\n");
		}

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
void CL_ParseServerData(void)
{
	extern Cvar* fs_gamedirvar;
	char* str;
	int i;

	Com_DPrintf("Serverdata packet received.\n");
//
// wipe the clientActive_t struct
//
	CL_ClearState();
	cls.state = CA_CONNECTED;

// parse protocol version number
	i = net_message.ReadLong();
	cls.q2_serverProtocol = i;

	// BIG HACK to let demos from release work with the 3.0x patch!!!
	if (Com_ServerState() && PROTOCOL_VERSION == 34)
	{
	}
	else if (i != PROTOCOL_VERSION)
	{
		Com_Error(ERR_DROP,"Server returned version %i, not %i", i, PROTOCOL_VERSION);
	}

	cl.servercount = net_message.ReadLong();
	cl.q2_attractloop = net_message.ReadByte();

	// game directory
	str = const_cast<char*>(net_message.ReadString2());
	String::NCpy(cl.q2_gamedir, str, sizeof(cl.q2_gamedir) - 1);

	// set gamedir
	if ((*str && (!fs_gamedirvar->string || !*fs_gamedirvar->string || String::Cmp(fs_gamedirvar->string, str))) || (!*str && (fs_gamedirvar->string || *fs_gamedirvar->string)))
	{
		Cvar_SetLatched("game", str);
	}

	// parse player entity number
	cl.playernum = net_message.ReadShort();
	cl.viewentity = cl.playernum + 1;

	// get the full level name
	str = const_cast<char*>(net_message.ReadString2());

	if (cl.playernum == -1)
	{	// playing a cinematic or showing a pic, not a level
		SCR_PlayCinematic(str);
	}
	else
	{
		// seperate the printfs so the server message can have a color
		Com_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
		Com_Printf(S_COLOR_GREEN "%s" S_COLOR_WHITE "\n", str);

		// need to prep refresh at next oportunity
		cl.q2_refresh_prepped = false;
	}
}

/*
==================
CL_ParseBaseline
==================
*/
void CL_ParseBaseline(void)
{
	q2entity_state_t* es;
	unsigned int bits;
	int newnum;
	q2entity_state_t nullstate;

	Com_Memset(&nullstate, 0, sizeof(nullstate));

	newnum = CL_ParseEntityBits(&bits);
	es = &clq2_entities[newnum].baseline;
	CL_ParseDelta(&nullstate, es, newnum, bits);
}


/*
================
CL_LoadClientinfo

================
*/
void CL_LoadClientinfo(q2clientinfo_t* ci, const char* s)
{
	int i;
	char model_name[MAX_QPATH];
	char skin_name[MAX_QPATH];
	char model_filename[MAX_QPATH];
	char skin_filename[MAX_QPATH];
	char weapon_filename[MAX_QPATH];

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

	if (cl_noskins->value || *s == 0)
	{
		String::Sprintf(model_filename, sizeof(model_filename), "players/male/tris.md2");
		String::Sprintf(weapon_filename, sizeof(weapon_filename), "players/male/weapon.md2");
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
		String::Cpy(skin_name, s + String::Length(model_name) + 1);

		// model file
		String::Sprintf(model_filename, sizeof(model_filename), "players/%s/tris.md2", model_name);
		ci->model = R_RegisterModel(model_filename);
		if (!ci->model)
		{
			String::Cpy(model_name, "male");
			String::Sprintf(model_filename, sizeof(model_filename), "players/male/tris.md2");
			ci->model = R_RegisterModel(model_filename);
		}

		// skin file
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
		for (i = 0; i < num_cl_weaponmodels; i++)
		{
			String::Sprintf(weapon_filename, sizeof(weapon_filename), "players/%s/%s", model_name, cl_weaponmodels[i]);
			ci->weaponmodel[i] = R_RegisterModel(weapon_filename);
			if (!ci->weaponmodel[i] && String::Cmp(model_name, "cyborg") == 0)
			{
				// try male
				String::Sprintf(weapon_filename, sizeof(weapon_filename), "players/male/%s", cl_weaponmodels[i]);
				ci->weaponmodel[i] = R_RegisterModel(weapon_filename);
			}
			if (!cl_vwep->value)
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

/*
================
CL_ParseClientinfo

Load the skin, icon, and model for a client
================
*/
void CL_ParseClientinfo(int player)
{
	char* s;
	q2clientinfo_t* ci;

	s = cl.q2_configstrings[player + Q2CS_PLAYERSKINS];

	ci = &cl.q2_clientinfo[player];

	CL_LoadClientinfo(ci, s);
}


/*
================
CL_ParseConfigString
================
*/
void CL_ParseConfigString(void)
{
	int i;
	char* s;

	i = net_message.ReadShort();
	if (i < 0 || i >= MAX_CONFIGSTRINGS_Q2)
	{
		Com_Error(ERR_DROP, "configstring > MAX_CONFIGSTRINGS_Q2");
	}
	s = const_cast<char*>(net_message.ReadString2());
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
			CL_ParseClientinfo(i - Q2CS_PLAYERSKINS);
		}
	}
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
	vec3_t pos_v;
	float* pos;
	int channel, ent;
	int sound_num;
	float volume;
	float attenuation;
	int flags;
	float ofs;

	flags = net_message.ReadByte();
	sound_num = net_message.ReadByte();

	if (flags & SND_VOLUME)
	{
		volume = net_message.ReadByte() / 255.0;
	}
	else
	{
		volume = DEFAULT_SOUND_PACKET_VOLUME;
	}

	if (flags & SND_ATTENUATION)
	{
		attenuation = net_message.ReadByte() / 64.0;
	}
	else
	{
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;
	}

	if (flags & SND_OFFSET)
	{
		ofs = net_message.ReadByte() / 1000.0;
	}
	else
	{
		ofs = 0;
	}

	if (flags & SND_ENT)
	{	// entity reletive
		channel = net_message.ReadShort();
		ent = channel >> 3;
		if (ent > MAX_EDICTS_Q2)
		{
			Com_Error(ERR_DROP,"CL_ParseStartSoundPacket: ent = %i", ent);
		}

		channel &= 7;
	}
	else
	{
		ent = 0;
		channel = 0;
	}

	if (flags & SND_POS)
	{	// positioned in space
		net_message.ReadPos(pos_v);

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


void SHOWNET(const char* s)
{
	if (cl_shownet->value >= 2)
	{
		Com_Printf("%3i:%s\n", net_message.readcount - 1, s);
	}
}

/*
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage(void)
{
	int cmd;
	char* s;
	int i;

//
// if recording demos, copy the message out
//
	if (cl_shownet->value == 1)
	{
		Com_Printf("%i ",net_message.cursize);
	}
	else if (cl_shownet->value >= 2)
	{
		Com_Printf("------------------\n");
	}


//
// parse the message
//
	while (1)
	{
		if (net_message.readcount > net_message.cursize)
		{
			Com_Error(ERR_DROP,"CL_ParseServerMessage: Bad server message");
			break;
		}

		cmd = net_message.ReadByte();

		if (cmd == -1)
		{
			SHOWNET("END OF MESSAGE");
			break;
		}

		if (cl_shownet->value >= 2)
		{
			if (!svc_strings[cmd])
			{
				Com_Printf("%3i:BAD CMD %i\n", net_message.readcount - 1,cmd);
			}
			else
			{
				SHOWNET(svc_strings[cmd]);
			}
		}

		// other commands
		switch (cmd)
		{
		default:
			Com_Error(ERR_DROP,"CL_ParseServerMessage: Illegible server message\n");
			break;

		case q2svc_nop:
//			Com_Printf ("q2svc_nop\n");
			break;

		case q2svc_disconnect:
			Com_Error(ERR_DISCONNECT,"Server disconnected\n");
			break;

		case q2svc_reconnect:
			Com_Printf("Server disconnected, reconnecting\n");
			if (clc.download)
			{
				//ZOID, close download
				FS_FCloseFile(clc.download);
				clc.download = 0;
			}
			cls.state = CA_CONNECTING;
			cls.q2_connect_time = -99999;	// CL_CheckForResend() will fire immediately
			break;

		case q2svc_print:
			i = net_message.ReadByte();
			if (i == PRINT_CHAT)
			{
				S_StartLocalSound("misc/talk.wav");
				con.ormask = 128;
			}
			Com_Printf("%s", net_message.ReadString2());
			con.ormask = 0;
			break;

		case q2svc_centerprint:
			SCR_CenterPrint(const_cast<char*>(net_message.ReadString2()));
			break;

		case q2svc_stufftext:
			s = const_cast<char*>(net_message.ReadString2());
			Com_DPrintf("stufftext: %s\n", s);
			Cbuf_AddText(s);
			break;

		case q2svc_serverdata:
			Cbuf_Execute();			// make sure any stuffed commands are done
			CL_ParseServerData();
			break;

		case q2svc_configstring:
			CL_ParseConfigString();
			break;

		case q2svc_sound:
			CL_ParseStartSoundPacket();
			break;

		case q2svc_spawnbaseline:
			CL_ParseBaseline();
			break;

		case q2svc_temp_entity:
			CLQ2_ParseTEnt(net_message);
			break;

		case q2svc_muzzleflash:
			CLQ2_ParseMuzzleFlash(net_message);
			break;

		case q2svc_muzzleflash2:
			CLQ2_ParseMuzzleFlash2(net_message);
			break;

		case q2svc_download:
			CL_ParseDownload();
			break;

		case q2svc_frame:
			CL_ParseFrame();
			break;

		case q2svc_inventory:
			CL_ParseInventory();
			break;

		case q2svc_layout:
			s = const_cast<char*>(net_message.ReadString2());
			String::NCpy(cl.q2_layout, s, sizeof(cl.q2_layout) - 1);
			break;

		case q2svc_playerinfo:
		case q2svc_packetentities:
		case q2svc_deltapacketentities:
			Com_Error(ERR_DROP, "Out of place frame data");
			break;
		}
	}

	CL_AddNetgraph();

	//
	// we don't know if it is ok to save a demo message until
	// after we have parsed the frame
	//
	if (clc.demorecording && !cls.q2_demowaiting)
	{
		CL_WriteDemoMessage();
	}

}
