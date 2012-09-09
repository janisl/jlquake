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
#include "menu.h"
#include "../../../server/public.h"
#include "../hexen2/local.h"

menu_state_t m_state;
menu_state_t m_return_state;

bool m_return_onerror;
char m_return_reason[32];

image_t* char_menufonttexture;
static char BigCharWidth[27][27];

float TitlePercent = 0;
float TitleTargetPercent = 1;
float LogoPercent = 0;
float LogoTargetPercent = 1;

bool mqh_entersound;			// play after drawing a frame, so caching
								// won't disrupt the sound

const char* mh2_message;
const char* mh2_message2;

Cvar* mh2_oldmission;

int setup_class;

static void MH2_Menu_Class_f();
static void MH2_Menu_Difficulty_f();
static void MQH_Menu_Help_f();

void MQH_DrawPic(int x, int y, image_t* pic)
{
	UI_DrawPic(x + ((viddef.width - 320) >> 1), y, pic);
}

//	Draws one solid graphics character
void MQH_DrawCharacter(int cx, int line, int num)
{
	UI_DrawChar(cx + ((viddef.width - 320) >> 1), line, num);
}

void MQH_Print(int cx, int cy, const char* str)
{
	UI_DrawString(cx + ((viddef.width - 320) >> 1), cy, str, GGameType & GAME_Hexen2 ? 256 : 128);
}

void MQH_PrintWhite(int cx, int cy, const char* str)
{
	UI_DrawString(cx + ((viddef.width - 320) >> 1), cy, str);
}

