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
void M_Menu_Video_f(void);
void M_Menu_Options_f(void);
void M_Menu_Keys_f(void);
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

KEYS MENU

=======================================================================
*/
const char* bindnames[][2] =
{
	{"+attack",         "attack"},
	{"weapnext",        "next weapon"},
	{"+forward",        "walk forward"},
	{"+back",           "backpedal"},
	{"+left",           "turn left"},
	{"+right",          "turn right"},
	{"+speed",          "run"},
	{"+moveleft",       "step left"},
	{"+moveright",      "step right"},
	{"+strafe",         "sidestep"},
	{"+lookup",         "look up"},
	{"+lookdown",       "look down"},
	{"centerview",      "center view"},
	{"+mlook",          "mouse look"},
	{"+moveup",         "up / jump"},
	{"+movedown",       "down / crouch"},

	{"inven",           "inventory"},
	{"invuse",          "use item"},
	{"invdrop",         "drop item"},
	{"invprev",         "prev item"},
	{"invnext",         "next item"},

	{"cmd help",        "help computer" },
	{ 0, 0 }
};

static int bind_grab;

static menuframework_s s_keys_menu;
static menuaction_s s_keys_attack_action;
static menuaction_s s_keys_change_weapon_action;
static menuaction_s s_keys_walk_forward_action;
static menuaction_s s_keys_backpedal_action;
static menuaction_s s_keys_turn_left_action;
static menuaction_s s_keys_turn_right_action;
static menuaction_s s_keys_run_action;
static menuaction_s s_keys_step_left_action;
static menuaction_s s_keys_step_right_action;
static menuaction_s s_keys_sidestep_action;
static menuaction_s s_keys_look_up_action;
static menuaction_s s_keys_look_down_action;
static menuaction_s s_keys_center_view_action;
static menuaction_s s_keys_mouse_look_action;
static menuaction_s s_keys_keyboard_look_action;
static menuaction_s s_keys_move_up_action;
static menuaction_s s_keys_move_down_action;
static menuaction_s s_keys_inventory_action;
static menuaction_s s_keys_inv_use_action;
static menuaction_s s_keys_inv_drop_action;
static menuaction_s s_keys_inv_prev_action;
static menuaction_s s_keys_inv_next_action;

static menuaction_s s_keys_help_computer_action;

static void KeyCursorDrawFunc(menuframework_s* menu)
{
	if (bind_grab)
	{
		UI_DrawChar(menu->x, menu->y + menu->cursor * 9, '=');
	}
	else
	{
		UI_DrawChar(menu->x, menu->y + menu->cursor * 9, 12 + ((int)(Sys_Milliseconds_() / 250) & 1));
	}
}

static void DrawKeyBindingFunc(void* self)
{
	int keys[2];
	menuaction_s* a = (menuaction_s*)self;

	Key_GetKeysForBinding(bindnames[a->generic.localdata[0]][0], &keys[0], &keys[1]);

	if (keys[0] == -1)
	{
		UI_DrawString(a->generic.x + a->generic.parent->x + 16, a->generic.y + a->generic.parent->y, "???");
	}
	else
	{
		int x;
		const char* name;

		name = Key_KeynumToString(keys[0], true);

		UI_DrawString(a->generic.x + a->generic.parent->x + 16, a->generic.y + a->generic.parent->y, name);

		x = String::Length(name) * 8;

		if (keys[1] != -1)
		{
			UI_DrawString(a->generic.x + a->generic.parent->x + 24 + x, a->generic.y + a->generic.parent->y, "or");
			UI_DrawString(a->generic.x + a->generic.parent->x + 48 + x, a->generic.y + a->generic.parent->y, Key_KeynumToString(keys[1], true));
		}
	}
}

static void KeyBindingFunc(void* self)
{
	menuaction_s* a = (menuaction_s*)self;
	int keys[2];

	Key_GetKeysForBinding(bindnames[a->generic.localdata[0]][0], &keys[0], &keys[1]);

	if (keys[1] != -1)
	{
		Key_UnbindCommand(bindnames[a->generic.localdata[0]][0]);
	}

	bind_grab = true;

	Menu_SetStatusBar(&s_keys_menu, "press a key or button for this action");
}

