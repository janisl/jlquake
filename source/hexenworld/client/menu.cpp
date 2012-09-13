/*
 * $Header: /HexenWorld/Client/menu.c 28    5/11/98 2:35p Mgummelt $
 */

#include "quakedef.h"

extern Cvar* r_gamma;

void M_Keys_Draw(void);

void M_Keys_Key(int key);
void M_Quit_Key(int key);

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

//=============================================================================
/* Menu Subsystem */


void M_Init(void)
{
	Cmd_AddCommand("togglemenu", M_ToggleMenu_f);

	MQH_Init();
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

	case m_keys:
		M_Keys_Draw();
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

	case m_keys:
		M_Keys_Key(key);
		return;

	case m_quit:
		M_Quit_Key(key);
		return;
	}
	MQH_Keydown(key);
}
