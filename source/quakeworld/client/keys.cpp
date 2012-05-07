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
#ifdef _WINDOWS
#include <windows.h>
#endif
/*

key up events are sent even if in console mode

*/


int shift_down = false;
int key_lastpress;

int key_count;				// incremented every key event

qboolean consolekeys[256];		// if true, can't be rebound while in console
qboolean menubound[256];	// if true, can't be rebound while in menu
int keyshift[256];			// key to map to if shift held down in console

/*
==============================================================================

            LINE TYPING INTO THE CONSOLE

==============================================================================
*/

qboolean CheckForCommand(void)
{
	char command[128];
	const char* cmd, * s;
	int i;

	s = g_consoleField.buffer;

	for (i = 0; i < 127; i++)
		if (s[i] <= ' ')
		{
			break;
		}
		else
		{
			command[i] = s[i];
		}
	command[i] = 0;

	cmd = Cmd_CompleteCommand(command);
	if (!cmd || String::Cmp(cmd, command))
	{
		cmd = Cvar_CompleteVariable(command);
	}
	if (!cmd  || String::Cmp(cmd, command))
	{
		return false;		// just a chat message
	}
	return true;
}

/*
====================
Key_Console

Interactive line editing and console scrollback
====================
*/
void Key_Console(int key)
{
	if (key == K_ENTER)
	{
		con.acLength = 0;

		// backslash text are commands, else chat
		if (g_consoleField.buffer[0] == '\\' || g_consoleField.buffer[0] == '/')
		{
			Cbuf_AddText(g_consoleField.buffer + 1);	// skip the >
		}
		else if (CheckForCommand())
		{
			Cbuf_AddText(g_consoleField.buffer);	// valid command
		}
		else
		{	// convert to a chat message
			if (cls.state == CA_CONNECTED || cls.state == CA_LOADING || cls.state == CA_ACTIVE)
			{
				Cbuf_AddText("say ");
			}
			Cbuf_AddText(g_consoleField.buffer);
		}

		Cbuf_AddText("\n");
		Con_Printf("]%s\n",g_consoleField.buffer);
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

	Console_KeyCommon(key);
}

//============================================================================

void Key_Message(int key)
{

	if (key == K_ENTER)
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
	Field_KeyDownEvent(&chatField, key);
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
	if (keys[keynum].binding)
	{
		Z_Free(keys[keynum].binding);
		keys[keynum].binding = NULL;
	}

// allocate memory for new binding
	l = String::Length(binding);
	newb = (char*)Z_Malloc(l + 1);
	String::Cpy(newb, binding);
	newb[l] = 0;
	keys[keynum].binding = newb;
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
		Con_Printf("unbind <key> : remove commands from a key\n");
		return;
	}

	b = Key_StringToKeynum(Cmd_Argv(1));
	if (b == -1)
	{
		Con_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	Key_SetBinding(b, "");
}

void Key_Unbindall_f(void)
{
	int i;

	for (i = 0; i < 256; i++)
		if (keys[i].binding)
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

	if (c != 2 && c != 3)
	{
		Con_Printf("bind <key> [command] : attach a command to a key\n");
		return;
	}
	b = Key_StringToKeynum(Cmd_Argv(1));
	if (b == -1)
	{
		Con_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	if (c == 2)
	{
		if (keys[b].binding)
		{
			Con_Printf("\"%s\" = \"%s\"\n", Cmd_Argv(1), keys[b].binding);
		}
		else
		{
			Con_Printf("\"%s\" is not bound\n", Cmd_Argv(1));
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
	int i;

	for (i = 0; i < 256; i++)
		if (keys[i].binding)
		{
			FS_Printf(f, "bind %s \"%s\"\n", Key_KeynumToString(i), keys[i].binding);
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
	consolekeys[K_TAB] = true;
	consolekeys[K_LEFTARROW] = true;
	consolekeys[K_RIGHTARROW] = true;
	consolekeys[K_UPARROW] = true;
	consolekeys[K_DOWNARROW] = true;
	consolekeys[K_BACKSPACE] = true;
	consolekeys[K_HOME] = true;
	consolekeys[K_END] = true;
	consolekeys[K_PGUP] = true;
	consolekeys[K_PGDN] = true;
	consolekeys[K_INS] = true;
	consolekeys[K_KP_INS] = true;
	consolekeys[K_DEL] = true;
	consolekeys[K_KP_DEL] = true;
	consolekeys[K_SHIFT] = true;
	consolekeys[K_MWHEELUP] = true;
	consolekeys[K_MWHEELDOWN] = true;
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

//	Con_Printf ("%i : %i\n", key, down); //@@@

	keys[key].down = down;

	if (!down)
	{
		keys[key].repeats = 0;
	}

	key_lastpress = key;
	key_count++;
	if (key_count <= 0)
	{
		return;		// just catching keys for Con_NotifyBox
	}

// update auto-repeat status
	if (down)
	{
		keys[key].repeats++;
		if (key != K_BACKSPACE &&
			key != K_PAUSE &&
			key != K_PGUP &&
			key != K_PGDN &&
			keys[key].repeats > 1)
		{
			return;	// ignore most autorepeats

		}
		if (key >= 200 && !keys[key].binding)
		{
			Con_Printf("%s is unbound, hit F4 to set.\n", Key_KeynumToString(key));
		}
	}

	if (key == K_SHIFT)
	{
		shift_down = down;
	}

//
// handle escape specialy, so the user can never unbind it
//
	if (key == K_ESCAPE)
	{
		if (!down)
		{
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
			M_ToggleMenu_f();
		}
		return;
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
		kb = keys[key].binding;
		if (kb && kb[0] == '+')
		{
			sprintf(cmd, "-%s %i %d\n", kb + 1, key, time);
			Cbuf_AddText(cmd);
		}
		if (keyshift[key] != key)
		{
			kb = keys[keyshift[key]].binding;
			if (kb && kb[0] == '+')
			{
				sprintf(cmd, "-%s %i %d\n", kb + 1, key, time);
				Cbuf_AddText(cmd);
			}
		}
		return;
	}

//
// during demo playback, most keys bring up the main menu
//
	if (clc.demoplaying && down && consolekeys[key] && in_keyCatchers == 0)
	{
		M_ToggleMenu_f();
		return;
	}

//
// if not a consolekey, send to the interpreter no matter what mode is
//
	if (((in_keyCatchers & KEYCATCH_UI) && menubound[key]) ||
		((in_keyCatchers & KEYCATCH_CONSOLE) && !consolekeys[key]) ||
		(in_keyCatchers == 0 && (cls.state == CA_ACTIVE || !consolekeys[key])))
	{
		kb = keys[key].binding;
		if (kb)
		{
			if (kb[0] == '+')
			{	// button commands add keynum as a parm
				sprintf(cmd, "%s %i %d\n", kb, key, time);
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
		Field_CharEvent(&g_consoleField, key);
	}
	else if (in_keyCatchers & KEYCATCH_UI)
	{
		M_CharEvent(key);
	}
	else if (in_keyCatchers & KEYCATCH_MESSAGE)
	{
		Field_CharEvent(&chatField, key);
	}
	else if (cls.state == CA_DISCONNECTED)
	{
		Field_CharEvent(&g_consoleField, key);
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

	for (i = 0; i < 256; i++)
	{
		keys[i].down = false;
		keys[i].repeats = false;
	}
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
