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
	m_load,
	m_save,
	m_multiplayer,
	m_setup,
	m_net,
	m_options,
	m_video,
	m_keys,
	m_help,
	m_quit,
	m_lanconfig,
	m_gameoptions,
	m_search,
	m_slist,
	m_class,
	m_difficulty,
	m_mload,
	m_msave,
	m_mconnect
};

extern menu_state_t m_state;
extern menu_state_t m_return_state;

extern bool m_return_onerror;
extern char m_return_reason[32];

extern image_t* char_menufonttexture;

extern float TitlePercent;
extern float TitleTargetPercent;
extern float LogoPercent;
extern float LogoTargetPercent;
extern bool mqh_entersound;
extern Cvar* mh2_oldmission;

extern int mqh_help_page;
extern bool wasInMenus;
extern int msgNumber;
extern menu_state_t m_quit_prevstate;
extern const char* mq1_quitMessage [];
extern float LinePos;
extern int LineTimes;
extern int MaxLines;
extern const char** LineText;
extern bool SoundPlayed;
#define NUM_HELP_PAGES_Q1   6
#define NUM_HELP_PAGES_H2   5
#define NUM_SG_HELP_PAGES   10	//Siege has more help
#define MAX_LINES2_H2 150
#define MAX_LINES2_HW 158 + 27
extern const char* Credit2TextH2[MAX_LINES2_H2];
extern const char* Credit2TextHW[MAX_LINES2_HW];
#define QUIT_SIZE_H2 18
extern image_t* mq1_translate_texture;
extern image_t* mh2_translate_texture[MAX_PLAYER_CLASS];
#define MAX_COLUMN_SIZE     9
#define MODE_AREA_HEIGHT    (MAX_COLUMN_SIZE + 2)

void MQH_DrawPic(int x, int y, image_t* pic);
void MQH_DrawCharacter(int cx, int line, int num);
void MQH_Print(int cx, int cy, const char* str);
void MQH_PrintWhite(int cx, int cy, const char* str);
void MQH_DrawTextBox(int x, int y, int width, int lines);
void MQH_DrawTextBox2(int x, int y, int width, int lines);
void MH2_ScrollTitle(const char* name);
void MQH_Menu_Main_f();
void MQH_Init();
void MQH_Draw();
void MQH_Menu_Options_f();
void MQH_Menu_Quit_f();
void MQH_Keydown(int key);
void MQH_CharEvent(int key);
