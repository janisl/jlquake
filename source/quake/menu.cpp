/*
Copyright (C) 1996-1997 Id Software, Inc.

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
#include "quakedef.h"

extern Cvar* r_gamma;

void M_Menu_Keys_f(void);
void M_Menu_Video_f(void);

void M_Setup_Draw(void);
void M_Options_Draw(void);
void M_Keys_Draw(void);
void M_Video_Draw(void);
void M_Help_Draw(void);
void M_Quit_Draw(void);
void M_SerialConfig_Draw(void);
void M_ModemConfig_Draw(void);

void M_Setup_Key(int key);
void M_Options_Key(int key);
void M_Keys_Key(int key);
void M_Video_Key(int key);
void M_Help_Key(int key);
void M_Quit_Key(int key);
void M_SerialConfig_Key(int key);
void M_ModemConfig_Key(int key);

qboolean m_recursiveDraw;

byte translationTable[256];

static byte menuplyr_pixels[4096];
image_t* translate_texture;

//=============================================================================

/*
================
M_ToggleMenu_f
================
*/
void M_ToggleMenu_f(void)
{
	mqh_entersound = true;

	if (in_keyCatchers & KEYCATCH_UI)
	{
		if (m_state != m_main)
		{
			MQH_Menu_Main_f();
			return;
		}
		in_keyCatchers &= ~KEYCATCH_UI;
		m_state = m_none;
		return;
	}
	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		Con_ToggleConsole_f();
	}
	else
	{
		MQH_Menu_Main_f();
	}
}

//=============================================================================
/* SETUP MENU */

void M_Setup_Draw(void)
{
	image_t* p;

	MQH_DrawPic(16, 4, R_CachePic("gfx/qplaque.lmp"));
	p = R_CachePic("gfx/p_multi.lmp");
	MQH_DrawPic((320 - R_GetImageWidth(p)) / 2, 4, p);

	MQH_Print(64, 40, "Hostname");
	MQH_DrawField(168, 40, &setup_hostname, mqh_setup_cursor == 0);

	MQH_Print(64, 56, "Your name");
	MQH_DrawField(168, 56, &setup_myname, mqh_setup_cursor == 1);

	MQH_Print(64, 80, "Shirt color");
	MQH_Print(64, 104, "Pants color");

	MQH_DrawTextBox(64, 140 - 8, 14, 1);
	MQH_Print(72, 140, "Accept Changes");

	p = R_CachePic("gfx/bigbox.lmp");
	MQH_DrawPic(160, 64, p);
	p = R_CachePicWithTransPixels("gfx/menuplyr.lmp", menuplyr_pixels);
	CL_CalcQuakeSkinTranslation(setup_top, setup_bottom, translationTable);
	R_CreateOrUpdateTranslatedImage(translate_texture, "*translate_pic", menuplyr_pixels, translationTable, R_GetImageWidth(p), R_GetImageHeight(p));
	MQH_DrawPic(172, 72, translate_texture);

	MQH_DrawCharacter(56, setup_cursor_table_q1 [mqh_setup_cursor], 12 + ((int)(realtime * 4) & 1));
}


