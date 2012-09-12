/*
 * $Header: /H2 Mission Pack/Menu.c 41    3/20/98 2:03p Jmonroe $
 */

#include "quakedef.h"

extern Cvar* r_gamma;

extern Cvar* crosshair;

void M_Menu_Keys_f(void);
void M_Menu_Video_f(void);

void M_Options_Draw(void);
void M_Keys_Draw(void);
void M_Video_Draw(void);
void M_Help_Draw(void);
void M_Quit_Draw(void);
void M_SerialConfig_Draw(void);
void M_ModemConfig_Draw(void);

void M_Options_Key(int key);
void M_Keys_Key(int key);
void M_Video_Key(int key);
void M_Help_Key(int key);
void M_Quit_Key(int key);
void M_SerialConfig_Key(int key);
void M_ModemConfig_Key(int key);

qboolean m_recursiveDraw;

extern qboolean introPlaying;

//=============================================================================
/* Support Routines */

void M_Print2(int cx, int cy, const char* str)
{
	UI_DrawString(cx + ((viddef.width - 320) >> 1), cy + ((viddef.height - 200) >> 1), str, 256);
}

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
			LogoTargetPercent = TitleTargetPercent = 1;
			LogoPercent = TitlePercent = 0;
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
		LogoTargetPercent = TitleTargetPercent = 1;
		LogoPercent = TitlePercent = 0;
		MQH_Menu_Main_f();
	}
}

const char* plaquemessage = NULL;	// Pointer to current plaque message
char* errormessage = NULL;

//=============================================================================
/* MULTIPLAYER LOAD/SAVE MENU */

void M_MLoad_Key(int k)
{
	switch (k)
	{
	case K_ESCAPE:
		MQH_Menu_MultiPlayer_f();
		break;

	case K_ENTER:
		S_StartLocalSound("raven/menu2.wav");
		if (!mqh_loadable[mqh_load_cursor])
		{
			return;
		}
		m_state = m_none;
		in_keyCatchers &= ~KEYCATCH_UI;

		if (sv.state != SS_DEAD)
		{
			Cbuf_AddText("disconnect\n");
		}
		Cbuf_AddText("listen 1\n");		// so host_netport will be re-examined

		// Host_Loadgame_f can't bring up the loading plaque because too much
		// stack space has been used, so do it now
		SCRQH_BeginLoadingPlaque();

		// issue the load command
		Cbuf_AddText(va("load ms%i\n", mqh_load_cursor));
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_StartLocalSound("raven/menu1.wav");
		mqh_load_cursor--;
		if (mqh_load_cursor < 0)
		{
			mqh_load_cursor = MAX_SAVEGAMES - 1;
		}
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_StartLocalSound("raven/menu1.wav");
		mqh_load_cursor++;
		if (mqh_load_cursor >= MAX_SAVEGAMES)
		{
			mqh_load_cursor = 0;
		}
		break;
	}
}


void M_MSave_Key(int k)
{
	switch (k)
	{
	case K_ESCAPE:
		MQH_Menu_MultiPlayer_f();
		break;

	case K_ENTER:
		m_state = m_none;
		in_keyCatchers &= ~KEYCATCH_UI;
		Cbuf_AddText(va("save ms%i\n", mqh_load_cursor));
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_StartLocalSound("raven/menu1.wav");
		mqh_load_cursor--;
		if (mqh_load_cursor < 0)
		{
			mqh_load_cursor = MAX_SAVEGAMES - 1;
		}
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_StartLocalSound("raven/menu1.wav");
		mqh_load_cursor++;
		if (mqh_load_cursor >= MAX_SAVEGAMES)
		{
			mqh_load_cursor = 0;
		}
		break;
	}
}

//=============================================================================
/* OPTIONS MENU */

