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

	if (String::ICmp(fsqhw_gamedirfile, str))
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
	CL_AddReliableCommand(va("soundlist %i %i", cl.servercount, 0));

	// now waiting for downloads, etc
	cls.state = CA_LOADING;
}

/*
=====================================================================

ACTION MESSAGES

=====================================================================
*/

/*
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage(void)
{
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

	CLQW_ParseClientdata();

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

		int cmd = net_message.ReadByte();

		if (cmd == -1)
		{
			net_message.readcount++;	// so the EOM showner has the right value
			SHOWNET(net_message, "END OF MESSAGE");
			break;
		}

		SHOWNET(net_message, svc_strings[cmd]);

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
			CLQW_ParseDownload(net_message);
			break;
		case qwsvc_playerinfo:
			CLQW_ParsePlayerinfo(net_message);
			break;
		case qwsvc_nails:
			CLQW_ParseNails(net_message);
			break;
		case qwsvc_chokecount:
			CLQW_ParseChokeCount(net_message);
			break;
		case qwsvc_modellist:
			CLQW_ParseModelList(net_message);
			break;
		case qwsvc_soundlist:
			CLQW_ParseSoundList(net_message);
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
			CLQW_ParseSetPause(net_message);
			break;
		}
	}

	CLQW_SetSolidEntities();
}
