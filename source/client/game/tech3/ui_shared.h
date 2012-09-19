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

#ifndef _TECH3_UI_SHARED_H
#define _TECH3_UI_SHARED_H

struct uiClientState_t
{
	connstate_t connState;
	int connectPacketCount;
	int clientNum;
	char servername[MAX_STRING_CHARS];
	char updateInfoString[MAX_STRING_CHARS];
	char messageString[MAX_STRING_CHARS];
};

#define SORT_HOST           0
#define SORT_MAP            1
#define SORT_CLIENTS        2
#define SORT_GAME           3
#define SORT_PING           4
#define SORT_PUNKBUSTER     5

enum
{
	UIMENU_NONE,
	UIMENU_MAIN,
	UIMENU_INGAME,
	UIMENU_NEED_CD
};

enum
{
	UI_GETAPIVERSION = 0,	// system reserved

	UI_INIT,
//	void UI_Init();

	UI_SHUTDOWN,
//	void UI_Shutdown();

	UI_KEY_EVENT,
//	void UI_KeyEvent(int key);

	UI_MOUSE_EVENT,
//	void UI_MouseEvent(int dx, int dy);

	UI_REFRESH,
//	void UI_Refresh(int time);

	UI_IS_FULLSCREEN,
//	qboolean UI_IsFullscreen();

	UI_SET_ACTIVE_MENU,
//	void UI_SetActiveMenu(uiMenuCommand_t menu);
};

void UIT3_GetClientState(uiClientState_t* state);
void LAN_LoadCachedServers();
void LAN_SaveServersToCache();
q3serverInfo_t* LAN_GetServerPtr(int source, int n);
int LAN_AddServer(int source, const char* name, const char* address);
void LAN_RemoveServer(int source, const char* addr);
int LAN_GetServerCount(int source);
void LAN_GetServerAddressString(int source, int n, char* buf, int buflen);
void LAN_GetServerInfo(int source, int n, char* buf, int buflen);
int LAN_GetServerPing(int source, int n);
void LAN_MarkServerVisible(int source, int n, qboolean visible);
int LAN_ServerIsVisible(int source, int n);
void LAN_ResetPings(int source);
int LAN_CompareServers(int source, int sortKey, int sortDir, int s1, int s2);
void CLT3_GetClipboardData(char* buf, int buflen);
void CLT3UI_GetCDKey(char* buf, int buflen);
void CLT3UI_SetCDKey(char* buf);

#endif
