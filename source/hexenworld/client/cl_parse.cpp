// cl_parse.c  -- parse a message received from the server

#include "quakedef.h"
#include "../../client/game/quake_hexen2/view.h"
#include "../../client/game/quake_hexen2/parse.h"
#include "../../client/game/parse.h"
#include "../../common/hexen2strings.h"

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
void CL_ParseServerData(QMsg& message)
{
	common->DPrintf("Serverdata packet received.\n");

	Cbuf_Execute();			// make sure any stuffed commands are done
	//
	// wipe the clientActive_t struct
	//
	R_Shutdown(false);
	CL_InitRenderer();

	CL_ClearState();

	// parse protocol version number
	int protover = message.ReadLong();
	if (protover != HWPROTOCOL_VERSION && protover != HWOLD_PROTOCOL_VERSION)
	{
		common->Error("Server returned version %i, not %i", protover, HWPROTOCOL_VERSION);
	}

	cl.servercount = message.ReadLong();

	// game directory
	const char* str = message.ReadString2();

	bool cflag = false;
	if (String::ICmp(fsqhw_gamedirfile, str))
	{
		// save current config
		Com_WriteConfiguration();
		cflag = true;
	}

	FS_SetGamedirQHW(str);

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
	cl.playernum = message.ReadByte();
	if (cl.playernum & 128)
	{
		cl.qh_spectator = true;
		cl.playernum &= ~128;
	}
	cl.viewentity = cl.playernum + 1;

	// get the full level name
	str = const_cast<char*>(message.ReadString2());
	String::NCpy(cl.qh_levelname, str, sizeof(cl.qh_levelname) - 1);

	// get the movevars
	if (protover == HWPROTOCOL_VERSION)
	{
		movevars.gravity            = message.ReadFloat();
		movevars.stopspeed          = message.ReadFloat();
		movevars.maxspeed           = message.ReadFloat();
		movevars.spectatormaxspeed  = message.ReadFloat();
		movevars.accelerate         = message.ReadFloat();
		movevars.airaccelerate      = message.ReadFloat();
		movevars.wateraccelerate    = message.ReadFloat();
		movevars.friction           = message.ReadFloat();
		movevars.waterfriction      = message.ReadFloat();
		movevars.entgravity         = message.ReadFloat();
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
	CL_AddReliableCommand(va("soundlist %i", cl.servercount));

	// now waiting for downloads, etc
	cls.state = CA_LOADING;
	clhw_keyholder = -1;
	clhw_doc = -1;
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
void CL_ParseServerMessage(QMsg& message)
{
	CLH2_ClearProjectiles();
	CLH2_ClearMissiles();
	// This clears out the target field on each netupdate; it won't be drawn unless another update comes...
	CLHW_ClearTarget();

	if (cl_shownet->value == 1)
	{
		common->Printf("%i ",message.cursize);
	}
	else if (cl_shownet->value == 2)
	{
		common->Printf("------------------\n");
	}

	CLHW_ParseClientdata();

	//
	// parse the message
	//
	while (1)
	{
		if (message.badread)
		{
			common->Error("CL_ParseServerMessage: Bad server message");
			break;
		}

		int cmd = message.ReadByte();

		if (cmd == -1)
		{
			message.readcount++;	// so the EOM showner has the right value
			SHOWNET(message, "END OF MESSAGE");
			break;
		}

		SHOWNET(message, svc_strings[cmd]);

		// other commands
		switch (cmd)
		{
		default:
			common->Error("CL_ParseServerMessage: Illegible server message\n");
			break;
		case h2svc_nop:
			break;
		case h2svc_disconnect:
			CLHW_ParseDisconnect();
			break;
		case h2svc_print:
			CLHW_ParsePrint(message);
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

		case hwsvc_serverdata:
			CL_ParseServerData(message);
			break;

		case h2svc_setangle:
			CLQH_ParseSetAngle(message);
			break;
		case h2svc_lightstyle:
			CLQH_ParseLightStyle(message);
			break;
		case h2svc_sound:
			CLHW_ParseStartSoundPacket(message);
			break;
		case hwsvc_sound_update_pos:
			CLH2_ParseSoundUpdatePos(message);
			break;
		case h2svc_stopsound:
			CLQH_ParseStopSound(message);
			break;
		case h2svc_updatefrags:
			CLHW_UpdateFrags(message);
			break;
		case hwsvc_updateping:
			CLHW_ParseUpdatePing(message);
			break;
		case hwsvc_updateentertime:
			CLHW_ParseUpdateEnterTime(message);
			break;
		case hwsvc_updatepclass:
			CLHW_ParseUpdateClass(message);
			break;
		case hwsvc_updatedminfo:
			CLHW_ParseUpdateDMInfo(message);
			break;
		case hwsvc_updatesiegelosses:
			CLHW_ParseUpdateSiegeLosses(message);
			break;
		case hwsvc_updatesiegeteam:
			CLHW_ParseUpdateSiegeTeam(message);
			break;
		case hwsvc_updatesiegeinfo:
			CLHW_ParseUpdateSiegeInfo(message);
			break;
		case hwsvc_haskey:
			CLHW_ParseHasKey(message);
			break;
		case hwsvc_isdoc:
			CLHW_ParseIsDoc(message);
			break;
		case hwsvc_nonehaskey:
			CLHW_ParseNoneHasKey();
			break;
		case hwsvc_nodoc:
			CLHW_ParseNoDoc();
			break;
		case h2svc_time:
			CLHW_ParseTime(message);
			break;
		case h2svc_spawnbaseline:
			CLH2_ParseSpawnBaseline(message);
			break;
		case h2svc_spawnstatic:
			CLH2_ParseSpawnStatic(message);
			break;
		case h2svc_temp_entity:
			CLHW_ParseTEnt(message);
			break;
		case h2svc_killedmonster:
			CLQH_ParseKilledMonster();
			break;
		case h2svc_foundsecret:
			CLQH_ParseFoundSecret();
			break;
		case h2svc_updatestat:
			CLHW_ParseUpdateStat(message);
			break;
		case hwsvc_updatestatlong:
			CLHW_ParseUpdateStatLong(message);
			break;
		case h2svc_spawnstaticsound:
			CLQH_ParseStaticSound(message);
			break;
		case h2svc_cdtrack:
			CLQHW_ParseCDTrack(message);
			break;
		case h2svc_intermission:
			CLHW_ParseIntermission(message);
			break;
		case h2svc_finale:
			CLQHW_ParseFinale(message);
			break;
		case h2svc_sellscreen:
			CLQH_ParseSellScreen();
			break;
		case hwsvc_smallkick:
			CLQHW_ParseSmallKick();
			break;
		case hwsvc_bigkick:
			CLQHW_ParseBigKick();
			break;
		case hwsvc_muzzleflash:
			CLHW_MuzzleFlash(message);
			break;
		case hwsvc_updateuserinfo:
			CLHW_UpdateUserinfo(message);
			break;
		case hwsvc_download:
			CLHW_ParseDownload(message);
			break;
		case hwsvc_playerinfo:
			CLHW_ParsePlayerinfo(message);
			break;
		case hwsvc_playerskipped:
			CLHW_SavePlayer(message);
			break;
		case hwsvc_nails:
			CLHW_ParseNails(message);
			break;
		case hwsvc_packmissile:
			CLHW_ParsePackMissiles(message);
			break;
		case hwsvc_chokecount:
			CLHW_ParseChokeCount(message);
			break;
		case hwsvc_modellist:
			CLHW_ParseModelList(message);
			break;
		case hwsvc_soundlist:
			CLHW_ParseSoundList(message);
			break;
		case hwsvc_packetentities:
			CLHW_ParsePacketEntities(message);
			break;
		case hwsvc_deltapacketentities:
			CLHW_ParseDeltaPacketEntities(message);
			break;
		case hwsvc_maxspeed:
			CLQHW_ParseMaxSpeed(message);
			break;
		case hwsvc_entgravity:
			CLQHW_ParseEntGravity(message);
			break;
		case hwsvc_plaque:
			CLH2_ParsePlaque(message);
			break;
		case hwsvc_indexed_print:
			CLHW_IndexedPrint(message);
			break;
		case hwsvc_name_print:
			CLHW_NamePrint(message);
			break;
		case hwsvc_particle_explosion:
			CLH2_ParseParticleExplosion(message);
			break;
		case hwsvc_set_view_tint:
			CLHw_ParseSetViewTint(message);
			break;
		case hwsvc_start_effect:
			CLH2_ParseEffect(message);
			break;
		case hwsvc_end_effect:
			CLH2_ParseEndEffect(message);
			break;
		case hwsvc_set_view_flags:
			CLH2_ParseSetViewFlags(message);
			break;
		case hwsvc_clear_view_flags:
			CLH2_ParseClearViewFlags(message);
			break;
		case hwsvc_update_inv:
			CLHW_ParseUpdateInv(message);
			break;
		case h2svc_particle:
			CLH2_ParseParticleEffect(message);
			break;
		case hwsvc_particle2:
			CLHW_ParseParticleEffect2(message);
			break;
		case hwsvc_particle3:
			CLHW_ParseParticleEffect3(message);
			break;
		case hwsvc_particle4:
			CLHW_ParseParticleEffect4(message);
			break;
		case hwsvc_turn_effect:
			CLHW_ParseTurnEffect(message);
			break;
		case hwsvc_update_effect:
			CLHW_ParseReviseEffect(message);
			break;
		case hwsvc_multieffect:
			CLHW_ParseMultiEffect(message);
			break;
		case hwsvc_midi_name:
			CLH2_ParseMidiName(message);
			break;
		case hwsvc_raineffect:
			CLH2_ParseRainEffect(message);
			break;
		case hwsvc_targetupdate:
			CLHW_ParseTarget(message);
			break;
		case hwsvc_update_piv:
			CLHW_ParseUpdatePiv(message);
			break;
		case hwsvc_player_sound:
			CLHW_ParsePlayerSound(message);
			break;
		}
	}

	CLHW_SetSolidEntities();
}
