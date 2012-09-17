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
#include "ui_public.h"

bool UIQ3_ConsoleCommand(int realTime)
{
	return VM_Call(uivm, Q3UI_CONSOLE_COMMAND, realTime);
}

void UIQ3_DrawConnectScreen(bool overlay)
{
	VM_Call(uivm, Q3UI_DRAW_CONNECT_SCREEN, overlay);
}

bool UIQ3_HasUniqueCDKey()
{
	return VM_Call(uivm, Q3UI_HASUNIQUECDKEY);
}
