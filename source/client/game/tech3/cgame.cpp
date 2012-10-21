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

#define RESET_TIME  500

vm_t* cgvm;

//	Should only be called by CLT3_StartHunkUsers
void CLT3_InitCGame()
{
	int t1 = Sys_Milliseconds();

	// put away the console
	Con_Close();

	// find the current mapname
	const char* info = GGameType & GAME_WolfSP ? cl.ws_gameState.stringData + cl.ws_gameState.stringOffsets[Q3CS_SERVERINFO] :
		GGameType & GAME_WolfMP ? cl.wm_gameState.stringData + cl.wm_gameState.stringOffsets[Q3CS_SERVERINFO] :
		GGameType & GAME_ET ? cl.et_gameState.stringData + cl.et_gameState.stringOffsets[Q3CS_SERVERINFO] :
		cl.q3_gameState.stringData + cl.q3_gameState.stringOffsets[Q3CS_SERVERINFO];
	const char* mapname = Info_ValueForKey(info, "mapname");
	String::Sprintf(cl.q3_mapname, sizeof(cl.q3_mapname), "maps/%s.bsp", mapname);

	if (GGameType & GAME_Quake3)
	{
		// load the dll or bytecode
		vmInterpret_t interpret;
		if (cl_connectedToPureServer != 0)
		{
			// if sv_pure is set we only allow qvms to be loaded
			interpret = VMI_COMPILED;
		}
		else
		{
			interpret = (vmInterpret_t)(int)Cvar_VariableValue("vm_cgame");
		}
		cgvm = VM_Create("cgame", CLQ3_CgameSystemCalls, interpret);
	}
	else if (GGameType & GAME_WolfSP)
	{
		cgvm = VM_Create("cgame", CLWS_CgameSystemCalls, VMI_NATIVE);
	}
	else if (GGameType & GAME_WolfMP)
	{
		cgvm = VM_Create("cgame", CLWM_CgameSystemCalls, VMI_NATIVE);
	}
	else
	{
		cgvm = VM_Create("cgame", CLET_CgameSystemCalls, VMI_NATIVE);
	}
	if (!cgvm)
	{
		common->Error("VM_Create on cgame failed");
	}
	cls.state = CA_LOADING;

	// init for this gamestate
	// use the lastExecutedServerCommand instead of the serverCommandSequence
	// otherwise server commands sent just before a gamestate are dropped
	if (GGameType & GAME_ET)
	{
		//bani - added clc.demoplaying, since some mods need this at init time, and drawactiveframe is too late for them
		VM_Call(cgvm, CG_INIT, clc.q3_serverMessageSequence, clc.q3_lastExecutedServerCommand, clc.q3_clientNum, clc.demoplaying);
	}
	else
	{
		VM_Call(cgvm, CG_INIT, clc.q3_serverMessageSequence, clc.q3_lastExecutedServerCommand, clc.q3_clientNum);
	}

	// we will send a usercmd this frame, which
	// will cause the server to send us the first snapshot
	cls.state = CA_PRIMED;

	int t2 = Sys_Milliseconds();

	common->Printf("CLT3_InitCGame: %5.2f seconds\n", (t2 - t1) / 1000.0);

	// have the renderer touch all its images, so they are present
	// on the card even if the driver does deferred loading
	R_EndRegistration();

	// clear anything that got printed
	Con_ClearNotify();
}

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

