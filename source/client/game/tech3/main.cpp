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
#include "../../system.h"
#include "../../translate.h"
#include "../../cinematic/public.h"
#include "../../ui/ui.h"
#include "../quake3/local.h"
#include "../wolfsp/local.h"
#include "../wolfmp/local.h"
#include "../et/local.h"
#include "../et/dl_public.h"
#include "../../../common/Common.h"
#include "../../../server/public.h"
#include "../../../common/common_defs.h"
#include "../../../common/strings.h"

Cvar* clet_profile;
Cvar* clt3_showServerCommands;
Cvar* clt3_showTimeDelta;
Cvar* clt3_activeAction;
Cvar* clwm_shownuments;			// DHM - Nerve
Cvar* clet_autorecord;
Cvar* clt3_maxpackets;
Cvar* clt3_showSend;
Cvar* clt3_packetdup;
Cvar* clt3_allowDownload;
Cvar* clt3_motd;
Cvar* clwm_wavefilerecord;
Cvar* clt3_timeNudge;
Cvar* clt3_freezeDemo;
Cvar* clt3_avidemo;
Cvar* clt3_forceavidemo;

void CLET_PurgeCache() {
	cls.et_doCachePurge = true;
}

void CLET_DoPurgeCache() {
	if ( !cls.et_doCachePurge ) {
		return;
	}

	cls.et_doCachePurge = false;

	if ( !com_cl_running ) {
		return;
	}

	if ( !com_cl_running->integer ) {
		return;
	}

	if ( !cls.q3_rendererStarted ) {
		return;
	}

	R_PurgeCache();
}

void CLET_WavRecord_f() {
	if ( clc.wm_wavefile ) {
		common->Printf( "Already recording a wav file\n" );
		return;
	}

	CL_WriteWaveOpen();
}

void CLET_WavStopRecord_f() {
	if ( !clc.wm_wavefile ) {
		common->Printf( "Not recording a wav file\n" );
		return;
	}

	CL_WriteWaveClose();
	Cvar_Set( "cl_waverecording", "0" );
	Cvar_Set( "cl_wavefilename", "" );
	Cvar_Set( "cl_waveoffset", "0" );
	clc.wm_waverecording = false;
}

//	After the server has cleared the hunk, these will need to be restarted
// This is the only place that any of these functions are called from
void CLT3_StartHunkUsers() {
	if ( !com_cl_running ) {
		return;
	}

	if ( !com_cl_running->integer ) {
		return;
	}

	if ( !cls.q3_rendererStarted ) {
		cls.q3_rendererStarted = true;
		CL_InitRenderer();
	}

	if ( !cls.q3_soundStarted ) {
		cls.q3_soundStarted = true;
		S_Init();
	}

	if ( !cls.q3_soundRegistered ) {
		cls.q3_soundRegistered = true;
		S_BeginRegistration();
	}

	if ( !cls.q3_uiStarted ) {
		cls.q3_uiStarted = true;
		CLT3_InitUI();
	}
}

void CLT3_ShutdownAll() {
	// clear sounds
	S_DisableSounds();
	if ( GGameType & GAME_ET ) {
		// download subsystem
		DL_Shutdown();
	}
	// shutdown CGame
	CLT3_ShutdownCGame();
	// shutdown UI
	CLT3_ShutdownUI();

	// shutdown the renderer
	R_Shutdown( false );			// don't destroy window or context

	if ( GGameType & GAME_ET ) {
		CLET_DoPurgeCache();
	}

	cls.q3_uiStarted = false;
	cls.q3_cgameStarted = false;
	cls.q3_rendererStarted = false;
	cls.q3_soundRegistered = false;

	if ( GGameType & GAME_ET ) {
		// Gordon: stop recording on map change etc, demos aren't valid over map changes anyway
		if ( clc.demorecording ) {
			CLT3_StopRecord_f();
		}

		if ( clc.wm_waverecording ) {
			CLET_WavStopRecord_f();
		}
	}
}

