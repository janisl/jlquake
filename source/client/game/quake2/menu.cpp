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
#include "qmenu.h"
#include "menu.h"

const char* menu_in_sound = "misc/menu1.wav";
const char* menu_move_sound = "misc/menu2.wav";
const char* menu_out_sound = "misc/menu3.wav";

bool mq2_entersound;			// play after drawing a frame, so caching
								// won't disrupt the sound

void (* mq2_drawfunc)();
const char*(*mq2_keyfunc)(int key);
void (*mq2_charfunc)(int key);

static void MQ2_Menu_LoadGame_f();
static void MQ2_Menu_SaveGame_f();
static void MQ2_Menu_Credits_f();

//=============================================================================
/* Support Routines */

menulayer_t mq2_layers[MAX_MENU_DEPTH];
int mq2_menudepth;

void MQ2_Banner(const char* name)
{
	int w, h;
	R_GetPicSize(&w, &h, name);
	UI_DrawNamedPic(viddef.width / 2 - w / 2, viddef.height / 2 - 110, name);
}

void MQ2_PushMenu(void (*draw)(void), const char*(*key)(int k), void (*charfunc)(int key))
{
	if (Cvar_VariableValue("maxclients") == 1 && ComQ2_ServerState())
	{
		Cvar_SetLatched("paused", "1");
	}

	// if this menu is already present, drop back to that level
	// to avoid stacking menus by hotkeys
	int i;
	for (i = 0; i < mq2_menudepth; i++)
	{
		if (mq2_layers[i].draw == draw &&
			mq2_layers[i].key == key)
		{
			mq2_menudepth = i;
		}
	}

	if (i == mq2_menudepth)
	{
		if (mq2_menudepth >= MAX_MENU_DEPTH)
		{
			common->FatalError("MQ2_PushMenu: MAX_MENU_DEPTH");
		}
		mq2_layers[mq2_menudepth].draw = mq2_drawfunc;
		mq2_layers[mq2_menudepth].key = mq2_keyfunc;
		mq2_layers[mq2_menudepth].charfunc = mq2_charfunc;
		mq2_menudepth++;
	}

	mq2_drawfunc = draw;
	mq2_keyfunc = key;
	mq2_charfunc = charfunc;

	mq2_entersound = true;

	in_keyCatchers |= KEYCATCH_UI;
}

void MQ2_ForceMenuOff()
{
	mq2_drawfunc = 0;
	mq2_keyfunc = 0;
	mq2_charfunc = NULL;
	in_keyCatchers &= ~KEYCATCH_UI;
	mq2_menudepth = 0;
	Key_ClearStates();
	Cvar_SetLatched("paused", "0");
}

void MQ2_PopMenu()
{
	S_StartLocalSound(menu_out_sound);
	if (mq2_menudepth < 1)
	{
		common->FatalError("MQ2_PopMenu: depth < 1");
	}
	mq2_menudepth--;

	mq2_drawfunc = mq2_layers[mq2_menudepth].draw;
	mq2_keyfunc = mq2_layers[mq2_menudepth].key;
	mq2_charfunc = mq2_layers[mq2_menudepth].charfunc;

	if (!mq2_menudepth)
	{
		MQ2_ForceMenuOff();
	}
}