void M_Setup_Key(int k)
{
	switch (k)
	{
	case K_ESCAPE:
		MQH_Menu_MultiPlayer_f();
		break;

	case K_UPARROW:
		S_StartLocalSound("misc/menu1.wav");
		mqh_setup_cursor--;
		if (mqh_setup_cursor < 0)
		{
			mqh_setup_cursor = NUM_SETUP_CMDS_Q1 - 1;
		}
		break;

	case K_DOWNARROW:
		S_StartLocalSound("misc/menu1.wav");
		mqh_setup_cursor++;
		if (mqh_setup_cursor >= NUM_SETUP_CMDS_Q1)
		{
			mqh_setup_cursor = 0;
		}
		break;

	case K_LEFTARROW:
		if (mqh_setup_cursor < 2)
		{
			break;
		}
		S_StartLocalSound("misc/menu3.wav");
		if (mqh_setup_cursor == 2)
		{
			setup_top = setup_top - 1;
		}
		if (mqh_setup_cursor == 3)
		{
			setup_bottom = setup_bottom - 1;
		}
		break;
	case K_RIGHTARROW:
		if (mqh_setup_cursor < 2)
		{
			break;
		}
forward:
		S_StartLocalSound("misc/menu3.wav");
		if (mqh_setup_cursor == 2)
		{
			setup_top = setup_top + 1;
		}
		if (mqh_setup_cursor == 3)
		{
			setup_bottom = setup_bottom + 1;
		}
		break;

	case K_ENTER:
		if (mqh_setup_cursor == 0 || mqh_setup_cursor == 1)
		{
			return;
		}

		if (mqh_setup_cursor == 2 || mqh_setup_cursor == 3)
		{
			goto forward;
		}

		// mqh_setup_cursor == 4 (OK)
		if (String::Cmp(clqh_name->string, setup_myname.buffer) != 0)
		{
			Cbuf_AddText(va("name \"%s\"\n", setup_myname.buffer));
		}
		if (String::Cmp(sv_hostname->string, setup_hostname.buffer) != 0)
		{
			Cvar_Set("hostname", setup_hostname.buffer);
		}
		if (setup_top != setup_oldtop || setup_bottom != setup_oldbottom)
		{
			Cbuf_AddText(va("color %i %i\n", setup_top, setup_bottom));
		}
		mqh_entersound = true;
		MQH_Menu_MultiPlayer_f();
		break;
	}
	if (mqh_setup_cursor == 0)
	{
		Field_KeyDownEvent(&setup_hostname, k);
	}
	if (mqh_setup_cursor == 1)
	{
		Field_KeyDownEvent(&setup_myname, k);
	}

	if (setup_top > 13)
	{
		setup_top = 0;
	}
	if (setup_top < 0)
	{
		setup_top = 13;
	}
	if (setup_bottom > 13)
	{
		setup_bottom = 0;
	}
	if (setup_bottom < 0)
	{
		setup_bottom = 13;
	}
}

void M_Setup_Char(int k)
{
	if (mqh_setup_cursor == 0)
	{
		Field_CharEvent(&setup_hostname, k);
	}
	if (mqh_setup_cursor == 1)
	{
		Field_CharEvent(&setup_myname, k);
	}
}

//=============================================================================
/* OPTIONS MENU */

void M_AdjustSliders(int dir)
{
	S_StartLocalSound("misc/menu3.wav");

	switch (mqh_options_cursor)
	{
	case 3:	// screen size
		scr_viewsize->value += dir * 10;
		if (scr_viewsize->value < 30)
		{
			scr_viewsize->value = 30;
		}
		if (scr_viewsize->value > 120)
		{
			scr_viewsize->value = 120;
		}
		Cvar_SetValue("viewsize", scr_viewsize->value);
		break;
	case 4:	// gamma
		r_gamma->value += dir * 0.1;
		if (r_gamma->value < 1)
		{
			r_gamma->value = 1;
		}
		if (r_gamma->value > 2)
		{
			r_gamma->value = 2;
		}
		Cvar_SetValue("r_gamma", r_gamma->value);
		break;
	case 5:	// mouse speed
		cl_sensitivity->value += dir * 0.5;
		if (cl_sensitivity->value < 1)
		{
			cl_sensitivity->value = 1;
		}
		if (cl_sensitivity->value > 11)
		{
			cl_sensitivity->value = 11;
		}
		Cvar_SetValue("sensitivity", cl_sensitivity->value);
		break;
	case 6:	// music volume
#ifdef _WIN32
		bgmvolume->value += dir * 1.0;
#else
		bgmvolume->value += dir * 0.1;
#endif
		if (bgmvolume->value < 0)
		{
			bgmvolume->value = 0;
		}
		if (bgmvolume->value > 1)
		{
			bgmvolume->value = 1;
		}
		Cvar_SetValue("bgmvolume", bgmvolume->value);
		break;
	case 7:	// sfx volume
		s_volume->value += dir * 0.1;
		if (s_volume->value < 0)
		{
			s_volume->value = 0;
		}
		if (s_volume->value > 1)
		{
			s_volume->value = 1;
		}
		Cvar_SetValue("s_volume", s_volume->value);
		break;

	case 8:	// allways run
		if (cl_forwardspeed->value > 200)
		{
			Cvar_SetValue("cl_forwardspeed", 200);
			Cvar_SetValue("cl_backspeed", 200);
		}
		else
		{
			Cvar_SetValue("cl_forwardspeed", 400);
			Cvar_SetValue("cl_backspeed", 400);
		}
		break;

	case 9:	// invert mouse
		Cvar_SetValue("m_pitch", -m_pitch->value);
		break;

	case 10:	// lookspring
		Cvar_SetValue("lookspring", !lookspring->value);
		break;
	}
}


