/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "client.h"

/*

key up events are sent even if in console mode

*/

qboolean UI_checkKeyExec(int key);			// NERVE - SMF

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
			// NERVE - SMF
			else if (chat_limbo)
			{

				String::Sprintf(buffer, sizeof(buffer), "say_limbo \"%s\"\n", chatField.buffer);
			}
			// -NERVE - SMF
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
void CL_KeyEvent(int key, qboolean down, unsigned time)
{
	char* kb;
	char cmd[1024];
	qboolean bypassMenu = false;		// NERVE - SMF

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

	// are we waiting to clear stats and move to briefing screen
	if (down && cl_waitForFire && cl_waitForFire->integer)			//DAJ BUG in dedicated cl_waitForFire don't exist
	{
		if (in_keyCatchers & KEYCATCH_CONSOLE)		// get rid of the consol
		{
			Con_ToggleConsole_f();
		}
		// clear all input controls
		CL_ClearKeys();
		// allow only attack command input
		kb = keys[key].binding;
		if (!String::ICmp(kb, "+attack"))
		{
			// clear the stats out, ignore the keypress
			Cvar_Set("g_missionStats", "xx");			// just remove the stats, but still wait until we're done loading, and player has hit fire to begin playing
			Cvar_Set("cl_waitForFire", "0");
		}
		return;		// no buttons while waiting
	}

	// are we waiting to begin the level
	if (down && cl_missionStats && cl_missionStats->string[0] && cl_missionStats->string[1])		//DAJ BUG in dedicated cl_missionStats don't exist
	{
		if (in_keyCatchers & KEYCATCH_CONSOLE)		// get rid of the consol
		{
			Con_ToggleConsole_f();
		}
		// clear all input controls
		CL_ClearKeys();
		// allow only attack command input
		kb = keys[key].binding;
		if (!String::ICmp(kb, "+attack"))
		{
			// clear the stats out, ignore the keypress
			Cvar_Set("com_expectedhunkusage", "-1");
			Cvar_Set("g_missionStats", "0");
		}
		return;		// no buttons while waiting
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
			VM_Call(cgvm, WMCG_EVENT_HANDLING, WMCGAME_EVENT_NONE);
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
		if (kb && kb[0] == '+')
		{
			// button commands add keynum and time as parms so that multiple
			// sources can be discriminated and subframe corrected
			String::Sprintf(cmd, sizeof(cmd), "-%s %i %i\n", kb + 1, key, time);
			Cbuf_AddText(cmd);
		}

		if (in_keyCatchers & KEYCATCH_UI && uivm)
		{
			VM_Call(uivm, UI_KEY_EVENT, key, down);
		}
		else if (in_keyCatchers & KEYCATCH_CGAME && cgvm)
		{
			VM_Call(cgvm, WMCG_KEY_EVENT, key, down);
		}

		return;
	}

	// NERVE - SMF - if we just want to pass it along to game
	if (cl_bypassMouseInput && cl_bypassMouseInput->integer)		//DAJ BUG in dedicated cl_missionStats don't exist
	{
		if ((key == K_MOUSE1 || key == K_MOUSE2 || key == K_MOUSE3))
		{
			if (cl_bypassMouseInput->integer == 1)
			{
				bypassMenu = true;
			}
		}
		else if (!UI_checkKeyExec(key))
		{
			bypassMenu = true;
		}
	}

	// distribute the key down event to the apropriate handler
	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		Con_KeyEvent(key);
	}
	else if (in_keyCatchers & KEYCATCH_UI && !bypassMenu)
	{
		kb = keys[key].binding;

		if (VM_Call(uivm, UI_GET_ACTIVE_MENU) == UIMENU_CLIPBOARD)
		{
			// any key gets out of clipboard
			key = K_ESCAPE;
		}
		else
		{

			// when in the notebook, check for the key bound to "notebook" and allow that as an escape key

			if (kb)
			{
				if (!String::ICmp("notebook", kb))
				{
					if (VM_Call(uivm, UI_GET_ACTIVE_MENU) == UIMENU_NOTEBOOK)
					{
						key = K_ESCAPE;
					}
				}

				if (!String::ICmp("help", kb))
				{
					if (VM_Call(uivm, UI_GET_ACTIVE_MENU) == UIMENU_HELP)
					{
						key = K_ESCAPE;
					}
				}
			}
		}

		VM_Call(uivm, UI_KEY_EVENT, key, down);
	}
	else if (in_keyCatchers & KEYCATCH_CGAME)
	{
		if (cgvm)
		{
			VM_Call(cgvm, WMCG_KEY_EVENT, key, down);
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
		VM_Call(uivm, UI_KEY_EVENT, key | K_CHAR_FLAG, true);
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