void M_AdjustSliders(int dir)
{
	S_StartLocalSound("raven/menu3.wav");

	switch (mqh_options_cursor)
	{
	case OPT_SCRSIZE:	// screen size
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
		SbarH2_ViewSizeChanged();
		break;
	case OPT_GAMMA:	// gamma
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
	case OPT_MOUSESPEED:	// mouse speed
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
	case OPT_MUSICTYPE:	// bgm type
		if (String::ICmp(bgmtype->string,"midi") == 0)
		{
			if (dir < 0)
			{
				Cvar_Set("bgmtype","none");
			}
			else
			{
				Cvar_Set("bgmtype","cd");
			}
		}
		else if (String::ICmp(bgmtype->string,"cd") == 0)
		{
			if (dir < 0)
			{
				Cvar_Set("bgmtype","midi");
			}
			else
			{
				Cvar_Set("bgmtype","none");
			}
		}
		else
		{
			if (dir < 0)
			{
				Cvar_Set("bgmtype","cd");
			}
			else
			{
				Cvar_Set("bgmtype","midi");
			}
		}
		break;

	case OPT_MUSICVOL:	// music volume
		bgmvolume->value += dir * 0.1;

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
	case OPT_SNDVOL:	// sfx volume
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

	case OPT_ALWAYRUN:	// allways run
		if (cl_forwardspeed->value > 200)
		{
			Cvar_SetValue("cl_forwardspeed", 200);
		}
		else
		{
			Cvar_SetValue("cl_forwardspeed", 400);
		}
		break;

	case OPT_INVMOUSE:	// invert mouse
		Cvar_SetValue("m_pitch", -m_pitch->value);
		break;

	case OPT_LOOKSPRING:	// lookspring
		Cvar_SetValue("lookspring", !lookspring->value);
		break;

	case OPT_CROSSHAIR:
		Cvar_SetValue("crosshair", !crosshair->value);
		break;

	case OPT_ALWAYSMLOOK:
		Cvar_SetValue("cl_freelook", !cl_freelook->value);
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
	MQH_DrawCharacter(x - 8, y, 256);
	for (i = 0; i < SLIDER_RANGE; i++)
		MQH_DrawCharacter(x + i * 8, y, 257);
	MQH_DrawCharacter(x + i * 8, y, 258);
	MQH_DrawCharacter(x + (SLIDER_RANGE - 1) * 8 * range, y, 259);
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

	MH2_ScrollTitle("gfx/menu/title3.lmp");

	MQH_Print(16, 60 + (0 * 8), "    Customize controls");
	MQH_Print(16, 60 + (1 * 8), "         Go to console");
	MQH_Print(16, 60 + (2 * 8), "     Reset to defaults");

	MQH_Print(16, 60 + (3 * 8), "           Screen size");
	r = (scr_viewsize->value - 30) / (120 - 30);
	M_DrawSlider(220, 60 + (3 * 8), r);

	MQH_Print(16, 60 + (4 * 8), "            Brightness");
	r = (r_gamma->value - 1);
	M_DrawSlider(220, 60 + (4 * 8), r);

	MQH_Print(16, 60 + (5 * 8), "           Mouse Speed");
	r = (cl_sensitivity->value - 1) / 10;
	M_DrawSlider(220, 60 + (5 * 8), r);

	MQH_Print(16, 60 + (6 * 8), "            Music Type");
	if (String::ICmp(bgmtype->string,"midi") == 0)
	{
		MQH_Print(220, 60 + (6 * 8), "MIDI");
	}
	else if (String::ICmp(bgmtype->string,"cd") == 0)
	{
		MQH_Print(220, 60 + (6 * 8), "CD");
	}
	else
	{
		MQH_Print(220, 60 + (6 * 8), "None");
	}

	MQH_Print(16, 60 + (7 * 8), "          Music Volume");
	r = bgmvolume->value;
	M_DrawSlider(220, 60 + (7 * 8), r);

	MQH_Print(16, 60 + (8 * 8), "          Sound Volume");
	r = s_volume->value;
	M_DrawSlider(220, 60 + (8 * 8), r);

	MQH_Print(16, 60 + (9 * 8),              "            Always Run");
	M_DrawCheckbox(220, 60 + (9 * 8), cl_forwardspeed->value > 200);

	MQH_Print(16, 60 + (OPT_INVMOUSE * 8),   "          Invert Mouse");
	M_DrawCheckbox(220, 60 + (OPT_INVMOUSE * 8), m_pitch->value < 0);

	MQH_Print(16, 60 + (OPT_LOOKSPRING * 8), "            Lookspring");
	M_DrawCheckbox(220, 60 + (OPT_LOOKSPRING * 8), lookspring->value);

	MQH_Print(16, 60 + (OPT_CROSSHAIR * 8),  "        Show Crosshair");
	M_DrawCheckbox(220, 60 + (OPT_CROSSHAIR * 8), crosshair->value);

	MQH_Print(16,60 + (OPT_ALWAYSMLOOK * 8), "            Mouse Look");
	M_DrawCheckbox(220, 60 + (OPT_ALWAYSMLOOK * 8), cl_freelook->value);

	MQH_Print(16, 60 + (OPT_VIDEO * 8),  "         Video Options");

// cursor
	MQH_DrawCharacter(200, 60 + mqh_options_cursor * 8, 12 + ((int)(realtime * 4) & 1));
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
		case OPT_CUSTOMIZE:
			M_Menu_Keys_f();
			break;
		case OPT_CONSOLE:
			m_state = m_none;
			Con_ToggleConsole_f();
			break;
		case OPT_DEFAULTS:
			Cbuf_AddText("exec default.cfg\n");
			break;
		case OPT_VIDEO:
			M_Menu_Video_f();
			break;
		default:
			M_AdjustSliders(1);
			break;
		}
		return;

	case K_UPARROW:
		S_StartLocalSound("raven/menu1.wav");
		mqh_options_cursor--;
		if (mqh_options_cursor < 0)
		{
			mqh_options_cursor = OPTIONS_ITEMS - 1;
		}
		break;

	case K_DOWNARROW:
		S_StartLocalSound("raven/menu1.wav");
		mqh_options_cursor++;
		if (mqh_options_cursor >= OPTIONS_ITEMS)
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
	{"+crouch",         "crouch"},
	{"+lookup",         "look up"},
	{"+lookdown",       "look down"},
	{"centerview",      "center view"},
	{"+mlook",          "mouse look"},
	{"+moveup",         "swim up"},
	{"+movedown",       "swim down"},
	{"impulse 13",      "lift object"},
	{"invuse",          "use inv item"},
	{"impulse 44",      "drop inv item"},
	{"+showinfo",       "full inventory"},
	{"+showdm",         "info / frags"},
	{"toggle_dm",       "toggle frags"},
	{"+infoplaque",     "objectives"},
	{"invleft",         "inv move left"},
	{"invright",        "inv move right"},
	{"impulse 100",     "inv:torch"},
	{"impulse 101",     "inv:qrtz flask"},
	{"impulse 102",     "inv:mystic urn"},
	{"impulse 103",     "inv:krater"},
	{"impulse 104",     "inv:chaos devc"},
	{"impulse 105",     "inv:tome power"},
	{"impulse 106",     "inv:summon stn"},
	{"impulse 107",     "inv:invisiblty"},
	{"impulse 108",     "inv:glyph"},
	{"impulse 109",     "inv:boots"},
	{"impulse 110",     "inv:repulsion"},
	{"impulse 111",     "inv:bo peep"},
	{"impulse 112",     "inv:flight"},
	{"impulse 113",     "inv:force cube"},
	{"impulse 114",     "inv:icon defn"}
};

