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
CL_ParseStartSoundPacket
==================
*/
void CL_ParseStartSoundPacket(void)
{
	vec3_t pos;
	int channel, ent;
	int sound_num;
	int volume;
	int field_mask;
	float attenuation;
	int i;

	field_mask = net_message.ReadByte();

	if (field_mask & QHSND_VOLUME)
	{
		volume = net_message.ReadByte();
	}
	else
	{
		volume = QHDEFAULT_SOUND_PACKET_VOLUME;
	}

	if (field_mask & QHSND_ATTENUATION)
	{
		attenuation = net_message.ReadByte() / 64.0;
	}
	else
	{
		attenuation = QHDEFAULT_SOUND_PACKET_ATTENUATION;
	}

	channel = net_message.ReadShort();
	sound_num = net_message.ReadByte();

	ent = channel >> 3;
	channel &= 7;

	if (ent > MAX_EDICTS_QH)
	{
		common->Error("CL_ParseStartSoundPacket: ent = %i", ent);
	}

	for (i = 0; i < 3; i++)
		pos[i] = net_message.ReadCoord();

	S_StartSound(pos, ent, channel, cl.sound_precache[sound_num], volume / 255.0, attenuation);
}

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
==================
CL_ParseServerInfo
==================
*/
void CL_ParseServerInfo(void)
{
	const char* str;
	int i;
	int nummodels, numsounds;
	char model_precache[MAX_MODELS_Q1][MAX_QPATH];
	char sound_precache[MAX_SOUNDS_Q1][MAX_QPATH];

	common->DPrintf("Serverinfo packet received.\n");
//
// wipe the clientActive_t struct
//
	CL_ClearState();

	SCR_ClearCenterString();

// parse protocol version number
	i = net_message.ReadLong();
	if (i != Q1PROTOCOL_VERSION)
	{
		common->Printf("Server returned version %i, not %i", i, Q1PROTOCOL_VERSION);
		return;
	}

// parse maxclients
	cl.qh_maxclients = net_message.ReadByte();
	if (cl.qh_maxclients < 1 || cl.qh_maxclients > MAX_CLIENTS_QH)
	{
		common->Printf("Bad maxclients (%u) from server\n", cl.qh_maxclients);
		return;
	}

// parse gametype
	cl.qh_gametype = net_message.ReadByte();

// parse signon message
	str = net_message.ReadString2();
	String::NCpy(cl.qh_levelname, str, sizeof(cl.qh_levelname) - 1);

// seperate the printfs so the server message can have a color
	common->Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	common->Printf(S_COLOR_ORANGE "%s" S_COLOR_WHITE "\n", str);

// precache models
	Com_Memset(cl.model_draw, 0, sizeof(cl.model_draw));
	for (nummodels = 1;; nummodels++)
	{
		str = net_message.ReadString2();
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
		str = net_message.ReadString2();
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
	R_NewMap();
}

/*
==================
CL_ParseClientdata

Server information pertaining to this client only
==================
*/
void CL_ParseClientdata(int bits)
{
	int i, j;

	if (bits & Q1SU_VIEWHEIGHT)
	{
		cl.qh_viewheight = net_message.ReadChar();
	}
	else
	{
		cl.qh_viewheight = Q1DEFAULT_VIEWHEIGHT;
	}

	if (bits & Q1SU_IDEALPITCH)
	{
		cl.qh_idealpitch = net_message.ReadChar();
	}
	else
	{
		cl.qh_idealpitch = 0;
	}

	VectorCopy(cl.qh_mvelocity[0], cl.qh_mvelocity[1]);
	for (i = 0; i < 3; i++)
	{
		if (bits & (Q1SU_PUNCH1 << i))
		{
			cl.qh_punchangles[i] = net_message.ReadChar();
		}
		else
		{
			cl.qh_punchangles[i] = 0;
		}
		if (bits & (Q1SU_VELOCITY1 << i))
		{
			cl.qh_mvelocity[0][i] = net_message.ReadChar() * 16;
		}
		else
		{
			cl.qh_mvelocity[0][i] = 0;
		}
	}

// [always sent]	if (bits & Q1SU_ITEMS)
	i = net_message.ReadLong();

	if (cl.q1_items != i)
	{	// set flash times
		for (j = 0; j < 32; j++)
			if ((i & (1 << j)) && !(cl.q1_items & (1 << j)))
			{
				cl.q1_item_gettime[j] = cl.qh_serverTimeFloat;
			}
		cl.q1_items = i;
	}

	cl.qh_onground = (bits & Q1SU_ONGROUND) != 0;

	if (bits & Q1SU_WEAPONFRAME)
	{
		cl.qh_stats[Q1STAT_WEAPONFRAME] = net_message.ReadByte();
	}
	else
	{
		cl.qh_stats[Q1STAT_WEAPONFRAME] = 0;
	}

	if (bits & Q1SU_ARMOR)
	{
		i = net_message.ReadByte();
	}
	else
	{
		i = 0;
	}
	if (cl.qh_stats[Q1STAT_ARMOR] != i)
	{
		cl.qh_stats[Q1STAT_ARMOR] = i;
	}

	if (bits & Q1SU_WEAPON)
	{
		i = net_message.ReadByte();
	}
	else
	{
		i = 0;
	}
	if (cl.qh_stats[Q1STAT_WEAPON] != i)
	{
		cl.qh_stats[Q1STAT_WEAPON] = i;
	}

	i = net_message.ReadShort();
	if (cl.qh_stats[Q1STAT_HEALTH] != i)
	{
		cl.qh_stats[Q1STAT_HEALTH] = i;
	}

	i = net_message.ReadByte();
	if (cl.qh_stats[Q1STAT_AMMO] != i)
	{
		cl.qh_stats[Q1STAT_AMMO] = i;
	}

	for (i = 0; i < 4; i++)
	{
		j = net_message.ReadByte();
		if (cl.qh_stats[Q1STAT_SHELLS + i] != j)
		{
			cl.qh_stats[Q1STAT_SHELLS + i] = j;
		}
	}

	i = net_message.ReadByte();

	if (q1_standard_quake)
	{
		if (cl.qh_stats[Q1STAT_ACTIVEWEAPON] != i)
		{
			cl.qh_stats[Q1STAT_ACTIVEWEAPON] = i;
		}
	}
	else
	{
		if (cl.qh_stats[Q1STAT_ACTIVEWEAPON] != (1 << i))
		{
			cl.qh_stats[Q1STAT_ACTIVEWEAPON] = (1 << i);
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
	if (slot > cl.qh_maxclients)
	{
		common->FatalError("CL_NewTranslation: slot > cl.maxclients");
	}
	CLQ1_TranslatePlayerSkin(slot);
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

static void CL_ParsePrint()
{
	const char* txt = net_message.ReadString2();
	if (txt[0] == 1)
	{
		S_StartLocalSound("misc/talk.wav");
	}
	if (txt[0] == 1 || txt[0] == 2)
	{
		common->Printf(S_COLOR_ORANGE "%s" S_COLOR_WHITE, txt + 1);
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
void CL_ParseServerMessage(void)
{
	int cmd;
	int i;

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

	cl.qh_onground = false;	// unless the server says otherwise
//
// parse the message
//
	net_message.BeginReadingOOB();

	while (1)
	{
		if (net_message.badread)
		{
			common->Error("CL_ParseServerMessage: Bad server message");
		}

		cmd = net_message.ReadByte();

		if (cmd == -1)
		{
			SHOWNET("END OF MESSAGE");
			return;		// end of message
		}

		// if the high bit of the command byte is set, it is a fast update
		if (cmd & Q1U_SIGNAL)
		{
			SHOWNET("fast update");
			CLQ1_ParseUpdate(net_message, cmd & 127);
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
//			common->Printf ("q1svc_nop\n");
			break;

		case q1svc_time:
			cl.qh_mtime[1] = cl.qh_mtime[0];
			cl.qh_mtime[0] = net_message.ReadFloat();
			break;

		case q1svc_clientdata:
			i = net_message.ReadShort();
			CL_ParseClientdata(i);
			break;

		case q1svc_version:
			i = net_message.ReadLong();
			if (i != Q1PROTOCOL_VERSION)
			{
				common->Error("CL_ParseServerMessage: Server is protocol %i instead of %i\n", i, Q1PROTOCOL_VERSION);
			}
			break;

		case q1svc_disconnect:
			common->EndGame("Server disconnected\n");

		case q1svc_print:
			CL_ParsePrint();
			break;

		case q1svc_centerprint:
			SCR_CenterPrint(net_message.ReadString2());
			break;

		case q1svc_stufftext:
			Cbuf_AddText(net_message.ReadString2());
			break;

		case q1svc_damage:
			VQH_ParseDamage(net_message);
			break;

		case q1svc_serverinfo:
			CL_ParseServerInfo();
			break;

		case q1svc_setangle:
			for (i = 0; i < 3; i++)
				cl.viewangles[i] = net_message.ReadAngle();
			break;

		case q1svc_setview:
			cl.viewentity = net_message.ReadShort();
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

		case q1svc_updatename:
			i = net_message.ReadByte();
			if (i >= cl.qh_maxclients)
			{
				common->Error("CL_ParseServerMessage: q1svc_updatename > MAX_CLIENTS_QH");
			}
			String::Cpy(cl.q1_players[i].name, net_message.ReadString2());
			break;

		case q1svc_updatefrags:
			i = net_message.ReadByte();
			if (i >= cl.qh_maxclients)
			{
				common->Error("CL_ParseServerMessage: q1svc_updatefrags > MAX_CLIENTS_QH");
			}
			cl.q1_players[i].frags = net_message.ReadShort();
			break;

		case q1svc_updatecolors:
		{
			i = net_message.ReadByte();
			if (i >= cl.qh_maxclients)
			{
				common->Error("CL_ParseServerMessage: q1svc_updatecolors > MAX_CLIENTS_QH");
			}
			int j = net_message.ReadByte();
			cl.q1_players[i].topcolor = (j & 0xf0) >> 4;
			cl.q1_players[i].bottomcolor = (j & 15);
			CL_NewTranslation(i);
		}
		break;

		case q1svc_particle:
			R_ParseParticleEffect();
			break;

		case q1svc_spawnbaseline:
			CLQ1_ParseSpawnBaseline(net_message);
			break;
		case q1svc_spawnstatic:
			CLQ1_ParseSpawnStatic(net_message);
			break;
		case q1svc_temp_entity:
			CLQ1_ParseTEnt(net_message);
			break;

		case q1svc_setpause:
		{
			cl.qh_paused = net_message.ReadByte();

			if (cl.qh_paused)
			{
				CDAudio_Pause();
			}
			else
			{
				CDAudio_Resume();
			}
		}
		break;

		case q1svc_signonnum:
			i = net_message.ReadByte();
			if (i <= clc.qh_signon)
			{
				common->Error("Received signon %i when at %i", i, clc.qh_signon);
			}
			clc.qh_signon = i;
			CLQ1_SignonReply();
			break;

		case q1svc_killedmonster:
			cl.qh_stats[Q1STAT_MONSTERS]++;
			break;

		case q1svc_foundsecret:
			cl.qh_stats[Q1STAT_SECRETS]++;
			break;

		case q1svc_updatestat:
			i = net_message.ReadByte();
			if (i < 0 || i >= MAX_CL_STATS)
			{
				common->FatalError("q1svc_updatestat: %i is invalid", i);
			}
			cl.qh_stats[i] = net_message.ReadLong();;
			break;

		case q1svc_spawnstaticsound:
			CL_ParseStaticSound();
			break;

		case q1svc_cdtrack:
		{
			byte cdtrack = net_message.ReadByte();
			net_message.ReadByte();	//	looptrack
			if ((clc.demoplaying || clc.demorecording) && (cls.qh_forcetrack != -1))
			{
				CDAudio_Play((byte)cls.qh_forcetrack, true);
			}
			else
			{
				CDAudio_Play(cdtrack, true);
			}
		}
		break;

		case q1svc_intermission:
			cl.qh_intermission = 1;
			cl.qh_completed_time = cl.qh_serverTimeFloat;
			break;

		case q1svc_finale:
			cl.qh_intermission = 2;
			cl.qh_completed_time = cl.qh_serverTimeFloat;
			SCR_CenterPrint(net_message.ReadString2());
			break;

		case q1svc_cutscene:
			cl.qh_intermission = 3;
			cl.qh_completed_time = cl.qh_serverTimeFloat;
			SCR_CenterPrint(net_message.ReadString2());
			break;

		case q1svc_sellscreen:
			Cmd_ExecuteString("help");
			break;
		}
	}
}