static void Keys_MenuInit(void)
{
	int y = 0;
	int i = 0;

	s_keys_menu.x = viddef.width * 0.50;
	s_keys_menu.nitems = 0;
	s_keys_menu.cursordraw = KeyCursorDrawFunc;

	s_keys_attack_action.generic.type   = MTYPE_ACTION;
	s_keys_attack_action.generic.flags  = QMF_GRAYED;
	s_keys_attack_action.generic.x      = 0;
	s_keys_attack_action.generic.y      = y;
	s_keys_attack_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_attack_action.generic.localdata[0] = i;
	s_keys_attack_action.generic.name   = bindnames[s_keys_attack_action.generic.localdata[0]][1];

	s_keys_change_weapon_action.generic.type    = MTYPE_ACTION;
	s_keys_change_weapon_action.generic.flags  = QMF_GRAYED;
	s_keys_change_weapon_action.generic.x       = 0;
	s_keys_change_weapon_action.generic.y       = y += 9;
	s_keys_change_weapon_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_change_weapon_action.generic.localdata[0] = ++i;
	s_keys_change_weapon_action.generic.name    = bindnames[s_keys_change_weapon_action.generic.localdata[0]][1];

	s_keys_walk_forward_action.generic.type = MTYPE_ACTION;
	s_keys_walk_forward_action.generic.flags  = QMF_GRAYED;
	s_keys_walk_forward_action.generic.x        = 0;
	s_keys_walk_forward_action.generic.y        = y += 9;
	s_keys_walk_forward_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_walk_forward_action.generic.localdata[0] = ++i;
	s_keys_walk_forward_action.generic.name = bindnames[s_keys_walk_forward_action.generic.localdata[0]][1];

	s_keys_backpedal_action.generic.type    = MTYPE_ACTION;
	s_keys_backpedal_action.generic.flags  = QMF_GRAYED;
	s_keys_backpedal_action.generic.x       = 0;
	s_keys_backpedal_action.generic.y       = y += 9;
	s_keys_backpedal_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_backpedal_action.generic.localdata[0] = ++i;
	s_keys_backpedal_action.generic.name    = bindnames[s_keys_backpedal_action.generic.localdata[0]][1];

	s_keys_turn_left_action.generic.type    = MTYPE_ACTION;
	s_keys_turn_left_action.generic.flags  = QMF_GRAYED;
	s_keys_turn_left_action.generic.x       = 0;
	s_keys_turn_left_action.generic.y       = y += 9;
	s_keys_turn_left_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_turn_left_action.generic.localdata[0] = ++i;
	s_keys_turn_left_action.generic.name    = bindnames[s_keys_turn_left_action.generic.localdata[0]][1];

	s_keys_turn_right_action.generic.type   = MTYPE_ACTION;
	s_keys_turn_right_action.generic.flags  = QMF_GRAYED;
	s_keys_turn_right_action.generic.x      = 0;
	s_keys_turn_right_action.generic.y      = y += 9;
	s_keys_turn_right_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_turn_right_action.generic.localdata[0] = ++i;
	s_keys_turn_right_action.generic.name   = bindnames[s_keys_turn_right_action.generic.localdata[0]][1];

	s_keys_run_action.generic.type  = MTYPE_ACTION;
	s_keys_run_action.generic.flags  = QMF_GRAYED;
	s_keys_run_action.generic.x     = 0;
	s_keys_run_action.generic.y     = y += 9;
	s_keys_run_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_run_action.generic.localdata[0] = ++i;
	s_keys_run_action.generic.name  = bindnames[s_keys_run_action.generic.localdata[0]][1];

	s_keys_step_left_action.generic.type    = MTYPE_ACTION;
	s_keys_step_left_action.generic.flags  = QMF_GRAYED;
	s_keys_step_left_action.generic.x       = 0;
	s_keys_step_left_action.generic.y       = y += 9;
	s_keys_step_left_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_step_left_action.generic.localdata[0] = ++i;
	s_keys_step_left_action.generic.name    = bindnames[s_keys_step_left_action.generic.localdata[0]][1];

	s_keys_step_right_action.generic.type   = MTYPE_ACTION;
	s_keys_step_right_action.generic.flags  = QMF_GRAYED;
	s_keys_step_right_action.generic.x      = 0;
	s_keys_step_right_action.generic.y      = y += 9;
	s_keys_step_right_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_step_right_action.generic.localdata[0] = ++i;
	s_keys_step_right_action.generic.name   = bindnames[s_keys_step_right_action.generic.localdata[0]][1];

	s_keys_sidestep_action.generic.type = MTYPE_ACTION;
	s_keys_sidestep_action.generic.flags  = QMF_GRAYED;
	s_keys_sidestep_action.generic.x        = 0;
	s_keys_sidestep_action.generic.y        = y += 9;
	s_keys_sidestep_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_sidestep_action.generic.localdata[0] = ++i;
	s_keys_sidestep_action.generic.name = bindnames[s_keys_sidestep_action.generic.localdata[0]][1];

	s_keys_look_up_action.generic.type  = MTYPE_ACTION;
	s_keys_look_up_action.generic.flags  = QMF_GRAYED;
	s_keys_look_up_action.generic.x     = 0;
	s_keys_look_up_action.generic.y     = y += 9;
	s_keys_look_up_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_look_up_action.generic.localdata[0] = ++i;
	s_keys_look_up_action.generic.name  = bindnames[s_keys_look_up_action.generic.localdata[0]][1];

	s_keys_look_down_action.generic.type    = MTYPE_ACTION;
	s_keys_look_down_action.generic.flags  = QMF_GRAYED;
	s_keys_look_down_action.generic.x       = 0;
	s_keys_look_down_action.generic.y       = y += 9;
	s_keys_look_down_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_look_down_action.generic.localdata[0] = ++i;
	s_keys_look_down_action.generic.name    = bindnames[s_keys_look_down_action.generic.localdata[0]][1];

	s_keys_center_view_action.generic.type  = MTYPE_ACTION;
	s_keys_center_view_action.generic.flags  = QMF_GRAYED;
	s_keys_center_view_action.generic.x     = 0;
	s_keys_center_view_action.generic.y     = y += 9;
	s_keys_center_view_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_center_view_action.generic.localdata[0] = ++i;
	s_keys_center_view_action.generic.name  = bindnames[s_keys_center_view_action.generic.localdata[0]][1];

	s_keys_mouse_look_action.generic.type   = MTYPE_ACTION;
	s_keys_mouse_look_action.generic.flags  = QMF_GRAYED;
	s_keys_mouse_look_action.generic.x      = 0;
	s_keys_mouse_look_action.generic.y      = y += 9;
	s_keys_mouse_look_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_mouse_look_action.generic.localdata[0] = ++i;
	s_keys_mouse_look_action.generic.name   = bindnames[s_keys_mouse_look_action.generic.localdata[0]][1];

	s_keys_keyboard_look_action.generic.type    = MTYPE_ACTION;
	s_keys_keyboard_look_action.generic.flags  = QMF_GRAYED;
	s_keys_keyboard_look_action.generic.x       = 0;
	s_keys_keyboard_look_action.generic.y       = y += 9;
	s_keys_keyboard_look_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_keyboard_look_action.generic.localdata[0] = ++i;
	s_keys_keyboard_look_action.generic.name    = bindnames[s_keys_keyboard_look_action.generic.localdata[0]][1];

	s_keys_move_up_action.generic.type  = MTYPE_ACTION;
	s_keys_move_up_action.generic.flags  = QMF_GRAYED;
	s_keys_move_up_action.generic.x     = 0;
	s_keys_move_up_action.generic.y     = y += 9;
	s_keys_move_up_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_move_up_action.generic.localdata[0] = ++i;
	s_keys_move_up_action.generic.name  = bindnames[s_keys_move_up_action.generic.localdata[0]][1];

	s_keys_move_down_action.generic.type    = MTYPE_ACTION;
	s_keys_move_down_action.generic.flags  = QMF_GRAYED;
	s_keys_move_down_action.generic.x       = 0;
	s_keys_move_down_action.generic.y       = y += 9;
	s_keys_move_down_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_move_down_action.generic.localdata[0] = ++i;
	s_keys_move_down_action.generic.name    = bindnames[s_keys_move_down_action.generic.localdata[0]][1];

	s_keys_inventory_action.generic.type    = MTYPE_ACTION;
	s_keys_inventory_action.generic.flags  = QMF_GRAYED;
	s_keys_inventory_action.generic.x       = 0;
	s_keys_inventory_action.generic.y       = y += 9;
	s_keys_inventory_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inventory_action.generic.localdata[0] = ++i;
	s_keys_inventory_action.generic.name    = bindnames[s_keys_inventory_action.generic.localdata[0]][1];

	s_keys_inv_use_action.generic.type  = MTYPE_ACTION;
	s_keys_inv_use_action.generic.flags  = QMF_GRAYED;
	s_keys_inv_use_action.generic.x     = 0;
	s_keys_inv_use_action.generic.y     = y += 9;
	s_keys_inv_use_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inv_use_action.generic.localdata[0] = ++i;
	s_keys_inv_use_action.generic.name  = bindnames[s_keys_inv_use_action.generic.localdata[0]][1];

	s_keys_inv_drop_action.generic.type = MTYPE_ACTION;
	s_keys_inv_drop_action.generic.flags  = QMF_GRAYED;
	s_keys_inv_drop_action.generic.x        = 0;
	s_keys_inv_drop_action.generic.y        = y += 9;
	s_keys_inv_drop_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inv_drop_action.generic.localdata[0] = ++i;
	s_keys_inv_drop_action.generic.name = bindnames[s_keys_inv_drop_action.generic.localdata[0]][1];

	s_keys_inv_prev_action.generic.type = MTYPE_ACTION;
	s_keys_inv_prev_action.generic.flags  = QMF_GRAYED;
	s_keys_inv_prev_action.generic.x        = 0;
	s_keys_inv_prev_action.generic.y        = y += 9;
	s_keys_inv_prev_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inv_prev_action.generic.localdata[0] = ++i;
	s_keys_inv_prev_action.generic.name = bindnames[s_keys_inv_prev_action.generic.localdata[0]][1];

	s_keys_inv_next_action.generic.type = MTYPE_ACTION;
	s_keys_inv_next_action.generic.flags  = QMF_GRAYED;
	s_keys_inv_next_action.generic.x        = 0;
	s_keys_inv_next_action.generic.y        = y += 9;
	s_keys_inv_next_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inv_next_action.generic.localdata[0] = ++i;
	s_keys_inv_next_action.generic.name = bindnames[s_keys_inv_next_action.generic.localdata[0]][1];

	s_keys_help_computer_action.generic.type    = MTYPE_ACTION;
	s_keys_help_computer_action.generic.flags  = QMF_GRAYED;
	s_keys_help_computer_action.generic.x       = 0;
	s_keys_help_computer_action.generic.y       = y += 9;
	s_keys_help_computer_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_help_computer_action.generic.localdata[0] = ++i;
	s_keys_help_computer_action.generic.name    = bindnames[s_keys_help_computer_action.generic.localdata[0]][1];

	Menu_AddItem(&s_keys_menu, (void*)&s_keys_attack_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_change_weapon_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_walk_forward_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_backpedal_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_turn_left_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_turn_right_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_run_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_step_left_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_step_right_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_sidestep_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_look_up_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_look_down_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_center_view_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_mouse_look_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_keyboard_look_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_move_up_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_move_down_action);

	Menu_AddItem(&s_keys_menu, (void*)&s_keys_inventory_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_inv_use_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_inv_drop_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_inv_prev_action);
	Menu_AddItem(&s_keys_menu, (void*)&s_keys_inv_next_action);

	Menu_AddItem(&s_keys_menu, (void*)&s_keys_help_computer_action);

	Menu_SetStatusBar(&s_keys_menu, "enter to change, backspace to clear");
	Menu_Center(&s_keys_menu);
}

