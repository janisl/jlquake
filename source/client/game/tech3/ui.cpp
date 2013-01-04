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
#include "../../public.h"
#include "../../system.h"
#include "ui_shared.h"
#include "../quake3/local.h"
#include "../wolfsp/local.h"
#include "../wolfmp/local.h"
#include "../et/local.h"
#include "../../../common/common_defs.h"
#include "../../../common/strings.h"

vm_t* uivm;

void UIT3_Init()
{
	if (GGameType & GAME_WolfSP)
	{
		UIWS_Init();
	}
	if (GGameType & GAME_WolfMP)
	{
		UIWM_Init();
	}
	if (GGameType & GAME_ET)
	{
		UIET_Init();
	}
}

void CLT3_InitUI()
{
	if (GGameType & GAME_Quake3)
	{
		vmInterpret_t interpret;
		// load the dll or bytecode
		if (cl_connectedToPureServer != 0)
		{
			// if sv_pure is set we only allow qvms to be loaded
			interpret = VMI_COMPILED;
		}
		else
		{
			interpret = (vmInterpret_t)(int)Cvar_VariableValue("vm_ui");
		}
		uivm = VM_Create("ui", CLQ3_UISystemCalls, interpret);
	}
	else if (GGameType & GAME_WolfSP)
	{
		uivm = VM_Create("ui", CLWS_UISystemCalls, VMI_NATIVE);
	}
	else if (GGameType & GAME_WolfMP)
	{
		uivm = VM_Create("ui", CLWM_UISystemCalls, VMI_NATIVE);
	}
	else
	{
		uivm = VM_Create("ui", CLET_UISystemCalls, VMI_NATIVE);
	}

	if (!uivm)
	{
		common->FatalError("VM_Create on UI failed");
	}

	// sanity check
	int v = VM_Call(uivm, UI_GETAPIVERSION);
	if (GGameType & GAME_Quake3 && v != Q3UI_OLD_API_VERSION && v != Q3UI_API_VERSION)
	{
		common->Error("User Interface is version %d, expected %d", v, Q3UI_API_VERSION);
		cls.q3_uiStarted = false;
		return;
	}
	else if (!(GGameType & GAME_Quake3) && v != WOLFUI_API_VERSION)
	{
		common->FatalError("User Interface is version %d, expected %d", v, WOLFUI_API_VERSION);
		cls.q3_uiStarted = false;
	}

	// init for this gamestate
	VM_Call(uivm, UI_INIT, (cls.state >= CA_AUTHORIZING && cls.state < CA_ACTIVE));
}

void CLT3_ShutdownUI()
{
	in_keyCatchers &= ~KEYCATCH_UI;
	cls.q3_uiStarted = false;
	if (!uivm)
	{
		return;
	}
	VM_Call(uivm, UI_SHUTDOWN);
	VM_Free(uivm);
	uivm = NULL;
}

void UIT3_KeyEvent(int key, bool down)
{
	if (!uivm)
	{
		return;
	}
	VM_Call(uivm, UI_KEY_EVENT, key, down);
}

void UIT3_KeyDownEvent(int key)
{
	if (GGameType & GAME_WolfSP)
	{
		UIWS_KeyDownEvent(key);
	}
	else if (GGameType & GAME_WolfMP)
	{
		UIWM_KeyDownEvent(key);
	}
	else
	{
		UIT3_KeyEvent(key, true);
	}
}

void UIT3_MouseEvent(int dx, int dy)
{
	VM_Call(uivm, UI_MOUSE_EVENT, dx, dy);
}

void UIT3_Refresh(int time)
{
	if (uivm)
	{
		VM_Call(uivm, UI_REFRESH, time);
	}
}

bool UIT3_IsFullscreen()
{
	return VM_Call(uivm, UI_IS_FULLSCREEN);
}

void UIT3_SetActiveMenu(int menu)
{
	VM_Call(uivm, UI_SET_ACTIVE_MENU, menu);
}

void UIT3_ForceMenuOff()
{
	if (uivm)
	{
		UIT3_SetActiveMenu(UIMENU_NONE);
	}
}

void UIT3_SetMainMenu()
{
	UIT3_SetActiveMenu(UIMENU_MAIN);
}

void UIT3_SetInGameMenu()
{
	UIT3_SetActiveMenu(UIMENU_INGAME);
}

