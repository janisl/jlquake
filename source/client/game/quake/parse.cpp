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
#include "../../public.h"
#include "../../ui/ui.h"
#include "../particles.h"
#include "../dynamic_lights.h"
#include "../parse.h"
#include "../quake_hexen2/parse.h"
#include "../quake_hexen2/connection.h"
#include "../quake_hexen2/view.h"
#include "../quake_hexen2/predict.h"
#include "../../../common/Common.h"
#include "../../../common/common_defs.h"
#include "../../../common/strings.h"
#include "../../../common/player_move.h"
#include "../../../common/crc.h"

static const char* svcq1_strings[] =
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

static const char* svcqw_strings[] =
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

//	Server information pertaining to this client only
static void CLQ1_ParseClientdata( QMsg& message ) {
	int bits = message.ReadShort();

	if ( bits & Q1SU_VIEWHEIGHT ) {
		cl.qh_viewheight = message.ReadChar();
	} else   {
		cl.qh_viewheight = Q1DEFAULT_VIEWHEIGHT;
	}

	if ( bits & Q1SU_IDEALPITCH ) {
		cl.qh_idealpitch = message.ReadChar();
	} else   {
		cl.qh_idealpitch = 0;
	}

	VectorCopy( cl.qh_mvelocity[ 0 ], cl.qh_mvelocity[ 1 ] );
	for ( int i = 0; i < 3; i++ ) {
		if ( bits & ( Q1SU_PUNCH1 << i ) ) {
			cl.qh_punchangles[ i ] = message.ReadChar();
		} else   {
			cl.qh_punchangles[ i ] = 0;
		}
		if ( bits & ( Q1SU_VELOCITY1 << i ) ) {
			cl.qh_mvelocity[ 0 ][ i ] = message.ReadChar() * 16;
		} else   {
			cl.qh_mvelocity[ 0 ][ i ] = 0;
		}
	}

	// [always sent]	if (bits & Q1SU_ITEMS)
	int i = message.ReadLong();

	if ( cl.q1_items != i ) {
		// set flash times
		for ( int j = 0; j < 32; j++ ) {
			if ( ( i & ( 1 << j ) ) && !( cl.q1_items & ( 1 << j ) ) ) {
				cl.q1_item_gettime[ j ] = cl.qh_serverTimeFloat;
			}
		}
		cl.q1_items = i;
	}

	cl.qh_onground = ( bits & Q1SU_ONGROUND ) != 0;

	if ( bits & Q1SU_WEAPONFRAME ) {
		cl.qh_stats[ Q1STAT_WEAPONFRAME ] = message.ReadByte();
	} else   {
		cl.qh_stats[ Q1STAT_WEAPONFRAME ] = 0;
	}

	if ( bits & Q1SU_ARMOR ) {
		i = message.ReadByte();
	} else   {
		i = 0;
	}
	if ( cl.qh_stats[ Q1STAT_ARMOR ] != i ) {
		cl.qh_stats[ Q1STAT_ARMOR ] = i;
	}

	if ( bits & Q1SU_WEAPON ) {
		i = message.ReadByte();
	} else   {
		i = 0;
	}
	if ( cl.qh_stats[ Q1STAT_WEAPON ] != i ) {
		cl.qh_stats[ Q1STAT_WEAPON ] = i;
	}

	i = message.ReadShort();
	if ( cl.qh_stats[ Q1STAT_HEALTH ] != i ) {
		cl.qh_stats[ Q1STAT_HEALTH ] = i;
	}

	i = message.ReadByte();
	if ( cl.qh_stats[ Q1STAT_AMMO ] != i ) {
		cl.qh_stats[ Q1STAT_AMMO ] = i;
	}

	for ( i = 0; i < 4; i++ ) {
		int j = message.ReadByte();
		if ( cl.qh_stats[ Q1STAT_SHELLS + i ] != j ) {
			cl.qh_stats[ Q1STAT_SHELLS + i ] = j;
		}
	}

	i = message.ReadByte();

	if ( q1_standard_quake ) {
		if ( cl.qh_stats[ Q1STAT_ACTIVEWEAPON ] != i ) {
			cl.qh_stats[ Q1STAT_ACTIVEWEAPON ] = i;
		}
	} else   {
		if ( cl.qh_stats[ Q1STAT_ACTIVEWEAPON ] != ( 1 << i ) ) {
			cl.qh_stats[ Q1STAT_ACTIVEWEAPON ] = ( 1 << i );
		}
	}
}

//	Server information pertaining to this client only, sent every frame
static void CLQW_ParseClientdata() {
	// calculate simulated time of message
	cl.qh_parsecount = clc.netchan.incomingAcknowledged;
	qwframe_t* frame = &cl.qw_frames[ cl.qh_parsecount &  UPDATE_MASK_QW ];

	frame->receivedtime = cls.realtime * 0.001;

	// calculate latency
	float latency = frame->receivedtime - frame->senttime;

	if ( latency < 0 || latency > 1.0 ) {
		//common->Printf ("Odd latency: %5.2f\n", latency);
	} else   {
		// drift the average latency towards the observed latency
		if ( latency < cls.qh_latency ) {
			cls.qh_latency = latency;
		} else   {
			cls.qh_latency += 0.001;	// drift up, so correction are needed
		}
	}
}

