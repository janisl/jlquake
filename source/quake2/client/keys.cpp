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
#include "client.h"

/*

key up events are sent even if in console mode

*/


int shift_down = false;

int key_waiting;
char* keybindings[256];
qboolean consolekeys[256];		// if true, can't be rebound while in console
qboolean menubound[256];	// if true, can't be rebound while in menu
int keyshift[256];			// key to map to if shift held down in console
int key_repeats[256];		// if > 1, it is autorepeating
qboolean keydown[256];

/*
==============================================================================

            LINE TYPING INTO THE CONSOLE

==============================================================================
*/

void CompleteCommand(void)
{
	const char* cmd;
	char* s;

	s = g_consoleField.buffer;
	if (*s == '\\' || *s == '/')
	{
		s++;
	}

	cmd = Cmd_CompleteCommand(s);
	if (!cmd)
	{
		cmd = Cvar_CompleteVariable(s);
	}
	if (cmd)
	{
		g_consoleField.buffer[0] = '/';
		String::Cpy(g_consoleField.buffer + 1, cmd);
		int key_linepos = String::Length(cmd) + 1;
		g_consoleField.buffer[key_linepos] = ' ';
		key_linepos++;
		g_consoleField.buffer[key_linepos] = 0;
		g_consoleField.cursor = String::Length(g_consoleField.buffer);
		return;
	}
}

/*
====================
Key_Console

Interactive line editing and console scrollback
====================
*/
void Key_Console(int key)
{

	switch (key)
	{
	case K_KP_SLASH:
		key = '/';
		break;
	case K_KP_MINUS:
		key = '-';
		break;
	case K_KP_PLUS:
		key = '+';
		break;
	case K_KP_HOME:
		key = '7';
		break;
	case K_KP_UPARROW:
		key = '8';
		break;
	case K_KP_PGUP:
		key = '9';
		break;
	case K_KP_LEFTARROW:
		key = '4';
		break;
	case K_KP_5:
		key = '5';
		break;
	case K_KP_RIGHTARROW:
		key = '6';
		break;
	case K_KP_END:
		key = '1';
		break;
	case K_KP_DOWNARROW:
		key = '2';
		break;
	case K_KP_PGDN:
		key = '3';
		break;
	case K_KP_INS:
		key = '0';
		break;
	case K_KP_DEL:
		key = '.';
		break;
	}

	if ((String::ToUpper(key) == 'V' && keydown[K_CTRL]) ||
		(((key == K_INS) || (key == K_KP_INS)) && keydown[K_SHIFT]))
	{
		char* cbd;

		if ((cbd = Sys_GetClipboardData()) != 0)
		{
			int i;

			strtok(cbd, "\n\r\b");

			i = String::Length(cbd);
			if (i + String::Length(g_consoleField.buffer) >= MAX_EDIT_LINE)
			{
				i = MAX_EDIT_LINE - String::Length(g_consoleField.buffer);
			}

			if (i > 0)
			{
				cbd[i] = 0;
				String::Cat(g_consoleField.buffer, sizeof(g_consoleField.buffer), cbd);
				g_consoleField.cursor = String::Length(g_consoleField.buffer);
			}
			delete[] cbd;
		}

		return;
	}

	if (key == 'l')
	{
		if (keydown[K_CTRL])
		{
			Cbuf_AddText("clear\n");
			return;
		}
	}

	if (key == K_ENTER || key == K_KP_ENTER)
	{	// backslash text are commands, else chat
		if (g_consoleField.buffer[0] == '\\' || g_consoleField.buffer[0] == '/')
		{
			Cbuf_AddText(g_consoleField.buffer + 1);	// skip the >
		}
		else
		{
			Cbuf_AddText(g_consoleField.buffer);	// valid command

		}
		Cbuf_AddText("\n");
		Com_Printf("]%s\n",g_consoleField.buffer);
		historyEditLines[nextHistoryLine % COMMAND_HISTORY] = g_consoleField;
		nextHistoryLine++;
		historyLine = nextHistoryLine;
		g_consoleField.buffer[0] = 0;
		g_consoleField.cursor = 0;
		if (cls.state == CA_DISCONNECTED)
		{
			SCR_UpdateScreen();		// force an update, because the command
		}
		// may take some time
		return;
	}

	if (key == K_TAB)
	{	// command completion
		CompleteCommand();
		return;
	}

	if ((key == K_UPARROW) || (key == K_KP_UPARROW) ||
		((key == 'p') && keydown[K_CTRL]))
	{
		do
		{
			historyLine--;
		}
		while (historyLine >= 0 && historyLine > nextHistoryLine - COMMAND_HISTORY && !historyEditLines[historyLine % COMMAND_HISTORY].buffer[0]);
		if (historyLine < 0)
		{
			historyLine = 0;
		}
		g_consoleField = historyEditLines[historyLine % COMMAND_HISTORY];
		g_consoleField.cursor = String::Length(g_consoleField.buffer);
		return;
	}

	if ((key == K_DOWNARROW) || (key == K_KP_DOWNARROW) ||
		((key == 'n') && keydown[K_CTRL]))
	{
		if (historyLine == nextHistoryLine)
		{
			return;
		}
		do
		{
			historyLine++;
		}
		while (historyLine < nextHistoryLine && !historyEditLines[historyLine % COMMAND_HISTORY].buffer[0]);
		if (historyLine == nextHistoryLine)
		{
			g_consoleField.buffer[0] = 0;
		}
		else
		{
			g_consoleField = historyEditLines[historyLine % COMMAND_HISTORY];
		}
		g_consoleField.cursor = String::Length(g_consoleField.buffer);
		return;
	}

	if (key == K_PGUP || key == K_KP_PGUP)
	{
		Con_PageUp();
		return;
	}

	if (key == K_PGDN || key == K_KP_PGDN)
	{
		Con_PageDown();
		return;
	}

	if (key == K_HOME || key == K_KP_HOME)
	{
		Con_Top();
		return;
	}

	if (key == K_END || key == K_KP_END)
	{
		Con_Bottom();
		return;
	}
}

