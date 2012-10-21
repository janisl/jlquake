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

#include "../../client.h"
#include "local.h"
#include "../quake3/local.h"
#include "../wolfsp/local.h"
#include "../wolfmp/local.h"
#include "../et/local.h"
#include "../et/dl_public.h"

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

void CLET_PurgeCache()
{
	cls.et_doCachePurge = true;
}

void CLET_DoPurgeCache()
{
	if (!cls.et_doCachePurge)
	{
		return;
	}

	cls.et_doCachePurge = false;

	if (!com_cl_running)
	{
		return;
	}

	if (!com_cl_running->integer)
	{
		return;
	}

	if (!cls.q3_rendererStarted)
	{
		return;
	}

	R_PurgeCache();
}

void CLET_WavRecord_f()
{
	if (clc.wm_wavefile)
	{
		common->Printf("Already recording a wav file\n");
		return;
	}

	CL_WriteWaveOpen();
}

void CLET_WavStopRecord_f()
{
	if (!clc.wm_wavefile)
	{
		common->Printf("Not recording a wav file\n");
		return;
	}

	CL_WriteWaveClose();
	Cvar_Set("cl_waverecording", "0");
	Cvar_Set("cl_wavefilename", "");
	Cvar_Set("cl_waveoffset", "0");
	clc.wm_waverecording = false;
}

//	After the server has cleared the hunk, these will need to be restarted
// This is the only place that any of these functions are called from
void CLT3_StartHunkUsers()
{
	if (!com_cl_running)
	{
		return;
	}

	if (!com_cl_running->integer)
	{
		return;
	}

	if (!cls.q3_rendererStarted)
	{
		cls.q3_rendererStarted = true;
		CL_InitRenderer();
	}

	if (!cls.q3_soundStarted)
	{
		cls.q3_soundStarted = true;
		S_Init();
	}

	if (!cls.q3_soundRegistered)
	{
		cls.q3_soundRegistered = true;
		S_BeginRegistration();
	}

	if (!cls.q3_uiStarted)
	{
		cls.q3_uiStarted = true;
		CLT3_InitUI();
	}
}

void CLT3_ShutdownAll()
{
	// clear sounds
	S_DisableSounds();
	if (GGameType & GAME_ET)
	{
		// download subsystem
		DL_Shutdown();
	}
	// shutdown CGame
	CLT3_ShutdownCGame();
	// shutdown UI
	CLT3_ShutdownUI();

	// shutdown the renderer
	R_Shutdown(false);			// don't destroy window or context

	if (GGameType & GAME_ET)
	{
		CLET_DoPurgeCache();
	}

	cls.q3_uiStarted = false;
	cls.q3_cgameStarted = false;
	cls.q3_rendererStarted = false;
	cls.q3_soundRegistered = false;

	if (GGameType & GAME_ET)
	{
		// Gordon: stop recording on map change etc, demos aren't valid over map changes anyway
		if (clc.demorecording)
		{
			CLT3_StopRecord_f();
		}

		if (clc.wm_waverecording)
		{
			CLET_WavStopRecord_f();
		}
	}
}

//	Called by CLT3_MapLoading, CLT3_Connect_f, CLT3_PlayDemo_f, and CLT3_ParseGamestate the only
// ways a client gets into a game
// Also called by Com_Error
void CLT3_FlushMemory()
{
	// shutdown all the client stuff
	CLT3_ShutdownAll();

	CIN_CloseAllVideos();

	// if not running a server clear the whole hunk
	if (GGameType & (GAME_WolfMP | GAME_ET) && !com_sv_running->integer)
	{
		// clear collision map data
		CM_ClearMap();
	}

	CLT3_StartHunkUsers();
}