static void Keys_MenuDraw(void)
{
	Menu_AdjustCursor(&s_keys_menu, 1);
	Menu_Draw(&s_keys_menu);
}

static const char* Keys_MenuKey(int key)
{
	menuaction_s* item = (menuaction_s*)Menu_ItemAtCursor(&s_keys_menu);

	if (bind_grab)
	{
		if (key != K_ESCAPE && key != '`')
		{
			char cmd[1024];

			String::Sprintf(cmd, sizeof(cmd), "bind \"%s\" \"%s\"\n", Key_KeynumToString(key, false), bindnames[item->generic.localdata[0]][0]);
			Cbuf_InsertText(cmd);
		}

		Menu_SetStatusBar(&s_keys_menu, "enter to change, backspace to clear");
		bind_grab = false;
		return menu_out_sound;
	}

	switch (key)
	{
	case K_KP_ENTER:
	case K_ENTER:
		KeyBindingFunc(item);
		return menu_in_sound;
	case K_BACKSPACE:		// delete bindings
	case K_DEL:				// delete bindings
	case K_KP_DEL:
		Key_UnbindCommand(bindnames[item->generic.localdata[0]][0]);
		return menu_out_sound;
	default:
		return Default_MenuKey(&s_keys_menu, key);
	}
}

void M_Menu_Keys_f(void)
{
	Keys_MenuInit();
	MQ2_PushMenu(Keys_MenuDraw, Keys_MenuKey, NULL);
}