const char* Default_MenuKey(menuframework_s* m, int key)
{
	if (m)
	{
		menucommon_s* item = (menucommon_s*)Menu_ItemAtCursor(m);
		if (item != 0)
		{
			if (item->type == MTYPE_FIELD)
			{
				if (MQ2_Field_Key((menufield_s*)item, key))
				{
					return NULL;
				}
			}
		}
	}

	const char* sound = NULL;
	switch (key)
	{
	case K_ESCAPE:
		MQ2_PopMenu();
		return menu_out_sound;
	case K_KP_UPARROW:
	case K_UPARROW:
		if (m)
		{
			m->cursor--;
			Menu_AdjustCursor(m, -1);
			sound = menu_move_sound;
		}
		break;
	case K_TAB:
		if (m)
		{
			m->cursor++;
			Menu_AdjustCursor(m, 1);
			sound = menu_move_sound;
		}
		break;
	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		if (m)
		{
			m->cursor++;
			Menu_AdjustCursor(m, 1);
			sound = menu_move_sound;
		}
		break;
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
		if (m)
		{
			Menu_SlideItem(m, -1);
			sound = menu_move_sound;
		}
		break;
	case K_KP_RIGHTARROW:
	case K_RIGHTARROW:
		if (m)
		{
			Menu_SlideItem(m, 1);
			sound = menu_move_sound;
		}
		break;

	case K_MOUSE1:
	case K_MOUSE2:
	case K_MOUSE3:
	case K_JOY1:
	case K_JOY2:
	case K_JOY3:
	case K_JOY4:
	case K_AUX1:
	case K_AUX2:
	case K_AUX3:
	case K_AUX4:
	case K_AUX5:
	case K_AUX6:
	case K_AUX7:
	case K_AUX8:
	case K_AUX9:
	case K_AUX10:
	case K_AUX11:
	case K_AUX12:
	case K_AUX13:
	case K_AUX14:
	case K_AUX15:
	case K_AUX16:
	case K_AUX17:
	case K_AUX18:
	case K_AUX19:
	case K_AUX20:
	case K_AUX21:
	case K_AUX22:
	case K_AUX23:
	case K_AUX24:
	case K_AUX25:
	case K_AUX26:
	case K_AUX27:
	case K_AUX28:
	case K_AUX29:
	case K_AUX30:
	case K_AUX31:
	case K_AUX32:

	case K_KP_ENTER:
	case K_ENTER:
		if (m)
		{
			Menu_SelectItem(m);
		}
		sound = menu_move_sound;
		break;
	}

	return sound;
}

void Default_MenuChar(menuframework_s* m, int key)
{
	if (m)
	{
		menucommon_s* item = (menucommon_s*)Menu_ItemAtCursor(m);
		if (item)
		{
			if (item->type == MTYPE_FIELD)
			{
				MQ2_Field_Char((menufield_s*)item, key);
			}
		}
	}
}

//	Draws one solid graphics character
// cx and cy are in 320*240 coordinates, and will be centered on
// higher res screens.
void MQ2_DrawCharacter(int cx, int cy, int num)
{
	UI_DrawChar(cx + ((viddef.width - 320) >> 1), cy + ((viddef.height - 240) >> 1), num);
}

void MQ2_Print(int cx, int cy, const char* str)
{
	UI_DrawString(cx + ((viddef.width - 320) >> 1), cy + ((viddef.height - 240) >> 1), str, 128);
}

//	Draws an animating cursor with the point at
// x,y.  The pic will extend to the left of x,
// and both above and below y.
void MQ2_DrawCursor(int x, int y, int f)
{
	static bool cached;
	if (!cached)
	{
		for (int i = 0; i < NUM_CURSOR_FRAMES; i++)
		{
			char cursorname[80];
			String::Sprintf(cursorname, sizeof(cursorname), "m_cursor%d", i);

			R_RegisterPic(cursorname);
		}
		cached = true;
	}

	char cursorname[80];
	String::Sprintf(cursorname, sizeof(cursorname), "m_cursor%d", f);
	UI_DrawNamedPic(x, y, cursorname);
}

void MQ2_DrawTextBox(int x, int y, int width, int lines)
{
	// draw left side
	int cx = x;
	int cy = y;
	MQ2_DrawCharacter(cx, cy, 1);
	for (int n = 0; n < lines; n++)
	{
		cy += 8;
		MQ2_DrawCharacter(cx, cy, 4);
	}
	MQ2_DrawCharacter(cx, cy + 8, 7);

	// draw middle
	cx += 8;
	while (width > 0)
	{
		cy = y;
		MQ2_DrawCharacter(cx, cy, 2);
		for (int n = 0; n < lines; n++)
		{
			cy += 8;
			MQ2_DrawCharacter(cx, cy, 5);
		}
		MQ2_DrawCharacter(cx, cy + 8, 8);
		width -= 1;
		cx += 8;
	}

	// draw right side
	cy = y;
	MQ2_DrawCharacter(cx, cy, 3);
	for (int n = 0; n < lines; n++)
	{
		cy += 8;
		MQ2_DrawCharacter(cx, cy, 6);
	}
	MQ2_DrawCharacter(cx, cy + 8, 9);
}