//	Called by CLT3_MapLoading, CLT3_Connect_f, CLT3_PlayDemo_f, and CLT3_ParseGamestate the only
// ways a client gets into a game
// Also called by Com_Error
void CLT3_FlushMemory() {
	// shutdown all the client stuff
	CLT3_ShutdownAll();

	CIN_CloseAllVideos();

	// if not running a server clear the whole hunk
	if ( GGameType & ( GAME_WolfMP | GAME_ET ) && !com_sv_running->integer ) {
		// clear collision map data
		CM_ClearMap();
	}

	CL_StartHunkUsers();
}

void CLT3_ShutdownRef() {
	R_Shutdown( true );
}

void CLT3_InitRef() {
	common->Printf( "----- Initializing Renderer ----\n" );

	BotDrawDebugPolygonsFunc = BotDrawDebugPolygons;

	common->Printf( "-------------------------------\n" );

	// unpause so the cgame definately gets a snapshot and renders a frame
	Cvar_Set( "cl_paused", "0" );
}

static void CLT3_ResetPureClientAtServer() {
	CL_AddReliableCommand( va( "vdr" ) );
}

//	Restart the video subsystem
//
//	we also have to reload the UI and CGame because the renderer
// doesn't know what graphics to reload
void CLT3_Vid_Restart_f() {
	// don't let them loop during the restart
	S_StopAllSounds();
	// shutdown the UI
	CLT3_ShutdownUI();
	// shutdown the CGame
	CLT3_ShutdownCGame();
	// shutdown the renderer and clear the renderer interface
	CLT3_ShutdownRef();
	// client is no longer pure untill new checksums are sent
	CLT3_ResetPureClientAtServer();
	// clear pak references
	FS_ClearPakReferences( FS_UI_REF | FS_CGAME_REF );
	// reinitialize the filesystem if the game directory or checksum has changed
	FS_ConditionalRestart( clc.q3_checksumFeed );

	if ( !( GGameType & GAME_Quake3 ) ) {
		S_BeginRegistration();	// all sound handles are now invalid
	}

	cls.q3_rendererStarted = false;
	cls.q3_uiStarted = false;
	cls.q3_cgameStarted = false;
	cls.q3_soundRegistered = false;

	// unpause so the cgame definately gets a snapshot and renders a frame
	Cvar_Set( "cl_paused", "0" );

	CIN_CloseAllVideos();

	// initialize the renderer interface
	CLT3_InitRef();

	// startup all the client stuff
	CL_StartHunkUsers();

#ifdef _WIN32
	if ( GGameType & GAME_ET ) {
		In_Restart_f();	// fretn
	}
#endif
	// start the cgame if connected
	if ( cls.state > CA_CONNECTED && cls.state != CA_CINEMATIC ) {
		cls.q3_cgameStarted = true;
		CLT3_InitCGame();
		// send pure checksums
		CLT3_SendPureChecksums();
	}

	if ( GGameType & GAME_WolfSP ) {
		// start music if there was any

		vmCvar_t musicCvar;
		Cvar_Register( &musicCvar, "s_currentMusic", "", CVAR_ROM );
		if ( String::Length( musicCvar.string ) ) {
			S_StartBackgroundTrack( musicCvar.string, musicCvar.string, 1000 );
		}

		// fade up volume
		S_FadeAllSounds( 1, 0, false );
	}
}

//	Restart the sound subsystem
// The cgame and game must also be forced to restart because
// handles will be invalid
static void CLT3_Snd_Restart_f() {
	S_Shutdown();
	S_Init();

	CLT3_Vid_Restart_f();
}

//	Restart the ui subsystem
static void CLT3_UI_Restart_f() {
	// shutdown the UI
	CLT3_ShutdownUI();

	// init the UI
	CLT3_InitUI();
}

static void CLT3_OpenedPK3List_f() {
	common->Printf( "Opened PK3 Names: %s\n", FS_LoadedPakNames() );
}

static void CLT3_ReferencedPK3List_f() {
	common->Printf( "Referenced PK3 Names: %s\n", FS_ReferencedPakNames() );
}

