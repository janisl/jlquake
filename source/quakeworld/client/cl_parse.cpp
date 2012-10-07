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
#include "../../client/game/parse.h"
#include "../../client/game/quake_hexen2/parse.h"
#include "../../client/game/quake_hexen2/view.h"

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

//=============================================================================

/*
=================
CLQW_Model_NextDownload
=================
*/
void CLQW_Model_NextDownload(void)
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
			common->Printf("\nThe required model file '%s' could not be found or downloaded.\n\n",
				cl.qh_model_name[i]);
			common->Printf("You may need to download or purchase a %s client "
					   "pack in order to play on this server.\n\n", gamedirfile);
			CL_Disconnect();
			return;
		}
	}

	CLQW_CalcModelChecksum("progs/player.mdl", pmodel_name);
	CLQW_CalcModelChecksum("progs/eyes.mdl", emodel_name);

	// all done
	R_EndRegistration();

	int CheckSum1;
	int CheckSum2;
	CM_MapChecksums(CheckSum1, CheckSum2);

	// done with modellist, request first of static signon messages
	CL_AddReliableCommand(va(prespawn_name, cl.servercount, CheckSum2));
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
	clqw_flagindex = -1;
	CL_AddReliableCommand(va(modellist_name, cl.servercount, 0));
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
		CLQW_Model_NextDownload();
		break;
	case dl_sound:
		Sound_NextDownload();
		break;
	case dl_none:
	default:
		common->DPrintf("Unknown download type.\n");
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

		CL_AddReliableCommand("nextdl");
	}
	else
	{
		char oldn[MAX_OSPATH];
		char newn[MAX_OSPATH];

#if 0
		common->Printf("100%%\n");
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

	common->DPrintf("Serverdata packet received.\n");

	Cbuf_Execute();			// make sure any stuffed commands are done

	R_Shutdown(false);
	CL_InitRenderer();

	//
	// wipe the clientActive_t struct
	//
	CL_ClearState();

// parse protocol version number
// allow 2.2 and 2.29 demos to play
	protover = net_message.ReadLong();
	if (protover != QWPROTOCOL_VERSION &&
		!(clc.demoplaying && (protover == 26 || protover == 27 || protover == 28)))
	{
		common->Error("Server returned version %i, not %i\nYou probably need to upgrade.\nCheck http://www.quakeworld.net/", protover, QWPROTOCOL_VERSION);
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
	common->Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	common->Printf(S_COLOR_ORANGE "%s" S_COLOR_WHITE "\n", str);

	// ask for the sound list next
	Com_Memset(cl.qh_sound_name, 0, sizeof(cl.qh_sound_name));
	CL_AddReliableCommand(va(soundlist_name, cl.servercount, 0));

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
			common->Error("Server sent too many sound_precache");
		}
		String::Cpy(cl.qh_sound_name[numsounds], str);
	}

	n = net_message.ReadByte();

	if (n)
	{
		CL_AddReliableCommand(va(soundlist_name, cl.servercount, n));
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
			common->Error("Server sent too many model_precache");
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
			clqw_flagindex = nummodels;
		}
	}

	n = net_message.ReadByte();

	if (n)
	{
		CL_AddReliableCommand(va(modellist_name, cl.servercount, n));
		return;
	}

	clc.downloadNumber = 0;
	clc.downloadType = dl_model;
	CLQW_Model_NextDownload();
}

/*
=====================================================================

ACTION MESSAGES

=====================================================================
*/

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

#define SHOWNET(x) if (cl_shownet->value == 2) {common->Printf("%3i:%s\n", net_message.readcount - 1, x); }
/*
=====================
CL_ParseServerMessage
=====================
*/
int received_framecount;
void CL_ParseServerMessage(void)
{
	int cmd;
	int i, j;

	received_framecount = host_framecount;
	CLQ1_ClearProjectiles();

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
			common->Error("CL_ParseServerMessage: Illegible server message");
			break;
		case q1svc_nop:
			break;
		case q1svc_disconnect:
			CLQW_ParseDisconnect();
			break;
		case q1svc_print:
			CLQW_ParsePrint(net_message);
			break;
		case q1svc_centerprint:
			CL_ParseCenterPrint(net_message);
			break;
		case q1svc_stufftext:
			CL_ParseStuffText(net_message);
			break;
		case q1svc_damage:
			VQH_ParseDamage(net_message);
			break;

		case qwsvc_serverdata:
			CL_ParseServerData();
			break;

		case q1svc_setangle:
			CLQH_ParseSetAngle(net_message);
			break;
		case q1svc_lightstyle:
			CLQH_ParseLightStyle(net_message);
			break;
		case q1svc_sound:
			CLQW_ParseStartSoundPacket(net_message);
			break;
		case q1svc_stopsound:
			CLQH_ParseStopSound(net_message);
			break;
		case q1svc_updatefrags:
			CLQW_ParseUpdateFrags(net_message);
			break;
		case qwsvc_updateping:
			CLQW_ParseUpdatePing(net_message);
			break;
		case qwsvc_updatepl:
			CLQW_ParseUpdatePacketLossage(net_message);
			break;
		case qwsvc_updateentertime:
			CLQW_ParseUpdateEnterTime(net_message);
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
			CLQH_ParseKilledMonster();
			break;
		case q1svc_foundsecret:
			CLQH_ParseFoundSecret();
			break;
		case q1svc_updatestat:
			CLQW_ParseUpdateStat(net_message);
			break;
		case qwsvc_updatestatlong:
			CLQW_ParseUpdateStatLong(net_message);
			break;
		case q1svc_spawnstaticsound:
			CLQH_ParseStaticSound(net_message);
			break;
		case q1svc_cdtrack:
			CLQHW_ParseCDTrack(net_message);
			break;
		case q1svc_intermission:
			CLQW_ParseIntermission(net_message);
			break;
		case q1svc_finale:
			CLQHW_ParseFinale(net_message);
			break;
		case q1svc_sellscreen:
			CLQH_ParseSellScreen();
			break;
		case qwsvc_smallkick:
			CLQHW_ParseSmallKick();
			break;
		case qwsvc_bigkick:
			CLQHW_ParseBigKick();
			break;
		case qwsvc_muzzleflash:
			CLQW_MuzzleFlash(net_message);
			break;
		case qwsvc_updateuserinfo:
			CLQW_ParseUpdateUserinfo(net_message);
			break;
		case qwsvc_setinfo:
			CLQW_ParseSetInfo(net_message);
			break;
		case qwsvc_serverinfo:
			CLQW_ParseServerInfo(net_message);
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
			CLQHW_ParseMaxSpeed(net_message);
			break;
		case qwsvc_entgravity:
			CLQHW_ParseEntGravity(net_message);
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

	CLQW_SetSolidEntities();
}
