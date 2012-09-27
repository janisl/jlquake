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

#ifndef _CGAME_WOLFSP_LOCAL_H
#define _CGAME_WOLFSP_LOCAL_H

#include "../tech3/local.h"

bool CLWS_GetTag(int clientNum, const char* tagname, orientation_t* _or);
qintptr CLWS_CgameSystemCalls(qintptr* args);

int UIWS_GetActiveMenu();
bool UIWS_ConsoleCommand(int realTime);
void UIWS_DrawConnectScreen(bool overlay);
bool UIWS_HasUniqueCDKey();
void UIWS_SetEndGameMenu();
void UIWS_KeyDownEvent(int key, bool down);
void UIWS_Init();
qintptr CLWS_UISystemCalls(qintptr* args);

#endif