static void CLT3_ConfigstringModified()
{
	int index = String::Atoi(Cmd_Argv(1));
	// get everything after "cs <num>"
	const char* s = Cmd_ArgsFrom(2);

	if (GGameType & GAME_Quake3)
	{
		if (index < 0 || index >= MAX_CONFIGSTRINGS_Q3)
		{
			common->Error("configstring > MAX_CONFIGSTRINGS_Q3");
		}

		const char* old = cl.q3_gameState.stringData + cl.q3_gameState.stringOffsets[index];
		if (!String::Cmp(old, s))
		{
			return;		// unchanged
		}

		// build the new q3gameState_t
		q3gameState_t oldGs = cl.q3_gameState;

		Com_Memset(&cl.q3_gameState, 0, sizeof(cl.q3_gameState));

		// leave the first 0 for uninitialized strings
		cl.q3_gameState.dataCount = 1;

		for (int i = 0; i < MAX_CONFIGSTRINGS_Q3; i++)
		{
			const char* dup;
			if (i == index)
			{
				dup = s;
			}
			else
			{
				dup = oldGs.stringData + oldGs.stringOffsets[i];
			}
			if (!dup[0])
			{
				continue;		// leave with the default empty string
			}

			int len = String::Length(dup);

			if (len + 1 + cl.q3_gameState.dataCount > MAX_GAMESTATE_CHARS_Q3)
			{
				common->Error("MAX_GAMESTATE_CHARS_Q3 exceeded");
			}

			// append it to the gameState string buffer
			cl.q3_gameState.stringOffsets[i] = cl.q3_gameState.dataCount;
			Com_Memcpy(cl.q3_gameState.stringData + cl.q3_gameState.dataCount, dup, len + 1);
			cl.q3_gameState.dataCount += len + 1;
		}

		if (index == Q3CS_SYSTEMINFO)
		{
			// parse serverId and other cvars
			CLT3_SystemInfoChanged();
		}
	}
	else if (GGameType & GAME_WolfSP)
	{
		if (index < 0 || index >= MAX_CONFIGSTRINGS_WS)
		{
			common->Error("configstring > MAX_CONFIGSTRINGS_WS");
		}

		const char* old = cl.ws_gameState.stringData + cl.ws_gameState.stringOffsets[index];
		if (!String::Cmp(old, s))
		{
			return;		// unchanged
		}

		// build the new wsgameState_t
		wsgameState_t oldGs = cl.ws_gameState;

		memset(&cl.ws_gameState, 0, sizeof(cl.ws_gameState));

		// leave the first 0 for uninitialized strings
		cl.ws_gameState.dataCount = 1;

		for (int i = 0; i < MAX_CONFIGSTRINGS_WS; i++)
		{
			const char* dup;
			if (i == index)
			{
				dup = s;
			}
			else
			{
				dup = oldGs.stringData + oldGs.stringOffsets[i];
			}
			if (!dup[0])
			{
				continue;		// leave with the default empty string
			}

			int len = String::Length(dup);

			if (len + 1 + cl.ws_gameState.dataCount > MAX_GAMESTATE_CHARS_Q3)
			{
				common->Error("MAX_GAMESTATE_CHARS_Q3 exceeded");
			}

			// append it to the gameState string buffer
			cl.ws_gameState.stringOffsets[i] = cl.ws_gameState.dataCount;
			memcpy(cl.ws_gameState.stringData + cl.ws_gameState.dataCount, dup, len + 1);
			cl.ws_gameState.dataCount += len + 1;
		}

		if (index == Q3CS_SYSTEMINFO)
		{
			// parse serverId and other cvars
			CLT3_SystemInfoChanged();
		}
	}
	else if (GGameType & GAME_WolfMP)
	{
		if (index < 0 || index >= MAX_CONFIGSTRINGS_WM)
		{
			common->Error("configstring > MAX_CONFIGSTRINGS_WM");
		}

		const char* old = cl.wm_gameState.stringData + cl.wm_gameState.stringOffsets[index];
		if (!String::Cmp(old, s))
		{
			return;		// unchanged
		}

		// build the new wmgameState_t
		wmgameState_t oldGs = cl.wm_gameState;

		memset(&cl.wm_gameState, 0, sizeof(cl.wm_gameState));

		// leave the first 0 for uninitialized strings
		cl.wm_gameState.dataCount = 1;

		for (int i = 0; i < MAX_CONFIGSTRINGS_WM; i++)
		{
			const char* dup;
			if (i == index)
			{
				dup = s;
			}
			else
			{
				dup = oldGs.stringData + oldGs.stringOffsets[i];
			}
			if (!dup[0])
			{
				continue;		// leave with the default empty string
			}

			int len = String::Length(dup);

			if (len + 1 + cl.wm_gameState.dataCount > MAX_GAMESTATE_CHARS_Q3)
			{
				common->Error("MAX_GAMESTATE_CHARS_Q3 exceeded");
			}

			// append it to the gameState string buffer
			cl.wm_gameState.stringOffsets[i] = cl.wm_gameState.dataCount;
			memcpy(cl.wm_gameState.stringData + cl.wm_gameState.dataCount, dup, len + 1);
			cl.wm_gameState.dataCount += len + 1;
		}

		if (index == Q3CS_SYSTEMINFO)
		{
			// parse serverId and other cvars
			CLT3_SystemInfoChanged();
		}
	}
	else
	{
		if (index < 0 || index >= MAX_CONFIGSTRINGS_ET)
		{
			common->Error("configstring > MAX_CONFIGSTRINGS_ET");
		}

		const char* old = cl.et_gameState.stringData + cl.et_gameState.stringOffsets[index];
		if (!String::Cmp(old, s))
		{
			return;		// unchanged
		}

		// build the new etgameState_t
		etgameState_t oldGs = cl.et_gameState;

		memset(&cl.et_gameState, 0, sizeof(cl.et_gameState));

		// leave the first 0 for uninitialized strings
		cl.et_gameState.dataCount = 1;

		for (int i = 0; i < MAX_CONFIGSTRINGS_ET; i++)
		{
			const char* dup;
			if (i == index)
			{
				dup = s;
			}
			else
			{
				dup = oldGs.stringData + oldGs.stringOffsets[i];
			}
			if (!dup[0])
			{
				continue;		// leave with the default empty string
			}

			int len = String::Length(dup);

			if (len + 1 + cl.et_gameState.dataCount > MAX_GAMESTATE_CHARS_Q3)
			{
				common->Error("MAX_GAMESTATE_CHARS_Q3 exceeded");
			}

			// append it to the gameState string buffer
			cl.et_gameState.stringOffsets[i] = cl.et_gameState.dataCount;
			memcpy(cl.et_gameState.stringData + cl.et_gameState.dataCount, dup, len + 1);
			cl.et_gameState.dataCount += len + 1;
		}

		if (index == Q3CS_SYSTEMINFO)
		{
			// parse serverId and other cvars
			CLT3_SystemInfoChanged();
		}
	}
}