static void CLQ1_ParseVersion( QMsg& message ) {
	int i = message.ReadLong();
	if ( i != Q1PROTOCOL_VERSION ) {
		common->Error( "CL_ParseServerMessage: Server is protocol %i instead of %i\n", i, Q1PROTOCOL_VERSION );
	}
}

static void CLQW_ParseDisconnect() {
	if ( cls.state == CA_CONNECTED ) {
		common->Error( "Server disconnected\n"
					   "Server version may not be compatible" );
	} else   {
		common->Error( "Server disconnected" );
	}
}

static void CLQ1_ParsePrint( QMsg& message ) {
	const char* txt = message.ReadString2();

	if ( txt[ 0 ] == 1 ) {
		S_StartLocalSound( "misc/talk.wav" );
	}
	if ( txt[ 0 ] == 1 || txt[ 0 ] == 2 ) {
		common->Printf( S_COLOR_ORANGE "%s" S_COLOR_WHITE, txt + 1 );
	} else   {
		common->Printf( "%s", txt );
	}
}

static void CLQW_ParsePrint( QMsg& message ) {
	int i = message.ReadByte();
	const char* txt = message.ReadString2();

	if ( i == PRINT_CHAT ) {
		S_StartLocalSound( "misc/talk.wav" );
		common->Printf( S_COLOR_ORANGE "%s" S_COLOR_WHITE, txt );
	} else   {
		common->Printf( "%s", txt );
	}
}

static void CLQ1_ParseStartSoundPacket( QMsg& message ) {
	CLQH_ParseStartSoundPacket( message, 0 );
}

static void CLQW_ParseStartSoundPacket( QMsg& message ) {
	CLQHW_ParseStartSoundPacket( message, 64.0 );
}

static void CLQ1_UpdateName( QMsg& message ) {
	int i = message.ReadByte();
	if ( i >= cl.qh_maxclients ) {
		common->Error( "CL_ParseServerMessage: svc_updatename > MAX_CLIENTS_QH" );
	}
	String::Cpy( cl.q1_players[ i ].name, message.ReadString2() );
}

static void CLQ1_ParseUpdateFrags( QMsg& message ) {
	int i = message.ReadByte();
	if ( i >= cl.qh_maxclients ) {
		common->Error( "CL_ParseServerMessage: q1svc_updatefrags > MAX_CLIENTS_QH" );
	}
	cl.q1_players[ i ].frags = message.ReadShort();
}

static void CLQW_ParseUpdateFrags( QMsg& message ) {
	int i = message.ReadByte();
	if ( i >= MAX_CLIENTS_QHW ) {
		common->Error( "CL_ParseServerMessage: q1svc_updatefrags > MAX_CLIENTS_QHW" );
	}
	cl.q1_players[ i ].frags = message.ReadShort();
}

static void CLQ1_ParseUpdateColours( QMsg& message ) {
	int i = message.ReadByte();
	if ( i >= cl.qh_maxclients ) {
		common->Error( "CL_ParseServerMessage: q1svc_updatecolors > MAX_CLIENTS_QH" );
	}
	int j = message.ReadByte();

	cl.q1_players[ i ].topcolor = ( j & 0xf0 ) >> 4;
	cl.q1_players[ i ].bottomcolor = ( j & 15 );
	CLQ1_TranslatePlayerSkin( i );
}

static void CLQW_ParseUpdatePing( QMsg& message ) {
	int i = message.ReadByte();
	if ( i >= MAX_CLIENTS_QHW ) {
		common->Error( "CL_ParseServerMessage: qwsvc_updateping > MAX_CLIENTS_QHW" );
	}
	cl.q1_players[ i ].ping = message.ReadShort();
}

static void CLQW_ParseUpdatePacketLossage( QMsg& message ) {
	int i = message.ReadByte();
	if ( i >= MAX_CLIENTS_QHW ) {
		common->Error( "CL_ParseServerMessage: qwsvc_updatepl > MAX_CLIENTS_QHW" );
	}
	cl.q1_players[ i ].pl = message.ReadByte();
}

static void CLQW_ParseUpdateEnterTime( QMsg& message ) {
	// time is sent over as seconds ago
	int i = message.ReadByte();
	if ( i >= MAX_CLIENTS_QHW ) {
		common->Error( "CL_ParseServerMessage: qwsvc_updateentertime > MAX_CLIENTS_QHW" );
	}
	cl.q1_players[ i ].entertime = cls.realtime * 0.001 - message.ReadFloat();
}

static void CLQ1_ParseParticleEffect( QMsg& message ) {
	vec3_t org;
	message.ReadPos( org );
	vec3_t dir;
	for ( int i = 0; i < 3; i++ ) {
		dir[ i ] = message.ReadChar() * ( 1.0 / 16 );
	}
	int count = message.ReadByte();
	int colour = message.ReadByte();

	if ( count == 255 ) {
		// rocket explosion
		CLQ1_ParticleExplosion( org );
	} else   {
		CLQ1_RunParticleEffect( org, dir, colour, count );
	}
}

