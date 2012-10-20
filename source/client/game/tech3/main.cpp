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
}