void MQH_DrawTextBox(int x, int y, int width, int lines)
{
	// draw left side
	int cx = x;
	int cy = y;
	image_t* p = R_CachePic("gfx/box_tl.lmp");
	MQH_DrawPic(cx, cy, p);
	p = R_CachePic("gfx/box_ml.lmp");
	for (int n = 0; n < lines; n++)
	{
		cy += 8;
		MQH_DrawPic(cx, cy, p);
	}
	p = R_CachePic("gfx/box_bl.lmp");
	MQH_DrawPic(cx, cy + 8, p);

	// draw middle
	cx += 8;
	while (width > 0)
	{
		cy = y;
		p = R_CachePic("gfx/box_tm.lmp");
		MQH_DrawPic(cx, cy, p);
		p = R_CachePic("gfx/box_mm.lmp");
		for (int n = 0; n < lines; n++)
		{
			cy += 8;
			if (n == 1)
			{
				p = R_CachePic("gfx/box_mm2.lmp");
			}
			MQH_DrawPic(cx, cy, p);
		}
		p = R_CachePic("gfx/box_bm.lmp");
		MQH_DrawPic(cx, cy + 8, p);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = R_CachePic("gfx/box_tr.lmp");
	MQH_DrawPic(cx, cy, p);
	p = R_CachePic("gfx/box_mr.lmp");
	for (int n = 0; n < lines; n++)
	{
		cy += 8;
		MQH_DrawPic(cx, cy, p);
	}
	p = R_CachePic("gfx/box_br.lmp");
	MQH_DrawPic(cx, cy + 8, p);
}

void MQH_DrawTextBox2(int x, int y, int width, int lines)
{
	MQH_DrawTextBox(x,  y + ((viddef.height - 200) >> 1), width, lines);
}

void MQH_DrawField(int x, int y, field_t* edit, bool showCursor)
{
	MQH_DrawTextBox(x - 8, y - 8, edit->widthInChars, 1);
	Field_Draw(edit, x + ((viddef.width - 320) >> 1), y, showCursor);
}

static void MH2_ReadBigCharWidth()
{
	void* data;
	FS_ReadFile("gfx/menu/fontsize.lmp", &data);
	Com_Memcpy(BigCharWidth, data, sizeof(BigCharWidth));
	FS_FreeFile(data);
}

static int MH2_DrawBigCharacter(int x, int y, int num, int numNext)
{
	if (num == ' ')
	{
		return 32;
	}

	if (num == '/')
	{
		num = 26;
	}
	else
	{
		num -= 65;
	}

	if (num < 0 || num >= 27)	// only a-z and /
	{
		return 0;
	}

	if (numNext == '/')
	{
		numNext = 26;
	}
	else
	{
		numNext -= 65;
	}

	UI_DrawCharBase(x, y, num, 20, 20, char_menufonttexture, 8, 4);

	if (numNext < 0 || numNext >= 27)
	{
		return 0;
	}

	int add = 0;
	if (num == (int)'C' - 65 && numNext == (int)'P' - 65)
	{
		add = 3;
	}

	return BigCharWidth[num][numNext] + add;
}

void MH2_DrawBigString(int x, int y, const char* string)
{
	x += ((viddef.width - 320) >> 1);

	int length = String::Length(string);
	for (int c = 0; c < length; c++)
	{
		x += MH2_DrawBigCharacter(x, y, string[c], string[c + 1]);
	}
}

void MH2_ScrollTitle(const char* name)
{
	static const char* LastName = "";
	static bool CanSwitch = true;

	float delta;
	if (TitlePercent < TitleTargetPercent)
	{
		delta = ((TitleTargetPercent - TitlePercent) / 0.5) * cls.frametime * 0.001;
		if (delta < 0.004)
		{
			delta = 0.004;
		}
		TitlePercent += delta;
		if (TitlePercent > TitleTargetPercent)
		{
			TitlePercent = TitleTargetPercent;
		}
	}
	else if (TitlePercent > TitleTargetPercent)
	{
		delta = ((TitlePercent - TitleTargetPercent) / 0.15) * cls.frametime * 0.001;
		if (delta < 0.02)
		{
			delta = 0.02;
		}
		TitlePercent -= delta;
		if (TitlePercent <= TitleTargetPercent)
		{
			TitlePercent = TitleTargetPercent;
			CanSwitch = true;
		}
	}

	if (LogoPercent < LogoTargetPercent)
	{
		delta = ((LogoTargetPercent - LogoPercent) / 0.15) * cls.frametime * 0.001;
		if (delta < 0.02)
		{
			delta = 0.02;
		}
		LogoPercent += delta;
		if (LogoPercent > LogoTargetPercent)
		{
			LogoPercent = LogoTargetPercent;
		}
	}

	if (String::ICmp(LastName,name) != 0 && TitleTargetPercent != 0)
	{
		TitleTargetPercent = 0;
	}

	if (CanSwitch)
	{
		LastName = name;
		CanSwitch = false;
		TitleTargetPercent = 1;
	}

	image_t* p = R_CachePic(LastName);
	int finaly = ((float)R_GetImageHeight(p) * TitlePercent) - R_GetImageHeight(p);
	MQH_DrawPic((320 - R_GetImageWidth(p)) / 2, finaly, p);

	if (m_state != m_keys)
	{
		p = R_CachePic("gfx/menu/hplaque.lmp");
		finaly = ((float)R_GetImageHeight(p) * LogoPercent) - R_GetImageHeight(p);
		MQH_DrawPic(10, finaly, p);
	}
}

//=============================================================================
/* MAIN MENU */

#define MAIN_ITEMS      5
#define MAIN_ITEMS_HW   4

static int mqh_main_cursor;
int mqh_save_demonum;

void MQH_Menu_Main_f()
{
	if (!(in_keyCatchers & KEYCATCH_UI))
	{
		mqh_save_demonum = cls.qh_demonum;
		cls.qh_demonum = -1;
	}
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_main;
	mqh_entersound = true;
}

static void MQH_Main_Draw()
{
	if (GGameType & GAME_Hexen2)
	{
		MH2_ScrollTitle("gfx/menu/title0.lmp");
		if (GGameType & GAME_HexenWorld)
		{
			MH2_DrawBigString(72, 60 + (0 * 20), "MULTIPLAYER");
			MH2_DrawBigString(72, 60 + (1 * 20), "OPTIONS");
			MH2_DrawBigString(72, 60 + (2 * 20), "HELP");
			MH2_DrawBigString(72, 60 + (3 * 20), "QUIT");
		}
		else
		{
			MH2_DrawBigString(72, 60 + (0 * 20), "SINGLE PLAYER");
			MH2_DrawBigString(72, 60 + (1 * 20), "MULTIPLAYER");
			MH2_DrawBigString(72, 60 + (2 * 20), "OPTIONS");
			MH2_DrawBigString(72, 60 + (3 * 20), "HELP");
			MH2_DrawBigString(72, 60 + (4 * 20), "QUIT");
		}

		int f = (cls.realtime / 100) % 8;
		MQH_DrawPic(43, 54 + mqh_main_cursor * 20,R_CachePic(va("gfx/menu/menudot%i.lmp", f + 1)));
	}
	else
	{
		MQH_DrawPic(16, 4, R_CachePic("gfx/qplaque.lmp"));
		image_t* p = R_CachePic("gfx/ttl_main.lmp");
		MQH_DrawPic((320 - R_GetImageWidth(p)) / 2, 4, p);
		MQH_DrawPic(72, 32, R_CachePic("gfx/mainmenu.lmp"));

		int f = (cls.realtime / 100) % 6;

		MQH_DrawPic(54, 32 + mqh_main_cursor * 20,R_CachePic(va("gfx/menudot%i.lmp", f + 1)));
	}
}

static void MQH_Main_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
		in_keyCatchers &= ~KEYCATCH_UI;
		m_state = m_none;
		cls.qh_demonum = mqh_save_demonum;
		if (cls.qh_demonum != -1 && !clc.demoplaying && cls.state == CA_DISCONNECTED)
		{
			CL_NextDemo();
		}
		break;

	case K_DOWNARROW:
		S_StartLocalSound(GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav");
		if (++mqh_main_cursor >= (GGameType & GAME_HexenWorld ? MAIN_ITEMS_HW : MAIN_ITEMS))
		{
			mqh_main_cursor = 0;
		}
		break;

	case K_UPARROW:
		S_StartLocalSound(GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav");
		if (--mqh_main_cursor < 0)
		{
			mqh_main_cursor = (GGameType & GAME_HexenWorld ? MAIN_ITEMS_HW : MAIN_ITEMS) - 1;
		}
		break;

	case K_ENTER:
		mqh_entersound = true;

		if (GGameType & GAME_HexenWorld)
		{
			switch (mqh_main_cursor)
			{
			case 4:
				MQH_Menu_SinglePlayer_f();
				break;

			case 0:
				MQH_Menu_MultiPlayer_f();
				break;

			case 1:
				MQH_Menu_Options_f();
				break;

			case 2:
				MQH_Menu_Help_f();
				break;

			case 3:
				MQH_Menu_Quit_f();
				break;
			}
		}
		else
		{
			switch (mqh_main_cursor)
			{
			case 0:
				MQH_Menu_SinglePlayer_f();
				break;

			case 1:
				MQH_Menu_MultiPlayer_f();
				break;

			case 2:
				MQH_Menu_Options_f();
				break;

			case 3:
				MQH_Menu_Help_f();
				break;

			case 4:
				MQH_Menu_Quit_f();
				break;
			}
		}
	}
}

//=============================================================================
/* SINGLE PLAYER MENU */

#define SINGLEPLAYER_ITEMS      3
#define SINGLEPLAYER_ITEMS_H2MP 5

static int mqh_singleplayer_cursor;
bool mh2_enter_portals;

void MQH_Menu_SinglePlayer_f()
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_singleplayer;
	if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)))
	{
		mqh_entersound = true;
		if (GGameType & GAME_Hexen2)
		{
			Cvar_SetValue("timelimit", 0);		//put this here to help play single after dm
		}
	}
}

