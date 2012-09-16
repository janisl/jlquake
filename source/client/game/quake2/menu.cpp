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