//	See if the current console command is claimed by the ui
bool UIT3_GameCommand()
{
	if (!uivm)
	{
		return false;
	}

	if (GGameType & GAME_WolfSP)
	{
		return UIWS_ConsoleCommand(cls.realtime);
	}
	if (GGameType & GAME_WolfMP)
	{
		return UIWM_ConsoleCommand(cls.realtime);
	}
	if (GGameType & GAME_ET)
	{
		return UIET_ConsoleCommand(cls.realtime);
	}
	return UIQ3_ConsoleCommand(cls.realtime);
}

void UIT3_DrawConnectScreen(bool overlay)
{
	if (GGameType & GAME_WolfSP)
	{
		UIWS_DrawConnectScreen(overlay);
	}
	else if (GGameType & GAME_WolfMP)
	{
		UIWM_DrawConnectScreen(overlay);
	}
	else if (GGameType & GAME_ET)
	{
		UIET_DrawConnectScreen(overlay);
	}
	else
	{
		UIQ3_DrawConnectScreen(overlay);
	}
}

bool UIT3_UsesUniqueCDKey()
{
	if (!uivm)
	{
		return false;
	}
	if (GGameType & GAME_WolfSP)
	{
		return UIWS_HasUniqueCDKey();
	}
	if (GGameType & GAME_WolfMP)
	{
		return UIWM_HasUniqueCDKey();
	}
	if (GGameType & GAME_ET)
	{
		return UIET_HasUniqueCDKey();
	}
	return UIQ3_HasUniqueCDKey();
}

bool UIT3_CheckKeyExec(int key)
{
	if (!uivm)
	{
		return false;
	}
	if (GGameType & GAME_WolfMP)
	{
		return UIWM_CheckExecKey(key);
	}
	return UIET_CheckExecKey(key);
}

void UIT3_GetClientState(uiClientState_t* state)
{
	state->connectPacketCount = clc.q3_connectPacketCount;
	state->connState = cls.state;
	String::NCpyZ(state->servername, cls.servername, sizeof(state->servername));
	String::NCpyZ(state->updateInfoString, cls.q3_updateInfoString, sizeof(state->updateInfoString));
	String::NCpyZ(state->messageString, clc.q3_serverMessage, sizeof(state->messageString));
	state->clientNum = GGameType & GAME_WolfSP ? cl.ws_snap.ps.clientNum :
		GGameType & GAME_WolfMP ? cl.wm_snap.ps.clientNum :
		GGameType & GAME_ET ? cl.et_snap.ps.clientNum : cl.q3_snap.ps.clientNum;
}

void LAN_LoadCachedServers()
{
	cls.q3_numglobalservers = cls.q3_nummplayerservers = cls.q3_numfavoriteservers = 0;
	cls.q3_numGlobalServerAddresses = 0;

	char filename[MAX_QPATH];
	if (GGameType & GAME_ET && comet_gameInfo.usesProfiles && clet_profile->string[0])
	{
		String::Sprintf(filename, sizeof(filename), "profiles/%s/servercache.dat", clet_profile->string);
	}
	else
	{
		String::NCpyZ(filename, "servercache.dat", sizeof(filename));
	}

	// Arnout: moved to mod/profiles dir
	fileHandle_t fileIn;
	if (FS_FOpenFileRead(filename, &fileIn, true))
	{
		FS_Read(&cls.q3_numglobalservers, sizeof(int), fileIn);
		if (!(GGameType & GAME_ET))
		{
			FS_Read(&cls.q3_nummplayerservers, sizeof(int), fileIn);
		}
		FS_Read(&cls.q3_numfavoriteservers, sizeof(int), fileIn);
		int size;
		FS_Read(&size, sizeof(int), fileIn);
		if (size == static_cast<int>(sizeof(cls.q3_globalServers) + sizeof(cls.q3_favoriteServers) + (GGameType & GAME_ET ? 0 : sizeof(cls.q3_mplayerServers))))
		{
			FS_Read(&cls.q3_globalServers, sizeof(cls.q3_globalServers), fileIn);
			if (!(GGameType & GAME_ET))
			{
				FS_Read(&cls.q3_mplayerServers, sizeof(cls.q3_mplayerServers), fileIn);
			}
			FS_Read(&cls.q3_favoriteServers, sizeof(cls.q3_favoriteServers), fileIn);
		}
		else
		{
			cls.q3_numglobalservers = cls.q3_nummplayerservers = cls.q3_numfavoriteservers = 0;
			cls.q3_numGlobalServerAddresses = 0;
		}
		FS_FCloseFile(fileIn);
	}
}

