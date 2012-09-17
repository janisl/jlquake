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
#include "ui_shared.h"
#include "../quake3/local.h"
#include "../wolfsp/local.h"
#include "../wolfmp/local.h"
#include "../et/local.h"

vm_t* uivm;

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

void UIT3_MouseEvent(int dx, int dy)
{
	VM_Call(uivm, UI_MOUSE_EVENT, dx, dy);
}

void UIT3_Refresh(int time)
{
	VM_Call(uivm, UI_REFRESH, time);
}

bool UIT3_IsFullscreen()
{
	return VM_Call(uivm, UI_IS_FULLSCREEN);
}

void UIT3_SetActiveMenu(int menu)
{
	VM_Call(uivm, UI_SET_ACTIVE_MENU, menu);
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