static void CLQ1_ParseSignonNum( QMsg& message ) {
	int i = message.ReadByte();
	if ( i <= clc.qh_signon ) {
		common->Error( "Received signon %i when at %i", i, clc.qh_signon );
	}
	clc.qh_signon = i;
	CLQ1_SignonReply();
}

static void CLQW_SetStat( int stat, int value ) {
	if ( stat < 0 || stat >= MAX_CL_STATS ) {
		common->FatalError( "CLQW_SetStat: %i is invalid", stat );
	}

	if ( stat == QWSTAT_ITEMS ) {
		// set flash times
		for ( int j = 0; j < 32; j++ ) {
			if ( ( value & ( 1 << j ) ) && !( cl.qh_stats[ stat ] & ( 1 << j ) ) ) {
				cl.q1_item_gettime[ j ] = cl.qh_serverTimeFloat;
			}
		}
	}

	cl.qh_stats[ stat ] = value;
}

static void CLQW_ParseUpdateStat( QMsg& message ) {
	int i = message.ReadByte();
	int j = message.ReadByte();
	CLQW_SetStat( i, j );
}

static void CLQW_ParseUpdateStatLong( QMsg& message ) {
	int i = message.ReadByte();
	int j = message.ReadLong();
	CLQW_SetStat( i, j );
}

static void CLQ1_ParseCDTrack( QMsg& message ) {
	byte cdtrack = message.ReadByte();
	message.ReadByte();	//	looptrack

	if ( ( clc.demoplaying || clc.demorecording ) && ( cls.qh_forcetrack != -1 ) ) {
		CDAudio_Play( ( byte )cls.qh_forcetrack, true );
	} else   {
		CDAudio_Play( cdtrack, true );
	}
}

static void CLQ1_ParseIntermission() {
	cl.qh_intermission = 1;
	cl.qh_completed_time = cl.qh_serverTimeFloat;
}

static void CLQW_ParseIntermission( QMsg& message ) {
	cl.qh_intermission = 1;
	cl.qh_completed_time = cls.realtime * 0.001;
	message.ReadPos( cl.qh_simorg );
	for ( int i = 0; i < 3; i++ ) {
		cl.qh_simangles[ i ] = message.ReadAngle();
	}
	VectorCopy( vec3_origin, cl.qh_simvel );
}

static void CLQ1_ParseFinale( QMsg& message ) {
	cl.qh_intermission = 2;
	cl.qh_completed_time = cl.qh_serverTimeFloat;
	SCR_CenterPrint( message.ReadString2() );
}

static void CLQ1_ParseCutscene( QMsg& message ) {
	cl.qh_intermission = 3;
	cl.qh_completed_time = cl.qh_serverTimeFloat;
	SCR_CenterPrint( message.ReadString2() );
}

static void CLQW_MuzzleFlash( QMsg& message ) {
	int i = message.ReadShort();

	if ( ( unsigned )( i - 1 ) >= MAX_CLIENTS_QHW ) {
		return;
	}
	qwplayer_state_t* pl = &cl.qw_frames[ cl.qh_parsecount &  UPDATE_MASK_QW ].playerstate[ i - 1 ];
	CLQ1_MuzzleFlashLight( i, pl->origin, pl->viewangles );
}

static void CLQW_ProcessUserInfo( int slot, q1player_info_t* player ) {
	String::NCpy( player->name, Info_ValueForKey( player->userinfo, "name" ), sizeof ( player->name ) - 1 );
	player->topcolor = String::Atoi( Info_ValueForKey( player->userinfo, "topcolor" ) );
	player->bottomcolor = String::Atoi( Info_ValueForKey( player->userinfo, "bottomcolor" ) );
	if ( Info_ValueForKey( player->userinfo, "*spectator" )[ 0 ] ) {
		player->spectator = true;
	} else   {
		player->spectator = false;
	}

	if ( cls.state == CA_ACTIVE ) {
		CLQW_SkinFind( player );
	}

	CLQ1_TranslatePlayerSkin( slot );
}

static void CLQW_ParseUpdateUserinfo( QMsg& message ) {
	int slot = message.ReadByte();
	if ( slot >= MAX_CLIENTS_QHW ) {
		common->Error( "CL_ParseServerMessage: qwsvc_updateuserinfo > MAX_CLIENTS_QHW" );
	}

	q1player_info_t* player = &cl.q1_players[ slot ];
	player->userid = message.ReadLong();
	String::NCpy( player->userinfo, message.ReadString2(), sizeof ( player->userinfo ) - 1 );

	CLQW_ProcessUserInfo( slot, player );
}