/*
=============================================================================

GAME MENU

=============================================================================
*/

static int m_game_cursor;

static menuframework_s s_game_menu;
static menuaction_s s_easy_game_action;
static menuaction_s s_medium_game_action;
static menuaction_s s_hard_game_action;
static menuaction_s s_load_game_action;
static menuaction_s s_save_game_action;
static menuaction_s s_credits_action;
static menuseparator_s s_blankline;

static void StartGame()
{
	// disable updates and start the cinematic going
	cl.servercount = -1;
	MQ2_ForceMenuOff();
	Cvar_SetValueLatched("deathmatch", 0);
	Cvar_SetValueLatched("coop", 0);

	Cvar_SetValueLatched("gamerules", 0);		//PGM

	Cbuf_AddText("loading ; killserver ; wait ; newgame\n");
	in_keyCatchers &= ~KEYCATCH_UI;
}

static void EasyGameFunc(void* data)
{
	Cvar_Set("skill", "0");
	StartGame();
}

static void MediumGameFunc(void* data)
{
	Cvar_Set("skill", "1");
	StartGame();
}

static void HardGameFunc(void* data)
{
	Cvar_Set("skill", "2");
	StartGame();
}

static void LoadGameFunc(void* unused)
{
	MQ2_Menu_LoadGame_f();
}

static void SaveGameFunc(void* unused)
{
	MQ2_Menu_SaveGame_f();
}

static void CreditsFunc(void* unused)
{
	MQ2_Menu_Credits_f();
}

static void Game_MenuInit()
{
	s_game_menu.x = viddef.width * 0.50;
	s_game_menu.nitems = 0;

	s_easy_game_action.generic.type = MTYPE_ACTION;
	s_easy_game_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_easy_game_action.generic.x        = 0;
	s_easy_game_action.generic.y        = 0;
	s_easy_game_action.generic.name = "easy";
	s_easy_game_action.generic.callback = EasyGameFunc;

	s_medium_game_action.generic.type   = MTYPE_ACTION;
	s_medium_game_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_medium_game_action.generic.x      = 0;
	s_medium_game_action.generic.y      = 10;
	s_medium_game_action.generic.name   = "medium";
	s_medium_game_action.generic.callback = MediumGameFunc;

	s_hard_game_action.generic.type = MTYPE_ACTION;
	s_hard_game_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_hard_game_action.generic.x        = 0;
	s_hard_game_action.generic.y        = 20;
	s_hard_game_action.generic.name = "hard";
	s_hard_game_action.generic.callback = HardGameFunc;

	s_blankline.generic.type = MTYPE_SEPARATOR;

	s_load_game_action.generic.type = MTYPE_ACTION;
	s_load_game_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_load_game_action.generic.x        = 0;
	s_load_game_action.generic.y        = 40;
	s_load_game_action.generic.name = "load game";
	s_load_game_action.generic.callback = LoadGameFunc;

	s_save_game_action.generic.type = MTYPE_ACTION;
	s_save_game_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_save_game_action.generic.x        = 0;
	s_save_game_action.generic.y        = 50;
	s_save_game_action.generic.name = "save game";
	s_save_game_action.generic.callback = SaveGameFunc;

	s_credits_action.generic.type   = MTYPE_ACTION;
	s_credits_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_credits_action.generic.x      = 0;
	s_credits_action.generic.y      = 60;
	s_credits_action.generic.name   = "credits";
	s_credits_action.generic.callback = CreditsFunc;

	Menu_AddItem(&s_game_menu, (void*)&s_easy_game_action);
	Menu_AddItem(&s_game_menu, (void*)&s_medium_game_action);
	Menu_AddItem(&s_game_menu, (void*)&s_hard_game_action);
	Menu_AddItem(&s_game_menu, (void*)&s_blankline);
	Menu_AddItem(&s_game_menu, (void*)&s_load_game_action);
	Menu_AddItem(&s_game_menu, (void*)&s_save_game_action);
	Menu_AddItem(&s_game_menu, (void*)&s_blankline);
	Menu_AddItem(&s_game_menu, (void*)&s_credits_action);

	Menu_Center(&s_game_menu);
}

