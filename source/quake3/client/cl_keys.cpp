/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#include "client.h"

/*

key up events are sent even if in console mode

*/

int chat_playerNum;

//============================================================================


/*
================
Message_Key

In game talk message
================
*/
void Message_Key(int key)
{

	char buffer[MAX_STRING_CHARS];


	if (key == K_ESCAPE)
	{
		in_keyCatchers &= ~KEYCATCH_MESSAGE;
		Field_Clear(&chatField);
		return;
	}

	if (key == K_ENTER || key == K_KP_ENTER)
	{
		if (chatField.buffer[0] && cls.state == CA_ACTIVE)
		{
			if (chat_playerNum != -1)
			{

				String::Sprintf(buffer, sizeof(buffer), "tell %i \"%s\"\n", chat_playerNum, chatField.buffer);
			}

			else if (chat_team)
			{

				String::Sprintf(buffer, sizeof(buffer), "say_team \"%s\"\n", chatField.buffer);
			}
			else
			{
				String::Sprintf(buffer, sizeof(buffer), "say \"%s\"\n", chatField.buffer);
			}



			CL_AddReliableCommand(buffer);
		}
		in_keyCatchers &= ~KEYCATCH_MESSAGE;
		Field_Clear(&chatField);
		return;
	}

	Field_KeyDownEvent(&chatField, key);
}

//============================================================================


qboolean Key_GetOverstrikeMode(void)
{
	return key_overstrikeMode;
}


void Key_SetOverstrikeMode(qboolean state)
{
	key_overstrikeMode = state;
}


/*
===================
Key_IsDown
===================
*/
qboolean Key_IsDown(int keynum)
{
	if (keynum == -1)
	{
		return qfalse;
	}

	return keys[keynum].down;
}


/*
===================
Key_StringToKeynum

Returns a key number to be used to index keys[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.

0x11 will be interpreted as raw hex, which will allow new controlers

to be configured even if they don't have defined names.
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

	// check for hex code
	if (str[0] == '0' && str[1] == 'x' && String::Length(str) == 4)
	{
		int n1, n2;

		n1 = str[2];
		if (n1 >= '0' && n1 <= '9')
		{
			n1 -= '0';
		}
		else if (n1 >= 'a' && n1 <= 'f')
		{
			n1 = n1 - 'a' + 10;
		}
		else
		{
			n1 = 0;
		}

		n2 = str[3];
		if (n2 >= '0' && n2 <= '9')
		{
			n2 -= '0';
		}
		else if (n2 >= 'a' && n2 <= 'f')
		{
			n2 = n2 - 'a' + 10;
		}
		else
		{
			n2 = 0;
		}

		return n1 * 16 + n2;
	}

	// scan for a text match
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

Returns a string (either a single ascii char, a K_* name, or a 0x11 hex string) for the
given keynum.
===================
*/
const char* Key_KeynumToString(int keynum)
{
	keyname_t* kn;
	static char tinystr[5];
	int i, j;

	if (keynum == -1)
	{
		return "<KEY NOT FOUND>";
	}

	if (keynum < 0 || keynum > 255)
	{
		return "<OUT OF RANGE>";
	}

	// check for printable ascii (don't use quote)
	if (keynum > 32 && keynum < 127 && keynum != '"' && keynum != ';')
	{
		tinystr[0] = keynum;
		tinystr[1] = 0;
		return tinystr;
	}

	// check for a key string
	for (kn = keynames; kn->name; kn++)
	{
		if (keynum == kn->keynum)
		{
			return kn->name;
		}
	}

	// make a hex string
	i = keynum >> 4;
	j = keynum & 15;

	tinystr[0] = '0';
	tinystr[1] = 'x';
	tinystr[2] = i > 9 ? i - 10 + 'a' : i + '0';
	tinystr[3] = j > 9 ? j - 10 + 'a' : j + '0';
	tinystr[4] = 0;

	return tinystr;
}


/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding(int keynum, const char* binding)
{
	if (keynum == -1)
	{
		return;
	}

	// free old bindings
	if (keys[keynum].binding)
	{
		Z_Free(keys[keynum].binding);
	}

	// allocate memory for new binding
	keys[keynum].binding = CopyString(binding);

	// consider this like modifying an archived cvar, so the
	// file write will be triggered at the next oportunity
	cvar_modifiedFlags |= CVAR_ARCHIVE;
}