//	Set up argc/argv for the given command
bool CLT3_GetServerCommand(int serverCommandNumber)
{
	static char bigConfigString[BIG_INFO_STRING];

	// if we have irretrievably lost a reliable command, drop the connection
	int maxReliableCommands = GGameType & GAME_Quake3 ? MAX_RELIABLE_COMMANDS_Q3 : MAX_RELIABLE_COMMANDS_WOLF;
	if (serverCommandNumber <= clc.q3_serverCommandSequence - maxReliableCommands)
	{
		// when a demo record was started after the client got a whole bunch of
		// reliable commands then the client never got those first reliable commands
		if (clc.demoplaying)
		{
			return false;
		}
		common->Error("CLT3_GetServerCommand: a reliable command was cycled out");
		return false;
	}

	if (serverCommandNumber > clc.q3_serverCommandSequence)
	{
		common->Error("CLT3_GetServerCommand: requested a command not received");
		return false;
	}

	const char* s = clc.q3_serverCommands[serverCommandNumber & (maxReliableCommands - 1)];
	clc.q3_lastExecutedServerCommand = serverCommandNumber;

	if (clt3_showServerCommands->integer)
	{
		common->DPrintf("serverCommand: %i : %s\n", serverCommandNumber, s);
	}

rescan:
	Cmd_TokenizeString(s);
	const char* cmd = Cmd_Argv(0);
	int argc = Cmd_Argc();

	if (!String::Cmp(cmd, "disconnect"))
	{
		// allow server to indicate why they were disconnected
		if (argc >= 2)
		{
			common->ServerDisconnected("Server Disconnected - %s", Cmd_Argv(1));
		}
		else
		{
			common->ServerDisconnected("Server disconnected\n");
		}
	}

	if (!String::Cmp(cmd, "bcs0"))
	{
		String::Sprintf(bigConfigString, BIG_INFO_STRING, "cs %s \"%s", Cmd_Argv(1), Cmd_Argv(2));
		return false;
	}

	if (!String::Cmp(cmd, "bcs1"))
	{
		s = Cmd_Argv(2);
		if (String::Length(bigConfigString) + String::Length(s) >= BIG_INFO_STRING)
		{
			common->Error("bcs exceeded BIG_INFO_STRING");
		}
		String::Cat(bigConfigString, sizeof(bigConfigString), s);
		return false;
	}

	if (!String::Cmp(cmd, "bcs2"))
	{
		s = Cmd_Argv(2);
		if (String::Length(bigConfigString) + String::Length(s) + 1 >= BIG_INFO_STRING)
		{
			common->Error("bcs exceeded BIG_INFO_STRING");
		}
		String::Cat(bigConfigString, sizeof(bigConfigString), s);
		String::Cat(bigConfigString, sizeof(bigConfigString), "\"");
		s = bigConfigString;
		goto rescan;
	}

	if (!String::Cmp(cmd, "cs"))
	{
		CLT3_ConfigstringModified();
		// reparse the string, because CLT3_ConfigstringModified may have done another Cmd_TokenizeString()
		Cmd_TokenizeString(s);
		return true;
	}

	if (!String::Cmp(cmd, "map_restart"))
	{
		// clear notify lines and outgoing commands before passing
		// the restart to the cgame
		Con_ClearNotify();
		Com_Memset(cl.q3_cmds, 0, sizeof(cl.q3_cmds));
		Com_Memset(cl.ws_cmds, 0, sizeof(cl.ws_cmds));
		Com_Memset(cl.wm_cmds, 0, sizeof(cl.wm_cmds));
		Com_Memset(cl.et_cmds, 0, sizeof(cl.et_cmds));
		return true;
	}

	// the clientLevelShot command is used during development
	// to generate 128*128 screenshots from the intermission
	// point of levels for the menu system to use
	// we pass it along to the cgame to make apropriate adjustments,
	// but we also clear the console and notify lines here
	if (!String::Cmp(cmd, "clientLevelShot"))
	{
		// don't do it if we aren't running the server locally,
		// otherwise malicious remote servers could overwrite
		// the existing thumbnails
		if (!com_sv_running->integer)
		{
			return false;
		}
		// close the console
		Con_Close();
		// take a special screenshot next frame
		Cbuf_AddText("wait ; wait ; wait ; wait ; screenshot levelshot\n");
		return true;
	}

	if (!(GGameType & GAME_Quake3) && !String::Cmp(cmd, "popup"))			// direct server to client popup request, bypassing cgame
	{
		return false;
	}

	// we may want to put a "connect to other server" command here

	// cgame can now act on the command
	return true;
}