//============================================================================

qboolean chat_team;

void Key_Message(int key)
{

	if (key == K_ENTER || key == K_KP_ENTER)
	{
		if (chat_team)
		{
			Cbuf_AddText("say_team \"");
		}
		else
		{
			Cbuf_AddText("say \"");
		}
		Cbuf_AddText(chatField.buffer);
		Cbuf_AddText("\"\n");

		in_keyCatchers &= ~KEYCATCH_MESSAGE;
		chatField.cursor = 0;
		chatField.buffer[0] = 0;
		return;
	}

	if (key == K_ESCAPE)
	{
		in_keyCatchers &= ~KEYCATCH_MESSAGE;
		chatField.cursor = 0;
		chatField.buffer[0] = 0;
		return;
	}
}

//============================================================================


/*
===================
Key_StringToKeynum

Returns a key number to be used to index keybindings[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.
===================
*/
int Key_StringToKeynum(char* str)
{
	keyname_t* kn;

	if (!str || !str[0])
	{
		return -1;
	}
	if (!str[1])
	{
		return str[0];
	}

	for (kn = keynames; kn->name; kn++)
	{
		if (!String::ICmp(str,kn->name))
		{
			return kn->keynum;
		}
	}
	return -1;
}

/*
===================
Key_KeynumToString

Returns a string (either a single ascii char, or a K_* name) for the
given keynum.
FIXME: handle quote special (general escape sequence?)
===================
*/
const char* Key_KeynumToString(int keynum)
{
	keyname_t* kn;
	static char tinystr[2];

	if (keynum == -1)
	{
		return "<KEY NOT FOUND>";
	}
	if (keynum > 32 && keynum < 127)
	{	// printable ascii
		tinystr[0] = keynum;
		tinystr[1] = 0;
		return tinystr;
	}

	for (kn = keynames; kn->name; kn++)
		if (keynum == kn->keynum)
		{
			return kn->name;
		}

	return "<UNKNOWN KEYNUM>";
}


