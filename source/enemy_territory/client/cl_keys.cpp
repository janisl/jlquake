/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

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
			if (chat_team)
			{
				String::Sprintf(buffer, sizeof(buffer), "say_team \"%s\"\n", chatField.buffer);
			}
			else if (chat_buddy)
			{
				String::Sprintf(buffer, sizeof(buffer), "say_buddy \"%s\"\n", chatField.buffer);
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
CL_KeyEvent

Called by the system for both key up and key down events
===================
*/
//static consoleCount = 0;
// fretn
qboolean consoleButtonWasPressed = false;

void CL_KeyEvent(int key, qboolean down, unsigned time)
{
	char* kb;
	char cmd[1024];
	qboolean bypassMenu = false;		// NERVE - SMF
	qboolean onlybinds = false;

	if (!key)
	{
		return;
	}

	switch (key)
	{
	case K_KP_PGUP:
	case K_KP_EQUALS:
	case K_KP_5:
	case K_KP_LEFTARROW:
	case K_KP_UPARROW:
	case K_KP_RIGHTARROW:
	case K_KP_DOWNARROW:
	case K_KP_END:
	case K_KP_PGDN:
	case K_KP_INS:
	case K_KP_DEL:
	case K_KP_HOME:
		if (Sys_IsNumLockDown())
		{
			onlybinds = true;
		}
		break;
	}

	// update auto-repeat status and WOLFBUTTON_ANY status
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

		// the console key should never be used as a char
		consoleButtonWasPressed = true;
		return;
	}
	else
	{
		consoleButtonWasPressed = false;
	}

//----(SA)	added
	if (cl.wa_cameraMode)
	{
		if (!(in_keyCatchers & (KEYCATCH_UI | KEYCATCH_CONSOLE)))			// let menu/console handle keys if necessary

		{	// in cutscenes we need to handle keys specially (pausing not allowed in camera mode)
			if ((key == K_ESCAPE ||
				 key == K_SPACE ||
				 key == K_ENTER) && down)
			{
				CL_AddReliableCommand("cameraInterrupt");
				return;
			}

			// eat all keys
			if (down)
			{
				return;
			}
		}

		if ((in_keyCatchers & KEYCATCH_CONSOLE) && key == K_ESCAPE)
		{
			// don't allow menu starting when console is down and camera running
			return;
		}
	}
	//----(SA)	end

	// most keys during demo playback will bring up the menu, but non-ascii

	// keys can still be used for bound actions
	if (down && (key < 128 || key == K_MOUSE1) &&
		(clc.demoplaying || cls.state == CA_CINEMATIC) && !in_keyCatchers)
	{

		Cvar_Set("nextdemo","");
		key = K_ESCAPE;
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
				// Arnout: on request
				if (in_keyCatchers & KEYCATCH_CONSOLE)		// get rid of the console
				{
					Con_ToggleConsole_f();
				}
				else
				{
					UI_SetInGameMenu();
				}
			}
			else
			{
				CL_Disconnect_f();
				S_StopAllSounds();
				UI_SetMainMenu();
			}
			return;
		}

		UI_KeyDownEvent(key);
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
		if (kb && kb[0] == '+')
		{
			// button commands add keynum and time as parms so that multiple
			// sources can be discriminated and subframe corrected
			String::Sprintf(cmd, sizeof(cmd), "-%s %i %i\n", kb + 1, key, time);
			Cbuf_AddText(cmd);
		}

		if (in_keyCatchers & KEYCATCH_UI)
		{
			if (!onlybinds || UIET_WantsBindKeys())
			{
				UI_KeyEvent(key, down);
			}
		}
		else if (in_keyCatchers & KEYCATCH_CGAME)
		{
			if (!onlybinds || CLET_WantsBindKeys())
			{
				CLT3_KeyEvent(key, down);
			}
		}

		return;
	}

	// NERVE - SMF - if we just want to pass it along to game
	if (cl_bypassMouseInput && cl_bypassMouseInput->integer)
	{
		if ((key == K_MOUSE1 || key == K_MOUSE2 || key == K_MOUSE3 || key == K_MOUSE4 || key == K_MOUSE5))
		{
			if (cl_bypassMouseInput->integer == 1)
			{
				bypassMenu = true;
			}
		}
		else if ((in_keyCatchers & KEYCATCH_UI && !UIT3_CheckKeyExec(key)) || (in_keyCatchers & KEYCATCH_CGAME && !CLET_CGameCheckKeyExec(key)))
		{
			bypassMenu = true;
		}
	}

	// distribute the key down event to the apropriate handler
	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		if (!onlybinds)
		{
			Con_KeyEvent(key);
		}
	}
	else if (in_keyCatchers & KEYCATCH_UI && !bypassMenu)
	{
		if (!onlybinds || UIET_WantsBindKeys())
		{
			UI_KeyDownEvent(key);
		}
	}
	else if (in_keyCatchers & KEYCATCH_CGAME && !bypassMenu)
	{
		if (!onlybinds || CLET_WantsBindKeys())
		{
			CLT3_KeyEvent(key, down);
		}
	}
	else if (in_keyCatchers & KEYCATCH_MESSAGE)
	{
		if (!onlybinds)
		{
			Message_Key(key);
		}
	}
	else if (cls.state == CA_DISCONNECTED)
	{

		if (!onlybinds)
		{
			Con_KeyEvent(key);
		}

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
					Key_KeynumToString(key, false));
			}
		}
		else if (kb[0] == '+')
		{
			// button commands add keynum and time as parms so that multiple
			// sources can be discriminated and subframe corrected
			String::Sprintf(cmd, sizeof(cmd), "%s %i %i\n", kb, key, time);
			Cbuf_AddText(cmd);
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
	// ydnar: added uk equivalent of shift+`
	// the RIGHT way to do this would be to have certain keys disable the equivalent SE_CHAR event

	// fretn - this should be fixed in Com_EventLoop
	// but I can't be arsed to leave this as is

	if (key == (unsigned char)'`' || key == (unsigned char)'~' || key == (unsigned char)'�')
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
		UI_CharEvent(key);
	}
	else if (in_keyCatchers & KEYCATCH_CGAME)
	{
		CLT3_KeyEvent(key | K_CHAR_FLAG, true);
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