void M_DrawSlider(int x, int y, float range)
{
	int i;

	if (range < 0)
	{
		range = 0;
	}
	if (range > 1)
	{
		range = 1;
	}
	MQH_DrawCharacter(x - 8, y, 128);
	for (i = 0; i < SLIDER_RANGE; i++)
		MQH_DrawCharacter(x + i * 8, y, 129);
	MQH_DrawCharacter(x + i * 8, y, 130);
	MQH_DrawCharacter(x + (SLIDER_RANGE - 1) * 8 * range, y, 131);
}

void M_DrawCheckbox(int x, int y, int on)
{
#if 0
	if (on)
	{
		MQH_DrawCharacter(x, y, 131);
	}
	else
	{
		MQH_DrawCharacter(x, y, 129);
	}
#endif
	if (on)
	{
		MQH_Print(x, y, "on");
	}
	else
	{
		MQH_Print(x, y, "off");
	}
}

void M_Options_Draw(void)
{
	float r;
	image_t* p;

	MQH_DrawPic(16, 4, R_CachePic("gfx/qplaque.lmp"));
	p = R_CachePic("gfx/p_option.lmp");
	MQH_DrawPic((320 - R_GetImageWidth(p)) / 2, 4, p);

	MQH_Print(16, 32, "    Customize controls");
	MQH_Print(16, 40, "         Go to console");
	MQH_Print(16, 48, "     Reset to defaults");

	MQH_Print(16, 56, "           Screen size");
	r = (scr_viewsize->value - 30) / (120 - 30);
	M_DrawSlider(220, 56, r);

	MQH_Print(16, 64, "            Brightness");
	r = (r_gamma->value - 1);
	M_DrawSlider(220, 64, r);

	MQH_Print(16, 72, "           Mouse Speed");
	r = (cl_sensitivity->value - 1) / 10;
	M_DrawSlider(220, 72, r);

	MQH_Print(16, 80, "       CD Music Volume");
	r = bgmvolume->value;
	M_DrawSlider(220, 80, r);

	MQH_Print(16, 88, "          Sound Volume");
	r = s_volume->value;
	M_DrawSlider(220, 88, r);

	MQH_Print(16, 96,  "            Always Run");
	M_DrawCheckbox(220, 96, cl_forwardspeed->value > 200);

	MQH_Print(16, 104, "          Invert Mouse");
	M_DrawCheckbox(220, 104, m_pitch->value < 0);

	MQH_Print(16, 112, "            Lookspring");
	M_DrawCheckbox(220, 112, lookspring->value);

	MQH_Print(16, 120, "         Video Options");

// cursor
	MQH_DrawCharacter(200, 32 + mqh_options_cursor * 8, 12 + ((int)(realtime * 4) & 1));
}