/*
=======================================================================

CONTROLS MENU

=======================================================================
*/
extern Cvar* in_joystick;

static menuframework_s s_options_menu;
static menuaction_s s_options_defaults_action;
static menuaction_s s_options_customize_options_action;
static menuslider_s s_options_sensitivity_slider;
static menulist_s s_options_freelook_box;
static menulist_s s_options_alwaysrun_box;
static menulist_s s_options_invertmouse_box;
static menulist_s s_options_lookspring_box;
static menulist_s s_options_crosshair_box;
static menuslider_s s_options_sfxvolume_slider;
static menulist_s s_options_joystick_box;
static menulist_s s_options_cdvolume_box;
static menulist_s s_options_console_action;

static void CrosshairFunc(void* unused)
{
	Cvar_SetValueLatched("crosshair", s_options_crosshair_box.curvalue);
}

static void JoystickFunc(void* unused)
{
	Cvar_SetValueLatched("in_joystick", s_options_joystick_box.curvalue);
}

static void CustomizeControlsFunc(void* unused)
{
	M_Menu_Keys_f();
}

static void AlwaysRunFunc(void* unused)
{
	Cvar_SetValueLatched("cl_run", s_options_alwaysrun_box.curvalue);
}

static void FreeLookFunc(void* unused)
{
	Cvar_SetValueLatched("cl_freelook", s_options_freelook_box.curvalue);
}