/*
===================
Key_GetBinding
===================
*/
const char* Key_GetBinding(int keynum)
{
	if (keynum == -1)
	{
		return "";
	}

	return keys[keynum].binding;
}

/*
===================
Key_GetKey
===================
*/

int Key_GetKey(const char* binding)
{
	int i;

	if (binding)
	{
		for (i = 0; i < 256; i++)
		{
			if (keys[i].binding && String::ICmp(binding, keys[i].binding) == 0)
			{
				return i;
			}
		}
	}
	return -1;
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

/*
===================
Key_Unbindall_f
===================
*/
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
		if (keys[b].binding)
		{
			Com_Printf("\"%s\" = \"%s\"\n", Cmd_Argv(1), keys[b].binding);
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
	int i;

	FS_Printf(f, "unbindall\n");

	for (i = 0; i < 256; i++)
	{
		if (keys[i].binding && keys[i].binding[0])
		{
			FS_Printf(f, "bind %s \"%s\"\n", Key_KeynumToString(i), keys[i].binding);

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
	{
		if (keys[i].binding && keys[i].binding[0])
		{
			Com_Printf("%s \"%s\"\n", Key_KeynumToString(i), keys[i].binding);
		}
	}
}

/*
===================
CL_InitKeyCommands
===================
*/
void CL_InitKeyCommands(void)
{
	// register our functions
	Cmd_AddCommand("bind",Key_Bind_f);
	Cmd_AddCommand("unbind",Key_Unbind_f);
	Cmd_AddCommand("unbindall",Key_Unbindall_f);
	Cmd_AddCommand("bindlist",Key_Bindlist_f);
}

/*
===================
CL_AddKeyUpCommands
===================
*/
void CL_AddKeyUpCommands(int key, char* kb)
{
	int i;
	char button[1024], * buttonPtr;
	char cmd[1024];
	qboolean keyevent;

	if (!kb)
	{
		return;
	}
	keyevent = qfalse;
	buttonPtr = button;
	for (i = 0;; i++)
	{
		if (kb[i] == ';' || !kb[i])
		{
			*buttonPtr = '\0';
			if (button[0] == '+')
			{
				// button commands add keynum and time as parms so that multiple
				// sources can be discriminated and subframe corrected
				String::Sprintf(cmd, sizeof(cmd), "-%s %i %i\n", button + 1, key, time);
				Cbuf_AddText(cmd);
				keyevent = qtrue;
			}
			else
			{
				if (keyevent)
				{
					// down-only command
					Cbuf_AddText(button);
					Cbuf_AddText("\n");
				}
			}
			buttonPtr = button;
			while ((kb[i] <= ' ' || kb[i] == ';') && kb[i] != 0)
			{
				i++;
			}
		}
		*buttonPtr++ = kb[i];
		if (!kb[i])
		{
			break;
		}
	}
}

/*
===================
CL_KeyEvent

Called by the system for both key up and key down events
===================
*/
void CL_KeyEvent(int key, qboolean down, unsigned time)
{
	char* kb;
	char cmd[1024];

	// update auto-repeat status and Q3BUTTON_ANY status
	keys[key].down = down;

	if (down)
	{
		keys[key].repeats++;
		if (keys[key].repeats == 1)
		{
			anykeydown++;
		}
	}
	else
	{
		keys[key].repeats = 0;
		anykeydown--;
		if (anykeydown < 0)
		{
			anykeydown = 0;
		}
	}

#ifdef __linux__
	if (key == K_ENTER)
	{
		if (down)
		{
			if (keys[K_ALT].down)
			{
				Key_ClearStates();
				if (Cvar_VariableValue("r_fullscreen") == 0)
				{
					Com_Printf("Switching to fullscreen rendering\n");
					Cvar_Set("r_fullscreen", "1");
				}
				else
				{
					Com_Printf("Switching to windowed rendering\n");
					Cvar_Set("r_fullscreen", "0");
				}
				Cbuf_ExecuteText(EXEC_APPEND, "vid_restart\n");
				return;
			}
		}
	}
#endif

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


	// keys can still be used for bound actions
	if (down && (key < 128 || key == K_MOUSE1) && (clc.demoplaying || cls.state == CA_CINEMATIC) && !in_keyCatchers)
	{

		if (Cvar_VariableValue("com_cameraMode") == 0)
		{
			Cvar_Set("nextdemo","");
			key = K_ESCAPE;
		}
	}


	// escape is always handled special
	if (key == K_ESCAPE && down)
	{
		if (in_keyCatchers & KEYCATCH_MESSAGE)
		{
			// clear message mode
			Message_Key(key);
			return;
		}

		// escape always gets out of CGAME stuff
		if (in_keyCatchers & KEYCATCH_CGAME)
		{
			in_keyCatchers &= ~KEYCATCH_CGAME;
			VM_Call(cgvm, CG_EVENT_HANDLING, Q3CGAME_EVENT_NONE);
			return;
		}

		if (!(in_keyCatchers & KEYCATCH_UI))
		{
			if (cls.state == CA_ACTIVE && !clc.demoplaying)
			{
				VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_INGAME);
			}
			else
			{
				CL_Disconnect_f();
				S_StopAllSounds();
				VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN);
			}
			return;
		}

		VM_Call(uivm, UI_KEY_EVENT, key, down);
		return;
	}

	//
	// key up events only perform actions if the game key binding is
	// a button command (leading + sign).  These will be processed even in
	// console mode and menu mode, to keep the character from continuing
	// an action started before a mode switch.
	//
	if (!down)
	{
		kb = keys[key].binding;

		CL_AddKeyUpCommands(key, kb);

		if (in_keyCatchers & KEYCATCH_UI && uivm)
		{
			VM_Call(uivm, UI_KEY_EVENT, key, down);
		}
		else if (in_keyCatchers & KEYCATCH_CGAME && cgvm)
		{
			VM_Call(cgvm, CG_KEY_EVENT, key, down);
		}

		return;
	}


	// distribute the key down event to the apropriate handler
	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		Con_KeyEvent(key);
	}
	else if (in_keyCatchers & KEYCATCH_UI)
	{
		if (uivm)
		{
			VM_Call(uivm, UI_KEY_EVENT, key, down);
		}
	}
	else if (in_keyCatchers & KEYCATCH_CGAME)
	{
		if (cgvm)
		{
			VM_Call(cgvm, CG_KEY_EVENT, key, down);
		}
	}
	else if (in_keyCatchers & KEYCATCH_MESSAGE)
	{
		Message_Key(key);
	}
	else if (cls.state == CA_DISCONNECTED)
	{
		Con_KeyEvent(key);
	}
	else
	{
		// send the bound action
		kb = keys[key].binding;
		if (!kb)
		{
			if (key >= 200)
			{
				Com_Printf("%s is unbound, use controls menu to set.\n",
					Key_KeynumToString(key));
			}
		}
		else if (kb[0] == '+')
		{
			int i;
			char button[1024], * buttonPtr;
			buttonPtr = button;
			for (i = 0;; i++)
			{
				if (kb[i] == ';' || !kb[i])
				{
					*buttonPtr = '\0';
					if (button[0] == '+')
					{
						// button commands add keynum and time as parms so that multiple
						// sources can be discriminated and subframe corrected
						String::Sprintf(cmd, sizeof(cmd), "%s %i %i\n", button, key, time);
						Cbuf_AddText(cmd);
					}
					else
					{
						// down-only command
						Cbuf_AddText(button);
						Cbuf_AddText("\n");
					}
					buttonPtr = button;
					while ((kb[i] <= ' ' || kb[i] == ';') && kb[i] != 0)
					{
						i++;
					}
				}
				*buttonPtr++ = kb[i];
				if (!kb[i])
				{
					break;
				}
			}
		}
		else
		{
			// down-only command
			Cbuf_AddText(kb);
			Cbuf_AddText("\n");
		}
	}
}


/*
===================
CL_CharEvent

Normal keyboard characters, already shifted / capslocked / etc
===================
*/
void CL_CharEvent(int key)
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
		VM_Call(uivm, UI_KEY_EVENT, key | K_CHAR_FLAG, qtrue);
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

	anykeydown = qfalse;

	for (i = 0; i < MAX_KEYS; i++)
	{
		if (keys[i].down)
		{
			CL_KeyEvent(i, qfalse, 0);

		}
		keys[i].down = 0;
		keys[i].repeats = 0;
	}
}
