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

enum menu_state_t
{
	m_none,
	m_main,
	m_singleplayer,
	m_singleplayer_confirm,
	m_class,
	m_difficulty,
	m_load,
	m_save,
	m_multiplayer,
	m_lanconfig,
	m_search,
	m_slist,
	m_mconnect,
	m_setup,
	m_gameoptions,
	m_mload,
	m_msave,
	m_options,
	m_keys,
	m_video,
	m_help,
	m_quit,
};

extern menu_state_t m_state;
extern menu_state_t m_return_state;

extern bool m_return_onerror;
extern char m_return_reason[32];

void MQH_DrawPic(int x, int y, image_t* pic);
void MQH_Print(int cx, int cy, const char* str);
void MQH_PrintWhite(int cx, int cy, const char* str);
void MQH_DrawTextBox(int x, int y, int width, int lines);
void MQH_DrawTextBox2(int x, int y, int width, int lines);

void MQH_Menu_Main_f();
void MQH_Menu_Quit_f();
void MQH_ToggleMenu_f();

void MQH_Init();
void MQH_InitImages();
void MQH_FadeScreen();
void MQH_Draw();
void MQH_Keydown(int key);
void MQH_CharEvent(int key);
