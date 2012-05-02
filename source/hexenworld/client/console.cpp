// console.c

#include "quakedef.h"

Cvar* con_notifytime;

qboolean con_debuglog;

extern int edit_line;


void Key_ClearTyping(void)
{
	g_consoleField.buffer[0] = 0;	// clear any typing
}

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f(void)
{
	Key_ClearTyping();

	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		if (cls.state == CA_ACTIVE)
		{
			in_keyCatchers &= ~KEYCATCH_CONSOLE;
		}
	}
	else
	{
		in_keyCatchers |= KEYCATCH_CONSOLE;
	}

	Con_ClearNotify();
}

/*
================
Con_ToggleChat_f
================
*/
void Con_ToggleChat_f(void)
{
	Key_ClearTyping();

	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		if (cls.state == CA_ACTIVE)
		{
			in_keyCatchers &= ~KEYCATCH_CONSOLE;
		}
	}
	else
	{
		in_keyCatchers |= KEYCATCH_CONSOLE;
	}

	Con_ClearNotify();
}

/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f(void)
{
	chat_team = false;
	in_keyCatchers |= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f(void)
{
	chat_team = true;
	in_keyCatchers |= KEYCATCH_MESSAGE;
}

/*
================
Con_Init
================
*/
void Con_Init(void)
{
	con_debuglog = COM_CheckParm("-condebug");

	con.cursorspeed = 4;

	Con_Printf("Console initialized.\n");

//
// register our commands
//
	con_notifytime = Cvar_Get("con_notifytime","3", 0);		//seconds

	Cmd_AddCommand("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand("togglechat", Con_ToggleChat_f);
	Cmd_AddCommand("messagemode", Con_MessageMode_f);
	Cmd_AddCommand("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand("clear", Con_Clear_f);
}

/*
================
Con_Print

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the notify window will pop up.
================
*/
void Con_Print(const char* txt)
{
	int mask;

	if (txt[0] == 1 || txt[0] == 2)
	{
		mask = 128 << 8;		// go to colored text
		txt++;
	}
	else
	{
		mask = 0;
	}

	CL_ConsolePrintCommon(txt, mask);
}


static void Con_DebugLog(const char* file, const char* fmt, ...)
{
	va_list argptr;
	static char data[1024];
	fileHandle_t fd;

	va_start(argptr, fmt);
	Q_vsnprintf(data, sizeof(data), fmt, argptr);
	va_end(argptr);
	FS_FOpenFileByMode(file, &fd, FS_APPEND);
	FS_Write(data, String::Length(data), fd);
	FS_FCloseFile(fd);
}

/*
================
Con_Printf

Handles cursor positioning, line wrapping, etc
================
*/
void Con_Printf(const char* fmt, ...)
{
	va_list argptr;
	char msg[MAXPRINTMSG];
	static qboolean inupdate;

	va_start(argptr,fmt);
	Q_vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end(argptr);

// also echo to debugging console
	Sys_Print(msg);	// also echo to debugging console

// log all messages to file
	if (con_debuglog)
	{
		Con_DebugLog("qconsole.log", "%s", msg);
	}

// write it to the scrollable buffer
	Con_Print(msg);

// update the screen immediately if the console is displayed
	if (cls.state != CA_ACTIVE)
	{
		// protect against infinite loop if something in SCR_UpdateScreen calls
		// Con_Printd
		if (!inupdate)
		{
			inupdate = true;
			SCR_UpdateScreen();
			inupdate = false;
		}
	}
}

/*
================
Con_DPrintf

A Con_Printf that only shows up if the "developer" cvar is set
================
*/
void Con_DPrintf(const char* fmt, ...)
{
	va_list argptr;
	char msg[MAXPRINTMSG];

	if (!com_developer || !com_developer->value)
	{
		return;			// don't confuse non-developers with techie stuff...

	}
	va_start(argptr,fmt);
	Q_vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end(argptr);

	Con_Printf("%s", msg);
}

/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

The input line scrolls horizontally if typing goes beyond the right edge
================
*/
void Con_DrawInput(void)
{
	int y;
	int i;
	char buffer[MAX_EDIT_LINE + 1];
	buffer[0] = ']';
	String::Cpy(buffer + 1, g_consoleField.buffer);
	int key_linepos = String::Length(buffer);
	char* text;

	if (!(in_keyCatchers & KEYCATCH_CONSOLE) && cls.state == CA_ACTIVE)
	{
		return;		// don't draw anything (allways draw if not active)

	}
	text = buffer;

// add the cursor frame
	text[key_linepos] = 10 + ((int)(realtime * con.cursorspeed) & 1);

// fill out remainder with spaces
	for (i = key_linepos + 1; i < con.linewidth; i++)
		text[i] = ' ';

//	prestep if horizontally scrolling
	if (key_linepos >= con.linewidth)
	{
		text += 1 + key_linepos - con.linewidth;
	}

// draw it
	y = con.vislines - 22;

	for (i = 0; i < con.linewidth; i++)
		Draw_Character((i + 1) << 3, con.vislines - 22, text[i]);

// remove cursor
	buffer[key_linepos] = 0;
}


/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify(void)
{
	int x, v;
	short* text;
	int i;
	char* s;
	int skip;

	v = 0;
	for (i = con.current - NUM_CON_TIMES + 1; i <= con.current; i++)
	{
		if (i < 0)
		{
			continue;
		}
		int time = con.times[i % NUM_CON_TIMES];
		if (time == 0)
		{
			continue;
		}
		time = realtime * 1000 - time;
		if (time > con_notifytime->value * 1000)
		{
			continue;
		}
		text = con.text + (i % con.totallines) * con.linewidth;

		for (x = 0; x < con.linewidth; x++)
			Draw_Character((x + 1) << 3, v, text[x]);

		v += 8;
	}


	if (in_keyCatchers & KEYCATCH_MESSAGE)
	{
		if (chat_team)
		{
			Draw_String(8, v, "say_team:");
			skip = 11;
		}
		else
		{
			Draw_String(8, v, "say:");
			skip = 5;
		}

		s = chat_buffer;
		if (chat_bufferlen > ((int)viddef.width >> 3) - (skip + 1))
		{
			s += chat_bufferlen - ((viddef.width >> 3) - (skip + 1));
		}
		x = 0;
		while (s[x])
		{
			Draw_Character((x + skip) << 3, v, s[x]);
			x++;
		}
		Draw_Character((x + skip) << 3, v, 10 + ((int)(realtime * con.cursorspeed) & 1));
		v += 8;
	}
}

/*
================
Con_DrawConsole

Draws the console with the solid background
================
*/
void Con_DrawConsole(int lines)
{
	int i, j, x, y, n;
	int rows;
	int row;
	char dlbar[1024];

	if (lines <= 0)
	{
		return;
	}

// draw the background
	Draw_ConsoleBackground(lines);

// draw the text
	con.vislines = lines;

// changed to line things up better
	rows = (lines - 22) >> 3;		// rows of text to draw

	y = lines - 30;

// draw from the bottom up
	if (con.display != con.current)
	{
		// draw arrows to show the buffer is backscrolled
		for (x = 0; x < con.linewidth; x += 4)
			Draw_Character((x + 1) << 3, y, '^');

		y -= 8;
		rows--;
	}

	row = con.display;
	for (i = 0; i < rows; i++, y -= 8, row--)
	{
		if (row < 0)
		{
			break;
		}
		if (con.current - row >= con.totallines)
		{
			break;		// past scrollback wrap point

		}
		short* text = con.text + (row % con.totallines) * con.linewidth;

		for (x = 0; x < con.linewidth; x++)
			Draw_Character((x + 1) << 3, y, text[x]);
	}

	// draw the download bar
	// figure out width
	if (clc.download)
	{
		char* text = String::RChr(clc.downloadName, '/');
		if (text != NULL)
		{
			text++;
		}
		else
		{
			text = clc.downloadName;
		}

		x = con.linewidth - ((con.linewidth * 7) / 40);
		y = x - String::Length(text) - 8;
		i = con.linewidth / 3;
		if (String::Length(text) > i)
		{
			y = x - i - 11;
			String::NCpy(dlbar, text, i);
			dlbar[i] = 0;
			String::Cat(dlbar, sizeof(dlbar), "...");
		}
		else
		{
			String::Cpy(dlbar, text);
		}
		String::Cat(dlbar, sizeof(dlbar), ": ");
		i = String::Length(dlbar);
		dlbar[i++] = '\x80';
		// where's the dot go?
		if (clc.downloadPercent == 0)
		{
			n = 0;
		}
		else
		{
			n = y * clc.downloadPercent / 100;
		}

		for (j = 0; j < y; j++)
			if (j == n)
			{
				dlbar[i++] = '\x83';
			}
			else
			{
				dlbar[i++] = '\x81';
			}
		dlbar[i++] = '\x82';
		dlbar[i] = 0;

		sprintf(dlbar + String::Length(dlbar), " %02d%%", clc.downloadPercent);

		// draw it
		y = con.vislines - 22 + 8;
		for (i = 0; i < String::Length(dlbar); i++)
			Draw_Character((i + 1) << 3, y, dlbar[i]);
	}


// draw the input prompt, user text, and cursor if desired
	Con_DrawInput();
}


/*
==================
Con_NotifyBox
==================
*/
void Con_NotifyBox(const char* text)
{
	double t1, t2;

// during startup for sound / cd warnings
	Con_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n");

	Con_Printf(text);

	Con_Printf("Press a key.\n");
	Con_Printf("\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n");

	key_count = -2;		// wait for a key down and up
	in_keyCatchers |= KEYCATCH_CONSOLE;

	do
	{
		t1 = Sys_DoubleTime();
		SCR_UpdateScreen();
		Sys_SendKeyEvents();
		IN_ProcessEvents();
		t2 = Sys_DoubleTime();
		realtime += t2 - t1;		// make the cursor blink
	}
	while (key_count < 0);

	Con_Printf("\n");
	in_keyCatchers &= ~KEYCATCH_CONSOLE;
	realtime = 0;				// put the cursor back to invisible
}


/*
==================
Con_SafePrintf

Okay to call even when the screen can't be updated
==================
*/
void Con_SafePrintf(const char* fmt, ...)
{
	va_list argptr;
	char msg[MAXPRINTMSG];
	int temp;

	va_start(argptr,fmt);
	Q_vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end(argptr);

	temp = cls.disable_screen;
	cls.disable_screen = true;
	Con_Printf("%s", msg);
	cls.disable_screen = temp;
}
