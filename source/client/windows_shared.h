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

#ifndef _WIN_SHARED_H
#define _WIN_SHARED_H

#include "../core/system_windows.h"

void SNDDMA_Activate();

LONG CDAudio_MessageHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

bool IN_HandleInputMessage(UINT uMsg, WPARAM  wParam, LPARAM  lParam);
void IN_Activate(bool active);

extern HWND			GMainWindow;
extern Cvar*		in_joystick;
extern bool			Minimized;
extern bool			ActiveApp;

#endif
