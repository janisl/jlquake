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

// console.c

#include "client.h"


vec4_t console_color = {1.0, 1.0, 1.0, 1.0};


/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f(void)
{
	con.acLength = 0;

	// closing a full screen console restarts the demo loop
	if (cls.state == CA_DISCONNECTED && in_keyCatchers == KEYCATCH_CONSOLE)
	{
		CL_StartDemoLoop();
		return;
	}

	Field_Clear(&g_consoleField);

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
	if (chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS_WM)
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
	if (chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS_WM)
	{
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
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
	chat_limbo = qtrue;
}

/*
================
Con_StopLimboMode_f
================
*/
void Con_StopLimboMode_f(void)
{
	chat_limbo = qfalse;
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
	Con_InitCommon();

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
