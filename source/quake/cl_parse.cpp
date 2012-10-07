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
#include "../common/file_formats/spr.h"
#include "../client/game/quake_hexen2/view.h"
#include "../client/game/parse.h"
#include "../client/game/quake_hexen2/parse.h"

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

	"q1svc_serverinfo",		// [long] version
	// [string] signon string
	// [string]..[0]model cache [string]...[0]sounds cache
	// [string]..[0]item cache
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
	"q1svc_finale",			// [string] music [string] text
	"q1svc_cdtrack",			// [byte] track [byte] looptrack
	"q1svc_sellscreen",
	"q1svc_cutscene"
};

//=============================================================================

/*
==================
CL_KeepaliveMessage

When the client is taking a long time to load stuff, send keepalive messages
so the server doesn't disconnect.
==================
*/
void CL_KeepaliveMessage(void)
{
	float time;
	static float lastmsg;
	int ret;
	QMsg old;
	byte olddata[8192];

	if (sv.state != SS_DEAD)
	{
		return;		// no need if server is local
	}
	if (clc.demoplaying)
	{
		return;
	}

// read messages from server, should just be nops
	old.Copy(olddata, sizeof(olddata), net_message);

	do
	{
		ret = CL_GetMessage();
		switch (ret)
		{
		default:
			common->Error("CL_KeepaliveMessage: CL_GetMessage failed");
		case 0:
			break;	// nothing waiting
		case 1:
			common->Error("CL_KeepaliveMessage: received a message");
			break;
		case 2:
			if (net_message.ReadByte() != q1svc_nop)
			{
				common->Error("CL_KeepaliveMessage: datagram wasn't a nop");
			}
			break;
		}
	}
	while (ret);

	net_message.Copy(net_message._data, net_message.maxsize, old);

// check time
	time = Sys_DoubleTime();
	if (time - lastmsg < 5)
	{
		return;
	}
	lastmsg = time;

// write out a nop
	common->Printf("--> client to server keepalive\n");

	clc.netchan.message.WriteByte(q1clc_nop);
	NET_SendMessage(cls.qh_netcon, &clc.netchan, &clc.netchan.message);
	clc.netchan.message.Clear();
}

/*
==================
CL_ParseServerInfo
==================
*/
void CL_ParseServerInfo(QMsg& message)
{
	const char* str;
	int i;
	int nummodels, numsounds;
	char model_precache[MAX_MODELS_Q1][MAX_QPATH];
	char sound_precache[MAX_SOUNDS_Q1][MAX_QPATH];

	common->DPrintf("Serverinfo packet received.\n");

	R_Shutdown(false);
	CL_InitRenderer();
	clc.qh_signon = 0;

	//
	// wipe the clientActive_t struct
	//
	CL_ClearState();

	SCR_ClearCenterString();

// parse protocol version number
	i = message.ReadLong();
	if (i != Q1PROTOCOL_VERSION)
	{
		common->Printf("Server returned version %i, not %i", i, Q1PROTOCOL_VERSION);
		return;
	}

// parse maxclients
	cl.qh_maxclients = message.ReadByte();
	if (cl.qh_maxclients < 1 || cl.qh_maxclients > MAX_CLIENTS_QH)
	{
		common->Printf("Bad maxclients (%u) from server\n", cl.qh_maxclients);
		return;
	}

// parse gametype
	cl.qh_gametype = message.ReadByte();

// parse signon message
	str = message.ReadString2();
	String::NCpy(cl.qh_levelname, str, sizeof(cl.qh_levelname) - 1);

// seperate the printfs so the server message can have a color
	common->Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	common->Printf(S_COLOR_ORANGE "%s" S_COLOR_WHITE "\n", str);

// precache models
	Com_Memset(cl.model_draw, 0, sizeof(cl.model_draw));
	for (nummodels = 1;; nummodels++)
	{
		str = message.ReadString2();
		if (!str[0])
		{
			break;
		}
		if (nummodels == MAX_MODELS_Q1)
		{
			common->Printf("Server sent too many model precaches\n");
			return;
		}
		String::Cpy(model_precache[nummodels], str);
	}

// precache sounds
	Com_Memset(cl.sound_precache, 0, sizeof(cl.sound_precache));
	for (numsounds = 1;; numsounds++)
	{
		str = message.ReadString2();
		if (!str[0])
		{
			break;
		}
		if (numsounds == MAX_SOUNDS_Q1)
		{
			common->Printf("Server sent too many sound precaches\n");
			return;
		}
		String::Cpy(sound_precache[numsounds], str);
	}

//
// now we try to load everything else until a cache allocation fails
//

	CM_LoadMap(model_precache[1], true, NULL);
	R_LoadWorld(model_precache[1]);
	CL_KeepaliveMessage();

	for (i = 2; i < nummodels; i++)
	{
		cl.model_draw[i] = R_RegisterModel(model_precache[i]);
		if (cl.model_draw[i] == 0)
		{
			common->Printf("Model %s not found\n", model_precache[i]);
			return;
		}
		CL_KeepaliveMessage();
	}

	S_BeginRegistration();
	for (i = 1; i < numsounds; i++)
	{
		cl.sound_precache[i] = S_RegisterSound(sound_precache[i]);
		CL_KeepaliveMessage();
	}
	S_EndRegistration();

	// local state
	R_EndRegistration();
}

