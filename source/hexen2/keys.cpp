#include "quakedef.h"

/*

key up events are sent even if in console mode

*/


#define		MAXCMDLINE	256
char	key_lines[32][MAXCMDLINE];
int		key_linepos;
int		shift_down=false;
int		key_lastpress;

int		edit_line=0;
int		history_line=0;

int		key_count;			// incremented every key event

char	*keybindings[256];
qboolean	consolekeys[256];	// if true, can't be rebound while in console
qboolean	menubound[256];	// if true, can't be rebound while in menu
int		keyshift[256];		// key to map to if shift held down in console
int		key_repeats[256];	// if > 1, it is autorepeating
qboolean	keydown[256];

/*
==============================================================================

			LINE TYPING INTO THE CONSOLE

==============================================================================
*/


/*
====================
Key_Console

Interactive line editing and console scrollback
====================
*/
void Key_Console (int key)
{
	const char	*cmd;
	
	if (key == K_ENTER)
	{
		Cbuf_AddText (key_lines[edit_line]+1);	// skip the >
		Cbuf_AddText ("\n");
		Con_Printf ("%s\n",key_lines[edit_line]);
		edit_line = (edit_line + 1) & 31;
		history_line = edit_line;
		key_lines[edit_line][0] = ']';
		key_linepos = 1;
		if (cls.state == ca_disconnected)
			SCR_UpdateScreen ();	// force an update, because the command
									// may take some time
		return;
	}

	if (key == K_TAB)
	{	// command completion
		cmd = Cmd_CompleteCommand (key_lines[edit_line]+1);
		if (!cmd)
			cmd = Cvar_CompleteVariable (key_lines[edit_line]+1);
		if (cmd)
		{
			String::Cpy(key_lines[edit_line]+1, cmd);
			key_linepos = String::Length(cmd)+1;
			key_lines[edit_line][key_linepos] = ' ';
			key_linepos++;
			key_lines[edit_line][key_linepos] = 0;
			return;
		}
	}
	
	if (key == K_BACKSPACE || key == K_LEFTARROW)
	{
		if (key_linepos > 1)
			key_linepos--;
		return;
	}

	if (key == K_UPARROW)
	{
		do
		{
			history_line = (history_line - 1) & 31;
		} while (history_line != edit_line
				&& !key_lines[history_line][1]);
		if (history_line == edit_line)
			history_line = (edit_line+1)&31;
		String::Cpy(key_lines[edit_line], key_lines[history_line]);
		key_linepos = String::Length(key_lines[edit_line]);
		return;
	}

	if (key == K_DOWNARROW)
	{
		if (history_line == edit_line) return;
		do
		{
			history_line = (history_line + 1) & 31;
		}
		while (history_line != edit_line
			&& !key_lines[history_line][1]);
		if (history_line == edit_line)
		{
			key_lines[edit_line][0] = ']';
			key_linepos = 1;
		}
		else
		{
			String::Cpy(key_lines[edit_line], key_lines[history_line]);
			key_linepos = String::Length(key_lines[edit_line]);
		}
		return;
	}

	if (key == K_PGUP)
	{
		con.display -= 2;
		return;
	}

	if (key == K_PGDN)
	{
		con.display += 2;
		if (con.display > con.current)
			con.display = con.current;
		return;
	}

	if (key == K_HOME)
	{
		con.display = con.current - con.totallines + (viddef.height>>3) + 1;
		return;
	}

	if (key == K_END)
	{
		con.display = con.current;
		return;
	}
	
	if (key < 32 || key > 127)
		return;	// non printable
		
	if (key_linepos < MAXCMDLINE-1)
	{
		key_lines[edit_line][key_linepos] = key;
		key_linepos++;
		key_lines[edit_line][key_linepos] = 0;
	}

}

//============================================================================

char chat_buffer[32];
qboolean team_message = false;

