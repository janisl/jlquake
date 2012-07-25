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
	chat_playerNum = VM_Call(cgvm, CG_CROSSHAIR_PLAYER);
	if (chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS_WS)
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
	chat_playerNum = VM_Call(cgvm, CG_LAST_ATTACKER);
	if (chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS_WS)
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
