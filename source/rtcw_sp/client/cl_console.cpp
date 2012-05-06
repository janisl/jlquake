/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// console.c

#include "client.h"


int g_console_field_width = 78;


Cvar* con_debug;
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
//	chat_limbo = qfalse;		// NERVE - SMF
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
//	chat_limbo = qfalse;		// NERVE - SMF
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
	if (chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS_WS)
	{
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
//	chat_limbo = qfalse;		// NERVE - SMF
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
	if (chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS_WS)
	{
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
//	chat_limbo = qfalse;		// NERVE - SMF
	Field_Clear(&chatField);
	chatField.widthInChars = 30;
	in_keyCatchers ^= KEYCATCH_MESSAGE;
}

// NERVE - SMF
/*
================
Con_StartLimboMode_f
================
*/
void Con_StartLimboMode_f(void)
{
//	chat_playerNum = -1;
//	chat_team = qfalse;
	chat_limbo = qtrue;		// NERVE - SMF
//	Field_Clear( &chatField );
//	chatField.widthInChars = 30;

//	in_keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_StopLimboMode_f
================
*/
void Con_StopLimboMode_f(void)
{
//	chat_playerNum = -1;
//	chat_team = qfalse;
	chat_limbo = qfalse;		// NERVE - SMF
//	Field_Clear( &chatField );
//	chatField.widthInChars = 30;

//	in_keyCatchers &= ~KEYCATCH_MESSAGE;
}
// -NERVE - SMF

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

#ifdef __MACOS__	//DAJ MacOS file typing
	{
		extern _MSL_IMP_EXP_C long _fcreator, _ftype;
		_ftype = 'TEXT';
		_fcreator = 'R*ch';
	}
#endif
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

	con_notifytime = Cvar_Get("con_notifytime", "3", 0);
	con_conspeed = Cvar_Get("scr_conspeed", "3", 0);
	con_debug = Cvar_Get("con_debug", "0", CVAR_ARCHIVE);	//----(SA)	added

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
	Cmd_AddCommand("startLimboMode", Con_StartLimboMode_f);			// NERVE - SMF
	Cmd_AddCommand("stopLimboMode", Con_StopLimboMode_f);				// NERVE - SMF
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

		if (cl.ws_snap.ps.pm_type != PM_INTERMISSION && in_keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME))
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

		Field_BigDraw(&chatField, skip * BIGCHAR_WIDTH, v, true);

		v += BIGCHAR_HEIGHT;
	}

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
	switch (cls.state)
	{
	case CA_UNINITIALIZED:
	case CA_CONNECTING:			// sending request packets to the server
	case CA_CHALLENGING:		// sending challenge packets to the server
	case CA_CONNECTED:			// netchan_t established, getting gamestate
	case CA_PRIMED:				// got gamestate, waiting for first frame
	case CA_LOADING:			// only during cgame initialization, never during main loop
		if (!con_debug->integer)	// these are all 'no console at all' when con_debug is not set
		{
			return;
		}

		if (in_keyCatchers & KEYCATCH_UI)
		{
			return;
		}

		Con_DrawSolidConsole(1.0);
		return;

	case CA_DISCONNECTED:		// not talking to a server
		if (!(in_keyCatchers & KEYCATCH_UI))
		{
			Con_DrawSolidConsole(1.0);
			return;
		}
		break;

	case CA_ACTIVE:				// game views should be displayed
		if (con.displayFrac)
		{
			if (con_debug->integer == 2)		// 2 means draw full screen console at '~'
			{	//					Con_DrawSolidConsole( 1.0f );
				Con_DrawSolidConsole(con.displayFrac * 2.0f);
				return;
			}
		}

		break;


	case CA_CINEMATIC:			// playing a cinematic or a static pic, not connected to a server
	default:
		break;
	}

	if (con.displayFrac)
	{
		Con_DrawSolidConsole(con.displayFrac);
	}
	else
	{
		Con_DrawNotify();		// draw notify lines
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
