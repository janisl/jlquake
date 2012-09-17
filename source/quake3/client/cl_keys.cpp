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

/*
================
Message_Key

In game talk message
================
*/
void Message_Key(int key)
{

	char buffer[MAX_STRING_CHARS];

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

	Con_MessageKeyEvent(key);
}

//============================================================================

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
	keyevent = false;
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
				keyevent = true;
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
					common->Printf("Switching to fullscreen rendering\n");
					Cvar_Set("r_fullscreen", "1");
				}
				else
				{
					common->Printf("Switching to windowed rendering\n");
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
			CLT3_EventHandling();
			return;
		}

		if (!(in_keyCatchers & KEYCATCH_UI))
		{
			if (cls.state == CA_ACTIVE && !clc.demoplaying)
			{
				UIT3_SetActiveMenu(UIMENU_INGAME);
			}
			else
			{
				CL_Disconnect_f();
				S_StopAllSounds();
				UIT3_SetActiveMenu(UIMENU_MAIN);
			}
			return;
		}

		UIT3_KeyEvent(key, down);
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

		if (in_keyCatchers & KEYCATCH_UI)
		{
			UIT3_KeyEvent(key, down);
		}
		else if (in_keyCatchers & KEYCATCH_CGAME)
		{
			CLT3_KeyEvent(key, down);
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
		UIT3_KeyEvent(key, down);
	}
	else if (in_keyCatchers & KEYCATCH_CGAME)
	{
		CLT3_KeyEvent(key, down);
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
				common->Printf("%s is unbound, use controls menu to set.\n",
					Key_KeynumToString(key, true));
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
		Con_CharEvent(key);
	}
	else if (in_keyCatchers & KEYCATCH_UI)
	{
		UIT3_KeyEvent(key | K_CHAR_FLAG, true);
	}
	else if (in_keyCatchers & KEYCATCH_MESSAGE)
	{
		Con_MessageCharEvent(key);
	}
	else if (cls.state == CA_DISCONNECTED)
	{
		Con_CharEvent(key);
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

	for (i = 0; i < MAX_KEYS; i++)
	{
		if (keys[i].down)
		{
			CL_KeyEvent(i, false, 0);

		}
		keys[i].down = 0;
		keys[i].repeats = 0;
	}
}
