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
extern const char* mh2_message;
extern const char* mh2_message2;
extern int mh2_message_time;
extern Cvar* mh2_oldmission;

#define OEM_START 9
#define REG_START 2
#define MP_START 7
#define DM_START 8
#define NUM_GAMEOPTIONS_Q1  9
#define NUM_GAMEOPTIONS_H2  11
extern int mqh_startepisode;
extern int mqh_startlevel;
extern int mqh_gameoptions_cursor;
extern int mqh_maxplayers;
#define NUM_SETUP_CMDS_Q1  5
#define NUM_SETUP_CMDS_H2  6
#define NUM_SETUP_CMDS_HW  7
extern int mqh_setup_cursor;
extern int setup_cursor_table_q1[];
extern int setup_cursor_table_h2[];
extern int setup_cursor_table_hw[];
extern field_t setup_hostname;
extern field_t setup_myname;
extern int setup_oldtop;
extern int setup_oldbottom;
extern int setup_top;
extern int setup_bottom;
extern int class_limit;
extern int which_class;
extern int mqh_options_cursor;
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
#define MAX_SAVEGAMES       12
#define SAVEGAME_COMMENT_LENGTH 39
extern int mqh_load_cursor;			// 0 < mqh_load_cursor < MAX_SAVEGAMES
extern char mqh_filenames[MAX_SAVEGAMES][SAVEGAME_COMMENT_LENGTH + 1];
extern bool mqh_loadable[MAX_SAVEGAMES];
#define OPTIONS_ITEMS_Q1    13
#define OPTIONS_ITEMS_QW    15
enum
{
	OPT_CUSTOMIZE = 0,
	OPT_CONSOLE,
	OPT_DEFAULTS,
	OPT_SCRSIZE,	//3
	OPT_GAMMA,		//4
	OPT_MOUSESPEED,	//5
	OPT_MUSICTYPE,	//6
	OPT_MUSICVOL,	//7
	OPT_SNDVOL,		//8
	OPT_ALWAYRUN,	//9
	OPT_INVMOUSE,	//10
	OPT_LOOKSPRING,	//11
	OPT_CROSSHAIR,	//13
	OPT_ALWAYSMLOOK,//14
	OPT_VIDEO,		//15
	OPTIONS_ITEMS
};
#define SLIDER_RANGE    10
#define NUM_HELP_PAGES_Q1   6
#define NUM_HELP_PAGES_H2   5
#define NUM_SG_HELP_PAGES   10	//Siege has more help
#define MAX_LINES2_H2 150
#define MAX_LINES2_HW 158 + 27
extern const char* Credit2TextH2[MAX_LINES2_H2];
extern const char* Credit2TextHW[MAX_LINES2_HW];
#define QUIT_SIZE_H2 18
extern int setup_class;
#define NUM_DIFFLEVELS  4
extern const char* DiffNames[NUM_CLASSES_H2MP][NUM_DIFFLEVELS];

void MQH_DrawPic(int x, int y, image_t* pic);
void MQH_DrawCharacter(int cx, int line, int num);
void MQH_Print(int cx, int cy, const char* str);
void MQH_PrintWhite(int cx, int cy, const char* str);
void MQH_DrawTextBox(int x, int y, int width, int lines);
void MQH_DrawTextBox2(int x, int y, int width, int lines);
void MQH_DrawField(int x, int y, field_t* edit, bool showCursor);
void MH2_DrawBigString(int x, int y, const char* string);
void MH2_ScrollTitle(const char* name);
void MQH_Menu_Main_f();
void MQH_Init();
void MQH_Draw();
void MQH_Menu_MultiPlayer_f();
void MQH_Menu_Options_f();
void MQH_Menu_Quit_f();
void MQH_Keydown(int key);
void MQH_Menu_Net_f();
void MQH_Menu_Setup_f();
void MH2_Menu_MLoad_f();
void MH2_Menu_MSave_f();
void MQH_Menu_GameOptions_f();
void MQH_CharEvent(int key);