void LAN_SaveServersToCache()
{
	int size;
	char filename[MAX_QPATH];

	if (GGameType & GAME_ET && comet_gameInfo.usesProfiles && clet_profile->string[0])
	{
		String::Sprintf(filename, sizeof(filename), "profiles/%s/servercache.dat", clet_profile->string);
	}
	else
	{
		String::NCpyZ(filename, "servercache.dat", sizeof(filename));
	}

	// Arnout: moved to mod/profiles dir
	fileHandle_t fileOut = FS_FOpenFileWrite(filename);
	FS_Write(&cls.q3_numglobalservers, sizeof(int), fileOut);
	if (!(GGameType & GAME_ET))
	{
		FS_Write(&cls.q3_nummplayerservers, sizeof(int), fileOut);
	}
	FS_Write(&cls.q3_numfavoriteservers, sizeof(int), fileOut);
	size = sizeof(cls.q3_globalServers) + sizeof(cls.q3_favoriteServers) + (GGameType & GAME_ET ? 0 : sizeof(cls.q3_mplayerServers));
	FS_Write(&size, sizeof(int), fileOut);
	FS_Write(&cls.q3_globalServers, sizeof(cls.q3_globalServers), fileOut);
	if (!(GGameType & GAME_ET))
	{
		FS_Write(&cls.q3_mplayerServers, sizeof(cls.q3_mplayerServers), fileOut);
	}
	FS_Write(&cls.q3_favoriteServers, sizeof(cls.q3_favoriteServers), fileOut);
	FS_FCloseFile(fileOut);
}

void CLT3_GetServersForSource(int source, q3serverInfo_t*& servers, int& max, int*& count)
{
	servers = NULL;
	max = MAX_OTHER_SERVERS_Q3;
	count = 0;

	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		switch (source)
		{
		case WMAS_LOCAL:
			count = &cls.q3_numlocalservers;
			servers = &cls.q3_localServers[0];
			break;
		case WMAS_MPLAYER:
			count = &cls.q3_nummplayerservers;
			servers = &cls.q3_mplayerServers[0];
			break;
		case WMAS_GLOBAL:
			max = GGameType & GAME_ET ? MAX_GLOBAL_SERVERS_ET : MAX_GLOBAL_SERVERS_WM;
			count = &cls.q3_numglobalservers;
			servers = &cls.q3_globalServers[0];
			break;
		case WMAS_FAVORITES:
			count = &cls.q3_numfavoriteservers;
			servers = &cls.q3_favoriteServers[0];
			break;
		}
	}
	else
	{
		switch (source)
		{
		case Q3AS_LOCAL:
			count = &cls.q3_numlocalservers;
			servers = &cls.q3_localServers[0];
			break;
		case Q3AS_MPLAYER:
			count = &cls.q3_nummplayerservers;
			servers = &cls.q3_mplayerServers[0];
			break;
		case Q3AS_GLOBAL:
			max = GGameType & GAME_WolfSP ? MAX_GLOBAL_SERVERS_WS : MAX_GLOBAL_SERVERS_Q3;
			count = &cls.q3_numglobalservers;
			servers = &cls.q3_globalServers[0];
			break;
		case Q3AS_FAVORITES:
			count = &cls.q3_numfavoriteservers;
			servers = &cls.q3_favoriteServers[0];
			break;
		}
	}
}

q3serverInfo_t* LAN_GetServerPtr(int source, int n)
{
	q3serverInfo_t* servers;
	int max;
	int* count;
	CLT3_GetServersForSource(source, servers, max, count);
	if (servers && n >= 0 && n < max)
	{
		return &servers[n];
	}
	return NULL;
}

int LAN_AddServer(int source, const char* name, const char* address)
{
	q3serverInfo_t* servers;
	int max;
	int* count;
	CLT3_GetServersForSource(source, servers, max, count);
	if (servers && *count < max)
	{
		netadr_t adr;
		SOCK_StringToAdr(address, &adr, Q3PORT_SERVER);
		int i;
		for (i = 0; i < *count; i++)
		{
			if (SOCK_CompareAdr(servers[i].adr, adr))
			{
				break;
			}
		}
		if (i >= *count)
		{
			servers[*count].adr = adr;
			String::NCpyZ(servers[*count].hostName, name, sizeof(servers[*count].hostName));
			servers[*count].visible = true;
			(*count)++;
			return 1;
		}
		return 0;
	}
	return -1;
}