static void MouseSpeedFunc(void* unused)
{
	Cvar_SetValueLatched("sensitivity", s_options_sensitivity_slider.curvalue / 2.0F);
}

static void ControlsSetMenuItemValues(void)
{
	s_options_sfxvolume_slider.curvalue     = Cvar_VariableValue("s_volume") * 10;
	s_options_cdvolume_box.curvalue         = !Cvar_VariableValue("cd_nocd");
	s_options_sensitivity_slider.curvalue   = (cl_sensitivity->value) * 2;

	Cvar_SetValueLatched("cl_run", ClampCvar(0, 1, cl_run->value));
	s_options_alwaysrun_box.curvalue        = cl_run->value;

	s_options_invertmouse_box.curvalue      = m_pitch->value < 0;

	Cvar_SetValueLatched("lookspring", ClampCvar(0, 1, lookspring->value));
	s_options_lookspring_box.curvalue       = lookspring->value;

	Cvar_SetValueLatched("cl_freelook", ClampCvar(0, 1, cl_freelook->value));
	s_options_freelook_box.curvalue         = cl_freelook->value;

	Cvar_SetValueLatched("crosshair", ClampCvar(0, 3, crosshair->value));
	s_options_crosshair_box.curvalue        = crosshair->value;

	Cvar_SetValueLatched("in_joystick", ClampCvar(0, 1, in_joystick->value));
	s_options_joystick_box.curvalue     = in_joystick->value;
}

static void ControlsResetDefaultsFunc(void* unused)
{
	Cbuf_AddText("exec default.cfg\n");
	Cbuf_Execute();

	ControlsSetMenuItemValues();
}

static void InvertMouseFunc(void* unused)
{
	if (s_options_invertmouse_box.curvalue == 0)
	{
		Cvar_SetValueLatched("m_pitch", fabs(m_pitch->value));
	}
	else
	{
		Cvar_SetValueLatched("m_pitch", -fabs(m_pitch->value));
	}
}

static void LookspringFunc(void* unused)
{
	Cvar_SetValueLatched("lookspring", s_options_lookspring_box.curvalue);
}

static void UpdateVolumeFunc(void* unused)
{
	Cvar_SetValueLatched("s_volume", s_options_sfxvolume_slider.curvalue / 10);
}

static void UpdateCDVolumeFunc(void* unused)
{
	Cvar_SetValueLatched("cd_nocd", !s_options_cdvolume_box.curvalue);
}

static void ConsoleFunc(void* unused)
{
	/*
	** the proper way to do this is probably to have ToggleConsole_f accept a parameter
	*/
	extern void Key_ClearTyping(void);

	if (cl.q2_attractloop)
	{
		Cbuf_AddText("killserver\n");
		return;
	}

	Key_ClearTyping();
	Con_ClearNotify();

	MQ2_ForceMenuOff();
	in_keyCatchers |= KEYCATCH_CONSOLE;
}