void CLT3_AddToLimboChat(const char* str)
{
	if (!str)
	{
		return;
	}
	cl.wa_limboChatPos = LIMBOCHAT_HEIGHT_WA - 1;

	// copy old strings
	for (int i = cl.wa_limboChatPos; i > 0; i--)
	{
		String::Cpy(cl.wa_limboChatMsgs[i], cl.wa_limboChatMsgs[i - 1]);
	}

	// copy new string
	char* p = cl.wa_limboChatMsgs[0];
	*p = 0;

	int len = 0;
	while (*str)
	{
		if (len > LIMBOCHAT_WIDTH_WA - 1)
		{
			break;
		}

		if (Q_IsColorString(str))
		{
			*p++ = *str++;
			*p++ = *str++;
			continue;
		}
		*p++ = *str++;
		len++;
	}
	*p = 0;
}

//	Adjust the clients view of server time.
//
//	We attempt to have cl.serverTime exactly equal the server's view
// of time plus the timeNudge, but with variable latencies over
// the internet it will often need to drift a bit to match conditions.
//
//	Our ideal time would be to have the adjusted time approach, but not pass,
// the very latest snapshot.
//
//	Adjustments are only made when a new snapshot arrives with a rational
// latency, which keeps the adjustment process framerate independent and
// prevents massive overadjustment during times of significant packet loss
// or bursted delayed packets.
static void CLT3_AdjustTimeDelta()
{
	cl.q3_newSnapshots = false;

	// the delta never drifts when replaying a demo
	if (clc.demoplaying)
	{
		return;
	}

	// if the current time is WAY off, just correct to the current value
	int resetTime;
	if (com_sv_running->integer)
	{
		resetTime = 100;
	}
	else
	{
		resetTime = RESET_TIME;
	}

	int newDelta = GGameType & GAME_WolfSP ? cl.ws_snap.serverTime - cls.realtime :
		GGameType & GAME_WolfMP ? cl.wm_snap.serverTime - cls.realtime :
		GGameType & GAME_ET ? cl.et_snap.serverTime - cls.realtime :
		cl.q3_snap.serverTime - cls.realtime;
	int deltaDelta = abs(newDelta - cl.q3_serverTimeDelta);

	if (deltaDelta > RESET_TIME)
	{
		cl.q3_serverTimeDelta = newDelta;
		if (GGameType & GAME_WolfSP)
		{
			cl.q3_oldServerTime = cl.ws_snap.serverTime;	// FIXME: is this a problem for cgame?
			cl.serverTime = cl.ws_snap.serverTime;
		}
		else if (GGameType & GAME_WolfMP)
		{
			cl.q3_oldServerTime = cl.wm_snap.serverTime;	// FIXME: is this a problem for cgame?
			cl.serverTime = cl.wm_snap.serverTime;
		}
		else if (GGameType & GAME_ET)
		{
			cl.q3_oldServerTime = cl.et_snap.serverTime;	// FIXME: is this a problem for cgame?
			cl.serverTime = cl.et_snap.serverTime;
		}
		else
		{
			cl.q3_oldServerTime = cl.q3_snap.serverTime;	// FIXME: is this a problem for cgame?
			cl.serverTime = cl.q3_snap.serverTime;
		}
		if (clt3_showTimeDelta->integer)
		{
			common->Printf("<RESET> ");
		}
	}
	else if (deltaDelta > 100)
	{
		// fast adjust, cut the difference in half
		if (clt3_showTimeDelta->integer)
		{
			common->Printf("<FAST> ");
		}
		cl.q3_serverTimeDelta = (cl.q3_serverTimeDelta + newDelta) >> 1;
	}
	else
	{
		// slow drift adjust, only move 1 or 2 msec

		// if any of the frames between this and the previous snapshot
		// had to be extrapolated, nudge our sense of time back a little
		// the granularity of +1 / -2 is too high for timescale modified frametimes
		if (com_timescale->value == 0 || com_timescale->value == 1)
		{
			if (cl.q3_extrapolatedSnapshot)
			{
				cl.q3_extrapolatedSnapshot = false;
				cl.q3_serverTimeDelta -= 2;
			}
			else
			{
				// otherwise, move our sense of time forward to minimize total latency
				cl.q3_serverTimeDelta++;
			}
		}
	}

	if (clt3_showTimeDelta->integer)
	{
		common->Printf("%i ", cl.q3_serverTimeDelta);
	}
}

