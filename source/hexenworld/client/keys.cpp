#include "quakedef.h"
#ifdef _WINDOWS
#include <windows.h>
#endif
/*

key up events are sent even if in console mode

*/

qboolean consolekeys[256];		// if true, can't be rebound while in console
qboolean menubound[256];	// if true, can't be rebound while in menu

//============================================================================


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

	menubound[K_ESCAPE] = true;
	for (i = 0; i < 12; i++)
		menubound[K_F1 + i] = true;

	CL_InitKeyCommands();
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

//	common->Printf ("%i : %i\n", key, down); //@@@

	keys[key].down = down;

	if (!down)
	{
		keys[key].repeats = 0;
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
			common->Printf("%s is unbound, hit F4 to set.\n", Key_KeynumToString(key, true));
		}
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
			Con_MessageKeyEvent(key);
		}
		else if (in_keyCatchers & KEYCATCH_UI)
		{
			UI_KeyDownEvent(key);
		}
		else if (in_keyCatchers & KEYCATCH_CONSOLE && (!(GGameType & GAME_HexenWorld) || cls.state == CA_ACTIVE))
		{
			Con_ToggleConsole_f();
		}
		else
		{
			UI_SetMainMenu();
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
		return;
	}

//
// during demo playback, most keys bring up the main menu
//
	if (clc.demoplaying && down && consolekeys[key] && in_keyCatchers == 0)
	{
		UI_SetMainMenu();
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

	if (in_keyCatchers & KEYCATCH_MESSAGE)
	{
		Con_MessageKeyEvent(key);
	}
	else if (in_keyCatchers & KEYCATCH_UI)
	{
		UI_KeyDownEvent(key);
	}
	else
	{
		Con_KeyEvent(key);
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
		Con_CharEvent(key);
	}
	else if (in_keyCatchers & KEYCATCH_UI)
	{
		UI_CharEvent(key);
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

	for (i = 0; i < 256; i++)
	{
		keys[i].down = false;
		keys[i].repeats = 0;
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