void LAN_RemoveServer(int source, const char* addr)
{
	q3serverInfo_t* servers;
	int max;
	int* count;
	CLT3_GetServersForSource(source, servers, max, count);
	if (servers)
	{
		netadr_t comp;
		SOCK_StringToAdr(addr, &comp, Q3PORT_SERVER);
		for (int i = 0; i < *count; i++)
		{
			if (SOCK_CompareAdr(comp, servers[i].adr))
			{
				int j = i;
				while (j < *count - 1)
				{
					Com_Memcpy(&servers[j], &servers[j + 1], sizeof(servers[j]));
					j++;
				}
				(*count)--;
				break;
			}
		}
	}
}

int LAN_GetServerCount(int source)
{
	q3serverInfo_t* servers;
	int max;
	int* count;
	CLT3_GetServersForSource(source, servers, max, count);
	return *count;
}

void LAN_GetServerAddressString(int source, int n, char* buf, int buflen)
{
	q3serverInfo_t* server = LAN_GetServerPtr(source, n);
	if (server)
	{
		String::NCpyZ(buf, SOCK_AdrToString(server->adr), buflen);
		return;
	}
	buf[0] = '\0';
}

void LAN_GetServerInfo(int source, int n, char* buf, int buflen)
{
	q3serverInfo_t* server = LAN_GetServerPtr(source, n);

	if (server && buf)
	{
		char info[MAX_STRING_CHARS];
		info[0] = '\0';
		Info_SetValueForKey(info, "hostname", server->hostName, MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "mapname", server->mapName, MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "clients", va("%i",server->clients), MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "sv_maxclients", va("%i",server->maxClients), MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "ping", va("%i",server->ping), MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "minping", va("%i",server->minPing), MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "maxping", va("%i",server->maxPing), MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "game", server->game, MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "gametype", va("%i",server->gameType), MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "nettype", va("%i",server->netType), MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "addr", SOCK_AdrToString(server->adr), MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "punkbuster", va("%i", server->punkbuster), MAX_INFO_STRING_Q3);
		if (GGameType & (GAME_WolfMP | GAME_ET))
		{
			Info_SetValueForKey(info, "sv_allowAnonymous", va("%i", server->allowAnonymous), MAX_INFO_STRING_Q3);
			Info_SetValueForKey(info, "friendlyFire", va("%i", server->friendlyFire), MAX_INFO_STRING_Q3);					// NERVE - SMF
			Info_SetValueForKey(info, "maxlives", va("%i", server->maxlives), MAX_INFO_STRING_Q3);							// NERVE - SMF
			Info_SetValueForKey(info, "gamename", server->gameName, MAX_INFO_STRING_Q3);									// Arnout
			Info_SetValueForKey(info, "g_antilag", va("%i", server->antilag), MAX_INFO_STRING_Q3);		// TTimo
		}
		if (GGameType & GAME_WolfMP)
		{
			Info_SetValueForKey(info, "tourney", va("%i", server->tourney), MAX_INFO_STRING_Q3);						// NERVE - SMF
		}
		if (GGameType & GAME_ET)
		{
			Info_SetValueForKey(info, "serverload", va("%i", server->load), MAX_INFO_STRING_Q3);
			Info_SetValueForKey(info, "needpass", va("%i", server->needpass), MAX_INFO_STRING_Q3);							// NERVE - SMF
			Info_SetValueForKey(info, "weaprestrict", va("%i", server->weaprestrict), MAX_INFO_STRING_Q3);
			Info_SetValueForKey(info, "balancedteams", va("%i", server->balancedteams), MAX_INFO_STRING_Q3);
		}
		String::NCpyZ(buf, info, buflen);
	}
	else
	{
		if (buf)
		{
			buf[0] = '\0';
		}
	}
}

int LAN_GetServerPing(int source, int n)
{
	q3serverInfo_t* server = LAN_GetServerPtr(source, n);
	if (server)
	{
		return server->ping;
	}
	return -1;
}

void LAN_MarkServerVisible(int source, int n, qboolean visible)
{
	if (n == -1)
	{
		q3serverInfo_t* servers;
		int max;
		int* count;
		CLT3_GetServersForSource(source, servers, max, count);
		if (servers)
		{
			for (n = 0; n < max; n++)
			{
				servers[n].visible = visible;
			}
		}
	}
	else
	{
		q3serverInfo_t* server = LAN_GetServerPtr(source, n);
		if (server)
		{
			server->visible = visible;
		}
	}
}

