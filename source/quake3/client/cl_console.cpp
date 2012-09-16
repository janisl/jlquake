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
	con.desiredFrac = 0.5;
}

/*
================
Con_MessageMode3_f
================
*/
void Con_MessageMode3_f(void)
{
	chat_playerNum = CLT3_CrosshairPlayer();
	if (chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS_Q3)
	{
		chat_playerNum = -1;
		return;
	}
	chat_team = false;
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
	chat_playerNum = CLT3_LastAttacker();
	if (chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS_Q3)
	{
		chat_playerNum = -1;
		return;
	}
	chat_team = false;
	Field_Clear(&chatField);
	chatField.widthInChars = 30;
	in_keyCatchers ^= KEYCATCH_MESSAGE;
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
	Cmd_AddCommand("messagemode3", Con_MessageMode3_f);
	Cmd_AddCommand("messagemode4", Con_MessageMode4_f);
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
