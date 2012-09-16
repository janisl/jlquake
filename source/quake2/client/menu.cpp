/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#ifdef _WIN32
#include <io.h>
#endif
#include "client.h"
#include "../../client/game/quake2/qmenu.h"
#include "../../client/renderer/cvars.h"

static int m_main_cursor;

void M_Menu_Main_f(void);
void M_Menu_Options_f(void);
void M_Menu_Quit_f(void);

void M_Menu_Credits(void);

/*
=======================================================================

MAIN MENU

=======================================================================
*/
#define MAIN_ITEMS  5


void M_Main_Draw(void)
{
	int i;
	int w, h;
	int ystart;
	int xoffset;
	int widest = -1;
	int totalheight = 0;
	char litname[80];
	const char* names[] =
	{
		"m_main_game",
		"m_main_multiplayer",
		"m_main_options",
		"m_main_video",
		"m_main_quit",
		0
	};

	for (i = 0; names[i] != 0; i++)
	{
		R_GetPicSize(&w, &h, names[i]);

		if (w > widest)
		{
			widest = w;
		}
		totalheight += (h + 12);
	}

	ystart = (viddef.height / 2 - 110);
	xoffset = (viddef.width - widest + 70) / 2;

	for (i = 0; names[i] != 0; i++)
	{
		if (i != m_main_cursor)
		{
			UI_DrawNamedPic(xoffset, ystart + i * 40 + 13, names[i]);
		}
	}
	String::Cpy(litname, names[m_main_cursor]);
	String::Cat(litname, sizeof(litname), "_sel");
	UI_DrawNamedPic(xoffset, ystart + m_main_cursor * 40 + 13, litname);

	MQ2_DrawCursor(xoffset - 25, ystart + m_main_cursor * 40 + 11, (int)(cls.realtime / 100) % NUM_CURSOR_FRAMES);

	R_GetPicSize(&w, &h, "m_main_plaque");
	UI_DrawNamedPic(xoffset - 30 - w, ystart, "m_main_plaque");

	UI_DrawNamedPic(xoffset - 30 - w, ystart + h + 5, "m_main_logo");
}


const char* M_Main_Key(int key)
{
	const char* sound = menu_move_sound;

	switch (key)
	{
	case K_ESCAPE:
		MQ2_PopMenu();
		break;

	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		if (++m_main_cursor >= MAIN_ITEMS)
		{
			m_main_cursor = 0;
		}
		return sound;

	case K_KP_UPARROW:
	case K_UPARROW:
		if (--m_main_cursor < 0)
		{
			m_main_cursor = MAIN_ITEMS - 1;
		}
		return sound;

	case K_KP_ENTER:
	case K_ENTER:
		mq2_entersound = true;

		switch (m_main_cursor)
		{
		case 0:
			MQ2_Menu_Game_f();
			break;

		case 1:
			MQ2_Menu_Multiplayer_f();
			break;

		case 2:
			M_Menu_Options_f();
			break;

		case 3:
			M_Menu_Video_f();
			break;

		case 4:
			M_Menu_Quit_f();
			break;
		}
	}

	return NULL;
}


void M_Menu_Main_f(void)
{
	MQ2_PushMenu(M_Main_Draw, M_Main_Key, NULL);
}

/*
=======================================================================

QUIT MENU

=======================================================================
*/

const char* M_Quit_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case 'n':
	case 'N':
		MQ2_PopMenu();
		break;

	case 'Y':
	case 'y':
		in_keyCatchers |= KEYCATCH_CONSOLE;
		CL_Quit_f();
		break;

	default:
		break;
	}

	return NULL;

}


void M_Quit_Draw(void)
{
	int w, h;

	R_GetPicSize(&w, &h, "quit");
	UI_DrawNamedPic((viddef.width - w) / 2, (viddef.height - h) / 2, "quit");
}


void M_Menu_Quit_f(void)
{
	MQ2_PushMenu(M_Quit_Draw, M_Quit_Key, NULL);
}



//=============================================================================
/* Menu Subsystem */


/*
=================
M_Init
=================
*/
void M_Init(void)
{
	MQ2_Init();
	Cmd_AddCommand("menu_main", M_Menu_Main_f);
	Cmd_AddCommand("menu_quit", M_Menu_Quit_f);
}


/*
=================
M_Draw
=================
*/
void M_Draw(void)
{
	if (!(in_keyCatchers & KEYCATCH_UI))
	{
		return;
	}

	// dim everything behind it down
	UI_Fill(0, 0, viddef.width, viddef.height, 0, 0, 0, 0.8);

	mq2_drawfunc();

	// delay playing the enter sound until after the
	// menu has been drawn, to avoid delay while
	// caching images
	if (mq2_entersound)
	{
		S_StartLocalSound(menu_in_sound);
		mq2_entersound = false;
	}
}


/*
=================
M_Keydown
=================
*/
void M_Keydown(int key)
{
	const char* s;

	if (mq2_keyfunc)
	{
		if ((s = mq2_keyfunc(key)) != 0)
		{
			S_StartLocalSound((char*)s);
		}
	}
}

void M_CharEvent(int key)
{
	if (mq2_charfunc)
	{
		mq2_charfunc(key);
	}
}