static void Game_MenuDraw()
{
	MQ2_Banner("m_banner_game");
	Menu_AdjustCursor(&s_game_menu, 1);
	Menu_Draw(&s_game_menu);
}

static const char* Game_MenuKey(int key)
{
	return Default_MenuKey(&s_game_menu, key);
}

void MQ2_Menu_Game_f()
{
	Game_MenuInit();
	MQ2_PushMenu(Game_MenuDraw, Game_MenuKey, NULL);
	m_game_cursor = 1;
}

/*
=============================================================================

LOADGAME MENU

=============================================================================
*/

#define MAX_SAVEGAMES   15

static menuframework_s s_savegame_menu;

static menuframework_s s_loadgame_menu;
static menuaction_s s_loadgame_actions[MAX_SAVEGAMES];

static char mq2_savestrings[MAX_SAVEGAMES][32];
static bool mq2_savevalid[MAX_SAVEGAMES];

static void Create_Savestrings()
{
	for (int i = 0; i < MAX_SAVEGAMES; i++)
	{
		char name[MAX_OSPATH];
		String::Sprintf(name, sizeof(name), "save/save%i/server.ssv", i);
		if (!FS_FileExists(name))
		{
			String::Cpy(mq2_savestrings[i], "<EMPTY>");
			mq2_savevalid[i] = false;
		}
		else
		{
			fileHandle_t f;
			FS_FOpenFileRead(name, &f, true);
			FS_Read(mq2_savestrings[i], sizeof(mq2_savestrings[i]), f);
			FS_FCloseFile(f);
			mq2_savevalid[i] = true;
		}
	}
}

static void LoadGameCallback(void* self)
{
	menuaction_s* a = (menuaction_s*)self;

	if (mq2_savevalid[a->generic.localdata[0]])
	{
		Cbuf_AddText(va("load save%i\n",  a->generic.localdata[0]));
	}
	MQ2_ForceMenuOff();
}

static void LoadGame_MenuInit()
{
	s_loadgame_menu.x = viddef.width / 2 - 120;
	s_loadgame_menu.y = viddef.height / 2 - 58;
	s_loadgame_menu.nitems = 0;

	Create_Savestrings();

	for (int i = 0; i < MAX_SAVEGAMES; i++)
	{
		s_loadgame_actions[i].generic.name          = mq2_savestrings[i];
		s_loadgame_actions[i].generic.flags         = QMF_LEFT_JUSTIFY;
		s_loadgame_actions[i].generic.localdata[0]  = i;
		s_loadgame_actions[i].generic.callback      = LoadGameCallback;

		s_loadgame_actions[i].generic.x = 0;
		s_loadgame_actions[i].generic.y = (i) * 10;
		if (i > 0)		// separate from autosave
		{
			s_loadgame_actions[i].generic.y += 10;
		}

		s_loadgame_actions[i].generic.type = MTYPE_ACTION;

		Menu_AddItem(&s_loadgame_menu, &s_loadgame_actions[i]);
	}
}

static void LoadGame_MenuDraw()
{
	MQ2_Banner("m_banner_load_game");
//	Menu_AdjustCursor( &s_loadgame_menu, 1 );
	Menu_Draw(&s_loadgame_menu);
}