static void MQH_SinglePlayer_Draw()
{
	if (GGameType & GAME_Hexen2)
	{
		MH2_ScrollTitle("gfx/menu/title1.lmp");

		if (GGameType & GAME_HexenWorld)
		{
			MQH_DrawTextBox(60, 10 * 8, 23, 4);
			MQH_PrintWhite(92, 12 * 8, "HexenWorld is for");
			MQH_PrintWhite(88, 13 * 8, "Internet play only");
		}
		else
		{
			MH2_DrawBigString(72,60 + (0 * 20),"NEW MISSION");
			MH2_DrawBigString(72,60 + (1 * 20),"LOAD");
			MH2_DrawBigString(72,60 + (2 * 20),"SAVE");
			if (GGameType & GAME_H2Portals)
			{
				if (mh2_oldmission->value)
				{
					MH2_DrawBigString(72,60 + (3 * 20),"OLD MISSION");
				}
				MH2_DrawBigString(72,60 + (4 * 20),"VIEW INTRO");
			}

			int f = (cls.realtime / 100) % 8;
			MQH_DrawPic(43, 54 + mqh_singleplayer_cursor * 20,R_CachePic(va("gfx/menu/menudot%i.lmp", f + 1)));
		}
	}
	else
	{
		MQH_DrawPic(16, 4, R_CachePic("gfx/qplaque.lmp"));
		image_t* p = R_CachePic("gfx/ttl_sgl.lmp");
		MQH_DrawPic((320 - R_GetImageWidth(p)) / 2, 4, p);
		if (GGameType & GAME_QuakeWorld)
		{
			MQH_DrawTextBox(60, 10 * 8, 23, 4);
			MQH_PrintWhite(92, 12 * 8, "QuakeWorld is for");
			MQH_PrintWhite(88, 13 * 8, "Internet play only");
		}
		else
		{
			MQH_DrawPic(72, 32, R_CachePic("gfx/sp_menu.lmp"));

			int f = (cls.realtime / 100) % 6;
			MQH_DrawPic(54, 32 + mqh_singleplayer_cursor * 20,R_CachePic(va("gfx/menudot%i.lmp", f + 1)));
		}
	}
}

static void MQH_SinglePlayer_Key(int key)
{
	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		if (key == K_ESCAPE || key == K_ENTER)
		{
			m_state = m_main;
		}
		return;
	}

	switch (key)
	{
	case K_ESCAPE:
		MQH_Menu_Main_f();
		break;

	case K_DOWNARROW:
		S_StartLocalSound(GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav");
		if (++mqh_singleplayer_cursor >= (GGameType & GAME_H2Portals ? SINGLEPLAYER_ITEMS_H2MP : SINGLEPLAYER_ITEMS))
		{
			mqh_singleplayer_cursor = 0;
		}
		if (GGameType & GAME_H2Portals && !mh2_oldmission->value)
		{
			if (mqh_singleplayer_cursor == 3)
			{
				mqh_singleplayer_cursor = 4;
			}
		}
		break;

	case K_UPARROW:
		S_StartLocalSound(GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav");
		if (--mqh_singleplayer_cursor < 0)
		{
			mqh_singleplayer_cursor = (GGameType & GAME_H2Portals ? SINGLEPLAYER_ITEMS_H2MP : SINGLEPLAYER_ITEMS) - 1;
		}
		if (GGameType & GAME_H2Portals && !mh2_oldmission->value)
		{
			if (mqh_singleplayer_cursor == 3)
			{
				mqh_singleplayer_cursor = 2;
			}
		}
		break;

	case K_ENTER:
		mqh_entersound = true;

		mh2_enter_portals = false;
		switch (mqh_singleplayer_cursor)
		{
		case 0:
			if (GGameType & GAME_H2Portals)
			{
				mh2_enter_portals = true;
			}

		case 3:
			if (SV_IsServerActive())
			{
				if (!SCR_ModalMessage("Are you sure you want to\nstart a new game?\n"))
				{
					break;
				}
			}
			in_keyCatchers &= ~KEYCATCH_UI;
			if (SV_IsServerActive())
			{
				Cbuf_AddText("disconnect\n");
			}
			Cbuf_AddText("maxplayers 1\n");
			if (GGameType & GAME_Hexen2)
			{
				SVH2_RemoveGIPFiles(NULL);
				MH2_Menu_Class_f();
			}
			else
			{
				Cbuf_AddText("map start\n");
			}
			break;

		case 1:
			MQH_Menu_Load_f();
			break;

		case 2:
			MQH_Menu_Save_f();
			break;

		case 4:
			in_keyCatchers &= ~KEYCATCH_UI;
			Cbuf_AddText("playdemo t9\n");
			break;
		}
	}
}

//=============================================================================
/* CLASS CHOICE MENU */

static int mh2_class_flag;
static int mqh_class_cursor;

static const char* h2_ClassNamesU[NUM_CLASSES_H2MP] =
{
	"PALADIN",
	"CRUSADER",
	"NECROMANCER",
	"ASSASSIN",
	"DEMONESS"
};

static const char* hw_ClassNamesU[MAX_PLAYER_CLASS] =
{
	"PALADIN",
	"CRUSADER",
	"NECROMANCER",
	"ASSASSIN",
	"SUCCUBUS",
	"DWARF"
};

static void MH2_Menu_Class_f()
{
	mh2_class_flag = 0;
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_class;
}

static void MH2_Menu_Class2_f()
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_class;
	mh2_class_flag = 1;
}

static void MH2_Class_Draw()
{
	MH2_ScrollTitle("gfx/menu/title2.lmp");

	for (int i = 0; i < (GGameType & GAME_HexenWorld ? MAX_PLAYER_CLASS : GGameType & GAME_H2Portals ? NUM_CLASSES_H2MP : NUM_CLASSES_H2); i++)
	{
		MH2_DrawBigString(72, 60 + (i * 20), GGameType & GAME_HexenWorld ? hw_ClassNamesU[i] : h2_ClassNamesU[i]);
	}

	int f = (cls.realtime / 100) % 8;
	MQH_DrawPic(43, 54 + mqh_class_cursor * 20,R_CachePic(va("gfx/menu/menudot%i.lmp", f + 1)));

	MQH_DrawPic(251,54 + 21, R_CachePic(va("gfx/cport%d.lmp", mqh_class_cursor + 1)));
	MQH_DrawPic(242,54, R_CachePic("gfx/menu/frame.lmp"));
}

