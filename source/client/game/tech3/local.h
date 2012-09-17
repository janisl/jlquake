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

//
//	UI
//
extern vm_t* uivm;				// interface to ui dll or vm

void CLT3_ShutdownUI();
void UIT3_KeyEvent(int key, bool down);
void UIT3_MouseEvent(int dx, int dy);
void UIT3_Refresh(int time);
bool UIT3_IsFullscreen();
void UIT3_SetActiveMenu(int menu);
void UIT3_DrawConnectScreen(bool overlay);
bool UIT3_UsesUniqueCDKey();
bool UIT3_CheckKeyExec(int key);

#endif