static void CLQW_ParseSetInfo( QMsg& message ) {
	int slot = message.ReadByte();
	if ( slot >= MAX_CLIENTS_QHW ) {
		common->Error( "CL_ParseServerMessage: qwsvc_setinfo > MAX_CLIENTS_QHW" );
	}

	q1player_info_t* player = &cl.q1_players[ slot ];

	char key[ MAX_MSGLEN_QW ];
	String::NCpy( key, message.ReadString2(), sizeof ( key ) - 1 );
	key[ sizeof ( key ) - 1 ] = 0;
	char value[ MAX_MSGLEN_QW ];
	String::NCpy( value, message.ReadString2(), sizeof ( value ) - 1 );
	key[ sizeof ( value ) - 1 ] = 0;

	common->DPrintf( "SETINFO %s: %s=%s\n", player->name, key, value );

	if ( key[ 0 ] != '*' ) {
		Info_SetValueForKey( player->userinfo, key, value, MAX_INFO_STRING_QW, 64, 64,
			String::ICmp( key, "name" ) != 0, String::ICmp( key, "team" ) == 0 );
	}

	CLQW_ProcessUserInfo( slot, player );
}

static void CLQW_ParseServerInfo( QMsg& message ) {
	char key[ MAX_MSGLEN_QW ];
	String::NCpy( key, message.ReadString2(), sizeof ( key ) - 1 );
	key[ sizeof ( key ) - 1 ] = 0;
	char value[ MAX_MSGLEN_QW ];
	String::NCpy( value, message.ReadString2(), sizeof ( value ) - 1 );
	key[ sizeof ( value ) - 1 ] = 0;

	common->DPrintf( "SERVERINFO: %s=%s\n", key, value );

	if ( key[ 0 ] != '*' ) {
		Info_SetValueForKey( cl.qh_serverinfo, key, value, MAX_SERVERINFO_STRING, 64, 64,
			String::ICmp( key, "name" ) != 0, String::ICmp( key, "team" ) == 0 );
	}
}

// some preceding packets were choked
static void CLQW_ParseChokeCount( QMsg& message ) {
	int i = message.ReadByte();
	for ( int j = 0; j < i; j++ ) {
		cl.qw_frames[ ( clc.netchan.incomingAcknowledged - 1 - j ) & UPDATE_MASK_QW ].receivedtime = -2;
	}
}

static void CLQW_CalcModelChecksum( const char* modelName, const char* cvarName ) {
	idList<byte> buffer;
	if ( !FS_ReadFile( modelName, buffer ) ) {
		common->Error( "Couldn't load %s", modelName );
	}

	unsigned short crc;
	CRC_Init( &crc );
	for ( int i = 0; i < buffer.Num(); i++ ) {
		CRC_ProcessByte( &crc, buffer[ i ] );
	}

	char st[ 40 ];
	sprintf( st, "%d", ( int )crc );
	Info_SetValueForKey( cls.qh_userinfo, cvarName, st, MAX_INFO_STRING_QW, 64, 64, true, false );

	sprintf( st, "setinfo %s %d", cvarName, ( int )crc );
	CL_AddReliableCommand( st );
}

static void CLQW_Model_NextDownload() {
	if ( clc.downloadNumber == 0 ) {
		common->Printf( "Checking models...\n" );
		clc.downloadNumber = 1;
	}

	clc.downloadType = dl_model;
	for (
		; cl.qh_model_name[ clc.downloadNumber ][ 0 ]
		; clc.downloadNumber++ ) {
		const char* s = cl.qh_model_name[ clc.downloadNumber ];
		if ( s[ 0 ] == '*' ) {
			continue;	// inline brush model
		}
		if ( !CL_CheckOrDownloadFile( s ) ) {
			return;		// started a download
		}
	}

	CM_LoadMap( cl.qh_model_name[ 1 ], true, NULL );
	cl.model_clip[ 1 ] = 0;
	R_LoadWorld( cl.qh_model_name[ 1 ] );

	for ( int i = 2; i < MAX_MODELS_Q1; i++ ) {
		if ( !cl.qh_model_name[ i ][ 0 ] ) {
			break;
		}

		cl.model_draw[ i ] = R_RegisterModel( cl.qh_model_name[ i ] );
		if ( cl.qh_model_name[ i ][ 0 ] == '*' ) {
			cl.model_clip[ i ] = CM_InlineModel( String::Atoi( cl.qh_model_name[ i ] + 1 ) );
		}

		if ( !cl.model_draw[ i ] ) {
			common->Printf( "\nThe required model file '%s' could not be found or downloaded.\n\n",
				cl.qh_model_name[ i ] );
			common->Printf( "You may need to download or purchase a %s client "
							"pack in order to play on this server.\n\n", fs_gamedir );
			CL_Disconnect( true );
			return;
		}
	}

	CLQW_CalcModelChecksum( "progs/player.mdl", "pmodel" );
	CLQW_CalcModelChecksum( "progs/eyes.mdl", "emodel" );

	// all done
	R_EndRegistration();

	int CheckSum1;
	int CheckSum2;
	CM_MapChecksums( CheckSum1, CheckSum2 );

	// done with modellist, request first of static signon messages
	CL_AddReliableCommand( va( "prespawn %i 0 %i", cl.servercount, CheckSum2 ) );
}