static void MH2_Class_Key(int key)
{
	switch (key)
	{
	case K_LEFTARROW:
	case K_RIGHTARROW:
		break;

	case K_ESCAPE:
		MQH_Menu_SinglePlayer_f();
		break;

	case K_DOWNARROW:
		S_StartLocalSound("raven/menu1.wav");
		if (++mqh_class_cursor >= (GGameType & GAME_HexenWorld ? MAX_PLAYER_CLASS : GGameType & GAME_H2Portals ? NUM_CLASSES_H2MP : NUM_CLASSES_H2))
		{
			mqh_class_cursor = 0;
		}
		break;

	case K_UPARROW:
		S_StartLocalSound("raven/menu1.wav");
		if (--mqh_class_cursor < 0)
		{
			mqh_class_cursor = (GGameType & GAME_HexenWorld ? MAX_PLAYER_CLASS : GGameType & GAME_H2Portals ? NUM_CLASSES_H2MP : NUM_CLASSES_H2) - 1;
		}
		break;

	case K_ENTER:
		Cbuf_AddText(va("playerclass %d\n", mqh_class_cursor + 1));
		mqh_entersound = true;
		if (!mh2_class_flag)
		{
			MH2_Menu_Difficulty_f();
		}
		else
		{
			in_keyCatchers &= ~KEYCATCH_UI;
			m_state = m_none;
		}
		break;
	default:
		in_keyCatchers &= ~KEYCATCH_UI;
		m_state = m_none;
		break;
	}
}

//=============================================================================
/* DIFFICULTY MENU */

static int mh2_diff_cursor;

const char* DiffNames[NUM_CLASSES_H2MP][NUM_DIFFLEVELS] =
{
	{	// Paladin
		"APPRENTICE",
		"SQUIRE",
		"ADEPT",
		"LORD"
	},
	{	// Crusader
		"GALLANT",
		"HOLY AVENGER",
		"DIVINE HERO",
		"LEGEND"
	},
	{	// Necromancer
		"SORCERER",
		"DARK SERVANT",
		"WARLOCK",
		"LICH KING"
	},
	{	// Assassin
		"ROGUE",
		"CUTTHROAT",
		"EXECUTIONER",
		"WIDOW MAKER"
	},
	{	// Demoness
		"LARVA",
		"SPAWN",
		"FIEND",
		"SHE BITCH"
	}
};

static void MH2_Menu_Difficulty_f()
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_difficulty;
}

static void MH2_Difficulty_Draw()
{
	MH2_ScrollTitle("gfx/menu/title5.lmp");

	setup_class = clh2_playerclass->value;

	if (setup_class < 1 || setup_class > (GGameType & GAME_H2Portals ? NUM_CLASSES_H2MP : NUM_CLASSES_H2))
	{
		setup_class = GGameType & GAME_H2Portals ? NUM_CLASSES_H2MP : NUM_CLASSES_H2;
	}
	setup_class--;

	for (int i = 0; i < NUM_DIFFLEVELS; ++i)
	{
		MH2_DrawBigString(72, 60 + (i * 20), DiffNames[setup_class][i]);
	}

	int f = (int)(cls.realtime / 100) % 8;
	MQH_DrawPic(43, 54 + mh2_diff_cursor * 20, R_CachePic(va("gfx/menu/menudot%i.lmp", f + 1)));
}

static void MH2_Difficulty_Key(int key)
{
	switch (key)
	{
	case K_LEFTARROW:
	case K_RIGHTARROW:
		break;
	case K_ESCAPE:
		MH2_Menu_Class_f();
		break;

	case K_DOWNARROW:
		S_StartLocalSound("raven/menu1.wav");
		if (++mh2_diff_cursor >= NUM_DIFFLEVELS)
		{
			mh2_diff_cursor = 0;
		}
		break;

	case K_UPARROW:
		S_StartLocalSound("raven/menu1.wav");
		if (--mh2_diff_cursor < 0)
		{
			mh2_diff_cursor = NUM_DIFFLEVELS - 1;
		}
		break;
	case K_ENTER:
		Cvar_SetValue("skill", mh2_diff_cursor);
		mqh_entersound = true;
		if (mh2_enter_portals)
		{
			h2_introTime = 0.0;
			cl.qh_intermission = 12;
			cl.qh_completed_time = cl.qh_serverTimeFloat;
			in_keyCatchers &= ~KEYCATCH_UI;
			m_state = m_none;
			cls.qh_demonum = mqh_save_demonum;

			//Cbuf_AddText ("map keep1\n");
		}
		else
		{
			Cbuf_AddText("map demo1\n");
		}
		break;
	default:
		in_keyCatchers &= ~KEYCATCH_UI;
		m_state = m_none;
		break;
	}
}

//=============================================================================
/* LOAD/SAVE MENU */

int mqh_load_cursor;			// 0 < mqh_load_cursor < MAX_SAVEGAMES

char mqh_filenames[MAX_SAVEGAMES][SAVEGAME_COMMENT_LENGTH + 1];
bool mqh_loadable[MAX_SAVEGAMES];

void MQH_ScanSaves()
{
	int i, j;
	char name[MAX_OSPATH];
	fileHandle_t f;

	for (i = 0; i < MAX_SAVEGAMES; i++)
	{
		String::Cpy(mqh_filenames[i], "--- UNUSED SLOT ---");
		mqh_loadable[i] = false;
		if (GGameType & GAME_Hexen2)
		{
			sprintf(name, "s%i/info.dat", i);
		}
		else
		{
			sprintf(name, "s%i.sav", i);
		}
		//	This will make sure that only savegames in current game directory
		// in home directory are listed
		if (!FS_FileExists(name))
		{
			continue;
		}
		FS_FOpenFileRead(name, &f, true);
		if (!f)
		{
			continue;
		}
		FS_Read(name, 80, f);
		name[80] = 0;
		//	Skip version
		char* Ptr = name;
		while (*Ptr && *Ptr != '\n')
			Ptr++;
		if (*Ptr == '\n')
		{
			*Ptr = 0;
			Ptr++;
		}
		char* SaveName = Ptr;
		while (*Ptr && *Ptr != '\n')
			Ptr++;
		*Ptr = 0;
		String::NCpy(mqh_filenames[i], SaveName, sizeof(mqh_filenames[i]) - 1);

		// change _ back to space
		for (j = 0; j < SAVEGAME_COMMENT_LENGTH; j++)
			if (mqh_filenames[i][j] == '_')
			{
				mqh_filenames[i][j] = ' ';
			}
		mqh_loadable[i] = true;
		FS_FCloseFile(f);
	}
}