void Options_MenuInit(void)
{
	static const char* cd_music_items[] =
	{
		"disabled",
		"enabled",
		0
	};

	static const char* yesno_names[] =
	{
		"no",
		"yes",
		0
	};

	static const char* crosshair_names[] =
	{
		"none",
		"cross",
		"dot",
		"angle",
		0
	};

	/*
	** configure controls menu and menu items
	*/
	s_options_menu.x = viddef.width / 2;
	s_options_menu.y = viddef.height / 2 - 58;
	s_options_menu.nitems = 0;

	s_options_sfxvolume_slider.generic.type = MTYPE_SLIDER;
	s_options_sfxvolume_slider.generic.x    = 0;
	s_options_sfxvolume_slider.generic.y    = 0;
	s_options_sfxvolume_slider.generic.name = "effects volume";
	s_options_sfxvolume_slider.generic.callback = UpdateVolumeFunc;
	s_options_sfxvolume_slider.minvalue     = 0;
	s_options_sfxvolume_slider.maxvalue     = 10;
	s_options_sfxvolume_slider.curvalue     = Cvar_VariableValue("s_volume") * 10;

	s_options_cdvolume_box.generic.type = MTYPE_SPINCONTROL;
	s_options_cdvolume_box.generic.x        = 0;
	s_options_cdvolume_box.generic.y        = 10;
	s_options_cdvolume_box.generic.name = "CD music";
	s_options_cdvolume_box.generic.callback = UpdateCDVolumeFunc;
	s_options_cdvolume_box.itemnames        = cd_music_items;
	s_options_cdvolume_box.curvalue         = !Cvar_VariableValue("cd_nocd");

	s_options_sensitivity_slider.generic.type   = MTYPE_SLIDER;
	s_options_sensitivity_slider.generic.x      = 0;
	s_options_sensitivity_slider.generic.y      = 50;
	s_options_sensitivity_slider.generic.name   = "mouse speed";
	s_options_sensitivity_slider.generic.callback = MouseSpeedFunc;
	s_options_sensitivity_slider.minvalue       = 2;
	s_options_sensitivity_slider.maxvalue       = 22;

	s_options_alwaysrun_box.generic.type = MTYPE_SPINCONTROL;
	s_options_alwaysrun_box.generic.x   = 0;
	s_options_alwaysrun_box.generic.y   = 60;
	s_options_alwaysrun_box.generic.name    = "always run";
	s_options_alwaysrun_box.generic.callback = AlwaysRunFunc;
	s_options_alwaysrun_box.itemnames = yesno_names;

	s_options_invertmouse_box.generic.type = MTYPE_SPINCONTROL;
	s_options_invertmouse_box.generic.x = 0;
	s_options_invertmouse_box.generic.y = 70;
	s_options_invertmouse_box.generic.name  = "invert mouse";
	s_options_invertmouse_box.generic.callback = InvertMouseFunc;
	s_options_invertmouse_box.itemnames = yesno_names;

	s_options_lookspring_box.generic.type = MTYPE_SPINCONTROL;
	s_options_lookspring_box.generic.x  = 0;
	s_options_lookspring_box.generic.y  = 80;
	s_options_lookspring_box.generic.name   = "lookspring";
	s_options_lookspring_box.generic.callback = LookspringFunc;
	s_options_lookspring_box.itemnames = yesno_names;

	s_options_freelook_box.generic.type = MTYPE_SPINCONTROL;
	s_options_freelook_box.generic.x    = 0;
	s_options_freelook_box.generic.y    = 100;
	s_options_freelook_box.generic.name = "free look";
	s_options_freelook_box.generic.callback = FreeLookFunc;
	s_options_freelook_box.itemnames = yesno_names;

	s_options_crosshair_box.generic.type = MTYPE_SPINCONTROL;
	s_options_crosshair_box.generic.x   = 0;
	s_options_crosshair_box.generic.y   = 110;
	s_options_crosshair_box.generic.name    = "crosshair";
	s_options_crosshair_box.generic.callback = CrosshairFunc;
	s_options_crosshair_box.itemnames = crosshair_names;

	s_options_joystick_box.generic.type = MTYPE_SPINCONTROL;
	s_options_joystick_box.generic.x    = 0;
	s_options_joystick_box.generic.y    = 120;
	s_options_joystick_box.generic.name = "use joystick";
	s_options_joystick_box.generic.callback = JoystickFunc;
	s_options_joystick_box.itemnames = yesno_names;

	s_options_customize_options_action.generic.type = MTYPE_ACTION;
	s_options_customize_options_action.generic.x        = 0;
	s_options_customize_options_action.generic.y        = 140;
	s_options_customize_options_action.generic.name = "customize controls";
	s_options_customize_options_action.generic.callback = CustomizeControlsFunc;

	s_options_defaults_action.generic.type  = MTYPE_ACTION;
	s_options_defaults_action.generic.x     = 0;
	s_options_defaults_action.generic.y     = 150;
	s_options_defaults_action.generic.name  = "reset defaults";
	s_options_defaults_action.generic.callback = ControlsResetDefaultsFunc;

	s_options_console_action.generic.type   = MTYPE_ACTION;
	s_options_console_action.generic.x      = 0;
	s_options_console_action.generic.y      = 160;
	s_options_console_action.generic.name   = "go to console";
	s_options_console_action.generic.callback = ConsoleFunc;

	ControlsSetMenuItemValues();

	Menu_AddItem(&s_options_menu, (void*)&s_options_sfxvolume_slider);
	Menu_AddItem(&s_options_menu, (void*)&s_options_cdvolume_box);
	Menu_AddItem(&s_options_menu, (void*)&s_options_sensitivity_slider);
	Menu_AddItem(&s_options_menu, (void*)&s_options_alwaysrun_box);
	Menu_AddItem(&s_options_menu, (void*)&s_options_invertmouse_box);
	Menu_AddItem(&s_options_menu, (void*)&s_options_lookspring_box);
	Menu_AddItem(&s_options_menu, (void*)&s_options_freelook_box);
	Menu_AddItem(&s_options_menu, (void*)&s_options_crosshair_box);
	Menu_AddItem(&s_options_menu, (void*)&s_options_joystick_box);
	Menu_AddItem(&s_options_menu, (void*)&s_options_customize_options_action);
	Menu_AddItem(&s_options_menu, (void*)&s_options_defaults_action);
	Menu_AddItem(&s_options_menu, (void*)&s_options_console_action);
}