static void CLQW_Sound_NextDownload() {
	if ( clc.downloadNumber == 0 ) {
		common->Printf( "Checking sounds...\n" );
		clc.downloadNumber = 1;
	}

	clc.downloadType = dl_sound;
	for (
		; cl.qh_sound_name[ clc.downloadNumber ][ 0 ]
		; clc.downloadNumber++ ) {
		char* s = cl.qh_sound_name[ clc.downloadNumber ];
		if ( !CL_CheckOrDownloadFile( va( "sound/%s",s ) ) ) {
			return;		// started a download
		}
	}

	S_BeginRegistration();
	CLQ1_InitTEnts();
	for ( int i = 1; i < MAX_SOUNDS_Q1; i++ ) {
		if ( !cl.qh_sound_name[ i ][ 0 ] ) {
			break;
		}
		cl.sound_precache[ i ] = S_RegisterSound( cl.qh_sound_name[ i ] );
	}
	S_EndRegistration();

	// done with sounds, request models now
	Com_Memset( cl.model_draw, 0, sizeof ( cl.model_draw ) );
	clq1_playerindex = -1;
	clq1_spikeindex = -1;
	clqw_flagindex = -1;
	CL_AddReliableCommand( va( "modellist %i %i", cl.servercount, 0 ) );
}

static void CLQW_RequestNextDownload() {
	switch ( clc.downloadType ) {
	case dl_single:
		break;
	case dl_skin:
		CLQW_SkinNextDownload();
		break;
	case dl_model:
		CLQW_Model_NextDownload();
		break;
	case dl_sound:
		CLQW_Sound_NextDownload();
		break;
	case dl_none:
	default:
		common->DPrintf( "Unknown download type.\n" );
	}
}

static void CLQW_ParseDownload( QMsg& message ) {
	// read the data
	int size = message.ReadShort();
	int percent = message.ReadByte();

	if ( clc.demoplaying ) {
		if ( size > 0 ) {
			message.readcount += size;
		}
		return;	// not in demo playback
	}

	if ( size == -1 ) {
		common->Printf( "File not found.\n" );
		if ( clc.download ) {
			common->Printf( "cls.download shouldn't have been set\n" );
			FS_FCloseFile( clc.download );
			clc.download = 0;
		}
		CLQW_RequestNextDownload();
		return;
	}

	// open the file if not opened yet
	if ( !clc.download ) {
		if ( String::NCmp( clc.downloadTempName, "skins/", 6 ) ) {
			clc.download = FS_FOpenFileWrite( clc.downloadTempName );
		} else   {
			char name[ 1024 ];
			sprintf( name, "qw/%s", clc.downloadTempName );
			clc.download = FS_SV_FOpenFileWrite( name );
		}

		if ( !clc.download ) {
			message.readcount += size;
			common->Printf( "Failed to open %s\n", clc.downloadTempName );
			CLQW_RequestNextDownload();
			return;
		}
	}

	FS_Write( message._data + message.readcount, size, clc.download );
	message.readcount += size;

	if ( percent != 100 ) {
		// change display routines by zoid
		// request next block
		clc.downloadPercent = percent;

		CL_AddReliableCommand( "nextdl" );
	} else   {
		FS_FCloseFile( clc.download );

		// rename the temp file to it's final name
		if ( String::Cmp( clc.downloadTempName, clc.downloadName ) ) {
			if ( String::NCmp( clc.downloadTempName,"skins/",6 ) ) {
				FS_Rename( clc.downloadTempName, clc.downloadName );
			} else   {
				char oldn[ MAX_OSPATH ];
				sprintf( oldn, "qw/%s", clc.downloadTempName );
				char newn[ MAX_OSPATH ];
				sprintf( newn, "qw/%s", clc.downloadName );
				FS_SV_Rename( oldn, newn );
			}
		}

		clc.download = 0;
		clc.downloadPercent = 0;

		// get another file if needed

		CLQW_RequestNextDownload();
	}
}

static void CLQW_ParseModelList( QMsg& message ) {
	// precache models and note certain default indexes
	int nummodels = message.ReadByte();

	for (;; ) {
		const char* str = message.ReadString2();
		if ( !str[ 0 ] ) {
			break;
		}
		nummodels++;
		if ( nummodels == MAX_MODELS_Q1 ) {
			common->Error( "Server sent too many model_precache" );
		}
		String::Cpy( cl.qh_model_name[ nummodels ], str );

		if ( !String::Cmp( cl.qh_model_name[ nummodels ],"progs/spike.mdl" ) ) {
			clq1_spikeindex = nummodels;
		}
		if ( !String::Cmp( cl.qh_model_name[ nummodels ],"progs/player.mdl" ) ) {
			clq1_playerindex = nummodels;
		}
		if ( !String::Cmp( cl.qh_model_name[ nummodels ],"progs/flag.mdl" ) ) {
			clqw_flagindex = nummodels;
		}
	}

	int n = message.ReadByte();

	if ( n ) {
		CL_AddReliableCommand( va( "modellist %i %i", cl.servercount, n ) );
		return;
	}

	clc.downloadNumber = 0;
	clc.downloadType = dl_model;
	CLQW_Model_NextDownload();
}