void MQH_Menu_Load_f()
{
	mqh_entersound = true;
	m_state = m_load;
	in_keyCatchers |= KEYCATCH_UI;
	MQH_ScanSaves();
}

void MQH_Menu_Save_f()
{
	if (!SV_IsServerActive())
	{
		return;
	}
	if (cl.qh_intermission)
	{
		return;
	}
	if (SVQH_GetMaxClients() != 1)
	{
		return;
	}
	mqh_entersound = true;
	m_state = m_save;
	in_keyCatchers |= KEYCATCH_UI;
	MQH_ScanSaves();
}

//=============================================================================
/* MULTIPLAYER MENU */

int mqh_multiplayer_cursor;

void MQH_Menu_MultiPlayer_f()
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_multiplayer;
	if (!(GGameType & GAME_QuakeWorld))
	{
		mqh_entersound = true;
	}

	mh2_message = NULL;
}

//=============================================================================
/* OPTIONS MENU */

int mqh_options_cursor;

void MQH_Menu_Options_f()
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_options;
	mqh_entersound = true;
}

//=============================================================================
/* HELP MENU */

int mqh_help_page;

static void MQH_Menu_Help_f()
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_help;
	mqh_entersound = true;
	mqh_help_page = 0;
}

//=============================================================================
/* QUIT MENU */

bool wasInMenus;
int msgNumber;
menu_state_t m_quit_prevstate;

float LinePos;
int LineTimes;
int MaxLines;
const char** LineText;
bool SoundPlayed;

const char* mq1_quitMessage [] =
{
/* .........1.........2.... */
	"  Are you gonna quit    ",
	"  this game just like   ",
	"   everything else?     ",
	"                        ",

	" Milord, methinks that  ",
	"   thou art a lowly     ",
	" quitter. Is this true? ",
	"                        ",

	" Do I need to bust your ",
	"  face open for trying  ",
	"        to quit?        ",
	"                        ",

	" Man, I oughta smack you",
	"   for trying to quit!  ",
	"     Press Y to get     ",
	"      smacked out.      ",

	" Press Y to quit like a ",
	"   big loser in life.   ",
	"  Press N to stay proud ",
	"    and successful!     ",

	"   If you press Y to    ",
	"  quit, I will summon   ",
	"  Satan all over your   ",
	"      hard drive!       ",

	"  Um, Asmodeus dislikes ",
	" his children trying to ",
	" quit. Press Y to return",
	"   to your Tinkertoys.  ",

	"  If you quit now, I'll ",
	"  throw a blanket-party ",
	"   for you next time!   ",
	"                        "
};

#define MAX_LINES_H2 138
static const char* CreditTextH2[MAX_LINES_H2] =
{
	"Project Director: James Monroe",
	"Creative Director: Brian Raffel",
	"Project Coordinator: Kevin Schilder",
	"",
	"Lead Programmer: James Monroe",
	"",
	"Programming:",
	"   Mike Gummelt",
	"   Josh Weier",
	"",
	"Additional Programming:",
	"   Josh Heitzman",
	"   Nathan Albury",
	"   Rick Johnson",
	"",
	"Assembly Consultant:",
	"   Mr. John Scott",
	"",
	"Lead Design: Jon Zuk",
	"",
	"Design:",
	"   Tom Odell",
	"   Jeremy Statz",
	"   Mike Renner",
	"   Eric Biessman",
	"   Kenn Hoekstra",
	"   Matt Pinkston",
	"   Bobby Duncanson",
	"   Brian Raffel",
	"",
	"Art Director: Les Dorscheid",
	"",
	"Art:",
	"   Kim Lathrop",
	"   Gina Garren",
	"   Joe Koberstein",
	"   Kevin Long",
	"   Jeff Butler",
	"   Scott Rice",
	"   John Payne",
	"   Steve Raffel",
	"",
	"Animation:",
	"   Eric Turman",
	"   Chaos (Mike Werckle)",
	"",
	"Music:",
	"   Kevin Schilder",
	"",
	"Sound:",
	"   Chia Chin Lee",
	"",
	"Activision",
	"",
	"Producer:",
	"   Steve Stringer",
	"",
	"Marketing Product Manager:",
	"   Henk Hartong",
	"",
	"Marketing Associate:",
	"   Kevin Kraff",
	"",
	"Senior Quality",
	"Assurance Lead:",
	"   Tim Vanlaw",
	"",
	"Quality Assurance Lead:",
	"   Doug Jacobs",
	"",
	"Quality Assurance Team:",
	"   Steve Rosenthal, Steve Elwell,",
	"   Chad Bordwell, David Baker,",
	"   Aaron Casillas, Damien Fischer,",
	"   Winnie Lee, Igor Krinitskiy,",
	"   Samantha Lee, John Park",
	"   Ian Stevens, Chris Toft",
	"",
	"Production Testers:",
	"   Steve Rosenthal and",
	"   Chad Bordwell",
	"",
	"Additional QA and Support:",
	"    Tony Villalobos",
	"    Jason Sullivan",
	"",
	"Installer by:",
	"   Steve Stringer, Adam Goldberg,",
	"   Tanya Martino, Eric Schmidt,",
	"   Ronnie Lane",
	"",
	"Art Assistance by:",
	"   Carey Chico and Franz Boehm",
	"",
	"BizDev Babe:",
	"   Jamie Bafus",
	"",
	"And...",
	"",
	"Our Big Toe:",
	"   Mitch Lasky",
	"",
	"",
	"Special Thanks to:",
	"  Id software",
	"  The original Hexen2 crew",
	"   We couldn't have done it",
	"   without you guys!",
	"",
	"",
	"Published by Id Software, Inc.",
	"Distributed by Activision, Inc.",
	"",
	"The Id Software Technology used",
	"under license in Hexen II (tm)",
	"(c) 1996, 1997 Id Software, Inc.",
	"All Rights Reserved.",
	"",
	"Hexen(r) is a registered trademark",
	"of Raven Software Corp.",
	"Hexen II (tm) and the Raven logo",
	"are trademarks of Raven Software",
	"Corp.  The Id Software name and",
	"id logo are trademarks of",
	"Id Software, Inc.  Activision(r)",
	"is a registered trademark of",
	"Activision, Inc. All other",
	"trademarks are the property of",
	"their respective owners.",
	"",
	"",
	"",
	"Send bug descriptions to:",
	"   h2bugs@mail.ravensoft.com",
	"",
	"",
	"No yaks were harmed in the",
	"making of this game!"
};

