/*
 * $Header: /HexenWorld/Client/menu.c 28    5/11/98 2:35p Mgummelt $
 */

#include "quakedef.h"

extern Cvar* r_gamma;
extern Cvar* crosshair;

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

//=============================================================================
/* Support Routines */

void M_Print2(int cx, int cy, const char* str)
{
	UI_DrawString(cx + ((viddef.width - 320) >> 1), cy + ((viddef.height - 200) >> 1), str, 256);
}

#define PLAYER_PIC_WIDTH 68
#define PLAYER_PIC_HEIGHT 114

byte translationTable[256];
static byte menuplyr_pixels[MAX_PLAYER_CLASS][PLAYER_PIC_WIDTH * PLAYER_PIC_HEIGHT];
image_t* translate_texture[MAX_PLAYER_CLASS];

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
	if ((in_keyCatchers & KEYCATCH_CONSOLE) && cls.state == CA_ACTIVE)
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

	case OPT_VIDEO:
		Cvar_SetValue("cl_sbar", !clqh_sbar->value);
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
			//m_state = m_none;
			//Con_ToggleConsole_f ();
			in_keyCatchers &= ~KEYCATCH_UI;
			m_state = m_none;
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
	{"impulse 13",      "use object"},
	{"invuse",          "use inv item"},
	{"invdrop",         "drop inv item"},
	{"+showinfo",       "full inventory"},
	{"+showdm",         "info / frags"},
	{"toggle_dm",       "toggle frags"},
	{"+shownames",      "player names"},
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
	if (clhw_siege)
	{
		MQH_DrawPic(0, 0, R_CachePic(va("gfx/menu/sghelp%02i.lmp", mqh_help_page + 1)));
	}
	else
	{
		MQH_DrawPic(0, 0, R_CachePic(va("gfx/menu/help%02i.lmp", mqh_help_page + 1)));
	}
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
		if (clhw_siege)
		{
			if (++mqh_help_page >= NUM_SG_HELP_PAGES)
			{
				mqh_help_page = 0;
			}
		}
		else if (++mqh_help_page >= NUM_HELP_PAGES_H2)
		{
			mqh_help_page = 0;
		}
		break;

	case K_DOWNARROW:
	case K_LEFTARROW:
		mqh_entersound = true;
		if (--mqh_help_page < 0)
		{
			if (clhw_siege)
			{
				mqh_help_page = NUM_SG_HELP_PAGES - 1;
			}
			else
			{
				mqh_help_page = NUM_HELP_PAGES_H2 - 1;
			}
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
		CL_Disconnect();
		Sys_Quit();
		break;

	default:
		break;
	}

}

#define VSTR(x) # x
#define VSTR2(x) VSTR(x)

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
			MaxLines = MAX_LINES2_HW;
			LineText = Credit2TextHW;
		}
	}

	y = 12;
	MQH_DrawTextBox(0, 0, 38, 23);
	MQH_PrintWhite(16, y,  "      Hexen2World version " VSTR2(VERSION) "      ");    y += 8;
	MQH_PrintWhite(16, y,  "         by Raven Software          ");  y += 16;

	if (LinePos > 55 && !SoundPlayed && LineText == Credit2TextHW)
	{
		S_StartLocalSound("rj/steve.wav");
		SoundPlayed = true;
	}
	topy = y;
	place = LinePos;
	y -= (LinePos - (int)LinePos) * 8;
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
	for (i = 4; i < 36; i++,x += 8)
	{
		MQH_DrawPic(x, y, p);
	}

	p = R_CachePic("gfx/box_mm2.lmp");
	x = 24;
	y = topy + (QUIT_SIZE_H2 * 8) - 8;
	for (i = 4; i < 36; i++,x += 8)
	{
		MQH_DrawPic(x, y, p);
	}

	y += 8;
	MQH_PrintWhite(16, y,  "          Press y to exit           ");

