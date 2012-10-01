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

//	Should only be called by CL_StartHunkUsers
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