const char* Credit2TextH2[MAX_LINES2_H2] =
{
	"PowerTrip: James (emorog) Monroe",
	"Cartoons: Brian Raffel",
	"         (use more puzzles)",
	"Doc Keeper: Kevin Schilder",
	"",
	"Whip cracker: James Monroe",
	"",
	"Whipees:",
	"   Mike (i didn't break it) Gummelt",
	"   Josh (extern) Weier",
	"",
	"We don't deserve whipping:",
	"   Josh (I'm not on this project)",
	"         Heitzman",
	"   Nathan (deer hunter) Albury",
	"   Rick (model crusher) Johnson",
	"",
	"Bit Packer:",
	"   Mr. John (Slaine) Scott",
	"",
	"Lead Slacker: Jon (devil boy) Zuk",
	"",
	"Other Slackers:",
	"   Tom (can i have an office) Odell",
	"   Jeremy (nt crashed again) Statz",
	"   Mike (i should be doing my ",
	"         homework) Renner",
	"   Eric (the nose) Biessman",
	"   Kenn (.plan) Hoekstra",
	"   Matt (big elbow) Pinkston",
	"   Bobby (needs haircut) Duncanson",
	"   Brian (they're in my town) Raffel",
	"",
	"Use the mouse: Les Dorscheid",
	"",
	"What's a mouse?:",
	"   Kim (where's my desk) Lathrop",
	"   Gina (i can do your laundry)",
	"        Garren",
	"   Joe (broken axle) Koberstein",
	"   Kevin (titanic) Long",
	"   Jeff (norbert) Butler",
	"   Scott (what's the DEL key for?)",
	"          Rice",
	"   John (Shpluuurt!) Payne",
	"   Steve (crash) Raffel",
	"",
	"Boners:",
	"   Eric (terminator) Turman",
	"   Chaos Device",
	"",
	"Drum beater:",
	"   Kevin Schilder",
	"",
	"Whistle blower:",
	"   Chia Chin (bruce) Lee",
	"",
	"",
	"Activision",
	"",
	"Producer:",
	"   Steve 'Ferris' Stringer",
	"",
	"Marketing Product Manager:",
	"   Henk 'GODMODE' Hartong",
	"",
	"Marketing Associate:",
	"   Kevin 'Kraffinator' Kraff",
	"",
	"Senior Quality",
	"Assurance Lead:",
	"   Tim 'Outlaw' Vanlaw",
	"",
	"Quality Assurance Lead:",
	"   Doug Jacobs",
	"",
	"Shadow Finders:",
	"   Steve Rosenthal, Steve Elwell,",
	"   Chad Bordwell,",
	"   David 'Spice Girl' Baker,",
	"   Error Casillas, Damien Fischer,",
	"   Winnie Lee,"
	"   Ygor Krynytyskyy,",
	"   Samantha (Crusher) Lee, John Park",
	"   Ian Stevens, Chris Toft",
	"",
	"Production Testers:",
	"   Steve 'Damn It's Cold!'",
	"       Rosenthal and",
	"   Chad 'What Hotel Receipt?'",
	"        Bordwell",
	"",
	"Additional QA and Support:",
	"    Tony Villalobos",
	"    Jason Sullivan",
	"",
	"Installer by:",
	"   Steve 'Bahh' Stringer,",
	"   Adam Goldberg, Tanya Martino,",
	"   Eric Schmidt, Ronnie Lane",
	"",
	"Art Assistance by:",
	"   Carey 'Damien' Chico and",
	"   Franz Boehm",
	"",
	"BizDev Babe:",
	"   Jamie Bafus",
	"",
	"And...",
	"",
	"Our Big Toe:",
	"   Mitch Lasky",
	"",
	"",
	"Special Thanks to:",
	"  Id software",
	"  Anyone who ever worked for Raven,",
	"  (except for Alex)",
	"",
	"",
	"Published by Id Software, Inc.",
	"Distributed by Activision, Inc.",
	"",
	"The Id Software Technology used",
	"under license in Hexen II (tm)",
	"(c) 1996, 1997 Id Software, Inc.",
	"All Rights Reserved.",
	"",
	"Hexen(r) is a registered trademark",
	"of Raven Software Corp.",
	"Hexen II (tm) and the Raven logo",
	"are trademarks of Raven Software",
	"Corp.  The Id Software name and",
	"id logo are trademarks of",
	"Id Software, Inc.  Activision(r)",
	"is a registered trademark of",
	"Activision, Inc. All other",
	"trademarks are the property of",
	"their respective owners.",
	"",
	"",
	"",
	"Send bug descriptions to:",
	"   h2bugs@mail.ravensoft.com",
	"",
	"Special Thanks To:",
	"   E.H.S., The Osmonds,",
	"   B.B.V.D., Daisy The Lovin' Lamb,",
	"  'You Killed' Kenny,",
	"   and Baby Biessman.",
	"",
};