static const char* LoadGame_MenuKey(int key)
{
	if (key == K_ESCAPE || key == K_ENTER)
	{
		s_savegame_menu.cursor = s_loadgame_menu.cursor - 1;
		if (s_savegame_menu.cursor < 0)
		{
			s_savegame_menu.cursor = 0;
		}
	}
	return Default_MenuKey(&s_loadgame_menu, key);
}

static void MQ2_Menu_LoadGame_f()
{
	LoadGame_MenuInit();
	MQ2_PushMenu(LoadGame_MenuDraw, LoadGame_MenuKey, NULL);
}


/*
=============================================================================

SAVEGAME MENU

=============================================================================
*/
static menuaction_s s_savegame_actions[MAX_SAVEGAMES];

static void SaveGameCallback(void* self)
{
	menuaction_s* a = (menuaction_s*)self;

	Cbuf_AddText(va("save save%i\n", a->generic.localdata[0]));
	MQ2_ForceMenuOff();
}

static void SaveGame_MenuDraw()
{
	MQ2_Banner("m_banner_save_game");
	Menu_AdjustCursor(&s_savegame_menu, 1);
	Menu_Draw(&s_savegame_menu);
}

static void SaveGame_MenuInit()
{
	s_savegame_menu.x = viddef.width / 2 - 120;
	s_savegame_menu.y = viddef.height / 2 - 58;
	s_savegame_menu.nitems = 0;

	Create_Savestrings();

	// don't include the autosave slot
	for (int i = 0; i < MAX_SAVEGAMES - 1; i++)
	{
		s_savegame_actions[i].generic.name = mq2_savestrings[i + 1];
		s_savegame_actions[i].generic.localdata[0] = i + 1;
		s_savegame_actions[i].generic.flags = QMF_LEFT_JUSTIFY;
		s_savegame_actions[i].generic.callback = SaveGameCallback;

		s_savegame_actions[i].generic.x = 0;
		s_savegame_actions[i].generic.y = (i) * 10;

		s_savegame_actions[i].generic.type = MTYPE_ACTION;

		Menu_AddItem(&s_savegame_menu, &s_savegame_actions[i]);
	}
}

static const char* SaveGame_MenuKey(int key)
{
	if (key == K_ENTER || key == K_ESCAPE)
	{
		s_loadgame_menu.cursor = s_savegame_menu.cursor - 1;
		if (s_loadgame_menu.cursor < 0)
		{
			s_loadgame_menu.cursor = 0;
		}
	}
	return Default_MenuKey(&s_savegame_menu, key);
}

static void MQ2_Menu_SaveGame_f()
{
	if (!ComQ2_ServerState())
	{
		return;		// not playing a game

	}
	SaveGame_MenuInit();
	MQ2_PushMenu(SaveGame_MenuDraw, SaveGame_MenuKey, NULL);
	Create_Savestrings();
}