static void CLT3_Configstrings_f() {
	int i;
	int ofs;

	if ( cls.state != CA_ACTIVE ) {
		common->Printf( "Not connected to a server.\n" );
		return;
	}

	if ( GGameType & GAME_WolfSP ) {
		for ( i = 0; i < MAX_CONFIGSTRINGS_WS; i++ ) {
			ofs = cl.ws_gameState.stringOffsets[ i ];
			if ( !ofs ) {
				continue;
			}
			common->Printf( "%4i: %s\n", i, cl.ws_gameState.stringData + ofs );
		}
	} else if ( GGameType & GAME_WolfMP ) {
		for ( i = 0; i < MAX_CONFIGSTRINGS_WM; i++ ) {
			ofs = cl.wm_gameState.stringOffsets[ i ];
			if ( !ofs ) {
				continue;
			}
			common->Printf( "%4i: %s\n", i, cl.wm_gameState.stringData + ofs );
		}
	} else if ( GGameType & GAME_ET ) {
		for ( i = 0; i < MAX_CONFIGSTRINGS_ET; i++ ) {
			ofs = cl.et_gameState.stringOffsets[ i ];
			if ( !ofs ) {
				continue;
			}
			common->Printf( "%4i: %s\n", i, cl.et_gameState.stringData + ofs );
		}
	} else {
		for ( i = 0; i < MAX_CONFIGSTRINGS_Q3; i++ ) {
			ofs = cl.q3_gameState.stringOffsets[ i ];
			if ( !ofs ) {
				continue;
			}
			common->Printf( "%4i: %s\n", i, cl.q3_gameState.stringData + ofs );
		}
	}
}

static void CLT3_Clientinfo_f() {
	common->Printf( "--------- Client Information ---------\n" );
	common->Printf( "state: %i\n", cls.state );
	common->Printf( "Server: %s\n", cls.servername );
	common->Printf( "User info settings:\n" );
	Info_Print( Cvar_InfoString( CVAR_USERINFO, MAX_INFO_STRING_Q3 ) );
	common->Printf( "--------------------------------------\n" );
}

static void CLT3_ShowIP_f() {
	SOCK_ShowIP();
}

static void CLQ3_SetModel_f() {
	char* arg = Cmd_Argv( 1 );
	if ( arg[ 0 ] ) {
		Cvar_Set( "model", arg );
		Cvar_Set( "headmodel", arg );
	} else {
		char name[ 256 ];
		Cvar_VariableStringBuffer( "model", name, sizeof ( name ) );
		common->Printf( "model is set to %s\n", name );
	}
}

// Ridah, startup-caching system
struct cacheItem_t {
	char name[ MAX_QPATH ];
	int hits;
	int lastSetIndex;
};

enum
{
	CACHE_SOUNDS,
	CACHE_MODELS,
	CACHE_IMAGES,

	CACHE_NUMGROUPS
};

static cacheItem_t cacheGroups[ CACHE_NUMGROUPS ] =
{
	{{'s','o','u','n','d',0}, CACHE_SOUNDS},
	{{'m','o','d','e','l',0}, CACHE_MODELS},
	{{'i','m','a','g','e',0}, CACHE_IMAGES},
};
#define MAX_CACHE_ITEMS     4096
#define CACHE_HIT_RATIO     0.75		// if hit on this percentage of maps, it'll get cached

static int cacheIndex;
static cacheItem_t cacheItems[ CACHE_NUMGROUPS ][ MAX_CACHE_ITEMS ];

static void CL_Cache_StartGather_f() {
	cacheIndex = 0;
	memset( cacheItems, 0, sizeof ( cacheItems ) );

	Cvar_Set( "cl_cacheGathering", "1" );
}