#define MAX_LINES_HW 145 + 25
static const char* CreditTextHW[MAX_LINES_HW] =
{
	"HexenWorld",
	"",
	"Lead Programmer: Rick Johnson",
	"",
	"Programming:",
	"   Nathan Albury",
	"   Ron Midthun",
	"   Steve Sengele",
	"   Mike Gummelt",
	"   James Monroe",
	"",
	"Deathmatch Levels:",
	"   Kenn Hoekstra",
	"   Mike Renner",
	"   Jeremy Statz",
	"   Jon Zuk",
	"",
	"Special thanks to:",
	"   Dave Kirsch",
	"   William Mull",
	"   Jack Mathews",
	"",
	"",
	"Hexen2",
	"",
	"Project Director: Brian Raffel",
	"",
	"Lead Programmer: Rick Johnson",
	"",
	"Programming:",
	"   Ben Gokey",
	"   Bob Love",
	"   Mike Gummelt",
	"",
	"Additional Programming:",
	"   Josh Weier",
	"",
	"Lead Design: Eric Biessman",
	"",
	"Design:",
	"   Brian Raffel",
	"   Brian Frank",
	"   Tom Odell",
	"",
	"Art Director: Brian Pelletier",
	"",
	"Art:",
	"   Shane Gurno",
	"   Jim Sumwalt",
	"   Mark Morgan",
	"   Kim Lathrop",
	"   Ted Halsted",
	"   Rebecca Rettenmund",
	"   Les Dorscheid",
	"",
	"Animation:",
	"   Chaos (Mike Werckle)",
	"   Brian Shubat",
	"",
	"Cinematics:",
	"   Jeff Dewitt",
	"   Jeffrey P. Lampo",
	"",
	"Music:",
	"   Kevin Schilder",
	"",
	"Sound:",
	"   Kevin Schilder",
	"   Chia Chin Lee",
	"",
	"",
	"Activision",
	"",
	"Producer:",
	"   Steve Stringer",
	"",
	"Localization Producer:",
	"   Sandi Isaacs",
	"",
	"Marketing Product Manager:",
	"   Henk Hartong",
	"",
	"European Marketing",
	"Product Director:",
	"   Janine Johnson",
	"",
	"Marketing Associate:",
	"   Kevin Kraff",
	"",
	"Senior Quality",
	"Assurance Lead:",
	"   Tim Vanlaw",
	"",
	"Quality Assurance Lead:",
	"   John Tam",
	"",
	"Quality Assurance Team:",
	"   Steve Rosenthal, Mike Spann,",
	"   Steve Elwell, Kelly Wand,",
	"   Kip Stolberg, Igor Krinitskiy,",
	"   Ian Stevens, Marilena Wahmann,",
	"   David Baker, Winnie Lee",
	"",
	"Documentation:",
	"   Mike Rivera, Sylvia Orzel,",
	"   Belinda Vansickle",
	"",
	"Chronicle of Deeds written by:",
	"   Joe Grant Bell",
	"",
	"Localization:",
	"   Nathalie Dove, Lucy Morgan,",
	"   Alex Wylde, Nicky Kerth",
	"",
	"Installer by:",
	"   Steve Stringer, Adam Goldberg,",
	"   Tanya Martino, Eric Schmidt,",
	"   Ronnie Lane",
	"",
	"Art Assistance by:",
	"   Carey Chico and Franz Boehm",
	"",
	"BizDev Babe:",
	"   Jamie Bafus",
	"",
	"And...",
	"",
	"Deal Guru:",
	"   Mitch Lasky",
	"",
	"",
	"Thanks to Id software:",
	"   John Carmack",
	"   Adrian Carmack",
	"   Kevin Cloud",
	"   Barrett 'Bear'  Alexander",
	"   American McGee",
	"",
	"",
	"Published by Id Software, Inc.",
	"Distributed by Activision, Inc.",
	"",
	"The Id Software Technology used",
	"under license in Hexen II (tm)",
	"(c) 1996, 1997 Id Software, Inc.",
	"All Rights Reserved.",
	"",
	"Hexen(r) is a registered trademark",
	"of Raven Software Corp.",
	"Hexen II (tm) and the Raven logo",
	"are trademarks of Raven Software",
	"Corp.  The Id Software name and",
	"id logo are trademarks of",
	"Id Software, Inc.  Activision(r)",
	"is a registered trademark of",
	"Activision, Inc. All other",
	"trademarks are the property of",
	"their respective owners.",
	"",
	"",
	"",
	"Send bug descriptions to:",
	"   h2bugs@mail.ravensoft.com",
	"",
	"Special thanks to Gary McTaggart",
	"at 3dfx for his help with",
	"the gl version!",
	"",
	"No snails were harmed in the",
	"making of this game!"
};

