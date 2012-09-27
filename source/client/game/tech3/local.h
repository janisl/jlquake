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

#ifndef _CGAME_TECH3_LOCAL_H
#define _CGAME_TECH3_LOCAL_H

//
//	CGame
//
void CLT3_ShutdownCGame();
void CLT3_CGameRendering(stereoFrame_t stereo);
int CLT3_CrosshairPlayer();
int CLT3_LastAttacker();
void CLT3_KeyEvent(int key, bool down);
void CLT3_MouseEvent(int dx, int dy);
void CLT3_EventHandling();
void Key_GetBindingBuf(int keynum, char* buf, int buflen);
void Key_KeynumToStringBuf(int keynum, char* buf, int buflen);
int Key_GetCatcher();
void KeyQ3_SetCatcher(int catcher);
void KeyWM_SetCatcher(int catcher);

//
//	Main
//
#define RETRANSMIT_TIMEOUT  3000	// time between connection packet retransmits

extern Cvar* clet_profile;

//
//	ServerList
//
int CLT3_ServerStatus(char* serverAddress, char* serverStatusString, int maxLen);
void CLT3_ServerStatusResponse(netadr_t from, QMsg* msg);
int CLT3_GetPingQueueCount();
void CLT3_ClearPing(int n);
void CLT3_GetPing(int n, char* buf, int buflen, int* pingtime);
void CLT3_GetPingInfo(int n, char* buf, int buflen);
bool CLT3_UpdateVisiblePings(int source);
void CLT3_ServerInfoPacket(netadr_t from, QMsg* msg);
void CLT3_ServersResponsePacket(netadr_t from, QMsg* msg);
void CLT3_InitServerLists();

//
//	UI
//
extern vm_t* uivm;				// interface to ui dll or vm

void UIT3_Init();
void CLT3_ShutdownUI();
void UIT3_KeyEvent(int key, bool down);
void UIT3_KeyDownEvent(int key);
void UIT3_MouseEvent(int dx, int dy);
void UIT3_Refresh(int time);
bool UIT3_IsFullscreen();
void UIT3_SetActiveMenu(int menu);
void UIT3_ForceMenuOff();
void UIT3_SetMainMenu();
void UIT3_SetInGameMenu();
void UIT3_DrawConnectScreen(bool overlay);
bool UIT3_UsesUniqueCDKey();
bool UIT3_CheckKeyExec(int key);
void CLT3_GetServersForSource(int source, q3serverInfo_t*& servers, int& max, int*& count);

#endif
