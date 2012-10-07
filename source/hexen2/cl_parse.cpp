// cl_parse.c  -- parse a message received from the server

/*
 * $Header: /H2 Mission Pack/Cl_parse.c 25    3/20/98 3:45p Jmonroe $
 */

#include "quakedef.h"
#include "../common/file_formats/spr.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include "../client/game/quake_hexen2/view.h"
#include "../client/game/quake_hexen2/parse.h"
#include "../client/game/parse.h"

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

	"h2svc_serverinfo",		// [long] version
	// [string] signon string
	// [string]..[0]model cache [string]...[0]sounds cache
	// [string]..[0]item cache
	"h2svc_lightstyle",		// [byte] [string]
	"h2svc_updatename",		// [byte] [string]
	"h2svc_updatefrags",	// [byte] [short]
	"h2svc_clientdata",		// <shortbits + data>
	"h2svc_stopsound",		// <see code>
	"h2svc_updatecolors",	// [byte] [byte]
	"h2svc_particle",		// [vec3] <variable>
	"h2svc_damage",			// [byte] impact [byte] blood [vec3] from

	"h2svc_spawnstatic",
	"h2svc_raineffect",
	"h2svc_spawnbaseline",

	"h2svc_temp_entity",		// <variable>
	"h2svc_setpause",
	"h2svc_signonnum",
	"h2svc_centerprint",
	"h2svc_killedmonster",
	"h2svc_foundsecret",
	"h2svc_spawnstaticsound",
	"h2svc_intermission",
	"h2svc_finale",			// [string] music [string] text
	"h2svc_cdtrack",			// [byte] track [byte] looptrack
	"h2svc_sellscreen",
	"h2svc_particle2",		// [vec3] <variable>
	"h2svc_cutscene",
	"h2svc_midi_name",
	"h2svc_updateclass",	// [byte] client [byte] class
	"h2svc_particle3",
	"h2svc_particle4",
	"h2svc_set_view_flags",
	"h2svc_clear_view_flags",
	"h2svc_start_effect",
	"h2svc_end_effect",
	"h2svc_plaque",
	"h2svc_particle_explosion",
	"h2svc_set_view_tint",
	"h2svc_reference",
	"h2svc_clear_edicts",
	"h2svc_update_inv",
	"h2svc_setangle_interpolate",
	"h2svc_update_kingofhill",
	"h2svc_toggle_statbar"
};

