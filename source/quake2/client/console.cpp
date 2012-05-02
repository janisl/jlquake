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
// console.c

#include "client.h"

Cvar* con_notifytime;

void DrawString(int x, int y, const char* s)
{
	while (*s)
	{
		Draw_Char(x, y, *s);
		x += 8;
		s++;
	}
}

void DrawAltString(int x, int y, const char* s)
{
	while (*s)
	{
		Draw_Char(x, y, *s ^ 0x80);
		x += 8;
		s++;
	}
}


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
	SCR_EndLoadingPlaque();		// get rid of loading plaque

	if (cl.q2_attractloop)
	{
		Cbuf_AddText("killserver\n");
		return;
	}

	if (cls.state == CA_DISCONNECTED)
	{	// start the demo loop again
		Cbuf_AddText("d1\n");
		return;
	}

	Key_ClearTyping();
	Con_ClearNotify();

	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		in_keyCatchers &= ~KEYCATCH_CONSOLE;
		M_ForceMenuOff();
		Cvar_SetLatched("paused", "0");
	}
	else
	{
		M_ForceMenuOff();
		in_keyCatchers |= KEYCATCH_CONSOLE;

		if (Cvar_VariableValue("maxclients") == 1 &&
			Com_ServerState())
		{
			Cvar_SetLatched("paused", "1");
		}
	}
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
			M_ForceMenuOff();
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
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump_f(void)
{
	int l, x;
	short* line;
	fileHandle_t f;
	char buffer[1024];
	char name[MAX_OSPATH];

	if (Cmd_Argc() != 2)
	{
		Com_Printf("usage: condump <filename>\n");
		return;
	}

	String::Sprintf(name, sizeof(name), "%s.txt", Cmd_Argv(1));

	Com_Printf("Dumped console text to %s.\n", name);
	f = FS_FOpenFileWrite(name);
	if (!f)
	{
		Com_Printf("ERROR: couldn't open.\n");
		return;
	}

	// skip empty lines
	for (l = con.current - con.totallines + 1; l <= con.current; l++)
	{
		line = con.text + (l % con.totallines) * con.linewidth;
		for (x = 0; x < con.linewidth; x++)
			if (line[x] != ' ')
			{
				break;
			}
		if (x != con.linewidth)
		{
			break;
		}
	}

	// write the remaining lines
	buffer[con.linewidth] = 0;
	for (; l <= con.current; l++)
	{
		line = con.text + (l % con.totallines) * con.linewidth;
		for (int i = 0; i < con.linewidth; i++)
			buffer[i] = line[i] & 0xff;
		for (x = con.linewidth - 1; x >= 0; x--)
		{
			if (buffer[x] == ' ')
			{
				buffer[x] = 0;
			}
			else
			{
				break;
			}
		}
		for (x = 0; buffer[x]; x++)
			buffer[x] &= 0x7f;

		FS_Printf(f, "%s\n", buffer);
	}

	FS_FCloseFile(f);
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
	Com_Printf("Console initialized.\n");

//
// register our commands
//
	con_notifytime = Cvar_Get("con_notifytime", "3", 0);

	Cmd_AddCommand("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand("togglechat", Con_ToggleChat_f);
	Cmd_AddCommand("messagemode", Con_MessageMode_f);
	Cmd_AddCommand("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand("clear", Con_Clear_f);
	Cmd_AddCommand("condump", Con_Dump_f);
}

/*
================
Con_Print

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
void Con_Print(const char* txt)
{
	int mask;

	if (txt[0] == 1 || txt[0] == 2)
	{
		mask = 128;		// go to colored text
		txt++;
	}
	else
	{
		mask = 0;
	}

	CL_ConsolePrintCommon(txt, mask);
}


/*
==============
Con_CenteredPrint
==============
*/
void Con_CenteredPrint(char* text)
{
	int l;
	char buffer[1024];

	l = String::Length(text);
	l = (con.linewidth - l) / 2;
	if (l < 0)
	{
		l = 0;
	}
	Com_Memset(buffer, ' ', l);
	String::Cpy(buffer + l, text);
	String::Cat(buffer, sizeof(buffer), "\n");
	Con_Print(buffer);
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

	if (in_keyCatchers & KEYCATCH_UI)
	{
		return;
	}
	if (!(in_keyCatchers & KEYCATCH_CONSOLE) && cls.state == CA_ACTIVE)
	{
		return;		// don't draw anything (always draw if not active)

	}
	text = buffer;

// add the cursor frame
	text[key_linepos] = 10 + ((int)(cls.realtime >> 8) & 1);

// fill out remainder with spaces
	for (i = key_linepos + 1; i < con.linewidth; i++)
		text[i] = ' ';

//	prestep if horizontally scrolling
	if (key_linepos >= con.linewidth)
	{
		text += 1 + key_linepos - con.linewidth;
	}

// draw it
	y = con.vislines - 16;

	for (i = 0; i < con.linewidth; i++)
		Draw_Char((i + 1) << 3, con.vislines - 22, text[i]);

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
	int time;
	char* s;
	int skip;

	v = 0;
	for (i = con.current - NUM_CON_TIMES + 1; i <= con.current; i++)
	{
		if (i < 0)
		{
			continue;
		}
		time = con.times[i % NUM_CON_TIMES];
		if (time == 0)
		{
			continue;
		}
		time = cls.realtime - time;
		if (time > con_notifytime->value * 1000)
		{
			continue;
		}
		text = con.text + (i % con.totallines) * con.linewidth;

		for (x = 0; x < con.linewidth; x++)
			Draw_Char((x + 1) << 3, v, text[x] & 0xff);

		v += 8;
	}


	if (in_keyCatchers & KEYCATCH_MESSAGE)
	{
		if (chat_team)
		{
			DrawString(8, v, "say_team:");
			skip = 11;
		}
		else
		{
			DrawString(8, v, "say:");
			skip = 5;
		}

		s = chat_buffer;
		if (chat_bufferlen > (viddef.width >> 3) - (skip + 1))
		{
			s += chat_bufferlen - ((viddef.width >> 3) - (skip + 1));
		}
		x = 0;
		while (s[x])
		{
			Draw_Char((x + skip) << 3, v, s[x]);
			x++;
		}
		Draw_Char((x + skip) << 3, v, 10 + ((cls.realtime >> 8) & 1));
		v += 8;
	}
}

/*
================
Con_DrawConsole

Draws the console with the solid background
================
*/
void Con_DrawConsole(float frac)
{
	int i, j, x, y, n;
	int rows;
	int row;
	int lines;
	char version[64];
	char dlbar[1024];

	lines = viddef.height * frac;
	if (lines <= 0)
	{
		return;
	}

	if (lines > viddef.height)
	{
		lines = viddef.height;
	}

// draw the background
	UI_DrawStretchNamedPic(0, -viddef.height + lines, viddef.width, viddef.height, "conback");

	String::Sprintf(version, sizeof(version), "v%4.2f", VERSION);
	for (x = 0; x < 5; x++)
		Draw_Char(viddef.width - 44 + x * 8, lines - 12, 128 + version[x]);

// draw the text
	con.vislines = lines;

#if 0
	rows = (lines - 8) >> 3;		// rows of text to draw

	y = lines - 24;
#else
	rows = (lines - 22) >> 3;		// rows of text to draw

	y = lines - 30;
#endif

// draw from the bottom up
	if (con.display != con.current)
	{
		// draw arrows to show the buffer is backscrolled
		for (x = 0; x < con.linewidth; x += 4)
			Draw_Char((x + 1) << 3, y, '^');

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
			Draw_Char((x + 1) << 3, y, text[x] & 0xff);
	}

//ZOID
	// draw the download bar
	// figure out width
	if (clc.download)
	{
		char* text = String::RChr(clc.downloadName, '/');
		if (text)
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
		y = con.vislines - 12;
		for (i = 0; i < String::Length(dlbar); i++)
			Draw_Char((i + 1) << 3, y, dlbar[i]);
	}
//ZOID

// draw the input prompt, user text, and cursor if desired
	Con_DrawInput();
}