void Key_Message (int key)
{
	static int chat_bufferlen = 0;

	if (key == K_ENTER)
	{
		if (team_message)
			Cbuf_AddText ("say_team \"");
		else
			Cbuf_AddText ("say \"");
		Cbuf_AddText(chat_buffer);
		Cbuf_AddText("\"\n");

		in_keyCatchers &= ~KEYCATCH_MESSAGE;
		chat_bufferlen = 0;
		chat_buffer[0] = 0;
		return;
	}

	if (key == K_ESCAPE)
	{
		in_keyCatchers &= ~KEYCATCH_MESSAGE;
		chat_bufferlen = 0;
		chat_buffer[0] = 0;
		return;
	}

	if (key < 32 || key > 127)
		return;	// non printable

	if (key == K_BACKSPACE)
	{
		if (chat_bufferlen)
		{
			chat_bufferlen--;
			chat_buffer[chat_bufferlen] = 0;
		}
		return;
	}

	if (chat_bufferlen == 31)
		return; // all full

	chat_buffer[chat_bufferlen++] = key;
	chat_buffer[chat_bufferlen] = 0;
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
int Key_StringToKeynum (char *str)
{
	keyname_t	*kn;
	
	if (!str || !str[0])
		return -1;
	if (!str[1])
		return str[0];

	for (kn=keynames ; kn->name ; kn++)
	{
		if (!String::ICmp(str,kn->name))
			return kn->keynum;
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
const char *Key_KeynumToString (int keynum)
{
	keyname_t	*kn;	
	static	char	tinystr[2];
	
	if (keynum == -1)
		return "<KEY NOT FOUND>";
	if (keynum > 32 && keynum < 127)
	{	// printable ascii
		tinystr[0] = keynum;
		tinystr[1] = 0;
		return tinystr;
	}
	
	for (kn=keynames ; kn->name ; kn++)
		if (keynum == kn->keynum)
			return kn->name;

	return "<UNKNOWN KEYNUM>";
}


/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding (int keynum, const char *binding)
{
	char	*newb;
	int		l;
			
	if (keynum == -1)
		return;

// free old bindings
	if (keybindings[keynum])
	{
		Z_Free (keybindings[keynum]);
		keybindings[keynum] = NULL;
	}
			
// allocate memory for new binding
	l = String::Length(binding);	
	newb = (char*)Z_Malloc (l+1);
	String::Cpy(newb, binding);
	newb[l] = 0;
	keybindings[keynum] = newb;
}

/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f (void)
{
	int		b;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("unbind <key> : remove commands from a key\n");
		return;
	}
	
	b = Key_StringToKeynum (Cmd_Argv(1));
	if (b==-1)
	{
		Con_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	Key_SetBinding (b, "");
}

void Key_Unbindall_f (void)
{
	int		i;
	
	for (i=0 ; i<256 ; i++)
		if (keybindings[i])
			Key_SetBinding (i, "");
}


/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f (void)
{
	int			i, c, b;
	char		cmd[1024];
	
	c = Cmd_Argc();

	if (c != 2 && c != 3)
	{
		Con_Printf ("bind <key> [command] : attach a command to a key\n");
		return;
	}
	b = Key_StringToKeynum (Cmd_Argv(1));
	if (b==-1)
	{
		Con_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	if (c == 2)
	{
		if (keybindings[b])
			Con_Printf ("\"%s\" = \"%s\"\n", Cmd_Argv(1), keybindings[b] );
		else
			Con_Printf ("\"%s\" is not bound\n", Cmd_Argv(1) );
		return;
	}
	
// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	for (i=2 ; i< c ; i++)
	{
		if (i > 2)
			String::Cat(cmd, sizeof(cmd), " ");
		String::Cat(cmd, sizeof(cmd), Cmd_Argv(i));
	}

	Key_SetBinding (b, cmd);
}

/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings (fileHandle_t f)
{
	int		i;

	for (i=0 ; i<256 ; i++)
		if (keybindings[i])
			if (*keybindings[i])
				FS_Printf (f, "bind \"%s\" \"%s\"\n", Key_KeynumToString(i), keybindings[i]);
}


/*
===================
Key_Init
===================
*/
void Key_Init (void)
{
	int		i;

	for (i=0 ; i<32 ; i++)
	{
		key_lines[i][0] = ']';
		key_lines[i][1] = 0;
	}
	key_linepos = 1;
	
//
// init ascii characters in console mode
//
	for (i=32 ; i<128 ; i++)
		consolekeys[i] = true;
	consolekeys[K_ENTER] = true;
	consolekeys[K_TAB] = true;
	consolekeys[K_LEFTARROW] = true;
	consolekeys[K_RIGHTARROW] = true;
	consolekeys[K_UPARROW] = true;
	consolekeys[K_DOWNARROW] = true;
	consolekeys[K_BACKSPACE] = true;
	consolekeys[K_PGUP] = true;
	consolekeys[K_PGDN] = true;
	consolekeys[K_SHIFT] = true;
	consolekeys['`'] = false;
	consolekeys['~'] = false;

	for (i=0 ; i<256 ; i++)
		keyshift[i] = i;
	for (i='a' ; i<='z' ; i++)
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
	for (i=0 ; i<12 ; i++)
		menubound[K_F1+i] = true;

//
// register our functions
//
	Cmd_AddCommand ("bind",Key_Bind_f);
	Cmd_AddCommand ("unbind",Key_Unbind_f);
	Cmd_AddCommand ("unbindall",Key_Unbindall_f);


}

/*
===================
Key_Event

Called by the system between frames for both key up and key down events
Should NOT be called during an interrupt!
===================
*/
void Key_Event (int key, qboolean down)
{
	char	*kb;
	char	cmd[1024];

	keydown[key] = down;

	if (keydown[K_CTRL] && keydown[K_ALT] && keydown[K_DEL])
		Sys_Error ("ctrl-alt-del pressed");

	if (!down)
		key_repeats[key] = 0;

	key_lastpress = key;
	key_count++;
	if (key_count <= 0)
	{
		return;		// just catching keys for Con_NotifyBox
	}

// update auto-repeat status
	if (down)
	{
		key_repeats[key]++;
		if (key != K_BACKSPACE && key != K_PAUSE && key_repeats[key] > 1)
		{
			return;	// ignore most autorepeats
		}
			
		if (key >= 200 && !keybindings[key])
			Con_Printf ("%s is unbound, hit F4 to set.\n", Key_KeynumToString (key) );
	}

	if (key == K_SHIFT)
		shift_down = down;

//
// handle escape specialy, so the user can never unbind it
//
	if (key == K_ESCAPE)
	{
		if (!down)
			return;
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
		kb = keybindings[key];
		if (kb && kb[0] == '+')
		{
			sprintf (cmd, "-%s %i\n", kb+1, key);
			Cbuf_AddText (cmd);
		}
		if (keyshift[key] != key)
		{
			kb = keybindings[keyshift[key]];
			if (kb && kb[0] == '+')
			{
				sprintf (cmd, "-%s %i\n", kb+1, key);
				Cbuf_AddText (cmd);
			}
		}
		return;
	}

//
// during demo playback, most keys bring up the main menu
//
	if (cls.demoplayback && down && consolekeys[key] && in_keyCatchers == 0)
	{
		M_ToggleMenu_f ();
		return;
	}

	if (cl.intermission == 12 && down)
	{
#ifdef MISSIONPACK
		Cbuf_AddText ("map keep1\n");
#else
		Cbuf_AddText ("map demo1\n");
#endif
	}
		
//
// if not a consolekey, send to the interpreter no matter what mode is
//
	if ( ((in_keyCatchers & KEYCATCH_UI) && menubound[key])
	|| ((in_keyCatchers & KEYCATCH_CONSOLE) && !consolekeys[key])
	|| (in_keyCatchers == 0 && ( !con_forcedup || !consolekeys[key] ) ) )
	{
		kb = keybindings[key];
		if (kb)
		{
			if (kb[0] == '+')
			{	// button commands add keynum as a parm
				sprintf (cmd, "%s %i\n", kb, key);
				Cbuf_AddText (cmd);
			}
			else
			{
				Cbuf_AddText (kb);
				Cbuf_AddText ("\n");
			}
		}
		return;
	}

	if (!down)
		return;		// other systems only care about key down events

	if (shift_down)
	{
		key = keyshift[key];
	}

	if (in_keyCatchers & KEYCATCH_MESSAGE)
	{
		Key_Message (key);
	}
	else if (in_keyCatchers & KEYCATCH_UI)
	{
		M_Keydown (key);
	}
	else
	{
		Key_Console (key);
	}
}


/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates (void)
{
	int		i;

	for (i=0 ; i<256 ; i++)
	{
		keydown[i] = false;
		key_repeats[i] = 0;
	}
}


void IN_ProcessEvents()
{
	for (sysEvent_t ev = Sys_SharedGetEvent(); ev.evType; ev = Sys_SharedGetEvent())
	{
		switch (ev.evType)
		{
		case SE_KEY:
			Key_Event(ev.evValue, ev.evValue2);
			break;
		case SE_MOUSE:
			CL_MouseEvent(ev.evValue, ev.evValue2);
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