/*	y = 12;
    MQH_DrawTextBox (0, 0, 38, 23);
    MQH_PrintWhite (16, y,  "        Hexen II version 0.0        ");	y += 8;
    MQH_PrintWhite (16, y,  "         by Raven Software          ");	y += 16;
    MQH_PrintWhite (16, y,  "Programming        Art              ");	y += 8;
    MQH_Print (16, y,       " Ben Gokey          Shane Gurno     ");	y += 8;
    MQH_Print (16, y,       " Rick Johnson       Mike Werckle    ");	y += 8;
    MQH_Print (16, y,       " Bob Love           Mark Morgan     ");	y += 8;
    MQH_Print (16, y,       " Mike Gummelt       Brian Pelletier ");	y += 8;
    MQH_Print (16, y,       "                    Kim Lathrop     ");	y += 8;
    MQH_PrintWhite (16, y,  "Design                              ");
    MQH_Print (16, y,       "                    Les Dorscheid   ");	y += 8;
    MQH_Print (16, y,       " Brian Raffel       Jim Sumwalt     ");	y += 8;
    MQH_Print (16, y,       " Eric Biessman      Brian Shubat    ");	y += 16;
    MQH_PrintWhite (16, y,  "Sound Effects      Intern           ");	y += 8;
    MQH_Print (16, y,       " Kevin Schilder     Josh Weier      ");	y += 16;
    MQH_PrintWhite (16, y,  "          Press y to exit           ");	y += 8;*/

}

//=============================================================================
/* SETUP MENU */

void M_Setup_Draw(void)
{
	image_t* p;
	int i;
	static qboolean wait;

	MH2_ScrollTitle("gfx/menu/title4.lmp");

	MQH_Print(64, 56, "Your name");
	MQH_DrawField(168, 56, &setup_myname, mqh_setup_cursor == 1);

	MQH_Print(64, 72, "Spectator: ");
	if (spectator->value)
	{
		MQH_PrintWhite(64 + 12 * 8, 72, "YES");
	}
	else
	{
		MQH_PrintWhite(64 + 12 * 8, 72, "NO");
	}

	MQH_Print(64, 88, "Current Class: ");

	if (!com_portals)
	{
		if (setup_class == CLASS_DEMON)
		{
			setup_class = 0;
		}
	}
	if (String::ICmp(fs_gamedir, "siege"))
	{
		if (setup_class == CLASS_DWARF)
		{
			setup_class = 0;
		}
	}
	switch (setup_class)
	{
	case 0: MQH_PrintWhite(88, 96, "Random");
		break;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5: MQH_PrintWhite(88, 96, hw_ClassNames[setup_class - 1]);
	case 6: MQH_PrintWhite(88, 96, hw_ClassNames[setup_class - 1]);
		break;
	}

	MQH_Print(64, 112, "First color patch");
	MQH_Print(64, 136, "Second color patch");

	MQH_DrawTextBox(64, 164 - 8, 14, 1);
	MQH_Print(72, 164, "Accept Changes");

	if (setup_class == 0)
	{
		i = (int)(realtime * 10) % 8;

		if ((i == 0 && !wait) || which_class == 0)
		{
			if (!com_portals)
			{	//not succubus
				if (String::ICmp(fs_gamedir, "siege"))
				{
					which_class = (rand() % CLASS_THEIF) + 1;
				}
				else
				{
					which_class = (rand() % CLASS_DEMON) + 1;
					if (which_class == CLASS_DEMON)
					{
						which_class = CLASS_DWARF;
					}
				}
			}
			else
			{
				if (String::ICmp(fs_gamedir, "siege"))
				{
					which_class = (rand() % CLASS_DEMON) + 1;
				}
				else
				{
					which_class = (rand() % class_limit) + 1;
				}
			}
			wait = true;
		}
		else if (i)
		{
			wait = false;
		}
	}
	else
	{
		which_class = setup_class;
	}
	p = R_CachePicWithTransPixels(va("gfx/menu/netp%i.lmp", which_class), menuplyr_pixels[which_class - 1]);
	CL_CalcHexen2SkinTranslation(setup_top, setup_bottom, which_class, translationTable);
	R_CreateOrUpdateTranslatedImage(translate_texture[which_class - 1], va("translate_pic%d", which_class), menuplyr_pixels[which_class - 1], translationTable, PLAYER_PIC_WIDTH, PLAYER_PIC_HEIGHT);
	MQH_DrawPic(220, 72, translate_texture[which_class - 1]);

	MQH_DrawCharacter(56, setup_cursor_table_hw [mqh_setup_cursor], 12 + ((int)(realtime * 4) & 1));
}