/*
=============================================================================

END GAME MENU

=============================================================================
*/
static int credits_start_time;
static const char** credits;
static char* creditsIndex[256];
static char* creditsBuffer;
static const char* idcredits[] =
{
	"+QUAKE II BY ID SOFTWARE",
	"",
	"+PROGRAMMING",
	"John Carmack",
	"John Cash",
	"Brian Hook",
	"",
	"+ART",
	"Adrian Carmack",
	"Kevin Cloud",
	"Paul Steed",
	"",
	"+LEVEL DESIGN",
	"Tim Willits",
	"American McGee",
	"Christian Antkow",
	"Paul Jaquays",
	"Brandon James",
	"",
	"+BIZ",
	"Todd Hollenshead",
	"Barrett (Bear) Alexander",
	"Donna Jackson",
	"",
	"",
	"+SPECIAL THANKS",
	"Ben Donges for beta testing",
	"",
	"",
	"",
	"",
	"",
	"",
	"+ADDITIONAL SUPPORT",
	"",
	"+LINUX PORT AND CTF",
	"Dave \"Zoid\" Kirsch",
	"",
	"+CINEMATIC SEQUENCES",
	"Ending Cinematic by Blur Studio - ",
	"Venice, CA",
	"",
	"Environment models for Introduction",
	"Cinematic by Karl Dolgener",
	"",
	"Assistance with environment design",
	"by Cliff Iwai",
	"",
	"+SOUND EFFECTS AND MUSIC",
	"Sound Design by Soundelux Media Labs.",
	"Music Composed and Produced by",
	"Soundelux Media Labs.  Special thanks",
	"to Bill Brown, Tom Ozanich, Brian",
	"Celano, Jeff Eisner, and The Soundelux",
	"Players.",
	"",
	"\"Level Music\" by Sonic Mayhem",
	"www.sonicmayhem.com",
	"",
	"\"Quake II Theme Song\"",
	"(C) 1997 Rob Zombie. All Rights",
	"Reserved.",
	"",
	"Track 10 (\"Climb\") by Jer Sypult",
	"",
	"Voice of computers by",
	"Carly Staehlin-Taylor",
	"",
	"+THANKS TO ACTIVISION",
	"+IN PARTICULAR:",
	"",
	"John Tam",
	"Steve Rosenthal",
	"Marty Stratton",
	"Henk Hartong",
	"",
	"Quake II(tm) (C)1997 Id Software, Inc.",
	"All Rights Reserved.  Distributed by",
	"Activision, Inc. under license.",
	"Quake II(tm), the Id Software name,",
	"the \"Q II\"(tm) logo and id(tm)",
	"logo are trademarks of Id Software,",
	"Inc. Activision(R) is a registered",
	"trademark of Activision, Inc. All",
	"other trademarks and trade names are",
	"properties of their respective owners.",
	0
};

static const char* xatcredits[] =
{
	"+QUAKE II MISSION PACK: THE RECKONING",
	"+BY",
	"+XATRIX ENTERTAINMENT, INC.",
	"",
	"+DESIGN AND DIRECTION",
	"Drew Markham",
	"",
	"+PRODUCED BY",
	"Greg Goodrich",
	"",
	"+PROGRAMMING",
	"Rafael Paiz",
	"",
	"+LEVEL DESIGN / ADDITIONAL GAME DESIGN",
	"Alex Mayberry",
	"",
	"+LEVEL DESIGN",
	"Mal Blackwell",
	"Dan Koppel",
	"",
	"+ART DIRECTION",
	"Michael \"Maxx\" Kaufman",
	"",
	"+COMPUTER GRAPHICS SUPERVISOR AND",
	"+CHARACTER ANIMATION DIRECTION",
	"Barry Dempsey",
	"",
	"+SENIOR ANIMATOR AND MODELER",
	"Jason Hoover",
	"",
	"+CHARACTER ANIMATION AND",
	"+MOTION CAPTURE SPECIALIST",
	"Amit Doron",
	"",
	"+ART",
	"Claire Praderie-Markham",
	"Viktor Antonov",
	"Corky Lehmkuhl",
	"",
	"+INTRODUCTION ANIMATION",
	"Dominique Drozdz",
	"",
	"+ADDITIONAL LEVEL DESIGN",
	"Aaron Barber",
	"Rhett Baldwin",
	"",
	"+3D CHARACTER ANIMATION TOOLS",
	"Gerry Tyra, SA Technology",
	"",
	"+ADDITIONAL EDITOR TOOL PROGRAMMING",
	"Robert Duffy",
	"",
	"+ADDITIONAL PROGRAMMING",
	"Ryan Feltrin",
	"",
	"+PRODUCTION COORDINATOR",
	"Victoria Sylvester",
	"",
	"+SOUND DESIGN",
	"Gary Bradfield",
	"",
	"+MUSIC BY",
	"Sonic Mayhem",
	"",
	"",
	"",
	"+SPECIAL THANKS",
	"+TO",
	"+OUR FRIENDS AT ID SOFTWARE",
	"",
	"John Carmack",
	"John Cash",
	"Brian Hook",
	"Adrian Carmack",
	"Kevin Cloud",
	"Paul Steed",
	"Tim Willits",
	"Christian Antkow",
	"Paul Jaquays",
	"Brandon James",
	"Todd Hollenshead",
	"Barrett (Bear) Alexander",
	"Dave \"Zoid\" Kirsch",
	"Donna Jackson",
	"",
	"",
	"",
	"+THANKS TO ACTIVISION",
	"+IN PARTICULAR:",
	"",
	"Marty Stratton",
	"Henk \"The Original Ripper\" Hartong",
	"Kevin Kraff",
	"Jamey Gottlieb",
	"Chris Hepburn",
	"",
	"+AND THE GAME TESTERS",
	"",
	"Tim Vanlaw",
	"Doug Jacobs",
	"Steven Rosenthal",
	"David Baker",
	"Chris Campbell",
	"Aaron Casillas",
	"Steve Elwell",
	"Derek Johnstone",
	"Igor Krinitskiy",
	"Samantha Lee",
	"Michael Spann",
	"Chris Toft",
	"Juan Valdes",
	"",
	"+THANKS TO INTERGRAPH COMPUTER SYTEMS",
	"+IN PARTICULAR:",
	"",
	"Michael T. Nicolaou",
	"",
	"",
	"Quake II Mission Pack: The Reckoning",
	"(tm) (C)1998 Id Software, Inc. All",
	"Rights Reserved. Developed by Xatrix",
	"Entertainment, Inc. for Id Software,",
	"Inc. Distributed by Activision Inc.",
	"under license. Quake(R) is a",
	"registered trademark of Id Software,",
	"Inc. Quake II Mission Pack: The",
	"Reckoning(tm), Quake II(tm), the Id",
	"Software name, the \"Q II\"(tm) logo",
	"and id(tm) logo are trademarks of Id",
	"Software, Inc. Activision(R) is a",
	"registered trademark of Activision,",
	"Inc. Xatrix(R) is a registered",
	"trademark of Xatrix Entertainment,",
	"Inc. All other trademarks and trade",
	"names are properties of their",
	"respective owners.",
	0
};