void Options_MenuDraw(void)
{
	MQ2_Banner("m_banner_options");
	Menu_AdjustCursor(&s_options_menu, 1);
	Menu_Draw(&s_options_menu);
}

const char* Options_MenuKey(int key)
{
	return Default_MenuKey(&s_options_menu, key);
}

void M_Menu_Options_f(void)
{
	Options_MenuInit();
	MQ2_PushMenu(Options_MenuDraw, Options_MenuKey, NULL);
}

/*
=======================================================================

VIDEO MENU

=======================================================================
*/

#define REF_OPENGL  1

extern Cvar* r_gamma;
extern Cvar* r_fullscreen;

static menuframework_s s_opengl_menu;

static menulist_s s_mode_list;
static menuslider_s s_tq_slider;
static menuslider_s s_screensize_slider;
static menuslider_s s_brightness_slider;
static menulist_s s_fs_box;
static menulist_s s_finish_box;
static menuaction_s s_cancel_action;
static menuaction_s s_defaults_action;

static void VID_MenuInit();

static void ScreenSizeCallback(void* s)
{
	menuslider_s* slider = (menuslider_s*)s;

	Cvar_SetValueLatched("viewsize", slider->curvalue * 10);
}

static void BrightnessCallback(void* s)
{
	menuslider_s* slider = (menuslider_s*)s;

	float gamma = (0.8 - (slider->curvalue / 10.0 - 0.5)) + 0.5;

	Cvar_SetValue("r_gamma", gamma);
}

static void ResetDefaults(void* unused)
{
	VID_MenuInit();
}

static void ApplyChanges(void* unused)
{
	float gamma;

	/*
	** scale to a range of 0.8 to 1.5
	*/
	gamma = s_brightness_slider.curvalue / 10.0;

	Cvar_SetValueLatched("r_gamma", gamma);
	Cvar_SetValueLatched("r_picmip", 3 - s_tq_slider.curvalue);
	Cvar_SetValueLatched("r_fullscreen", s_fs_box.curvalue);
	Cvar_SetValueLatched("r_finish", s_finish_box.curvalue);
	Cvar_SetValueLatched("r_mode", s_mode_list.curvalue);

	MQ2_ForceMenuOff();
}

static void CancelChanges(void* unused)
{
	MQ2_PopMenu();
}