#define NUMCOMMANDS (sizeof(bindnames) / sizeof(bindnames[0]))

#define KEYS_SIZE 14

int keys_cursor;
int bind_grab;
int keys_top = 0;

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
	int l,l2;
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
			l2 = String::Length(b);
			if (l == l2)
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

	MH2_ScrollTitle("gfx/menu/title6.lmp");

//	MQH_DrawTextBox (6,56, 35,16);

//	p = R_CachePic("gfx/menu/hback.lmp");
//	MQH_DrawPic(8, 62, p);

	if (bind_grab)
	{
		MQH_Print(12, 64, "Press a key or button for this action");
	}
	else
	{
		MQH_Print(18, 64, "Enter to change, backspace to clear");
	}

	if (keys_top)
	{
		MQH_DrawCharacter(6, 80, 128);
	}
	if (keys_top + KEYS_SIZE < (int)NUMCOMMANDS)
	{
		MQH_DrawCharacter(6, 80 + ((KEYS_SIZE - 1) * 8), 129);
	}

// search for known bindings
	for (i = 0; i < KEYS_SIZE; i++)
	{
		y = 80 + 8 * i;

		MQH_Print(16, y, bindnames[i + keys_top][1]);

		l = String::Length(bindnames[i + keys_top][0]);

		M_FindKeysForCommand(bindnames[i + keys_top][0], keys);

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
		MQH_DrawCharacter(130, 80 + (keys_cursor - keys_top) * 8, '=');
	}
	else
	{
		MQH_DrawCharacter(130, 80 + (keys_cursor - keys_top) * 8, 12 + ((int)(realtime * 4) & 1));
	}
}