static const char* roguecredits[] =
{
	"+QUAKE II MISSION PACK 2: GROUND ZERO",
	"+BY",
	"+ROGUE ENTERTAINMENT, INC.",
	"",
	"+PRODUCED BY",
	"Jim Molinets",
	"",
	"+PROGRAMMING",
	"Peter Mack",
	"Patrick Magruder",
	"",
	"+LEVEL DESIGN",
	"Jim Molinets",
	"Cameron Lamprecht",
	"Berenger Fish",
	"Robert Selitto",
	"Steve Tietze",
	"Steve Thoms",
	"",
	"+ART DIRECTION",
	"Rich Fleider",
	"",
	"+ART",
	"Rich Fleider",
	"Steve Maines",
	"Won Choi",
	"",
	"+ANIMATION SEQUENCES",
	"Creat Studios",
	"Steve Maines",
	"",
	"+ADDITIONAL LEVEL DESIGN",
	"Rich Fleider",
	"Steve Maines",
	"Peter Mack",
	"",
	"+SOUND",
	"James Grunke",
	"",
	"+GROUND ZERO THEME",
	"+AND",
	"+MUSIC BY",
	"Sonic Mayhem",
	"",
	"+VWEP MODELS",
	"Brent \"Hentai\" Dill",
	"",
	"",
	"",
	"+SPECIAL THANKS",
	"+TO",
	"+OUR FRIENDS AT ID SOFTWARE",
	"",
	"John Carmack",
	"John Cash",
	"Brian Hook",
	"Adrian Carmack",
	"Kevin Cloud",
	"Paul Steed",
	"Tim Willits",
	"Christian Antkow",
	"Paul Jaquays",
	"Brandon James",
	"Todd Hollenshead",
	"Barrett (Bear) Alexander",
	"Katherine Anna Kang",
	"Donna Jackson",
	"Dave \"Zoid\" Kirsch",
	"",
	"",
	"",
	"+THANKS TO ACTIVISION",
	"+IN PARTICULAR:",
	"",
	"Marty Stratton",
	"Henk Hartong",
	"Mitch Lasky",
	"Steve Rosenthal",
	"Steve Elwell",
	"",
	"+AND THE GAME TESTERS",
	"",
	"The Ranger Clan",
	"Dave \"Zoid\" Kirsch",
	"Nihilistic Software",
	"Robert Duffy",
	"",
	"And Countless Others",
	"",
	"",
	"",
	"Quake II Mission Pack 2: Ground Zero",
	"(tm) (C)1998 Id Software, Inc. All",
	"Rights Reserved. Developed by Rogue",
	"Entertainment, Inc. for Id Software,",
	"Inc. Distributed by Activision Inc.",
	"under license. Quake(R) is a",
	"registered trademark of Id Software,",
	"Inc. Quake II Mission Pack 2: Ground",
	"Zero(tm), Quake II(tm), the Id",
	"Software name, the \"Q II\"(tm) logo",
	"and id(tm) logo are trademarks of Id",
	"Software, Inc. Activision(R) is a",
	"registered trademark of Activision,",
	"Inc. Rogue(R) is a registered",
	"trademark of Rogue Entertainment,",
	"Inc. All other trademarks and trade",
	"names are properties of their",
	"respective owners.",
	0
};