static void CLT3_FirstSnapshot()
{
	if (GGameType & GAME_WolfSP)
	{
		// ignore snapshots that don't have entities
		if (cl.ws_snap.snapFlags & Q3SNAPFLAG_NOT_ACTIVE)
		{
			return;
		}
		cls.state = CA_ACTIVE;

		// set the timedelta so we are exactly on this first frame
		cl.q3_serverTimeDelta = cl.ws_snap.serverTime - cls.realtime;
		cl.q3_oldServerTime = cl.ws_snap.serverTime;

		clc.q3_timeDemoBaseTime = cl.ws_snap.serverTime;
	}
	else if (GGameType & GAME_WolfMP)
	{
		// ignore snapshots that don't have entities
		if (cl.wm_snap.snapFlags & Q3SNAPFLAG_NOT_ACTIVE)
		{
			return;
		}
		cls.state = CA_ACTIVE;

		// set the timedelta so we are exactly on this first frame
		cl.q3_serverTimeDelta = cl.wm_snap.serverTime - cls.realtime;
		cl.q3_oldServerTime = cl.wm_snap.serverTime;

		clc.q3_timeDemoBaseTime = cl.wm_snap.serverTime;
	}
	else if (GGameType & GAME_ET)
	{
		// ignore snapshots that don't have entities
		if (cl.et_snap.snapFlags & Q3SNAPFLAG_NOT_ACTIVE)
		{
			return;
		}
		cls.state = CA_ACTIVE;

		// set the timedelta so we are exactly on this first frame
		cl.q3_serverTimeDelta = cl.et_snap.serverTime - cls.realtime;
		cl.q3_oldServerTime = cl.et_snap.serverTime;

		clc.q3_timeDemoBaseTime = cl.et_snap.serverTime;
	}
	else
	{
		// ignore snapshots that don't have entities
		if (cl.q3_snap.snapFlags & Q3SNAPFLAG_NOT_ACTIVE)
		{
			return;
		}
		cls.state = CA_ACTIVE;

		// set the timedelta so we are exactly on this first frame
		cl.q3_serverTimeDelta = cl.q3_snap.serverTime - cls.realtime;
		cl.q3_oldServerTime = cl.q3_snap.serverTime;

		clc.q3_timeDemoBaseTime = cl.q3_snap.serverTime;
	}

	// if this is the first frame of active play,
	// execute the contents of activeAction now
	// this is to allow scripting a timedemo to start right
	// after loading
	if (clt3_activeAction->string[0])
	{
		Cbuf_AddText(clt3_activeAction->string);
		Cbuf_AddText("\n");
		Cvar_Set("activeAction", "");
	}
}