/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding(int keynum, const char* binding)
{
	char* newb;
	int l;

	if (keynum == -1)
	{
		return;
	}

// free old bindings
	if (keybindings[keynum])
	{
		Z_Free(keybindings[keynum]);
		keybindings[keynum] = NULL;
	}

// allocate memory for new binding
	l = String::Length(binding);
	newb = (char*)Z_Malloc(l + 1);
	String::Cpy(newb, binding);
	newb[l] = 0;
	keybindings[keynum] = newb;
}

/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f(void)
{
	int b;

	if (Cmd_Argc() != 2)
	{
		Com_Printf("unbind <key> : remove commands from a key\n");
		return;
	}

	b = Key_StringToKeynum(Cmd_Argv(1));
	if (b == -1)
	{
		Com_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	Key_SetBinding(b, "");
}

void Key_Unbindall_f(void)
{
	int i;

	for (i = 0; i < 256; i++)
		if (keybindings[i])
		{
			Key_SetBinding(i, "");
		}
}


/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f(void)
{
	int i, c, b;
	char cmd[1024];

	c = Cmd_Argc();

	if (c < 2)
	{
		Com_Printf("bind <key> [command] : attach a command to a key\n");
		return;
	}
	b = Key_StringToKeynum(Cmd_Argv(1));
	if (b == -1)
	{
		Com_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	if (c == 2)
	{
		if (keybindings[b])
		{
			Com_Printf("\"%s\" = \"%s\"\n", Cmd_Argv(1), keybindings[b]);
		}
		else
		{
			Com_Printf("\"%s\" is not bound\n", Cmd_Argv(1));
		}
		return;
	}

// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	for (i = 2; i < c; i++)
	{
		String::Cat(cmd, sizeof(cmd), Cmd_Argv(i));
		if (i != (c - 1))
		{
			String::Cat(cmd, sizeof(cmd), " ");
		}
	}

	Key_SetBinding(b, cmd);
}

/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings(fileHandle_t f)
{
	for (int i = 0; i < 256; i++)
	{
		if (keybindings[i] && keybindings[i][0])
		{
			FS_Printf(f, "bind %s \"%s\"\n", Key_KeynumToString(i), keybindings[i]);
		}
	}
}


/*
============
Key_Bindlist_f

============
*/
void Key_Bindlist_f(void)
{
	int i;

	for (i = 0; i < 256; i++)
		if (keybindings[i] && keybindings[i][0])
		{
			Com_Printf("%s \"%s\"\n", Key_KeynumToString(i), keybindings[i]);
		}
}


/*
===================
Key_Init
===================
*/
void Key_Init(void)
{
	int i;

//
// init ascii characters in console mode
//
	for (i = 32; i < 128; i++)
		consolekeys[i] = true;
	consolekeys[K_ENTER] = true;
	consolekeys[K_KP_ENTER] = true;
	consolekeys[K_TAB] = true;
	consolekeys[K_LEFTARROW] = true;
	consolekeys[K_KP_LEFTARROW] = true;
	consolekeys[K_RIGHTARROW] = true;
	consolekeys[K_KP_RIGHTARROW] = true;
	consolekeys[K_UPARROW] = true;
	consolekeys[K_KP_UPARROW] = true;
	consolekeys[K_DOWNARROW] = true;
	consolekeys[K_KP_DOWNARROW] = true;
	consolekeys[K_BACKSPACE] = true;
	consolekeys[K_HOME] = true;
	consolekeys[K_KP_HOME] = true;
	consolekeys[K_END] = true;
	consolekeys[K_KP_END] = true;
	consolekeys[K_PGUP] = true;
	consolekeys[K_KP_PGUP] = true;
	consolekeys[K_PGDN] = true;
	consolekeys[K_KP_PGDN] = true;
	consolekeys[K_SHIFT] = true;
	consolekeys[K_INS] = true;
	consolekeys[K_KP_INS] = true;
	consolekeys[K_KP_DEL] = true;
	consolekeys[K_KP_SLASH] = true;
	consolekeys[K_KP_PLUS] = true;
	consolekeys[K_KP_MINUS] = true;
	consolekeys[K_KP_5] = true;

	consolekeys['`'] = false;
	consolekeys['~'] = false;

	for (i = 0; i < 256; i++)
		keyshift[i] = i;
	for (i = 'a'; i <= 'z'; i++)
		keyshift[i] = i - 'a' + 'A';
	keyshift['1'] = '!';
	keyshift['2'] = '@';
	keyshift['3'] = '#';
	keyshift['4'] = '$';
	keyshift['5'] = '%';
	keyshift['6'] = '^';
	keyshift['7'] = '&';
	keyshift['8'] = '*';
	keyshift['9'] = '(';
	keyshift['0'] = ')';
	keyshift['-'] = '_';
	keyshift['='] = '+';
	keyshift[','] = '<';
	keyshift['.'] = '>';
	keyshift['/'] = '?';
	keyshift[';'] = ':';
	keyshift['\''] = '"';
	keyshift['['] = '{';
	keyshift[']'] = '}';
	keyshift['`'] = '~';
	keyshift['\\'] = '|';

	menubound[K_ESCAPE] = true;
	for (i = 0; i < 12; i++)
		menubound[K_F1 + i] = true;

//
// register our functions
//
	Cmd_AddCommand("bind",Key_Bind_f);
	Cmd_AddCommand("unbind",Key_Unbind_f);
	Cmd_AddCommand("unbindall",Key_Unbindall_f);
	Cmd_AddCommand("bindlist",Key_Bindlist_f);
}

/*
===================
Key_Event

Called by the system between frames for both key up and key down events
Should NOT be called during an interrupt!
===================
*/
void Key_Event(int key, qboolean down, unsigned time)
{
	char* kb;
	char cmd[1024];

	// hack for modal presses
	if (key_waiting == -1)
	{
		if (down)
		{
			key_waiting = key;
		}
		return;
	}

	// update auto-repeat status
	if (down)
	{
		key_repeats[key]++;
		if (key != K_BACKSPACE &&
			key != K_PAUSE &&
			key != K_PGUP &&
			key != K_KP_PGUP &&
			key != K_PGDN &&
			key != K_KP_PGDN &&
			key_repeats[key] > 1)
		{
			return;	// ignore most autorepeats

		}
		if (key >= 200 && !keybindings[key])
		{
			Com_Printf("%s is unbound, hit F4 to set.\n", Key_KeynumToString(key));
		}
	}
	else
	{
		key_repeats[key] = 0;
	}

	if (key == K_SHIFT)
	{
		shift_down = down;
	}

	// console key is hardcoded, so the user can never unbind it
	if (key == '`' || key == '~')
	{
		if (!down)
		{
			return;
		}
		Con_ToggleConsole_f();
		return;
	}

	// any key during the attract mode will bring up the menu
	if (cl.q2_attractloop && !(in_keyCatchers & KEYCATCH_UI))
	{
		key = K_ESCAPE;
	}

	// menu key is hardcoded, so the user can never unbind it
	if (key == K_ESCAPE)
	{
		if (!down)
		{
			return;
		}

		if (cl.q2_frame.playerstate.stats[Q2STAT_LAYOUTS] && in_keyCatchers == 0)
		{	// put away help computer / inventory
			Cbuf_AddText("cmd putaway\n");
			return;
		}
		if (in_keyCatchers & KEYCATCH_MESSAGE)
		{
			Key_Message(key);
		}
		else if (in_keyCatchers & KEYCATCH_UI)
		{
			M_Keydown(key);
		}
		else
		{
			M_Menu_Main_f();
		}
		return;
	}

	// track if any key is down for Q2BUTTON_ANY
	keydown[key] = down;
	if (down)
	{
		if (key_repeats[key] == 1)
		{
			anykeydown++;
		}
	}
	else
	{
		anykeydown--;
		if (anykeydown < 0)
		{
			anykeydown = 0;
		}
	}

//
// key up events only generate commands if the game key binding is
// a button command (leading + sign).  These will occur even in console mode,
// to keep the character from continuing an action started before a console
// switch.  Button commands include the kenum as a parameter, so multiple
// downs can be matched with ups
//
	if (!down)
	{
		kb = keybindings[key];
		if (kb && kb[0] == '+')
		{
			String::Sprintf(cmd, sizeof(cmd), "-%s %i %i\n", kb + 1, key, time);
			Cbuf_AddText(cmd);
		}
		if (keyshift[key] != key)
		{
			kb = keybindings[keyshift[key]];
			if (kb && kb[0] == '+')
			{
				String::Sprintf(cmd, sizeof(cmd), "-%s %i %i\n", kb + 1, key, time);
				Cbuf_AddText(cmd);
			}
		}
		return;
	}

//
// if not a consolekey, send to the interpreter no matter what mode is
//
	if (((in_keyCatchers & KEYCATCH_UI) && menubound[key]) ||
		((in_keyCatchers & KEYCATCH_CONSOLE) && !consolekeys[key]) ||
		(in_keyCatchers == 0 && (cls.state == CA_ACTIVE || !consolekeys[key])))
	{
		kb = keybindings[key];
		if (kb)
		{
			if (kb[0] == '+')
			{	// button commands add keynum and time as a parm
				String::Sprintf(cmd, sizeof(cmd), "%s %i %i\n", kb, key, time);
				Cbuf_AddText(cmd);
			}
			else
			{
				Cbuf_AddText(kb);
				Cbuf_AddText("\n");
			}
		}
		return;
	}

	if (!down)
	{
		return;		// other systems only care about key down events

	}
	if (shift_down)
	{
		key = keyshift[key];
	}

	if (in_keyCatchers & KEYCATCH_MESSAGE)
	{
		Key_Message(key);
	}
	else if (in_keyCatchers & KEYCATCH_UI)
	{
		M_Keydown(key);
	}
	else
	{
		Key_Console(key);
	}
}

void Key_CharEvent(int key)
{
	// the console key should never be used as a char
	if (key == '`' || key == '~')
	{
		return;
	}

	// distribute the key down event to the apropriate handler
	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		Field_CharEventCommon(&g_consoleField, key);
	}
	else if (in_keyCatchers & KEYCATCH_UI)
	{
	}
	else if (in_keyCatchers & KEYCATCH_MESSAGE)
	{
		Field_CharEventCommon(&chatField, key);
	}
	else if (cls.state == CA_DISCONNECTED)
	{
		Field_CharEventCommon(&g_consoleField, key);
	}
}

/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates(void)
{
	int i;

	anykeydown = false;

	for (i = 0; i < 256; i++)
	{
		if (keydown[i] || key_repeats[i])
		{
			Key_Event(i, false, 0);
		}
		keydown[i] = 0;
		key_repeats[i] = 0;
	}
}


/*
===================
Key_GetKey
===================
*/
int Key_GetKey(void)
{
	key_waiting = -1;

	while (key_waiting == -1)
	{
		Sys_SendKeyEvents();
		IN_ProcessEvents();
	}

	return key_waiting;
}


void IN_ProcessEvents()
{
	for (sysEvent_t ev = Sys_SharedGetEvent(); ev.evType; ev = Sys_SharedGetEvent())
	{
		switch (ev.evType)
		{
		case SE_KEY:
			Key_Event(ev.evValue, ev.evValue2, ev.evTime);
			break;
		case SE_CHAR:
			Key_CharEvent(ev.evValue);
			break;
		case SE_MOUSE:
			CL_MouseEvent(ev.evValue, ev.evValue2);
			break;
		case SE_JOYSTICK_AXIS:
			CL_JoystickEvent(ev.evValue, ev.evValue2, ev.evTime);
			break;
		case SE_CONSOLE:
			Cbuf_AddText((char*)ev.evPtr);
			Cbuf_AddText("\n");
			break;
		}

		// free any block data
		if (ev.evPtr)
		{
			Mem_Free(ev.evPtr);
		}
	}
}