static void CL_Cache_UsedFile_f() {
	if ( Cmd_Argc() < 2 ) {
		common->Error( "usedfile without enough parameters\n" );
		return;
	}

	char groupStr[ MAX_QPATH ];
	String::Cpy( groupStr, Cmd_Argv( 1 ) );

	char itemStr[ MAX_QPATH ];
	String::Cpy( itemStr, Cmd_Argv( 2 ) );
	for ( int i = 3; i < Cmd_Argc(); i++ ) {
		String::Cat( itemStr, sizeof ( itemStr ), " " );
		String::Cat( itemStr, sizeof ( itemStr ), Cmd_Argv( i ) );
	}
	String::ToLower( itemStr );

	// find the cache group
	int group;
	for ( group = 0; group < CACHE_NUMGROUPS; group++ ) {
		if ( !String::NCmp( groupStr, cacheGroups[ group ].name, MAX_QPATH ) ) {
			break;
		}
	}
	if ( group == CACHE_NUMGROUPS ) {
		common->Error( "usedfile without a valid cache group\n" );
		return;
	}

	// see if it's already there
	cacheItem_t* item = cacheItems[ group ];
	for ( int i = 0; i < MAX_CACHE_ITEMS; i++, item++ ) {
		if ( !item->name[ 0 ] ) {
			// didn't find it, so add it here
			String::NCpyZ( item->name, itemStr, MAX_QPATH );
			if ( cacheIndex > 9999 ) {		// hack, but yeh
				item->hits = cacheIndex;
			} else {
				item->hits++;
			}
			item->lastSetIndex = cacheIndex;
			break;
		}
		if ( item->name[ 0 ] == itemStr[ 0 ] && !String::NCmp( item->name, itemStr, MAX_QPATH ) ) {
			if ( item->lastSetIndex != cacheIndex ) {
				item->hits++;
				item->lastSetIndex = cacheIndex;
			}
			break;
		}
	}
}

static void CL_Cache_SetIndex_f( void ) {
	if ( Cmd_Argc() < 2 ) {
		common->Error( "setindex needs an index\n" );
		return;
	}

	cacheIndex = String::Atoi( Cmd_Argv( 1 ) );
}

static void CL_Cache_MapChange_f( void ) {
	cacheIndex++;
}

static void CL_Cache_EndGather_f( void ) {
	// save the frequently used files to the cache list file

	int cachePass = ( int )floor( ( float )cacheIndex * CACHE_HIT_RATIO );

	for ( int i = 0; i < CACHE_NUMGROUPS; i++ ) {
		char filename[ MAX_QPATH ];
		String::NCpyZ( filename, cacheGroups[ i ].name, MAX_QPATH );
		String::Cat( filename, MAX_QPATH, ".cache" );

		int handle = FS_FOpenFileWrite( filename );

		for ( int j = 0; j < MAX_CACHE_ITEMS; j++ ) {
			// if it's a valid filename, and it's been hit enough times, cache it
			if ( cacheItems[ i ][ j ].hits >= cachePass && strstr( cacheItems[ i ][ j ].name, "/" ) ) {
				FS_Write( cacheItems[ i ][ j ].name, String::Length( cacheItems[ i ][ j ].name ), handle );
				FS_Write( "\n", 1, handle );
			}
		}

		FS_FCloseFile( handle );
	}

	Cvar_Set( "cl_cacheGathering", "0" );
}

// RF, trap manual client damage commands so users can't issue them manually
static void CLWS_ClientDamageCommand() {
	// do nothing
}

static void CLWS_startMultiplayer_f() {
#ifdef __MACOS__	//DAJ
	Sys_StartProcess( "Wolfenstein MP", true );
#elif defined( __linux__ )
	Sys_StartProcess( "./wolf.x86", true );
#else
	Sys_StartProcess( "WolfMP.exe", true );
#endif
}

//	only supporting "open" syntax for URL openings, others are not portable or need to be added on a case-by-case basis
// the shellExecute syntax as been kept to remain compatible with win32 SP demo pk3, but this thing only does open URL
static void CLWS_ShellExecute_URL_f() {
	common->DPrintf( "CLWS_ShellExecute_URL_f\n" );

	if ( String::ICmp( Cmd_Argv( 1 ),"open" ) ) {
		common->DPrintf( "invalid CLWS_ShellExecute_URL_f syntax (shellExecute \"open\" <url> <doExit>)\n" );
		return;
	}

	bool doexit;
	if ( Cmd_Argc() < 4 ) {
		doexit = true;
	} else {
		doexit = ( qboolean )( String::Atoi( Cmd_Argv( 3 ) ) );
	}

	Sys_OpenURL( Cmd_Argv( 2 ),doexit );
}