const char* Credit2TextHW[MAX_LINES2_HW] =
{
	"HexenWorld",
	"",
	"Superior Groucher:"
	"   Rick 'Grrr' Johnson",
	"",
	"Bug Creators:",
	"   Nathan 'Glory Code' Albury",
	"   Ron 'Stealth' Midthun",
	"   Steve 'Tie Dye' Sengele",
	"   Mike 'Foos' Gummelt",
	"   James 'Happy' Monroe",
	"",
	"Sloppy Joe Makers:",
	"   Kenn 'I'm a broken man'",
	"      Hoekstra",
	"   Mike 'Outback' Renner",
	"   Jeremy 'Under-rated' Statz",
	"   Jon Zuk",
	"",
	"Avoid the Noid:",
	"   Dave 'Zoid' Kirsch",
	"   William 'Phoeb' Mull",
	"   Jack 'Morbid' Mathews",
	"",
	"",
	"Hexen2",
	"",
	"Map Master: ",
	"   'Caffeine Buzz' Raffel",
	"",
	"Code Warrior:",
	"   Rick 'Superfly' Johnson",
	"",
	"Grunt Boys:",
	"   'Judah' Ben Gokey",
	"   Bob 'Whipped' Love",
	"   Mike '9-Pointer' Gummelt",
	"",
	"Additional Grunting:",
	"   Josh 'Intern' Weier",
	"",
	"Whippin' Boy:",
	"   Eric 'Baby' Biessman",
	"",
	"Crazy Levelers:",
	"   'Big Daddy' Brian Raffel",
	"   Brian 'Red' Frank",
	"   Tom 'Texture Alignment' Odell",
	"",
	"Art Lord:",
	"   Brian 'Mr. Calm' Pelletier",
	"",
	"Pixel Monkies:",
	"   Shane 'Duh' Gurno",
	"   'Slim' Jim Sumwalt",
	"   Mark 'Dad Gummit' Morgan",
	"   Kim 'Toy Master' Lathrop",
	"   'Drop Dead' Ted Halsted",
	"   Rebecca 'Zombie' Rettenmund",
	"   Les 'is not more' Dorscheid",
	"",
	"Salad Shooters:",
	"   Mike 'Chaos' Werckle",
	"   Brian 'Mutton Chops' Shubat",
	"",
	"Last Minute Addition:",
	"   Jeff 'Bud Bundy' Dewitt",
	"   Jeffrey 'Misspalld' Lampo",
	"",
	"Random Notes:",
	"   Kevin 'I Already Paid' Schilder",
	"",
	"Grunts, Groans, and Moans:",
	"   Kevin 'I Already Paid' Schilder",
	"   Chia 'Pet' Chin Lee",
	"",
	"",
	"Activision",
	"",
	"Producer:",
	"   Steve 'Ferris' Stringer",
	"",
	"Localization Producer:",
	"   Sandi 'Boneduster' Isaacs",
	"",
	"Marketing Product Manager:",
	"   Henk 'A-10' Hartong",
	"",
	"European Marketing",
	"Product Director:",
	"   Janine Johnson",
	"",
	"Marketing Associate:",
	"   Kevin 'Savage' Kraff",
	"",
	"Senior Quality",
	"Assurance Lead:",
	"   Tim 'Outlaw' Vanlaw",
	"",
	"Quality Assurance Lead:",
	"   John 'Armadillo' Tam",
	"",
	"Quality Assurance Team:",
	"   Steve 'Rhinochoadicus'",
	"      Rosenthal,",
	"   Mike 'Dragonhawk' Spann,",
	"   Steve 'Zendog' Elwell,",
	"   Kelly 'Li'l Bastard' Wand,",
	"   Kip 'Angus' Stolberg,",
	"   Igor 'Russo' Krinitskiy,",
	"   Ian 'Cracker' Stevens,",
	"   Marilena 'Raveness-X' Wahmann,",
	"   David 'Spicegirl' Baker,",
	"   Winnie 'Mew' Lee",
	"",
	"Documentation:",
	"   Mike Rivera, Sylvia Orzel,",
	"   Belinda Vansickle",
	"",
	"Chronicle of Deeds written by:",
	"   Joe Grant Bell",
	"",
	"Localization:",
	"   Nathalie Dove, Lucy Morgan,",
	"   Alex Wylde, Nicky Kerth",
	"",
	"Installer by:",
	"   Steve 'Bahh' Stringer,",
	"   Adam Goldberg, Tanya Martino,",
	"   Eric Schmidt, Ronnie Lane",
	"",
	"Art Assistance by:",
	"   Carey 'Damien' Chico and",
	"   Franz Boehm",
	"",
	"BizDev Babe:",
	"   Jamie Bafus",
	"",
	"And...",
	"",
	"Deal Guru:",
	"   Mitch 'I'll buy that' Lasky",
	"",
	"",
	"Thanks to Id software:",
	"   John Carmack",
	"   Adrian Carmack",
	"   Kevin Cloud",
	"   Barrett 'Bear'  Alexander",
	"   American McGee",
	"",
	"",
	"Published by Id Software, Inc.",
	"Distributed by Activision, Inc.",
	"",
	"The Id Software Technology used",
	"under license in Hexen II (tm)",
	"(c) 1996, 1997 Id Software, Inc.",
	"All Rights Reserved.",
	"",
	"Hexen(r) is a registered trademark",
	"of Raven Software Corp.",
	"Hexen II (tm) and the Raven logo",
	"are trademarks of Raven Software",
	"Corp.  The Id Software name and",
	"id logo are trademarks of",
	"Id Software, Inc.  Activision(r)",
	"is a registered trademark of",
	"Activision, Inc. All other",
	"trademarks are the property of",
	"their respective owners.",
	"",
	"",
	"",
	"Send bug descriptions to:",
	"   h2bugs@mail.ravensoft.com",
	"",
	"Special thanks to Bob for",
	"remembering 'P' is for Polymorph",
	"",
	"",
	"See the next movie in the long",
	"awaited sequel, starring",
	"Bobby Love in,",
	"   Out of Traction, Back in Action!",
};

void MQH_Menu_Quit_f()
{
	if (m_state == m_quit)
	{
		return;
	}
	wasInMenus = !!(in_keyCatchers & KEYCATCH_UI);
	in_keyCatchers |= KEYCATCH_UI;
	m_quit_prevstate = m_state;
	m_state = m_quit;
	mqh_entersound = true;
	if (GGameType & GAME_Quake)
	{
		msgNumber = rand() & 7;
	}
	else
	{
		LinePos = 0;
		LineTimes = 0;
		if (GGameType & GAME_HexenWorld)
		{
			LineText = CreditTextHW;
			MaxLines = MAX_LINES_HW;
		}
		else
		{
			LineText = CreditTextH2;
			MaxLines = MAX_LINES_H2;
		}
		SoundPlayed = false;
	}
}

void MQH_Init()
{
	Cmd_AddCommand("menu_main", MQH_Menu_Main_f);
	Cmd_AddCommand("menu_options", MQH_Menu_Options_f);
	Cmd_AddCommand("help", MQH_Menu_Help_f);
	Cmd_AddCommand("menu_quit", MQH_Menu_Quit_f);
	if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)))
	{
		Cmd_AddCommand("menu_singleplayer", MQH_Menu_SinglePlayer_f);
		Cmd_AddCommand("menu_load", MQH_Menu_Load_f);
		Cmd_AddCommand("menu_save", MQH_Menu_Save_f);
		Cmd_AddCommand("menu_multiplayer", MQH_Menu_MultiPlayer_f);
	}
	if (GGameType & GAME_Hexen2)
	{
		Cmd_AddCommand("menu_class", MH2_Menu_Class2_f);

		MH2_ReadBigCharWidth();
	}
}

void MQH_Draw()
{
	switch (m_state)
	{
	case m_none:
		break;

	case m_main:
		MQH_Main_Draw();
		break;

	case m_singleplayer:
		MQH_SinglePlayer_Draw();
		break;

	case m_class:
		MH2_Class_Draw();
		break;

	case m_difficulty:
		MH2_Difficulty_Draw();
		break;
	}
}

void MQH_Keydown(int key)
{
	switch (m_state)
	{
	case m_none:
		return;

	case m_main:
		MQH_Main_Key(key);
		return;

	case m_singleplayer:
		MQH_SinglePlayer_Key(key);
		return;

	case m_class:
		MH2_Class_Key(key);
		return;

	case m_difficulty:
		MH2_Difficulty_Key(key);
		return;
	}
}