void M_Setup_Key(int k)
{
	switch (k)
	{
	case K_ESCAPE:
		MQH_Menu_MultiPlayer_f();
		break;

	case K_UPARROW:
		S_StartLocalSound("raven/menu1.wav");
		mqh_setup_cursor--;
		if (mqh_setup_cursor < 1)
		{
			mqh_setup_cursor = NUM_SETUP_CMDS_HW - 1;
		}
		break;

	case K_DOWNARROW:
		S_StartLocalSound("raven/menu1.wav");
		mqh_setup_cursor++;
		if (mqh_setup_cursor >= NUM_SETUP_CMDS_HW)
		{
			mqh_setup_cursor = 1;
		}
		break;

	case K_LEFTARROW:
		if (mqh_setup_cursor < 2)
		{
			break;
		}
		S_StartLocalSound("raven/menu3.wav");
		if (mqh_setup_cursor == 2)
		{
			if (spectator->value)
			{
				Cvar_Set("spectator","0");
//				spectator.value = 0;
			}
			else
			{
				Cvar_Set("spectator","1");
//				spectator.value = 1;
			}
			cl.qh_spectator = spectator->value;
		}
		if (mqh_setup_cursor == 3)
		{
			setup_class--;
			if (setup_class < 0)
			{
				setup_class = class_limit;
			}
		}
		if (mqh_setup_cursor == 4)
		{
			setup_top = setup_top - 1;
		}
		if (mqh_setup_cursor == 5)
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
		S_StartLocalSound("raven/menu3.wav");
		if (mqh_setup_cursor == 2)
		{
			if (spectator->value)
			{
				Cvar_Set("spectator","0");
//				spectator.value = 0;
			}
			else
			{
				Cvar_Set("spectator","1");
//				spectator.value = 1;
			}
			cl.qh_spectator = spectator->value;

		}
		if (mqh_setup_cursor == 3)
		{
			setup_class++;
			if (setup_class > class_limit)
			{
				setup_class = 0;
			}
		}
		if (mqh_setup_cursor == 4)
		{
			setup_top = setup_top + 1;
		}
		if (mqh_setup_cursor == 5)
		{
			setup_bottom = setup_bottom + 1;
		}
		break;

	case K_ENTER:
		if (mqh_setup_cursor == 0 || mqh_setup_cursor == 1)
		{
			return;
		}

		if (mqh_setup_cursor == 2 || mqh_setup_cursor == 3 || mqh_setup_cursor == 4 || mqh_setup_cursor == 5)
		{
			goto forward;
		}

		if (String::Cmp(clqh_name->string, setup_myname.buffer) != 0)
		{
			Cbuf_AddText(va("name \"%s\"\n", setup_myname.buffer));
		}
		if (setup_top != setup_oldtop || setup_bottom != setup_oldbottom)
		{
			Cbuf_AddText(va("color %i %i\n", setup_top, setup_bottom));
		}
		Cbuf_AddText(va("playerclass %d\n", setup_class));
		mqh_entersound = true;
		MQH_Menu_MultiPlayer_f();
		break;
	}
	if (mqh_setup_cursor == 1)
	{
		Field_KeyDownEvent(&setup_myname, k);
	}

	if (setup_top > 10)
	{
		setup_top = 0;
	}
	if (setup_top < 0)
	{
		setup_top = 10;
	}
	if (setup_bottom > 10)
	{
		setup_bottom = 0;
	}
	if (setup_bottom < 0)
	{
		setup_bottom = 10;
	}
}

void M_Setup_Char(int k)
{
	if (mqh_setup_cursor == 1)
	{
		Field_CharEvent(&setup_myname, k);
	}
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
		S_StartLocalSound("raven/menu2.wav");
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