static void CLWS_MapRestart_f() {
	if ( !com_cl_running ) {
		return;
	}
	if ( !com_cl_running->integer ) {
		return;
	}
	common->Printf( "This command is no longer functional.\nUse \"loadgame current\" to load the current map." );
}

static void CLWM_startSingleplayer_f() {
#if defined( __linux__ )
	Sys_StartProcess( "./wolfsp.x86", true );
#else
	Sys_StartProcess( "WolfSP.exe", true );
#endif
}

static void CLWM_buyNow_f() {
	Sys_OpenURL( "http://www.activision.com/games/wolfenstein/purchase.html", true );
}

static void CLWM_singlePlayLink_f() {
	Sys_OpenURL( "http://www.activision.com/games/wolfenstein/home.html", true );
}

//	Eat misc console commands to prevent exploits
static void CLET_EatMe_f() {
	//do nothing kthxbye
}

static void CLT3_SetRecommended_f() {
	if ( GGameType & GAME_WolfSP && Cmd_Argc() > 1 ) {
		Com_SetRecommended( true );
	} else {
		Com_SetRecommended( false );
	}
}

void CLT3_Init() {
	CL_ClearState();

	cls.realtime = 0;

	CLT3_InitServerLists();

	//
	// register our variables
	//
	clt3_motd = Cvar_Get( "cl_motd", "1", 0 );
	clt3_showServerCommands = Cvar_Get( "cl_showServerCommands", "0", 0 );
	clt3_showSend = Cvar_Get( "cl_showSend", "0", CVAR_TEMP );
	clt3_showTimeDelta = Cvar_Get( "cl_showTimeDelta", "0", CVAR_TEMP );
	clt3_activeAction = Cvar_Get( "activeAction", "", CVAR_TEMP );
	clt3_maxpackets = Cvar_Get( "cl_maxpackets", "30", CVAR_ARCHIVE );
	clt3_packetdup = Cvar_Get( "cl_packetdup", "1", CVAR_ARCHIVE );
	clt3_allowDownload = Cvar_Get( "cl_allowDownload", GGameType & GAME_ET ? "1" : "0", CVAR_ARCHIVE );
	cl_timeout = Cvar_Get( "cl_timeout", "200", 0 );
	clt3_timeNudge = Cvar_Get( "cl_timeNudge", "0", CVAR_TEMP );
	clt3_freezeDemo = Cvar_Get( "cl_freezeDemo", "0", CVAR_TEMP );
	clt3_avidemo = Cvar_Get( "cl_avidemo", "0", 0 );
	clt3_forceavidemo = Cvar_Get( "cl_forceavidemo", "0", 0 );

	// init autoswitch so the ui will have it correctly even
	// if the cgame hasn't been started
	// -NERVE - SMF - disabled autoswitch by default
	Cvar_Get( "cg_autoswitch", GGameType & GAME_Quake3 ? "1" : GGameType & GAME_WolfSP ? "2" : "0", CVAR_ARCHIVE );

	Cvar_Get( "cl_motdString", "", CVAR_ROM );
	Cvar_Get( "cl_maxPing", "800", CVAR_ARCHIVE );

	// cgame might not be initialized before menu is used
	Cvar_Get( "cg_viewsize", "100", CVAR_ARCHIVE );

	if ( !( GGameType & GAME_Quake3 ) ) {
		// Rafael - particle switch
		Cvar_Get( "cg_wolfparticles", "1", CVAR_ARCHIVE );
	}
	if ( GGameType & ( GAME_WolfMP | GAME_ET ) ) {
		clwm_wavefilerecord = Cvar_Get( "cl_wavefilerecord", "0", CVAR_TEMP );
		clwm_shownuments = Cvar_Get( "cl_shownuments", "0", CVAR_TEMP );
		Cvar_Get( "cl_visibleClients", "0", CVAR_TEMP );

		Cvar_Get( "cg_drawCompass", "1", CVAR_ARCHIVE );
		Cvar_Get( "cg_drawNotifyText", "1", CVAR_ARCHIVE );
		Cvar_Get( "cg_quickMessageAlt", "1", CVAR_ARCHIVE );
		Cvar_Get( "cg_popupLimboMenu", "1", CVAR_ARCHIVE );
		Cvar_Get( "cg_descriptiveText", "1", CVAR_ARCHIVE );
		Cvar_Get( "cg_drawTeamOverlay", "2", CVAR_ARCHIVE );
		Cvar_Get( "cg_drawGun", "1", CVAR_ARCHIVE );
		Cvar_Get( "cg_cursorHints", "1", CVAR_ARCHIVE );
		Cvar_Get( "cg_voiceSpriteTime", "6000", CVAR_ARCHIVE );
		Cvar_Get( "cg_crosshairSize", "48", CVAR_ARCHIVE );
		Cvar_Get( "cg_drawCrosshair", "1", CVAR_ARCHIVE );
		Cvar_Get( "cg_zoomDefaultSniper", "20", CVAR_ARCHIVE );
		Cvar_Get( "cg_zoomstepsniper", "2", CVAR_ARCHIVE );
	}
	if ( GGameType & GAME_WolfMP ) {
		clwm_missionStats = Cvar_Get( "g_missionStats", "0", CVAR_ROM );
		clwm_waitForFire = Cvar_Get( "cl_waitForFire", "0", CVAR_ROM );

		Cvar_Get( "cg_uselessNostalgia", "0", CVAR_ARCHIVE );		// JPW NERVE
		Cvar_Get( "cg_teamChatsOnly", "0", CVAR_ARCHIVE );
		Cvar_Get( "cg_noVoiceChats", "0", CVAR_ARCHIVE );
		Cvar_Get( "cg_noVoiceText", "0", CVAR_ARCHIVE );

		Cvar_Get( "mp_playerType", "0", 0 );
		Cvar_Get( "mp_currentPlayerType", "0", 0 );
		Cvar_Get( "mp_weapon", "0", 0 );
		Cvar_Get( "mp_team", "0", 0 );
		Cvar_Get( "mp_currentTeam", "0", 0 );
	}
	if ( GGameType & GAME_ET ) {
		clet_autorecord = Cvar_Get( "clet_autorecord", "0", CVAR_TEMP );
		clet_profile = Cvar_Get( "cl_profile", "", CVAR_ROM );
		Cvar_Get( "cl_defaultProfile", "", CVAR_ROM );

		//bani
		cl_packetloss = Cvar_Get( "cl_packetloss", "0", CVAR_CHEAT );
		cl_packetdelay = Cvar_Get( "cl_packetdelay", "0", CVAR_CHEAT );

		//bani - make these cvars visible to cgame
		Cvar_Get( "cl_demorecording", "0", CVAR_ROM );
		Cvar_Get( "cl_demofilename", "", CVAR_ROM );
		Cvar_Get( "cl_demooffset", "0", CVAR_ROM );
		Cvar_Get( "cl_waverecording", "0", CVAR_ROM );
		Cvar_Get( "cl_wavefilename", "", CVAR_ROM );
		Cvar_Get( "cl_waveoffset", "0", CVAR_ROM );
	}

	// userinfo
	Cvar_Get( "snaps", "20", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get( "cl_anonymous", "0", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get( "password", "", CVAR_USERINFO );
	if ( !( GGameType & GAME_ET ) ) {
		Cvar_Get( "handicap", "100", CVAR_USERINFO | CVAR_ARCHIVE );
		Cvar_Get( "sex", "male", CVAR_USERINFO | CVAR_ARCHIVE );
		Cvar_Get( "cg_predictItems", "1", CVAR_USERINFO | CVAR_ARCHIVE );
	}
	if ( GGameType & GAME_Quake3 ) {
		Cvar_Get( "name", "UnnamedPlayer", CVAR_USERINFO | CVAR_ARCHIVE );
		Cvar_Get( "rate", "3000", CVAR_USERINFO | CVAR_ARCHIVE );
		Cvar_Get( "model", "sarge", CVAR_USERINFO | CVAR_ARCHIVE );
		Cvar_Get( "headmodel", "sarge", CVAR_USERINFO | CVAR_ARCHIVE );
		Cvar_Get( "team_model", "james", CVAR_USERINFO | CVAR_ARCHIVE );
		Cvar_Get( "team_headmodel", "*james", CVAR_USERINFO | CVAR_ARCHIVE );
		Cvar_Get( "g_redTeam", "Stroggs", CVAR_SERVERINFO | CVAR_ARCHIVE );
		Cvar_Get( "g_blueTeam", "Pagans", CVAR_SERVERINFO | CVAR_ARCHIVE );
		Cvar_Get( "color1", "4", CVAR_USERINFO | CVAR_ARCHIVE );
		Cvar_Get( "color2", "5", CVAR_USERINFO | CVAR_ARCHIVE );
		Cvar_Get( "teamtask", "0", CVAR_USERINFO );
	} else if ( GGameType & GAME_WolfSP ) {
		Cvar_Get( "name", "Player", CVAR_USERINFO | CVAR_ARCHIVE );
		Cvar_Get( "rate", "3000", CVAR_USERINFO | CVAR_ARCHIVE );
		Cvar_Get( "model", "bj2", CVAR_USERINFO | CVAR_ARCHIVE );		// temp until we have an skeletal american model
		Cvar_Get( "head", "default", CVAR_USERINFO | CVAR_ARCHIVE );
		Cvar_Get( "color", "4", CVAR_USERINFO | CVAR_ARCHIVE );
		Cvar_Get( "cg_autoactivate", "1", CVAR_USERINFO | CVAR_ARCHIVE );
		Cvar_Get( "cg_emptyswitch", "0", CVAR_USERINFO | CVAR_ARCHIVE );
	} else if ( GGameType & GAME_WolfMP ) {
		Cvar_Get( "name", "WolfPlayer", CVAR_USERINFO | CVAR_ARCHIVE );
		Cvar_Get( "rate", "5000", CVAR_USERINFO | CVAR_ARCHIVE );			// NERVE - SMF - changed from 3000
		Cvar_Get( "model", "multi", CVAR_USERINFO | CVAR_ARCHIVE );
		Cvar_Get( "head", "default", CVAR_USERINFO | CVAR_ARCHIVE );
		Cvar_Get( "color", "4", CVAR_USERINFO | CVAR_ARCHIVE );
		Cvar_Get( "cg_autoactivate", "1", CVAR_USERINFO | CVAR_ARCHIVE );
		Cvar_Get( "cg_autoReload", "1", CVAR_ARCHIVE | CVAR_USERINFO );
	} else {
		Cvar_Get( "name", "ETPlayer", CVAR_USERINFO | CVAR_ARCHIVE );
		Cvar_Get( "rate", "5000", CVAR_USERINFO | CVAR_ARCHIVE );			// NERVE - SMF - changed from 3000
		Cvar_Get( "cl_wwwDownload", "1", CVAR_USERINFO | CVAR_ARCHIVE );
		Cvar_Get( "cg_predictItems", "1", CVAR_ARCHIVE );
		Cvar_Get( "cg_autoactivate", "1", CVAR_ARCHIVE );
		Cvar_Get( "cg_autoReload", "1", CVAR_ARCHIVE );
	}

	//
	// register our commands
	//
	Cmd_AddCommand( "record", CLT3_Record_f );
	Cmd_AddCommand( "demo", CLT3_PlayDemo_f );
	Cmd_AddCommand( "stoprecord", CLT3_StopRecord_f );
	Cmd_AddCommand( "connect", CLT3_Connect_f );
	Cmd_AddCommand( "reconnect", CLT3_Reconnect_f );

	Cmd_AddCommand( "in_restart", In_Restart_f );
	Cmd_AddCommand( "vid_restart", CLT3_Vid_Restart_f );
	Cmd_AddCommand( "snd_restart", CLT3_Snd_Restart_f );
	if ( GGameType & ( GAME_WolfMP | GAME_ET ) ) {
		Cmd_AddCommand( "ui_restart", CLT3_UI_Restart_f );
	}
	Cmd_AddCommand( "fs_openedList", CLT3_OpenedPK3List_f );
	Cmd_AddCommand( "fs_referencedList", CLT3_ReferencedPK3List_f );
	Cmd_AddCommand( "configstrings", CLT3_Configstrings_f );
	Cmd_AddCommand( "clientinfo", CLT3_Clientinfo_f );
	Cmd_AddCommand( "cinematic", CL_PlayCinematic_f );
	Cmd_AddCommand( "showip", CLT3_ShowIP_f );
	if ( GGameType & GAME_Quake3 ) {
		Cmd_AddCommand( "model", CLQ3_SetModel_f );
	} else {
		// Ridah, startup-caching system
		Cmd_AddCommand( "cache_startgather", CL_Cache_StartGather_f );
		Cmd_AddCommand( "cache_usedfile", CL_Cache_UsedFile_f );
		Cmd_AddCommand( "cache_setindex", CL_Cache_SetIndex_f );
		Cmd_AddCommand( "cache_mapchange", CL_Cache_MapChange_f );
		Cmd_AddCommand( "cache_endgather", CL_Cache_EndGather_f );

		Cmd_AddCommand( "updatescreen", SCR_UpdateScreen );
		Cmd_AddCommand( "setRecommended", CLT3_SetRecommended_f );

		CL_InitTranslation();
	}
	if ( GGameType & GAME_WolfSP ) {
		// RF, add this command so clients can't bind a key to send client damage commands to the server
		Cmd_AddCommand( "cld", CLWS_ClientDamageCommand );
		Cmd_AddCommand( "startMultiplayer", CLWS_startMultiplayer_f );
		Cmd_AddCommand( "shellExecute", CLWS_ShellExecute_URL_f );
		// RF, prevent users from issuing a map_restart manually
		Cmd_AddCommand( "map_restart", CLWS_MapRestart_f );
	}
	if ( GGameType & GAME_WolfMP ) {
		Cmd_AddCommand( "startSingleplayer", CLWM_startSingleplayer_f );
		Cmd_AddCommand( "buyNow", CLWM_buyNow_f );
		Cmd_AddCommand( "singlePlayLink", CLWM_singlePlayLink_f );
	}
	if ( GGameType & GAME_ET ) {
		//bani - we eat these commands to prevent exploits
		Cmd_AddCommand( "userinfo", CLET_EatMe_f );
		Cmd_AddCommand( "wav_record", CLET_WavRecord_f );
		Cmd_AddCommand( "wav_stoprecord", CLET_WavStopRecord_f );
	}

	CLT3_InitRef();

	Cbuf_Execute();

	Cvar_Set( "cl_running", "1" );
}

void CLT3_CheckUserinfo() {
	// don't add reliable commands when not yet connected
	if ( cls.state < CA_CHALLENGING ) {
		return;
	}
	// don't overflow the reliable command buffer when paused
	if ( cl_paused->integer ) {
		return;
	}
	// send a reliable userinfo update if needed
	if ( cvar_modifiedFlags & CVAR_USERINFO ) {
		cvar_modifiedFlags &= ~CVAR_USERINFO;
		CL_AddReliableCommand( va( "userinfo \"%s\"", Cvar_InfoString( CVAR_USERINFO, MAX_INFO_STRING_Q3 ) ) );
	}

}

//	Called by Com_Error when a game has ended and is dropping out to main menu in the "endgame" menu ('credits' right now)
void CLWS_EndgameMenu() {
	cls.ws_endgamemenu = true;		// start it next frame
}
