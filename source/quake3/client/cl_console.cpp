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
// console.c

#include "client.h"


int g_console_field_width = 78;

Cvar* con_conspeed;
Cvar* con_notifytime;

vec4_t console_color = {1.0, 1.0, 1.0, 1.0};


/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f(void)
{
	// closing a full screen console restarts the demo loop
	if (cls.state == CA_DISCONNECTED && in_keyCatchers == KEYCATCH_CONSOLE)
	{
		CL_StartDemoLoop();
		return;
	}

	Field_Clear(&g_consoleField);
	g_consoleField.widthInChars = g_console_field_width;

	Con_ClearNotify();
	in_keyCatchers ^= KEYCATCH_CONSOLE;
}

/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f(void)
{
	chat_playerNum = -1;
	chat_team = qfalse;
	Field_Clear(&chatField);
	chatField.widthInChars = 30;

	in_keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f(void)
{
	chat_playerNum = -1;
	chat_team = qtrue;
	Field_Clear(&chatField);
	chatField.widthInChars = 25;
	in_keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode3_f
================
*/
void Con_MessageMode3_f(void)
{
	chat_playerNum = VM_Call(cgvm, CG_CROSSHAIR_PLAYER);
	if (chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS_Q3)
	{
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	Field_Clear(&chatField);
	chatField.widthInChars = 30;
	in_keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode4_f
================
*/
void Con_MessageMode4_f(void)
{
	chat_playerNum = VM_Call(cgvm, CG_LAST_ATTACKER);
	if (chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS_Q3)
	{
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	Field_Clear(&chatField);
	chatField.widthInChars = 30;
	in_keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump_f(void)
{
	int l, x, i;
	short* line;
	fileHandle_t f;
	char buffer[1024];

	if (Cmd_Argc() != 2)
	{
		Com_Printf("usage: condump <filename>\n");
		return;
	}

	Com_Printf("Dumped console text to %s.\n", Cmd_Argv(1));

	f = FS_FOpenFileWrite(Cmd_Argv(1));
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
			if ((line[x] & 0xff) != ' ')
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
		for (i = 0; i < con.linewidth; i++)
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
		String::Cat(buffer, sizeof(buffer), "\n");
		FS_Write(buffer, String::Length(buffer), f);
	}

	FS_FCloseFile(f);
}




/*
================
Con_Init
================
*/
void Con_Init(void)
{
	int i;

	con_notifytime = Cvar_Get("con_notifytime", "3", 0);
	con_conspeed = Cvar_Get("scr_conspeed", "3", 0);

	Field_Clear(&g_consoleField);
	g_consoleField.widthInChars = g_console_field_width;
	for (i = 0; i < COMMAND_HISTORY; i++)
	{
		Field_Clear(&historyEditLines[i]);
		historyEditLines[i].widthInChars = g_console_field_width;
	}

	Cmd_AddCommand("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand("messagemode", Con_MessageMode_f);
	Cmd_AddCommand("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand("messagemode3", Con_MessageMode3_f);
	Cmd_AddCommand("messagemode4", Con_MessageMode4_f);
	Cmd_AddCommand("clear", Con_Clear_f);
	Cmd_AddCommand("condump", Con_Dump_f);
}

/*
================
CL_ConsolePrint

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
void CL_ConsolePrint(const char* txt)
{
	int mask = 0;

	CL_ConsolePrintCommon(txt, mask);
}


/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

Draw the editline after a ] prompt
================
*/
void Con_DrawInput(void)
{
	int y;

	if (cls.state != CA_DISCONNECTED && !(in_keyCatchers & KEYCATCH_CONSOLE))
	{
		return;
	}

	y = con.vislines - (SMALLCHAR_HEIGHT * 2);

	R_SetColor(con.color);

	SCR_DrawSmallChar(con.xadjust + 1 * SMALLCHAR_WIDTH, y, ']');

	Field_Draw(&g_consoleField, con.xadjust + 2 * SMALLCHAR_WIDTH, y,
		SCREEN_WIDTH - 3 * SMALLCHAR_WIDTH, qtrue);
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
	int skip;
	int currentColor;

	currentColor = 7;
	R_SetColor(g_color_table[currentColor]);

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

		if (cl.q3_snap.ps.pm_type != PM_INTERMISSION && in_keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME))
		{
			continue;
		}

		for (x = 0; x < con.linewidth; x++)
		{
			if ((text[x] & 0xff) == ' ')
			{
				continue;
			}
			if (((text[x] >> 8) & 7) != currentColor)
			{
				currentColor = (text[x] >> 8) & 7;
				R_SetColor(g_color_table[currentColor]);
			}
			SCR_DrawSmallChar(cl_conXOffset->integer + con.xadjust + (x + 1) * SMALLCHAR_WIDTH, v, text[x] & 0xff);
		}

		v += SMALLCHAR_HEIGHT;
	}

	R_SetColor(NULL);

	if (in_keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME))
	{
		return;
	}

	// draw the chat line
	if (in_keyCatchers & KEYCATCH_MESSAGE)
	{
		if (chat_team)
		{
			SCR_DrawBigString(8, v, "say_team:", 1.0f);
			skip = 11;
		}
		else
		{
			SCR_DrawBigString(8, v, "say:", 1.0f);
			skip = 5;
		}

		Field_BigDraw(&chatField, skip * BIGCHAR_WIDTH, v,
			SCREEN_WIDTH - (skip + 1) * BIGCHAR_WIDTH, qtrue);

		v += BIGCHAR_HEIGHT;
	}

}

/*
================
Con_DrawSolidConsole

Draws the console with the solid background
================
*/
void Con_DrawSolidConsole(float frac)
{
	int i, x, y;
	int rows;
	short* text;
	int row;
	int lines;
//	qhandle_t		conShader;
	int currentColor;
	vec4_t color;

	lines = cls.glconfig.vidHeight * frac;
	if (lines <= 0)
	{
		return;
	}

	if (lines > cls.glconfig.vidHeight)
	{
		lines = cls.glconfig.vidHeight;
	}

	// on wide screens, we will center the text
	con.xadjust = 0;
	UI_AdjustFromVirtualScreen(&con.xadjust, NULL, NULL, NULL);

	// draw the background
	y = frac * SCREEN_HEIGHT - 2;
	if (y < 1)
	{
		y = 0;
	}
	else
	{
		SCR_DrawPic(0, 0, SCREEN_WIDTH, y, cls.consoleShader);
	}

	color[0] = 1;
	color[1] = 0;
	color[2] = 0;
	color[3] = 1;
	SCR_FillRect(0, y, SCREEN_WIDTH, 2, color);


	// draw the version number

	R_SetColor(g_color_table[ColorIndex(COLOR_RED)]);

	i = String::Length(Q3_VERSION);

	for (x = 0; x < i; x++)
	{

		SCR_DrawSmallChar(cls.glconfig.vidWidth - (i - x) * SMALLCHAR_WIDTH,

			(lines - (SMALLCHAR_HEIGHT + SMALLCHAR_HEIGHT / 2)), Q3_VERSION[x]);

	}


	// draw the text
	con.vislines = lines;
	rows = (lines - SMALLCHAR_WIDTH) / SMALLCHAR_WIDTH;		// rows of text to draw

	y = lines - (SMALLCHAR_HEIGHT * 3);

	// draw from the bottom up
	if (con.display != con.current)
	{
		// draw arrows to show the buffer is backscrolled
		R_SetColor(g_color_table[ColorIndex(COLOR_RED)]);
		for (x = 0; x < con.linewidth; x += 4)
			SCR_DrawSmallChar(con.xadjust + (x + 1) * SMALLCHAR_WIDTH, y, '^');
		y -= SMALLCHAR_HEIGHT;
		rows--;
	}

	row = con.display;

	if (con.x == 0)
	{
		row--;
	}

	currentColor = 7;
	R_SetColor(g_color_table[currentColor]);

	for (i = 0; i < rows; i++, y -= SMALLCHAR_HEIGHT, row--)
	{
		if (row < 0)
		{
			break;
		}
		if (con.current - row >= con.totallines)
		{
			// past scrollback wrap point
			continue;
		}

		text = con.text + (row % con.totallines) * con.linewidth;

		for (x = 0; x < con.linewidth; x++)
		{
			if ((text[x] & 0xff) == ' ')
			{
				continue;
			}

			if (((text[x] >> 8) & 7) != currentColor)
			{
				currentColor = (text[x] >> 8) & 7;
				R_SetColor(g_color_table[currentColor]);
			}
			SCR_DrawSmallChar(con.xadjust + (x + 1) * SMALLCHAR_WIDTH, y, text[x] & 0xff);
		}
	}

	// draw the input prompt, user text, and cursor if desired
	Con_DrawInput();

	R_SetColor(NULL);
}



/*
==================
Con_DrawConsole
==================
*/
void Con_DrawConsole(void)
{
	// check for console width changes from a vid mode change
	Con_CheckResize();

	// if disconnected, render console full screen
	if (cls.state == CA_DISCONNECTED)
	{
		if (!(in_keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME)))
		{
			Con_DrawSolidConsole(1.0);
			return;
		}
	}

	if (con.displayFrac)
	{
		Con_DrawSolidConsole(con.displayFrac);
	}
	else
	{
		// draw notify lines
		if (cls.state == CA_ACTIVE)
		{
			Con_DrawNotify();
		}
	}
}

//================================================================

/*
==================
Con_RunConsole

Scroll it up or down
==================
*/
void Con_RunConsole(void)
{
	// decide on the destination height of the console
	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		con.finalFrac = 0.5;		// half screen
	}
	else
	{
		con.finalFrac = 0;				// none visible

	}
	// scroll towards the destination height
	if (con.finalFrac < con.displayFrac)
	{
		con.displayFrac -= con_conspeed->value * cls.realFrametime * 0.001;
		if (con.finalFrac > con.displayFrac)
		{
			con.displayFrac = con.finalFrac;
		}

	}
	else if (con.finalFrac > con.displayFrac)
	{
		con.displayFrac += con_conspeed->value * cls.realFrametime * 0.001;
		if (con.finalFrac < con.displayFrac)
		{
			con.displayFrac = con.finalFrac;
		}
	}

}

void Con_Close(void)
{
	if (!com_cl_running->integer)
	{
		return;
	}
	Field_Clear(&g_consoleField);
	Con_ClearNotify();
	in_keyCatchers &= ~KEYCATCH_CONSOLE;
	con.finalFrac = 0;				// none visible
	con.displayFrac = 0;
}