void M_Keys_Key(int k)
{
	char cmd[80];
	int keys[2];

	if (bind_grab)
	{	// defining a key
		S_StartLocalSound("raven/menu1.wav");
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
		S_StartLocalSound("raven/menu1.wav");
		keys_cursor--;
		if (keys_cursor < 0)
		{
			keys_cursor = NUMCOMMANDS - 1;
		}
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_StartLocalSound("raven/menu1.wav");
		keys_cursor++;
		if (keys_cursor >= (int)NUMCOMMANDS)
		{
			keys_cursor = 0;
		}
		break;

	case K_ENTER:		// go into bind mode
		M_FindKeysForCommand(bindnames[keys_cursor][0], keys);
		S_StartLocalSound("raven/menu2.wav");
		if (keys[1] != -1)
		{
			M_UnbindCommand(bindnames[keys_cursor][0]);
		}
		bind_grab = true;
		break;

	case K_BACKSPACE:		// delete bindings
	case K_DEL:				// delete bindings
		S_StartLocalSound("raven/menu2.wav");
		M_UnbindCommand(bindnames[keys_cursor][0]);
		break;
	}

	if (keys_cursor < keys_top)
	{
		keys_top = keys_cursor;
	}
	else if (keys_cursor >= keys_top + KEYS_SIZE)
	{
		keys_top = keys_cursor - KEYS_SIZE + 1;
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
	MH2_ScrollTitle("gfx/menu/title7.lmp");

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
		S_StartLocalSound("raven/menu1.wav");
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
	MQH_DrawPic(0, 0, R_CachePic(va("gfx/menu/help%02i.lmp", mqh_help_page + 1)));
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
		if (++mqh_help_page >= NUM_HELP_PAGES_H2)
		{
			mqh_help_page = 0;
		}
		break;

	case K_DOWNARROW:
	case K_LEFTARROW:
		mqh_entersound = true;
		if (--mqh_help_page < 0)
		{
			mqh_help_page = NUM_HELP_PAGES_H2 - 1;
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
	int i,x,y,place,topy;
	image_t* p;

	if (wasInMenus)
	{
		m_state = m_quit_prevstate;
		m_recursiveDraw = true;
		M_Draw();
		m_state = m_quit;
	}

	LinePos += host_frametime * 1.75;
	if (LinePos > MaxLines + QUIT_SIZE_H2 + 2)
	{
		LinePos = 0;
		SoundPlayed = false;
		LineTimes++;
		if (LineTimes >= 2)
		{
			MaxLines = MAX_LINES2_H2;
			LineText = Credit2TextH2;
			CDAudio_Play(12, false);
		}
	}

	y = 12;
	MQH_DrawTextBox(0, 0, 38, 23);
	MQH_PrintWhite(16, y,  "        Hexen II version 1.12       ");  y += 8;
	MQH_PrintWhite(16, y,  "         by Raven Software          ");  y += 16;

	if (LinePos > 55 && !SoundPlayed && LineText == Credit2TextH2)
	{
		S_StartLocalSound("rj/steve.wav");
		SoundPlayed = true;
	}
	topy = y;
	place = floor(LinePos);
	y -= floor((LinePos - place) * 8);
	for (i = 0; i < QUIT_SIZE_H2; i++,y += 8)
	{
		if (i + place - QUIT_SIZE_H2 >= MaxLines)
		{
			break;
		}
		if (i + place < QUIT_SIZE_H2)
		{
			continue;
		}

		if (LineText[i + place - QUIT_SIZE_H2][0] == ' ')
		{
			MQH_PrintWhite(24,y,LineText[i + place - QUIT_SIZE_H2]);
		}
		else
		{
			MQH_Print(24,y,LineText[i + place - QUIT_SIZE_H2]);
		}
	}

	p = R_CachePic("gfx/box_mm2.lmp");
	x = 24;
	y = topy - 8;
	for (i = 4; i < 38; i++,x += 8)
	{
		MQH_DrawPic(x, y, p);	//background at top for smooth scroll out
		MQH_DrawPic(x, y + (QUIT_SIZE_H2 * 8), p);	//draw at bottom for smooth scroll in
	}

	y += (QUIT_SIZE_H2 * 8) + 8;
	MQH_PrintWhite(16, y,  "          Press y to exit           ");
}

//=============================================================================
/* Menu Subsystem */


void M_Init(void)
{
	Cmd_AddCommand("togglemenu", M_ToggleMenu_f);

	MQH_Init();
	Cmd_AddCommand("menu_keys", M_Menu_Keys_f);
	Cmd_AddCommand("menu_video", M_Menu_Video_f);

	mh2_oldmission = Cvar_Get("m_oldmission", "0", CVAR_ARCHIVE);
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
		S_StartLocalSound("raven/menu2.wav");
		mqh_entersound = false;
	}

	S_ExtraUpdate();
}


void M_Keydown(int key)
{
	switch (m_state)
	{

	case m_mload:
		M_MLoad_Key(key);
		return;

	case m_msave:
		M_MSave_Key(key);
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