static void CLQW_ParseSoundList( QMsg& message ) {
	int numsounds = message.ReadByte();

	for (;; ) {
		const char* str = message.ReadString2();
		if ( !str[ 0 ] ) {
			break;
		}
		numsounds++;
		if ( numsounds == MAX_SOUNDS_Q1 ) {
			common->Error( "Server sent too many sound_precache" );
		}
		String::Cpy( cl.qh_sound_name[ numsounds ], str );
	}

	int n = message.ReadByte();

	if ( n ) {
		CL_AddReliableCommand( va( "soundlist %i %i", cl.servercount, n ) );
		return;
	}

	clc.downloadNumber = 0;
	clc.downloadType = dl_sound;
	CLQW_Sound_NextDownload();
}

static void CLQW_ParseSetPause( QMsg& message ) {
	cl.qh_paused = message.ReadByte();
	if ( cl.qh_paused ) {
		CDAudio_Pause();
	} else   {
		CDAudio_Resume();
	}
}

static void CLQ1_ParseServerInfo( QMsg& message ) {
	common->DPrintf( "Serverinfo packet received.\n" );

	R_Shutdown( false );
	CL_InitRenderer();

	clc.qh_signon = 0;

	//
	// wipe the clientActive_t struct
	//
	CL_ClearState();

	SCR_ClearCenterString();

	// parse protocol version number
	int i = message.ReadLong();
	if ( i != Q1PROTOCOL_VERSION ) {
		common->Printf( "Server returned version %i, not %i", i, Q1PROTOCOL_VERSION );
		return;
	}

	// parse maxclients
	cl.qh_maxclients = message.ReadByte();
	if ( cl.qh_maxclients < 1 || cl.qh_maxclients > MAX_CLIENTS_QH ) {
		common->Printf( "Bad maxclients (%u) from server\n", cl.qh_maxclients );
		return;
	}

	// parse gametype
	cl.qh_gametype = message.ReadByte();

	// parse signon message
	const char* str = message.ReadString2();
	String::NCpy( cl.qh_levelname, str, sizeof ( cl.qh_levelname ) - 1 );

	// seperate the printfs so the server message can have a color
	common->Printf( "\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n" );
	common->Printf( S_COLOR_ORANGE "%s" S_COLOR_WHITE "\n", str );

	// precache models
	Com_Memset( cl.model_draw, 0, sizeof ( cl.model_draw ) );
	char model_precache[ MAX_MODELS_Q1 ][ MAX_QPATH ];
	int nummodels;
	for ( nummodels = 1;; nummodels++ ) {
		str = message.ReadString2();
		if ( !str[ 0 ] ) {
			break;
		}
		if ( nummodels == MAX_MODELS_Q1 ) {
			common->Printf( "Server sent too many model precaches\n" );
			return;
		}
		String::Cpy( model_precache[ nummodels ], str );
	}

	// precache sounds
	Com_Memset( cl.sound_precache, 0, sizeof ( cl.sound_precache ) );
	char sound_precache[ MAX_SOUNDS_Q1 ][ MAX_QPATH ];
	int numsounds;
	for ( numsounds = 1;; numsounds++ ) {
		str = message.ReadString2();
		if ( !str[ 0 ] ) {
			break;
		}
		if ( numsounds == MAX_SOUNDS_Q1 ) {
			common->Printf( "Server sent too many sound precaches\n" );
			return;
		}
		String::Cpy( sound_precache[ numsounds ], str );
	}

	//
	// now we try to load everything else until a cache allocation fails
	//

	CM_LoadMap( model_precache[ 1 ], true, NULL );
	R_LoadWorld( model_precache[ 1 ] );
	CLQH_KeepaliveMessage();

	for ( i = 2; i < nummodels; i++ ) {
		cl.model_draw[ i ] = R_RegisterModel( model_precache[ i ] );
		if ( cl.model_draw[ i ] == 0 ) {
			common->Printf( "Model %s not found\n", model_precache[ i ] );
			return;
		}
		CLQH_KeepaliveMessage();
	}

	S_BeginRegistration();
	CLQ1_InitTEnts();
	for ( i = 1; i < numsounds; i++ ) {
		cl.sound_precache[ i ] = S_RegisterSound( sound_precache[ i ] );
		CLQH_KeepaliveMessage();
	}
	S_EndRegistration();

	// local state
	R_EndRegistration();
}