/*
** VID_MenuInit
*/
static void VID_MenuInit(void)
{
	static const char* resolutions[] =
	{
		"[320 240  ]",
		"[400 300  ]",
		"[512 384  ]",
		"[640 480  ]",
		"[800 600  ]",
		"[960 720  ]",
		"[1024 768 ]",
		"[1152 864 ]",
		"[1280 1024]",
		"[1600 1200]",
		0
	};
	static const char* yesno_names[] =
	{
		"no",
		"yes",
		0
	};

	if (!r_picmip)
	{
		r_picmip = Cvar_Get("r_picmip", "0", 0);
	}
	if (!r_mode)
	{
		r_mode = Cvar_Get("r_mode", "3", 0);
	}
	if (!r_finish)
	{
		r_finish = Cvar_Get("r_finish", "0", CVAR_ARCHIVE);
	}

	s_mode_list.curvalue = r_mode->value;

	if (!scr_viewsize)
	{
		scr_viewsize = Cvar_Get("viewsize", "100", CVAR_ARCHIVE);
	}

	s_screensize_slider.curvalue = scr_viewsize->value / 10;

	s_opengl_menu.x = viddef.width * 0.50;
	s_opengl_menu.nitems = 0;

	s_mode_list.generic.type = MTYPE_SPINCONTROL;
	s_mode_list.generic.name = "video mode";
	s_mode_list.generic.x = 0;
	s_mode_list.generic.y = 0;
	s_mode_list.itemnames = resolutions;

	s_screensize_slider.generic.type    = MTYPE_SLIDER;
	s_screensize_slider.generic.x       = 0;
	s_screensize_slider.generic.y       = 10;
	s_screensize_slider.generic.name    = "screen size";
	s_screensize_slider.minvalue = 3;
	s_screensize_slider.maxvalue = 12;
	s_screensize_slider.generic.callback = ScreenSizeCallback;

	s_brightness_slider.generic.type    = MTYPE_SLIDER;
	s_brightness_slider.generic.x   = 0;
	s_brightness_slider.generic.y   = 20;
	s_brightness_slider.generic.name    = "brightness";
	s_brightness_slider.generic.callback = BrightnessCallback;
	s_brightness_slider.minvalue = 8;
	s_brightness_slider.maxvalue = 20;
	s_brightness_slider.curvalue = r_gamma->value * 10;

	s_fs_box.generic.type = MTYPE_SPINCONTROL;
	s_fs_box.generic.x  = 0;
	s_fs_box.generic.y  = 30;
	s_fs_box.generic.name   = "fullscreen";
	s_fs_box.itemnames = yesno_names;
	s_fs_box.curvalue = r_fullscreen->value;

	s_tq_slider.generic.type    = MTYPE_SLIDER;
	s_tq_slider.generic.x       = 0;
	s_tq_slider.generic.y       = 40;
	s_tq_slider.generic.name    = "texture quality";
	s_tq_slider.minvalue = 0;
	s_tq_slider.maxvalue = 3;
	s_tq_slider.curvalue = 3 - r_picmip->value;

	s_finish_box.generic.type = MTYPE_SPINCONTROL;
	s_finish_box.generic.x  = 0;
	s_finish_box.generic.y  = 50;
	s_finish_box.generic.name   = "sync every frame";
	s_finish_box.curvalue = r_finish->value;
	s_finish_box.itemnames = yesno_names;

	s_defaults_action.generic.type = MTYPE_ACTION;
	s_defaults_action.generic.name = "reset to defaults";
	s_defaults_action.generic.x    = 0;
	s_defaults_action.generic.y    = 60;
	s_defaults_action.generic.callback = ResetDefaults;

	s_cancel_action.generic.type = MTYPE_ACTION;
	s_cancel_action.generic.name = "cancel";
	s_cancel_action.generic.x    = 0;
	s_cancel_action.generic.y    = 70;
	s_cancel_action.generic.callback = CancelChanges;

	Menu_AddItem(&s_opengl_menu, (void*)&s_mode_list);
	Menu_AddItem(&s_opengl_menu, (void*)&s_screensize_slider);
	Menu_AddItem(&s_opengl_menu, (void*)&s_brightness_slider);
	Menu_AddItem(&s_opengl_menu, (void*)&s_fs_box);
	Menu_AddItem(&s_opengl_menu, (void*)&s_tq_slider);
	Menu_AddItem(&s_opengl_menu, (void*)&s_finish_box);

	Menu_AddItem(&s_opengl_menu, (void*)&s_defaults_action);
	Menu_AddItem(&s_opengl_menu, (void*)&s_cancel_action);

	Menu_Center(&s_opengl_menu);
	s_opengl_menu.x -= 8;
}

/*
================
VID_MenuDraw
================
*/
static void VID_MenuDraw(void)
{
	int w, h;

	/*
	** draw the banner
	*/
	R_GetPicSize(&w, &h, "m_banner_video");
	UI_DrawNamedPic(viddef.width / 2 - w / 2, viddef.height / 2 - 110, "m_banner_video");

	/*
	** move cursor to a reasonable starting position
	*/
	Menu_AdjustCursor(&s_opengl_menu, 1);

	/*
	** draw the menu
	*/
	Menu_Draw(&s_opengl_menu);
}

/*
================
VID_MenuKey
================
*/
static const char* VID_MenuKey(int key)
{
	menuframework_s* m = &s_opengl_menu;
	static const char* sound = "misc/menu1.wav";

	switch (key)
	{
	case K_ESCAPE:
		ApplyChanges(0);
		return NULL;
	case K_KP_UPARROW:
	case K_UPARROW:
		m->cursor--;
		Menu_AdjustCursor(m, -1);
		break;
	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		m->cursor++;
		Menu_AdjustCursor(m, 1);
		break;
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
		Menu_SlideItem(m, -1);
		break;
	case K_KP_RIGHTARROW:
	case K_RIGHTARROW:
		Menu_SlideItem(m, 1);
		break;
	case K_KP_ENTER:
	case K_ENTER:
		if (!Menu_SelectItem(m))
		{
			ApplyChanges(NULL);
		}
		break;
	}

	return sound;
}

void M_Menu_Video_f(void)
{
	VID_MenuInit();
	MQ2_PushMenu(VID_MenuDraw, VID_MenuKey, NULL);
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
	Cmd_AddCommand("menu_video", M_Menu_Video_f);
	Cmd_AddCommand("menu_options", M_Menu_Options_f);
	Cmd_AddCommand("menu_keys", M_Menu_Keys_f);
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