static void MQ2_Credits_MenuDraw()
{
	//	draw the credits
	int y = viddef.height - ((cls.realtime - credits_start_time) / 40.0F);
	for (int i = 0; credits[i] && y < viddef.height; y += 10, i++)
	{
		if (y <= -8)
		{
			continue;
		}

		int bold = false;
		int stringoffset = 0;
		if (credits[i][0] == '+')
		{
			bold = true;
			stringoffset = 1;
		}
		else
		{
			bold = false;
			stringoffset = 0;
		}

		int x = (viddef.width - String::Length(&credits[i][stringoffset]) * 8) / 2;
		if (bold)
		{
			UI_DrawString(x, y, &credits[i][stringoffset], 128);
		}
		else
		{
			UI_DrawString(x, y, &credits[i][stringoffset]);
		}
	}

	if (y < 0)
	{
		credits_start_time = cls.realtime;
	}
}

static const char* MQ2_Credits_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
		if (creditsBuffer)
		{
			FS_FreeFile(creditsBuffer);
		}
		MQ2_PopMenu();
		break;
	}

	return menu_out_sound;

}

static void MQ2_Menu_Credits_f()
{
	creditsBuffer = NULL;
	int count = FS_ReadFile("credits", (void**)&creditsBuffer);
	if (count != -1)
	{
		char* p = creditsBuffer;
		int n;
		for (n = 0; n < 255; n++)
		{
			creditsIndex[n] = p;
			while (*p != '\r' && *p != '\n')
			{
				p++;
				if (--count == 0)
				{
					break;
				}
			}
			if (*p == '\r')
			{
				*p++ = 0;
				if (--count == 0)
				{
					break;
				}
			}
			*p++ = 0;
			if (--count == 0)
			{
				break;
			}
		}
		creditsIndex[++n] = 0;
		credits = (const char**)creditsIndex;
	}
	else
	{
		int isdeveloper = FS_GetQuake2GameType();

		if (isdeveloper == 1)			// xatrix
		{
			credits = xatcredits;
		}
		else if (isdeveloper == 2)		// ROGUE
		{
			credits = roguecredits;
		}
		else
		{
			credits = idcredits;
		}

	}

	credits_start_time = cls.realtime;
	MQ2_PushMenu(MQ2_Credits_MenuDraw, MQ2_Credits_Key, NULL);
}

void MQ2_Init()
{
	Cmd_AddCommand("menu_game", MQ2_Menu_Game_f);
	Cmd_AddCommand("menu_loadgame", MQ2_Menu_LoadGame_f);
	Cmd_AddCommand("menu_savegame", MQ2_Menu_SaveGame_f);
	Cmd_AddCommand("menu_credits", MQ2_Menu_Credits_f);
}