void M_Options_Key(int k)
{
	switch (k)
	{
	case K_ESCAPE:
		MQH_Menu_Main_f();
		break;

	case K_ENTER:
		mqh_entersound = true;
		switch (mqh_options_cursor)
		{
		case 0:
			M_Menu_Keys_f();
			break;
		case 1:
			m_state = m_none;
			Con_ToggleConsole_f();
			break;
		case 2:
			Cbuf_AddText("exec default.cfg\n");
			break;
		case 11:
			M_Menu_Video_f();
			break;
		default:
			M_AdjustSliders(1);
			break;
		}
		return;

	case K_UPARROW:
		S_StartLocalSound("misc/menu1.wav");
		mqh_options_cursor--;
		if (mqh_options_cursor < 0)
		{
			mqh_options_cursor = OPTIONS_ITEMS_Q1 - 1;
		}
		break;

	case K_DOWNARROW:
		S_StartLocalSound("misc/menu1.wav");
		mqh_options_cursor++;
		if (mqh_options_cursor >= OPTIONS_ITEMS_Q1)
		{
			mqh_options_cursor = 0;
		}
		break;

	case K_LEFTARROW:
		M_AdjustSliders(-1);
		break;

	case K_RIGHTARROW:
		M_AdjustSliders(1);
		break;
	}
}

//=============================================================================
/* KEYS MENU */

const char* bindnames[][2] =
{
	{"+attack",         "attack"},
	{"impulse 10",      "change weapon"},
	{"+jump",           "jump / swim up"},
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
	{"+moveup",         "swim up"},
	{"+movedown",       "swim down"}
};

#define NUMCOMMANDS (sizeof(bindnames) / sizeof(bindnames[0]))

int keys_cursor;
int bind_grab;

void M_Menu_Keys_f(void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_keys;
	mqh_entersound = true;
}


void M_FindKeysForCommand(const char* command, int* twokeys)
{
	int count;
	int j;
	int l;
	char* b;

	twokeys[0] = twokeys[1] = -1;
	l = String::Length(command);
	count = 0;

	for (j = 0; j < 256; j++)
	{
		b = keys[j].binding;
		if (!b)
		{
			continue;
		}
		if (!String::NCmp(b, command, l))
		{
			twokeys[count] = j;
			count++;
			if (count == 2)
			{
				break;
			}
		}
	}
}

void M_UnbindCommand(const char* command)
{
	int j;
	int l;
	char* b;

	l = String::Length(command);

	for (j = 0; j < 256; j++)
	{
		b = keys[j].binding;
		if (!b)
		{
			continue;
		}
		if (!String::NCmp(b, command, l))
		{
			Key_SetBinding(j, "");
		}
	}
}


void M_Keys_Draw(void)
{
	int i, l;
	int keys[2];
	const char* name;
	int x, y;
	image_t* p;

	p = R_CachePic("gfx/ttl_cstm.lmp");
	MQH_DrawPic((320 - R_GetImageWidth(p)) / 2, 4, p);

	if (bind_grab)
	{
		MQH_Print(12, 32, "Press a key or button for this action");
	}
	else
	{
		MQH_Print(18, 32, "Enter to change, backspace to clear");
	}

// search for known bindings
	for (i = 0; i < (int)NUMCOMMANDS; i++)
	{
		y = 48 + 8 * i;

		MQH_Print(16, y, bindnames[i][1]);

		l = String::Length(bindnames[i][0]);

		M_FindKeysForCommand(bindnames[i][0], keys);

		if (keys[0] == -1)
		{
			MQH_Print(140, y, "???");
		}
		else
		{
			name = Key_KeynumToString(keys[0]);
			MQH_Print(140, y, name);
			x = String::Length(name) * 8;
			if (keys[1] != -1)
			{
				MQH_Print(140 + x + 8, y, "or");
				MQH_Print(140 + x + 32, y, Key_KeynumToString(keys[1]));
			}
		}
	}

	if (bind_grab)
	{
		MQH_DrawCharacter(130, 48 + keys_cursor * 8, '=');
	}
	else
	{
		MQH_DrawCharacter(130, 48 + keys_cursor * 8, 12 + ((int)(realtime * 4) & 1));
	}
}