static void CLQW_ParseServerData( QMsg& message ) {
	common->DPrintf( "Serverdata packet received.\n" );

	Cbuf_Execute();			// make sure any stuffed commands are done

	R_Shutdown( false );
	CL_InitRenderer();

	//
	// wipe the clientActive_t struct
	//
	CL_ClearState();

	// parse protocol version number
	// allow 2.2 and 2.29 demos to play
	int protover = message.ReadLong();
	if ( protover != QWPROTOCOL_VERSION &&
		 !( clc.demoplaying && ( protover == 26 || protover == 27 || protover == 28 ) ) ) {
		common->Error( "Server returned version %i, not %i\nYou probably need to upgrade.\nCheck http://www.quakeworld.net/", protover, QWPROTOCOL_VERSION );
	}

	cl.servercount = message.ReadLong();

	// game directory
	const char* str = message.ReadString2();

	bool cflag = false;
	if ( String::ICmp( fsqhw_gamedirfile, str ) ) {
		// save current config
		Com_WriteConfiguration();
		cflag = true;
	}

	FS_SetGamedirQHW( str );

	//ZOID--run the autoexec.cfg in the gamedir
	//if it exists
	if ( cflag ) {
		if ( FS_FileExists( "config.cfg" ) ) {
			Cbuf_AddText( "cl_warncmd 0\n" );
			Cbuf_AddText( "exec config.cfg\n" );
			Cbuf_AddText( "exec frontend.cfg\n" );
			Cbuf_AddText( "cl_warncmd 1\n" );
		}
	}

	// parse player slot, high bit means spectator
	cl.playernum = message.ReadByte();
	if ( cl.playernum & 128 ) {
		cl.qh_spectator = true;
		cl.playernum &= ~128;
	}
	cl.viewentity = cl.playernum + 1;

	// get the full level name
	str = const_cast<char*>( message.ReadString2() );
	String::NCpy( cl.qh_levelname, str, sizeof ( cl.qh_levelname ) - 1 );

	// get the movevars
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

	// seperate the printfs so the server message can have a color
	common->Printf( "\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n" );
	common->Printf( S_COLOR_ORANGE "%s" S_COLOR_WHITE "\n", str );

	// ask for the sound list next
	Com_Memset( cl.qh_sound_name, 0, sizeof ( cl.qh_sound_name ) );
	CL_AddReliableCommand( va( "soundlist %i %i", cl.servercount, 0 ) );

	// now waiting for downloads, etc
	cls.state = CA_LOADING;
}

void CLQ1_ParseServerMessage( QMsg& message ) {
	if ( cl_shownet->value == 1 ) {
		common->Printf( "%i ", message.cursize );
	} else if ( cl_shownet->value == 2 )     {
		common->Printf( "------------------\n" );
	}

	cl.qh_onground = false;	// unless the server says otherwise
	//
	// parse the message
	//
	message.BeginReadingOOB();

	while ( 1 ) {
		if ( message.badread ) {
			common->Error( "CLQ1_ParseServerMessage: Bad server message" );
		}

		int cmd = message.ReadByte();

		if ( cmd == -1 ) {
			SHOWNET( message, "END OF MESSAGE" );
			return;		// end of message
		}

		// if the high bit of the command byte is set, it is a fast update
		if ( cmd & Q1U_SIGNAL ) {
			SHOWNET( message, "fast update" );
			CLQ1_ParseUpdate( message, cmd & 127 );
			continue;
		}

		SHOWNET( message, svcq1_strings[ cmd ] );

		// other commands
		switch ( cmd ) {
		default:
			common->Error( "CLQ1_ParseServerMessage: Illegible server message\n" );
			break;
		case q1svc_nop:
			break;
		case q1svc_time:
			CLQH_ParseTime( message );
			break;
		case q1svc_clientdata:
			CLQ1_ParseClientdata( message );
			break;
		case q1svc_version:
			CLQ1_ParseVersion( message );
			break;
		case q1svc_disconnect:
			CLQH_ParseDisconnect();
			break;
		case q1svc_print:
			CLQ1_ParsePrint( message );
			break;
		case q1svc_centerprint:
			CL_ParseCenterPrint( message );
			break;
		case q1svc_stufftext:
			CL_ParseStuffText( message );
			break;
		case q1svc_damage:
			VQH_ParseDamage( message );
			break;
		case q1svc_serverinfo:
			CLQ1_ParseServerInfo( message );
			break;
		case q1svc_setangle:
			CLQH_ParseSetAngle( message );
			break;
		case q1svc_setview:
			CLQH_ParseSetView( message );
			break;
		case q1svc_lightstyle:
			CLQH_ParseLightStyle( message );
			break;
		case q1svc_sound:
			CLQ1_ParseStartSoundPacket( message );
			break;
		case q1svc_stopsound:
			CLQH_ParseStopSound( message );
			break;
		case q1svc_updatename:
			CLQ1_UpdateName( message );
			break;
		case q1svc_updatefrags:
			CLQ1_ParseUpdateFrags( message );
			break;
		case q1svc_updatecolors:
			CLQ1_ParseUpdateColours( message );
			break;
		case q1svc_particle:
			CLQ1_ParseParticleEffect( message );
			break;
		case q1svc_spawnbaseline:
			CLQ1_ParseSpawnBaseline( message );
			break;
		case q1svc_spawnstatic:
			CLQ1_ParseSpawnStatic( message );
			break;
		case q1svc_temp_entity:
			CLQ1_ParseTEnt( message );
			break;
		case q1svc_setpause:
			CLQH_ParseSetPause( message );
			break;
		case q1svc_signonnum:
			CLQ1_ParseSignonNum( message );
			break;
		case q1svc_killedmonster:
			CLQH_ParseKilledMonster();
			break;
		case q1svc_foundsecret:
			CLQH_ParseFoundSecret();
			break;
		case q1svc_updatestat:
			CLQH_ParseUpdateStat( message );
			break;
		case q1svc_spawnstaticsound:
			CLQH_ParseStaticSound( message );
			break;
		case q1svc_cdtrack:
			CLQ1_ParseCDTrack( message );
			break;
		case q1svc_intermission:
			CLQ1_ParseIntermission();
			break;
		case q1svc_finale:
			CLQ1_ParseFinale( message );
			break;
		case q1svc_cutscene:
			CLQ1_ParseCutscene( message );
			break;
		case q1svc_sellscreen:
			CLQH_ParseSellScreen();
			break;
		}
	}
}

