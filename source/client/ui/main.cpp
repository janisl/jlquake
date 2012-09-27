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

#include "../client.h"
#include "../game/quake_hexen2/menu.h"
#include "../game/quake2/menu.h"
#include "../game/tech3/local.h"

void UI_Init()
{
	if (GGameType & GAME_QuakeHexen)
	{
		MQH_Init();
	}
	else if (GGameType & GAME_Quake2)
	{
		MQ2_Init();
	}
	else
	{
		UIT3_Init();
	}
}

void UI_KeyEvent(int key, bool down)
{
	if (GGameType & GAME_Tech3)
	{
		UIT3_KeyEvent(key, down);
	}
}

void UI_KeyDownEvent(int key)
{
	if (GGameType & GAME_QuakeHexen)
	{
		MQH_Keydown(key);
	}
	else if (GGameType & GAME_Quake2)
	{
		MQ2_Keydown(key);
	}
	else
	{
		UIT3_KeyDownEvent(key);
	}
}

void UI_CharEvent(int key)
{
	if (GGameType & GAME_QuakeHexen)
	{
		MQH_CharEvent(key);
	}
	else if (GGameType & GAME_Quake2)
	{
		MQ2_CharEvent(key);
	}
	else
	{
		UIT3_KeyEvent(key | K_CHAR_FLAG, true);
	}
}

void UI_MouseEvent(int dx, int dy)
{
	if (GGameType & GAME_Tech3)
	{
		UIT3_MouseEvent(dx, dy);
	}
}

void UI_DrawMenu()
{
	if (GGameType & GAME_QuakeHexen)
	{
		MQH_Draw();
	}
	else if (GGameType & GAME_Quake2)
	{
		MQ2_Draw();
	}
	else
	{
		UIT3_Refresh(cls.realtime);
	}
}

bool UI_IsFullscreen()
{
	if (!(GGameType & GAME_Tech3))
	{
		return false;
	}
	return UIT3_IsFullscreen();
}

void UI_ForceMenuOff()
{
	if (GGameType & GAME_Quake2)
	{
		MQ2_ForceMenuOff();
	}
	else if (GGameType & GAME_Tech3)
	{
		UIT3_ForceMenuOff();
	}
}

void UI_SetMainMenu()
{
	if (GGameType & GAME_QuakeHexen)
	{
		MQH_Menu_Main_f();
	}
	else if (GGameType & GAME_QuakeHexen)
	{
		MQ2_Menu_Main_f();
	}
	else
	{
		UIT3_SetMainMenu();
	}
}

void UI_SetInGameMenu()
{
	UIT3_SetInGameMenu();
}
