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
#include "cg_shared.h"
#include "../quake3/local.h"
#include "../wolfsp/local.h"
#include "../wolfmp/local.h"
#include "../et/local.h"

vm_t* cgvm;

void CLT3_ShutdownCGame()
{
	in_keyCatchers &= ~KEYCATCH_CGAME;
	cls.q3_cgameStarted = false;
	if (!cgvm)
	{
		return;
	}
	VM_Call(cgvm, CG_SHUTDOWN);
	VM_Free(cgvm);
	cgvm = NULL;
}

//	See if the current console command is claimed by the cgame
bool CLT3_GameCommand()
{
	if (!cgvm)
	{
		return false;
	}

	return VM_Call(cgvm, CG_CONSOLE_COMMAND);
}

void CLT3_CGameRendering(stereoFrame_t stereo)
{
	VM_Call(cgvm, CG_DRAW_ACTIVE_FRAME, cl.serverTime, stereo, clc.demoplaying);
	VM_Debug(0);
}

int CLT3_CrosshairPlayer()
{
	return VM_Call(cgvm, CG_CROSSHAIR_PLAYER);
}

int CLT3_LastAttacker()
{
	return VM_Call(cgvm, CG_LAST_ATTACKER);
}

void CLT3_KeyEvent(int key, bool down)
{
	if (!cgvm)
	{
		return;
	}
	VM_Call(cgvm, CG_KEY_EVENT, key, down);
}

void CLT3_MouseEvent(int dx, int dy)
{
	VM_Call(cgvm, CG_MOUSE_EVENT, dx, dy);
}

void CLT3_EventHandling()
{
	VM_Call(cgvm, CG_EVENT_HANDLING, CGAME_EVENT_NONE);
}

bool CL_GetTag(int clientNum, const char* tagname, orientation_t* _or)
{
	if (!cgvm)
	{
		return false;
	}

	if (GGameType & GAME_WolfSP)
	{
		return CLWS_GetTag(clientNum, tagname, _or);
	}
	if (GGameType & GAME_WolfMP)
	{
		return CLWM_GetTag(clientNum, tagname, _or);
	}
	if (GGameType & GAME_ET)
	{
		return CLET_GetTag(clientNum, tagname, _or);
	}
	return false;
}

int CLT3_GetCurrentCmdNumber()
{
	return cl.q3_cmdNumber;
}

void CLT3_AddCgameCommand(const char* cmdName)
{
	Cmd_AddCommand(cmdName, NULL);
}

//	Just adds default parameters that cgame doesn't need to know about
void CLT3_CM_LoadMap(const char* mapname)
{
	if (GGameType & (GAME_WolfMP | GAME_ET) && com_sv_running->integer)
	{
		// TTimo
		// catch here when a local server is started to avoid outdated com_errorDiagnoseIP
		Cvar_Set("com_errorDiagnoseIP", "");
	}

	int checksum;
	CM_LoadMap(mapname, true, &checksum);
}

bool CLT3_InPvs(const vec3_t p1, const vec3_t p2)
{
	byte* vis = CM_ClusterPVS(CM_LeafCluster(CM_PointLeafnum(p1)));
	int cluster = CM_LeafCluster(CM_PointLeafnum(p2));
	return !!(vis[cluster >> 3] & (1 << (cluster & 7)));
}
