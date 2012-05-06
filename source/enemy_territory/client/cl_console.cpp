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

// console.c

#include "client.h"


int g_console_field_width = 78;

Cvar* con_debug;
Cvar* con_conspeed;
Cvar* con_notifytime;
Cvar* con_autoclear;

// DHM - Nerve :: Must hold CTRL + SHIFT + ~ to get console
Cvar* con_restricted;

vec4_t console_color = {1.0, 1.0, 1.0, 1.0};


/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f(void)
{
	con.acLength = 0;

	if (con_restricted->integer && (!keys[K_CTRL].down || !keys[K_SHIFT].down))
	{
		return;
	}

	// ydnar: persistent console input is more useful
	// Arnout: added cvar
	if (con_autoclear->integer)
	{
		Field_Clear(&g_consoleField);
	}

	g_consoleField.widthInChars = g_console_field_width;

	Con_ClearNotify();

	// ydnar: multiple console size support
	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		in_keyCatchers &= ~KEYCATCH_CONSOLE;
		con.desiredFrac = 0.0;
	}
	else
	{
		in_keyCatchers |= KEYCATCH_CONSOLE;

		// short console
		if (keys[K_CTRL].down)
		{
			con.desiredFrac = (5.0 * SMALLCHAR_HEIGHT) / cls.glconfig.vidHeight;
		}
		// full console
		else if (keys[K_ALT].down)
		{
			con.desiredFrac = 1.0;
		}
		// normal half-screen console
		else
		{
			con.desiredFrac = 0.5;
		}
	}
}

/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f(void)
{
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
	chat_team = qfalse;
	chat_buddy = qtrue;
	Field_Clear(&chatField);
	chatField.widthInChars = 26;
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
		strcat(buffer, "\n");
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

	con_notifytime = Cvar_Get("con_notifytime", "7", 0);	// JPW NERVE increased per id req for obits
	con_conspeed = Cvar_Get("scr_conspeed", "3", 0);
	con_debug = Cvar_Get("con_debug", "0", CVAR_ARCHIVE);		//----(SA)	added
	con_autoclear = Cvar_Get("con_autoclear", "1", CVAR_ARCHIVE);
	con_restricted = Cvar_Get("con_restricted", "0", CVAR_INIT);			// DHM - Nerve

	Field_Clear(&g_consoleField);
	g_consoleField.widthInChars = g_console_field_width;
	for (i = 0; i < COMMAND_HISTORY; i++)
	{
		Field_Clear(&historyEditLines[i]);
		historyEditLines[i].widthInChars = g_console_field_width;
	}

	Cmd_AddCommand("toggleConsole", Con_ToggleConsole_f);
	Cmd_AddCommand("clear", Con_Clear_f);
	Cmd_AddCommand("condump", Con_Dump_f);

	// ydnar: these are deprecated in favor of cgame/ui based version
	Cmd_AddCommand("clMessageMode", Con_MessageMode_f);
	Cmd_AddCommand("clMessageMode2", Con_MessageMode2_f);
	Cmd_AddCommand("clMessageMode3", Con_MessageMode3_f);
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

		if (cl.et_snap.ps.pm_type != PM_INTERMISSION && in_keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME))
		{
			continue;
		}

		for (x = 0; x < con.linewidth; x++)
		{
			if ((text[x] & 0xff) == ' ')
			{
				continue;
			}
			if (((text[x] >> 8) & COLOR_BITS) != currentColor)
			{
				currentColor = (text[x] >> 8) & COLOR_BITS;
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
			char buf[128];
			CL_TranslateString("say_team:", buf);
			SCR_DrawBigString(8, v, buf, 1.0f);
			skip = String::Length(buf) + 2;
		}
		else if (chat_buddy)
		{
			char buf[128];
			CL_TranslateString("say_fireteam:", buf);
			SCR_DrawBigString(8, v, buf, 1.0f);
			skip = String::Length(buf) + 2;
		}
		else
		{
			char buf[128];
			CL_TranslateString("say:", buf);
			SCR_DrawBigString(8, v, buf, 1.0f);
			skip = String::Length(buf) + 1;
		}

		Field_BigDraw(&chatField, skip * BIGCHAR_WIDTH, v, true);

		v += BIGCHAR_HEIGHT;
	}

}

extern Cvar* con_drawnotify;

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
		if (cls.state == CA_ACTIVE && con_drawnotify->integer)
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
	// ydnar: added short console support (via shift+~)
	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		con.finalFrac = con.desiredFrac;
	}
	else
	{
		con.finalFrac = 0;	// none visible

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