void CLT3_Init()
{
	CLT3_InitServerLists();

	//
	// register our variables
	//
	clt3_motd = Cvar_Get("cl_motd", "1", 0);
	clt3_showServerCommands = Cvar_Get("cl_showServerCommands", "0", 0);
	clt3_showSend = Cvar_Get("cl_showSend", "0", CVAR_TEMP);
	clt3_showTimeDelta = Cvar_Get("cl_showTimeDelta", "0", CVAR_TEMP);
	clt3_activeAction = Cvar_Get("activeAction", "", CVAR_TEMP);
	clt3_maxpackets = Cvar_Get("cl_maxpackets", "30", CVAR_ARCHIVE);
	clt3_packetdup = Cvar_Get("cl_packetdup", "1", CVAR_ARCHIVE);
	clt3_allowDownload = Cvar_Get("cl_allowDownload", GGameType & GAME_ET ? "1" : "0", CVAR_ARCHIVE);
	cl_timeout = Cvar_Get("cl_timeout", "200", 0);
	clt3_timeNudge = Cvar_Get("cl_timeNudge", "0", CVAR_TEMP);
	clt3_freezeDemo = Cvar_Get("cl_freezeDemo", "0", CVAR_TEMP);

	// init autoswitch so the ui will have it correctly even
	// if the cgame hasn't been started
	// -NERVE - SMF - disabled autoswitch by default
	Cvar_Get("cg_autoswitch", GGameType & GAME_Quake3 ? "1" : GGameType & GAME_WolfSP ? "2" : "0", CVAR_ARCHIVE);

	Cvar_Get("cl_motdString", "", CVAR_ROM);
	Cvar_Get("cl_maxPing", "800", CVAR_ARCHIVE);

	// cgame might not be initialized before menu is used
	Cvar_Get("cg_viewsize", "100", CVAR_ARCHIVE);

	if (!(GGameType & GAME_Quake3))
	{
		// Rafael - particle switch
		Cvar_Get("cg_wolfparticles", "1", CVAR_ARCHIVE);
	}
	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		clwm_wavefilerecord = Cvar_Get("cl_wavefilerecord", "0", CVAR_TEMP);
		clwm_shownuments = Cvar_Get("cl_shownuments", "0", CVAR_TEMP);
		Cvar_Get("cl_visibleClients", "0", CVAR_TEMP);

		Cvar_Get("cg_drawCompass", "1", CVAR_ARCHIVE);
		Cvar_Get("cg_drawNotifyText", "1", CVAR_ARCHIVE);
		Cvar_Get("cg_quickMessageAlt", "1", CVAR_ARCHIVE);
		Cvar_Get("cg_popupLimboMenu", "1", CVAR_ARCHIVE);
		Cvar_Get("cg_descriptiveText", "1", CVAR_ARCHIVE);
		Cvar_Get("cg_drawTeamOverlay", "2", CVAR_ARCHIVE);
		Cvar_Get("cg_drawGun", "1", CVAR_ARCHIVE);
		Cvar_Get("cg_cursorHints", "1", CVAR_ARCHIVE);
		Cvar_Get("cg_voiceSpriteTime", "6000", CVAR_ARCHIVE);
		Cvar_Get("cg_crosshairSize", "48", CVAR_ARCHIVE);
		Cvar_Get("cg_drawCrosshair", "1", CVAR_ARCHIVE);
		Cvar_Get("cg_zoomDefaultSniper", "20", CVAR_ARCHIVE);
		Cvar_Get("cg_zoomstepsniper", "2", CVAR_ARCHIVE);
	}
	if (GGameType & GAME_WolfMP)
	{
		Cvar_Get("cg_uselessNostalgia", "0", CVAR_ARCHIVE);		// JPW NERVE
		Cvar_Get("cg_teamChatsOnly", "0", CVAR_ARCHIVE);
		Cvar_Get("cg_noVoiceChats", "0", CVAR_ARCHIVE);
		Cvar_Get("cg_noVoiceText", "0", CVAR_ARCHIVE);

		Cvar_Get("mp_playerType", "0", 0);
		Cvar_Get("mp_currentPlayerType", "0", 0);
		Cvar_Get("mp_weapon", "0", 0);
		Cvar_Get("mp_team", "0", 0);
		Cvar_Get("mp_currentTeam", "0", 0);
	}
	if (GGameType & GAME_ET)
	{
		clet_autorecord = Cvar_Get("clet_autorecord", "0", CVAR_TEMP);
		clet_profile = Cvar_Get("cl_profile", "", CVAR_ROM);
		Cvar_Get("cl_defaultProfile", "", CVAR_ROM);

		//bani
		cl_packetloss = Cvar_Get("cl_packetloss", "0", CVAR_CHEAT);
		cl_packetdelay = Cvar_Get("cl_packetdelay", "0", CVAR_CHEAT);

		//bani - make these cvars visible to cgame
		Cvar_Get("cl_demorecording", "0", CVAR_ROM);
		Cvar_Get("cl_demofilename", "", CVAR_ROM);
		Cvar_Get("cl_demooffset", "0", CVAR_ROM);
	}

	// userinfo
	Cvar_Get("snaps", "20", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("cl_anonymous", "0", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("password", "", CVAR_USERINFO);
	if (!(GGameType & GAME_ET))
	{
		Cvar_Get("handicap", "100", CVAR_USERINFO | CVAR_ARCHIVE);
		Cvar_Get("sex", "male", CVAR_USERINFO | CVAR_ARCHIVE);
		Cvar_Get("cg_predictItems", "1", CVAR_USERINFO | CVAR_ARCHIVE);
	}
	if (GGameType & GAME_Quake3)
	{
		Cvar_Get("name", "UnnamedPlayer", CVAR_USERINFO | CVAR_ARCHIVE);
		Cvar_Get("rate", "3000", CVAR_USERINFO | CVAR_ARCHIVE);
		Cvar_Get("model", "sarge", CVAR_USERINFO | CVAR_ARCHIVE);
		Cvar_Get("headmodel", "sarge", CVAR_USERINFO | CVAR_ARCHIVE);
		Cvar_Get("team_model", "james", CVAR_USERINFO | CVAR_ARCHIVE);
		Cvar_Get("team_headmodel", "*james", CVAR_USERINFO | CVAR_ARCHIVE);
		Cvar_Get("g_redTeam", "Stroggs", CVAR_SERVERINFO | CVAR_ARCHIVE);
		Cvar_Get("g_blueTeam", "Pagans", CVAR_SERVERINFO | CVAR_ARCHIVE);
		Cvar_Get("color1", "4", CVAR_USERINFO | CVAR_ARCHIVE);
		Cvar_Get("color2", "5", CVAR_USERINFO | CVAR_ARCHIVE);
		Cvar_Get("teamtask", "0", CVAR_USERINFO);
	}
	else if (GGameType & GAME_WolfSP)
	{
		Cvar_Get("name", "Player", CVAR_USERINFO | CVAR_ARCHIVE);
		Cvar_Get("rate", "3000", CVAR_USERINFO | CVAR_ARCHIVE);
		Cvar_Get("model", "bj2", CVAR_USERINFO | CVAR_ARCHIVE);		// temp until we have an skeletal american model
		Cvar_Get("head", "default", CVAR_USERINFO | CVAR_ARCHIVE);
		Cvar_Get("color", "4", CVAR_USERINFO | CVAR_ARCHIVE);
		Cvar_Get("cg_autoactivate", "1", CVAR_USERINFO | CVAR_ARCHIVE);
		Cvar_Get("cg_emptyswitch", "0", CVAR_USERINFO | CVAR_ARCHIVE);
	}
	else if (GGameType & GAME_WolfMP)
	{
		Cvar_Get("name", "WolfPlayer", CVAR_USERINFO | CVAR_ARCHIVE);
		Cvar_Get("rate", "5000", CVAR_USERINFO | CVAR_ARCHIVE);			// NERVE - SMF - changed from 3000
		Cvar_Get("model", "multi", CVAR_USERINFO | CVAR_ARCHIVE);
		Cvar_Get("head", "default", CVAR_USERINFO | CVAR_ARCHIVE);
		Cvar_Get("color", "4", CVAR_USERINFO | CVAR_ARCHIVE);
		Cvar_Get("cg_autoactivate", "1", CVAR_USERINFO | CVAR_ARCHIVE);
		Cvar_Get("cg_autoReload", "1", CVAR_ARCHIVE | CVAR_USERINFO);
	}
	else
	{
		Cvar_Get("name", "ETPlayer", CVAR_USERINFO | CVAR_ARCHIVE);
		Cvar_Get("rate", "5000", CVAR_USERINFO | CVAR_ARCHIVE);			// NERVE - SMF - changed from 3000
		Cvar_Get("cg_predictItems", "1", CVAR_ARCHIVE);
		Cvar_Get("cg_autoactivate", "1", CVAR_ARCHIVE);
		Cvar_Get("cg_autoReload", "1", CVAR_ARCHIVE);
	}

	//
	// register our commands
	//
	Cmd_AddCommand("record", CLT3_Record_f);
	Cmd_AddCommand("demo", CLT3_PlayDemo_f);
	Cmd_AddCommand("stoprecord", CLT3_StopRecord_f);
	Cmd_AddCommand("connect", CLT3_Connect_f);
	Cmd_AddCommand("reconnect", CLT3_Reconnect_f);
}