void CLQW_ParseServerMessage( QMsg& message ) {
	CLQ1_ClearProjectiles();

	//
	// if recording demos, copy the message out
	//
	if ( cl_shownet->value == 1 ) {
		common->Printf( "%i ",message.cursize );
	} else if ( cl_shownet->value == 2 )     {
		common->Printf( "------------------\n" );
	}

	CLQW_ParseClientdata();

	//
	// parse the message
	//
	while ( 1 ) {
		if ( message.badread ) {
			common->Error( "CLQW_ParseServerMessage: Bad server message" );
			break;
		}

		int cmd = message.ReadByte();

		if ( cmd == -1 ) {
			message.readcount++;	// so the EOM showner has the right value
			SHOWNET( message, "END OF MESSAGE" );
			break;
		}

		SHOWNET( message, svcqw_strings[ cmd ] );

		// other commands
		switch ( cmd ) {
		default:
			common->Error( "CLQW_ParseServerMessage: Illegible server message" );
			break;
		case q1svc_nop:
			break;
		case q1svc_disconnect:
			CLQW_ParseDisconnect();
			break;
		case q1svc_print:
			CLQW_ParsePrint( message );
			break;
		case q1svc_centerprint:
			CL_ParseCenterPrint( message );
			break;
		case q1svc_stufftext:
			CL_ParseStuffText( message );
			break;
		case q1svc_damage:
			VQH_ParseDamage( message );
			break;
		case qwsvc_serverdata:
			CLQW_ParseServerData( message );
			break;
		case q1svc_setangle:
			CLQH_ParseSetAngle( message );
			break;
		case q1svc_lightstyle:
			CLQH_ParseLightStyle( message );
			break;
		case q1svc_sound:
			CLQW_ParseStartSoundPacket( message );
			break;
		case q1svc_stopsound:
			CLQH_ParseStopSound( message );
			break;
		case q1svc_updatefrags:
			CLQW_ParseUpdateFrags( message );
			break;
		case qwsvc_updateping:
			CLQW_ParseUpdatePing( message );
			break;
		case qwsvc_updatepl:
			CLQW_ParseUpdatePacketLossage( message );
			break;
		case qwsvc_updateentertime:
			CLQW_ParseUpdateEnterTime( message );
			break;
		case q1svc_spawnbaseline:
			CLQ1_ParseSpawnBaseline( message );
			break;
		case q1svc_spawnstatic:
			CLQ1_ParseSpawnStatic( message );
			break;
		case q1svc_temp_entity:
			CLQW_ParseTEnt( message );
			break;
		case q1svc_killedmonster:
			CLQH_ParseKilledMonster();
			break;
		case q1svc_foundsecret:
			CLQH_ParseFoundSecret();
			break;
		case q1svc_updatestat:
			CLQW_ParseUpdateStat( message );
			break;
		case qwsvc_updatestatlong:
			CLQW_ParseUpdateStatLong( message );
			break;
		case q1svc_spawnstaticsound:
			CLQH_ParseStaticSound( message );
			break;
		case q1svc_cdtrack:
			CLQHW_ParseCDTrack( message );
			break;
		case q1svc_intermission:
			CLQW_ParseIntermission( message );
			break;
		case q1svc_finale:
			CLQHW_ParseFinale( message );
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
			CLQW_MuzzleFlash( message );
			break;
		case qwsvc_updateuserinfo:
			CLQW_ParseUpdateUserinfo( message );
			break;
		case qwsvc_setinfo:
			CLQW_ParseSetInfo( message );
			break;
		case qwsvc_serverinfo:
			CLQW_ParseServerInfo( message );
			break;
		case qwsvc_download:
			CLQW_ParseDownload( message );
			break;
		case qwsvc_playerinfo:
			CLQW_ParsePlayerinfo( message );
			break;
		case qwsvc_nails:
			CLQW_ParseNails( message );
			break;
		case qwsvc_chokecount:
			CLQW_ParseChokeCount( message );
			break;
		case qwsvc_modellist:
			CLQW_ParseModelList( message );
			break;
		case qwsvc_soundlist:
			CLQW_ParseSoundList( message );
			break;
		case qwsvc_packetentities:
			CLQW_ParsePacketEntities( message );
			break;
		case qwsvc_deltapacketentities:
			CLQW_ParseDeltaPacketEntities( message );
			break;
		case qwsvc_maxspeed:
			CLQHW_ParseMaxSpeed( message );
			break;
		case qwsvc_entgravity:
			CLQHW_ParseEntGravity( message );
			break;
		case q1svc_setpause:
			CLQW_ParseSetPause( message );
			break;
		}
	}

	CLQW_SetSolidEntities();
}