void M_Keys_Key(int k)
{
	char cmd[80];
	int keys[2];

	if (bind_grab)
	{	// defining a key
		S_StartLocalSound("misc/menu1.wav");
		if (k == K_ESCAPE)
		{
			bind_grab = false;
		}
		else if (k != '`')
		{
			sprintf(cmd, "bind \"%s\" \"%s\"\n", Key_KeynumToString(k), bindnames[keys_cursor][0]);
			Cbuf_InsertText(cmd);
		}

		bind_grab = false;
		return;
	}

	switch (k)
	{
	case K_ESCAPE:
		MQH_Menu_Options_f();
		break;

	case K_LEFTARROW:
	case K_UPARROW:
		S_StartLocalSound("misc/menu1.wav");
		keys_cursor--;
		if (keys_cursor < 0)
		{
			keys_cursor = NUMCOMMANDS - 1;
		}
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_StartLocalSound("misc/menu1.wav");
		keys_cursor++;
		if (keys_cursor >= (int)NUMCOMMANDS)
		{
			keys_cursor = 0;
		}
		break;

	case K_ENTER:		// go into bind mode
		M_FindKeysForCommand(bindnames[keys_cursor][0], keys);
		S_StartLocalSound("misc/menu2.wav");
		if (keys[1] != -1)
		{
			M_UnbindCommand(bindnames[keys_cursor][0]);
		}
		bind_grab = true;
		break;

	case K_BACKSPACE:		// delete bindings
	case K_DEL:				// delete bindings
		S_StartLocalSound("misc/menu2.wav");
		M_UnbindCommand(bindnames[keys_cursor][0]);
		break;
	}
}

//=============================================================================
/* VIDEO MENU */

#define MAX_COLUMN_SIZE     9
#define MODE_AREA_HEIGHT    (MAX_COLUMN_SIZE + 2)

void M_Menu_Video_f(void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_video;
	mqh_entersound = true;
}


void M_Video_Draw(void)
{
	image_t* p = R_CachePic("gfx/vidmodes.lmp");
	MQH_DrawPic((320 - R_GetImageWidth(p)) / 2, 4, p);

	MQH_Print(3 * 8, 36 + MODE_AREA_HEIGHT * 8 + 8 * 2,
		"Video modes must be set from the");
	MQH_Print(3 * 8, 36 + MODE_AREA_HEIGHT * 8 + 8 * 3,
		"console with set r_mode <number>");
	MQH_Print(3 * 8, 36 + MODE_AREA_HEIGHT * 8 + 8 * 4,
		"and set r_colorbits <bits-per-pixel>");
	MQH_Print(3 * 8, 36 + MODE_AREA_HEIGHT * 8 + 8 * 6,
		"Select windowed mode with set r_fullscreen 0");
}


void M_Video_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
		S_StartLocalSound("misc/menu1.wav");
		MQH_Menu_Options_f();
		break;

	default:
		break;
	}
}

//=============================================================================
/* HELP MENU */

void M_Help_Draw(void)
{
	MQH_DrawPic(0, 0, R_CachePic(va("gfx/help%i.lmp", mqh_help_page)));
}


void M_Help_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
		MQH_Menu_Main_f();
		break;

	case K_UPARROW:
	case K_RIGHTARROW:
		mqh_entersound = true;
		if (++mqh_help_page >= NUM_HELP_PAGES_Q1)
		{
			mqh_help_page = 0;
		}
		break;

	case K_DOWNARROW:
	case K_LEFTARROW:
		mqh_entersound = true;
		if (--mqh_help_page < 0)
		{
			mqh_help_page = NUM_HELP_PAGES_Q1 - 1;
		}
		break;
	}

}

//=============================================================================
/* QUIT MENU */

void M_Quit_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case 'n':
	case 'N':
		if (wasInMenus)
		{
			m_state = m_quit_prevstate;
			mqh_entersound = true;
		}
		else
		{
			in_keyCatchers &= ~KEYCATCH_UI;
			m_state = m_none;
		}
		break;

	case 'Y':
	case 'y':
		in_keyCatchers |= KEYCATCH_CONSOLE;
		Host_Quit_f();
		break;

	default:
		break;
	}

}