int LAN_ServerIsVisible(int source, int n)
{
	q3serverInfo_t* server = LAN_GetServerPtr(source, n);
	if (server)
	{
		return server->visible;
	}
	return false;
}

void LAN_ResetPings(int source)
{
	q3serverInfo_t* servers;
	int max;
	int* count;
	CLT3_GetServersForSource(source, servers, max, count);
	if (servers)
	{
		for (int i = 0; i < max; i++)
		{
			servers[i].ping = -1;
		}
	}
}

int LAN_CompareServers(int source, int sortKey, int sortDir, int s1, int s2)
{
	int res;
	q3serverInfo_t* server1, * server2;
	char name1[MAX_NAME_LENGTH_ET], name2[MAX_NAME_LENGTH_ET];

	server1 = LAN_GetServerPtr(source, s1);
	server2 = LAN_GetServerPtr(source, s2);
	if (!server1 || !server2)
	{
		return 0;
	}

	res = 0;
	switch (sortKey)
	{
	case SORT_HOST:
		String::NCpyZ(name1, server1->hostName, sizeof(name1));
		String::CleanStr(name1);
		String::NCpyZ(name2, server2->hostName, sizeof(name2));
		String::CleanStr(name2);
		res = String::ICmp(name1, name2);
		break;

	case SORT_MAP:
		res = String::ICmp(server1->mapName, server2->mapName);
		break;
	case SORT_CLIENTS:
		if (server1->clients < server2->clients)
		{
			res = -1;
		}
		else if (server1->clients > server2->clients)
		{
			res = 1;
		}
		else
		{
			res = 0;
		}
		break;
	case SORT_GAME:
		if (server1->gameType < server2->gameType)
		{
			res = -1;
		}
		else if (server1->gameType > server2->gameType)
		{
			res = 1;
		}
		else
		{
			res = 0;
		}
		break;
	case SORT_PING:
		if (server1->ping < server2->ping)
		{
			res = -1;
		}
		else if (server1->ping > server2->ping)
		{
			res = 1;
		}
		else
		{
			res = 0;
		}
		break;
	case SORT_PUNKBUSTER:
		if (server1->punkbuster < server2->punkbuster)
		{
			res = -1;
		}
		else if (server1->punkbuster > server2->punkbuster)
		{
			res = 1;
		}
		else
		{
			res = 0;
		}
		break;
	}

	if (sortDir)
	{
		if (res < 0)
		{
			return 1;
		}
		if (res > 0)
		{
			return -1;
		}
		return 0;
	}
	return res;
}

void CLT3_GetClipboardData(char* buf, int buflen)
{
	char* cbd = Sys_GetClipboardData();

	if (!cbd)
	{
		*buf = 0;
		return;
	}

	String::NCpyZ(buf, cbd, buflen);

	delete[] cbd;
}

void Key_KeynumToStringBuf(int keynum, char* buf, int buflen)
{
	String::NCpyZ(buf, Key_KeynumToString(keynum, true), buflen);
}

void Key_GetBindingBuf(int keynum, char* buf, int buflen)
{
	const char* value = Key_GetBinding(keynum);
	if (value)
	{
		String::NCpyZ(buf, value, buflen);
	}
	else
	{
		*buf = 0;
	}
}

int Key_GetCatcher()
{
	return in_keyCatchers;
}

void KeyQ3_SetCatcher(int catcher)
{
	in_keyCatchers = catcher;
}

void KeyWM_SetCatcher(int catcher)
{
	// NERVE - SMF - console overrides everything
	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		in_keyCatchers = catcher | KEYCATCH_CONSOLE;
	}
	else
	{
		in_keyCatchers = catcher;
	}

}

bool CLT3_GetLimboString(int index, char* buf)
{
	if (index >= LIMBOCHAT_HEIGHT_WA)
	{
		return false;
	}

	String::NCpy(buf, cl.wa_limboChatMsgs[index], 140);
	return true;
}

void CLT3_OpenURL(const char* url)
{
	if (!url || !String::Length(url))
	{
		common->Printf("%s", CL_TranslateStringBuf("invalid/empty URL\n"));
		return;
	}
	Sys_OpenURL(url, true);
}