void CLT3_SetCGameTime()
{
	// getting a valid frame message ends the connection process
	if (cls.state != CA_ACTIVE)
	{
		if (cls.state != CA_PRIMED)
		{
			return;
		}
		if (clc.demoplaying)
		{
			// we shouldn't get the first snapshot on the same frame
			// as the gamestate, because it causes a bad time skip
			if (!clc.q3_firstDemoFrameSkipped)
			{
				clc.q3_firstDemoFrameSkipped = true;
				return;
			}
			CLT3_ReadDemoMessage();
		}
		if (cl.q3_newSnapshots)
		{
			cl.q3_newSnapshots = false;
			CLT3_FirstSnapshot();
		}
		if (cls.state != CA_ACTIVE)
		{
			return;
		}
	}

	// if we have gotten to this point, cl.snap is guaranteed to be valid
	if (GGameType & GAME_WolfSP)
	{
		if (!cl.ws_snap.valid)
		{
			common->Error("CLT3_SetCGameTime: !cl.snap.valid");
		}
	}
	else if (GGameType & GAME_WolfMP)
	{
		if (!cl.wm_snap.valid)
		{
			common->Error("CLT3_SetCGameTime: !cl.snap.valid");
		}
	}
	else if (GGameType & GAME_ET)
	{
		if (!cl.et_snap.valid)
		{
			common->Error("CLT3_SetCGameTime: !cl.snap.valid");
		}
	}
	else
	{
		if (!cl.q3_snap.valid)
		{
			common->Error("CLT3_SetCGameTime: !cl.snap.valid");
		}
	}

	// allow pause in single player
	if (sv_paused->integer && cl_paused->integer && com_sv_running->integer)
	{
		// paused
		return;
	}

	if (GGameType & GAME_WolfSP)
	{
		if (cl.ws_snap.serverTime < cl.q3_oldFrameServerTime)
		{
			// Ridah, if this is a localhost, then we are probably loading a savegame
			if (!String::ICmp(cls.servername, "localhost"))
			{
				// do nothing?
				CLT3_FirstSnapshot();
			}
			else
			{
				common->Error("cl.ws_snap.serverTime < cl.oldFrameServerTime");
			}
		}
		cl.q3_oldFrameServerTime = cl.ws_snap.serverTime;
	}
	else if (GGameType & GAME_WolfMP)
	{
		if (cl.wm_snap.serverTime < cl.q3_oldFrameServerTime)
		{
			// Ridah, if this is a localhost, then we are probably loading a savegame
			if (!String::ICmp(cls.servername, "localhost"))
			{
				// do nothing?
				CLT3_FirstSnapshot();
			}
			else
			{
				common->Error("cl.wm_snap.serverTime < cl.oldFrameServerTime");
			}
		}
		cl.q3_oldFrameServerTime = cl.wm_snap.serverTime;
	}
	else if (GGameType & GAME_ET)
	{
		if (cl.et_snap.serverTime < cl.q3_oldFrameServerTime)
		{
			// Ridah, if this is a localhost, then we are probably loading a savegame
			if (!String::ICmp(cls.servername, "localhost"))
			{
				// do nothing?
				CLT3_FirstSnapshot();
			}
			else
			{
				common->Error("cl.et_snap.serverTime < cl.oldFrameServerTime");
			}
		}
		cl.q3_oldFrameServerTime = cl.et_snap.serverTime;
	}
	else
	{
		if (cl.q3_snap.serverTime < cl.q3_oldFrameServerTime)
		{
			common->Error("cl.snap.serverTime < cl.oldFrameServerTime");
		}
		cl.q3_oldFrameServerTime = cl.q3_snap.serverTime;
	}

	// get our current view of time

	if (clc.demoplaying && clt3_freezeDemo->integer)
	{
		// cl_freezeDemo is used to lock a demo in place for single frame advances

	}
	else
	{
		// cl_timeNudge is a user adjustable cvar that allows more
		// or less latency to be added in the interest of better
		// smoothness or better responsiveness.
		int tn;

		tn = clt3_timeNudge->integer;
		if (tn < -30)
		{
			tn = -30;
		}
		else if (tn > 30)
		{
			tn = 30;
		}

		cl.serverTime = cls.realtime + cl.q3_serverTimeDelta - tn;

		// guarantee that time will never flow backwards, even if
		// serverTimeDelta made an adjustment or cl_timeNudge was changed
		if (cl.serverTime < cl.q3_oldServerTime)
		{
			cl.serverTime = cl.q3_oldServerTime;
		}
		cl.q3_oldServerTime = cl.serverTime;

		// note if we are almost past the latest frame (without timeNudge),
		// so we will try and adjust back a bit when the next snapshot arrives
		if (GGameType & GAME_WolfSP)
		{
			if (cls.realtime + cl.q3_serverTimeDelta >= cl.ws_snap.serverTime - 5)
			{
				cl.q3_extrapolatedSnapshot = true;
			}
		}
		else if (GGameType & GAME_WolfMP)
		{
			if (cls.realtime + cl.q3_serverTimeDelta >= cl.wm_snap.serverTime - 5)
			{
				cl.q3_extrapolatedSnapshot = true;
			}
		}
		else if (GGameType & GAME_ET)
		{
			if (cls.realtime + cl.q3_serverTimeDelta >= cl.et_snap.serverTime - 5)
			{
				cl.q3_extrapolatedSnapshot = true;
			}
		}
		else
		{
			if (cls.realtime + cl.q3_serverTimeDelta >= cl.q3_snap.serverTime - 5)
			{
				cl.q3_extrapolatedSnapshot = true;
			}
		}
	}

	// if we have gotten new snapshots, drift serverTimeDelta
	// don't do this every frame, or a period of packet loss would
	// make a huge adjustment
	if (cl.q3_newSnapshots)
	{
		CLT3_AdjustTimeDelta();
	}

	if (!clc.demoplaying)
	{
		return;
	}

	// if we are playing a demo back, we can just keep reading
	// messages from the demo file until the cgame definately
	// has valid snapshots to interpolate between

	// a timedemo will always use a deterministic set of time samples
	// no matter what speed machine it is run on,
	// while a normal demo may have different time samples
	// each time it is played back
	if (cl_timedemo->integer)
	{
		if (!clc.q3_timeDemoStart)
		{
			clc.q3_timeDemoStart = Sys_Milliseconds();
		}
		clc.q3_timeDemoFrames++;
		cl.serverTime = clc.q3_timeDemoBaseTime + clc.q3_timeDemoFrames * 50;
	}

	while (cl.serverTime >= (GGameType & GAME_WolfSP ? cl.ws_snap.serverTime :
		GGameType & GAME_WolfMP ? cl.wm_snap.serverTime :
		GGameType & GAME_ET ? cl.et_snap.serverTime : cl.q3_snap.serverTime))
	{
		// feed another messag, which should change
		// the contents of cl.snap
		CLT3_ReadDemoMessage();
		if (cls.state != CA_ACTIVE)
		{
			if (GGameType & GAME_ET)
			{
				Cvar_Set("timescale", "1");
			}
			return;		// end of demo
		}
	}
}