void M_Quit_Draw(void)
{
	if (wasInMenus)
	{
		m_state = m_quit_prevstate;
		m_recursiveDraw = true;
		M_Draw();
		m_state = m_quit;
	}

#ifdef _WIN32
	MQH_DrawTextBox(0, 0, 38, 23);
	MQH_PrintWhite(16, 12,  "  Quake version 1.09 by id Software\n\n");
	MQH_PrintWhite(16, 28,  "Programming        Art \n");
	MQH_Print(16, 36,  " John Carmack       Adrian Carmack\n");
	MQH_Print(16, 44,  " Michael Abrash     Kevin Cloud\n");
	MQH_Print(16, 52,  " John Cash          Paul Steed\n");
	MQH_Print(16, 60,  " Dave 'Zoid' Kirsch\n");
	MQH_PrintWhite(16, 68,  "Design             Biz\n");
	MQH_Print(16, 76,  " John Romero        Jay Wilbur\n");
	MQH_Print(16, 84,  " Sandy Petersen     Mike Wilson\n");
	MQH_Print(16, 92,  " American McGee     Donna Jackson\n");
	MQH_Print(16, 100,  " Tim Willits        Todd Hollenshead\n");
	MQH_PrintWhite(16, 108, "Support            Projects\n");
	MQH_Print(16, 116, " Barrett Alexander  Shawn Green\n");
	MQH_PrintWhite(16, 124, "Sound Effects\n");
	MQH_Print(16, 132, " Trent Reznor and Nine Inch Nails\n\n");
	MQH_PrintWhite(16, 140, "Quake is a trademark of Id Software,\n");
	MQH_PrintWhite(16, 148, "inc., (c)1996 Id Software, inc. All\n");
	MQH_PrintWhite(16, 156, "rights reserved. NIN logo is a\n");
	MQH_PrintWhite(16, 164, "registered trademark licensed to\n");
	MQH_PrintWhite(16, 172, "Nothing Interactive, Inc. All rights\n");
	MQH_PrintWhite(16, 180, "reserved. Press y to exit\n");
#else
	MQH_DrawTextBox(56, 76, 24, 4);
	MQH_Print(64, 84,  mq1_quitMessage[msgNumber * 4 + 0]);
	MQH_Print(64, 92,  mq1_quitMessage[msgNumber * 4 + 1]);
	MQH_Print(64, 100, mq1_quitMessage[msgNumber * 4 + 2]);
	MQH_Print(64, 108, mq1_quitMessage[msgNumber * 4 + 3]);
#endif
}

//=============================================================================
/* Menu Subsystem */


void M_Init(void)
{
	Cmd_AddCommand("togglemenu", M_ToggleMenu_f);

	MQH_Init();
	Cmd_AddCommand("menu_keys", M_Menu_Keys_f);
	Cmd_AddCommand("menu_video", M_Menu_Video_f);
}


void M_Draw(void)
{
	if (m_state == m_none || !(in_keyCatchers & KEYCATCH_UI))
	{
		return;
	}

	if (!m_recursiveDraw)
	{
		if (con.displayFrac)
		{
			Con_DrawFullBackground();
			S_ExtraUpdate();
		}
		else
		{
			Draw_FadeScreen();
		}
	}
	else
	{
		m_recursiveDraw = false;
	}

	MQH_Draw();
	switch (m_state)
	{

	case m_setup:
		M_Setup_Draw();
		break;

	case m_options:
		M_Options_Draw();
		break;

	case m_keys:
		M_Keys_Draw();
		break;

	case m_video:
		M_Video_Draw();
		break;

	case m_help:
		M_Help_Draw();
		break;

	case m_quit:
		M_Quit_Draw();
		break;
	}

	if (mqh_entersound)
	{
		S_StartLocalSound("misc/menu2.wav");
		mqh_entersound = false;
	}

	S_ExtraUpdate();
}


void M_Keydown(int key)
{
	switch (m_state)
	{

	case m_setup:
		M_Setup_Key(key);
		return;

	case m_options:
		M_Options_Key(key);
		return;

	case m_keys:
		M_Keys_Key(key);
		return;

	case m_video:
		M_Video_Key(key);
		return;

	case m_help:
		M_Help_Key(key);
		return;

	case m_quit:
		M_Quit_Key(key);
		return;
	}
	MQH_Keydown(key);
}

void M_CharEvent(int key)
{
	switch (m_state)
	{
	case m_setup:
		M_Setup_Char(key);
		break;
	default:
		break;
	}
	MQH_CharEvent(key);
}