/*
==================
CL_KeepaliveMessage

When the client is taking a long time to load stuff, send keepalive messages
so the server doesn't disconnect.
==================
*/
void CL_KeepaliveMessage(void)
{
	static float lastmsg;

	if (sv.state != SS_DEAD)
	{
		return;		// no need if server is local
	}
	if (clc.demoplaying)
	{
		return;
	}

	// read messages from server, should just be nops
	QMsg old;
	byte olddata[MAX_MSGLEN_H2];
	old.Copy(olddata, sizeof(olddata), net_message);

	int ret;
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
			if (net_message.ReadByte() != h2svc_nop)
			{
				common->Error("CL_KeepaliveMessage: datagram wasn't a nop");
			}
			break;
		}
	}
	while (ret);

	net_message.Copy(net_message._data, net_message.maxsize, old);

	// check time
	float time = Sys_DoubleTime();
	if (time - lastmsg < 5)
	{
		return;
	}
	lastmsg = time;

	// write out a nop
	common->Printf("--> client to server keepalive\n");

	clc.netchan.message.WriteByte(h2clc_nop);
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
	int i = message.ReadLong();
	if (i != H2PROTOCOL_VERSION)
	{
		common->Printf("Server returned version %i, not %i", i, H2PROTOCOL_VERSION);
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

	if (cl.qh_gametype == QHGAME_DEATHMATCH)
	{
		svh2_kingofhill = message.ReadShort();
	}

	// parse signon message
	const char* str = message.ReadString2();
	String::NCpy(cl.qh_levelname, str, sizeof(cl.qh_levelname) - 1);

	// seperate the printfs so the server message can have a color
	common->Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	common->Printf(S_COLOR_RED "%s" S_COLOR_WHITE "\n", str);

	//
	// first we go through and touch all of the precache data that still
	// happens to be in the cache, so precaching something else doesn't
	// needlessly purge it
	//

	// precache models
	Com_Memset(cl.model_draw, 0, sizeof(cl.model_draw));
	int nummodels;
	char model_precache[MAX_MODELS_H2][MAX_QPATH];
	for (nummodels = 1;; nummodels++)
	{
		str = message.ReadString2();
		if (!str[0])
		{
			break;
		}
		if (nummodels == MAX_MODELS_H2)
		{
			common->Printf("Server sent too many model precaches\n");
			return;
		}
		String::Cpy(model_precache[nummodels], str);
	}

	// precache sounds
	Com_Memset(cl.sound_precache, 0, sizeof(cl.sound_precache));
	int numsounds;
	char sound_precache[MAX_SOUNDS_H2][MAX_QPATH];
	for (numsounds = 1;; numsounds++)
	{
		str = message.ReadString2();
		if (!str[0])
		{
			break;
		}
		if (numsounds == MAX_SOUNDS_H2)
		{
			common->Printf("Server sent too many sound precaches\n");
			return;
		}
		String::Cpy(sound_precache[numsounds], str);
	}

	//
	// now we try to load everything else until a cache allocation fails
	//
	clh2_total_loading_size = nummodels + numsounds;
	clh2_current_loading_size = 1;
	clh2_loading_stage = 2;

	//always precache the world!!!
	CM_LoadMap(model_precache[1], true, NULL);
	R_LoadWorld(model_precache[1]);

	for (i = 2; i < nummodels; i++)
	{
		cl.model_draw[i] = R_RegisterModel(model_precache[i]);
		clh2_current_loading_size++;
		SCR_UpdateScreen();

		if (cl.model_draw[i] == 0)
		{
			common->Printf("Model %s not found\n", model_precache[i]);
			return;
		}
		CL_KeepaliveMessage();
	}

	clh2_player_models[0] = R_RegisterModel("models/paladin.mdl");
	clh2_player_models[1] = R_RegisterModel("models/crusader.mdl");
	clh2_player_models[2] = R_RegisterModel("models/necro.mdl");
	clh2_player_models[3] = R_RegisterModel("models/assassin.mdl");
	clh2_player_models[4] = R_RegisterModel("models/succubus.mdl");

	S_BeginRegistration();
	for (i = 1; i < numsounds; i++)
	{
		cl.sound_precache[i] = S_RegisterSound(sound_precache[i]);
		clh2_current_loading_size++;
		SCR_UpdateScreen();

		CL_KeepaliveMessage();
	}
	S_EndRegistration();

	clh2_total_loading_size = 0;
	clh2_loading_stage = 0;


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
	static double lasttime;

	if (cl_shownet->value == 1)
	{
		common->Printf("Time: %2.2f Pck: %i ", host_time - lasttime, message.cursize);
		lasttime = host_time;
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

	int EntityCount = 0;
	int EntitySize = 0;
	while (1)
	{
		if (message.badread)
		{
			common->Error("CL_ParseServerMessage: Bad server message");
		}

		int cmd = message.ReadByte();

		if (cmd == -1)
		{
			if (cl_shownet->value == 1)
			{
				common->Printf("Ent: %i (%i bytes)", EntityCount, EntitySize);
			}

			SHOWNET("END OF MESSAGE");
			return;		// end of message
		}

		// if the high bit of the command byte is set, it is a fast update
		if (cmd & 128)
		{
			int before = message.readcount;
			SHOWNET("fast update");
			CLH2_ParseUpdate(message, cmd & 127);

			EntityCount++;
			EntitySize += message.readcount - before + 1;
			continue;
		}

		SHOWNET(svc_strings[cmd]);

		// other commands
		switch (cmd)
		{
		default:
			common->Error("CL_ParseServerMessage: Illegible server message\n");
			break;
		case h2svc_nop:
			break;
		case h2svc_time:
			CLQH_ParseTime(message);
			break;
		case h2svc_clientdata:
			CLH2_ParseClientdata(message);
			break;
		case h2svc_version:
			CLH2_ParseVersion(message);
			break;
		case h2svc_disconnect:
			CLQH_ParseDisconnect();
			break;
		case h2svc_print:
			CLH2_ParsePrint(message);
			break;
		case h2svc_centerprint:
			CL_ParseCenterPrint(message);
			break;
		case h2svc_stufftext:
			CL_ParseStuffText(message);
			break;
		case h2svc_damage:
			VQH_ParseDamage(message);
			break;

		case h2svc_serverinfo:
			CL_ParseServerInfo(message);
			break;

		case h2svc_setangle:
			CLQH_ParseSetAngle(message);
			break;
		case h2svc_setangle_interpolate:
			CLH2_ParseSetAngleInterpolate(message);
			break;
		case h2svc_setview:
			CLQH_ParseSetView(message);
			break;
		case h2svc_lightstyle:
			CLQH_ParseLightStyle(message);
			break;
		case h2svc_sound:
			CLH2_ParseStartSoundPacket(message);
			break;
		case h2svc_sound_update_pos:
			CLH2_ParseSoundUpdatePos(message);
			break;
		case h2svc_stopsound:
			CLQH_ParseStopSound(message);
			break;
		case h2svc_updatename:
			CLH2_ParseUpdateName(message);
			break;
		case h2svc_updateclass:
			CLH2_ParseUpdateClass(message);
			break;
		case h2svc_updatefrags:
			CLH2_UpdateFrags(message);
			break;
		case h2svc_update_kingofhill:
			CLH2_ParseUpdateKingOfHill(message);
			break;
		case h2svc_updatecolors:
			CLH2_ParseUpdateColors(message);
			break;
		case h2svc_particle:
			CLH2_ParseParticleEffect(message);
			break;
		case h2svc_particle2:
			CLH2_ParseParticleEffect2(message);
			break;
		case h2svc_particle3:
			CLH2_ParseParticleEffect3(message);
			break;
		case h2svc_particle4:
			CLH2_ParseParticleEffect4(message);
			break;
		case h2svc_spawnbaseline:
			CLH2_ParseSpawnBaseline(message);
			break;
		case h2svc_spawnstatic:
			CLH2_ParseSpawnStatic(message);
			break;
		case h2svc_raineffect:
			CLH2_ParseRainEffect(message);
			break;
		case h2svc_temp_entity:
			CLH2_ParseTEnt(message);
			break;
		case h2svc_setpause:
			CLQH_ParseSetPause(message);
			break;
		case h2svc_signonnum:
			CLH2_ParseSignonNum(message);
			break;
		case h2svc_killedmonster:
			CLQH_ParseKilledMonster();
			break;
		case h2svc_foundsecret:
			CLQH_ParseFoundSecret();
			break;
		case h2svc_updatestat:
			CLQH_UpdateStat(message);
			break;
		case h2svc_spawnstaticsound:
			CLH2_ParseStaticSound(message);
			break;
		case h2svc_cdtrack:
			CLH2_ParseCDTrack(message);
			break;
		case h2svc_midi_name:
			CLH2_ParseMidiName(message);
			break;
		case h2svc_toggle_statbar:
			break;
		case h2svc_intermission:
			CLH2_ParseIntermission(message);
			break;
		case h2svc_set_view_flags:
			CLH2_ParseSetViewFlags(message);
			break;
		case h2svc_clear_view_flags:
			CLH2_ParseClearViewFlags(message);
			break;
		case h2svc_start_effect:
			CLH2_ParseEffect(message);
			break;
		case h2svc_end_effect:
			CLH2_ParseEndEffect(message);
			break;
		case h2svc_plaque:
			CLH2_ParsePlaque(message);
			break;
		case h2svc_particle_explosion:
			CLH2_ParseParticleExplosion(message);
			break;
		case h2svc_set_view_tint:
			CLH2_ParseSetViewTint(message);
			break;
		case h2svc_reference:
			CLH2_ParseReference(message);
			break;
		case h2svc_clear_edicts:
			CLH2_ParseClearEdicts(message);
			break;
		case h2svc_update_inv:
			CLH2_ParseUpdateInv(message);
			break;
		}
	}
}