#define SHOWNET(x) if (cl_shownet->value == 2) {common->Printf("%3i:%s\n", net_message.readcount - 1, x); }

/*
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage(QMsg& message)
{
	if (cl_shownet->value == 1)
	{
		common->Printf("%i ", message.cursize);
	}
	else if (cl_shownet->value == 2)
	{
		common->Printf("------------------\n");
	}

	cl.qh_onground = false;	// unless the server says otherwise
	//
	// parse the message
	//
	message.BeginReadingOOB();

	while (1)
	{
		if (message.badread)
		{
			common->Error("CL_ParseServerMessage: Bad server message");
		}

		int cmd = message.ReadByte();

		if (cmd == -1)
		{
			SHOWNET("END OF MESSAGE");
			return;		// end of message
		}

		// if the high bit of the command byte is set, it is a fast update
		if (cmd & Q1U_SIGNAL)
		{
			SHOWNET("fast update");
			CLQ1_ParseUpdate(message, cmd & 127);
			continue;
		}

		SHOWNET(svc_strings[cmd]);

		// other commands
		switch (cmd)
		{
		default:
			common->Error("CL_ParseServerMessage: Illegible server message\n");
			break;
		case q1svc_nop:
			break;
		case q1svc_time:
			CLQH_ParseTime(message);
			break;
		case q1svc_clientdata:
			CLQ1_ParseClientdata(message);
			break;
		case q1svc_version:
			CLQ1_ParseVersion(message);
			break;
		case q1svc_disconnect:
			CLQH_ParseDisconnect();
			break;
		case q1svc_print:
			CLQ1_ParsePrint(message);
			break;
		case q1svc_centerprint:
			CL_ParseCenterPrint(message);
			break;
		case q1svc_stufftext:
			CL_ParseStuffText(message);
			break;
		case q1svc_damage:
			VQH_ParseDamage(message);
			break;

		case q1svc_serverinfo:
			CL_ParseServerInfo(message);
			break;

		case q1svc_setangle:
			CLQH_ParseSetAngle(message);
			break;
		case q1svc_setview:
			CLQH_ParseSetView(message);
			break;
		case q1svc_lightstyle:
			CLQH_ParseLightStyle(message);
			break;
		case q1svc_sound:
			CLQ1_ParseStartSoundPacket(message);
			break;
		case q1svc_stopsound:
			CLQH_ParseStopSound(message);
			break;
		case q1svc_updatename:
			CLQ1_UpdateName(message);
			break;
		case q1svc_updatefrags:
			CLQ1_ParseUpdateFrags(message);
			break;
		case q1svc_updatecolors:
			CLQ1_ParseUpdateColours(message);
			break;
		case q1svc_particle:
			CLQ1_ParseParticleEffect(message);
			break;
		case q1svc_spawnbaseline:
			CLQ1_ParseSpawnBaseline(message);
			break;
		case q1svc_spawnstatic:
			CLQ1_ParseSpawnStatic(message);
			break;
		case q1svc_temp_entity:
			CLQ1_ParseTEnt(message);
			break;
		case q1svc_setpause:
			CLQH_ParseSetPause(message);
			break;
		case q1svc_signonnum:
			CLQ1_ParseSignonNum(message);
			break;
		case q1svc_killedmonster:
			CLQH_ParseKilledMonster();
			break;
		case q1svc_foundsecret:
			CLQH_ParseFoundSecret();
			break;
		case q1svc_updatestat:
			CLQH_UpdateStat(message);
			break;
		case q1svc_spawnstaticsound:
			CLQH_ParseStaticSound(message);
			break;
		case q1svc_cdtrack:
			CLQ1_ParseCDTrack(message);
			break;
		case q1svc_intermission:
			CLQ1_ParseIntermission();
			break;
		case q1svc_finale:
			CLQ1_ParseFinale(message);
			break;
		case q1svc_cutscene:
			CLQ1_ParseCutscene(message);
			break;
		case q1svc_sellscreen:
			CLQH_ParseSellScreen();
			break;
		}
	}
}
